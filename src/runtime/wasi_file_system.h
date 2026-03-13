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

#ifndef UWVM2_VULKAN_SRC_RUNTIME_WASI_FILE_SYSTEM_H_
#define UWVM2_VULKAN_SRC_RUNTIME_WASI_FILE_SYSTEM_H_

#include <cstddef>

#include "abi/uwvm_abi.h"

namespace uwvm2_vulkan::runtime {

class WasiFileSystem {
public:
  void SetHostApi(uwvm_wasip1_host_api_v1 const *host_api) noexcept {
    host_api_ = host_api;
  }

  [[nodiscard]] bool Available() const noexcept {
    return host_api_ != nullptr &&
           host_api_->abi_version == UWVM_WASIP1_HOST_API_V1_ABI_VERSION &&
           host_api_->struct_size >=
               offsetof(uwvm_wasip1_host_api_v1, args_sizes_get) +
                   sizeof(host_api_->args_sizes_get);
  }

  [[nodiscard]] bool SupportsFdClose() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_close),
                    sizeof(host_api_->fd_close)) &&
           host_api_->fd_close != nullptr;
  }

  [[nodiscard]] bool SupportsFdRead() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_read),
                    sizeof(host_api_->fd_read)) &&
           host_api_->fd_read != nullptr;
  }

  [[nodiscard]] bool SupportsFdPRead() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_pread),
                    sizeof(host_api_->fd_pread)) &&
           host_api_->fd_pread != nullptr;
  }

  [[nodiscard]] bool SupportsFdSeek() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_seek),
                    sizeof(host_api_->fd_seek)) &&
           host_api_->fd_seek != nullptr;
  }

  [[nodiscard]] bool SupportsFdWrite() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_write),
                    sizeof(host_api_->fd_write)) &&
           host_api_->fd_write != nullptr;
  }

  [[nodiscard]] bool SupportsFdPrestatGet() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_prestat_get),
                    sizeof(host_api_->fd_prestat_get)) &&
           host_api_->fd_prestat_get != nullptr;
  }

  [[nodiscard]] bool SupportsFdPrestatDirName() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, fd_prestat_dir_name),
                    sizeof(host_api_->fd_prestat_dir_name)) &&
           host_api_->fd_prestat_dir_name != nullptr;
  }

  [[nodiscard]] bool SupportsPathOpen() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, path_open),
                    sizeof(host_api_->path_open)) &&
           host_api_->path_open != nullptr;
  }

  [[nodiscard]] bool SupportsPathFilestatGet() const noexcept {
    return HasField(offsetof(uwvm_wasip1_host_api_v1, path_filestat_get),
                    sizeof(host_api_->path_filestat_get)) &&
           host_api_->path_filestat_get != nullptr;
  }

  [[nodiscard]] bool FdClose(uwvm_wasi_fd_t fd,
                             uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdClose()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_close(fd);
    return true;
  }

  [[nodiscard]] bool FdRead(uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs,
                            uwvm_wasi_size_t iovs_len,
                            uwvm_wasi_void_ptr_t nread,
                            uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdRead()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_read(fd, iovs, iovs_len, nread);
    return true;
  }

  [[nodiscard]] bool
  FdPRead(uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs,
          uwvm_wasi_size_t iovs_len, uwvm_wasi_filesize_t offset,
          uwvm_wasi_void_ptr_t nread,
          uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdPRead()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_pread(fd, iovs, iovs_len, offset, nread);
    return true;
  }

  [[nodiscard]] bool FdSeek(uwvm_wasi_fd_t fd, uwvm_wasi_filedelta_t offset,
                            uwvm_wasi_whence_t whence,
                            uwvm_wasi_void_ptr_t new_offset_ptrsz,
                            uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdSeek()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_seek(fd, offset, whence, new_offset_ptrsz);
    return true;
  }

  [[nodiscard]] bool FdWrite(uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs,
                             uwvm_wasi_size_t iovs_len,
                             uwvm_wasi_void_ptr_t nwritten,
                             uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdWrite()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_write(fd, iovs, iovs_len, nwritten);
    return true;
  }

  [[nodiscard]] bool FdPrestatGet(
      uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t prestat_ptrsz,
      uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdPrestatGet()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_prestat_get(fd, prestat_ptrsz);
    return true;
  }

  [[nodiscard]] bool FdPrestatDirName(
      uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t path_ptrsz,
      uwvm_wasi_size_t path_len,
      uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsFdPrestatDirName()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->fd_prestat_dir_name(fd, path_ptrsz, path_len);
    return true;
  }

  [[nodiscard]] bool PathOpen(uwvm_wasi_fd_t dirfd,
                              uwvm_wasi_lookupflags_t dirflags,
                              uwvm_wasi_void_ptr_t path_ptrsz,
                              uwvm_wasi_size_t path_len,
                              uwvm_wasi_oflags_t oflags,
                              uwvm_wasi_rights_t fs_rights_base,
                              uwvm_wasi_rights_t fs_rights_inheriting,
                              uwvm_wasi_fdflags_t fdflags,
                              uwvm_wasi_void_ptr_t fd_ptrsz,
                              uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsPathOpen()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code =
        host_api_->path_open(dirfd, dirflags, path_ptrsz, path_len, oflags,
                             fs_rights_base, fs_rights_inheriting, fdflags,
                             fd_ptrsz);
    return true;
  }

  [[nodiscard]] bool PathFilestatGet(
      uwvm_wasi_fd_t fd, uwvm_wasi_lookupflags_t flags,
      uwvm_wasi_void_ptr_t path_ptrsz, uwvm_wasi_size_t path_len,
      uwvm_wasi_void_ptr_t stat_ptrsz,
      uwvm_wasi_errno_t &errno_code) const noexcept {
    if (!SupportsPathFilestatGet()) {
      errno_code = UWVM_WASI_ENOSYS;
      return false;
    }
    errno_code = host_api_->path_filestat_get(fd, flags, path_ptrsz, path_len,
                                              stat_ptrsz);
    return true;
  }

private:
  [[nodiscard]] bool HasField(std::size_t offset,
                              std::size_t field_size) const noexcept {
    return Available() && host_api_->struct_size >= offset + field_size;
  }

  uwvm_wasip1_host_api_v1 const *host_api_{};
};

} // namespace uwvm2_vulkan::runtime

#endif
