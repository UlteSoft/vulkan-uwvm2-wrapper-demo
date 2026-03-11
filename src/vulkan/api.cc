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
#include "vulkan/api.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <uwvm2_vulkan/vulkan_types.h>

#include "runtime/plugin_context.h"

namespace uwvm2_vulkan::api {

namespace {

inline constexpr std::size_t kDefaultGuestMemoryIndex{0u};
static_assert(sizeof(uwvm_vk_physical_device_features) ==
              sizeof(vk::native::VkPhysicalDeviceFeatures));

template <typename T>
[[nodiscard]] bool ReadGuestObject(std::uint64_t address, T &out) {
  auto &memory{runtime::PluginContext::Instance().memory()};
  return address != 0u &&
         memory.ReadObject(kDefaultGuestMemoryIndex, address, out);
}

template <typename T>
[[nodiscard]] bool WriteGuestObject(std::uint64_t address, T const &value) {
  auto &memory{runtime::PluginContext::Instance().memory()};
  return address != 0u &&
         memory.WriteObject(kDefaultGuestMemoryIndex, address, value);
}

template <typename T>
[[nodiscard]] bool ReadGuestArray(std::uint64_t address, std::uint32_t count,
                                  std::vector<T> &out) {
  out.clear();
  if (count == 0u) {
    return true;
  }
  if (address == 0u) {
    return false;
  }

  out.resize(count);
  auto &memory{runtime::PluginContext::Instance().memory()};
  return memory.Read(kDefaultGuestMemoryIndex, address, out.data(),
                     sizeof(T) * out.size());
}

template <typename T>
[[nodiscard]] std::int32_t WriteGuestBuffer(std::uint64_t buffer_address,
                                            std::uint32_t capacity,
                                            std::uint64_t out_count_address,
                                            std::span<T> values) noexcept {
  auto const total_count{static_cast<std::uint32_t>(values.size())};
  if (!WriteGuestObject(out_count_address, total_count)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  if (capacity == 0u || buffer_address == 0u || values.empty()) {
    return capacity < total_count ? UWVM_VK_INCOMPLETE : UWVM_VK_SUCCESS;
  }

  auto const write_count{std::min(capacity, total_count)};
  auto &memory{runtime::PluginContext::Instance().memory()};
  if (!memory.Write(kDefaultGuestMemoryIndex, buffer_address, values.data(),
                    sizeof(T) * write_count)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }
  return write_count < total_count ? UWVM_VK_INCOMPLETE : UWVM_VK_SUCCESS;
}

[[nodiscard]] bool ReadStringView(uwvm_vk_string_view const &view,
                                  std::string &out) {
  auto &memory{runtime::PluginContext::Instance().memory()};
  return memory.ReadString(kDefaultGuestMemoryIndex, view.data_address,
                           view.size, out);
}

[[nodiscard]] bool ReadStringViewList(std::uint64_t address,
                                      std::uint32_t count,
                                      std::vector<std::string> &out) {
  out.clear();
  std::vector<uwvm_vk_string_view> views{};
  if (!ReadGuestArray(address, count, views)) {
    return false;
  }

  out.reserve(views.size());
  for (auto const &view : views) {
    std::string text{};
    if (!ReadStringView(view, text)) {
      return false;
    }
    out.push_back(std::move(text));
  }
  return true;
}

template <typename NativeHandle, typename Record>
[[nodiscard]] std::int32_t RequireHandle(runtime::HandleTable<Record> &table,
                                         std::uint64_t handle,
                                         Record *&record) noexcept {
  if (handle == 0u) {
    record = nullptr;
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  record = table.Find(handle);
  if (record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }
  return UWVM_VK_SUCCESS;
}

struct InstanceCreateStorage {
  uwvm_vk_instance_create_info guest_create_info{};
  uwvm_vk_application_info guest_application_info{};
  std::string application_name{};
  std::string engine_name{};
  std::vector<std::string> enabled_layers{};
  std::vector<std::string> enabled_extensions{};
  std::vector<char const *> enabled_layer_ptrs{};
  std::vector<char const *> enabled_extension_ptrs{};
  vk::native::VkApplicationInfo native_application_info{};
  vk::native::VkInstanceCreateInfo native_create_info{};
};

[[nodiscard]] bool BuildInstanceCreateInfo(std::uint64_t create_info_address,
                                           InstanceCreateStorage &storage) {
  if (!ReadGuestObject(create_info_address, storage.guest_create_info)) {
    return false;
  }

  if (storage.guest_create_info.application_info_address != 0u) {
    if (!ReadGuestObject(storage.guest_create_info.application_info_address,
                         storage.guest_application_info)) {
      return false;
    }
    if (!ReadStringView(storage.guest_application_info.application_name,
                        storage.application_name) ||
        !ReadStringView(storage.guest_application_info.engine_name,
                        storage.engine_name)) {
      return false;
    }

    storage.native_application_info = vk::native::VkApplicationInfo{
        .sType = vk::native::VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = storage.application_name.empty()
                                ? nullptr
                                : storage.application_name.c_str(),
        .applicationVersion =
            storage.guest_application_info.application_version,
        .pEngineName =
            storage.engine_name.empty() ? nullptr : storage.engine_name.c_str(),
        .engineVersion = storage.guest_application_info.engine_version,
        .apiVersion = storage.guest_application_info.api_version};
  }

  if (!ReadStringViewList(storage.guest_create_info.enabled_layers_address,
                          storage.guest_create_info.enabled_layer_count,
                          storage.enabled_layers) ||
      !ReadStringViewList(storage.guest_create_info.enabled_extensions_address,
                          storage.guest_create_info.enabled_extension_count,
                          storage.enabled_extensions)) {
    return false;
  }

  storage.enabled_layer_ptrs.reserve(storage.enabled_layers.size());
  for (auto const &layer : storage.enabled_layers) {
    storage.enabled_layer_ptrs.push_back(layer.c_str());
  }

  storage.enabled_extension_ptrs.reserve(storage.enabled_extensions.size());
  for (auto const &extension : storage.enabled_extensions) {
    storage.enabled_extension_ptrs.push_back(extension.c_str());
  }

  storage.native_create_info = vk::native::VkInstanceCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = storage.guest_create_info.flags,
      .pApplicationInfo =
          storage.guest_create_info.application_info_address == 0u
              ? nullptr
              : &storage.native_application_info,
      .enabledLayerCount = storage.guest_create_info.enabled_layer_count,
      .ppEnabledLayerNames = storage.enabled_layer_ptrs.empty()
                                 ? nullptr
                                 : storage.enabled_layer_ptrs.data(),
      .enabledExtensionCount =
          storage.guest_create_info.enabled_extension_count,
      .ppEnabledExtensionNames = storage.enabled_extension_ptrs.empty()
                                     ? nullptr
                                     : storage.enabled_extension_ptrs.data()};
  return true;
}

struct DeviceCreateStorage {
  uwvm_vk_device_create_info guest_create_info{};
  std::vector<uwvm_vk_device_queue_create_info> guest_queue_infos{};
  std::vector<std::vector<float>> queue_priorities{};
  std::vector<vk::native::VkDeviceQueueCreateInfo> native_queue_infos{};
  std::vector<std::string> enabled_extensions{};
  std::vector<char const *> enabled_extension_ptrs{};
  uwvm_vk_physical_device_features guest_enabled_features{};
  vk::native::VkPhysicalDeviceFeatures native_enabled_features{};
  vk::native::VkDeviceCreateInfo native_create_info{};
};

[[nodiscard]] bool BuildDeviceCreateInfo(std::uint64_t create_info_address,
                                         DeviceCreateStorage &storage) {
  if (!ReadGuestObject(create_info_address, storage.guest_create_info)) {
    return false;
  }
  if (!ReadGuestArray(storage.guest_create_info.queue_create_infos_address,
                      storage.guest_create_info.queue_create_info_count,
                      storage.guest_queue_infos)) {
    return false;
  }

  storage.queue_priorities.reserve(storage.guest_queue_infos.size());
  storage.native_queue_infos.reserve(storage.guest_queue_infos.size());
  for (auto const &queue_info : storage.guest_queue_infos) {
    std::vector<float> priorities(queue_info.queue_count);
    if (queue_info.queue_count != 0u) {
      auto &memory{runtime::PluginContext::Instance().memory()};
      if (queue_info.priorities_address == 0u ||
          !memory.Read(kDefaultGuestMemoryIndex, queue_info.priorities_address,
                       priorities.data(), sizeof(float) * priorities.size())) {
        return false;
      }
    }

    storage.queue_priorities.push_back(priorities);
    storage.native_queue_infos.push_back(vk::native::VkDeviceQueueCreateInfo{
        .sType = vk::native::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = queue_info.flags,
        .queueFamilyIndex = queue_info.queue_family_index,
        .queueCount = queue_info.queue_count,
        .pQueuePriorities = storage.queue_priorities.back().empty()
                                ? nullptr
                                : storage.queue_priorities.back().data()});
  }

  if (!ReadStringViewList(storage.guest_create_info.enabled_extensions_address,
                          storage.guest_create_info.enabled_extension_count,
                          storage.enabled_extensions)) {
    return false;
  }

  if (storage.guest_create_info.enabled_features_address != 0u) {
    if (!ReadGuestObject(storage.guest_create_info.enabled_features_address,
                         storage.guest_enabled_features)) {
      return false;
    }
    std::memcpy(&storage.native_enabled_features,
                &storage.guest_enabled_features,
                sizeof(storage.native_enabled_features));
  }

  storage.enabled_extension_ptrs.reserve(storage.enabled_extensions.size());
  for (auto const &extension : storage.enabled_extensions) {
    storage.enabled_extension_ptrs.push_back(extension.c_str());
  }

  storage.native_create_info = vk::native::VkDeviceCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .flags = storage.guest_create_info.flags,
      .queueCreateInfoCount = storage.guest_create_info.queue_create_info_count,
      .pQueueCreateInfos = storage.native_queue_infos.empty()
                               ? nullptr
                               : storage.native_queue_infos.data(),
      .enabledLayerCount = 0u,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount =
          storage.guest_create_info.enabled_extension_count,
      .ppEnabledExtensionNames = storage.enabled_extension_ptrs.empty()
                                     ? nullptr
                                     : storage.enabled_extension_ptrs.data(),
      .pEnabledFeatures =
          storage.guest_create_info.enabled_features_address == 0u
              ? nullptr
              : &storage.native_enabled_features};
  return true;
}

struct BufferCreateStorage {
  uwvm_vk_buffer_create_info guest_create_info{};
  std::vector<std::uint32_t> queue_family_indices{};
  vk::native::VkBufferCreateInfo native_create_info{};
};

[[nodiscard]] bool BuildBufferCreateInfo(std::uint64_t create_info_address,
                                         BufferCreateStorage &storage) {
  if (!ReadGuestObject(create_info_address, storage.guest_create_info)) {
    return false;
  }

  if (storage.guest_create_info.queue_family_index_count != 0u) {
    storage.queue_family_indices.resize(
        storage.guest_create_info.queue_family_index_count);
    auto &memory{runtime::PluginContext::Instance().memory()};
    if (storage.guest_create_info.queue_family_indices_address == 0u ||
        !memory.Read(kDefaultGuestMemoryIndex,
                     storage.guest_create_info.queue_family_indices_address,
                     storage.queue_family_indices.data(),
                     sizeof(std::uint32_t) *
                         storage.queue_family_indices.size())) {
      return false;
    }
  }

  storage.native_create_info = vk::native::VkBufferCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = storage.guest_create_info.flags,
      .size = storage.guest_create_info.size,
      .usage = storage.guest_create_info.usage,
      .sharingMode = static_cast<vk::native::VkSharingMode>(
          storage.guest_create_info.sharing_mode),
      .queueFamilyIndexCount =
          storage.guest_create_info.queue_family_index_count,
      .pQueueFamilyIndices = storage.queue_family_indices.empty()
                                 ? nullptr
                                 : storage.queue_family_indices.data()};
  return true;
}

[[nodiscard]] bool
BuildMemoryAllocateInfo(std::uint64_t allocate_info_address,
                        vk::native::VkMemoryAllocateInfo &allocate_info) {
  uwvm_vk_memory_allocate_info guest_allocate_info{};
  if (!ReadGuestObject(allocate_info_address, guest_allocate_info)) {
    return false;
  }

  allocate_info = vk::native::VkMemoryAllocateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = guest_allocate_info.allocation_size,
      .memoryTypeIndex = guest_allocate_info.memory_type_index};
  return true;
}

