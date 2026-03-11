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

#ifndef UWVM2_VULKAN_SRC_RUNTIME_HANDLE_REGISTRY_H_
#define UWVM2_VULKAN_SRC_RUNTIME_HANDLE_REGISTRY_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "vulkan/native_abi.h"

namespace uwvm2_vulkan::runtime {

template <typename Record> class HandleTable {
public:
  [[nodiscard]] std::uint64_t Insert(Record record) {
    auto const handle{next_handle_.fetch_add(1u, std::memory_order_relaxed)};
    records_.emplace(handle, std::move(record));
    return handle;
  }

  [[nodiscard]] Record *Find(std::uint64_t handle) noexcept {
    auto const iterator{records_.find(handle)};
    return iterator == records_.end() ? nullptr
                                      : std::addressof(iterator->second);
  }

  [[nodiscard]] Record const *Find(std::uint64_t handle) const noexcept {
    auto const iterator{records_.find(handle)};
    return iterator == records_.end() ? nullptr
                                      : std::addressof(iterator->second);
  }

  void Erase(std::uint64_t handle) noexcept { records_.erase(handle); }

  template <typename Predicate> void EraseIf(Predicate predicate) {
    for (auto iterator{records_.begin()}; iterator != records_.end();) {
      if (predicate(iterator->second)) {
        iterator = records_.erase(iterator);
      } else {
        ++iterator;
      }
    }
  }

  template <typename Callback> void ForEach(Callback callback) {
    for (auto &entry : records_) {
      callback(entry.first, entry.second);
    }
  }

private:
  std::atomic<std::uint64_t> next_handle_{1u};
  std::unordered_map<std::uint64_t, Record> records_{};
};

struct InstanceRecord {
  vk::native::VkInstance native_handle{};
};

struct PhysicalDeviceRecord {
  vk::native::VkPhysicalDevice native_handle{};
  std::uint64_t parent_instance_handle{};
};

struct DeviceRecord {
  vk::native::VkDevice native_handle{};
  std::uint64_t parent_physical_device_handle{};
};

struct QueueRecord {
  vk::native::VkQueue native_handle{};
  std::uint64_t parent_device_handle{};
  std::uint32_t queue_family_index{};
  std::uint32_t queue_index{};
};

struct BufferRecord {
  vk::native::VkBuffer native_handle{};
  std::uint64_t parent_device_handle{};
};

struct ImageRecord {
  vk::native::VkImage native_handle{};
  std::uint64_t parent_device_handle{};
};

struct DeviceMemoryRecord {
  vk::native::VkDeviceMemory native_handle{};
  std::uint64_t parent_device_handle{};
  std::uint64_t allocation_size{};
  std::uint32_t memory_type_index{};
};

} // namespace uwvm2_vulkan::runtime

#endif
