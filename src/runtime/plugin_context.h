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

#ifndef UWVM2_VULKAN_SRC_RUNTIME_PLUGIN_CONTEXT_H_
#define UWVM2_VULKAN_SRC_RUNTIME_PLUGIN_CONTEXT_H_

#include <cstdint>
#include <mutex>

#include "abi/uwvm_abi.h"
#include "runtime/handle_registry.h"
#include "runtime/memory_access.h"
#include "runtime/wasi_file_system.h"
#include "vulkan/backend.h"

namespace uwvm2_vulkan::runtime {

class PluginContext {
public:
  static PluginContext &Instance() noexcept;

  void SetExplicitHostApi(uwvm_preload_host_api_v1 const *host_api) noexcept;
  void SetExplicitWasiHostApi(
      uwvm_wasip1_host_api_v1 const *host_api) noexcept;
  [[nodiscard]] uwvm_preload_host_api_v1 const *ResolveHostApi() const noexcept;
  [[nodiscard]] uwvm_wasip1_host_api_v1 const *
  ResolveWasiHostApi() const noexcept;
  [[nodiscard]] GuestMemoryAccessor &memory() noexcept;
  [[nodiscard]] WasiFileSystem &wasi_filesystem() noexcept;
  [[nodiscard]] vk::Backend &backend() noexcept;
  void SetBackendForTesting(vk::Backend *backend) noexcept;

  [[nodiscard]] std::mutex &mutex() noexcept;

  HandleTable<InstanceRecord> instances{};
  HandleTable<PhysicalDeviceRecord> physical_devices{};
  HandleTable<DeviceRecord> devices{};
  HandleTable<QueueRecord> queues{};
  HandleTable<CommandPoolRecord> command_pools{};
  HandleTable<CommandBufferRecord> command_buffers{};
  HandleTable<ShaderModuleRecord> shader_modules{};
  HandleTable<SemaphoreRecord> semaphores{};
  HandleTable<FenceRecord> fences{};
  HandleTable<BufferRecord> buffers{};
  HandleTable<ImageRecord> images{};
  HandleTable<DeviceMemoryRecord> memories{};

private:
  PluginContext() = default;

  mutable std::mutex mutex_{};
  uwvm_preload_host_api_v1 const *explicit_host_api_{};
  uwvm_wasip1_host_api_v1 const *explicit_wasi_host_api_{};
  GuestMemoryAccessor memory_{};
  WasiFileSystem wasi_filesystem_{};
  vk::DynamicBackend dynamic_backend_{};
  vk::Backend *override_backend_{};
};

} // namespace uwvm2_vulkan::runtime

#endif
