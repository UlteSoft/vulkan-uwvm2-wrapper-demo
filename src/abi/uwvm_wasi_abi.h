/*************************************************************
 * UWVM2 Vulkan Wrapper                                     *
 * Copyright (c) 2026-present MacroModel. All rights        *
 * reserved.                                                *
 *************************************************************/

/**
 * @author      MacroModel
 * @version     0.1.0
 * @date        2026-03-13
 */

/****************************************
 *  _   _ __        ____     __ __  __  *
 * | | | |\ \      / /\ \   / /|  \/  | *
 * | | | | \ \ /\ / /  \ \ / / | |\/| | *
 * | |_| |  \ V  V /    \ V /  | |  | | *
 *  \___/    \_/\_/      \_/   |_|  |_| *
 *                                      *
 ****************************************/

#ifndef UWVM2_VULKAN_SRC_ABI_UWVM_WASI_ABI_H_
#define UWVM2_VULKAN_SRC_ABI_UWVM_WASI_ABI_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { UWVM_WASIP1_HOST_API_V1_ABI_VERSION = 1u };

typedef uint32_t uwvm_wasi_size_t;
typedef uint32_t uwvm_wasi_void_ptr_t;
typedef int32_t uwvm_wasi_fd_t;
typedef uint16_t uwvm_wasi_errno_t;
typedef uint8_t uwvm_wasi_advice_t;
typedef uint32_t uwvm_wasi_clockid_t;
typedef uint64_t uwvm_wasi_timestamp_t;
typedef uint64_t uwvm_wasi_filesize_t;
typedef int64_t uwvm_wasi_filedelta_t;
typedef uint8_t uwvm_wasi_filetype_t;
typedef uint16_t uwvm_wasi_fdflags_t;
typedef uint16_t uwvm_wasi_fstflags_t;
typedef uint32_t uwvm_wasi_lookupflags_t;
typedef uint16_t uwvm_wasi_oflags_t;
typedef uint64_t uwvm_wasi_rights_t;
typedef uint64_t uwvm_wasi_dircookie_t;
typedef uint8_t uwvm_wasi_whence_t;

enum {
  UWVM_WASI_ESUCCESS = 0u,
  UWVM_WASI_EBADF = 8u,
  UWVM_WASI_EINVAL = 28u,
  UWVM_WASI_ENOENT = 44u,
  UWVM_WASI_ENOSYS = 52u,
  UWVM_WASI_ENOTDIR = 54u,
  UWVM_WASI_ENOTCAPABLE = 76u
};

enum {
  UWVM_WASI_FDFLAG_APPEND = 0x0001u,
  UWVM_WASI_FDFLAG_DSYNC = 0x0002u,
  UWVM_WASI_FDFLAG_NONBLOCK = 0x0004u,
  UWVM_WASI_FDFLAG_RSYNC = 0x0008u,
  UWVM_WASI_FDFLAG_SYNC = 0x0010u
};

enum {
  UWVM_WASI_LOOKUP_SYMLINK_FOLLOW = 0x00000001u
};

enum {
  UWVM_WASI_O_CREAT = 0x0001u,
  UWVM_WASI_O_DIRECTORY = 0x0002u,
  UWVM_WASI_O_EXCL = 0x0004u,
  UWVM_WASI_O_TRUNC = 0x0008u
};

enum {
  UWVM_WASI_WHENCE_SET = 0u,
  UWVM_WASI_WHENCE_CUR = 1u,
  UWVM_WASI_WHENCE_END = 2u
};

