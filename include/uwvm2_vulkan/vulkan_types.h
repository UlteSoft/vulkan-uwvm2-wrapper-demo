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
#ifndef UWVM2_VULKAN_VULKAN_TYPES_H_
#define UWVM2_VULKAN_VULKAN_TYPES_H_

#include <stdint.h>

#include "attributes.h"

UWVM2_VULKAN_EXTERN_C_BEGIN

typedef uint32_t uwvm_vk_bool32;
typedef uint64_t uwvm_vk_handle_t;

enum {
  UWVM_VK_SUCCESS = 0,
  UWVM_VK_NOT_READY = 1,
  UWVM_VK_TIMEOUT = 2,
  UWVM_VK_EVENT_SET = 3,
  UWVM_VK_EVENT_RESET = 4,
  UWVM_VK_INCOMPLETE = 5,
  UWVM_VK_ERROR_OUT_OF_HOST_MEMORY = -1,
  UWVM_VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
  UWVM_VK_ERROR_INITIALIZATION_FAILED = -3,
  UWVM_VK_ERROR_DEVICE_LOST = -4,
  UWVM_VK_ERROR_MEMORY_MAP_FAILED = -5,
  UWVM_VK_ERROR_LAYER_NOT_PRESENT = -6,
  UWVM_VK_ERROR_EXTENSION_NOT_PRESENT = -7,
  UWVM_VK_ERROR_FEATURE_NOT_PRESENT = -8,
  UWVM_VK_ERROR_INCOMPATIBLE_DRIVER = -9,
  UWVM_VK_ERROR_TOO_MANY_OBJECTS = -10,
  UWVM_VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
  UWVM_VK_ERROR_FRAGMENTED_POOL = -12,
  UWVM_VK_ERROR_UNKNOWN = -13,
  UWVM_VK_ERROR_INVALID_ARGUMENT = -320000000,
  UWVM_VK_ERROR_GUEST_MEMORY = -320000001,
  UWVM_VK_ERROR_INVALID_HANDLE = -320000002,
  UWVM_VK_ERROR_LOADER_UNAVAILABLE = -320000003,
  UWVM_VK_ERROR_UNSUPPORTED_OPERATION = -320000004
};

enum {
  UWVM_VK_MAX_EXTENSION_NAME_SIZE = 256,
  UWVM_VK_MAX_DESCRIPTION_SIZE = 256,
  UWVM_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256,
  UWVM_VK_UUID_SIZE = 16,
  UWVM_VK_MAX_MEMORY_TYPES = 32,
  UWVM_VK_MAX_MEMORY_HEAPS = 16
};

enum {
  UWVM_VK_DEVICE_TYPE_OTHER = 0,
  UWVM_VK_DEVICE_TYPE_INTEGRATED_GPU = 1,
  UWVM_VK_DEVICE_TYPE_DISCRETE_GPU = 2,
  UWVM_VK_DEVICE_TYPE_VIRTUAL_GPU = 3,
  UWVM_VK_DEVICE_TYPE_CPU = 4
};

enum {
  UWVM_VK_SHARING_MODE_EXCLUSIVE = 0,
  UWVM_VK_SHARING_MODE_CONCURRENT = 1
};

enum {
  UWVM_VK_IMAGE_TYPE_1D = 0,
  UWVM_VK_IMAGE_TYPE_2D = 1,
  UWVM_VK_IMAGE_TYPE_3D = 2
};

enum {
  UWVM_VK_IMAGE_TILING_OPTIMAL = 0,
  UWVM_VK_IMAGE_TILING_LINEAR = 1
};

