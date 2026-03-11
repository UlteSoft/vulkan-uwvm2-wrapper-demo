#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <uwvm2_vulkan/vulkan_types.h>

#include "runtime/plugin_context.h"
#include "vulkan/api.h"

namespace {

using uwvm2_vulkan::runtime::PluginContext;
namespace native = uwvm2_vulkan::vk::native;

struct FakeMemoryHost {
  std::vector<std::byte> memory{};
  std::atomic_size_t dynamic_length{};
  uwvm_preload_memory_descriptor_t descriptor{};
};

struct DestroyBufferCall {
  native::VkDevice device{};
  native::VkBuffer buffer{};
};

struct FreeMemoryCall {
  native::VkDevice device{};
  native::VkDeviceMemory memory{};
};

FakeMemoryHost *g_host{};

std::size_t descriptor_count() noexcept { return 1u; }

bool descriptor_at(std::size_t index,
                   uwvm_preload_memory_descriptor_t *out) noexcept {
  if (g_host == nullptr || index != 0u || out == nullptr) {
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

  auto const begin{static_cast<std::size_t>(address)};
  if (begin > g_host->memory.size() || size > g_host->memory.size() - begin) {
    return false;
  }

  std::memcpy(destination, g_host->memory.data() + begin, size);
  return true;
}

bool memory_write(std::size_t memory_index, std::uint_least64_t address,
                  void const *source, std::size_t size) noexcept {
  if (g_host == nullptr || source == nullptr ||
      memory_index != g_host->descriptor.memory_index) {
    return false;
  }

  auto const begin{static_cast<std::size_t>(address)};
  if (begin > g_host->memory.size() || size > g_host->memory.size() - begin) {
    return false;
  }

  std::memcpy(g_host->memory.data() + begin, source, size);
  return true;
}

void require_impl(bool condition, int line) {
  if (!condition) {
    std::fprintf(stderr, "require failed at line %d\n", line);
    std::abort();
  }
}

#define require(condition) require_impl((condition), __LINE__)

template <typename T> void WriteGuest(std::uint64_t address, T const &value) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(sizeof(T) <= g_host->memory.size() - begin);
  std::memcpy(g_host->memory.data() + begin, &value, sizeof(T));
}

template <typename T> [[nodiscard]] T ReadGuest(std::uint64_t address) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(sizeof(T) <= g_host->memory.size() - begin);

  T value{};
  std::memcpy(&value, g_host->memory.data() + begin, sizeof(T));
  return value;
}

void WriteGuestBytes(std::uint64_t address, void const *source,
                     std::size_t size) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(size <= g_host->memory.size() - begin);
  std::memcpy(g_host->memory.data() + begin, source, size);
}

void WriteGuestString(std::uint64_t address, char const *text) {
  WriteGuestBytes(address, text, std::strlen(text));
}

[[nodiscard]] uwvm_vk_extension_property MakeExtensionProperty(
    char const *name, std::uint32_t spec_version) {
  uwvm_vk_extension_property property{};
  auto const name_length{std::strlen(name)};
  auto const copy_length{
      name_length < sizeof(property.extension_name) - 1u
          ? name_length
          : sizeof(property.extension_name) - 1u};
  std::memcpy(property.extension_name, name, copy_length);
  property.extension_name[copy_length] = '\0';
  property.spec_version = spec_version;
  return property;
}

class FakeBackend final : public uwvm2_vulkan::vk::Backend {
public:
  bool LoaderAvailable() override { return true; }

