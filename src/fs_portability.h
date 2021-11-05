//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <hedley/hedley.h>

// clang-format off
#include <filesystem/config.h>

#if defined(ASAP_POSIX)
# if !defined(ASAP_APPLE) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE ASAP_POSIX_LEVEL  // Request POSIX api
# endif
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/statvfs.h>
# include <dirent.h>
#endif

#include <ctime>    // for struct timespec

#include <climits>
#include <cstdlib>

#if defined(ASAP_FS_USE_UTIME)
# include <utime.h>
#endif

// Which method to use for copy file
#if defined(ASAP_FS_USE_SENDFILE)
# include <sys/sendfile.h>
#elif defined(ASAP_FS_USE_COPYFILE)
# include <copyfile.h>
#endif

#if defined(ASAP_WINDOWS)
# include <windows.h>
#include <aclapi.h>
#endif

#include <filesystem/filesystem.h>
#include "fs_error.h"
// clang-format on

namespace asap {
namespace filesystem {
namespace detail {

namespace linux_port {
#if defined(ASAP_FS_USE_SENDFILE)
using ::sendfile;
#endif
#if defined(ASAP_FS_USE_UTIMENSAT)
using ::utimensat;
#endif
}  // namespace linux_port

namespace apple_port {
#if defined(ASAP_FS_USE_COPYFILE)
using ::copyfile_state_alloc;
using ::copyfile_state_free;
using ::fcopyfile;
#endif
}  // namespace apple_port

#if defined(ASAP_POSIX)
namespace posix_port {
const int invalid_fd_value = -1;

using ::chdir;
using ::close;
using ::fchmod;
using ::fstat;
using ::ftruncate;
using ::getcwd;
using ::link;
using ::lstat;
using ::mkdir;
using ::open;
using ::pathconf;
using ::read;
using ::readdir;
using ::readlink;
using ::remove;
using ::stat;
using ::symlink;
using ::truncate;
#if defined(ASAP_FS_USE_UTIME)
using ::utime;
#endif
using ::write;
}  // namespace posix_port
#endif  // ASAP_POSIX

#if defined(ASAP_WINDOWS)
namespace win32_port {
typedef ULONG_PTR ulong_ptr;
typedef HANDLE handle;
const handle invalid_handle_value = (handle)((ulong_ptr)-1);

// This should be defined when (_WIN32_WINNT >= 0x0600) which is what we
// request from the compiler, but for some environments such as travis CI
// the build fails because of this symbol.
#if !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE (0x2)
#endif

using ::CloseHandle;
using ::CopyFileW;
using ::CreateDirectoryExW;
using ::CreateDirectoryW;
using ::CreateFileW;
using ::CreateHardLinkW;
using ::CreateSymbolicLinkW;
using ::DeleteFileW;
using ::DeviceIoControl;
using ::GetCurrentDirectoryW;
using ::GetDiskFreeSpaceExW;
using ::GetFileAttributesExW;
using ::GetFileAttributesW;
using ::GetFileInformationByHandle;
using ::GetFileInformationByHandleEx;
using ::GetFileTime;
using ::GetFileType;
using ::GetLastError;
using ::GetTempPathW;
using ::RemoveDirectoryW;
using ::SetCurrentDirectoryW;
using ::SetEndOfFile;
using ::SetFileAttributesW;
using ::SetFilePointerEx;
using ::SetFileTime;

}  // namespace win32_port
#endif  // ASAP_WINDOWS

// -----------------------------------------------------------------------------
//                          detail: file time utils
// -----------------------------------------------------------------------------

using TimeSpec = struct ::timespec;

#if defined(ASAP_POSIX)
namespace posix_port {

auto FileTimeTypeFromPosixTimeSpec(TimeSpec tm) -> file_time_type;

}  // namespace posix_port
#endif  // ASAP_POSIX

#if defined(ASAP_WINDOWS)
namespace win32_port {

file_time_type FileTimeTypeFromWindowsFileTime(const FILETIME &ft,
                                               std::error_code &ec);
FILETIME FileTimeTypeToWindowsFileTime(const file_time_type &ft,
                                       std::error_code &ec);

}  // namespace win32_port
#endif  // ASAP_WINDOWS

// -----------------------------------------------------------------------------
//                            detail: posix stat
// -----------------------------------------------------------------------------

#if defined(ASAP_POSIX)
namespace posix_port {

// Use typedef instead of using to avoid gcc warning on struct ::stat not
// declaring anything. (alternative is: "using StatT = struct ::stat;" )
#if defined(HEDLEY_GNUC_VERSION)
typedef struct ::stat StatT;
#else
using StatT = struct ::stat;
#endif

#if defined(ASAP_APPLE)
inline TimeSpec ExtractModificationTime(StatT const &st) {
  return st.st_mtimespec;
}
#else
inline auto ExtractModificationTime(StatT const &st) -> TimeSpec {
  return st.st_mtim;
}
#endif

#if defined(ASAP_FS_USE_UTIME)
#if defined(ASAP_APPLE)
inline TimeSpec ExtractAccessTime(StatT const &st) { return st.st_atimespec; }
#else
inline TimeSpec ExtractAccessTime(StatT const &st) { return st.st_atim; }
#endif
#endif  // ASAP_FS_USE_UTIME

auto CreateFileStatus(std::error_code &m_ec, path const &p,
                      const posix_port::StatT &path_stat, std::error_code *ec)
    -> file_status;

auto GetFileStatus(path const &p, posix_port::StatT &path_stat,
                   std::error_code *ec) -> file_status;

auto GetLinkStatus(path const &p, posix_port::StatT &path_stat,
                   std::error_code *ec) -> file_status;

auto ExtractLastWriteTime(const path &p, const StatT &st, std::error_code *ec)
    -> file_time_type;

}  // namespace posix_port
#endif  // ASAP_POSIX

// -----------------------------------------------------------------------------
//                           detail: windows stat
// -----------------------------------------------------------------------------

#if defined(ASAP_WINDOWS)
namespace win32_port {
bool IsNotFoundError(int errval);

bool IsReparsePointSymlink(const path &p, std::error_code *ec = nullptr);

path ReadSymlinkFromReparsePoint(const path &p, std::error_code *ec = nullptr);

file_status ProcessStatusFailure(std::error_code m_ec, const path &p,
                                 std::error_code *ec);

perms GetPermissions(const path &p, DWORD attr, bool follow_symlinks,
                     std::error_code *ec);
void SetPermissions(const path &p, perms prms, bool follow_symlinks,
                    std::error_code *ec);
}  // namespace win32_port
#endif  // ASAP_WINDOWS

// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

struct FileDescriptor {
  const path &name_;
#if defined(ASAP_WINDOWS)
  using fd_type = win32_port::handle;
#else
  using fd_type = int;
  posix_port::StatT stat_{};
#endif
  static const fd_type invalid_value;
  fd_type fd_{invalid_value};
  file_status status_;

