/*************************************************************
 * UWVM2 Vulkan Wrapper                                     *
 * Copyright (c) 2026-present MacroModel. All rights        *
 * reserved.                                                *
 *************************************************************/

/**
 * @author      MacroModel
 * @version     0.1.0
 * @date        2026-03-11
 */

/****************************************
 *  _   _ __        ____     __ __  __  *
 * | | | |\ \      / /\ \   / /|  \/  | *
 * | | | | \ \ /\ / /  \ \ / / | |\/| | *
 * | |_| |  \ V  V /    \ V /  | |  | | *
 *  \___/    \_/\_/      \_/   |_|  |_| *
 *                                      *
 ****************************************/
#include "runtime/memory_access.h"

#include <algorithm>
#include <new>

namespace uwvm2_vulkan::runtime {

void GuestMemoryAccessor::SetHostApi(
    uwvm_preload_host_api_v1 const *host_api) noexcept {
  host_api_ = host_api;
}

uwvm_preload_host_api_v1 const *GuestMemoryAccessor::host_api() const noexcept {
  return host_api_;
}

bool GuestMemoryAccessor::IsAvailable() const noexcept {
  return host_api_ != nullptr &&
         host_api_->memory_descriptor_count != nullptr &&
         host_api_->memory_descriptor_at != nullptr &&
         host_api_->memory_read != nullptr &&
         host_api_->memory_write != nullptr;
}

std::optional<uwvm_preload_memory_descriptor_t>
GuestMemoryAccessor::FindDescriptor(std::size_t memory_index) const noexcept {
  if (host_api_ == nullptr || host_api_->memory_descriptor_count == nullptr ||
      host_api_->memory_descriptor_at == nullptr) {
    return std::nullopt;
  }

  auto const descriptor_count{host_api_->memory_descriptor_count()};
  for (std::size_t descriptor_index{}; descriptor_index != descriptor_count;
       ++descriptor_index) {
    uwvm_preload_memory_descriptor_t descriptor{};
    if (!host_api_->memory_descriptor_at(descriptor_index, &descriptor)) {
      continue;
    }
    if (descriptor.memory_index == memory_index) {
      return descriptor;
    }
  }

  return std::nullopt;
}

bool GuestMemoryAccessor::Read(std::size_t memory_index, std::uint64_t address,
                               void *destination,
                               std::size_t size) const noexcept {
  if (size == 0u) {
    return true;
  }
  if (destination == nullptr) {
    return false;
  }

  auto descriptor{FindDescriptor(memory_index)};
  if (!descriptor.has_value()) {
    return false;
  }

  switch (static_cast<uwvm_preload_memory_delivery_state_t>(
      descriptor->delivery_state)) {
  case UWVM_PRELOAD_MEMORY_DELIVERY_COPY: {
    return host_api_ != nullptr && host_api_->memory_read != nullptr &&
           host_api_->memory_read(memory_index, address, destination, size);
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_FULL_PROTECTION:
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_PARTIAL_PROTECTION:
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_DYNAMIC_BOUNDS: {
    if (descriptor->mmap_view_begin == nullptr ||
        !DirectRangeIsValid(*descriptor, address, size)) {
      return false;
    }
    std::memcpy(destination,
                static_cast<std::byte const *>(descriptor->mmap_view_begin) +
                    static_cast<std::size_t>(address),
                size);
    return true;
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_NONE:
  default: {
    return false;
  }
  }
}

bool GuestMemoryAccessor::Write(std::size_t memory_index, std::uint64_t address,
                                void const *source,
                                std::size_t size) const noexcept {
  if (size == 0u) {
    return true;
  }
  if (source == nullptr) {
    return false;
  }

  auto descriptor{FindDescriptor(memory_index)};
  if (!descriptor.has_value()) {
    return false;
  }

  switch (static_cast<uwvm_preload_memory_delivery_state_t>(
      descriptor->delivery_state)) {
  case UWVM_PRELOAD_MEMORY_DELIVERY_COPY: {
    return host_api_ != nullptr && host_api_->memory_write != nullptr &&
           host_api_->memory_write(memory_index, address, source, size);
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_FULL_PROTECTION:
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_PARTIAL_PROTECTION:
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_DYNAMIC_BOUNDS: {
    if (descriptor->mmap_view_begin == nullptr ||
        !DirectRangeIsValid(*descriptor, address, size)) {
      return false;
    }
    std::memcpy(static_cast<std::byte *>(descriptor->mmap_view_begin) +
                    static_cast<std::size_t>(address),
                source, size);
    return true;
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_NONE:
  default: {
    return false;
  }
  }
}

bool GuestMemoryAccessor::ReadString(std::size_t memory_index,
                                     std::uint64_t address, std::uint32_t size,
                                     std::string &out) const {
  out.clear();
  if (size == 0u) {
    return true;
  }

  out.resize(size);
  return Read(memory_index, address, out.data(), out.size());
}

bool GuestMemoryAccessor::RangeIsValid(std::uint64_t byte_length,
                                       std::uint64_t address,
                                       std::size_t size) noexcept {
  auto const size64{static_cast<std::uint64_t>(size)};
  return address <= byte_length && size64 <= (byte_length - address);
}

bool GuestMemoryAccessor::DirectRangeIsValid(
    uwvm_preload_memory_descriptor_t const &descriptor, std::uint64_t address,
    std::size_t size) noexcept {
  switch (static_cast<uwvm_preload_memory_delivery_state_t>(
      descriptor.delivery_state)) {
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_FULL_PROTECTION: {
    return RangeIsValid(descriptor.byte_length, address, size);
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_PARTIAL_PROTECTION: {
    if (RangeIsValid(descriptor.partial_protection_limit_bytes, address,
                     size)) {
      return true;
    }
    return RangeIsValid(descriptor.byte_length, address, size);
  }
  case UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_DYNAMIC_BOUNDS: {
    if (descriptor.dynamic_length_atomic_object == nullptr) {
      return false;
    }

    auto const *current_length_atomic{static_cast<std::atomic_size_t const *>(
        descriptor.dynamic_length_atomic_object)};
    auto const current_length{static_cast<std::uint64_t>(
        current_length_atomic->load(std::memory_order_acquire))};
    return RangeIsValid(current_length, address, size);
  }
  default: {
    return false;
  }
  }
}

} // namespace uwvm2_vulkan::runtime
