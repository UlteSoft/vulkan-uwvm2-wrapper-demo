#include <stdint.h>

#include <uwvm2_vulkan/vulkan.h>
#include <uwvm2_vulkan/wasi_snapshot_preview1.h>

int guest_header_smoke_use(void) {
  uint64_t handle = 0;
  uint64_t guest_address = 0;
  uwvm_vk_physical_device_features features = {0};
  int32_t result =
      __imported_vulkan_create_instance(guest_address, guest_address);
  result += __imported_vulkan_destroy_instance(handle);
  result += __imported_vulkan_enumerate_device_extension_properties(
      handle, guest_address, guest_address, 0u, guest_address);
  result += __imported_vulkan_get_physical_device_features(handle, guest_address);
  result += __imported_vulkan_device_wait_idle(handle);
  result += __imported_vulkan_queue_wait_idle(handle);
  result += __imported_wasi_snapshot_preview1_args_sizes_get(
      (intptr_t)guest_address, (intptr_t)guest_address);
  return result + (int32_t)features.sampler_anisotropy;
}
