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
#include "plugin/module_exports.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "runtime/plugin_context.h"
#include "uwvm2_vulkan/impl.h"
#include "vulkan/api.h"

namespace uwvm2_vulkan::plugin {

namespace {

template <typename T>
[[nodiscard]] T LoadParameter(unsigned char const *bytes) noexcept {
  T value{};
  if (bytes != nullptr) {
    std::memcpy(&value, bytes, sizeof(T));
  }
  return value;
}

inline void StoreI32Result(unsigned char *result_bytes,
                           std::int32_t value) noexcept {
  if (result_bytes != nullptr) {
    std::memcpy(result_bytes, &value, sizeof(value));
  }
}

struct UWVM2_VULKAN_PACKED one_u64_param {
  std::uint64_t a0;
};

struct UWVM2_VULKAN_PACKED two_u64_param {
  std::uint64_t a0;
  std::uint64_t a1;
};

struct UWVM2_VULKAN_PACKED three_u64_param {
  std::uint64_t a0;
  std::uint64_t a1;
  std::uint64_t a2;
};

struct UWVM2_VULKAN_PACKED four_u64_param {
  std::uint64_t a0;
  std::uint64_t a1;
  std::uint64_t a2;
  std::uint64_t a3;
};

struct UWVM2_VULKAN_PACKED enumerate_extension_param {
  std::uint64_t layer_name_address;
  std::uint64_t property_buffer_address;
  std::uint32_t property_capacity;
  std::uint64_t out_property_count_address;
};

struct UWVM2_VULKAN_PACKED enumerate_device_extension_param {
  std::uint64_t physical_device_handle;
  std::uint64_t layer_name_address;
  std::uint64_t property_buffer_address;
  std::uint32_t property_capacity;
  std::uint64_t out_property_count_address;
};

struct UWVM2_VULKAN_PACKED enumerate_layer_param {
  std::uint64_t property_buffer_address;
  std::uint32_t property_capacity;
  std::uint64_t out_property_count_address;
};

struct UWVM2_VULKAN_PACKED enumerate_physical_device_param {
  std::uint64_t instance_handle;
  std::uint64_t out_device_buffer_address;
  std::uint32_t device_capacity;
  std::uint64_t out_device_count_address;
};

struct UWVM2_VULKAN_PACKED get_queue_param {
  std::uint64_t device_handle;
  std::uint32_t queue_family_index;
  std::uint32_t queue_index;
  std::uint64_t out_queue_address;
};

struct UWVM2_VULKAN_PACKED destroy_buffer_param {
  std::uint64_t device_handle;
  std::uint64_t buffer_handle;
};

struct UWVM2_VULKAN_PACKED bind_buffer_memory_param {
  std::uint64_t device_handle;
  std::uint64_t buffer_handle;
  std::uint64_t memory_handle;
  std::uint64_t offset;
};

void loader_available_entry(unsigned char *result_bytes,
                            unsigned char *parameter_bytes) noexcept {
  (void)parameter_bytes;
  StoreI32Result(result_bytes, api::LoaderAvailable());
}

void enumerate_instance_version_entry(unsigned char *result_bytes,
                                      unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<one_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::EnumerateInstanceVersion(parameter.a0));
}

void enumerate_instance_extension_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{
      LoadParameter<enumerate_extension_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::EnumerateInstanceExtensionProperties(
                                   parameter.layer_name_address,
                                   parameter.property_buffer_address,
                                   parameter.property_capacity,
                                   parameter.out_property_count_address));
}

void enumerate_instance_layer_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<enumerate_layer_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::EnumerateInstanceLayerProperties(
                                   parameter.property_buffer_address,
                                   parameter.property_capacity,
                                   parameter.out_property_count_address));
}

void create_instance_entry(unsigned char *result_bytes,
                           unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<two_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::CreateInstance(parameter.a0, parameter.a1));
}

void destroy_instance_entry(unsigned char *result_bytes,
                            unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<one_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::DestroyInstance(parameter.a0));
}

void enumerate_physical_devices_entry(unsigned char *result_bytes,
                                      unsigned char *parameter_bytes) noexcept {
  auto const parameter{
      LoadParameter<enumerate_physical_device_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::EnumeratePhysicalDevices(
                                   parameter.instance_handle,
                                   parameter.out_device_buffer_address,
                                   parameter.device_capacity,
                                   parameter.out_device_count_address));
}

