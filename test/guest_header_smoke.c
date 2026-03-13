#include <stdint.h>

#include <uwvm2_vulkan/vulkan.h>
#include <uwvm2_vulkan/wasi_snapshot_preview1.h>

int guest_header_smoke_use(void) {
  uint64_t handle = 0;
  uint64_t image_handle = 0;
  uint64_t memory_handle = 0;
  uint64_t semaphore_handle = 0;
  uint64_t fence_handle = 0;
  uint64_t command_pool_handle = 0;
  uint64_t command_buffer_handle = 0;
  uint64_t shader_module_handle = 0;
  uint64_t guest_address = 0;
  uwvm_vk_physical_device_features features = {0};
  uwvm_vk_image_create_info image_create_info = {0};
  uwvm_vk_semaphore_create_info semaphore_create_info = {0};
  uwvm_vk_fence_create_info fence_create_info = {0};
  uwvm_vk_command_pool_create_info command_pool_create_info = {0};
  uwvm_vk_command_buffer_allocate_info command_buffer_allocate_info = {0};
  uwvm_vk_command_buffer_begin_info command_buffer_begin_info = {0};
  uwvm_vk_shader_module_create_info shader_module_create_info = {0};
  int32_t result =
      __imported_vulkan_create_instance(guest_address, guest_address);
  result += __imported_vulkan_destroy_instance(handle);
  result += __imported_vulkan_enumerate_device_extension_properties(
      handle, guest_address, guest_address, 0u, guest_address);
  result += __imported_vulkan_get_physical_device_features(handle, guest_address);
  result += __imported_vulkan_device_wait_idle(handle);
  result += __imported_vulkan_queue_wait_idle(handle);
  result += __imported_vulkan_create_command_pool(handle, guest_address,
                                                  guest_address);
  result += __imported_vulkan_destroy_command_pool(handle, command_pool_handle);
  result += __imported_vulkan_reset_command_pool(handle, command_pool_handle, 0u);
  result += __imported_vulkan_allocate_command_buffers(handle, guest_address,
                                                       guest_address);
  result += __imported_vulkan_free_command_buffers(handle, command_pool_handle,
                                                   guest_address, 0u);
  result += __imported_vulkan_begin_command_buffer(handle, command_buffer_handle,
                                                   guest_address);
  result += __imported_vulkan_end_command_buffer(handle, command_buffer_handle);
  result += __imported_vulkan_reset_command_buffer(handle,
                                                   command_buffer_handle, 0u);
  result += __imported_vulkan_create_shader_module(handle, guest_address,
                                                   guest_address);
  result += __imported_vulkan_destroy_shader_module(handle,
                                                    shader_module_handle);
  result += __imported_vulkan_create_semaphore(handle, guest_address,
                                               guest_address);
  result += __imported_vulkan_destroy_semaphore(handle, semaphore_handle);
  result += __imported_vulkan_create_fence(handle, guest_address, guest_address);
  result += __imported_vulkan_destroy_fence(handle, fence_handle);
  result += __imported_vulkan_get_fence_status(handle, fence_handle);
  result += __imported_vulkan_wait_for_fences(handle, guest_address, 0u, 0u, 0u);
  result += __imported_vulkan_reset_fences(handle, guest_address, 0u);
  result += __imported_vulkan_create_image(handle, guest_address, guest_address);
  result += __imported_vulkan_destroy_image(handle, image_handle);
  result += __imported_vulkan_get_image_memory_requirements(
      handle, image_handle, guest_address);
  result += __imported_vulkan_bind_image_memory(handle, image_handle,
                                                memory_handle, 0u);
  result += __imported_wasi_snapshot_preview1_args_sizes_get(
      (intptr_t)guest_address, (intptr_t)guest_address);
  return result + (int32_t)features.sampler_anisotropy +
         (int32_t)image_create_info.extent.width +
         (int32_t)command_pool_create_info.queue_family_index +
         (int32_t)command_buffer_allocate_info.command_buffer_count +
         (int32_t)command_buffer_begin_info.flags +
         (int32_t)shader_module_create_info.flags +
         (int32_t)semaphore_create_info.flags +
         (int32_t)fence_create_info.flags;
}