enum {
  UWVM_VK_SAMPLE_COUNT_1_BIT = 0x00000001u,
  UWVM_VK_SAMPLE_COUNT_2_BIT = 0x00000002u,
  UWVM_VK_SAMPLE_COUNT_4_BIT = 0x00000004u,
  UWVM_VK_SAMPLE_COUNT_8_BIT = 0x00000008u,
  UWVM_VK_SAMPLE_COUNT_16_BIT = 0x00000010u,
  UWVM_VK_SAMPLE_COUNT_32_BIT = 0x00000020u,
  UWVM_VK_SAMPLE_COUNT_64_BIT = 0x00000040u
};

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_string_view {
  uint64_t data_address;
  uint32_t size;
  uint32_t reserved;
} uwvm_vk_string_view;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_application_info {
  uwvm_vk_string_view application_name;
  uint32_t application_version;
  uint32_t reserved0;
  uwvm_vk_string_view engine_name;
  uint32_t engine_version;
  uint32_t api_version;
} uwvm_vk_application_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_instance_create_info {
  uint32_t flags;
  uint32_t enabled_layer_count;
  uint32_t enabled_extension_count;
  uint32_t reserved0;
  uint64_t application_info_address;
  uint64_t enabled_layers_address;
  uint64_t enabled_extensions_address;
  uint64_t reserved1;
} uwvm_vk_instance_create_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_extension_property {
  char extension_name[UWVM_VK_MAX_EXTENSION_NAME_SIZE];
  uint32_t spec_version;
} uwvm_vk_extension_property;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_layer_property {
  char layer_name[UWVM_VK_MAX_EXTENSION_NAME_SIZE];
  uint32_t spec_version;
  uint32_t implementation_version;
  char description[UWVM_VK_MAX_DESCRIPTION_SIZE];
} uwvm_vk_layer_property;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_physical_device_properties {
  uint32_t api_version;
  uint32_t driver_version;
  uint32_t vendor_id;
  uint32_t device_id;
  uint32_t device_type;
  uint8_t pipeline_cache_uuid[UWVM_VK_UUID_SIZE];
  char device_name[UWVM_VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
} uwvm_vk_physical_device_properties;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_physical_device_features {
  uwvm_vk_bool32 robust_buffer_access;
  uwvm_vk_bool32 full_draw_index_uint32;
  uwvm_vk_bool32 image_cube_array;
  uwvm_vk_bool32 independent_blend;
  uwvm_vk_bool32 geometry_shader;
  uwvm_vk_bool32 tessellation_shader;
  uwvm_vk_bool32 sample_rate_shading;
  uwvm_vk_bool32 dual_src_blend;
  uwvm_vk_bool32 logic_op;
  uwvm_vk_bool32 multi_draw_indirect;
  uwvm_vk_bool32 draw_indirect_first_instance;
  uwvm_vk_bool32 depth_clamp;
  uwvm_vk_bool32 depth_bias_clamp;
  uwvm_vk_bool32 fill_mode_non_solid;
  uwvm_vk_bool32 depth_bounds;
  uwvm_vk_bool32 wide_lines;
  uwvm_vk_bool32 large_points;
  uwvm_vk_bool32 alpha_to_one;
  uwvm_vk_bool32 multi_viewport;
  uwvm_vk_bool32 sampler_anisotropy;
  uwvm_vk_bool32 texture_compression_etc2;
  uwvm_vk_bool32 texture_compression_astc_ldr;
  uwvm_vk_bool32 texture_compression_bc;
  uwvm_vk_bool32 occlusion_query_precise;
  uwvm_vk_bool32 pipeline_statistics_query;
  uwvm_vk_bool32 vertex_pipeline_stores_and_atomics;
  uwvm_vk_bool32 fragment_stores_and_atomics;
  uwvm_vk_bool32 shader_tessellation_and_geometry_point_size;
  uwvm_vk_bool32 shader_image_gather_extended;
  uwvm_vk_bool32 shader_storage_image_extended_formats;
  uwvm_vk_bool32 shader_storage_image_multisample;
  uwvm_vk_bool32 shader_storage_image_read_without_format;
  uwvm_vk_bool32 shader_storage_image_write_without_format;
  uwvm_vk_bool32 shader_uniform_buffer_array_dynamic_indexing;
  uwvm_vk_bool32 shader_sampled_image_array_dynamic_indexing;
  uwvm_vk_bool32 shader_storage_buffer_array_dynamic_indexing;
  uwvm_vk_bool32 shader_storage_image_array_dynamic_indexing;
  uwvm_vk_bool32 shader_clip_distance;
  uwvm_vk_bool32 shader_cull_distance;
  uwvm_vk_bool32 shader_float64;
  uwvm_vk_bool32 shader_int64;
  uwvm_vk_bool32 shader_int16;
  uwvm_vk_bool32 shader_resource_residency;
  uwvm_vk_bool32 shader_resource_min_lod;
  uwvm_vk_bool32 sparse_binding;
  uwvm_vk_bool32 sparse_residency_buffer;
  uwvm_vk_bool32 sparse_residency_image2d;
  uwvm_vk_bool32 sparse_residency_image3d;
  uwvm_vk_bool32 sparse_residency2_samples;
  uwvm_vk_bool32 sparse_residency4_samples;
  uwvm_vk_bool32 sparse_residency8_samples;
  uwvm_vk_bool32 sparse_residency16_samples;
  uwvm_vk_bool32 sparse_residency_aliased;
  uwvm_vk_bool32 variable_multisample_rate;
  uwvm_vk_bool32 inherited_queries;
} uwvm_vk_physical_device_features;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_queue_family_properties {
  uint32_t queue_flags;
  uint32_t queue_count;
  uint32_t timestamp_valid_bits;
  uint32_t min_image_transfer_granularity_x;
  uint32_t min_image_transfer_granularity_y;
  uint32_t min_image_transfer_granularity_z;
} uwvm_vk_queue_family_properties;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_memory_type {
  uint32_t property_flags;
  uint32_t heap_index;
} uwvm_vk_memory_type;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_memory_heap {
  uint64_t size;
  uint32_t flags;
  uint32_t reserved;
} uwvm_vk_memory_heap;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_physical_device_memory_properties {
  uint32_t memory_type_count;
  uint32_t memory_heap_count;
  uwvm_vk_memory_type memory_types[UWVM_VK_MAX_MEMORY_TYPES];
  uwvm_vk_memory_heap memory_heaps[UWVM_VK_MAX_MEMORY_HEAPS];
} uwvm_vk_physical_device_memory_properties;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_device_queue_create_info {
  uint32_t flags;
  uint32_t queue_family_index;
  uint32_t queue_count;
  uint32_t reserved0;
  uint64_t priorities_address;
} uwvm_vk_device_queue_create_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_device_create_info {
  uint32_t flags;
  uint32_t queue_create_info_count;
  uint32_t enabled_extension_count;
  uint32_t reserved0;
  uint64_t queue_create_infos_address;
  uint64_t enabled_extensions_address;
  uint64_t enabled_features_address;
} uwvm_vk_device_create_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_buffer_create_info {
  uint32_t flags;
  uint32_t usage;
  uint32_t sharing_mode;
  uint32_t queue_family_index_count;
  uint64_t size;
  uint64_t queue_family_indices_address;
} uwvm_vk_buffer_create_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_extent3d {
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  uint32_t reserved;
} uwvm_vk_extent3d;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_image_create_info {
  uint32_t flags;
  uint32_t image_type;
  uint32_t format;
  uint32_t mip_levels;
  uint32_t array_layers;
  uint32_t samples;
  uint32_t tiling;
  uint32_t usage;
  uint32_t sharing_mode;
  uint32_t initial_layout;
  uwvm_vk_extent3d extent;
  uint32_t queue_family_index_count;
  uint32_t reserved0;
  uint64_t queue_family_indices_address;
} uwvm_vk_image_create_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_memory_requirements {
  uint64_t size;
  uint64_t alignment;
  uint32_t memory_type_bits;
  uint32_t reserved;
} uwvm_vk_memory_requirements;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_memory_allocate_info {
  uint64_t allocation_size;
  uint32_t memory_type_index;
  uint32_t reserved;
} uwvm_vk_memory_allocate_info;

typedef struct UWVM2_VULKAN_PACKED uwvm_vk_memory_copy_region {
  uint32_t guest_memory_index;
  uint32_t reserved;
  uint64_t guest_address;
  uint64_t device_offset;
  uint64_t size;
} uwvm_vk_memory_copy_region;

UWVM2_VULKAN_EXTERN_C_END

#endif