  FileDescriptor(FileDescriptor &&other) noexcept
      : name_(other.name_),
#if defined(ASAP_POSIX)
        stat_(other.stat_),
#endif
        fd_(other.fd_),
        status_(other.status_) {
    other.fd_ = invalid_value;
    other.status_ = file_status{};
  }

  ~FileDescriptor() { Close(); }

  FileDescriptor() = delete;
  FileDescriptor(FileDescriptor const &) = delete;
  auto operator=(FileDescriptor const &) -> FileDescriptor & = delete;

  template <class... Args>
  static auto Create(const path *p, std::error_code &ec, Args... args)
      -> FileDescriptor {
    ec.clear();
    fd_type fd{invalid_value};
#if defined(ASAP_WINDOWS)
    auto wpath = p->wstring();
    fd = win32_port::CreateFileW(wpath.c_str(), args...);
#else
    fd = posix_port::open(p->c_str(), args...);
#endif
    if (fd == invalid_value) {
      ec = capture_errno();
      return FileDescriptor{p};
    }
    return FileDescriptor(p, fd);
  }

  auto Status() const -> file_status { return status_; }
#if defined(ASAP_POSIX)
  auto PosixStatus() const -> posix_port::StatT const & { return stat_; }
#endif

  auto RefreshStatus(bool follow_symlinks, std::error_code &ec) -> file_status;

 private:
  explicit FileDescriptor(const path *p,
                          fd_type fd = FileDescriptor::invalid_value)
      : name_(*p), fd_(fd) {}

  void Close() noexcept {
    if (fd_ != invalid_value) {
#if defined(ASAP_WINDOWS)
      win32_port::CloseHandle(fd_);
#else
      posix_port::close(fd_);
#endif
    }
    fd_ = invalid_value;
  }
};

}  // namespace detail
}  // namespace filesystem
}  // namespace asap
