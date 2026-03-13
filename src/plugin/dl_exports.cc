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

#include "uwvm2_vulkan/impl.h"

extern "C" UWVM2_VULKAN_DLL_EXPORT void uwvm_set_preload_host_api_v1(
    uwvm_preload_host_api_v1 const *host_api) noexcept {
  uwvm2_vulkan::plugin::SetPreloadHostApi(host_api);
}

extern "C" UWVM2_VULKAN_DLL_EXPORT void uwvm_set_wasip1_host_api_v1(
    uwvm_wasip1_host_api_v1 const *host_api) noexcept {
  uwvm2_vulkan::plugin::SetWasiHostApi(host_api);
}

extern "C" UWVM2_VULKAN_DLL_EXPORT uwvm_capi_module_name_t
uwvm_get_module_name() noexcept {
  return uwvm2_vulkan::plugin::GetModuleName();
}

extern "C" UWVM2_VULKAN_DLL_EXPORT uwvm_capi_custom_handler_vec_t
uwvm_get_custom_handler() noexcept {
  return uwvm2_vulkan::plugin::GetCustomHandlerVec();
}

extern "C" UWVM2_VULKAN_DLL_EXPORT uwvm_capi_function_vec_t
uwvm_function() noexcept {
  return uwvm2_vulkan::plugin::GetFunctionVec();
}
