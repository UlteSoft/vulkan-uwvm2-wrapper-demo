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
[[nodiscard]] bool WriteGuestArray(std::uint64_t address,
                                   std::span<T const> values) {
  if (values.empty()) {
    return true;
  }
  if (address == 0u) {
    return false;
  }

  auto &memory{runtime::PluginContext::Instance().memory()};
  return memory.Write(kDefaultGuestMemoryIndex, address, values.data(),
                      sizeof(T) * values.size());
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

struct ImageCreateStorage {
  uwvm_vk_image_create_info guest_create_info{};
  std::vector<std::uint32_t> queue_family_indices{};
  vk::native::VkImageCreateInfo native_create_info{};
};

[[nodiscard]] bool BuildImageCreateInfo(std::uint64_t create_info_address,
                                        ImageCreateStorage &storage) {
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

  storage.native_create_info = vk::native::VkImageCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = storage.guest_create_info.flags,
      .imageType = static_cast<vk::native::VkImageType>(
          storage.guest_create_info.image_type),
      .format = storage.guest_create_info.format,
      .extent =
          vk::native::VkExtent3D{
              .width = storage.guest_create_info.extent.width,
              .height = storage.guest_create_info.extent.height,
              .depth = storage.guest_create_info.extent.depth},
      .mipLevels = storage.guest_create_info.mip_levels,
      .arrayLayers = storage.guest_create_info.array_layers,
      .samples = storage.guest_create_info.samples,
      .tiling = static_cast<vk::native::VkImageTiling>(
          storage.guest_create_info.tiling),
      .usage = storage.guest_create_info.usage,
      .sharingMode = static_cast<vk::native::VkSharingMode>(
          storage.guest_create_info.sharing_mode),
      .queueFamilyIndexCount =
          storage.guest_create_info.queue_family_index_count,
      .pQueueFamilyIndices = storage.queue_family_indices.empty()
                                 ? nullptr
                                 : storage.queue_family_indices.data(),
      .initialLayout = static_cast<vk::native::VkImageLayout>(
          storage.guest_create_info.initial_layout)};
  return true;
}

[[nodiscard]] bool BuildSemaphoreCreateInfo(
    std::uint64_t create_info_address,
    vk::native::VkSemaphoreCreateInfo &create_info) {
  uwvm_vk_semaphore_create_info guest_create_info{};
  if (!ReadGuestObject(create_info_address, guest_create_info)) {
    return false;
  }

  create_info = vk::native::VkSemaphoreCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = guest_create_info.flags};
  return true;
}

[[nodiscard]] bool BuildFenceCreateInfo(
    std::uint64_t create_info_address,
    vk::native::VkFenceCreateInfo &create_info) {
  uwvm_vk_fence_create_info guest_create_info{};
  if (!ReadGuestObject(create_info_address, guest_create_info)) {
    return false;
  }

  create_info = vk::native::VkFenceCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = guest_create_info.flags};
  return true;
}

[[nodiscard]] bool BuildCommandPoolCreateInfo(
    std::uint64_t create_info_address,
    vk::native::VkCommandPoolCreateInfo &create_info) {
  uwvm_vk_command_pool_create_info guest_create_info{};
  if (!ReadGuestObject(create_info_address, guest_create_info)) {
    return false;
  }

  create_info = vk::native::VkCommandPoolCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = guest_create_info.flags,
      .queueFamilyIndex = guest_create_info.queue_family_index};
  return true;
}

[[nodiscard]] bool BuildCommandBufferAllocateInfo(
    std::uint64_t allocate_info_address,
    uwvm_vk_command_buffer_allocate_info &allocate_info) {
  return ReadGuestObject(allocate_info_address, allocate_info);
}

[[nodiscard]] bool BuildCommandBufferBeginInfo(
    std::uint64_t begin_info_address,
    vk::native::VkCommandBufferBeginInfo &begin_info) {
  uwvm_vk_command_buffer_begin_info guest_begin_info{};
  if (!ReadGuestObject(begin_info_address, guest_begin_info)) {
    return false;
  }

  begin_info = vk::native::VkCommandBufferBeginInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = guest_begin_info.flags,
      .pInheritanceInfo = nullptr};
  return true;
}

