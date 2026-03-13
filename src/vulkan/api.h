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

#ifndef UWVM2_VULKAN_SRC_VULKAN_API_H_
#define UWVM2_VULKAN_SRC_VULKAN_API_H_

#include <cstdint>

namespace uwvm2_vulkan::api {

[[nodiscard]] std::int32_t LoaderAvailable() noexcept;
[[nodiscard]] std::int32_t
EnumerateInstanceVersion(std::uint64_t out_api_version_address) noexcept;
[[nodiscard]] std::int32_t EnumerateInstanceExtensionProperties(
    std::uint64_t layer_name_address, std::uint64_t property_buffer_address,
    std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept;
[[nodiscard]] std::int32_t EnumerateInstanceLayerProperties(
    std::uint64_t property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept;
[[nodiscard]] std::int32_t
CreateInstance(std::uint64_t create_info_address,
               std::uint64_t out_instance_address) noexcept;
[[nodiscard]] std::int32_t
DestroyInstance(std::uint64_t instance_handle) noexcept;
[[nodiscard]] std::int32_t
EnumeratePhysicalDevices(std::uint64_t instance_handle,
                         std::uint64_t out_device_buffer_address,
                         std::uint32_t device_capacity,
                         std::uint64_t out_device_count_address) noexcept;
[[nodiscard]] std::int32_t EnumerateDeviceExtensionProperties(
    std::uint64_t physical_device_handle, std::uint64_t layer_name_address,
    std::uint64_t property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept;
[[nodiscard]] std::int32_t
GetPhysicalDeviceProperties(std::uint64_t physical_device_handle,
                            std::uint64_t out_properties_address) noexcept;
[[nodiscard]] std::int32_t GetPhysicalDeviceFeatures(
    std::uint64_t physical_device_handle,
    std::uint64_t out_features_address) noexcept;
[[nodiscard]] std::int32_t GetPhysicalDeviceMemoryProperties(
    std::uint64_t physical_device_handle,
    std::uint64_t out_properties_address) noexcept;
[[nodiscard]] std::int32_t GetPhysicalDeviceQueueFamilyProperties(
    std::uint64_t physical_device_handle,
    std::uint64_t out_property_buffer_address, std::uint32_t property_capacity,
    std::uint64_t out_property_count_address) noexcept;
[[nodiscard]] std::int32_t
CreateDevice(std::uint64_t physical_device_handle,
             std::uint64_t create_info_address,
             std::uint64_t out_device_address) noexcept;
[[nodiscard]] std::int32_t DestroyDevice(std::uint64_t device_handle) noexcept;
[[nodiscard]] std::int32_t DeviceWaitIdle(std::uint64_t device_handle) noexcept;
[[nodiscard]] std::int32_t
GetDeviceQueue(std::uint64_t device_handle, std::uint32_t queue_family_index,
               std::uint32_t queue_index,
               std::uint64_t out_queue_address) noexcept;
[[nodiscard]] std::int32_t QueueWaitIdle(std::uint64_t queue_handle) noexcept;
[[nodiscard]] std::int32_t
CreateCommandPool(std::uint64_t device_handle,
                  std::uint64_t create_info_address,
                  std::uint64_t out_command_pool_address) noexcept;
[[nodiscard]] std::int32_t DestroyCommandPool(
    std::uint64_t device_handle, std::uint64_t command_pool_handle) noexcept;
[[nodiscard]] std::int32_t ResetCommandPool(std::uint64_t device_handle,
                                            std::uint64_t command_pool_handle,
                                            std::uint32_t flags) noexcept;
[[nodiscard]] std::int32_t
AllocateCommandBuffers(std::uint64_t device_handle,
                       std::uint64_t allocate_info_address,
                       std::uint64_t out_command_buffer_buffer_address) noexcept;
[[nodiscard]] std::int32_t FreeCommandBuffers(
    std::uint64_t device_handle, std::uint64_t command_pool_handle,
    std::uint64_t command_buffer_handle_buffer_address,
    std::uint32_t command_buffer_count) noexcept;
[[nodiscard]] std::int32_t
BeginCommandBuffer(std::uint64_t device_handle,
                   std::uint64_t command_buffer_handle,
                   std::uint64_t begin_info_address) noexcept;
[[nodiscard]] std::int32_t
EndCommandBuffer(std::uint64_t device_handle,
                 std::uint64_t command_buffer_handle) noexcept;
[[nodiscard]] std::int32_t
ResetCommandBuffer(std::uint64_t device_handle,
                   std::uint64_t command_buffer_handle,
                   std::uint32_t flags) noexcept;
[[nodiscard]] std::int32_t
CreateShaderModule(std::uint64_t device_handle,
                   std::uint64_t create_info_address,
                   std::uint64_t out_shader_module_address) noexcept;
[[nodiscard]] std::int32_t
DestroyShaderModule(std::uint64_t device_handle,
                    std::uint64_t shader_module_handle) noexcept;
[[nodiscard]] std::int32_t
CreateSemaphore(std::uint64_t device_handle, std::uint64_t create_info_address,
                std::uint64_t out_semaphore_address) noexcept;
[[nodiscard]] std::int32_t
DestroySemaphore(std::uint64_t device_handle,
                 std::uint64_t semaphore_handle) noexcept;
[[nodiscard]] std::int32_t
CreateFence(std::uint64_t device_handle, std::uint64_t create_info_address,
            std::uint64_t out_fence_address) noexcept;
[[nodiscard]] std::int32_t DestroyFence(std::uint64_t device_handle,
                                        std::uint64_t fence_handle) noexcept;
[[nodiscard]] std::int32_t GetFenceStatus(std::uint64_t device_handle,
                                          std::uint64_t fence_handle) noexcept;
[[nodiscard]] std::int32_t
WaitForFences(std::uint64_t device_handle,
              std::uint64_t fence_handle_buffer_address,
              std::uint32_t fence_count, std::uint32_t wait_all,
              std::uint64_t timeout_nanoseconds) noexcept;
[[nodiscard]] std::int32_t ResetFences(std::uint64_t device_handle,
                                       std::uint64_t fence_handle_buffer_address,
                                       std::uint32_t fence_count) noexcept;
[[nodiscard]] std::int32_t
CreateBuffer(std::uint64_t device_handle, std::uint64_t create_info_address,
             std::uint64_t out_buffer_address) noexcept;
[[nodiscard]] std::int32_t DestroyBuffer(std::uint64_t device_handle,
                                         std::uint64_t buffer_handle) noexcept;
[[nodiscard]] std::int32_t
GetBufferMemoryRequirements(std::uint64_t device_handle,
                            std::uint64_t buffer_handle,
                            std::uint64_t out_requirements_address) noexcept;
[[nodiscard]] std::int32_t
CreateImage(std::uint64_t device_handle, std::uint64_t create_info_address,
            std::uint64_t out_image_address) noexcept;
[[nodiscard]] std::int32_t DestroyImage(std::uint64_t device_handle,
                                        std::uint64_t image_handle) noexcept;
[[nodiscard]] std::int32_t
GetImageMemoryRequirements(std::uint64_t device_handle,
                           std::uint64_t image_handle,
                           std::uint64_t out_requirements_address) noexcept;
[[nodiscard]] std::int32_t
AllocateMemory(std::uint64_t device_handle, std::uint64_t allocate_info_address,
               std::uint64_t out_memory_address) noexcept;
[[nodiscard]] std::int32_t FreeMemory(std::uint64_t device_handle,
                                      std::uint64_t memory_handle) noexcept;
[[nodiscard]] std::int32_t BindBufferMemory(std::uint64_t device_handle,
                                            std::uint64_t buffer_handle,
                                            std::uint64_t memory_handle,
                                            std::uint64_t offset) noexcept;
[[nodiscard]] std::int32_t BindImageMemory(std::uint64_t device_handle,
                                           std::uint64_t image_handle,
                                           std::uint64_t memory_handle,
                                           std::uint64_t offset) noexcept;
[[nodiscard]] std::int32_t
CopyGuestToDeviceMemory(std::uint64_t device_handle,
                        std::uint64_t memory_handle,
                        std::uint64_t copy_region_address) noexcept;
[[nodiscard]] std::int32_t
CopyDeviceMemoryToGuest(std::uint64_t device_handle,
                        std::uint64_t memory_handle,
                        std::uint64_t copy_region_address) noexcept;

} // namespace uwvm2_vulkan::api

#endif
