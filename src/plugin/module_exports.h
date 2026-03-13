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

#ifndef UWVM2_VULKAN_SRC_PLUGIN_MODULE_EXPORTS_H_
#define UWVM2_VULKAN_SRC_PLUGIN_MODULE_EXPORTS_H_

#include "abi/uwvm_abi.h"

namespace uwvm2_vulkan::plugin {

[[nodiscard]] uwvm_capi_module_name_t GetModuleName() noexcept;
[[nodiscard]] uwvm_capi_custom_handler_vec_t GetCustomHandlerVec() noexcept;
[[nodiscard]] uwvm_capi_function_vec_t GetFunctionVec() noexcept;
[[nodiscard]] uwvm_weak_symbol_module_vector_c const *
GetWeakModuleVector() noexcept;
void SetPreloadHostApi(uwvm_preload_host_api_v1 const *host_api) noexcept;
void SetWasiHostApi(uwvm_wasip1_host_api_v1 const *host_api) noexcept;

} // namespace uwvm2_vulkan::plugin

#endif