struct ShaderModuleCreateStorage {
  uwvm_vk_shader_module_create_info guest_create_info{};
  std::vector<std::uint32_t> code_words{};
  vk::native::VkShaderModuleCreateInfo native_create_info{};
};

[[nodiscard]] bool BuildShaderModuleCreateInfo(
    std::uint64_t create_info_address, ShaderModuleCreateStorage &storage) {
  if (!ReadGuestObject(create_info_address, storage.guest_create_info)) {
    return false;
  }
  if (storage.guest_create_info.code_address == 0u ||
      storage.guest_create_info.code_size == 0u ||
      (storage.guest_create_info.code_size % sizeof(std::uint32_t)) != 0u ||
      storage.guest_create_info.code_size >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return false;
  }

  auto const word_count{
      static_cast<std::size_t>(storage.guest_create_info.code_size /
                               sizeof(std::uint32_t))};
  storage.code_words.resize(word_count);
  auto &memory{runtime::PluginContext::Instance().memory()};
  if (!memory.Read(kDefaultGuestMemoryIndex, storage.guest_create_info.code_address,
                   storage.code_words.data(),
                   static_cast<std::size_t>(
                       storage.guest_create_info.code_size))) {
    return false;
  }

  storage.native_create_info = vk::native::VkShaderModuleCreateInfo{
      .sType = vk::native::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = storage.guest_create_info.flags,
      .codeSize = static_cast<std::size_t>(storage.guest_create_info.code_size),
      .pCode = storage.code_words.data()};
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

[[nodiscard]] std::int32_t ResolveFenceList(
    runtime::PluginContext &context, std::uint64_t device_handle,
    std::uint64_t fence_handle_buffer_address, std::uint32_t fence_count,
    std::vector<vk::native::VkFence> &native_fences) {
  native_fences.clear();
  if (fence_count == 0u || fence_handle_buffer_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::vector<std::uint64_t> guest_fences{};
  if (!ReadGuestArray(fence_handle_buffer_address, fence_count, guest_fences)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  native_fences.reserve(guest_fences.size());
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  for (auto const fence_handle : guest_fences) {
    auto *fence_record{context.fences.Find(fence_handle)};
    if (fence_record == nullptr ||
        fence_record->parent_device_handle != device_handle) {
      native_fences.clear();
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }
    native_fences.push_back(fence_record->native_handle);
  }

  return UWVM_VK_SUCCESS;
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
  context.images.ForEach([&](std::uint64_t handle,
                             runtime::ImageRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().DestroyImage(device_record->native_handle,
                                       record.native_handle);
      }
    }
  });
  context.semaphores.ForEach([&](std::uint64_t handle,
                                 runtime::SemaphoreRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().DestroySemaphore(device_record->native_handle,
                                           record.native_handle);
      }
    }
  });
  context.fences.ForEach([&](std::uint64_t handle,
                             runtime::FenceRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().DestroyFence(device_record->native_handle,
                                       record.native_handle);
      }
    }
  });
  context.command_pools.ForEach([&](std::uint64_t handle,
                                    runtime::CommandPoolRecord const &record) {
    (void)handle;
    if (device_handles.contains(record.parent_device_handle)) {
      auto *device_record{context.devices.Find(record.parent_device_handle)};
      if (device_record != nullptr) {
        context.backend().DestroyCommandPool(device_record->native_handle,
                                             record.native_handle);
      }
    }
  });
  context.shader_modules.ForEach(
      [&](std::uint64_t handle, runtime::ShaderModuleRecord const &record) {
        (void)handle;
        if (device_handles.contains(record.parent_device_handle)) {
          auto *device_record{context.devices.Find(record.parent_device_handle)};
          if (device_record != nullptr) {
            context.backend().DestroyShaderModule(device_record->native_handle,
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
  context.command_buffers.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.command_pools.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.shader_modules.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.buffers.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.images.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.semaphores.EraseIf([&](auto const &record) {
    return device_handles.contains(record.parent_device_handle);
  });
  context.fences.EraseIf([&](auto const &record) {
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
  context.images.ForEach([&](std::uint64_t handle,
                             runtime::ImageRecord const &record) {
    (void)handle;
    if (record.parent_device_handle == device_handle) {
      context.backend().DestroyImage(device_record->native_handle,
                                     record.native_handle);
    }
  });
  context.semaphores.ForEach([&](std::uint64_t handle,
                                 runtime::SemaphoreRecord const &record) {
    (void)handle;
    if (record.parent_device_handle == device_handle) {
      context.backend().DestroySemaphore(device_record->native_handle,
                                         record.native_handle);
    }
  });
  context.fences.ForEach([&](std::uint64_t handle,
                             runtime::FenceRecord const &record) {
    (void)handle;
    if (record.parent_device_handle == device_handle) {
      context.backend().DestroyFence(device_record->native_handle,
                                     record.native_handle);
    }
  });
  context.command_pools.ForEach(
      [&](std::uint64_t handle, runtime::CommandPoolRecord const &record) {
        (void)handle;
        if (record.parent_device_handle == device_handle) {
          context.backend().DestroyCommandPool(device_record->native_handle,
                                               record.native_handle);
        }
      });
  context.shader_modules.ForEach(
      [&](std::uint64_t handle, runtime::ShaderModuleRecord const &record) {
        (void)handle;
        if (record.parent_device_handle == device_handle) {
          context.backend().DestroyShaderModule(device_record->native_handle,
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
  context.command_buffers.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.command_pools.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.shader_modules.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.buffers.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.images.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.semaphores.EraseIf([device_handle](auto const &record) {
    return record.parent_device_handle == device_handle;
  });
  context.fences.EraseIf([device_handle](auto const &record) {
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

std::int32_t CreateCommandPool(std::uint64_t device_handle,
                               std::uint64_t create_info_address,
                               std::uint64_t out_command_pool_address) noexcept {
  if (create_info_address == 0u || out_command_pool_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  vk::native::VkCommandPoolCreateInfo create_info{};
  if (!BuildCommandPoolCreateInfo(create_info_address, create_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkCommandPool native_command_pool{};
  auto result{context.backend().CreateCommandPool(
      device_record->native_handle, create_info, native_command_pool)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.command_pools.Insert(runtime::CommandPoolRecord{
      .native_handle = native_command_pool,
      .parent_device_handle = device_handle,
      .queue_family_index = create_info.queueFamilyIndex})};
  if (!WriteGuestObject(out_command_pool_address, handle)) {
    context.command_pools.Erase(handle);
    context.backend().DestroyCommandPool(device_record->native_handle,
                                         native_command_pool);
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }
  return UWVM_VK_SUCCESS;
}

std::int32_t DestroyCommandPool(std::uint64_t device_handle,
                                std::uint64_t command_pool_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_pool_record{context.command_pools.Find(command_pool_handle)};
  if (device_record == nullptr || command_pool_record == nullptr ||
      command_pool_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroyCommandPool(device_record->native_handle,
                                       command_pool_record->native_handle);
  context.command_buffers.EraseIf([command_pool_handle](auto const &record) {
    return record.parent_command_pool_handle == command_pool_handle;
  });
  context.command_pools.Erase(command_pool_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t ResetCommandPool(std::uint64_t device_handle,
                              std::uint64_t command_pool_handle,
                              std::uint32_t flags) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_pool_record{context.command_pools.Find(command_pool_handle)};
  if (device_record == nullptr || command_pool_record == nullptr ||
      command_pool_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().ResetCommandPool(device_record->native_handle,
                                            command_pool_record->native_handle,
                                            flags);
}

std::int32_t AllocateCommandBuffers(
    std::uint64_t device_handle, std::uint64_t allocate_info_address,
    std::uint64_t out_command_buffer_buffer_address) noexcept {
  if (allocate_info_address == 0u || out_command_buffer_buffer_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  uwvm_vk_command_buffer_allocate_info guest_allocate_info{};
  if (!BuildCommandBufferAllocateInfo(allocate_info_address,
                                      guest_allocate_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }
  if (guest_allocate_info.command_pool_handle == 0u ||
      guest_allocate_info.command_buffer_count == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_pool_record{
      context.command_pools.Find(guest_allocate_info.command_pool_handle)};
  if (device_record == nullptr || command_pool_record == nullptr ||
      command_pool_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkCommandBufferAllocateInfo allocate_info{
      .sType = vk::native::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = command_pool_record->native_handle,
      .level = static_cast<vk::native::VkCommandBufferLevel>(
          guest_allocate_info.level),
      .commandBufferCount = guest_allocate_info.command_buffer_count};

  std::vector<vk::native::VkCommandBuffer> native_command_buffers{};
  auto result{context.backend().AllocateCommandBuffers(
      device_record->native_handle, allocate_info, native_command_buffers)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }
  if (native_command_buffers.size() !=
      guest_allocate_info.command_buffer_count) {
    context.backend().FreeCommandBuffers(device_record->native_handle,
                                         command_pool_record->native_handle,
                                         native_command_buffers);
    return UWVM_VK_ERROR_UNKNOWN;
  }

  std::vector<std::uint64_t> guest_command_buffer_handles{};
  guest_command_buffer_handles.reserve(native_command_buffers.size());
  for (auto const native_command_buffer : native_command_buffers) {
    guest_command_buffer_handles.push_back(
        context.command_buffers.Insert(runtime::CommandBufferRecord{
            .native_handle = native_command_buffer,
            .parent_device_handle = device_handle,
            .parent_command_pool_handle =
                guest_allocate_info.command_pool_handle,
            .level = guest_allocate_info.level}));
  }

  if (!WriteGuestArray(
          out_command_buffer_buffer_address,
          std::span<std::uint64_t const>{guest_command_buffer_handles.data(),
                                         guest_command_buffer_handles.size()})) {
    for (auto const handle : guest_command_buffer_handles) {
      context.command_buffers.Erase(handle);
    }
    context.backend().FreeCommandBuffers(device_record->native_handle,
                                         command_pool_record->native_handle,
                                         native_command_buffers);
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }
  return UWVM_VK_SUCCESS;
}

std::int32_t FreeCommandBuffers(
    std::uint64_t device_handle, std::uint64_t command_pool_handle,
    std::uint64_t command_buffer_handle_buffer_address,
    std::uint32_t command_buffer_count) noexcept {
  if (command_pool_handle == 0u || command_buffer_handle_buffer_address == 0u ||
      command_buffer_count == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  std::vector<std::uint64_t> guest_command_buffer_handles{};
  if (!ReadGuestArray(command_buffer_handle_buffer_address, command_buffer_count,
                      guest_command_buffer_handles)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_pool_record{context.command_pools.Find(command_pool_handle)};
  if (device_record == nullptr || command_pool_record == nullptr ||
      command_pool_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  std::vector<vk::native::VkCommandBuffer> native_command_buffers{};
  native_command_buffers.reserve(guest_command_buffer_handles.size());
  for (auto const command_buffer_handle : guest_command_buffer_handles) {
    auto *command_buffer_record{
        context.command_buffers.Find(command_buffer_handle)};
    if (command_buffer_record == nullptr ||
        command_buffer_record->parent_device_handle != device_handle ||
        command_buffer_record->parent_command_pool_handle !=
            command_pool_handle) {
      return UWVM_VK_ERROR_INVALID_HANDLE;
    }
    native_command_buffers.push_back(command_buffer_record->native_handle);
  }

  context.backend().FreeCommandBuffers(device_record->native_handle,
                                       command_pool_record->native_handle,
                                       native_command_buffers);
  for (auto const command_buffer_handle : guest_command_buffer_handles) {
    context.command_buffers.Erase(command_buffer_handle);
  }
  return UWVM_VK_SUCCESS;
}

std::int32_t BeginCommandBuffer(std::uint64_t device_handle,
                                std::uint64_t command_buffer_handle,
                                std::uint64_t begin_info_address) noexcept {
  if (begin_info_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  vk::native::VkCommandBufferBeginInfo begin_info{};
  if (!BuildCommandBufferBeginInfo(begin_info_address, begin_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }
  if ((begin_info.flags &
       UWVM_VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) != 0u) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_buffer_record{context.command_buffers.Find(command_buffer_handle)};
  if (device_record == nullptr || command_buffer_record == nullptr ||
      command_buffer_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }
  if (command_buffer_record->level != UWVM_VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  return context.backend().BeginCommandBuffer(device_record->native_handle,
                                              command_buffer_record->native_handle,
                                              begin_info);
}

std::int32_t EndCommandBuffer(std::uint64_t device_handle,
                              std::uint64_t command_buffer_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_buffer_record{context.command_buffers.Find(command_buffer_handle)};
  if (device_record == nullptr || command_buffer_record == nullptr ||
      command_buffer_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().EndCommandBuffer(device_record->native_handle,
                                            command_buffer_record->native_handle);
}

std::int32_t ResetCommandBuffer(std::uint64_t device_handle,
                                std::uint64_t command_buffer_handle,
                                std::uint32_t flags) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *command_buffer_record{context.command_buffers.Find(command_buffer_handle)};
  if (device_record == nullptr || command_buffer_record == nullptr ||
      command_buffer_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().ResetCommandBuffer(device_record->native_handle,
                                              command_buffer_record->native_handle,
                                              flags);
}

std::int32_t CreateShaderModule(std::uint64_t device_handle,
                                std::uint64_t create_info_address,
                                std::uint64_t out_shader_module_address) noexcept {
  if (create_info_address == 0u || out_shader_module_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  ShaderModuleCreateStorage storage{};
  if (!BuildShaderModuleCreateInfo(create_info_address, storage)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkShaderModule native_shader_module{};
  auto result{context.backend().CreateShaderModule(
      device_record->native_handle, storage.native_create_info,
      native_shader_module)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.shader_modules.Insert(runtime::ShaderModuleRecord{
      .native_handle = native_shader_module,
      .parent_device_handle = device_handle})};
  return WriteGuestObject(out_shader_module_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroyShaderModule(std::uint64_t device_handle,
                                 std::uint64_t shader_module_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *shader_module_record{context.shader_modules.Find(shader_module_handle)};
  if (device_record == nullptr || shader_module_record == nullptr ||
      shader_module_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroyShaderModule(device_record->native_handle,
                                        shader_module_record->native_handle);
  context.shader_modules.Erase(shader_module_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t CreateSemaphore(std::uint64_t device_handle,
                             std::uint64_t create_info_address,
                             std::uint64_t out_semaphore_address) noexcept {
  if (create_info_address == 0u || out_semaphore_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  vk::native::VkSemaphoreCreateInfo create_info{};
  if (!BuildSemaphoreCreateInfo(create_info_address, create_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkSemaphore native_semaphore{};
  auto result{context.backend().CreateSemaphore(device_record->native_handle,
                                                create_info,
                                                native_semaphore)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.semaphores.Insert(runtime::SemaphoreRecord{
      .native_handle = native_semaphore, .parent_device_handle = device_handle})};
  return WriteGuestObject(out_semaphore_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroySemaphore(std::uint64_t device_handle,
                              std::uint64_t semaphore_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *semaphore_record{context.semaphores.Find(semaphore_handle)};
  if (device_record == nullptr || semaphore_record == nullptr ||
      semaphore_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroySemaphore(device_record->native_handle,
                                     semaphore_record->native_handle);
  context.semaphores.Erase(semaphore_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t CreateFence(std::uint64_t device_handle,
                         std::uint64_t create_info_address,
                         std::uint64_t out_fence_address) noexcept {
  if (create_info_address == 0u || out_fence_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  vk::native::VkFenceCreateInfo create_info{};
  if (!BuildFenceCreateInfo(create_info_address, create_info)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkFence native_fence{};
  auto result{
      context.backend().CreateFence(device_record->native_handle, create_info,
                                    native_fence)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.fences.Insert(runtime::FenceRecord{
      .native_handle = native_fence, .parent_device_handle = device_handle})};
  return WriteGuestObject(out_fence_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroyFence(std::uint64_t device_handle,
                          std::uint64_t fence_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *fence_record{context.fences.Find(fence_handle)};
  if (device_record == nullptr || fence_record == nullptr ||
      fence_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroyFence(device_record->native_handle,
                                 fence_record->native_handle);
  context.fences.Erase(fence_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t GetFenceStatus(std::uint64_t device_handle,
                            std::uint64_t fence_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *fence_record{context.fences.Find(fence_handle)};
  if (device_record == nullptr || fence_record == nullptr ||
      fence_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().GetFenceStatus(device_record->native_handle,
                                          fence_record->native_handle);
}

std::int32_t WaitForFences(std::uint64_t device_handle,
                           std::uint64_t fence_handle_buffer_address,
                           std::uint32_t fence_count, std::uint32_t wait_all,
                           std::uint64_t timeout_nanoseconds) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::vector<vk::native::VkFence> native_fences{};
  auto result{ResolveFenceList(context, device_handle,
                               fence_handle_buffer_address, fence_count,
                               native_fences)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().WaitForFences(device_record->native_handle,
                                         native_fences, wait_all != 0u,
                                         timeout_nanoseconds);
}

std::int32_t ResetFences(std::uint64_t device_handle,
                         std::uint64_t fence_handle_buffer_address,
                         std::uint32_t fence_count) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::vector<vk::native::VkFence> native_fences{};
  auto result{ResolveFenceList(context, device_handle,
                               fence_handle_buffer_address, fence_count,
                               native_fences)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().ResetFences(device_record->native_handle,
                                       native_fences);
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

std::int32_t CreateImage(std::uint64_t device_handle,
                         std::uint64_t create_info_address,
                         std::uint64_t out_image_address) noexcept {
  if (create_info_address == 0u || out_image_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  ImageCreateStorage storage{};
  if (!BuildImageCreateInfo(create_info_address, storage)) {
    return UWVM_VK_ERROR_GUEST_MEMORY;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  if (device_record == nullptr) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  vk::native::VkImage native_image{};
  auto result{context.backend().CreateImage(
      device_record->native_handle, storage.native_create_info, native_image)};
  if (result != UWVM_VK_SUCCESS) {
    return result;
  }

  auto const handle{context.images.Insert(runtime::ImageRecord{
      .native_handle = native_image, .parent_device_handle = device_handle})};
  return WriteGuestObject(out_image_address, handle)
             ? UWVM_VK_SUCCESS
             : UWVM_VK_ERROR_GUEST_MEMORY;
}

std::int32_t DestroyImage(std::uint64_t device_handle,
                          std::uint64_t image_handle) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *image_record{context.images.Find(image_handle)};
  if (device_record == nullptr || image_record == nullptr ||
      image_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  context.backend().DestroyImage(device_record->native_handle,
                                 image_record->native_handle);
  context.images.Erase(image_handle);
  return UWVM_VK_SUCCESS;
}

std::int32_t
GetImageMemoryRequirements(std::uint64_t device_handle,
                           std::uint64_t image_handle,
                           std::uint64_t out_requirements_address) noexcept {
  if (out_requirements_address == 0u) {
    return UWVM_VK_ERROR_INVALID_ARGUMENT;
  }

  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *image_record{context.images.Find(image_handle)};
  if (device_record == nullptr || image_record == nullptr ||
      image_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  uwvm_vk_memory_requirements requirements{};
  auto result{context.backend().GetImageMemoryRequirements(
      device_record->native_handle, image_record->native_handle, requirements)};
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

std::int32_t BindImageMemory(std::uint64_t device_handle,
                             std::uint64_t image_handle,
                             std::uint64_t memory_handle,
                             std::uint64_t offset) noexcept {
  auto &context{runtime::PluginContext::Instance()};
  std::scoped_lock lock{context.mutex()};
  auto *device_record{context.devices.Find(device_handle)};
  auto *image_record{context.images.Find(image_handle)};
  auto *memory_record{context.memories.Find(memory_handle)};
  if (device_record == nullptr || image_record == nullptr ||
      memory_record == nullptr ||
      image_record->parent_device_handle != device_handle ||
      memory_record->parent_device_handle != device_handle) {
    return UWVM_VK_ERROR_INVALID_HANDLE;
  }

  return context.backend().BindImageMemory(
      device_record->native_handle, image_record->native_handle,
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
