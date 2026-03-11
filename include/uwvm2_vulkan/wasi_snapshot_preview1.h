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
#ifndef UWVM2_VULKAN_WASI_SNAPSHOT_PREVIEW1_H_
#define UWVM2_VULKAN_WASI_SNAPSHOT_PREVIEW1_H_

#include <stdint.h>

#include "attributes.h"

UWVM2_VULKAN_EXTERN_C_BEGIN

int32_t __imported_wasi_snapshot_preview1_args_get(intptr_t argv_ptr,
                                                   intptr_t argv_buf_ptr)
    __WASI_NOEXCEPT UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("args_get");

int32_t __imported_wasi_snapshot_preview1_args_sizes_get(
    intptr_t argc_ptr, intptr_t argv_buf_size_ptr) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("args_sizes_get");

int32_t __imported_wasi_snapshot_preview1_environ_get(intptr_t environ_ptr,
                                                      intptr_t environ_buf_ptr)
    __WASI_NOEXCEPT UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("environ_get");

int32_t __imported_wasi_snapshot_preview1_environ_sizes_get(
    intptr_t environ_count_ptr, intptr_t environ_buf_size_ptr) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("environ_sizes_get");

int32_t __imported_wasi_snapshot_preview1_clock_time_get(
    int32_t clock_id, int64_t precision, intptr_t result_ptr) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("clock_time_get");

int32_t __imported_wasi_snapshot_preview1_fd_write(
    int32_t fd, intptr_t iovs_ptr, int32_t iovs_len,
    intptr_t nwritten_ptr) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("fd_write");

int32_t
__imported_wasi_snapshot_preview1_fd_read(int32_t fd, intptr_t iovs_ptr,
                                          int32_t iovs_len,
                                          intptr_t nread_ptr) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1") IMPORT_NAME("fd_read");

int32_t __imported_wasi_snapshot_preview1_random_get(intptr_t buffer_ptr,
                                                     intptr_t buffer_len)
    __WASI_NOEXCEPT UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("random_get");

void __imported_wasi_snapshot_preview1_proc_exit(int32_t exit_code)
    __WASI_NOEXCEPT UWVM2_VULKAN_IMPORT_MODULE("wasi_snapshot_preview1")
        IMPORT_NAME("proc_exit");

UWVM2_VULKAN_EXTERN_C_END

#endif
