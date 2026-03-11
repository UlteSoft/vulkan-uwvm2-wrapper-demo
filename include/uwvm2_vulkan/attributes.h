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
#ifndef UWVM2_VULKAN_ATTRIBUTES_H_
#define UWVM2_VULKAN_ATTRIBUTES_H_

#include <stdint.h>

#ifdef __cplusplus
#define UWVM2_VULKAN_EXTERN_C_BEGIN extern "C" {
#define UWVM2_VULKAN_EXTERN_C_END }
#define __WASI_NOEXCEPT noexcept
#else
#define UWVM2_VULKAN_EXTERN_C_BEGIN
#define UWVM2_VULKAN_EXTERN_C_END
#define __WASI_NOEXCEPT
#endif

#if (defined(__clang__) || defined(__GNUC__)) && defined(__wasm__)
#define UWVM2_VULKAN_IMPORT_MODULE(name_literal)                               \
  __attribute__((__import_module__(name_literal)))
#define UWVM2_VULKAN_IMPORT_NAME(name_literal)                                 \
  __attribute__((__import_name__(name_literal)))
#else
#define UWVM2_VULKAN_IMPORT_MODULE(name_literal)
#define UWVM2_VULKAN_IMPORT_NAME(name_literal)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define UWVM2_VULKAN_PACKED __attribute__((packed))
#else
#define UWVM2_VULKAN_PACKED
#endif

#ifndef IMPORT_NAME
#define IMPORT_NAME(name_literal) UWVM2_VULKAN_IMPORT_NAME(name_literal)
#endif

#endif
