#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <uwvm2_vulkan/vulkan_types.h>

#include "runtime/plugin_context.h"
#include "vulkan/api.h"

namespace {

using uwvm2_vulkan::runtime::PluginContext;
namespace native = uwvm2_vulkan::vk::native;

struct FakeMemoryHost {
  std::vector<std::byte> memory{};
  std::atomic_size_t dynamic_length{};
  uwvm_preload_memory_descriptor_t descriptor{};
};

struct DestroyBufferCall {
  native::VkDevice device{};
  native::VkBuffer buffer{};
};

struct DestroyImageCall {
  native::VkDevice device{};
  native::VkImage image{};
};

struct DestroyShaderModuleCall {
  native::VkDevice device{};
  native::VkShaderModule shader_module{};
};

struct DestroySemaphoreCall {
  native::VkDevice device{};
  native::VkSemaphore semaphore{};
};

struct DestroyFenceCall {
  native::VkDevice device{};
  native::VkFence fence{};
};

struct DestroyCommandPoolCall {
  native::VkDevice device{};
  native::VkCommandPool command_pool{};
};

struct FreeCommandBuffersCall {
  native::VkDevice device{};
  native::VkCommandPool command_pool{};
  std::vector<native::VkCommandBuffer> command_buffers{};
};

struct FreeMemoryCall {
  native::VkDevice device{};
  native::VkDeviceMemory memory{};
};

FakeMemoryHost *g_host{};

std::size_t descriptor_count() noexcept { return 1u; }

bool descriptor_at(std::size_t index,
                   uwvm_preload_memory_descriptor_t *out) noexcept {
  if (g_host == nullptr || index != 0u || out == nullptr) {
    return false;
  }

  *out = g_host->descriptor;
  return true;
}

bool memory_read(std::size_t memory_index, std::uint_least64_t address,
                 void *destination, std::size_t size) noexcept {
  if (g_host == nullptr || destination == nullptr ||
      memory_index != g_host->descriptor.memory_index) {
    return false;
  }

  auto const begin{static_cast<std::size_t>(address)};
  if (begin > g_host->memory.size() || size > g_host->memory.size() - begin) {
    return false;
  }

  std::memcpy(destination, g_host->memory.data() + begin, size);
  return true;
}

bool memory_write(std::size_t memory_index, std::uint_least64_t address,
                  void const *source, std::size_t size) noexcept {
  if (g_host == nullptr || source == nullptr ||
      memory_index != g_host->descriptor.memory_index) {
    return false;
  }

  auto const begin{static_cast<std::size_t>(address)};
  if (begin > g_host->memory.size() || size > g_host->memory.size() - begin) {
    return false;
  }

  std::memcpy(g_host->memory.data() + begin, source, size);
  return true;
}

void require_impl(bool condition, int line) {
  if (!condition) {
    std::fprintf(stderr, "require failed at line %d\n", line);
    std::abort();
  }
}

#define require(condition) require_impl((condition), __LINE__)

template <typename T> void WriteGuest(std::uint64_t address, T const &value) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(sizeof(T) <= g_host->memory.size() - begin);
  std::memcpy(g_host->memory.data() + begin, &value, sizeof(T));
}

template <typename T> [[nodiscard]] T ReadGuest(std::uint64_t address) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(sizeof(T) <= g_host->memory.size() - begin);

  T value{};
  std::memcpy(&value, g_host->memory.data() + begin, sizeof(T));
  return value;
}

void WriteGuestBytes(std::uint64_t address, void const *source,
                     std::size_t size) {
  require(g_host != nullptr);
  auto const begin{static_cast<std::size_t>(address)};
  require(begin <= g_host->memory.size());
  require(size <= g_host->memory.size() - begin);
  std::memcpy(g_host->memory.data() + begin, source, size);
}

void WriteGuestString(std::uint64_t address, char const *text) {
  WriteGuestBytes(address, text, std::strlen(text));
}

[[nodiscard]] uwvm_vk_extension_property MakeExtensionProperty(
    char const *name, std::uint32_t spec_version) {
  uwvm_vk_extension_property property{};
  auto const name_length{std::strlen(name)};
  auto const copy_length{
      name_length < sizeof(property.extension_name) - 1u
          ? name_length
          : sizeof(property.extension_name) - 1u};
  std::memcpy(property.extension_name, name, copy_length);
  property.extension_name[copy_length] = '\0';
  property.spec_version = spec_version;
  return property;
}

class FakeBackend final : public uwvm2_vulkan::vk::Backend {
public:
  bool LoaderAvailable() override { return true; }

