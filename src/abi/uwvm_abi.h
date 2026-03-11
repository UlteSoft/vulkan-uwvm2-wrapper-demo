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

#ifndef UWVM2_VULKAN_SRC_ABI_UWVM_ABI_H_
#define UWVM2_VULKAN_SRC_ABI_UWVM_ABI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "uwvm2_vulkan/impl.h"

extern "C" {

typedef void (*uwvm_imported_c_handlefunc_ptr_t)(void);
typedef void (*uwvm_capi_wasm_function)(unsigned char *res,
                                        unsigned char *para);

enum {
  UWVM_WASM_VALTYPE_I32 = 0x7F,
  UWVM_WASM_VALTYPE_I64 = 0x7E,
  UWVM_WASM_VALTYPE_F32 = 0x7D,
  UWVM_WASM_VALTYPE_F64 = 0x7C
};

typedef struct uwvm_capi_module_name_t {
  char const *name;
  size_t name_length;
} uwvm_capi_module_name_t;

typedef struct uwvm_capi_custom_handler_t {
  char const *custom_name_ptr;
  size_t custom_name_length;
  uwvm_imported_c_handlefunc_ptr_t custom_handle_func;
} uwvm_capi_custom_handler_t;

typedef struct uwvm_capi_custom_handler_vec_t {
  uwvm_capi_custom_handler_t const *custom_handler_begin;
  size_t custom_handler_size;
} uwvm_capi_custom_handler_vec_t;

typedef struct uwvm_capi_function_t {
  char const *func_name_ptr;
  size_t func_name_length;
  uint_least8_t const *para_type_vec_begin;
  size_t para_type_vec_size;
  uint_least8_t const *res_type_vec_begin;
  size_t res_type_vec_size;
  uwvm_capi_wasm_function func_ptr;
} uwvm_capi_function_t;

typedef struct uwvm_capi_function_vec_t {
  uwvm_capi_function_t const *function_begin;
  size_t function_size;
} uwvm_capi_function_vec_t;

typedef struct uwvm_weak_symbol_module_c {
  char const *module_name_ptr;
  size_t module_name_length;
  uwvm_capi_custom_handler_vec_t custom_handler_vec;
  uwvm_capi_function_vec_t function_vec;
} uwvm_weak_symbol_module_c;

typedef struct uwvm_weak_symbol_module_vector_c {
  uwvm_weak_symbol_module_c const *module_ptr;
  size_t module_count;
} uwvm_weak_symbol_module_vector_c;

typedef enum uwvm_preload_memory_delivery_state_t {
  UWVM_PRELOAD_MEMORY_DELIVERY_NONE = 0u,
  UWVM_PRELOAD_MEMORY_DELIVERY_COPY = 1u,
  UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_FULL_PROTECTION = 2u,
  UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_PARTIAL_PROTECTION = 3u,
  UWVM_PRELOAD_MEMORY_DELIVERY_MMAP_DYNAMIC_BOUNDS = 4u
} uwvm_preload_memory_delivery_state_t;

typedef enum uwvm_preload_memory_backend_kind_t {
  UWVM_PRELOAD_MEMORY_BACKEND_NATIVE_DEFINED = 0u,
  UWVM_PRELOAD_MEMORY_BACKEND_LOCAL_IMPORTED = 1u
} uwvm_preload_memory_backend_kind_t;

typedef struct uwvm_preload_memory_descriptor_t {
  size_t memory_index;
  unsigned delivery_state;
  unsigned backend_kind;
  unsigned reserved0;
  unsigned reserved1;
  uint_least64_t page_count;
  uint_least64_t page_size_bytes;
  uint_least64_t byte_length;
  uint_least64_t partial_protection_limit_bytes;
  void *mmap_view_begin;
  void const *dynamic_length_atomic_object;
} uwvm_preload_memory_descriptor_t;

typedef size_t (*uwvm_preload_memory_descriptor_count_t)(void);
typedef bool (*uwvm_preload_memory_descriptor_at_t)(
    size_t, uwvm_preload_memory_descriptor_t *);
typedef bool (*uwvm_preload_memory_read_t)(size_t, uint_least64_t, void *,
                                           size_t);
typedef bool (*uwvm_preload_memory_write_t)(size_t, uint_least64_t,
                                            void const *, size_t);

typedef struct uwvm_preload_host_api_v1 {
  size_t struct_size;
  uint_least32_t abi_version;
  uwvm_preload_memory_descriptor_count_t memory_descriptor_count;
  uwvm_preload_memory_descriptor_at_t memory_descriptor_at;
  uwvm_preload_memory_read_t memory_read;
  uwvm_preload_memory_write_t memory_write;
} uwvm_preload_host_api_v1;

typedef void (*uwvm_set_preload_host_api_v1_t)(
    uwvm_preload_host_api_v1 const *);

UWVM2_VULKAN_WEAK_IMPORT uwvm_preload_host_api_v1 const *
uwvm_get_preload_host_api_v1(void);
}

#endif
