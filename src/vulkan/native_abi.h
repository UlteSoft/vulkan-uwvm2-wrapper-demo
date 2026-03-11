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

#ifndef UWVM2_VULKAN_SRC_VULKAN_NATIVE_ABI_H_
#define UWVM2_VULKAN_SRC_VULKAN_NATIVE_ABI_H_

#include <cstddef>
#include <cstdint>

namespace uwvm2_vulkan::vk::native {

using VkFlags = std::uint32_t;
using VkBool32 = std::uint32_t;
using VkDeviceSize = std::uint64_t;
using VkResult = std::int32_t;

struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkDevice_T;
struct VkQueue_T;

using VkInstance = VkInstance_T *;
using VkPhysicalDevice = VkPhysicalDevice_T *;
using VkDevice = VkDevice_T *;
using VkQueue = VkQueue_T *;
using VkBuffer = std::uint64_t;
using VkImage = std::uint64_t;
using VkDeviceMemory = std::uint64_t;
using VkFormat = std::uint32_t;
using VkSampleCountFlagBits = std::uint32_t;

inline constexpr std::size_t kMaxExtensionNameSize{256u};
inline constexpr std::size_t kMaxDescriptionSize{256u};
inline constexpr std::size_t kMaxDeviceNameSize{256u};
inline constexpr std::size_t kUuidSize{16u};
inline constexpr VkDeviceSize kWholeSize{~static_cast<VkDeviceSize>(0)};

enum VkStructureType : std::uint32_t {
  VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
  VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
  VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE = 6,
  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
  VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14,
  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 17
};

enum VkSharingMode : std::uint32_t {
  VK_SHARING_MODE_EXCLUSIVE = 0,
  VK_SHARING_MODE_CONCURRENT = 1
};

enum VkImageType : std::uint32_t {
  VK_IMAGE_TYPE_1D = 0,
  VK_IMAGE_TYPE_2D = 1,
  VK_IMAGE_TYPE_3D = 2
};

enum VkImageTiling : std::uint32_t {
  VK_IMAGE_TILING_OPTIMAL = 0,
  VK_IMAGE_TILING_LINEAR = 1
};

enum VkImageLayout : std::uint32_t {
  VK_IMAGE_LAYOUT_UNDEFINED = 0,
  VK_IMAGE_LAYOUT_GENERAL = 1,
  VK_IMAGE_LAYOUT_PREINITIALIZED = 8
};

struct VkApplicationInfo {
  VkStructureType sType;
  void const *pNext;
  char const *pApplicationName;
  std::uint32_t applicationVersion;
  char const *pEngineName;
  std::uint32_t engineVersion;
  std::uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
  VkStructureType sType;
  void const *pNext;
  VkFlags flags;
  VkApplicationInfo const *pApplicationInfo;
  std::uint32_t enabledLayerCount;
  char const *const *ppEnabledLayerNames;
  std::uint32_t enabledExtensionCount;
  char const *const *ppEnabledExtensionNames;
};

struct VkExtensionProperty {
  char extensionName[kMaxExtensionNameSize];
  std::uint32_t specVersion;
};

struct VkLayerProperty {
  char layerName[kMaxExtensionNameSize];
  std::uint32_t specVersion;
  std::uint32_t implementationVersion;
  char description[kMaxDescriptionSize];
};

struct VkQueueFamilyProperties {
  VkFlags queueFlags;
  std::uint32_t queueCount;
  std::uint32_t timestampValidBits;
  struct {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
  } minImageTransferGranularity;
};

struct VkMemoryType {
  VkFlags propertyFlags;
  std::uint32_t heapIndex;
};

struct VkMemoryHeap {
  VkDeviceSize size;
  VkFlags flags;
};

struct VkPhysicalDeviceMemoryProperties {
  std::uint32_t memoryTypeCount;
  VkMemoryType memoryTypes[32];
  std::uint32_t memoryHeapCount;
  VkMemoryHeap memoryHeaps[16];
};

struct VkDeviceQueueCreateInfo {
  VkStructureType sType;
  void const *pNext;
  VkFlags flags;
  std::uint32_t queueFamilyIndex;
  std::uint32_t queueCount;
  float const *pQueuePriorities;
};

struct VkPhysicalDeviceFeatures {
  std::uint32_t placeholder[55];
};

struct VkDeviceCreateInfo {
  VkStructureType sType;
  void const *pNext;
  VkFlags flags;
  std::uint32_t queueCreateInfoCount;
  VkDeviceQueueCreateInfo const *pQueueCreateInfos;
  std::uint32_t enabledLayerCount;
  char const *const *ppEnabledLayerNames;
  std::uint32_t enabledExtensionCount;
  char const *const *ppEnabledExtensionNames;
  VkPhysicalDeviceFeatures const *pEnabledFeatures;
};

struct VkBufferCreateInfo {
  VkStructureType sType;
  void const *pNext;
  VkFlags flags;
  VkDeviceSize size;
  VkFlags usage;
  VkSharingMode sharingMode;
  std::uint32_t queueFamilyIndexCount;
  std::uint32_t const *pQueueFamilyIndices;
};

struct VkExtent3D {
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t depth;
};

struct VkImageCreateInfo {
  VkStructureType sType;
  void const *pNext;
  VkFlags flags;
  VkImageType imageType;
  VkFormat format;
  VkExtent3D extent;
  std::uint32_t mipLevels;
  std::uint32_t arrayLayers;
  VkSampleCountFlagBits samples;
  VkImageTiling tiling;
  VkFlags usage;
  VkSharingMode sharingMode;
  std::uint32_t queueFamilyIndexCount;
  std::uint32_t const *pQueueFamilyIndices;
  VkImageLayout initialLayout;
};

struct VkMemoryRequirements {
  VkDeviceSize size;
  VkDeviceSize alignment;
  std::uint32_t memoryTypeBits;
};

struct VkMemoryAllocateInfo {
  VkStructureType sType;
  void const *pNext;
  VkDeviceSize allocationSize;
  std::uint32_t memoryTypeIndex;
};

struct VkMappedMemoryRange {
  VkStructureType sType;
  void const *pNext;
  VkDeviceMemory memory;
  VkDeviceSize offset;
  VkDeviceSize size;
};

using PFN_vkVoidFunction = void (*)();
using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (*)(VkInstance,
                                                         char const *);
using PFN_vkGetDeviceProcAddr = PFN_vkVoidFunction (*)(VkDevice, char const *);
using PFN_vkEnumerateInstanceVersion = VkResult (*)(std::uint32_t *);
using PFN_vkEnumerateInstanceExtensionProperties =
    VkResult (*)(char const *, std::uint32_t *, VkExtensionProperty *);
using PFN_vkEnumerateInstanceLayerProperties = VkResult (*)(std::uint32_t *,
                                                            VkLayerProperty *);
using PFN_vkCreateInstance = VkResult (*)(VkInstanceCreateInfo const *,
                                          void const *, VkInstance *);
using PFN_vkDestroyInstance = void (*)(VkInstance, void const *);
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(VkInstance, std::uint32_t *,
                                                    VkPhysicalDevice *);
using PFN_vkEnumerateDeviceExtensionProperties =
    VkResult (*)(VkPhysicalDevice, char const *, std::uint32_t *,
                 VkExtensionProperty *);
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, void *);
using PFN_vkGetPhysicalDeviceFeatures = void (*)(VkPhysicalDevice,
                                                 VkPhysicalDeviceFeatures *);