enum {
  UWVM_WASI_RIGHT_FD_DATASYNC = UINT64_C(0x0000000000000001),
  UWVM_WASI_RIGHT_FD_READ = UINT64_C(0x0000000000000002),
  UWVM_WASI_RIGHT_FD_SEEK = UINT64_C(0x0000000000000004),
  UWVM_WASI_RIGHT_FD_FDSTAT_SET_FLAGS = UINT64_C(0x0000000000000008),
  UWVM_WASI_RIGHT_FD_SYNC = UINT64_C(0x0000000000000010),
  UWVM_WASI_RIGHT_FD_TELL = UINT64_C(0x0000000000000020),
  UWVM_WASI_RIGHT_FD_WRITE = UINT64_C(0x0000000000000040),
  UWVM_WASI_RIGHT_FD_ADVISE = UINT64_C(0x0000000000000080),
  UWVM_WASI_RIGHT_FD_ALLOCATE = UINT64_C(0x0000000000000100),
  UWVM_WASI_RIGHT_PATH_CREATE_DIRECTORY = UINT64_C(0x0000000000000200),
  UWVM_WASI_RIGHT_PATH_CREATE_FILE = UINT64_C(0x0000000000000400),
  UWVM_WASI_RIGHT_PATH_LINK_SOURCE = UINT64_C(0x0000000000000800),
  UWVM_WASI_RIGHT_PATH_LINK_TARGET = UINT64_C(0x0000000000001000),
  UWVM_WASI_RIGHT_PATH_OPEN = UINT64_C(0x0000000000002000),
  UWVM_WASI_RIGHT_FD_READDIR = UINT64_C(0x0000000000004000),
  UWVM_WASI_RIGHT_PATH_READLINK = UINT64_C(0x0000000000008000),
  UWVM_WASI_RIGHT_PATH_RENAME_SOURCE = UINT64_C(0x0000000000010000),
  UWVM_WASI_RIGHT_PATH_RENAME_TARGET = UINT64_C(0x0000000000020000),
  UWVM_WASI_RIGHT_PATH_FILESTAT_GET = UINT64_C(0x0000000000040000),
  UWVM_WASI_RIGHT_PATH_FILESTAT_SET_SIZE = UINT64_C(0x0000000000080000),
  UWVM_WASI_RIGHT_PATH_FILESTAT_SET_TIMES = UINT64_C(0x0000000000100000),
  UWVM_WASI_RIGHT_FD_FILESTAT_GET = UINT64_C(0x0000000000200000),
  UWVM_WASI_RIGHT_FD_FILESTAT_SET_SIZE = UINT64_C(0x0000000000400000),
  UWVM_WASI_RIGHT_FD_FILESTAT_SET_TIMES = UINT64_C(0x0000000000800000),
  UWVM_WASI_RIGHT_PATH_SYMLINK = UINT64_C(0x0000000001000000),
  UWVM_WASI_RIGHT_PATH_REMOVE_DIRECTORY = UINT64_C(0x0000000002000000),
  UWVM_WASI_RIGHT_PATH_UNLINK_FILE = UINT64_C(0x0000000004000000),
  UWVM_WASI_RIGHT_POLL_FD_READWRITE = UINT64_C(0x0000000008000000),
  UWVM_WASI_RIGHT_SOCK_SHUTDOWN = UINT64_C(0x0000000010000000),
  UWVM_WASI_RIGHT_SOCK_ACCEPT = UINT64_C(0x0000000020000000)
};

typedef struct uwvm_wasi_iovec_t {
  uwvm_wasi_void_ptr_t buf;
  uwvm_wasi_size_t buf_len;
} uwvm_wasi_iovec_t;

typedef uwvm_wasi_errno_t (*uwvm_wasip1_args_get_t)(
    uwvm_wasi_void_ptr_t argv_ptrsz, uwvm_wasi_void_ptr_t argv_buf_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_args_sizes_get_t)(
    uwvm_wasi_void_ptr_t argc_ptrsz,
    uwvm_wasi_void_ptr_t argv_buf_size_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_clock_res_get_t)(
    uwvm_wasi_clockid_t clock_id, uwvm_wasi_void_ptr_t resolution_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_clock_time_get_t)(
    uwvm_wasi_clockid_t clock_id, uwvm_wasi_timestamp_t precision,
    uwvm_wasi_void_ptr_t time_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_environ_get_t)(
    uwvm_wasi_void_ptr_t environ_ptrsz, uwvm_wasi_void_ptr_t environ_buf_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_environ_sizes_get_t)(
    uwvm_wasi_void_ptr_t environ_count_ptrsz,
    uwvm_wasi_void_ptr_t environ_buf_size_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_advise_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_filesize_t offset, uwvm_wasi_filesize_t len,
    uwvm_wasi_advice_t advice);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_allocate_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_filesize_t offset, uwvm_wasi_filesize_t len);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_close_t)(uwvm_wasi_fd_t fd);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_datasync_t)(uwvm_wasi_fd_t fd);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_fdstat_get_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t stat_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_fdstat_set_flags_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_fdflags_t flags);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_fdstat_set_rights_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_rights_t fs_rights_base,
    uwvm_wasi_rights_t fs_rights_inheriting);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_filestat_get_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t stat_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_filestat_set_size_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_filesize_t size);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_filestat_set_times_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_timestamp_t atim,
    uwvm_wasi_timestamp_t mtim, uwvm_wasi_fstflags_t fstflags);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_pread_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs, uwvm_wasi_size_t iovs_len,
    uwvm_wasi_filesize_t offset, uwvm_wasi_void_ptr_t nread);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_prestat_dir_name_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t path,
    uwvm_wasi_size_t path_len);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_prestat_get_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t buf_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_pwrite_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs, uwvm_wasi_size_t iovs_len,
    uwvm_wasi_filesize_t offset, uwvm_wasi_void_ptr_t nwritten);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_read_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs, uwvm_wasi_size_t iovs_len,
    uwvm_wasi_void_ptr_t nread);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_readdir_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t buf_ptrsz,
    uwvm_wasi_size_t buf_len, uwvm_wasi_dircookie_t cookie,
    uwvm_wasi_void_ptr_t buf_used_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_renumber_t)(
    uwvm_wasi_fd_t fd_from, uwvm_wasi_fd_t fd_to);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_seek_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_filedelta_t offset,
    uwvm_wasi_whence_t whence, uwvm_wasi_void_ptr_t new_offset_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_sync_t)(uwvm_wasi_fd_t fd);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_tell_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t tell_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_fd_write_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs, uwvm_wasi_size_t iovs_len,
    uwvm_wasi_void_ptr_t nwritten);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_path_create_directory_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t path_ptrsz,
    uwvm_wasi_size_t path_len);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_path_filestat_get_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_lookupflags_t flags,
    uwvm_wasi_void_ptr_t path_ptrsz, uwvm_wasi_size_t path_len,
    uwvm_wasi_void_ptr_t buf_ptrsz);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_path_filestat_set_times_t)(
    uwvm_wasi_fd_t fd, uwvm_wasi_lookupflags_t flags,
    uwvm_wasi_void_ptr_t path_ptrsz, uwvm_wasi_size_t path_len,
    uwvm_wasi_timestamp_t atim, uwvm_wasi_timestamp_t mtim,
    uwvm_wasi_fstflags_t fstflags);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_path_link_t)(
    uwvm_wasi_fd_t old_fd, uwvm_wasi_lookupflags_t old_flags,
    uwvm_wasi_void_ptr_t old_path_ptrsz, uwvm_wasi_size_t old_path_len,
    uwvm_wasi_fd_t new_fd, uwvm_wasi_void_ptr_t new_path_ptrsz,
    uwvm_wasi_size_t new_path_len);
