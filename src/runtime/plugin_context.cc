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
#include "runtime/plugin_context.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace uwvm2_vulkan::runtime {

namespace {

using host_api_getter_t = uwvm_preload_host_api_v1 const *(*)();
using wasi_host_api_getter_t = uwvm_wasip1_host_api_v1 const *(*)();

[[nodiscard]] host_api_getter_t LookupHostApiGetter() noexcept {
#if defined(_WIN32)
  auto *module_handle{::GetModuleHandleW(nullptr)};
  if (module_handle == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<host_api_getter_t>(
      ::GetProcAddress(module_handle, "uwvm_get_preload_host_api_v1"));
#else
  return reinterpret_cast<host_api_getter_t>(
      ::dlsym(RTLD_DEFAULT, "uwvm_get_preload_host_api_v1"));
#endif
}

[[nodiscard]] wasi_host_api_getter_t LookupWasiHostApiGetter() noexcept {
#if defined(_WIN32)
  auto *module_handle{::GetModuleHandleW(nullptr)};
  if (module_handle == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<wasi_host_api_getter_t>(
      ::GetProcAddress(module_handle, "uwvm_get_wasip1_host_api_v1"));
#else
  return reinterpret_cast<wasi_host_api_getter_t>(
      ::dlsym(RTLD_DEFAULT, "uwvm_get_wasip1_host_api_v1"));
#endif
}

} // namespace

PluginContext &PluginContext::Instance() noexcept {
  static PluginContext context;
  return context;
}

void PluginContext::SetExplicitHostApi(
    uwvm_preload_host_api_v1 const *host_api) noexcept {
  explicit_host_api_ = host_api;
  memory_.SetHostApi(ResolveHostApi());
}

void PluginContext::SetExplicitWasiHostApi(
    uwvm_wasip1_host_api_v1 const *host_api) noexcept {
  explicit_wasi_host_api_ = host_api;
  wasi_filesystem_.SetHostApi(ResolveWasiHostApi());
}

uwvm_preload_host_api_v1 const *PluginContext::ResolveHostApi() const noexcept {
  if (explicit_host_api_ != nullptr) {
    return explicit_host_api_;
  }

  auto const getter{LookupHostApiGetter()};
  if (getter != nullptr) {
    return getter();
  }

  return nullptr;
}

uwvm_wasip1_host_api_v1 const *PluginContext::ResolveWasiHostApi() const noexcept {
  if (explicit_wasi_host_api_ != nullptr) {
    return explicit_wasi_host_api_;
  }

  auto const getter{LookupWasiHostApiGetter()};
  if (getter != nullptr) {
    return getter();
  }

  return nullptr;
}

GuestMemoryAccessor &PluginContext::memory() noexcept {
  memory_.SetHostApi(ResolveHostApi());
  return memory_;
}

WasiFileSystem &PluginContext::wasi_filesystem() noexcept {
  wasi_filesystem_.SetHostApi(ResolveWasiHostApi());
  return wasi_filesystem_;
}

vk::Backend &PluginContext::backend() noexcept {
  return override_backend_ == nullptr
             ? static_cast<vk::Backend &>(dynamic_backend_)
             : *override_backend_;
}

void PluginContext::SetBackendForTesting(vk::Backend *backend) noexcept {
  override_backend_ = backend;
}

std::mutex &PluginContext::mutex() noexcept { return mutex_; }

} // namespace uwvm2_vulkan::runtime