using PFN_vkGetPhysicalDeviceMemoryProperties =
    void (*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);
using PFN_vkGetPhysicalDeviceQueueFamilyProperties =
    void (*)(VkPhysicalDevice, std::uint32_t *, VkQueueFamilyProperties *);
using PFN_vkCreateDevice = VkResult (*)(VkPhysicalDevice,
                                        VkDeviceCreateInfo const *,
                                        void const *, VkDevice *);
using PFN_vkDestroyDevice = void (*)(VkDevice, void const *);
using PFN_vkDeviceWaitIdle = VkResult (*)(VkDevice);
using PFN_vkGetDeviceQueue = void (*)(VkDevice, std::uint32_t, std::uint32_t,
                                      VkQueue *);
using PFN_vkQueueWaitIdle = VkResult (*)(VkQueue);
using PFN_vkCreateBuffer = VkResult (*)(VkDevice, VkBufferCreateInfo const *,
                                        void const *, VkBuffer *);
using PFN_vkDestroyBuffer = void (*)(VkDevice, VkBuffer, void const *);
using PFN_vkGetBufferMemoryRequirements = void (*)(VkDevice, VkBuffer,
                                                   VkMemoryRequirements *);
using PFN_vkCreateImage = VkResult (*)(VkDevice, VkImageCreateInfo const *,
                                       void const *, VkImage *);
using PFN_vkDestroyImage = void (*)(VkDevice, VkImage, void const *);
using PFN_vkGetImageMemoryRequirements = void (*)(VkDevice, VkImage,
                                                  VkMemoryRequirements *);
using PFN_vkAllocateMemory = VkResult (*)(VkDevice,
                                          VkMemoryAllocateInfo const *,
                                          void const *, VkDeviceMemory *);
using PFN_vkFreeMemory = void (*)(VkDevice, VkDeviceMemory, void const *);
using PFN_vkBindBufferMemory = VkResult (*)(VkDevice, VkBuffer, VkDeviceMemory,
                                            VkDeviceSize);
using PFN_vkBindImageMemory = VkResult (*)(VkDevice, VkImage, VkDeviceMemory,
                                           VkDeviceSize);
using PFN_vkMapMemory = VkResult (*)(VkDevice, VkDeviceMemory, VkDeviceSize,
                                     VkDeviceSize, VkFlags, void **);
using PFN_vkUnmapMemory = void (*)(VkDevice, VkDeviceMemory);
using PFN_vkFlushMappedMemoryRanges = VkResult (*)(VkDevice, std::uint32_t,
                                                   VkMappedMemoryRange const *);
using PFN_vkInvalidateMappedMemoryRanges =
    VkResult (*)(VkDevice, std::uint32_t, VkMappedMemoryRange const *);

} // namespace uwvm2_vulkan::vk::native

#endif
