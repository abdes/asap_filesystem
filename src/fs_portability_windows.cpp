//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "fs_error.h"
#include "fs_portability.h"

#if defined(ASAP_WINDOWS)

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
#endif  // ASAP_WINDOWS

namespace asap {
namespace filesystem {
namespace detail {

// -----------------------------------------------------------------------------
//                          detail: file time utils
// -----------------------------------------------------------------------------

#if defined(ASAP_WINDOWS)
namespace win32 {

file_time_type ft_convert_from_filetime(const FILETIME &ft) {
  // Contains a 64-bit value representing the number of 100-nanosecond intervals
  // since January 1, 1601 (UTC).
  constexpr auto EPOCH_DIFF = 11644473600;
  ULARGE_INTEGER ulft;
  ulft.HighPart = ft.dwHighDateTime;
  ulft.LowPart = ft.dwLowDateTime;

  // Convert to EPOCH
  using namespace std::chrono;
  using rep = typename file_time_type::rep;
  using fs_seconds = duration<rep>;
  return file_time_type(fs_seconds(ulft.QuadPart / 10000000 - EPOCH_DIFF));
}

// -----------------------------------------------------------------------------
//                           detail: windows status
// -----------------------------------------------------------------------------

bool not_found_error(int errval) {
  return errval == ERROR_FILE_NOT_FOUND || errval == ERROR_PATH_NOT_FOUND ||
         errval == ERROR_INVALID_NAME  // "tools/jam/src/:sys:stat.h", "//foo"
         ||
         errval == ERROR_INVALID_DRIVE  // USB card reader with no card inserted
         || errval == ERROR_NOT_READY   // CD/DVD drive with no disc inserted
         || errval == ERROR_INVALID_PARAMETER  // ":sys:stat.h"
         || errval == ERROR_BAD_PATHNAME       // "//nosuch" on Win64
         || errval == ERROR_BAD_NETPATH;       // "//nosuch" on Win32
}

bool is_reparse_point_a_symlink(const path &p, std::error_code *ec) {
  ErrorHandler<bool> err("is_reparse_point_a_symlink", ec, &p);

  std::error_code m_ec;
  auto file = detail::FileDescriptor::create(
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

path read_reparse_point_symlink(const path &p, std::error_code *ec) {
  ErrorHandler<path> err("read_symlink", ec, &p);

  // Open a handle to the symbolic link and read the reparse point data
  // to obtain the symbolic link target
  std::error_code m_ec;
  auto file = detail::FileDescriptor::create(
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

file_status process_status_failure(std::error_code m_ec, const path &p,
                                   std::error_code *ec) {
  if (ec) *ec = m_ec;
  if (m_ec) {
    if (not_found_error(m_ec.value())) {
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

namespace {
bool equal_extension(wchar_t const *p, wchar_t const (&x1)[5],
                     wchar_t const (&x2)[5]) {
  return (p[0] == x1[0] || p[0] == x2[0]) && (p[1] == x1[1] || p[1] == x2[1]) &&
         (p[2] == x1[2] || p[2] == x2[2]) && (p[3] == x1[3] || p[3] == x2[3]) &&
         p[4] == 0;
}
}  // namespace

perms make_permissions(const path &p, DWORD attr) {
  // TODO: See if we can get the permissions in a better way
  perms prms = perms::owner_read | perms::group_read | perms::others_read;
  if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
    prms |= perms::owner_write | perms::group_write | perms::others_write;
  auto wext = p.extension().wstring();
  wchar_t const *q = wext.c_str();
  if (equal_extension(q, L".exe", L".EXE") ||
      equal_extension(q, L".com", L".COM") ||
      equal_extension(q, L".bat", L".BAT") ||
      equal_extension(q, L".cmd", L".CMD"))
    prms |= perms::owner_exec | perms::group_exec | perms::others_exec;
  return prms;
}

}  // namespace win32
#endif  // ASAP_WINDOWS

// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

#if defined(ASAP_WINDOWS)

const FileDescriptor::fd_type FileDescriptor::invalid_value =
    win32::invalid_handle_value;

file_status FileDescriptor::refresh_status(std::error_code &ec) {
  // FD must be open and good.
  status_ = file_status{};

  // TODO: implement status refresh for windows

  return status_;
}

#endif  // ASAP_WINDOWS

}  // namespace detail
}  // namespace filesystem
}  // namespace asap
