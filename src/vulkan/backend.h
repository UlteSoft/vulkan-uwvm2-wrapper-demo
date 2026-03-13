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

#ifndef UWVM2_VULKAN_SRC_VULKAN_BACKEND_H_
#define UWVM2_VULKAN_SRC_VULKAN_BACKEND_H_

#include <cstdint>
#include <vector>

#include <uwvm2_vulkan/vulkan_types.h>

#include "vulkan/native_abi.h"

namespace uwvm2_vulkan::vk {

class Backend {
public:
  virtual ~Backend() = default;

  [[nodiscard]] virtual bool LoaderAvailable() = 0;
  [[nodiscard]] virtual std::int32_t
  EnumerateInstanceVersion(std::uint32_t &api_version) = 0;
  [[nodiscard]] virtual std::int32_t EnumerateInstanceExtensionProperties(
      char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) = 0;
  [[nodiscard]] virtual std::int32_t EnumerateInstanceLayerProperties(
      std::vector<uwvm_vk_layer_property> &properties) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateInstance(native::VkInstanceCreateInfo const &create_info,
                 native::VkInstance &instance) = 0;
  virtual void DestroyInstance(native::VkInstance instance) = 0;
  [[nodiscard]] virtual std::int32_t EnumeratePhysicalDevices(
      native::VkInstance instance,
      std::vector<native::VkPhysicalDevice> &physical_devices) = 0;
  [[nodiscard]] virtual std::int32_t EnumerateDeviceExtensionProperties(
      native::VkPhysicalDevice physical_device, char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) = 0;
  [[nodiscard]] virtual std::int32_t GetPhysicalDeviceProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_properties &properties) = 0;
  [[nodiscard]] virtual std::int32_t GetPhysicalDeviceFeatures(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_features &features) = 0;
  [[nodiscard]] virtual std::int32_t GetPhysicalDeviceMemoryProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_memory_properties &properties) = 0;
  [[nodiscard]] virtual std::int32_t GetPhysicalDeviceQueueFamilyProperties(
      native::VkPhysicalDevice physical_device,
      std::vector<uwvm_vk_queue_family_properties> &properties) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateDevice(native::VkPhysicalDevice physical_device,
               native::VkDeviceCreateInfo const &create_info,
               native::VkDevice &device) = 0;
  virtual void DestroyDevice(native::VkDevice device) = 0;
  [[nodiscard]] virtual std::int32_t DeviceWaitIdle(native::VkDevice device) = 0;
  [[nodiscard]] virtual std::int32_t
  GetDeviceQueue(native::VkDevice device, std::uint32_t queue_family_index,
                 std::uint32_t queue_index, native::VkQueue &queue) = 0;
  [[nodiscard]] virtual std::int32_t
  QueueWaitIdle(native::VkDevice device, native::VkQueue queue) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateCommandPool(native::VkDevice device,
                    native::VkCommandPoolCreateInfo const &create_info,
                    native::VkCommandPool &command_pool) = 0;
  virtual void DestroyCommandPool(native::VkDevice device,
                                  native::VkCommandPool command_pool) = 0;
  [[nodiscard]] virtual std::int32_t
  ResetCommandPool(native::VkDevice device, native::VkCommandPool command_pool,
                   std::uint32_t flags) = 0;
  [[nodiscard]] virtual std::int32_t
  AllocateCommandBuffers(
      native::VkDevice device,
      native::VkCommandBufferAllocateInfo const &allocate_info,
      std::vector<native::VkCommandBuffer> &command_buffers) = 0;
  virtual void FreeCommandBuffers(
      native::VkDevice device, native::VkCommandPool command_pool,
      std::vector<native::VkCommandBuffer> const &command_buffers) = 0;
  [[nodiscard]] virtual std::int32_t
  BeginCommandBuffer(native::VkDevice device,
                     native::VkCommandBuffer command_buffer,
                     native::VkCommandBufferBeginInfo const &begin_info) = 0;
  [[nodiscard]] virtual std::int32_t
  EndCommandBuffer(native::VkDevice device,
                   native::VkCommandBuffer command_buffer) = 0;
  [[nodiscard]] virtual std::int32_t
  ResetCommandBuffer(native::VkDevice device,
                     native::VkCommandBuffer command_buffer,
                     std::uint32_t flags) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateShaderModule(native::VkDevice device,
                     native::VkShaderModuleCreateInfo const &create_info,
                     native::VkShaderModule &shader_module) = 0;
  virtual void DestroyShaderModule(native::VkDevice device,
                                   native::VkShaderModule shader_module) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateSemaphore(native::VkDevice device,
                  native::VkSemaphoreCreateInfo const &create_info,
                  native::VkSemaphore &semaphore) = 0;
  virtual void DestroySemaphore(native::VkDevice device,
                                native::VkSemaphore semaphore) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateFence(native::VkDevice device,
              native::VkFenceCreateInfo const &create_info,
              native::VkFence &fence) = 0;
  virtual void DestroyFence(native::VkDevice device, native::VkFence fence) = 0;
  [[nodiscard]] virtual std::int32_t
  GetFenceStatus(native::VkDevice device, native::VkFence fence) = 0;
  [[nodiscard]] virtual std::int32_t
  WaitForFences(native::VkDevice device,
                std::vector<native::VkFence> const &fences, bool wait_all,
                std::uint64_t timeout_nanoseconds) = 0;
  [[nodiscard]] virtual std::int32_t
  ResetFences(native::VkDevice device,
              std::vector<native::VkFence> const &fences) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateBuffer(native::VkDevice device,
               native::VkBufferCreateInfo const &create_info,
               native::VkBuffer &buffer) = 0;
  virtual void DestroyBuffer(native::VkDevice device,
                             native::VkBuffer buffer) = 0;
  [[nodiscard]] virtual std::int32_t
  GetBufferMemoryRequirements(native::VkDevice device, native::VkBuffer buffer,
                              uwvm_vk_memory_requirements &requirements) = 0;
  [[nodiscard]] virtual std::int32_t
  CreateImage(native::VkDevice device,
              native::VkImageCreateInfo const &create_info,
              native::VkImage &image) = 0;
  virtual void DestroyImage(native::VkDevice device, native::VkImage image) = 0;
  [[nodiscard]] virtual std::int32_t
  GetImageMemoryRequirements(native::VkDevice device, native::VkImage image,
                             uwvm_vk_memory_requirements &requirements) = 0;
  [[nodiscard]] virtual std::int32_t
  AllocateMemory(native::VkDevice device,
                 native::VkMemoryAllocateInfo const &allocate_info,
                 native::VkDeviceMemory &memory) = 0;
  virtual void FreeMemory(native::VkDevice device,
                          native::VkDeviceMemory memory) = 0;
  [[nodiscard]] virtual std::int32_t
  BindBufferMemory(native::VkDevice device, native::VkBuffer buffer,
                   native::VkDeviceMemory memory, std::uint64_t offset) = 0;
  [[nodiscard]] virtual std::int32_t
  BindImageMemory(native::VkDevice device, native::VkImage image,
                  native::VkDeviceMemory memory, std::uint64_t offset) = 0;
  [[nodiscard]] virtual std::int32_t
  MapMemory(native::VkDevice device, native::VkDeviceMemory memory,
            std::uint64_t offset, std::uint64_t size, void *&mapped_data) = 0;
  virtual void UnmapMemory(native::VkDevice device,
                           native::VkDeviceMemory memory) = 0;
  [[nodiscard]] virtual std::int32_t
  FlushMappedMemory(native::VkDevice device, native::VkDeviceMemory memory,
                    std::uint64_t offset, std::uint64_t size) = 0;
  [[nodiscard]] virtual std::int32_t
  InvalidateMappedMemory(native::VkDevice device, native::VkDeviceMemory memory,
                         std::uint64_t offset, std::uint64_t size) = 0;
};

