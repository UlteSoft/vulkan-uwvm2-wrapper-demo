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
#include "vulkan/backend.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace uwvm2_vulkan::vk {

namespace {

class DynamicLibrary {
public:
  DynamicLibrary() = default;
  DynamicLibrary(DynamicLibrary const &) = delete;
  DynamicLibrary &operator=(DynamicLibrary const &) = delete;

  ~DynamicLibrary() { Close(); }

  [[nodiscard]] bool Open(char const *path) noexcept {
    Close();
#if defined(_WIN32)
    handle_ = ::LoadLibraryA(path);
#else
    handle_ = ::dlopen(path, RTLD_LAZY | RTLD_LOCAL);
#endif
    return handle_ != nullptr;
  }

  void Close() noexcept {
    if (handle_ == nullptr) {
      return;
    }
#if defined(_WIN32)
    ::FreeLibrary(handle_);
#else
    ::dlclose(handle_);
#endif
    handle_ = nullptr;
  }

  template <typename Symbol>
  [[nodiscard]] Symbol Load(char const *symbol_name) const noexcept {
    if (handle_ == nullptr) {
      return nullptr;
    }
#if defined(_WIN32)
    return reinterpret_cast<Symbol>(::GetProcAddress(handle_, symbol_name));
#else
    return reinterpret_cast<Symbol>(::dlsym(handle_, symbol_name));
#endif
  }

private:
#if defined(_WIN32)
  HMODULE handle_{};
#else
  void *handle_{};
#endif
};

struct PhysicalDevicePropertiesPrefix {
  std::uint32_t api_version;
  std::uint32_t driver_version;
  std::uint32_t vendor_id;
  std::uint32_t device_id;
  std::uint32_t device_type;
  char device_name[UWVM_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
  std::uint8_t pipeline_cache_uuid[UWVM_VK_UUID_SIZE];
};

char const *const kLibraryCandidates[] = {
#if defined(_WIN32)
    "vulkan-1.dll",
#elif defined(__APPLE__)
    "libvulkan.1.dylib",
    "libvulkan.dylib",
    "libMoltenVK.dylib",
#else
    "libvulkan.so.1",
    "libvulkan.so",
#endif
};

} // namespace

struct DynamicBackend::Impl {
  std::mutex mutex{};
  bool loader_attempted{};
  bool loader_available{};
  DynamicLibrary library{};
  native::PFN_vkGetInstanceProcAddr get_instance_proc_addr{};
  native::PFN_vkGetDeviceProcAddr get_device_proc_addr{};
  native::PFN_vkEnumerateInstanceVersion enumerate_instance_version{};
  native::PFN_vkEnumerateInstanceExtensionProperties
      enumerate_instance_extension_properties{};
  native::PFN_vkEnumerateInstanceLayerProperties
      enumerate_instance_layer_properties{};
  native::PFN_vkCreateInstance create_instance{};

  [[nodiscard]] bool EnsureLoaderLocked() {
    if (loader_attempted) {
      return loader_available;
    }

    loader_attempted = true;
    for (auto const *candidate : kLibraryCandidates) {
      if (library.Open(candidate)) {
        break;
      }
    }

    if (library.Load<void *>("vkGetInstanceProcAddr") == nullptr) {
      loader_available = false;
      library.Close();
      return false;
    }

    get_instance_proc_addr = library.Load<native::PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr");
    if (get_instance_proc_addr == nullptr) {
      loader_available = false;
      library.Close();
      return false;
    }

    get_device_proc_addr = reinterpret_cast<native::PFN_vkGetDeviceProcAddr>(
        get_instance_proc_addr(nullptr, "vkGetDeviceProcAddr"));
    if (get_device_proc_addr == nullptr) {
      get_device_proc_addr =
          library.Load<native::PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
    }

    enumerate_instance_version =
        LoadGlobalLocked<native::PFN_vkEnumerateInstanceVersion>(
            "vkEnumerateInstanceVersion");
    enumerate_instance_extension_properties =
        LoadGlobalLocked<native::PFN_vkEnumerateInstanceExtensionProperties>(
            "vkEnumerateInstanceExtensionProperties");
    enumerate_instance_layer_properties =
        LoadGlobalLocked<native::PFN_vkEnumerateInstanceLayerProperties>(
            "vkEnumerateInstanceLayerProperties");
    create_instance =
        LoadGlobalLocked<native::PFN_vkCreateInstance>("vkCreateInstance");

    loader_available = create_instance != nullptr &&
                       enumerate_instance_extension_properties != nullptr &&
                       enumerate_instance_layer_properties != nullptr;
    if (!loader_available) {
      library.Close();
    }
    return loader_available;
  }