  std::int32_t EnumerateInstanceVersion(std::uint32_t &api_version) override {
    api_version = 0u;
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateInstanceExtensionProperties(
      char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override {
    (void)layer_name;
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateInstanceLayerProperties(
      std::vector<uwvm_vk_layer_property> &properties) override {
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t CreateInstance(native::VkInstanceCreateInfo const &create_info,
                              native::VkInstance &instance) override {
    (void)create_info;
    instance = reinterpret_cast<native::VkInstance>(0x1000u);
    return UWVM_VK_SUCCESS;
  }

  void DestroyInstance(native::VkInstance instance) override {
    (void)instance;
  }

  std::int32_t EnumeratePhysicalDevices(
      native::VkInstance instance,
      std::vector<native::VkPhysicalDevice> &physical_devices) override {
    (void)instance;
    physical_devices.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateDeviceExtensionProperties(
      native::VkPhysicalDevice physical_device, char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override {
    last_extension_query_physical_device = physical_device;
    last_extension_query_layer_name = layer_name == nullptr ? "" : layer_name;
    properties = device_extension_properties;
    return device_extension_query_result;
  }

  std::int32_t GetPhysicalDeviceProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_properties &properties) override {
    (void)physical_device;
    properties = {};
    return UWVM_VK_SUCCESS;
  }

  std::int32_t GetPhysicalDeviceFeatures(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_features &features) override {
    last_feature_query_physical_device = physical_device;
    features = physical_device_features;
    return physical_device_features_result;
  }

  std::int32_t GetPhysicalDeviceMemoryProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_memory_properties &properties) override {
    (void)physical_device;
    properties = {};
    return UWVM_VK_SUCCESS;
  }

  std::int32_t GetPhysicalDeviceQueueFamilyProperties(
      native::VkPhysicalDevice physical_device,
      std::vector<uwvm_vk_queue_family_properties> &properties) override {
    (void)physical_device;
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t CreateDevice(native::VkPhysicalDevice physical_device,
                            native::VkDeviceCreateInfo const &create_info,
                            native::VkDevice &device) override {
    last_create_device_physical_device = physical_device;
    last_queue_family_index =
        create_info.queueCreateInfoCount == 0u
            ? 0u
            : create_info.pQueueCreateInfos[0].queueFamilyIndex;
    last_queue_priority =
        create_info.queueCreateInfoCount == 0u ||
                create_info.pQueueCreateInfos[0].queueCount == 0u ||
                create_info.pQueueCreateInfos[0].pQueuePriorities == nullptr
            ? 0.0f
            : create_info.pQueueCreateInfos[0].pQueuePriorities[0];
    create_device_enabled_features_present =
        create_info.pEnabledFeatures != nullptr;
    if (create_info.pEnabledFeatures != nullptr) {
      std::memcpy(&captured_enabled_features, create_info.pEnabledFeatures,
                  sizeof(captured_enabled_features));
    } else {
      captured_enabled_features = {};
    }

    device = create_device_result_handle;
    return create_device_result;
  }

  void DestroyDevice(native::VkDevice device) override {
    destroyed_devices.push_back(device);
  }

  std::int32_t DeviceWaitIdle(native::VkDevice device) override {
    last_device_wait_idle_device = device;
    ++device_wait_idle_calls;
    return device_wait_idle_result;
  }

  std::int32_t GetDeviceQueue(native::VkDevice device,
                              std::uint32_t queue_family_index,
                              std::uint32_t queue_index,
                              native::VkQueue &queue) override {
    last_get_device_queue_device = device;
    last_get_device_queue_family_index = queue_family_index;
    last_get_device_queue_index = queue_index;
    queue = get_device_queue_result_handle;
    return get_device_queue_result;
  }

  std::int32_t QueueWaitIdle(native::VkDevice device,
                             native::VkQueue queue) override {
    last_queue_wait_idle_device = device;
    last_queue_wait_idle_queue = queue;
    ++queue_wait_idle_calls;
    return queue_wait_idle_result;
  }

  std::int32_t CreateBuffer(native::VkDevice device,
                            native::VkBufferCreateInfo const &create_info,
                            native::VkBuffer &buffer) override {
    (void)device;
    (void)create_info;
    buffer = 0u;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  void DestroyBuffer(native::VkDevice device, native::VkBuffer buffer) override {
    destroyed_buffers.push_back(DestroyBufferCall{device, buffer});
  }

  std::int32_t GetBufferMemoryRequirements(
      native::VkDevice device, native::VkBuffer buffer,
      uwvm_vk_memory_requirements &requirements) override {
    (void)device;
    (void)buffer;
    requirements = {};
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::int32_t AllocateMemory(
      native::VkDevice device, native::VkMemoryAllocateInfo const &allocate_info,
      native::VkDeviceMemory &memory) override {
    (void)device;
    (void)allocate_info;
    memory = 0u;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  void FreeMemory(native::VkDevice device, native::VkDeviceMemory memory) override {
    freed_memories.push_back(FreeMemoryCall{device, memory});
  }

  std::int32_t BindBufferMemory(native::VkDevice device, native::VkBuffer buffer,
                                native::VkDeviceMemory memory,
                                std::uint64_t offset) override {
    (void)device;
    (void)buffer;
    (void)memory;
    (void)offset;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::int32_t MapMemory(native::VkDevice device, native::VkDeviceMemory memory,
                         std::uint64_t offset, std::uint64_t size,
                         void *&mapped_data) override {
    (void)device;
    (void)memory;
    ++map_memory_calls;

    auto const begin{static_cast<std::size_t>(offset)};
    auto const byte_count{static_cast<std::size_t>(size)};
    if (begin > mapped_memory.size() ||
        byte_count > mapped_memory.size() - begin) {
      mapped_data = nullptr;
      return UWVM_VK_ERROR_INVALID_ARGUMENT;
    }

    mapped_data = mapped_memory.data() + begin;
    return map_memory_result;
  }

  void UnmapMemory(native::VkDevice device, native::VkDeviceMemory memory) override {
    (void)device;
    (void)memory;
    ++unmap_memory_calls;
  }

  std::int32_t FlushMappedMemory(native::VkDevice device,
                                 native::VkDeviceMemory memory,
                                 std::uint64_t offset,
                                 std::uint64_t size) override {
    (void)device;
    (void)memory;
    (void)offset;
    (void)size;
    ++flush_mapped_memory_calls;
    return flush_mapped_memory_result;
  }

  std::int32_t InvalidateMappedMemory(native::VkDevice device,
                                      native::VkDeviceMemory memory,
                                      std::uint64_t offset,
                                      std::uint64_t size) override {
    (void)device;
    (void)memory;
    (void)offset;
    (void)size;
    ++invalidate_mapped_memory_calls;
    return invalidate_mapped_memory_result;
  }

  std::vector<uwvm_vk_extension_property> device_extension_properties{};
  std::int32_t device_extension_query_result{UWVM_VK_SUCCESS};
  uwvm_vk_physical_device_features physical_device_features{};
  std::int32_t physical_device_features_result{UWVM_VK_SUCCESS};
  native::VkDevice create_device_result_handle{
      reinterpret_cast<native::VkDevice>(0x3000u)};
  std::int32_t create_device_result{UWVM_VK_SUCCESS};
  native::VkQueue get_device_queue_result_handle{
      reinterpret_cast<native::VkQueue>(0x4000u)};
  std::int32_t get_device_queue_result{UWVM_VK_SUCCESS};
  std::int32_t device_wait_idle_result{UWVM_VK_SUCCESS};
  std::int32_t queue_wait_idle_result{UWVM_VK_SUCCESS};
  std::int32_t map_memory_result{UWVM_VK_SUCCESS};
  std::int32_t flush_mapped_memory_result{UWVM_VK_SUCCESS};
  std::int32_t invalidate_mapped_memory_result{UWVM_VK_SUCCESS};
  std::vector<std::byte> mapped_memory{std::vector<std::byte>(64u)};

  native::VkPhysicalDevice last_extension_query_physical_device{};
  std::string last_extension_query_layer_name{};
  native::VkPhysicalDevice last_feature_query_physical_device{};
  native::VkPhysicalDevice last_create_device_physical_device{};
  std::uint32_t last_queue_family_index{};
  float last_queue_priority{};
  bool create_device_enabled_features_present{};
  uwvm_vk_physical_device_features captured_enabled_features{};
  native::VkDevice last_device_wait_idle_device{};
  std::size_t device_wait_idle_calls{};
  native::VkDevice last_get_device_queue_device{};
  std::uint32_t last_get_device_queue_family_index{};
  std::uint32_t last_get_device_queue_index{};
  native::VkDevice last_queue_wait_idle_device{};
  native::VkQueue last_queue_wait_idle_queue{};
  std::size_t queue_wait_idle_calls{};
  std::size_t map_memory_calls{};
  std::size_t unmap_memory_calls{};
  std::size_t flush_mapped_memory_calls{};
  std::size_t invalidate_mapped_memory_calls{};
  std::vector<DestroyBufferCall> destroyed_buffers{};
  std::vector<FreeMemoryCall> freed_memories{};
  std::vector<native::VkDevice> destroyed_devices{};
};

} // namespace

int main() {
  FakeMemoryHost host{};
  host.memory = std::vector<std::byte>(4096u);
  host.dynamic_length.store(4096u);
  host.descriptor = uwvm_preload_memory_descriptor_t{
      .memory_index = 0u,
      .delivery_state = UWVM_PRELOAD_MEMORY_DELIVERY_COPY,
      .backend_kind = UWVM_PRELOAD_MEMORY_BACKEND_NATIVE_DEFINED,
      .reserved0 = 0u,
      .reserved1 = 0u,
      .page_count = 1u,
      .page_size_bytes = 65536u,
      .byte_length = 4096u,
      .partial_protection_limit_bytes = 4096u,
      .mmap_view_begin = nullptr,
      .dynamic_length_atomic_object = &host.dynamic_length};
  g_host = &host;

  uwvm_preload_host_api_v1 host_api{.struct_size = sizeof(uwvm_preload_host_api_v1),
                                    .abi_version = 1u,
                                    .memory_descriptor_count = descriptor_count,
                                    .memory_descriptor_at = descriptor_at,
                                    .memory_read = memory_read,
                                    .memory_write = memory_write};

  FakeBackend backend{};
  backend.device_extension_properties.push_back(
      MakeExtensionProperty("VK_KHR_swapchain", 70u));
  backend.device_extension_properties.push_back(
      MakeExtensionProperty("VK_EXT_memory_budget", 1u));
  backend.physical_device_features.geometry_shader = 1u;
  backend.physical_device_features.sampler_anisotropy = 1u;
  backend.physical_device_features.variable_multisample_rate = 1u;

  auto &context{PluginContext::Instance()};
  context.SetExplicitHostApi(&host_api);
  context.SetBackendForTesting(&backend);

  auto const native_instance{
      reinterpret_cast<native::VkInstance>(0x1100u)};
  auto const native_physical_device{
      reinterpret_cast<native::VkPhysicalDevice>(0x2200u)};
  auto const native_memory{static_cast<native::VkDeviceMemory>(0x5500u)};
  auto const native_buffer{static_cast<native::VkBuffer>(0x7700u)};

  auto const instance_handle{
      context.instances.Insert({.native_handle = native_instance})};
  auto const physical_device_handle{
      context.physical_devices.Insert({.native_handle = native_physical_device,
                                       .parent_instance_handle = instance_handle})};

  constexpr std::uint64_t kPriorityAddress{64u};
  constexpr std::uint64_t kQueueInfoAddress{96u};
  constexpr std::uint64_t kEnabledFeaturesAddress{128u};
  constexpr std::uint64_t kCreateInfoAddress{256u};
  constexpr std::uint64_t kOutDeviceHandleAddress{320u};
  constexpr std::uint64_t kLayerNameBytesAddress{384u};
  constexpr std::uint64_t kLayerNameViewAddress{448u};
  constexpr std::uint64_t kExtensionBufferAddress{512u};
  constexpr std::uint64_t kExtensionCountAddress{1088u};
  constexpr std::uint64_t kFeaturesOutAddress{1152u};
  constexpr std::uint64_t kOutQueueHandleAddress{1408u};
  constexpr std::uint64_t kGuestUploadAddress{1536u};
  constexpr std::uint64_t kCopyRegionAddress{1600u};

  float queue_priority{1.0f};
  WriteGuest(kPriorityAddress, queue_priority);

  uwvm_vk_device_queue_create_info queue_create_info{
      .flags = 0u,
      .queue_family_index = 3u,
      .queue_count = 1u,
      .reserved0 = 0u,
      .priorities_address = kPriorityAddress};
  WriteGuest(kQueueInfoAddress, queue_create_info);

  uwvm_vk_physical_device_features enabled_features{};
  enabled_features.sampler_anisotropy = 1u;
  enabled_features.geometry_shader = 1u;
  WriteGuest(kEnabledFeaturesAddress, enabled_features);

  uwvm_vk_device_create_info create_info{
      .flags = 0u,
      .queue_create_info_count = 1u,
      .enabled_extension_count = 0u,
      .reserved0 = 0u,
      .queue_create_infos_address = kQueueInfoAddress,
      .enabled_extensions_address = 0u,
      .enabled_features_address = kEnabledFeaturesAddress};
  WriteGuest(kCreateInfoAddress, create_info);

  require(uwvm2_vulkan::api::CreateDevice(physical_device_handle,
                                          kCreateInfoAddress,
                                          kOutDeviceHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const device_handle{ReadGuest<std::uint64_t>(kOutDeviceHandleAddress)};
  require(device_handle != 0u);
  require(backend.last_create_device_physical_device == native_physical_device);
  require(backend.last_queue_family_index == 3u);
  require(backend.last_queue_priority == 1.0f);
  require(backend.create_device_enabled_features_present);
  require(backend.captured_enabled_features.sampler_anisotropy == 1u);
  require(backend.captured_enabled_features.geometry_shader == 1u);

  auto const device_record{context.devices.Find(device_handle)};
  require(device_record != nullptr);
  auto const native_device_handle{device_record->native_handle};

  WriteGuestString(kLayerNameBytesAddress, "VK_LAYER_KHRONOS_validation");
  uwvm_vk_string_view layer_name_view{
      .data_address = kLayerNameBytesAddress,
      .size = 27u,
      .reserved = 0u};
  WriteGuest(kLayerNameViewAddress, layer_name_view);

  require(uwvm2_vulkan::api::EnumerateDeviceExtensionProperties(
              physical_device_handle, kLayerNameViewAddress,
              kExtensionBufferAddress, 1u, kExtensionCountAddress) ==
          UWVM_VK_INCOMPLETE);
  auto const extension_count{ReadGuest<std::uint32_t>(kExtensionCountAddress)};
  if (extension_count != 2u) {
    std::fprintf(stderr, "extension_count=%u\n", extension_count);
  }
  require(extension_count == 2u);
  auto const first_extension{
      ReadGuest<uwvm_vk_extension_property>(kExtensionBufferAddress)};
  require(std::strncmp(first_extension.extension_name, "VK_KHR_swapchain",
                       sizeof(first_extension.extension_name)) == 0);
  require(first_extension.spec_version == 70u);
  require(backend.last_extension_query_physical_device == native_physical_device);
  require(backend.last_extension_query_layer_name ==
          "VK_LAYER_KHRONOS_validation");

  require(uwvm2_vulkan::api::GetPhysicalDeviceFeatures(
              physical_device_handle, kFeaturesOutAddress) == UWVM_VK_SUCCESS);
  auto const queried_features{
      ReadGuest<uwvm_vk_physical_device_features>(kFeaturesOutAddress)};
  require(queried_features.geometry_shader == 1u);
  require(queried_features.sampler_anisotropy == 1u);
  require(queried_features.variable_multisample_rate == 1u);
  require(backend.last_feature_query_physical_device == native_physical_device);

  require(uwvm2_vulkan::api::GetDeviceQueue(device_handle, 3u, 0u,
                                            kOutQueueHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const queue_handle{ReadGuest<std::uint64_t>(kOutQueueHandleAddress)};
  require(queue_handle != 0u);
  require(backend.last_get_device_queue_device == native_device_handle);
  require(backend.last_get_device_queue_family_index == 3u);
  require(backend.last_get_device_queue_index == 0u);

  require(uwvm2_vulkan::api::DeviceWaitIdle(device_handle) == UWVM_VK_SUCCESS);
  require(backend.device_wait_idle_calls == 1u);
  require(backend.last_device_wait_idle_device == native_device_handle);

  require(uwvm2_vulkan::api::QueueWaitIdle(queue_handle) == UWVM_VK_SUCCESS);
  require(backend.queue_wait_idle_calls == 1u);
  require(backend.last_queue_wait_idle_device == native_device_handle);
  require(backend.last_queue_wait_idle_queue ==
          backend.get_device_queue_result_handle);

  auto const memory_handle{context.memories.Insert(
      {.native_handle = native_memory,
       .parent_device_handle = device_handle,
       .allocation_size = 64u,
       .memory_type_index = 0u})};
  auto const buffer_handle{context.buffers.Insert(
      {.native_handle = native_buffer, .parent_device_handle = device_handle})};
  require(memory_handle != 0u);
  require(buffer_handle != 0u);

  std::array<std::byte, 4u> upload_bytes{
      std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44}};
  WriteGuestBytes(kGuestUploadAddress, upload_bytes.data(), upload_bytes.size());

  uwvm_vk_memory_copy_region copy_region{
      .guest_memory_index = 0u,
      .reserved = 0u,
      .guest_address = kGuestUploadAddress,
      .device_offset = 0u,
      .size = upload_bytes.size()};
  WriteGuest(kCopyRegionAddress, copy_region);

  backend.flush_mapped_memory_result = UWVM_VK_ERROR_DEVICE_LOST;
  require(uwvm2_vulkan::api::CopyGuestToDeviceMemory(device_handle, memory_handle,
                                                      kCopyRegionAddress) ==
          UWVM_VK_ERROR_DEVICE_LOST);
  require(backend.map_memory_calls == 1u);
  require(backend.flush_mapped_memory_calls == 1u);
  require(backend.unmap_memory_calls == 1u);

  require(uwvm2_vulkan::api::DestroyDevice(device_handle) == UWVM_VK_SUCCESS);
  require(context.devices.Find(device_handle) == nullptr);
  require(context.queues.Find(queue_handle) == nullptr);
  require(context.buffers.Find(buffer_handle) == nullptr);
  require(context.memories.Find(memory_handle) == nullptr);
  require(backend.destroyed_devices.size() == 1u);
  require(backend.destroyed_devices[0] == native_device_handle);
  require(backend.destroyed_buffers.size() == 1u);
  require(backend.destroyed_buffers[0].device == native_device_handle);
  require(backend.destroyed_buffers[0].buffer == native_buffer);
  require(backend.freed_memories.size() == 1u);
  require(backend.freed_memories[0].device == native_device_handle);
  require(backend.freed_memories[0].memory == native_memory);

  context.SetBackendForTesting(nullptr);
  context.SetExplicitHostApi(nullptr);
  g_host = nullptr;
  return 0;
}