void enumerate_device_extension_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{
      LoadParameter<enumerate_device_extension_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::EnumerateDeviceExtensionProperties(
                                   parameter.physical_device_handle,
                                   parameter.layer_name_address,
                                   parameter.property_buffer_address,
                                   parameter.property_capacity,
                                   parameter.out_property_count_address));
}

void get_physical_device_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<two_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes,
                 api::GetPhysicalDeviceProperties(parameter.a0, parameter.a1));
}

void get_physical_device_features_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<two_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes,
                 api::GetPhysicalDeviceFeatures(parameter.a0, parameter.a1));
}

void get_physical_device_memory_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<two_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::GetPhysicalDeviceMemoryProperties(
                                   parameter.a0, parameter.a1));
}

void get_physical_device_queue_family_properties_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{
      LoadParameter<enumerate_physical_device_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::GetPhysicalDeviceQueueFamilyProperties(
                                   parameter.instance_handle,
                                   parameter.out_device_buffer_address,
                                   parameter.device_capacity,
                                   parameter.out_device_count_address));
}

void create_device_entry(unsigned char *result_bytes,
                         unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes,
                 api::CreateDevice(parameter.a0, parameter.a1, parameter.a2));
}

void destroy_device_entry(unsigned char *result_bytes,
                          unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<one_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::DestroyDevice(parameter.a0));
}

void device_wait_idle_entry(unsigned char *result_bytes,
                            unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<one_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::DeviceWaitIdle(parameter.a0));
}

void get_device_queue_entry(unsigned char *result_bytes,
                            unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<get_queue_param>(parameter_bytes)};
  StoreI32Result(
      result_bytes,
      api::GetDeviceQueue(parameter.device_handle, parameter.queue_family_index,
                          parameter.queue_index, parameter.out_queue_address));
}

void queue_wait_idle_entry(unsigned char *result_bytes,
                           unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<one_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::QueueWaitIdle(parameter.a0));
}

void create_buffer_entry(unsigned char *result_bytes,
                         unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes,
                 api::CreateBuffer(parameter.a0, parameter.a1, parameter.a2));
}

void destroy_buffer_entry(unsigned char *result_bytes,
                          unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<destroy_buffer_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::DestroyBuffer(parameter.device_handle,
                                                  parameter.buffer_handle));
}

void get_buffer_memory_requirements_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::GetBufferMemoryRequirements(
                                   parameter.a0, parameter.a1, parameter.a2));
}

void allocate_memory_entry(unsigned char *result_bytes,
                           unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes,
                 api::AllocateMemory(parameter.a0, parameter.a1, parameter.a2));
}

void free_memory_entry(unsigned char *result_bytes,
                       unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<two_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::FreeMemory(parameter.a0, parameter.a1));
}

void bind_buffer_memory_entry(unsigned char *result_bytes,
                              unsigned char *parameter_bytes) noexcept {
  auto const parameter{
      LoadParameter<bind_buffer_memory_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::BindBufferMemory(parameter.device_handle,
                                                     parameter.buffer_handle,
                                                     parameter.memory_handle,
                                                     parameter.offset));
}

void copy_guest_to_device_memory_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::CopyGuestToDeviceMemory(
                                   parameter.a0, parameter.a1, parameter.a2));
}

void copy_device_memory_to_guest_entry(
    unsigned char *result_bytes, unsigned char *parameter_bytes) noexcept {
  auto const parameter{LoadParameter<three_u64_param>(parameter_bytes)};
  StoreI32Result(result_bytes, api::CopyDeviceMemoryToGuest(
                                   parameter.a0, parameter.a1, parameter.a2));
}

