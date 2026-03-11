#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "runtime/memory_access.h"

namespace {

using uwvm2_vulkan::runtime::GuestMemoryAccessor;

struct FakeMemoryHost {
  std::vector<std::byte> memory{};
  std::atomic_size_t dynamic_length{};
  uwvm_preload_memory_descriptor_t descriptor{};
};

FakeMemoryHost *g_host{};

std::size_t descriptor_count() noexcept { return 1u; }

bool descriptor_at(std::size_t index,
                   uwvm_preload_memory_descriptor_t *out) noexcept {
  if (index != 0u || out == nullptr || g_host == nullptr) {
    return false;
  }
  *out = g_host->descriptor;
  return true;
}

bool memory_read(std::size_t memory_index, std::uint_least64_t address,
                 void *destination, std::size_t size) noexcept {
  if (g_host == nullptr || destination == nullptr ||
      memory_index != g_host->descriptor.memory_index) {
    return false;
  }
  if (address + size > g_host->memory.size()) {
    return false;
  }
  std::memcpy(destination,
              g_host->memory.data() + static_cast<std::size_t>(address), size);
  return true;
}

bool memory_write(std::size_t memory_index, std::uint_least64_t address,
                  void const *source, std::size_t size) noexcept {
  if (g_host == nullptr || source == nullptr ||
      memory_index != g_host->descriptor.memory_index) {
    return false;
  }
  if (address + size > g_host->memory.size()) {
    return false;
  }
  std::memcpy(g_host->memory.data() + static_cast<std::size_t>(address), source,
              size);
  return true;
}

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

} // namespace

int main() {
  FakeMemoryHost host{
      .memory = std::vector<std::byte>(16u),
      .dynamic_length = 12u,
      .descriptor = uwvm_preload_memory_descriptor_t{
          .memory_index = 1u,
          .delivery_state = UWVM_PRELOAD_MEMORY_DELIVERY_COPY,
          .backend_kind = UWVM_PRELOAD_MEMORY_BACKEND_NATIVE_DEFINED,
          .reserved0 = 0u,
          .reserved1 = 0u,
          .page_count = 1u,
          .page_size_bytes = 65536u,
          .byte_length = 16u,
          .partial_protection_limit_bytes = 8u,
          .mmap_view_begin = nullptr,
          .dynamic_length_atomic_object = nullptr}};
  g_host = &host;

  uwvm_preload_host_api_v1 api{.struct_size = sizeof(uwvm_preload_host_api_v1),
                               .abi_version = 1u,
                               .memory_descriptor_count = descriptor_count,
                               .memory_descriptor_at = descriptor_at,
                               .memory_read = memory_read,
                               .memory_write = memory_write};

  GuestMemoryAccessor accessor{};
  accessor.SetHostApi(&api);

  host.memory[0] = std::byte{0x41};
  host.memory[1] = std::byte{0x42};
  std::array<std::byte, 2u> buffer{};

  require(accessor.Read(1u, 0u, buffer.data(), buffer.size()));
  require(buffer[0] == std::byte{0x41} && buffer[1] == std::byte{0x42});

  std::array<std::byte, 2u> write_buffer{std::byte{0x51}, std::byte{0x52}};
  require(accessor.Write(1u, 2u, write_buffer.data(), write_buffer.size()));
  require(host.memory[2] == std::byte{0x51} &&
          host.memory[3] == std::byte{0x52});

  host.descriptor.delivery_state =
      UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_FULL_PROTECTION;
  host.descriptor.mmap_view_begin = host.memory.data();
  std::array<std::byte, 2u> full_view{};
  require(accessor.Read(1u, 2u, full_view.data(), full_view.size()));
  require(full_view[0] == std::byte{0x51});

  host.descriptor.delivery_state =
      UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_PARTIAL_PROTECTION;
  require(accessor.Write(1u, 7u, write_buffer.data(), 1u));
  require(host.memory[7] == std::byte{0x51});
  require(accessor.Read(1u, 9u, full_view.data(), 1u));

  host.descriptor.delivery_state =
      UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_DYNAMIC_BOUNDS;
  host.descriptor.dynamic_length_atomic_object = &host.dynamic_length;
  require(accessor.Read(1u, 4u, full_view.data(), 1u));
  require(!accessor.Read(1u, 15u, full_view.data(), 2u));

  host.descriptor.delivery_state = UWVM_PRELOAD_MEMORY_DELIVERY_NONE;
  require(!accessor.Read(1u, 0u, buffer.data(), buffer.size()));

  return 0;
}
