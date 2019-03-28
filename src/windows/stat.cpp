//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_error.h"
#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: stat
// -----------------------------------------------------------------------------

#if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)
typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
      /*  Example of distinction between substitute and print names:
      mklink /d ldrive c:\
      SubstituteName: c:\\??\
      PrintName: c:\
      */
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE \
  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif

#if !defined(MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
#endif

#if !defined(FSCTL_GET_REPARSE_POINT)
#define FSCTL_GET_REPARSE_POINT 0x900a8
#endif

#if !defined(IO_REPARSE_TAG_SYMLINK)
#define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#endif

namespace asap {
namespace filesystem {
namespace detail {
namespace win32 {

bool IsNotFoundError(int errval) {
  return errval == ERROR_FILE_NOT_FOUND || errval == ERROR_PATH_NOT_FOUND ||
         errval == ERROR_INVALID_NAME  // "tools/jam/src/:sys:stat.h", "//foo"
         ||
         errval == ERROR_INVALID_DRIVE  // USB card reader with no card inserted
         || errval == ERROR_NOT_READY   // CD/DVD drive with no disc inserted
         || errval == ERROR_INVALID_PARAMETER  // ":sys:stat.h"
         || errval == ERROR_BAD_PATHNAME       // "//nosuch" on Win64
         || errval == ERROR_BAD_NETPATH;       // "//nosuch" on Win32
}

bool IsReparsePointSymlink(const path &p, std::error_code *ec) {
  ErrorHandler<bool> err("IsReparsePointSymlink", ec, &p);

  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, FILE_READ_EA,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      nullptr);
  if (m_ec) return err.report(m_ec);

  union info_t {
    char
        buf[REPARSE_DATA_BUFFER_HEADER_SIZE + MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    REPARSE_DATA_BUFFER rdb;
  } info;

  // Query the reparse data
  DWORD dwRetLen;
  BOOL result = detail::win32::DeviceIoControl(
      file.fd_, FSCTL_GET_REPARSE_POINT, nullptr, 0, info.buf,
      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRetLen, nullptr);
  if (!result) return err.report(capture_errno());

  return info.rdb.ReparseTag == IO_REPARSE_TAG_SYMLINK
         // Directory junctions are very similar to symlinks, but have some
         // performance and other advantages over symlinks. They can be created
         // from the command line with "mklink /j junction-name target-path".
         || info.rdb.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT;
}

path ReadSymlinkFromReparsePoint(const path &p, std::error_code *ec) {
  ErrorHandler<path> err("read_symlink", ec, &p);

  // Open a handle to the symbolic link and read the reparse point data
  // to obtain the symbolic link target
  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, FILE_READ_EA,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      nullptr);
  if (m_ec) return err.report(m_ec);

  union info_t {
    char
        buf[REPARSE_DATA_BUFFER_HEADER_SIZE + MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    REPARSE_DATA_BUFFER rdb;
  } info;

  // Query the reparse data
  DWORD dwRetLen;
  BOOL result = detail::win32::DeviceIoControl(
      file.fd_, FSCTL_GET_REPARSE_POINT, nullptr, 0, info.buf,
      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRetLen, nullptr);
  if (!result) return err.report(capture_errno());

  return path(
      static_cast<wchar_t *>(info.rdb.SymbolicLinkReparseBuffer.PathBuffer) +
          info.rdb.SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(wchar_t),
      static_cast<wchar_t *>(info.rdb.SymbolicLinkReparseBuffer.PathBuffer) +
          info.rdb.SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(wchar_t) +
          info.rdb.SymbolicLinkReparseBuffer.PrintNameLength / sizeof(wchar_t));
}

file_status ProcessStatusFailure(std::error_code m_ec, const path &p,
                                 std::error_code *ec = nullptr) {
  if (ec) *ec = m_ec;
  if (m_ec) {
    if (IsNotFoundError(m_ec.value())) {
      return file_status(file_type::not_found, perms::none);
    } else if (m_ec.value() == ERROR_SHARING_VIOLATION) {
      return file_status(file_type::unknown);
    } else {
      if (ec) {
        ErrorHandler<void> err("file_stat", ec, &p);
        err.report(m_ec,
                   "failed to determine attributes for the specified path");
      }
    }
  }
  return file_status(file_type::none);
}

}  // namespace win32
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
