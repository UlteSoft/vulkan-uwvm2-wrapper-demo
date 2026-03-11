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
#ifndef UWVM2_VULKAN_VULKAN_H_
#define UWVM2_VULKAN_VULKAN_H_

#include <stdint.h>

#include "attributes.h"
#include "vulkan_types.h"

UWVM2_VULKAN_EXTERN_C_BEGIN

int32_t __imported_vulkan_loader_available(void) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("loader_available");

int32_t __imported_vulkan_enumerate_instance_version(
    uint64_t out_api_version_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("enumerate_instance_version");

int32_t __imported_vulkan_enumerate_instance_extension_properties(
    uint64_t layer_name_address, uint64_t property_buffer_address,
    uint32_t property_capacity,
    uint64_t out_property_count_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("enumerate_instance_extension_properties");

int32_t __imported_vulkan_enumerate_instance_layer_properties(
    uint64_t property_buffer_address, uint32_t property_capacity,
    uint64_t out_property_count_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("enumerate_instance_layer_properties");

int32_t
__imported_vulkan_create_instance(uint64_t create_info_address,
                                  uint64_t out_instance_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("create_instance");

int32_t
__imported_vulkan_destroy_instance(uint64_t instance_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("destroy_instance");

int32_t __imported_vulkan_enumerate_physical_devices(
    uint64_t instance_handle, uint64_t out_device_buffer_address,
    uint32_t device_capacity, uint64_t out_device_count_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("enumerate_physical_devices");

int32_t __imported_vulkan_enumerate_device_extension_properties(
    uint64_t physical_device_handle, uint64_t layer_name_address,
    uint64_t property_buffer_address, uint32_t property_capacity,
    uint64_t out_property_count_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("enumerate_device_extension_properties");

int32_t __imported_vulkan_get_physical_device_properties(
    uint64_t physical_device_handle,
    uint64_t out_properties_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_physical_device_properties");

int32_t __imported_vulkan_get_physical_device_features(
    uint64_t physical_device_handle,
    uint64_t out_features_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_physical_device_features");

int32_t __imported_vulkan_get_physical_device_memory_properties(
    uint64_t physical_device_handle,
    uint64_t out_properties_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_physical_device_memory_properties");

int32_t __imported_vulkan_get_physical_device_queue_family_properties(
    uint64_t physical_device_handle, uint64_t out_property_buffer_address,
    uint32_t property_capacity,
    uint64_t out_property_count_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_physical_device_queue_family_properties");

int32_t
__imported_vulkan_create_device(uint64_t physical_device_handle,
                                uint64_t create_info_address,
                                uint64_t out_device_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("create_device");

int32_t __imported_vulkan_destroy_device(uint64_t device_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("destroy_device");

int32_t __imported_vulkan_device_wait_idle(
    uint64_t device_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("device_wait_idle");

int32_t __imported_vulkan_get_device_queue(
    uint64_t device_handle, uint32_t queue_family_index, uint32_t queue_index,
    uint64_t out_queue_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("get_device_queue");

int32_t __imported_vulkan_queue_wait_idle(
    uint64_t queue_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("queue_wait_idle");

int32_t
__imported_vulkan_create_buffer(uint64_t device_handle,
                                uint64_t create_info_address,
                                uint64_t out_buffer_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("create_buffer");

int32_t __imported_vulkan_destroy_buffer(uint64_t device_handle,
                                         uint64_t buffer_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("destroy_buffer");

int32_t __imported_vulkan_get_buffer_memory_requirements(
    uint64_t device_handle, uint64_t buffer_handle,
    uint64_t out_requirements_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_buffer_memory_requirements");

int32_t __imported_vulkan_create_image(
    uint64_t device_handle, uint64_t create_info_address,
    uint64_t out_image_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("create_image");

int32_t __imported_vulkan_destroy_image(uint64_t device_handle,
                                        uint64_t image_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("destroy_image");

int32_t __imported_vulkan_get_image_memory_requirements(
    uint64_t device_handle, uint64_t image_handle,
    uint64_t out_requirements_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("get_image_memory_requirements");

int32_t
__imported_vulkan_allocate_memory(uint64_t device_handle,
                                  uint64_t allocate_info_address,
                                  uint64_t out_memory_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("allocate_memory");

int32_t __imported_vulkan_free_memory(uint64_t device_handle,
                                      uint64_t memory_handle) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan") IMPORT_NAME("free_memory");

int32_t __imported_vulkan_bind_buffer_memory(uint64_t device_handle,
                                             uint64_t buffer_handle,
                                             uint64_t memory_handle,
                                             uint64_t offset) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("bind_buffer_memory");

int32_t __imported_vulkan_bind_image_memory(uint64_t device_handle,
                                            uint64_t image_handle,
                                            uint64_t memory_handle,
                                            uint64_t offset) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("bind_image_memory");

int32_t __imported_vulkan_copy_guest_to_device_memory(
    uint64_t device_handle, uint64_t memory_handle,
    uint64_t copy_region_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("copy_guest_to_device_memory");

int32_t __imported_vulkan_copy_device_memory_to_guest(
    uint64_t device_handle, uint64_t memory_handle,
    uint64_t copy_region_address) __WASI_NOEXCEPT
    UWVM2_VULKAN_IMPORT_MODULE("wasiu-vulkan")
        IMPORT_NAME("copy_device_memory_to_guest");

UWVM2_VULKAN_EXTERN_C_END

#endif