class DynamicBackend final : public Backend {
public:
  DynamicBackend();
  ~DynamicBackend() override;

  [[nodiscard]] bool LoaderAvailable() override;
  [[nodiscard]] std::int32_t
  EnumerateInstanceVersion(std::uint32_t &api_version) override;
  [[nodiscard]] std::int32_t EnumerateInstanceExtensionProperties(
      char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override;
  [[nodiscard]] std::int32_t EnumerateInstanceLayerProperties(
      std::vector<uwvm_vk_layer_property> &properties) override;
  [[nodiscard]] std::int32_t
  CreateInstance(native::VkInstanceCreateInfo const &create_info,
                 native::VkInstance &instance) override;
  void DestroyInstance(native::VkInstance instance) override;
  [[nodiscard]] std::int32_t EnumeratePhysicalDevices(
      native::VkInstance instance,
      std::vector<native::VkPhysicalDevice> &physical_devices) override;
  [[nodiscard]] std::int32_t EnumerateDeviceExtensionProperties(
      native::VkPhysicalDevice physical_device, char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override;
  [[nodiscard]] std::int32_t GetPhysicalDeviceProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_properties &properties) override;
  [[nodiscard]] std::int32_t GetPhysicalDeviceFeatures(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_features &features) override;
  [[nodiscard]] std::int32_t GetPhysicalDeviceMemoryProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_memory_properties &properties) override;
  [[nodiscard]] std::int32_t GetPhysicalDeviceQueueFamilyProperties(
      native::VkPhysicalDevice physical_device,
      std::vector<uwvm_vk_queue_family_properties> &properties) override;
  [[nodiscard]] std::int32_t
  CreateDevice(native::VkPhysicalDevice physical_device,
               native::VkDeviceCreateInfo const &create_info,
               native::VkDevice &device) override;
  void DestroyDevice(native::VkDevice device) override;
  [[nodiscard]] std::int32_t DeviceWaitIdle(native::VkDevice device) override;
  [[nodiscard]] std::int32_t GetDeviceQueue(native::VkDevice device,
                                            std::uint32_t queue_family_index,
                                            std::uint32_t queue_index,
                                            native::VkQueue &queue) override;
  [[nodiscard]] std::int32_t
  QueueWaitIdle(native::VkDevice device, native::VkQueue queue) override;
  [[nodiscard]] std::int32_t
  CreateCommandPool(native::VkDevice device,
                    native::VkCommandPoolCreateInfo const &create_info,
                    native::VkCommandPool &command_pool) override;
  void DestroyCommandPool(native::VkDevice device,
                          native::VkCommandPool command_pool) override;
  [[nodiscard]] std::int32_t
  ResetCommandPool(native::VkDevice device, native::VkCommandPool command_pool,
                   std::uint32_t flags) override;
  [[nodiscard]] std::int32_t AllocateCommandBuffers(
      native::VkDevice device,
      native::VkCommandBufferAllocateInfo const &allocate_info,
      std::vector<native::VkCommandBuffer> &command_buffers) override;
  void FreeCommandBuffers(
      native::VkDevice device, native::VkCommandPool command_pool,
      std::vector<native::VkCommandBuffer> const &command_buffers) override;
  [[nodiscard]] std::int32_t
  BeginCommandBuffer(native::VkDevice device,
                     native::VkCommandBuffer command_buffer,
                     native::VkCommandBufferBeginInfo const &begin_info) override;
  [[nodiscard]] std::int32_t
  EndCommandBuffer(native::VkDevice device,
                   native::VkCommandBuffer command_buffer) override;
  [[nodiscard]] std::int32_t
  ResetCommandBuffer(native::VkDevice device,
                     native::VkCommandBuffer command_buffer,
                     std::uint32_t flags) override;
  [[nodiscard]] std::int32_t
  CreateShaderModule(native::VkDevice device,
                     native::VkShaderModuleCreateInfo const &create_info,
                     native::VkShaderModule &shader_module) override;
  void DestroyShaderModule(native::VkDevice device,
                           native::VkShaderModule shader_module) override;
  [[nodiscard]] std::int32_t
  CreateSemaphore(native::VkDevice device,
                  native::VkSemaphoreCreateInfo const &create_info,
                  native::VkSemaphore &semaphore) override;
  void DestroySemaphore(native::VkDevice device,
                        native::VkSemaphore semaphore) override;
  [[nodiscard]] std::int32_t
  CreateFence(native::VkDevice device,
              native::VkFenceCreateInfo const &create_info,
              native::VkFence &fence) override;
  void DestroyFence(native::VkDevice device, native::VkFence fence) override;
  [[nodiscard]] std::int32_t GetFenceStatus(native::VkDevice device,
                                            native::VkFence fence) override;
  [[nodiscard]] std::int32_t
  WaitForFences(native::VkDevice device,
                std::vector<native::VkFence> const &fences, bool wait_all,
                std::uint64_t timeout_nanoseconds) override;
  [[nodiscard]] std::int32_t
  ResetFences(native::VkDevice device,
              std::vector<native::VkFence> const &fences) override;
  [[nodiscard]] std::int32_t
  CreateBuffer(native::VkDevice device,
               native::VkBufferCreateInfo const &create_info,
               native::VkBuffer &buffer) override;
  void DestroyBuffer(native::VkDevice device, native::VkBuffer buffer) override;
  [[nodiscard]] std::int32_t GetBufferMemoryRequirements(
      native::VkDevice device, native::VkBuffer buffer,
      uwvm_vk_memory_requirements &requirements) override;
  [[nodiscard]] std::int32_t
  CreateImage(native::VkDevice device,
              native::VkImageCreateInfo const &create_info,
              native::VkImage &image) override;
  void DestroyImage(native::VkDevice device, native::VkImage image) override;
  [[nodiscard]] std::int32_t GetImageMemoryRequirements(
      native::VkDevice device, native::VkImage image,
      uwvm_vk_memory_requirements &requirements) override;
  [[nodiscard]] std::int32_t
  AllocateMemory(native::VkDevice device,
                 native::VkMemoryAllocateInfo const &allocate_info,
                 native::VkDeviceMemory &memory) override;
  void FreeMemory(native::VkDevice device,
                  native::VkDeviceMemory memory) override;
  [[nodiscard]] std::int32_t BindBufferMemory(native::VkDevice device,
                                              native::VkBuffer buffer,
                                              native::VkDeviceMemory memory,
                                              std::uint64_t offset) override;
  [[nodiscard]] std::int32_t BindImageMemory(native::VkDevice device,
                                             native::VkImage image,
                                             native::VkDeviceMemory memory,
                                             std::uint64_t offset) override;
  [[nodiscard]] std::int32_t MapMemory(native::VkDevice device,
                                       native::VkDeviceMemory memory,
                                       std::uint64_t offset, std::uint64_t size,
                                       void *&mapped_data) override;
  void UnmapMemory(native::VkDevice device,
                   native::VkDeviceMemory memory) override;
  [[nodiscard]] std::int32_t FlushMappedMemory(native::VkDevice device,
                                               native::VkDeviceMemory memory,
                                               std::uint64_t offset,
                                               std::uint64_t size) override;
  [[nodiscard]] std::int32_t
  InvalidateMappedMemory(native::VkDevice device, native::VkDeviceMemory memory,
                         std::uint64_t offset, std::uint64_t size) override;

private:
  struct Impl;
  Impl *impl_{};
};

} // namespace uwvm2_vulkan::vk

#endif