  template <typename Symbol>
  [[nodiscard]] Symbol LoadGlobalLocked(char const *symbol_name) {
    if (get_instance_proc_addr != nullptr) {
      auto const loaded{reinterpret_cast<Symbol>(
          get_instance_proc_addr(nullptr, symbol_name))};
      if (loaded != nullptr) {
        return loaded;
      }
    }
    return library.Load<Symbol>(symbol_name);
  }

  template <typename Symbol>
  [[nodiscard]] Symbol LoadInstanceLocked(native::VkInstance instance,
                                          char const *symbol_name) {
    if (get_instance_proc_addr == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<Symbol>(
        get_instance_proc_addr(instance, symbol_name));
  }

  template <typename Symbol>
  [[nodiscard]] Symbol LoadDeviceLocked(native::VkDevice device,
                                        char const *symbol_name) {
    if (get_device_proc_addr != nullptr) {
      return reinterpret_cast<Symbol>(
          get_device_proc_addr(device, symbol_name));
    }
    return nullptr;
  }
};

DynamicBackend::DynamicBackend() : impl_(new Impl{}) {}

DynamicBackend::~DynamicBackend() { delete impl_; }

bool DynamicBackend::LoaderAvailable() {
  std::scoped_lock lock{impl_->mutex};
  return impl_->EnsureLoaderLocked();
}

std::int32_t
DynamicBackend::EnumerateInstanceVersion(std::uint32_t &api_version) {
  std::scoped_lock lock{impl_->mutex};
  if (!impl_->EnsureLoaderLocked()) {
    return UWVM_VK_ERROR_LOADER_UNAVAILABLE;
  }
  if (impl_->enumerate_instance_version == nullptr) {
    api_version = (1u << 22u);
    return UWVM_VK_SUCCESS;
  }
  return impl_->enumerate_instance_version(&api_version);
}

std::int32_t DynamicBackend::EnumerateInstanceExtensionProperties(
    char const *layer_name,
    std::vector<uwvm_vk_extension_property> &properties) {
  properties.clear();

  std::scoped_lock lock{impl_->mutex};
  if (!impl_->EnsureLoaderLocked()) {
    return UWVM_VK_ERROR_LOADER_UNAVAILABLE;
  }

  std::uint32_t count{};
  auto result{impl_->enumerate_instance_extension_properties(layer_name, &count,
                                                             nullptr)};
  if (result < 0) {
    return result;
  }

  std::vector<native::VkExtensionProperty> native_properties(count);
  if (count != 0u) {
    result = impl_->enumerate_instance_extension_properties(
        layer_name, &count, native_properties.data());
    if (result < 0) {
      return result;
    }
  }

  properties.resize(count);
  for (std::uint32_t index{}; index != count; ++index) {
    std::memset(properties[index].extension_name, 0,
                sizeof(properties[index].extension_name));
    std::memcpy(properties[index].extension_name,
                native_properties[index].extensionName,
                sizeof(properties[index].extension_name));
    properties[index].spec_version = native_properties[index].specVersion;
  }

  return result;
}

std::int32_t DynamicBackend::EnumerateInstanceLayerProperties(
    std::vector<uwvm_vk_layer_property> &properties) {
  properties.clear();

  std::scoped_lock lock{impl_->mutex};
  if (!impl_->EnsureLoaderLocked()) {
    return UWVM_VK_ERROR_LOADER_UNAVAILABLE;
  }

  std::uint32_t count{};
  auto result{impl_->enumerate_instance_layer_properties(&count, nullptr)};
  if (result < 0) {
    return result;
  }

  std::vector<native::VkLayerProperty> native_properties(count);
  if (count != 0u) {
    result = impl_->enumerate_instance_layer_properties(
        &count, native_properties.data());
    if (result < 0) {
      return result;
    }
  }

  properties.resize(count);
  for (std::uint32_t index{}; index != count; ++index) {
    std::memset(properties[index].layer_name, 0,
                sizeof(properties[index].layer_name));
    std::memcpy(properties[index].layer_name,
                native_properties[index].layerName,
                sizeof(properties[index].layer_name));
    properties[index].spec_version = native_properties[index].specVersion;
    properties[index].implementation_version =
        native_properties[index].implementationVersion;
    std::memset(properties[index].description, 0,
                sizeof(properties[index].description));
    std::memcpy(properties[index].description,
                native_properties[index].description,
                sizeof(properties[index].description));
  }

  return result;
}

std::int32_t
DynamicBackend::CreateInstance(native::VkInstanceCreateInfo const &create_info,
                               native::VkInstance &instance) {
  std::scoped_lock lock{impl_->mutex};
  if (!impl_->EnsureLoaderLocked()) {
    return UWVM_VK_ERROR_LOADER_UNAVAILABLE;
  }
  return impl_->create_instance(&create_info, nullptr, &instance);
}

void DynamicBackend::DestroyInstance(native::VkInstance instance) {
  std::scoped_lock lock{impl_->mutex};
  auto const destroy_instance{
      impl_->LoadInstanceLocked<native::PFN_vkDestroyInstance>(
          instance, "vkDestroyInstance")};
  if (destroy_instance != nullptr) {
    destroy_instance(instance, nullptr);
  }
}

std::int32_t DynamicBackend::EnumeratePhysicalDevices(
    native::VkInstance instance,
    std::vector<native::VkPhysicalDevice> &physical_devices) {
  physical_devices.clear();

  std::scoped_lock lock{impl_->mutex};
  auto const enumerate_physical_devices{
      impl_->LoadInstanceLocked<native::PFN_vkEnumeratePhysicalDevices>(
          instance, "vkEnumeratePhysicalDevices")};
  if (enumerate_physical_devices == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::uint32_t count{};
  auto result{enumerate_physical_devices(instance, &count, nullptr)};
  if (result < 0) {
    return result;
  }

  physical_devices.resize(count);
  if (count != 0u) {
    result =
        enumerate_physical_devices(instance, &count, physical_devices.data());
  }
  return result;
}

std::int32_t DynamicBackend::EnumerateDeviceExtensionProperties(
    native::VkPhysicalDevice physical_device, char const *layer_name,
    std::vector<uwvm_vk_extension_property> &properties) {
  properties.clear();

  std::scoped_lock lock{impl_->mutex};
  auto const enumerate_device_extension_properties{impl_->LoadGlobalLocked<
      native::PFN_vkEnumerateDeviceExtensionProperties>(
      "vkEnumerateDeviceExtensionProperties")};
  if (enumerate_device_extension_properties == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::uint32_t count{};
  auto result{enumerate_device_extension_properties(physical_device, layer_name,
                                                    &count, nullptr)};
  if (result < 0) {
    return result;
  }

  std::vector<native::VkExtensionProperty> native_properties(count);
  if (count != 0u) {
    result = enumerate_device_extension_properties(
        physical_device, layer_name, &count, native_properties.data());
    if (result < 0) {
      return result;
    }
  }

  properties.resize(count);
  for (std::uint32_t index{}; index != count; ++index) {
    std::memset(properties[index].extension_name, 0,
                sizeof(properties[index].extension_name));
    std::memcpy(properties[index].extension_name,
                native_properties[index].extensionName,
                sizeof(properties[index].extension_name));
    properties[index].spec_version = native_properties[index].specVersion;
  }

  return result;
}

std::int32_t DynamicBackend::GetPhysicalDeviceProperties(
    native::VkPhysicalDevice physical_device,
    uwvm_vk_physical_device_properties &properties) {
  std::scoped_lock lock{impl_->mutex};
  auto const get_physical_device_properties{
      impl_->LoadGlobalLocked<native::PFN_vkGetPhysicalDeviceProperties>(
          "vkGetPhysicalDeviceProperties")};
  if (get_physical_device_properties == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::array<std::byte, 4096u> raw_properties{};
  get_physical_device_properties(physical_device, raw_properties.data());

  auto const *prefix{reinterpret_cast<PhysicalDevicePropertiesPrefix const *>(
      raw_properties.data())};
  properties.api_version = prefix->api_version;
  properties.driver_version = prefix->driver_version;
  properties.vendor_id = prefix->vendor_id;
  properties.device_id = prefix->device_id;
  properties.device_type = prefix->device_type;
  std::memcpy(properties.pipeline_cache_uuid, prefix->pipeline_cache_uuid,
              sizeof(properties.pipeline_cache_uuid));
  std::memset(properties.device_name, 0, sizeof(properties.device_name));
  std::memcpy(properties.device_name, prefix->device_name,
              sizeof(properties.device_name));
  return UWVM_VK_SUCCESS;
}

std::int32_t DynamicBackend::GetPhysicalDeviceFeatures(
    native::VkPhysicalDevice physical_device,
    uwvm_vk_physical_device_features &features) {
  std::scoped_lock lock{impl_->mutex};
  auto const get_physical_device_features{
      impl_->LoadGlobalLocked<native::PFN_vkGetPhysicalDeviceFeatures>(
          "vkGetPhysicalDeviceFeatures")};
  if (get_physical_device_features == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  native::VkPhysicalDeviceFeatures native_features{};
  get_physical_device_features(physical_device, &native_features);
  std::memcpy(&features, &native_features, sizeof(features));
  return UWVM_VK_SUCCESS;
}

std::int32_t DynamicBackend::GetPhysicalDeviceMemoryProperties(
    native::VkPhysicalDevice physical_device,
    uwvm_vk_physical_device_memory_properties &properties) {
  std::scoped_lock lock{impl_->mutex};
  auto const get_memory_properties{
      impl_->LoadGlobalLocked<native::PFN_vkGetPhysicalDeviceMemoryProperties>(
          "vkGetPhysicalDeviceMemoryProperties")};
  if (get_memory_properties == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  native::VkPhysicalDeviceMemoryProperties native_properties{};
  get_memory_properties(physical_device, &native_properties);

  properties.memory_type_count =
      std::min(native_properties.memoryTypeCount,
               static_cast<std::uint32_t>(UWVM_VK_MAX_MEMORY_TYPES));
  properties.memory_heap_count =
      std::min(native_properties.memoryHeapCount,
               static_cast<std::uint32_t>(UWVM_VK_MAX_MEMORY_HEAPS));

  for (std::uint32_t index{}; index != properties.memory_type_count; ++index) {
    properties.memory_types[index].property_flags =
        native_properties.memoryTypes[index].propertyFlags;
    properties.memory_types[index].heap_index =
        native_properties.memoryTypes[index].heapIndex;
  }
  for (std::uint32_t index{}; index != properties.memory_heap_count; ++index) {
    properties.memory_heaps[index].size =
        native_properties.memoryHeaps[index].size;
    properties.memory_heaps[index].flags =
        native_properties.memoryHeaps[index].flags;
    properties.memory_heaps[index].reserved = 0u;
  }

  return UWVM_VK_SUCCESS;
}

std::int32_t DynamicBackend::GetPhysicalDeviceQueueFamilyProperties(
    native::VkPhysicalDevice physical_device,
    std::vector<uwvm_vk_queue_family_properties> &properties) {
  properties.clear();

  std::scoped_lock lock{impl_->mutex};
  auto const get_queue_family_properties{impl_->LoadGlobalLocked<
      native::PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
      "vkGetPhysicalDeviceQueueFamilyProperties")};
  if (get_queue_family_properties == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::uint32_t count{};
  get_queue_family_properties(physical_device, &count, nullptr);

  std::vector<native::VkQueueFamilyProperties> native_properties(count);
  if (count != 0u) {
    get_queue_family_properties(physical_device, &count,
                                native_properties.data());
  }

  properties.resize(count);
  for (std::uint32_t index{}; index != count; ++index) {
    properties[index].queue_flags = native_properties[index].queueFlags;
    properties[index].queue_count = native_properties[index].queueCount;
    properties[index].timestamp_valid_bits =
        native_properties[index].timestampValidBits;
    properties[index].min_image_transfer_granularity_x =
        native_properties[index].minImageTransferGranularity.width;
    properties[index].min_image_transfer_granularity_y =
        native_properties[index].minImageTransferGranularity.height;
    properties[index].min_image_transfer_granularity_z =
        native_properties[index].minImageTransferGranularity.depth;
  }

  return UWVM_VK_SUCCESS;
}

std::int32_t
DynamicBackend::CreateDevice(native::VkPhysicalDevice physical_device,
                             native::VkDeviceCreateInfo const &create_info,
                             native::VkDevice &device) {
  std::scoped_lock lock{impl_->mutex};
  auto const create_device{
      impl_->LoadGlobalLocked<native::PFN_vkCreateDevice>("vkCreateDevice")};
  if (create_device == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  return create_device(physical_device, &create_info, nullptr, &device);
}

void DynamicBackend::DestroyDevice(native::VkDevice device) {
  std::scoped_lock lock{impl_->mutex};
  auto const destroy_device{
      impl_->LoadDeviceLocked<native::PFN_vkDestroyDevice>(device,
                                                           "vkDestroyDevice")};
  if (destroy_device != nullptr) {
    destroy_device(device, nullptr);
  }
}

std::int32_t DynamicBackend::DeviceWaitIdle(native::VkDevice device) {
  std::scoped_lock lock{impl_->mutex};
  auto const device_wait_idle{
      impl_->LoadDeviceLocked<native::PFN_vkDeviceWaitIdle>(
          device, "vkDeviceWaitIdle")};
  if (device_wait_idle == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  return device_wait_idle(device);
}

std::int32_t DynamicBackend::GetDeviceQueue(native::VkDevice device,
                                            std::uint32_t queue_family_index,
                                            std::uint32_t queue_index,
                                            native::VkQueue &queue) {
  std::scoped_lock lock{impl_->mutex};
  auto const get_device_queue{
      impl_->LoadDeviceLocked<native::PFN_vkGetDeviceQueue>(
          device, "vkGetDeviceQueue")};
  if (get_device_queue == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  get_device_queue(device, queue_family_index, queue_index, &queue);
  return UWVM_VK_SUCCESS;
}

std::int32_t DynamicBackend::QueueWaitIdle(native::VkDevice device,
                                           native::VkQueue queue) {
  std::scoped_lock lock{impl_->mutex};
  auto const queue_wait_idle{
      impl_->LoadDeviceLocked<native::PFN_vkQueueWaitIdle>(
          device, "vkQueueWaitIdle")};
  if (queue_wait_idle == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  return queue_wait_idle(queue);
}

std::int32_t
DynamicBackend::CreateBuffer(native::VkDevice device,
                             native::VkBufferCreateInfo const &create_info,
                             native::VkBuffer &buffer) {
  std::scoped_lock lock{impl_->mutex};
  auto const create_buffer{impl_->LoadDeviceLocked<native::PFN_vkCreateBuffer>(
      device, "vkCreateBuffer")};
  if (create_buffer == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  return create_buffer(device, &create_info, nullptr, &buffer);
}

void DynamicBackend::DestroyBuffer(native::VkDevice device,
                                   native::VkBuffer buffer) {
  std::scoped_lock lock{impl_->mutex};
  auto const destroy_buffer{
      impl_->LoadDeviceLocked<native::PFN_vkDestroyBuffer>(device,
                                                           "vkDestroyBuffer")};
  if (destroy_buffer != nullptr) {
    destroy_buffer(device, buffer, nullptr);
  }
}

std::int32_t DynamicBackend::GetBufferMemoryRequirements(
    native::VkDevice device, native::VkBuffer buffer,
    uwvm_vk_memory_requirements &requirements) {
  std::scoped_lock lock{impl_->mutex};
  auto const get_requirements{
      impl_->LoadDeviceLocked<native::PFN_vkGetBufferMemoryRequirements>(
          device, "vkGetBufferMemoryRequirements")};
  if (get_requirements == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  native::VkMemoryRequirements native_requirements{};
  get_requirements(device, buffer, &native_requirements);
  requirements.size = native_requirements.size;
  requirements.alignment = native_requirements.alignment;
  requirements.memory_type_bits = native_requirements.memoryTypeBits;
  requirements.reserved = 0u;
  return UWVM_VK_SUCCESS;
}

std::int32_t DynamicBackend::AllocateMemory(
    native::VkDevice device, native::VkMemoryAllocateInfo const &allocate_info,
    native::VkDeviceMemory &memory) {
  std::scoped_lock lock{impl_->mutex};
  auto const allocate_memory{
      impl_->LoadDeviceLocked<native::PFN_vkAllocateMemory>(
          device, "vkAllocateMemory")};
  if (allocate_memory == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  return allocate_memory(device, &allocate_info, nullptr, &memory);
}

void DynamicBackend::FreeMemory(native::VkDevice device,
                                native::VkDeviceMemory memory) {
  std::scoped_lock lock{impl_->mutex};
  auto const free_memory{impl_->LoadDeviceLocked<native::PFN_vkFreeMemory>(
      device, "vkFreeMemory")};
  if (free_memory != nullptr) {
    free_memory(device, memory, nullptr);
  }
}

std::int32_t DynamicBackend::BindBufferMemory(native::VkDevice device,
                                              native::VkBuffer buffer,
                                              native::VkDeviceMemory memory,
                                              std::uint64_t offset) {
  std::scoped_lock lock{impl_->mutex};
  auto const bind_buffer_memory{
      impl_->LoadDeviceLocked<native::PFN_vkBindBufferMemory>(
          device, "vkBindBufferMemory")};
  if (bind_buffer_memory == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  return bind_buffer_memory(device, buffer, memory, offset);
}

std::int32_t DynamicBackend::MapMemory(native::VkDevice device,
                                       native::VkDeviceMemory memory,
                                       std::uint64_t offset, std::uint64_t size,
                                       void *&mapped_data) {
  std::scoped_lock lock{impl_->mutex};
  auto const map_memory{
      impl_->LoadDeviceLocked<native::PFN_vkMapMemory>(device, "vkMapMemory")};
  if (map_memory == nullptr) {
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }
  return map_memory(device, memory, offset, size, 0u, &mapped_data);
}

void DynamicBackend::UnmapMemory(native::VkDevice device,
                                 native::VkDeviceMemory memory) {
  std::scoped_lock lock{impl_->mutex};
  auto const unmap_memory{impl_->LoadDeviceLocked<native::PFN_vkUnmapMemory>(
      device, "vkUnmapMemory")};
  if (unmap_memory != nullptr) {
    unmap_memory(device, memory);
  }
}

std::int32_t DynamicBackend::FlushMappedMemory(native::VkDevice device,
                                               native::VkDeviceMemory memory,
                                               std::uint64_t offset,
                                               std::uint64_t size) {
  std::scoped_lock lock{impl_->mutex};
  auto const flush{
      impl_->LoadDeviceLocked<native::PFN_vkFlushMappedMemoryRanges>(
          device, "vkFlushMappedMemoryRanges")};
  if (flush == nullptr) {
    return UWVM_VK_SUCCESS;
  }

  native::VkMappedMemoryRange range{
      .sType = native::VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = nullptr,
      .memory = memory,
      .offset = offset,
      .size = size};
  return flush(device, 1u, &range);
}

std::int32_t DynamicBackend::InvalidateMappedMemory(
    native::VkDevice device, native::VkDeviceMemory memory,
    std::uint64_t offset, std::uint64_t size) {
  std::scoped_lock lock{impl_->mutex};
  auto const invalidate{
      impl_->LoadDeviceLocked<native::PFN_vkInvalidateMappedMemoryRanges>(
          device, "vkInvalidateMappedMemoryRanges")};
  if (invalidate == nullptr) {
    return UWVM_VK_SUCCESS;
  }

  native::VkMappedMemoryRange range{
      .sType = native::VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = nullptr,
      .memory = memory,
      .offset = offset,
      .size = size};
  return invalidate(device, 1u, &range);
}

} // namespace uwvm2_vulkan::vk
