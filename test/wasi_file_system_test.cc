#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "runtime/plugin_context.h"

namespace {

using uwvm2_vulkan::runtime::PluginContext;

struct WasiState {
  uwvm_wasi_fd_t fd_close_fd{};
  uwvm_wasi_fd_t fd_read_fd{};
  uwvm_wasi_void_ptr_t fd_read_iovs{};
  uwvm_wasi_size_t fd_read_iovs_len{};
  uwvm_wasi_void_ptr_t fd_read_nread{};
  uwvm_wasi_fd_t fd_seek_fd{};
  uwvm_wasi_filedelta_t fd_seek_offset{};
  uwvm_wasi_whence_t fd_seek_whence{};
  uwvm_wasi_void_ptr_t fd_seek_new_offset{};
  uwvm_wasi_fd_t fd_prestat_get_fd{};
  uwvm_wasi_void_ptr_t fd_prestat_get_buffer{};
  uwvm_wasi_fd_t fd_prestat_dir_name_fd{};
  uwvm_wasi_void_ptr_t fd_prestat_dir_name_path{};
  uwvm_wasi_size_t fd_prestat_dir_name_path_len{};
  uwvm_wasi_fd_t path_open_dirfd{};
  uwvm_wasi_lookupflags_t path_open_dirflags{};
  uwvm_wasi_void_ptr_t path_open_path{};
  uwvm_wasi_size_t path_open_path_len{};
  uwvm_wasi_oflags_t path_open_oflags{};
  uwvm_wasi_rights_t path_open_rights_base{};
  uwvm_wasi_rights_t path_open_rights_inheriting{};
  uwvm_wasi_fdflags_t path_open_fdflags{};
  uwvm_wasi_void_ptr_t path_open_fd_ptr{};
  uwvm_wasi_fd_t path_filestat_get_fd{};
  uwvm_wasi_lookupflags_t path_filestat_get_flags{};
  uwvm_wasi_void_ptr_t path_filestat_get_path{};
  uwvm_wasi_size_t path_filestat_get_path_len{};
  uwvm_wasi_void_ptr_t path_filestat_get_stat{};
  std::uint32_t fd_close_calls{};
  std::uint32_t fd_read_calls{};
  std::uint32_t fd_seek_calls{};
  std::uint32_t fd_prestat_get_calls{};
  std::uint32_t fd_prestat_dir_name_calls{};
  std::uint32_t path_open_calls{};
  std::uint32_t path_filestat_get_calls{};
};

WasiState g_state{};

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

uwvm_wasi_errno_t FakeFdClose(uwvm_wasi_fd_t fd) noexcept {
  g_state.fd_close_fd = fd;
  ++g_state.fd_close_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakeFdRead(uwvm_wasi_fd_t fd, uwvm_wasi_void_ptr_t iovs,
                             uwvm_wasi_size_t iovs_len,
                             uwvm_wasi_void_ptr_t nread) noexcept {
  g_state.fd_read_fd = fd;
  g_state.fd_read_iovs = iovs;
  g_state.fd_read_iovs_len = iovs_len;
  g_state.fd_read_nread = nread;
  ++g_state.fd_read_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakeFdSeek(uwvm_wasi_fd_t fd, uwvm_wasi_filedelta_t offset,
                             uwvm_wasi_whence_t whence,
                             uwvm_wasi_void_ptr_t new_offset_ptrsz) noexcept {
  g_state.fd_seek_fd = fd;
  g_state.fd_seek_offset = offset;
  g_state.fd_seek_whence = whence;
  g_state.fd_seek_new_offset = new_offset_ptrsz;
  ++g_state.fd_seek_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakeFdPrestatGet(uwvm_wasi_fd_t fd,
                                   uwvm_wasi_void_ptr_t buf_ptrsz) noexcept {
  g_state.fd_prestat_get_fd = fd;
  g_state.fd_prestat_get_buffer = buf_ptrsz;
  ++g_state.fd_prestat_get_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakeFdPrestatDirName(uwvm_wasi_fd_t fd,
                                       uwvm_wasi_void_ptr_t path_ptrsz,
                                       uwvm_wasi_size_t path_len) noexcept {
  g_state.fd_prestat_dir_name_fd = fd;
  g_state.fd_prestat_dir_name_path = path_ptrsz;
  g_state.fd_prestat_dir_name_path_len = path_len;
  ++g_state.fd_prestat_dir_name_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakePathOpen(uwvm_wasi_fd_t dirfd,
                               uwvm_wasi_lookupflags_t dirflags,
                               uwvm_wasi_void_ptr_t path_ptrsz,
                               uwvm_wasi_size_t path_len,
                               uwvm_wasi_oflags_t oflags,
                               uwvm_wasi_rights_t fs_rights_base,
                               uwvm_wasi_rights_t fs_rights_inheriting,
                               uwvm_wasi_fdflags_t fdflags,
                               uwvm_wasi_void_ptr_t fd_ptrsz) noexcept {
  g_state.path_open_dirfd = dirfd;
  g_state.path_open_dirflags = dirflags;
  g_state.path_open_path = path_ptrsz;
  g_state.path_open_path_len = path_len;
  g_state.path_open_oflags = oflags;
  g_state.path_open_rights_base = fs_rights_base;
  g_state.path_open_rights_inheriting = fs_rights_inheriting;
  g_state.path_open_fdflags = fdflags;
  g_state.path_open_fd_ptr = fd_ptrsz;
  ++g_state.path_open_calls;
  return UWVM_WASI_ESUCCESS;
}

uwvm_wasi_errno_t FakePathFilestatGet(
    uwvm_wasi_fd_t fd, uwvm_wasi_lookupflags_t flags,
    uwvm_wasi_void_ptr_t path_ptrsz, uwvm_wasi_size_t path_len,
    uwvm_wasi_void_ptr_t buf_ptrsz) noexcept {
  g_state.path_filestat_get_fd = fd;
  g_state.path_filestat_get_flags = flags;
  g_state.path_filestat_get_path = path_ptrsz;
  g_state.path_filestat_get_path_len = path_len;
  g_state.path_filestat_get_stat = buf_ptrsz;
  ++g_state.path_filestat_get_calls;
  return UWVM_WASI_ESUCCESS;
}

} // namespace

int main() {
  auto &context{PluginContext::Instance()};
  context.SetExplicitWasiHostApi(nullptr);

  auto &empty_filesystem{context.wasi_filesystem()};
  require(!empty_filesystem.Available());

  uwvm_wasi_errno_t errno_code{UWVM_WASI_ESUCCESS};
  require(!empty_filesystem.PathOpen(
      3, UWVM_WASI_LOOKUP_SYMLINK_FOLLOW, 64u, 5u, UWVM_WASI_O_CREAT,
      UWVM_WASI_RIGHT_PATH_OPEN, UWVM_WASI_RIGHT_FD_READ,
      UWVM_WASI_FDFLAG_APPEND, 96u, errno_code));
  require(errno_code == UWVM_WASI_ENOSYS);

  uwvm_wasip1_host_api_v1 host_api{};
  host_api.struct_size = sizeof(host_api);
  host_api.abi_version = UWVM_WASIP1_HOST_API_V1_ABI_VERSION;
  host_api.fd_close = FakeFdClose;
  host_api.fd_read = FakeFdRead;
  host_api.fd_seek = FakeFdSeek;
  host_api.fd_prestat_get = FakeFdPrestatGet;
  host_api.fd_prestat_dir_name = FakeFdPrestatDirName;
  host_api.path_open = FakePathOpen;
  host_api.path_filestat_get = FakePathFilestatGet;
  context.SetExplicitWasiHostApi(&host_api);

  auto &filesystem{context.wasi_filesystem()};
  require(filesystem.Available());
  require(filesystem.SupportsFdClose());
  require(filesystem.SupportsFdRead());
  require(filesystem.SupportsFdSeek());
  require(filesystem.SupportsFdPrestatGet());
  require(filesystem.SupportsFdPrestatDirName());
  require(filesystem.SupportsPathOpen());
  require(filesystem.SupportsPathFilestatGet());

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.FdClose(17, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.fd_close_calls == 1u);
  require(g_state.fd_close_fd == 17);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.FdRead(19, 128u, 2u, 160u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.fd_read_calls == 1u);
  require(g_state.fd_read_fd == 19);
  require(g_state.fd_read_iovs == 128u);
  require(g_state.fd_read_iovs_len == 2u);
  require(g_state.fd_read_nread == 160u);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.FdSeek(23, 4096, UWVM_WASI_WHENCE_END, 192u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.fd_seek_calls == 1u);
  require(g_state.fd_seek_fd == 23);
  require(g_state.fd_seek_offset == 4096);
  require(g_state.fd_seek_whence == UWVM_WASI_WHENCE_END);
  require(g_state.fd_seek_new_offset == 192u);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.FdPrestatGet(3, 224u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.fd_prestat_get_calls == 1u);
  require(g_state.fd_prestat_get_fd == 3);
  require(g_state.fd_prestat_get_buffer == 224u);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.FdPrestatDirName(3, 256u, 12u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.fd_prestat_dir_name_calls == 1u);
  require(g_state.fd_prestat_dir_name_fd == 3);
  require(g_state.fd_prestat_dir_name_path == 256u);
  require(g_state.fd_prestat_dir_name_path_len == 12u);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.PathOpen(
      3, UWVM_WASI_LOOKUP_SYMLINK_FOLLOW, 288u, 7u, UWVM_WASI_O_CREAT,
      UWVM_WASI_RIGHT_PATH_OPEN | UWVM_WASI_RIGHT_FD_READ,
      UWVM_WASI_RIGHT_FD_READ, UWVM_WASI_FDFLAG_APPEND, 320u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.path_open_calls == 1u);
  require(g_state.path_open_dirfd == 3);
  require(g_state.path_open_dirflags == UWVM_WASI_LOOKUP_SYMLINK_FOLLOW);
  require(g_state.path_open_path == 288u);
  require(g_state.path_open_path_len == 7u);
  require(g_state.path_open_oflags == UWVM_WASI_O_CREAT);
  require(g_state.path_open_rights_base ==
          (UWVM_WASI_RIGHT_PATH_OPEN | UWVM_WASI_RIGHT_FD_READ));
  require(g_state.path_open_rights_inheriting == UWVM_WASI_RIGHT_FD_READ);
  require(g_state.path_open_fdflags == UWVM_WASI_FDFLAG_APPEND);
  require(g_state.path_open_fd_ptr == 320u);

  errno_code = UWVM_WASI_ENOSYS;
  require(filesystem.PathFilestatGet(3, 0u, 352u, 11u, 384u, errno_code));
  require(errno_code == UWVM_WASI_ESUCCESS);
  require(g_state.path_filestat_get_calls == 1u);
  require(g_state.path_filestat_get_fd == 3);
  require(g_state.path_filestat_get_flags == 0u);
  require(g_state.path_filestat_get_path == 352u);
  require(g_state.path_filestat_get_path_len == 11u);
  require(g_state.path_filestat_get_stat == 384u);

  auto truncated_api{host_api};
  truncated_api.struct_size = offsetof(uwvm_wasip1_host_api_v1, path_open);
  context.SetExplicitWasiHostApi(&truncated_api);

  auto &truncated_filesystem{context.wasi_filesystem()};
  require(truncated_filesystem.Available());
  require(!truncated_filesystem.SupportsPathOpen());
  errno_code = UWVM_WASI_ESUCCESS;
  require(!truncated_filesystem.PathOpen(
      3, 0u, 1u, 1u, 0u, UWVM_WASI_RIGHT_PATH_OPEN, 0u, 0u, 2u, errno_code));
  require(errno_code == UWVM_WASI_ENOSYS);

  auto invalid_api{host_api};
  invalid_api.abi_version = 2u;
  context.SetExplicitWasiHostApi(&invalid_api);

  auto &invalid_filesystem{context.wasi_filesystem()};
  require(!invalid_filesystem.Available());
  require(!invalid_filesystem.SupportsFdClose());
  errno_code = UWVM_WASI_ESUCCESS;
  require(!invalid_filesystem.FdClose(7, errno_code));
  require(errno_code == UWVM_WASI_ENOSYS);

  context.SetExplicitWasiHostApi(nullptr);
  return 0;
}
