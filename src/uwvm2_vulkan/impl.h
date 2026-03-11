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

#pragma once

#ifndef UWVM2_VULKAN_MODULE_EXPORT
#define UWVM2_VULKAN_MODULE_EXPORT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define UWVM2_VULKAN_PACKED __attribute__((packed))
#define UWVM2_VULKAN_MAY_ALIAS __attribute__((__may_alias__))
#else
#define UWVM2_VULKAN_PACKED
#define UWVM2_VULKAN_MAY_ALIAS
#endif

#if defined(__APPLE__)
#define UWVM2_VULKAN_WEAK_IMPORT __attribute__((weak_import))
#elif defined(__GNUC__) || defined(__clang__)
#define UWVM2_VULKAN_WEAK_IMPORT __attribute__((weak))
#else
#define UWVM2_VULKAN_WEAK_IMPORT
#endif

#if defined(UWVM2_VULKAN_BUILD_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#define UWVM2_VULKAN_DLL_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define UWVM2_VULKAN_DLL_EXPORT __attribute__((visibility("default")))
#else
#define UWVM2_VULKAN_DLL_EXPORT
#endif
#else
#define UWVM2_VULKAN_DLL_EXPORT
#endif