inline constexpr std::uint_least8_t kI32Types[]{UWVM_WASM_VALTYPE_I32};
inline constexpr std::uint_least8_t kI64Types[]{UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I64Types[]{UWVM_WASM_VALTYPE_I64,
                                                   UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I64I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I64I64I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64,
    UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I64I32I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I32,
    UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I64I64I32I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I64,
    UWVM_WASM_VALTYPE_I32, UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I32I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I32, UWVM_WASM_VALTYPE_I64};
inline constexpr std::uint_least8_t kI64I32I32I64Types[]{
    UWVM_WASM_VALTYPE_I64, UWVM_WASM_VALTYPE_I32, UWVM_WASM_VALTYPE_I32,
    UWVM_WASM_VALTYPE_I64};

inline constexpr char kModuleName[]{"wasiu-vulkan"};

inline constexpr char kLoaderAvailableName[]{"loader_available"};
inline constexpr char kEnumerateInstanceVersionName[]{
    "enumerate_instance_version"};
inline constexpr char kEnumerateInstanceExtensionPropertiesName[]{
    "enumerate_instance_extension_properties"};
inline constexpr char kEnumerateInstanceLayerPropertiesName[]{
    "enumerate_instance_layer_properties"};
inline constexpr char kCreateInstanceName[]{"create_instance"};
inline constexpr char kDestroyInstanceName[]{"destroy_instance"};
inline constexpr char kEnumeratePhysicalDevicesName[]{
    "enumerate_physical_devices"};
inline constexpr char kEnumerateDeviceExtensionPropertiesName[]{
    "enumerate_device_extension_properties"};
inline constexpr char kGetPhysicalDevicePropertiesName[]{
    "get_physical_device_properties"};
inline constexpr char kGetPhysicalDeviceFeaturesName[]{
    "get_physical_device_features"};
inline constexpr char kGetPhysicalDeviceMemoryPropertiesName[]{
    "get_physical_device_memory_properties"};
inline constexpr char kGetPhysicalDeviceQueueFamilyPropertiesName[]{
    "get_physical_device_queue_family_properties"};
inline constexpr char kCreateDeviceName[]{"create_device"};
inline constexpr char kDestroyDeviceName[]{"destroy_device"};
inline constexpr char kDeviceWaitIdleName[]{"device_wait_idle"};
inline constexpr char kGetDeviceQueueName[]{"get_device_queue"};
inline constexpr char kQueueWaitIdleName[]{"queue_wait_idle"};
inline constexpr char kCreateBufferName[]{"create_buffer"};
inline constexpr char kDestroyBufferName[]{"destroy_buffer"};
inline constexpr char kGetBufferMemoryRequirementsName[]{
    "get_buffer_memory_requirements"};
inline constexpr char kAllocateMemoryName[]{"allocate_memory"};
inline constexpr char kFreeMemoryName[]{"free_memory"};
inline constexpr char kBindBufferMemoryName[]{"bind_buffer_memory"};
inline constexpr char kCopyGuestToDeviceMemoryName[]{
    "copy_guest_to_device_memory"};
inline constexpr char kCopyDeviceMemoryToGuestName[]{
    "copy_device_memory_to_guest"};

inline constexpr uwvm_capi_function_t kFunctions[] = {
    {kLoaderAvailableName, sizeof(kLoaderAvailableName) - 1u, nullptr, 0u,
     kI32Types, 1u, loader_available_entry},
    {kEnumerateInstanceVersionName, sizeof(kEnumerateInstanceVersionName) - 1u,
     kI64Types, 1u, kI32Types, 1u, enumerate_instance_version_entry},
    {kEnumerateInstanceExtensionPropertiesName,
     sizeof(kEnumerateInstanceExtensionPropertiesName) - 1u, kI64I64I32I64Types,
     4u, kI32Types, 1u, enumerate_instance_extension_properties_entry},
    {kEnumerateInstanceLayerPropertiesName,
     sizeof(kEnumerateInstanceLayerPropertiesName) - 1u, kI64I32I64Types, 3u,
     kI32Types, 1u, enumerate_instance_layer_properties_entry},
    {kCreateInstanceName, sizeof(kCreateInstanceName) - 1u, kI64I64Types, 2u,
     kI32Types, 1u, create_instance_entry},
    {kDestroyInstanceName, sizeof(kDestroyInstanceName) - 1u, kI64Types, 1u,
     kI32Types, 1u, destroy_instance_entry},
    {kEnumeratePhysicalDevicesName, sizeof(kEnumeratePhysicalDevicesName) - 1u,
     kI64I64I32I64Types, 4u, kI32Types, 1u, enumerate_physical_devices_entry},
    {kEnumerateDeviceExtensionPropertiesName,
     sizeof(kEnumerateDeviceExtensionPropertiesName) - 1u,
     kI64I64I64I32I64Types, 5u, kI32Types, 1u,
     enumerate_device_extension_properties_entry},
    {kGetPhysicalDevicePropertiesName,
     sizeof(kGetPhysicalDevicePropertiesName) - 1u, kI64I64Types, 2u, kI32Types,
     1u, get_physical_device_properties_entry},
    {kGetPhysicalDeviceFeaturesName,
     sizeof(kGetPhysicalDeviceFeaturesName) - 1u, kI64I64Types, 2u, kI32Types,
     1u, get_physical_device_features_entry},
    {kGetPhysicalDeviceMemoryPropertiesName,
     sizeof(kGetPhysicalDeviceMemoryPropertiesName) - 1u, kI64I64Types, 2u,
     kI32Types, 1u, get_physical_device_memory_properties_entry},
    {kGetPhysicalDeviceQueueFamilyPropertiesName,
     sizeof(kGetPhysicalDeviceQueueFamilyPropertiesName) - 1u,
     kI64I64I32I64Types, 4u, kI32Types, 1u,
     get_physical_device_queue_family_properties_entry},
    {kCreateDeviceName, sizeof(kCreateDeviceName) - 1u, kI64I64I64Types, 3u,
     kI32Types, 1u, create_device_entry},
    {kDestroyDeviceName, sizeof(kDestroyDeviceName) - 1u, kI64Types, 1u,
     kI32Types, 1u, destroy_device_entry},
    {kDeviceWaitIdleName, sizeof(kDeviceWaitIdleName) - 1u, kI64Types, 1u,
     kI32Types, 1u, device_wait_idle_entry},
    {kGetDeviceQueueName, sizeof(kGetDeviceQueueName) - 1u, kI64I32I32I64Types,
     4u, kI32Types, 1u, get_device_queue_entry},
    {kQueueWaitIdleName, sizeof(kQueueWaitIdleName) - 1u, kI64Types, 1u,
     kI32Types, 1u, queue_wait_idle_entry},
    {kCreateBufferName, sizeof(kCreateBufferName) - 1u, kI64I64I64Types, 3u,
     kI32Types, 1u, create_buffer_entry},
    {kDestroyBufferName, sizeof(kDestroyBufferName) - 1u, kI64I64Types, 2u,
     kI32Types, 1u, destroy_buffer_entry},
    {kGetBufferMemoryRequirementsName,
     sizeof(kGetBufferMemoryRequirementsName) - 1u, kI64I64I64Types, 3u,
     kI32Types, 1u, get_buffer_memory_requirements_entry},
    {kAllocateMemoryName, sizeof(kAllocateMemoryName) - 1u, kI64I64I64Types, 3u,
     kI32Types, 1u, allocate_memory_entry},
    {kFreeMemoryName, sizeof(kFreeMemoryName) - 1u, kI64I64Types, 2u, kI32Types,
     1u, free_memory_entry},
    {kBindBufferMemoryName, sizeof(kBindBufferMemoryName) - 1u,
     kI64I64I64I64Types, 4u, kI32Types, 1u, bind_buffer_memory_entry},
    {kCopyGuestToDeviceMemoryName, sizeof(kCopyGuestToDeviceMemoryName) - 1u,
     kI64I64I64Types, 3u, kI32Types, 1u, copy_guest_to_device_memory_entry},
    {kCopyDeviceMemoryToGuestName, sizeof(kCopyDeviceMemoryToGuestName) - 1u,
     kI64I64I64Types, 3u, kI32Types, 1u, copy_device_memory_to_guest_entry},
};

inline constexpr uwvm_capi_function_vec_t kFunctionVec{
    kFunctions, sizeof(kFunctions) / sizeof(kFunctions[0])};
inline constexpr uwvm_capi_custom_handler_vec_t kCustomHandlers{nullptr, 0u};
inline constexpr uwvm_weak_symbol_module_c kWeakModule{
    kModuleName, sizeof(kModuleName) - 1u, kCustomHandlers, kFunctionVec};
inline constexpr uwvm_weak_symbol_module_vector_c kWeakVector{&kWeakModule, 1u};

} // namespace

uwvm_capi_module_name_t GetModuleName() noexcept {
  return uwvm_capi_module_name_t{kModuleName, sizeof(kModuleName) - 1u};
}

uwvm_capi_custom_handler_vec_t GetCustomHandlerVec() noexcept {
  return kCustomHandlers;
}

uwvm_capi_function_vec_t GetFunctionVec() noexcept { return kFunctionVec; }

uwvm_weak_symbol_module_vector_c const *GetWeakModuleVector() noexcept {
  return &kWeakVector;
}

void SetPreloadHostApi(uwvm_preload_host_api_v1 const *host_api) noexcept {
  runtime::PluginContext::Instance().SetExplicitHostApi(host_api);
}

} // namespace uwvm2_vulkan::plugin