typedef uwvm_wasi_errno_t (*uwvm_wasip1_path_open_t)(
    uwvm_wasi_fd_t dirfd, uwvm_wasi_lookupflags_t dirflags,
    uwvm_wasi_void_ptr_t path_ptrsz, uwvm_wasi_size_t path_len,
    uwvm_wasi_oflags_t oflags, uwvm_wasi_rights_t fs_rights_base,
    uwvm_wasi_rights_t fs_rights_inheriting, uwvm_wasi_fdflags_t fdflags,
    uwvm_wasi_void_ptr_t fd_ptrsz);

typedef struct uwvm_wasip1_host_api_v1 {
  size_t struct_size;
  uint_least32_t abi_version;
  uwvm_wasip1_args_get_t args_get;
  uwvm_wasip1_args_sizes_get_t args_sizes_get;
  uwvm_wasip1_clock_res_get_t clock_res_get;
  uwvm_wasip1_clock_time_get_t clock_time_get;
  uwvm_wasip1_environ_get_t environ_get;
  uwvm_wasip1_environ_sizes_get_t environ_sizes_get;
  uwvm_wasip1_fd_advise_t fd_advise;
  uwvm_wasip1_fd_allocate_t fd_allocate;
  uwvm_wasip1_fd_close_t fd_close;
  uwvm_wasip1_fd_datasync_t fd_datasync;
  uwvm_wasip1_fd_fdstat_get_t fd_fdstat_get;
  uwvm_wasip1_fd_fdstat_set_flags_t fd_fdstat_set_flags;
  uwvm_wasip1_fd_fdstat_set_rights_t fd_fdstat_set_rights;
  uwvm_wasip1_fd_filestat_get_t fd_filestat_get;
  uwvm_wasip1_fd_filestat_set_size_t fd_filestat_set_size;
  uwvm_wasip1_fd_filestat_set_times_t fd_filestat_set_times;
  uwvm_wasip1_fd_pread_t fd_pread;
  uwvm_wasip1_fd_prestat_dir_name_t fd_prestat_dir_name;
  uwvm_wasip1_fd_prestat_get_t fd_prestat_get;
  uwvm_wasip1_fd_pwrite_t fd_pwrite;
  uwvm_wasip1_fd_read_t fd_read;
  uwvm_wasip1_fd_readdir_t fd_readdir;
  uwvm_wasip1_fd_renumber_t fd_renumber;
  uwvm_wasip1_fd_seek_t fd_seek;
  uwvm_wasip1_fd_sync_t fd_sync;
  uwvm_wasip1_fd_tell_t fd_tell;
  uwvm_wasip1_fd_write_t fd_write;
  uwvm_wasip1_path_create_directory_t path_create_directory;
  uwvm_wasip1_path_filestat_get_t path_filestat_get;
  uwvm_wasip1_path_filestat_set_times_t path_filestat_set_times;
  uwvm_wasip1_path_link_t path_link;
  uwvm_wasip1_path_open_t path_open;
} uwvm_wasip1_host_api_v1;

typedef void (*uwvm_set_wasip1_host_api_v1_t)(
    uwvm_wasip1_host_api_v1 const *);

#ifdef __cplusplus
}
#endif

#endif