  std::int32_t EnumerateInstanceVersion(std::uint32_t &api_version) override {
    api_version = 0u;
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateInstanceExtensionProperties(
      char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override {
    (void)layer_name;
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateInstanceLayerProperties(
      std::vector<uwvm_vk_layer_property> &properties) override {
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t CreateInstance(native::VkInstanceCreateInfo const &create_info,
                              native::VkInstance &instance) override {
    (void)create_info;
    instance = reinterpret_cast<native::VkInstance>(0x1000u);
    return UWVM_VK_SUCCESS;
  }

  void DestroyInstance(native::VkInstance instance) override {
    (void)instance;
  }

  std::int32_t EnumeratePhysicalDevices(
      native::VkInstance instance,
      std::vector<native::VkPhysicalDevice> &physical_devices) override {
    (void)instance;
    physical_devices.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t EnumerateDeviceExtensionProperties(
      native::VkPhysicalDevice physical_device, char const *layer_name,
      std::vector<uwvm_vk_extension_property> &properties) override {
    last_extension_query_physical_device = physical_device;
    last_extension_query_layer_name = layer_name == nullptr ? "" : layer_name;
    properties = device_extension_properties;
    return device_extension_query_result;
  }

  std::int32_t GetPhysicalDeviceProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_properties &properties) override {
    (void)physical_device;
    properties = {};
    return UWVM_VK_SUCCESS;
  }

  std::int32_t GetPhysicalDeviceFeatures(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_features &features) override {
    last_feature_query_physical_device = physical_device;
    features = physical_device_features;
    return physical_device_features_result;
  }

  std::int32_t GetPhysicalDeviceMemoryProperties(
      native::VkPhysicalDevice physical_device,
      uwvm_vk_physical_device_memory_properties &properties) override {
    (void)physical_device;
    properties = {};
    return UWVM_VK_SUCCESS;
  }

  std::int32_t GetPhysicalDeviceQueueFamilyProperties(
      native::VkPhysicalDevice physical_device,
      std::vector<uwvm_vk_queue_family_properties> &properties) override {
    (void)physical_device;
    properties.clear();
    return UWVM_VK_SUCCESS;
  }

  std::int32_t CreateDevice(native::VkPhysicalDevice physical_device,
                            native::VkDeviceCreateInfo const &create_info,
                            native::VkDevice &device) override {
    last_create_device_physical_device = physical_device;
    last_queue_family_index =
        create_info.queueCreateInfoCount == 0u
            ? 0u
            : create_info.pQueueCreateInfos[0].queueFamilyIndex;
    last_queue_priority =
        create_info.queueCreateInfoCount == 0u ||
                create_info.pQueueCreateInfos[0].queueCount == 0u ||
                create_info.pQueueCreateInfos[0].pQueuePriorities == nullptr
            ? 0.0f
            : create_info.pQueueCreateInfos[0].pQueuePriorities[0];
    create_device_enabled_features_present =
        create_info.pEnabledFeatures != nullptr;
    if (create_info.pEnabledFeatures != nullptr) {
      std::memcpy(&captured_enabled_features, create_info.pEnabledFeatures,
                  sizeof(captured_enabled_features));
    } else {
      captured_enabled_features = {};
    }

    device = create_device_result_handle;
    return create_device_result;
  }

  void DestroyDevice(native::VkDevice device) override {
    destroyed_devices.push_back(device);
  }

  std::int32_t DeviceWaitIdle(native::VkDevice device) override {
    last_device_wait_idle_device = device;
    ++device_wait_idle_calls;
    return device_wait_idle_result;
  }

  std::int32_t GetDeviceQueue(native::VkDevice device,
                              std::uint32_t queue_family_index,
                              std::uint32_t queue_index,
                              native::VkQueue &queue) override {
    last_get_device_queue_device = device;
    last_get_device_queue_family_index = queue_family_index;
    last_get_device_queue_index = queue_index;
    queue = get_device_queue_result_handle;
    return get_device_queue_result;
  }

  std::int32_t QueueWaitIdle(native::VkDevice device,
                             native::VkQueue queue) override {
    last_queue_wait_idle_device = device;
    last_queue_wait_idle_queue = queue;
    ++queue_wait_idle_calls;
    return queue_wait_idle_result;
  }

  std::int32_t CreateCommandPool(
      native::VkDevice device,
      native::VkCommandPoolCreateInfo const &create_info,
      native::VkCommandPool &command_pool) override {
    last_create_command_pool_device = device;
    last_create_command_pool_flags = create_info.flags;
    last_create_command_pool_queue_family_index =
        create_info.queueFamilyIndex;

    auto const index{create_command_pool_call_count};
    ++create_command_pool_call_count;
    command_pool = index < create_command_pool_result_handles.size()
                       ? create_command_pool_result_handles[index]
                       : create_command_pool_result_handles.back();
    return create_command_pool_result;
  }

  void DestroyCommandPool(native::VkDevice device,
                          native::VkCommandPool command_pool) override {
    destroyed_command_pools.push_back(
        DestroyCommandPoolCall{device, command_pool});
  }

  std::int32_t ResetCommandPool(native::VkDevice device,
                                native::VkCommandPool command_pool,
                                std::uint32_t flags) override {
    last_reset_command_pool_device = device;
    last_reset_command_pool_pool = command_pool;
    last_reset_command_pool_flags = flags;
    return reset_command_pool_result;
  }

  std::int32_t AllocateCommandBuffers(
      native::VkDevice device,
      native::VkCommandBufferAllocateInfo const &allocate_info,
      std::vector<native::VkCommandBuffer> &command_buffers) override {
    last_allocate_command_buffers_device = device;
    last_allocate_command_buffers_pool = allocate_info.commandPool;
    last_allocate_command_buffers_level =
        static_cast<std::uint32_t>(allocate_info.level);
    last_allocate_command_buffers_count = allocate_info.commandBufferCount;

    auto const index{allocate_command_buffers_call_count};
    ++allocate_command_buffers_call_count;
    command_buffers =
        index < allocate_command_buffers_result_batches.size()
            ? allocate_command_buffers_result_batches[index]
            : allocate_command_buffers_result_batches.back();
    return allocate_command_buffers_result;
  }

  void FreeCommandBuffers(
      native::VkDevice device, native::VkCommandPool command_pool,
      std::vector<native::VkCommandBuffer> const &command_buffers) override {
    last_free_command_buffers_device = device;
    last_free_command_buffers_pool = command_pool;
    last_free_command_buffers_command_buffers = command_buffers;
    free_command_buffers_calls.push_back(
        FreeCommandBuffersCall{device, command_pool, command_buffers});
  }

  std::int32_t BeginCommandBuffer(
      native::VkDevice device, native::VkCommandBuffer command_buffer,
      native::VkCommandBufferBeginInfo const &begin_info) override {
    last_begin_command_buffer_device = device;
    last_begin_command_buffer = command_buffer;
    last_begin_command_buffer_flags = begin_info.flags;
    return begin_command_buffer_result;
  }

  std::int32_t EndCommandBuffer(native::VkDevice device,
                                native::VkCommandBuffer command_buffer) override {
    last_end_command_buffer_device = device;
    last_end_command_buffer = command_buffer;
    return end_command_buffer_result;
  }

  std::int32_t ResetCommandBuffer(
      native::VkDevice device, native::VkCommandBuffer command_buffer,
      std::uint32_t flags) override {
    last_reset_command_buffer_device = device;
    last_reset_command_buffer = command_buffer;
    last_reset_command_buffer_flags = flags;
    return reset_command_buffer_result;
  }

  std::int32_t CreateShaderModule(
      native::VkDevice device,
      native::VkShaderModuleCreateInfo const &create_info,
      native::VkShaderModule &shader_module) override {
    last_create_shader_module_device = device;
    last_create_shader_module_flags = create_info.flags;
    last_create_shader_module_code_size = create_info.codeSize;
    last_create_shader_module_words.assign(
        create_info.pCode, create_info.pCode +
                              (create_info.codeSize / sizeof(std::uint32_t)));
    shader_module = create_shader_module_result_handle;
    return create_shader_module_result;
  }

  void DestroyShaderModule(native::VkDevice device,
                           native::VkShaderModule shader_module) override {
    destroyed_shader_modules.push_back(
        DestroyShaderModuleCall{device, shader_module});
  }

  std::int32_t CreateSemaphore(
      native::VkDevice device, native::VkSemaphoreCreateInfo const &create_info,
      native::VkSemaphore &semaphore) override {
    last_create_semaphore_device = device;
    last_create_semaphore_flags = create_info.flags;
    semaphore = create_semaphore_result_handle;
    return create_semaphore_result;
  }

  void DestroySemaphore(native::VkDevice device,
                        native::VkSemaphore semaphore) override {
    destroyed_semaphores.push_back(DestroySemaphoreCall{device, semaphore});
  }

  std::int32_t CreateFence(native::VkDevice device,
                           native::VkFenceCreateInfo const &create_info,
                           native::VkFence &fence) override {
    last_create_fence_device = device;
    last_create_fence_flags = create_info.flags;
    fence = create_fence_result_handle;
    return create_fence_result;
  }

  void DestroyFence(native::VkDevice device, native::VkFence fence) override {
    destroyed_fences.push_back(DestroyFenceCall{device, fence});
  }

  std::int32_t GetFenceStatus(native::VkDevice device,
                              native::VkFence fence) override {
    last_get_fence_status_device = device;
    last_get_fence_status_fence = fence;
    return get_fence_status_result;
  }

  std::int32_t WaitForFences(native::VkDevice device,
                             std::vector<native::VkFence> const &fences,
                             bool wait_all,
                             std::uint64_t timeout_nanoseconds) override {
    last_wait_for_fences_device = device;
    last_wait_for_fences_fences = fences;
    last_wait_for_fences_wait_all = wait_all;
    last_wait_for_fences_timeout_nanoseconds = timeout_nanoseconds;
    return wait_for_fences_result;
  }

  std::int32_t ResetFences(
      native::VkDevice device,
      std::vector<native::VkFence> const &fences) override {
    last_reset_fences_device = device;
    last_reset_fences_fences = fences;
    return reset_fences_result;
  }

  std::int32_t CreateBuffer(native::VkDevice device,
                            native::VkBufferCreateInfo const &create_info,
                            native::VkBuffer &buffer) override {
    (void)device;
    (void)create_info;
    buffer = 0u;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  void DestroyBuffer(native::VkDevice device, native::VkBuffer buffer) override {
    destroyed_buffers.push_back(DestroyBufferCall{device, buffer});
  }

  std::int32_t GetBufferMemoryRequirements(
      native::VkDevice device, native::VkBuffer buffer,
      uwvm_vk_memory_requirements &requirements) override {
    (void)device;
    (void)buffer;
    requirements = {};
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::int32_t CreateImage(native::VkDevice device,
                           native::VkImageCreateInfo const &create_info,
                           native::VkImage &image) override {
    last_create_image_device = device;
    last_create_image_type = static_cast<std::uint32_t>(create_info.imageType);
    last_create_image_format = create_info.format;
    last_create_image_extent = create_info.extent;
    last_create_image_mip_levels = create_info.mipLevels;
    last_create_image_array_layers = create_info.arrayLayers;
    last_create_image_samples = create_info.samples;
    last_create_image_tiling =
        static_cast<std::uint32_t>(create_info.tiling);
    last_create_image_usage = create_info.usage;
    last_create_image_sharing_mode =
        static_cast<std::uint32_t>(create_info.sharingMode);
    last_create_image_initial_layout =
        static_cast<std::uint32_t>(create_info.initialLayout);
    last_create_image_queue_families.clear();
    for (std::uint32_t i{}; i != create_info.queueFamilyIndexCount; ++i) {
      last_create_image_queue_families.push_back(
          create_info.pQueueFamilyIndices[i]);
    }

    image = create_image_result_handle;
    return create_image_result;
  }

  void DestroyImage(native::VkDevice device, native::VkImage image) override {
    destroyed_images.push_back(DestroyImageCall{device, image});
  }

  std::int32_t GetImageMemoryRequirements(
      native::VkDevice device, native::VkImage image,
      uwvm_vk_memory_requirements &requirements) override {
    (void)device;
    last_get_image_memory_requirements_image = image;
    requirements = image_memory_requirements;
    return get_image_memory_requirements_result;
  }

  std::int32_t AllocateMemory(
      native::VkDevice device, native::VkMemoryAllocateInfo const &allocate_info,
      native::VkDeviceMemory &memory) override {
    (void)device;
    (void)allocate_info;
    memory = 0u;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  void FreeMemory(native::VkDevice device, native::VkDeviceMemory memory) override {
    freed_memories.push_back(FreeMemoryCall{device, memory});
  }

  std::int32_t BindBufferMemory(native::VkDevice device, native::VkBuffer buffer,
                                native::VkDeviceMemory memory,
                                std::uint64_t offset) override {
    (void)device;
    (void)buffer;
    (void)memory;
    (void)offset;
    return UWVM_VK_ERROR_UNSUPPORTED_OPERATION;
  }

  std::int32_t BindImageMemory(native::VkDevice device, native::VkImage image,
                               native::VkDeviceMemory memory,
                               std::uint64_t offset) override {
    last_bind_image_memory_device = device;
    last_bind_image_memory_image = image;
    last_bind_image_memory_memory = memory;
    last_bind_image_memory_offset = offset;
    return bind_image_memory_result;
  }

  std::int32_t MapMemory(native::VkDevice device, native::VkDeviceMemory memory,
                         std::uint64_t offset, std::uint64_t size,
                         void *&mapped_data) override {
    (void)device;
    (void)memory;
    ++map_memory_calls;

    auto const begin{static_cast<std::size_t>(offset)};
    auto const byte_count{static_cast<std::size_t>(size)};
    if (begin > mapped_memory.size() ||
        byte_count > mapped_memory.size() - begin) {
      mapped_data = nullptr;
      return UWVM_VK_ERROR_INVALID_ARGUMENT;
    }

    mapped_data = mapped_memory.data() + begin;
    return map_memory_result;
  }

  void UnmapMemory(native::VkDevice device, native::VkDeviceMemory memory) override {
    (void)device;
    (void)memory;
    ++unmap_memory_calls;
  }

  std::int32_t FlushMappedMemory(native::VkDevice device,
                                 native::VkDeviceMemory memory,
                                 std::uint64_t offset,
                                 std::uint64_t size) override {
    (void)device;
    (void)memory;
    (void)offset;
    (void)size;
    ++flush_mapped_memory_calls;
    return flush_mapped_memory_result;
  }

  std::int32_t InvalidateMappedMemory(native::VkDevice device,
                                      native::VkDeviceMemory memory,
                                      std::uint64_t offset,
                                      std::uint64_t size) override {
    (void)device;
    (void)memory;
    (void)offset;
    (void)size;
    ++invalidate_mapped_memory_calls;
    return invalidate_mapped_memory_result;
  }

  std::vector<uwvm_vk_extension_property> device_extension_properties{};
  std::int32_t device_extension_query_result{UWVM_VK_SUCCESS};
  uwvm_vk_physical_device_features physical_device_features{};
  std::int32_t physical_device_features_result{UWVM_VK_SUCCESS};
  native::VkDevice create_device_result_handle{
      reinterpret_cast<native::VkDevice>(0x3000u)};
  std::int32_t create_device_result{UWVM_VK_SUCCESS};
  native::VkQueue get_device_queue_result_handle{
      reinterpret_cast<native::VkQueue>(0x4000u)};
  std::int32_t get_device_queue_result{UWVM_VK_SUCCESS};
  std::vector<native::VkCommandPool> create_command_pool_result_handles{
      static_cast<native::VkCommandPool>(0x4600u),
      static_cast<native::VkCommandPool>(0x4610u)};
  std::size_t create_command_pool_call_count{};
  std::int32_t create_command_pool_result{UWVM_VK_SUCCESS};
  std::vector<std::vector<native::VkCommandBuffer>>
      allocate_command_buffers_result_batches{
          {reinterpret_cast<native::VkCommandBuffer>(0x4700u),
           reinterpret_cast<native::VkCommandBuffer>(0x4800u)},
          {reinterpret_cast<native::VkCommandBuffer>(0x4900u)}};
  std::size_t allocate_command_buffers_call_count{};
  std::int32_t allocate_command_buffers_result{UWVM_VK_SUCCESS};
  std::int32_t reset_command_pool_result{UWVM_VK_SUCCESS};
  std::int32_t begin_command_buffer_result{UWVM_VK_SUCCESS};
  std::int32_t end_command_buffer_result{UWVM_VK_SUCCESS};
  std::int32_t reset_command_buffer_result{UWVM_VK_SUCCESS};
  native::VkShaderModule create_shader_module_result_handle{
      static_cast<native::VkShaderModule>(0x4A00u)};
  std::int32_t create_shader_module_result{UWVM_VK_SUCCESS};
  native::VkSemaphore create_semaphore_result_handle{
      static_cast<native::VkSemaphore>(0x4400u)};
  std::int32_t create_semaphore_result{UWVM_VK_SUCCESS};
  native::VkFence create_fence_result_handle{
      static_cast<native::VkFence>(0x4500u)};
  std::int32_t create_fence_result{UWVM_VK_SUCCESS};
  std::int32_t get_fence_status_result{UWVM_VK_NOT_READY};
  std::int32_t wait_for_fences_result{UWVM_VK_SUCCESS};
  std::int32_t reset_fences_result{UWVM_VK_SUCCESS};
  native::VkImage create_image_result_handle{
      static_cast<native::VkImage>(0x8800u)};
  std::int32_t create_image_result{UWVM_VK_SUCCESS};
  uwvm_vk_memory_requirements image_memory_requirements{
      .size = 4096u, .alignment = 256u, .memory_type_bits = 0x12u, .reserved = 0u};
  std::int32_t get_image_memory_requirements_result{UWVM_VK_SUCCESS};
  std::int32_t bind_image_memory_result{UWVM_VK_SUCCESS};
  std::int32_t device_wait_idle_result{UWVM_VK_SUCCESS};
  std::int32_t queue_wait_idle_result{UWVM_VK_SUCCESS};
  std::int32_t map_memory_result{UWVM_VK_SUCCESS};
  std::int32_t flush_mapped_memory_result{UWVM_VK_SUCCESS};
  std::int32_t invalidate_mapped_memory_result{UWVM_VK_SUCCESS};
  std::vector<std::byte> mapped_memory{std::vector<std::byte>(64u)};

  native::VkPhysicalDevice last_extension_query_physical_device{};
  std::string last_extension_query_layer_name{};
  native::VkPhysicalDevice last_feature_query_physical_device{};
  native::VkPhysicalDevice last_create_device_physical_device{};
  std::uint32_t last_queue_family_index{};
  float last_queue_priority{};
  bool create_device_enabled_features_present{};
  uwvm_vk_physical_device_features captured_enabled_features{};
  native::VkDevice last_device_wait_idle_device{};
  std::size_t device_wait_idle_calls{};
  native::VkDevice last_get_device_queue_device{};
  std::uint32_t last_get_device_queue_family_index{};
  std::uint32_t last_get_device_queue_index{};
  native::VkDevice last_queue_wait_idle_device{};
  native::VkQueue last_queue_wait_idle_queue{};
  native::VkDevice last_create_command_pool_device{};
  std::uint32_t last_create_command_pool_flags{};
  std::uint32_t last_create_command_pool_queue_family_index{};
  native::VkDevice last_reset_command_pool_device{};
  native::VkCommandPool last_reset_command_pool_pool{};
  std::uint32_t last_reset_command_pool_flags{};
  native::VkDevice last_allocate_command_buffers_device{};
  native::VkCommandPool last_allocate_command_buffers_pool{};
  std::uint32_t last_allocate_command_buffers_level{};
  std::uint32_t last_allocate_command_buffers_count{};
  native::VkDevice last_free_command_buffers_device{};
  native::VkCommandPool last_free_command_buffers_pool{};
  std::vector<native::VkCommandBuffer> last_free_command_buffers_command_buffers{};
  native::VkDevice last_begin_command_buffer_device{};
  native::VkCommandBuffer last_begin_command_buffer{};
  std::uint32_t last_begin_command_buffer_flags{};
  native::VkDevice last_end_command_buffer_device{};
  native::VkCommandBuffer last_end_command_buffer{};
  native::VkDevice last_reset_command_buffer_device{};
  native::VkCommandBuffer last_reset_command_buffer{};
  std::uint32_t last_reset_command_buffer_flags{};
  native::VkDevice last_create_shader_module_device{};
  std::uint32_t last_create_shader_module_flags{};
  std::size_t last_create_shader_module_code_size{};
  std::vector<std::uint32_t> last_create_shader_module_words{};
  native::VkDevice last_create_semaphore_device{};
  std::uint32_t last_create_semaphore_flags{};
  native::VkDevice last_create_fence_device{};
  std::uint32_t last_create_fence_flags{};
  native::VkDevice last_get_fence_status_device{};
  native::VkFence last_get_fence_status_fence{};
  native::VkDevice last_wait_for_fences_device{};
  std::vector<native::VkFence> last_wait_for_fences_fences{};
  bool last_wait_for_fences_wait_all{};
  std::uint64_t last_wait_for_fences_timeout_nanoseconds{};
  native::VkDevice last_reset_fences_device{};
  std::vector<native::VkFence> last_reset_fences_fences{};
  native::VkDevice last_create_image_device{};
  std::uint32_t last_create_image_type{};
  std::uint32_t last_create_image_format{};
  native::VkExtent3D last_create_image_extent{};
  std::uint32_t last_create_image_mip_levels{};
  std::uint32_t last_create_image_array_layers{};
  std::uint32_t last_create_image_samples{};
  std::uint32_t last_create_image_tiling{};
  std::uint32_t last_create_image_usage{};
  std::uint32_t last_create_image_sharing_mode{};
  std::uint32_t last_create_image_initial_layout{};
  std::vector<std::uint32_t> last_create_image_queue_families{};
  native::VkImage last_get_image_memory_requirements_image{};
  native::VkDevice last_bind_image_memory_device{};
  native::VkImage last_bind_image_memory_image{};
  native::VkDeviceMemory last_bind_image_memory_memory{};
  std::uint64_t last_bind_image_memory_offset{};
  std::size_t queue_wait_idle_calls{};
  std::size_t map_memory_calls{};
  std::size_t unmap_memory_calls{};
  std::size_t flush_mapped_memory_calls{};
  std::size_t invalidate_mapped_memory_calls{};
  std::vector<DestroyCommandPoolCall> destroyed_command_pools{};
  std::vector<DestroyShaderModuleCall> destroyed_shader_modules{};
  std::vector<DestroySemaphoreCall> destroyed_semaphores{};
  std::vector<DestroyFenceCall> destroyed_fences{};
  std::vector<DestroyBufferCall> destroyed_buffers{};
  std::vector<DestroyImageCall> destroyed_images{};
  std::vector<FreeCommandBuffersCall> free_command_buffers_calls{};
  std::vector<FreeMemoryCall> freed_memories{};
  std::vector<native::VkDevice> destroyed_devices{};
};

} // namespace

int main() {
  FakeMemoryHost host{};
  host.memory = std::vector<std::byte>(4096u);
  host.dynamic_length.store(4096u);
  host.descriptor = uwvm_preload_memory_descriptor_t{
      .memory_index = 0u,
      .delivery_state = UWVM_PRELOAD_MEMORY_DELIVERY_COPY,
      .backend_kind = UWVM_PRELOAD_MEMORY_BACKEND_NATIVE_DEFINED,
      .reserved0 = 0u,
      .reserved1 = 0u,
      .page_count = 1u,
      .page_size_bytes = 65536u,
      .byte_length = 4096u,
      .partial_protection_limit_bytes = 4096u,
      .mmap_view_begin = nullptr,
      .dynamic_length_atomic_object = &host.dynamic_length};
  g_host = &host;

  uwvm_preload_host_api_v1 host_api{.struct_size = sizeof(uwvm_preload_host_api_v1),
                                    .abi_version = 1u,
                                    .memory_descriptor_count = descriptor_count,
                                    .memory_descriptor_at = descriptor_at,
                                    .memory_read = memory_read,
                                    .memory_write = memory_write};

  FakeBackend backend{};
  backend.device_extension_properties.push_back(
      MakeExtensionProperty("VK_KHR_swapchain", 70u));
  backend.device_extension_properties.push_back(
      MakeExtensionProperty("VK_EXT_memory_budget", 1u));
  backend.physical_device_features.geometry_shader = 1u;
  backend.physical_device_features.sampler_anisotropy = 1u;
  backend.physical_device_features.variable_multisample_rate = 1u;

  auto &context{PluginContext::Instance()};
  context.SetExplicitHostApi(&host_api);
  context.SetBackendForTesting(&backend);

  auto const native_instance{
      reinterpret_cast<native::VkInstance>(0x1100u)};
  auto const native_physical_device{
      reinterpret_cast<native::VkPhysicalDevice>(0x2200u)};
  auto const native_memory{static_cast<native::VkDeviceMemory>(0x5500u)};
  auto const native_buffer{static_cast<native::VkBuffer>(0x7700u)};
  auto const native_image{static_cast<native::VkImage>(0x8800u)};
  auto const native_semaphore{static_cast<native::VkSemaphore>(0x4400u)};
  auto const native_fence{static_cast<native::VkFence>(0x4500u)};
  auto const native_command_pool{
      static_cast<native::VkCommandPool>(0x4600u)};
  auto const native_destroyed_command_pool{
      static_cast<native::VkCommandPool>(0x4610u)};
  auto const native_command_buffer{
      reinterpret_cast<native::VkCommandBuffer>(0x4700u)};
  auto const native_freed_command_buffer{
      reinterpret_cast<native::VkCommandBuffer>(0x4800u)};
  auto const native_shader_module{
      static_cast<native::VkShaderModule>(0x4A00u)};
  auto const native_cleanup_shader_module{
      static_cast<native::VkShaderModule>(0x4B00u)};

  auto const instance_handle{
      context.instances.Insert({.native_handle = native_instance})};
  auto const physical_device_handle{
      context.physical_devices.Insert({.native_handle = native_physical_device,
                                       .parent_instance_handle = instance_handle})};

  constexpr std::uint64_t kPriorityAddress{64u};
  constexpr std::uint64_t kQueueInfoAddress{96u};
  constexpr std::uint64_t kEnabledFeaturesAddress{128u};
  constexpr std::uint64_t kCreateInfoAddress{256u};
  constexpr std::uint64_t kOutDeviceHandleAddress{320u};
  constexpr std::uint64_t kLayerNameBytesAddress{384u};
  constexpr std::uint64_t kLayerNameViewAddress{448u};
  constexpr std::uint64_t kExtensionBufferAddress{512u};
  constexpr std::uint64_t kExtensionCountAddress{1088u};
  constexpr std::uint64_t kFeaturesOutAddress{1152u};
  constexpr std::uint64_t kOutQueueHandleAddress{1408u};
  constexpr std::uint64_t kSemaphoreCreateInfoAddress{1440u};
  constexpr std::uint64_t kOutSemaphoreHandleAddress{1456u};
  constexpr std::uint64_t kFenceCreateInfoAddress{1472u};
  constexpr std::uint64_t kOutFenceHandleAddress{1488u};
  constexpr std::uint64_t kFenceHandleBufferAddress{1504u};
  constexpr std::uint64_t kGuestUploadAddress{1536u};
  constexpr std::uint64_t kCopyRegionAddress{1600u};
  constexpr std::uint64_t kImageQueueFamiliesAddress{1664u};
  constexpr std::uint64_t kImageCreateInfoAddress{1728u};
  constexpr std::uint64_t kOutImageHandleAddress{1824u};
  constexpr std::uint64_t kImageRequirementsAddress{1888u};
  constexpr std::uint64_t kCommandPoolCreateInfoAddress{1952u};
  constexpr std::uint64_t kOutCommandPoolHandleAddress{1968u};
  constexpr std::uint64_t kCommandBufferAllocateInfoAddress{1984u};
  constexpr std::uint64_t kCommandBufferHandleBufferAddress{2016u};
  constexpr std::uint64_t kCommandBufferBeginInfoAddress{2048u};
  constexpr std::uint64_t kFreeCommandBufferHandleBufferAddress{2080u};
  constexpr std::uint64_t kOutDestroyedCommandPoolHandleAddress{2096u};
  constexpr std::uint64_t kDestroyedCommandBufferAllocateInfoAddress{2112u};
  constexpr std::uint64_t kDestroyedCommandBufferHandleBufferAddress{2144u};
  constexpr std::uint64_t kShaderCodeAddress{2208u};
  constexpr std::uint64_t kShaderModuleCreateInfoAddress{2240u};
  constexpr std::uint64_t kOutShaderModuleHandleAddress{2272u};
  constexpr std::uint64_t kOutCleanupShaderModuleHandleAddress{2288u};

  float queue_priority{1.0f};
  WriteGuest(kPriorityAddress, queue_priority);

  uwvm_vk_device_queue_create_info queue_create_info{
      .flags = 0u,
      .queue_family_index = 3u,
      .queue_count = 1u,
      .reserved0 = 0u,
      .priorities_address = kPriorityAddress};
  WriteGuest(kQueueInfoAddress, queue_create_info);

  uwvm_vk_physical_device_features enabled_features{};
  enabled_features.sampler_anisotropy = 1u;
  enabled_features.geometry_shader = 1u;
  WriteGuest(kEnabledFeaturesAddress, enabled_features);

  uwvm_vk_device_create_info create_info{
      .flags = 0u,
      .queue_create_info_count = 1u,
      .enabled_extension_count = 0u,
      .reserved0 = 0u,
      .queue_create_infos_address = kQueueInfoAddress,
      .enabled_extensions_address = 0u,
      .enabled_features_address = kEnabledFeaturesAddress};
  WriteGuest(kCreateInfoAddress, create_info);

  require(uwvm2_vulkan::api::CreateDevice(physical_device_handle,
                                          kCreateInfoAddress,
                                          kOutDeviceHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const device_handle{ReadGuest<std::uint64_t>(kOutDeviceHandleAddress)};
  require(device_handle != 0u);
  require(backend.last_create_device_physical_device == native_physical_device);
  require(backend.last_queue_family_index == 3u);
  require(backend.last_queue_priority == 1.0f);
  require(backend.create_device_enabled_features_present);
  require(backend.captured_enabled_features.sampler_anisotropy == 1u);
  require(backend.captured_enabled_features.geometry_shader == 1u);

  auto const device_record{context.devices.Find(device_handle)};
  require(device_record != nullptr);
  auto const native_device_handle{device_record->native_handle};

  WriteGuestString(kLayerNameBytesAddress, "VK_LAYER_KHRONOS_validation");
  uwvm_vk_string_view layer_name_view{
      .data_address = kLayerNameBytesAddress,
      .size = 27u,
      .reserved = 0u};
  WriteGuest(kLayerNameViewAddress, layer_name_view);

  require(uwvm2_vulkan::api::EnumerateDeviceExtensionProperties(
              physical_device_handle, kLayerNameViewAddress,
              kExtensionBufferAddress, 1u, kExtensionCountAddress) ==
          UWVM_VK_INCOMPLETE);
  auto const extension_count{ReadGuest<std::uint32_t>(kExtensionCountAddress)};
  require(extension_count == 2u);
  auto const first_extension{
      ReadGuest<uwvm_vk_extension_property>(kExtensionBufferAddress)};
  require(std::strncmp(first_extension.extension_name, "VK_KHR_swapchain",
                       sizeof(first_extension.extension_name)) == 0);
  require(first_extension.spec_version == 70u);
  require(backend.last_extension_query_physical_device == native_physical_device);
  require(backend.last_extension_query_layer_name ==
          "VK_LAYER_KHRONOS_validation");

  require(uwvm2_vulkan::api::GetPhysicalDeviceFeatures(
              physical_device_handle, kFeaturesOutAddress) == UWVM_VK_SUCCESS);
  auto const queried_features{
      ReadGuest<uwvm_vk_physical_device_features>(kFeaturesOutAddress)};
  require(queried_features.geometry_shader == 1u);
  require(queried_features.sampler_anisotropy == 1u);
  require(queried_features.variable_multisample_rate == 1u);
  require(backend.last_feature_query_physical_device == native_physical_device);

  require(uwvm2_vulkan::api::GetDeviceQueue(device_handle, 3u, 0u,
                                            kOutQueueHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const queue_handle{ReadGuest<std::uint64_t>(kOutQueueHandleAddress)};
  require(queue_handle != 0u);
  require(backend.last_get_device_queue_device == native_device_handle);
  require(backend.last_get_device_queue_family_index == 3u);
  require(backend.last_get_device_queue_index == 0u);

  require(uwvm2_vulkan::api::DeviceWaitIdle(device_handle) == UWVM_VK_SUCCESS);
  require(backend.device_wait_idle_calls == 1u);
  require(backend.last_device_wait_idle_device == native_device_handle);

  require(uwvm2_vulkan::api::QueueWaitIdle(queue_handle) == UWVM_VK_SUCCESS);
  require(backend.queue_wait_idle_calls == 1u);
  require(backend.last_queue_wait_idle_device == native_device_handle);
  require(backend.last_queue_wait_idle_queue ==
          backend.get_device_queue_result_handle);

  uwvm_vk_command_pool_create_info command_pool_create_info{
      .flags = UWVM_VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queue_family_index = 3u,
      .reserved0 = 0u};
  WriteGuest(kCommandPoolCreateInfoAddress, command_pool_create_info);
  require(uwvm2_vulkan::api::CreateCommandPool(device_handle,
                                               kCommandPoolCreateInfoAddress,
                                               kOutCommandPoolHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const command_pool_handle{
      ReadGuest<std::uint64_t>(kOutCommandPoolHandleAddress)};
  require(command_pool_handle != 0u);
  require(backend.last_create_command_pool_device == native_device_handle);
  require(backend.last_create_command_pool_flags ==
          UWVM_VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  require(backend.last_create_command_pool_queue_family_index == 3u);

  uwvm_vk_command_buffer_allocate_info command_buffer_allocate_info{
      .command_pool_handle = command_pool_handle,
      .level = UWVM_VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .command_buffer_count = 2u,
      .reserved0 = 0u};
  WriteGuest(kCommandBufferAllocateInfoAddress, command_buffer_allocate_info);
  require(uwvm2_vulkan::api::AllocateCommandBuffers(
              device_handle, kCommandBufferAllocateInfoAddress,
              kCommandBufferHandleBufferAddress) == UWVM_VK_SUCCESS);
  auto const command_buffer_handle{
      ReadGuest<std::uint64_t>(kCommandBufferHandleBufferAddress)};
  auto const freed_command_buffer_handle{
      ReadGuest<std::uint64_t>(kCommandBufferHandleBufferAddress +
                               sizeof(std::uint64_t))};
  require(command_buffer_handle != 0u);
  require(freed_command_buffer_handle != 0u);
  require(command_buffer_handle != freed_command_buffer_handle);
  require(backend.last_allocate_command_buffers_device == native_device_handle);
  require(backend.last_allocate_command_buffers_pool == native_command_pool);
  require(backend.last_allocate_command_buffers_level ==
          UWVM_VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  require(backend.last_allocate_command_buffers_count == 2u);

  uwvm_vk_command_buffer_begin_info command_buffer_begin_info{
      .flags = UWVM_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .reserved0 = 0u,
      .reserved1 = 0u};
  WriteGuest(kCommandBufferBeginInfoAddress, command_buffer_begin_info);
  require(uwvm2_vulkan::api::BeginCommandBuffer(
              device_handle, command_buffer_handle,
              kCommandBufferBeginInfoAddress) == UWVM_VK_SUCCESS);
  require(backend.last_begin_command_buffer_device == native_device_handle);
  require(backend.last_begin_command_buffer == native_command_buffer);
  require(backend.last_begin_command_buffer_flags ==
          UWVM_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  require(uwvm2_vulkan::api::EndCommandBuffer(device_handle,
                                              command_buffer_handle) ==
          UWVM_VK_SUCCESS);
  require(backend.last_end_command_buffer_device == native_device_handle);
  require(backend.last_end_command_buffer == native_command_buffer);

  require(uwvm2_vulkan::api::ResetCommandBuffer(
              device_handle, command_buffer_handle,
              UWVM_VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT) ==
          UWVM_VK_SUCCESS);
  require(backend.last_reset_command_buffer_device == native_device_handle);
  require(backend.last_reset_command_buffer == native_command_buffer);
  require(backend.last_reset_command_buffer_flags ==
          UWVM_VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  WriteGuest(kFreeCommandBufferHandleBufferAddress, freed_command_buffer_handle);
  require(uwvm2_vulkan::api::FreeCommandBuffers(
              device_handle, command_pool_handle,
              kFreeCommandBufferHandleBufferAddress, 1u) == UWVM_VK_SUCCESS);
  require(context.command_buffers.Find(freed_command_buffer_handle) == nullptr);
  require(context.command_buffers.Find(command_buffer_handle) != nullptr);
  require(backend.last_free_command_buffers_device == native_device_handle);
  require(backend.last_free_command_buffers_pool == native_command_pool);
  require(backend.last_free_command_buffers_command_buffers.size() == 1u);
  require(backend.last_free_command_buffers_command_buffers[0] ==
          native_freed_command_buffer);
  require(backend.free_command_buffers_calls.size() == 1u);

  require(uwvm2_vulkan::api::ResetCommandPool(
              device_handle, command_pool_handle,
              UWVM_VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT) ==
          UWVM_VK_SUCCESS);
  require(backend.last_reset_command_pool_device == native_device_handle);
  require(backend.last_reset_command_pool_pool == native_command_pool);
  require(backend.last_reset_command_pool_flags ==
          UWVM_VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

  command_pool_create_info.flags = UWVM_VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  WriteGuest(kCommandPoolCreateInfoAddress, command_pool_create_info);
  require(uwvm2_vulkan::api::CreateCommandPool(
              device_handle, kCommandPoolCreateInfoAddress,
              kOutDestroyedCommandPoolHandleAddress) == UWVM_VK_SUCCESS);
  auto const destroyed_command_pool_handle{
      ReadGuest<std::uint64_t>(kOutDestroyedCommandPoolHandleAddress)};
  require(destroyed_command_pool_handle != 0u);

  uwvm_vk_command_buffer_allocate_info destroyed_command_buffer_allocate_info{
      .command_pool_handle = destroyed_command_pool_handle,
      .level = UWVM_VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .command_buffer_count = 1u,
      .reserved0 = 0u};
  WriteGuest(kDestroyedCommandBufferAllocateInfoAddress,
             destroyed_command_buffer_allocate_info);
  require(uwvm2_vulkan::api::AllocateCommandBuffers(
              device_handle, kDestroyedCommandBufferAllocateInfoAddress,
              kDestroyedCommandBufferHandleBufferAddress) == UWVM_VK_SUCCESS);
  auto const destroyed_command_buffer_handle{
      ReadGuest<std::uint64_t>(kDestroyedCommandBufferHandleBufferAddress)};
  require(destroyed_command_buffer_handle != 0u);
  require(destroyed_command_buffer_handle != command_buffer_handle);

  require(uwvm2_vulkan::api::DestroyCommandPool(
              device_handle, destroyed_command_pool_handle) ==
          UWVM_VK_SUCCESS);
  require(context.command_pools.Find(destroyed_command_pool_handle) == nullptr);
  require(context.command_buffers.Find(destroyed_command_buffer_handle) ==
          nullptr);
  require(backend.destroyed_command_pools.size() == 1u);
  require(backend.destroyed_command_pools[0].device == native_device_handle);
  require(backend.destroyed_command_pools[0].command_pool ==
          native_destroyed_command_pool);

  std::array<std::uint32_t, 4u> shader_module_words{
      0x07230203u, 0x00010000u, 0x000D0003u, 0x00000002u};
  WriteGuestBytes(kShaderCodeAddress, shader_module_words.data(),
                  sizeof(shader_module_words));

  uwvm_vk_shader_module_create_info shader_module_create_info{
      .flags = 5u,
      .reserved0 = 0u,
      .code_size = sizeof(shader_module_words),
      .code_address = kShaderCodeAddress};
  WriteGuest(kShaderModuleCreateInfoAddress, shader_module_create_info);
  require(uwvm2_vulkan::api::CreateShaderModule(
              device_handle, kShaderModuleCreateInfoAddress,
              kOutShaderModuleHandleAddress) == UWVM_VK_SUCCESS);
  auto const shader_module_handle{
      ReadGuest<std::uint64_t>(kOutShaderModuleHandleAddress)};
  require(shader_module_handle != 0u);
  require(context.shader_modules.Find(shader_module_handle) != nullptr);
  require(backend.last_create_shader_module_device == native_device_handle);
  require(backend.last_create_shader_module_flags == 5u);
  require(backend.last_create_shader_module_code_size ==
          sizeof(shader_module_words));
  require(backend.last_create_shader_module_words.size() ==
          shader_module_words.size());
  for (std::size_t i{}; i != shader_module_words.size(); ++i) {
    require(backend.last_create_shader_module_words[i] ==
            shader_module_words[i]);
  }

  require(uwvm2_vulkan::api::DestroyShaderModule(device_handle,
                                                 shader_module_handle) ==
          UWVM_VK_SUCCESS);
  require(context.shader_modules.Find(shader_module_handle) == nullptr);
  require(backend.destroyed_shader_modules.size() == 1u);
  require(backend.destroyed_shader_modules[0].device == native_device_handle);
  require(backend.destroyed_shader_modules[0].shader_module ==
          native_shader_module);

  backend.create_shader_module_result_handle = native_cleanup_shader_module;
  require(uwvm2_vulkan::api::CreateShaderModule(
              device_handle, kShaderModuleCreateInfoAddress,
              kOutCleanupShaderModuleHandleAddress) == UWVM_VK_SUCCESS);
  auto const cleanup_shader_module_handle{
      ReadGuest<std::uint64_t>(kOutCleanupShaderModuleHandleAddress)};
  require(cleanup_shader_module_handle != 0u);
  require(cleanup_shader_module_handle != shader_module_handle);
  require(context.shader_modules.Find(cleanup_shader_module_handle) != nullptr);

  uwvm_vk_semaphore_create_info semaphore_create_info{
      .flags = 0u, .reserved0 = 0u, .reserved1 = 0u};
  WriteGuest(kSemaphoreCreateInfoAddress, semaphore_create_info);
  require(uwvm2_vulkan::api::CreateSemaphore(device_handle,
                                             kSemaphoreCreateInfoAddress,
                                             kOutSemaphoreHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const semaphore_handle{
      ReadGuest<std::uint64_t>(kOutSemaphoreHandleAddress)};
  require(semaphore_handle != 0u);
  require(backend.last_create_semaphore_device == native_device_handle);
  require(backend.last_create_semaphore_flags == 0u);

  uwvm_vk_fence_create_info fence_create_info{
      .flags = UWVM_VK_FENCE_CREATE_SIGNALED_BIT,
      .reserved0 = 0u,
      .reserved1 = 0u};
  WriteGuest(kFenceCreateInfoAddress, fence_create_info);
  require(uwvm2_vulkan::api::CreateFence(device_handle,
                                         kFenceCreateInfoAddress,
                                         kOutFenceHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const fence_handle{ReadGuest<std::uint64_t>(kOutFenceHandleAddress)};
  require(fence_handle != 0u);
  require(backend.last_create_fence_device == native_device_handle);
  require(backend.last_create_fence_flags == UWVM_VK_FENCE_CREATE_SIGNALED_BIT);

  WriteGuest(kFenceHandleBufferAddress, fence_handle);
  require(uwvm2_vulkan::api::GetFenceStatus(device_handle, fence_handle) ==
          UWVM_VK_NOT_READY);
  require(backend.last_get_fence_status_device == native_device_handle);
  require(backend.last_get_fence_status_fence == native_fence);

  require(uwvm2_vulkan::api::WaitForFences(device_handle,
                                           kFenceHandleBufferAddress, 1u, 1u,
                                           123456789u) == UWVM_VK_SUCCESS);
  require(backend.last_wait_for_fences_device == native_device_handle);
  require(backend.last_wait_for_fences_fences.size() == 1u);
  require(backend.last_wait_for_fences_fences[0] == native_fence);
  require(backend.last_wait_for_fences_wait_all);
  require(backend.last_wait_for_fences_timeout_nanoseconds == 123456789u);

  require(uwvm2_vulkan::api::ResetFences(device_handle,
                                         kFenceHandleBufferAddress, 1u) ==
          UWVM_VK_SUCCESS);
  require(backend.last_reset_fences_device == native_device_handle);
  require(backend.last_reset_fences_fences.size() == 1u);
  require(backend.last_reset_fences_fences[0] == native_fence);

  auto const memory_handle{context.memories.Insert(
      {.native_handle = native_memory,
       .parent_device_handle = device_handle,
       .allocation_size = 64u,
       .memory_type_index = 0u})};
  auto const buffer_handle{context.buffers.Insert(
      {.native_handle = native_buffer, .parent_device_handle = device_handle})};
  require(memory_handle != 0u);
  require(buffer_handle != 0u);

  std::array<std::uint32_t, 2u> image_queue_families{1u, 3u};
  WriteGuestBytes(kImageQueueFamiliesAddress, image_queue_families.data(),
                  sizeof(image_queue_families));

  uwvm_vk_image_create_info image_create_info{
      .flags = 0u,
      .image_type = UWVM_VK_IMAGE_TYPE_2D,
      .format = 37u,
      .mip_levels = 1u,
      .array_layers = 1u,
      .samples = UWVM_VK_SAMPLE_COUNT_1_BIT,
      .tiling = UWVM_VK_IMAGE_TILING_OPTIMAL,
      .usage = 0x10u,
      .sharing_mode = UWVM_VK_SHARING_MODE_CONCURRENT,
      .initial_layout = 0u,
      .extent = {.width = 64u, .height = 32u, .depth = 1u, .reserved = 0u},
      .queue_family_index_count = 2u,
      .reserved0 = 0u,
      .queue_family_indices_address = kImageQueueFamiliesAddress};
  WriteGuest(kImageCreateInfoAddress, image_create_info);

  require(uwvm2_vulkan::api::CreateImage(device_handle, kImageCreateInfoAddress,
                                         kOutImageHandleAddress) ==
          UWVM_VK_SUCCESS);
  auto const image_handle{ReadGuest<std::uint64_t>(kOutImageHandleAddress)};
  require(image_handle != 0u);
  require(backend.last_create_image_device == native_device_handle);
  require(backend.last_create_image_type == UWVM_VK_IMAGE_TYPE_2D);
  require(backend.last_create_image_format == 37u);
  require(backend.last_create_image_extent.width == 64u);
  require(backend.last_create_image_extent.height == 32u);
  require(backend.last_create_image_extent.depth == 1u);
  require(backend.last_create_image_mip_levels == 1u);
  require(backend.last_create_image_array_layers == 1u);
  require(backend.last_create_image_samples == UWVM_VK_SAMPLE_COUNT_1_BIT);
  require(backend.last_create_image_tiling == UWVM_VK_IMAGE_TILING_OPTIMAL);
  require(backend.last_create_image_usage == 0x10u);
  require(backend.last_create_image_sharing_mode ==
          UWVM_VK_SHARING_MODE_CONCURRENT);
  require(backend.last_create_image_initial_layout == 0u);
  require(backend.last_create_image_queue_families.size() == 2u);
  require(backend.last_create_image_queue_families[0] == 1u);
  require(backend.last_create_image_queue_families[1] == 3u);

  require(uwvm2_vulkan::api::GetImageMemoryRequirements(
              device_handle, image_handle, kImageRequirementsAddress) ==
          UWVM_VK_SUCCESS);
  auto const image_requirements{
      ReadGuest<uwvm_vk_memory_requirements>(kImageRequirementsAddress)};
  require(image_requirements.size == backend.image_memory_requirements.size);
  require(image_requirements.alignment ==
          backend.image_memory_requirements.alignment);
  require(image_requirements.memory_type_bits ==
          backend.image_memory_requirements.memory_type_bits);
  require(backend.last_get_image_memory_requirements_image == native_image);

  require(uwvm2_vulkan::api::BindImageMemory(device_handle, image_handle,
                                             memory_handle, 16u) ==
          UWVM_VK_SUCCESS);
  require(backend.last_bind_image_memory_device == native_device_handle);
  require(backend.last_bind_image_memory_image == native_image);
  require(backend.last_bind_image_memory_memory == native_memory);
  require(backend.last_bind_image_memory_offset == 16u);

  std::array<std::byte, 4u> upload_bytes{
      std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44}};
  WriteGuestBytes(kGuestUploadAddress, upload_bytes.data(), upload_bytes.size());

  uwvm_vk_memory_copy_region copy_region{
      .guest_memory_index = 0u,
      .reserved = 0u,
      .guest_address = kGuestUploadAddress,
      .device_offset = 0u,
      .size = upload_bytes.size()};
  WriteGuest(kCopyRegionAddress, copy_region);

  backend.flush_mapped_memory_result = UWVM_VK_ERROR_DEVICE_LOST;
  require(uwvm2_vulkan::api::CopyGuestToDeviceMemory(device_handle, memory_handle,
                                                      kCopyRegionAddress) ==
          UWVM_VK_ERROR_DEVICE_LOST);
  require(backend.map_memory_calls == 1u);
  require(backend.flush_mapped_memory_calls == 1u);
  require(backend.unmap_memory_calls == 1u);

  require(uwvm2_vulkan::api::DestroyDevice(device_handle) == UWVM_VK_SUCCESS);
  require(context.devices.Find(device_handle) == nullptr);
  require(context.queues.Find(queue_handle) == nullptr);
  require(context.command_pools.Find(command_pool_handle) == nullptr);
  require(context.command_buffers.Find(command_buffer_handle) == nullptr);
  require(context.command_buffers.Find(freed_command_buffer_handle) == nullptr);
  require(context.shader_modules.Find(cleanup_shader_module_handle) == nullptr);
  require(context.semaphores.Find(semaphore_handle) == nullptr);
  require(context.fences.Find(fence_handle) == nullptr);
  require(context.buffers.Find(buffer_handle) == nullptr);
  require(context.images.Find(image_handle) == nullptr);
  require(context.memories.Find(memory_handle) == nullptr);
  require(backend.destroyed_devices.size() == 1u);
  require(backend.destroyed_devices[0] == native_device_handle);
  require(backend.destroyed_command_pools.size() == 2u);
  require(backend.destroyed_command_pools[1].device == native_device_handle);
  require(backend.destroyed_command_pools[1].command_pool ==
          native_command_pool);
  require(backend.destroyed_shader_modules.size() == 2u);
  require(backend.destroyed_shader_modules[1].device == native_device_handle);
  require(backend.destroyed_shader_modules[1].shader_module ==
          native_cleanup_shader_module);
  require(backend.destroyed_semaphores.size() == 1u);
  require(backend.destroyed_semaphores[0].device == native_device_handle);
  require(backend.destroyed_semaphores[0].semaphore == native_semaphore);
  require(backend.destroyed_fences.size() == 1u);
  require(backend.destroyed_fences[0].device == native_device_handle);
  require(backend.destroyed_fences[0].fence == native_fence);
  require(backend.destroyed_buffers.size() == 1u);
  require(backend.destroyed_buffers[0].device == native_device_handle);
  require(backend.destroyed_buffers[0].buffer == native_buffer);
  require(backend.destroyed_images.size() == 1u);
  require(backend.destroyed_images[0].device == native_device_handle);
  require(backend.destroyed_images[0].image == native_image);
  require(backend.freed_memories.size() == 1u);
  require(backend.freed_memories[0].device == native_device_handle);
  require(backend.freed_memories[0].memory == native_memory);

  context.SetBackendForTesting(nullptr);
  context.SetExplicitHostApi(nullptr);
  context.SetExplicitWasiHostApi(nullptr);
  g_host = nullptr;
  return 0;
}
