#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <uwvm2_vulkan/vulkan.h>

namespace {

constexpr char kPortabilityEnumerationExtensionName[]{
    "VK_KHR_portability_enumeration"};

constexpr uint64_t GuestAddress(void const *pointer) noexcept {
  return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pointer));
}

constexpr uint32_t VulkanMakeVersion(uint32_t variant, uint32_t major,
                                     uint32_t minor,
                                     uint32_t patch) noexcept {
  return (variant << 29u) | (major << 22u) | (minor << 12u) | patch;
}

constexpr uint32_t VulkanVersionVariant(uint32_t version) noexcept {
  return version >> 29u;
}

constexpr uint32_t VulkanVersionMajor(uint32_t version) noexcept {
  return (version >> 22u) & 0x7Fu;
}

constexpr uint32_t VulkanVersionMinor(uint32_t version) noexcept {
  return (version >> 12u) & 0x3FFu;
}

constexpr uint32_t VulkanVersionPatch(uint32_t version) noexcept {
  return version & 0xFFFu;
}

void PrintResult(char const *step, int32_t result) {
  (void)printf("uwvm2-vulkan-smoke: %s -> %d\n", step, result);
}

bool HasExtension(uwvm_vk_extension_property const *extensions,
                  uint32_t extension_count,
                  char const *extension_name) noexcept {
  for (uint32_t i{}; i != extension_count; ++i) {
    if (strcmp(extensions[i].extension_name, extension_name) == 0) {
      return true;
    }
  }
  return false;
}

} // namespace

int main() {
  int32_t const loader_available{__imported_vulkan_loader_available()};
  (void)printf("uwvm2-vulkan-smoke: loader_available=%d\n", loader_available);
  if (loader_available == 0) {
    (void)puts(
        "uwvm2-vulkan-smoke: no host Vulkan loader was found "
        "(libvulkan/libMoltenVK).");
    return 2;
  }

  uint32_t api_version{};
  int32_t result{
      __imported_vulkan_enumerate_instance_version(GuestAddress(&api_version))};
  PrintResult("enumerate_instance_version", result);
  if (result != UWVM_VK_SUCCESS) {
    return 3;
  }

  (void)printf(
      "uwvm2-vulkan-smoke: instance api version=%u.%u.%u variant=%u\n",
      VulkanVersionMajor(api_version), VulkanVersionMinor(api_version),
      VulkanVersionPatch(api_version), VulkanVersionVariant(api_version));

  uint32_t instance_extension_count{};
  result = __imported_vulkan_enumerate_instance_extension_properties(
      0u, 0u, 0u, GuestAddress(&instance_extension_count));
  PrintResult("enumerate_instance_extension_properties.count", result);
  if (result != UWVM_VK_SUCCESS && result != UWVM_VK_INCOMPLETE) {
    return 6;
  }

  uwvm_vk_extension_property instance_extensions[64]{};
  uint32_t const queried_extension_capacity{
      instance_extension_count <
              (sizeof(instance_extensions) / sizeof(instance_extensions[0]))
          ? instance_extension_count
          : static_cast<uint32_t>(sizeof(instance_extensions) /
                                  sizeof(instance_extensions[0]))};
  if (queried_extension_capacity != 0u) {
    result = __imported_vulkan_enumerate_instance_extension_properties(
        0u, GuestAddress(instance_extensions), queried_extension_capacity,
        GuestAddress(&instance_extension_count));
    PrintResult("enumerate_instance_extension_properties.list", result);
    if (result != UWVM_VK_SUCCESS && result != UWVM_VK_INCOMPLETE) {
      return 7;
    }
  }

  bool const needs_portability_enumeration{
      HasExtension(instance_extensions, queried_extension_capacity,
                   kPortabilityEnumerationExtensionName)};
  (void)printf("uwvm2-vulkan-smoke: portability_enumeration=%s\n",
               needs_portability_enumeration ? "enabled" : "not-needed");

  uint32_t const requested_api_version{VulkanMakeVersion(0u, 1u, 0u, 0u)};
  char const application_name[]{"uwvm2-vulkan-smoke"};
  char const engine_name[]{"uwvm2-vulkan-wrapper"};
  uwvm_vk_string_view enabled_extensions[1]{};
  uint64_t enabled_extensions_address{};
  uint32_t enabled_extension_count{};
  uint32_t instance_flags{};
  if (needs_portability_enumeration) {
    enabled_extensions[0] = uwvm_vk_string_view{
        .data_address = GuestAddress(kPortabilityEnumerationExtensionName),
        .size =
            static_cast<uint32_t>(sizeof(kPortabilityEnumerationExtensionName) -
                                  1u),
        .reserved = 0u};
    enabled_extensions_address = GuestAddress(enabled_extensions);
    enabled_extension_count = 1u;
    instance_flags = UWVM_VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }

  uwvm_vk_application_info application_info{
      .application_name =
          {.data_address = GuestAddress(application_name),
           .size = static_cast<uint32_t>(sizeof(application_name) - 1u),
           .reserved = 0u},
      .application_version = 1u,
      .reserved0 = 0u,
      .engine_name = {.data_address = GuestAddress(engine_name),
                      .size = static_cast<uint32_t>(sizeof(engine_name) - 1u),
                      .reserved = 0u},
      .engine_version = 1u,
      .api_version = requested_api_version};

  uwvm_vk_instance_create_info instance_create_info{
      .flags = instance_flags,
      .enabled_layer_count = 0u,
      .enabled_extension_count = enabled_extension_count,
      .reserved0 = 0u,
      .application_info_address = GuestAddress(&application_info),
      .enabled_layers_address = 0u,
      .enabled_extensions_address = enabled_extensions_address,
      .reserved1 = 0u};

  uint64_t instance_handle{};
  result = __imported_vulkan_create_instance(GuestAddress(&instance_create_info),
                                             GuestAddress(&instance_handle));
  PrintResult("create_instance", result);
  (void)printf("uwvm2-vulkan-smoke: instance_handle=0x%llx\n",
               static_cast<unsigned long long>(instance_handle));
  if (result != UWVM_VK_SUCCESS || instance_handle == 0u) {
    return 4;
  }

  result = __imported_vulkan_destroy_instance(instance_handle);
  PrintResult("destroy_instance", result);
  return result == UWVM_VK_SUCCESS ? 0 : 5;
}