[[nodiscard]] bool ReadCopyRegion(std::uint64_t copy_region_address,
                                  uwvm_vk_memory_copy_region &copy_region) {
  return ReadGuestObject(copy_region_address, copy_region);
}

[[nodiscard]] std::int32_t CopyBetweenGuestAndMappedMemory(
    bool guest_to_device, runtime::PluginContext &context,
    runtime::DeviceRecord const &device_record,
    runtime::DeviceMemoryRecord const &memory_record,
    uwvm_vk_memory_copy_region const &copy_region) {
  if (copy_region.size >
      static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto const byte_count{static_cast<std::size_t>(copy_region.size)};
  auto &backend{context.backend()};
  void *mapped_data{};
  auto result{backend.MapMemory(
      device_record.native_handle, memory_record.native_handle,
      copy_region.device_offset, copy_region.size, mapped_data)};
  if (result != UWVM_VK_SUCCESS || mapped_data == nullptr) {
    return result == UWVM_VK_SUCCESS ? UWVM_VK_ERROR_MEMORY_MAP_FAILED : result;
  }

  auto unmap_guard = [&backend, &device_record, &memory_record]() {
    backend.UnmapMemory(device_record.native_handle,
                        memory_record.native_handle);
  };

  auto &memory{context.memory()};
  if (guest_to_device) {
    if (!memory.Read(copy_region.guest_memory_index, copy_region.guest_address,
                     mapped_data, byte_count)) {
      unmap_guard();
      return UWVM_VK_ERROR_GUEST_MEMORY;
    }

    result = backend.FlushMappedMemory(device_record.native_handle,
                                       memory_record.native_handle,
                                       copy_region.device_offset,
                                       copy_region.size);
    unmap_guard();
    return result;
  }

  result = backend.InvalidateMappedMemory(device_record.native_handle,
                                          memory_record.native_handle,
                                          copy_region.device_offset,
                                          copy_region.size);
  if (result != UWVM_VK_SUCCESS) {
    unmap_guard();
    return result;
  }

  if (!memory.Write(copy_region.guest_memory_index, copy_region.guest_address,
                    mapped_data, byte_count)) {
    unmap_guard();
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  unmap_guard();
  return UWVM_VK_SUCCESS;
}

[[nodiscard]] std::int32_t ReadOptionalLayerNameAddress(
    std::uint64_t layer_name_address, std::string &layer_name,
    char const *&layer_name_ptr) {
  layer_name.clear();
  layer_name_ptr = nullptr;

  if (layer_name_address == 0u) {
    return UWVM_VK_SUCCESS;
  }

  uwvm_vk_string_view layer_name_view{};
  if (!ReadGuestObject(layer_name_address, layer_name_view) ||
      !ReadStringView(layer_name_view, layer_name)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  layer_name_ptr = layer_name.c_str();
  return UWVM_VK_SUCCESS;
}

} // namespace

std::int32_t LoaderAvailable() noexcept {
  return runtime::PluginContext::Instance().backend().LoaderAvailable() ? 1 : 0;
}

std::int32_t
EnumerateInstanceVersion(std::uint64_t out_api_version_address) noexcept {
  if (out_api_version_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::uint32_t api_version{};
  auto result{
      runtime::PluginContext::Instance().backend().EnumerateInstanceVersion(
          api_version)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }
  return WriteGuestObject(out_api_version_address, api_version)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t EnumerateInstanceExtensionProperties(
    std::uint64_t layer_name_address, std::uint64_t property_buffer_address,
    std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept {
  if (out_property_count_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::string layer_name{};
  char const *layer_name_ptr{};
  auto const layer_name_result{ReadOptionalLayerNameAddress(
      layer_name_address, layer_name, layer_name_ptr)};
  if (layer_name_result != UWVM_VK_SUCCESS) {
    return layer_name_result;
  }

  std::vector<uwvm_vk_extension_property> properties{};
  auto result{
      runtime::PluginContext::Instance()
          .backend()
          .EnumerateInstanceExtensionProperties(layer_name_ptr, properties)};
  if (result < 0) {
    return result;
  }
  return WriteGuestBuffer(property_buffer_address, property_capacity,
                          out_property_count_address, std::span{properties});
}

std::int32_t EnumerateInstanceLayerProperties(
    std::uint64_t property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept {
  if (out_property_count_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::vector<uwvm_vk_layer_property> properties{};
  auto result{runtime::PluginContext::Instance()
                  .backend()
                  .EnumerateInstanceLayerProperties(properties)};
  if (result < 0) {
    return result;
  }
  return WriteGuestBuffer(property_buffer_address, property_capacity,
                          out_property_count_address, std::span{properties});
}

std::int32_t CreateInstance(std::uint64_t create_info_address,
                            std::uint64_t out_instance_address) noexcept {
  if (create_info_address == 0u || out_instance_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  InstanceCreateStorage storage{};
  if (!BuildInstanceCreateInfo(create_info_address, storage)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  vk::native::VkInstance native_instance{};
  auto result{context.backend().CreateInstance(storage.native_create_info,
                                               native_instance)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  std::scoped_lock lock{context.mutex()};
  auto const handle{context.instances.Insert(
      runtime::InstanceRecord{.native_handle = native_instance})};
  return WriteGuestObject(out_instance_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroyInstance(std::uint64_t instance_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};

  auto *instance_record{context.instances.Find(instance_handle)};
  if (instance_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  std::unordered_set<std::uint64_t> physical_device_handles{};
  context.physical_devices.ForEach(
      [&](std::uint64_t handle, runtime::PhysicalDeviceRecord const &record) {
        if (record.parent_instance_handle == instance_handle) {
          physical_device_handles.insert(handle);
        }
      });

  std::unordered_set<std::uint64_t> device_handles{};
  context.devices.ForEach(
      [&](std::uint64_t handle, runtime::DeviceRecord const &record) {
        if (physical_device_handles.contains(
                record.parent_physical_device_handle)) {
          device_handles.insert(handle);
        }
      });

  context.buffers.ForEach([&](std::uint64_t handle,
                              runtime::BufferRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().DestroyBuffer(device_record->native_handle,
                                        record.native_handle);
      }
    }
  });

  context.memories.ForEach([&](std::uint64_t handle,
                               runtime::DeviceMemoryRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().FreeMemory(device_record->native_handle,
                                     record.native_handle);
      }
    }
  });

  context.devices.ForEach(
      [&](std::uint64_t handle, runtime::DeviceRecord const &record) {
        (void)handle;
        if (device_handles.contains(handle)) {
          context.backend().DestroyDevice(record.native_handle);
        }
      });

  context.backend().DestroyInstance(instance_record->native_handle);
  context.queues.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.buffers.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.memories.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.devices.EraseIf([&](auto const &record) {
    return physical_device_handles.contains(
        record.parent_physical_device_handle);
  });
  context.physical_devices.EraseIf([instance_handle](auto const &record) {
    return record.parent_instance_handle == instance_handle;
  });
  context.instances.Erase(instance_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t
EnumeratePhysicalDevices(std::uint64_t instance_handle,
                         std::uint64_t out_device_buffer_address,
                         std::uint32_t device_capacity,
                         std::uint64_t out_device_count_address) noexcept {
  if (out_device_count_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::vector<vk::native::VkPhysicalDevice> native_devices{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *instance_record{context.instances.Find(instance_handle)};
    if (instance_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().EnumeratePhysicalDevices(
        instance_record->native_handle, native_devices)};
    if (result < 0) {
      return result;
    }
  }

  auto const write_count{std::min(
      device_capacity, static_cast<std::uint32_t>(native_devices.size()))};
  std::vector<std::uint64_t> guest_handles{};
  guest_handles.reserve(write_count);

  {
    std::scoped_lock lock{context.mutex()};
    for (std::uint32_t index{}; index != write_count; ++index) {
      guest_handles.push_back(
          context.physical_devices.Insert(runtime::PhysicalDeviceRecord{
              .native_handle = native_devices[index],
              .parent_instance_handle = instance_handle}));
    }
  }

  return WriteGuestBuffer(out_device_buffer_address, device_capacity,
                          out_device_count_address, std::span{guest_handles});
}

std::int32_t EnumerateDeviceExtensionProperties(
    std::uint64_t physical_device_handle, std::uint64_t layer_name_address,
    std::uint64_t property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept {
  if (out_property_count_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::string layer_name{};
  char const *layer_name_ptr{};
  auto const layer_name_result{ReadOptionalLayerNameAddress(
      layer_name_address, layer_name, layer_name_ptr)};
  if (layer_name_result != UWVM_VK_SUCCESS) {
    return layer_name_result;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::vector<uwvm_vk_extension_property> properties{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().EnumerateDeviceExtensionProperties(
        physical_device_record->native_handle, layer_name_ptr, properties)};
    if (result < 0) {
      return result;
    }
  }

  return WriteGuestBuffer(property_buffer_address, property_capacity,
                          out_property_count_address, std::span{properties});
}

std::int32_t
GetPhysicalDeviceProperties(std::uint64_t physical_device_handle,
                            std::uint64_t out_properties_address) noexcept {
  if (out_properties_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  uwvm_vk_physical_device_properties properties{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().GetPhysicalDeviceProperties(
        physical_device_record->native_handle, properties)};
    if (result != UWVM_VK_SUCCESS) {
      return result;
    }
  }

  return WriteGuestObject(out_properties_address, properties)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t GetPhysicalDeviceFeatures(
    std::uint64_t physical_device_handle,
    std::uint64_t out_features_address) noexcept {
  if (out_features_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  uwvm_vk_physical_device_features features{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().GetPhysicalDeviceFeatures(
        physical_device_record->native_handle, features)};
    if (result != UWVM_VK_SUCCESS) {
      return result;
    }
  }

  return WriteGuestObject(out_features_address, features)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t GetPhysicalDeviceMemoryProperties(
    std::uint64_t physical_device_handle,
    std::uint64_t out_properties_address) noexcept {
  if (out_properties_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  uwvm_vk_physical_device_memory_properties properties{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().GetPhysicalDeviceMemoryProperties(
        physical_device_record->native_handle, properties)};
    if (result != UWVM_VK_SUCCESS) {
      return result;
    }
  }

  return WriteGuestObject(out_properties_address, properties)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t GetPhysicalDeviceQueueFamilyProperties(
    std::uint64_t physical_device_handle,
    std::uint64_t out_property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept {
  if (out_property_count_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::vector<uwvm_vk_queue_family_properties> properties{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().GetPhysicalDeviceQueueFamilyProperties(
        physical_device_record->native_handle, properties)};
    if (result != UWVM_VK_SUCCESS) {
      return result;
    }
  }

  return WriteGuestBuffer(out_property_buffer_address, property_capacity,
                          out_property_count_address, std::span{properties});
}

std::int32_t CreateDevice(std::uint64_t physical_device_handle,
                          std::uint64_t create_info_address,
                          std::uint64_t out_device_address) noexcept {
  if (create_info_address == 0u || out_device_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  DeviceCreateStorage storage{};
  if (!BuildDeviceCreateInfo(create_info_address, storage)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  vk::native::VkDevice native_device{};
  {
    std::scoped_lock lock{context.mutex()};
    auto *physical_device_record{
        context.physical_devices.Find(physical_device_handle)};
    if (physical_device_record == nullptr) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }

    auto result{context.backend().CreateDevice(
        physical_device_record->native_handle, storage.native_create_info,
        native_device)};
    if (result != UWVM_VK_SUCCESS) {
      return result;
    }

    auto const handle{context.devices.Insert(runtime::DeviceRecord{
        .native_handle = native_device,
        .parent_physical_device_handle = physical_device_handle})};
    return WriteGuestObject(out_device_address, handle)
               ? UWVM_VK_SUCCESS
               : UWVM_VK_ERROR_GUEST_MEMORY;
  }
}

std::int32_t DestroyDevice(std::uint64_t device_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};

  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.buffers.ForEach([&](std::uint64_t handle,
                              runtime::BufferRecord const &record) {
    (void)handle;
    if (record.parent_device_handle == device_handle) {
      context.backend().DestroyBuffer(device_record->native_handle,
                                      record.native_handle);
    }
  });
  context.memories.ForEach([&](std::uint64_t handle,
                               runtime::DeviceMemoryRecord const &record) {
    (void)handle;
    if (record.parent_device_handle == device_handle) {
      context.backend().FreeMemory(device_record->native_handle,
                                   record.native_handle);
    }
  });

  context.backend().DestroyDevice(device_record->native_handle);
  context.queues.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.buffers.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.memories.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.devices.Erase(device_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t DeviceWaitIdle(std::uint64_t device_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};

  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().DeviceWaitIdle(device_record->native_handle);
}

std::int32_t GetDeviceQueue(std::uint64_t device_handle,
                            std::uint32_t queue_family_index,
                            std::uint32_t queue_index,
                            std::uint64_t out_queue_address) noexcept {
  if (out_queue_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkQueue native_queue{};
  auto result{context.backend().GetDeviceQueue(device_record->native_handle,
                                               queue_family_index, queue_index,
                                               native_queue)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.queues.Insert(
      runtime::QueueRecord{.native_handle = native_queue,
                           .parent_device_handle = device_handle,
                           .queue_family_index = queue_family_index,
                           .queue_index = queue_index})};
  return WriteGuestObject(out_queue_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t QueueWaitIdle(std::uint64_t queue_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};

  auto *queue_record{context.queues.Find(queue_handle)};
  if (queue_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  auto *device_record{context.devices.Find(queue_record->parent_device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().QueueWaitIdle(device_record->native_handle,
                                         queue_record->native_handle);
}

std::int32_t CreateBuffer(std::uint64_t device_handle,
                          std::uint64_t create_info_address,
                          std::uint64_t out_buffer_address) noexcept {
  if (create_info_address == 0u || out_buffer_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  BufferCreateStorage storage{};
  if (!BuildBufferCreateInfo(create_info_address, storage)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkBuffer native_buffer{};
  auto result{context.backend().CreateBuffer(
      device_record->native_handle, storage.native_create_info, native_buffer)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.buffers.Insert(runtime::BufferRecord{
      .native_handle = native_buffer, .parent_device_handle = device_handle})};
  return WriteGuestObject(out_buffer_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroyBuffer(std::uint64_t device_handle,
                           std::uint64_t buffer_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *buffer_record{context.buffers.Find(buffer_handle)};
  if (device_record == nullptr || buffer_record == nullptr ||
      buffer_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroyBuffer(device_record->native_handle,
                                  buffer_record->native_handle);
  context.buffers.Erase(buffer_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t
GetBufferMemoryRequirements(std::uint64_t device_handle,
                            std::uint64_t buffer_handle,
                            std::uint64_t out_requirements_address) noexcept {
  if (out_requirements_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *buffer_record{context.buffers.Find(buffer_handle)};
  if (device_record == nullptr || buffer_record == nullptr ||
      buffer_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  uwvm_vk_memory_requirements requirements{};
  auto result{context.backend().GetBufferMemoryRequirements(
      device_record->native_handle, buffer_record->native_handle,
      requirements)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  return WriteGuestObject(out_requirements_address, requirements)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t AllocateMemory(std::uint64_t device_handle,
                            std::uint64_t allocate_info_address,
                            std::uint64_t out_memory_address) noexcept {
  if (allocate_info_address == 0u || out_memory_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  vk::native::VkMemoryAllocateInfo allocate_info{};
  if (!BuildMemoryAllocateInfo(allocate_info_address, allocate_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkDeviceMemory native_memory{};
  auto result{context.backend().AllocateMemory(device_record->native_handle,
                                               allocate_info, native_memory)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.memories.Insert(runtime::DeviceMemoryRecord{
      .native_handle = native_memory,
      .parent_device_handle = device_handle,
      .allocation_size = allocate_info.allocationSize,
      .memory_type_index = allocate_info.memoryTypeIndex})};
  return WriteGuestObject(out_memory_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t FreeMemory(std::uint64_t device_handle,
                        std::uint64_t memory_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *memory_record{context.memories.Find(memory_handle)};
  if (device_record == nullptr || memory_record == nullptr ||
      memory_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().FreeMemory(device_record->native_handle,
                               memory_record->native_handle);
  context.memories.Erase(memory_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t BindBufferMemory(std::uint64_t device_handle,
                              std::uint64_t buffer_handle,
                              std::uint64_t memory_handle,
                              std::uint64_t offset) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *buffer_record{context.buffers.Find(buffer_handle)};
  auto *memory_record{context.memories.Find(memory_handle)};
  if (device_record == nullptr || buffer_record == nullptr ||
      memory_record == nullptr ||
      buffer_record->parent_device_handle != device_handle ||
      memory_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().BindBufferMemory(
      device_record->native_handle, buffer_record->native_handle,
      memory_record->native_handle, offset);
}

std::int32_t
CopyGuestToDeviceMemory(std::uint64_t device_handle,
                        std::uint64_t memory_handle,
                        std::uint64_t copy_region_address) noexcept {
  if (copy_region_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  uwvm_vk_memory_copy_region copy_region{};
  if (!ReadCopyRegion(copy_region_address, copy_region)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *memory_record{context.memories.Find(memory_handle)};
  if (device_record == nullptr || memory_record == nullptr ||
      memory_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return CopyBetweenGuestAndMappedMemory(true, context, *device_record,
                                         *memory_record, copy_region);
}

std::int32_t
CopyDeviceMemoryToGuest(std::uint64_t device_handle,
                        std::uint64_t memory_handle,
                        std::uint64_t copy_region_address) noexcept {
  if (copy_region_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  uwvm_vk_memory_copy_region copy_region{};
  if (!ReadCopyRegion(copy_region_address, copy_region)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *memory_record{context.memories.Find(memory_handle)};
  if (device_record == nullptr || memory_record == nullptr ||
      memory_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return CopyBetweenGuestAndMappedMemory(false, context, *device_record,
                                         *memory_record, copy_region);
}

} // namespace uwvm2_vulkan::api
