//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/filesystem.h>
#include <hedley/hedley.h>

#include <stack>

#include "fs_error.h"
#include "fs_portability.h"

namespace asap {
namespace filesystem {

using detail::ErrorHandler;

// -----------------------------------------------------------------------------
//                           directory entry definitions
// -----------------------------------------------------------------------------

void directory_entry::UpdateBasicFileInformation(bool follow_symlinks,
                                                 std::error_code *ec) const {
  ErrorHandler<void> err("UpdateBasicFileInformation", ec, &path_);

#if defined(ASAP_WINDOWS)
  auto wpath = path_.wstring();
  DWORD attr(detail::win32_port::GetFileAttributesW(wpath.c_str()));
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return err.report(detail::capture_errno());
  } else {
    // Check if we have a symbolic link
    if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
      try {
        if (detail::win32_port::IsReparsePointSymlink(path_)) {
          cached_data_.symlink = true;
          cached_data_.type = file_type::symlink;
        } else {
          cached_data_.type = file_type::reparse_file;
        }
      } catch (filesystem_error const &ex) {
        return err.report(ex.code());
      }
    }

    // Check if we have a directory
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
      cached_data_.type = file_type::directory;
    }

    if (cached_data_.type == file_type::none ||
        (cached_data_.type == file_type::symlink && follow_symlinks)) {
      // Either we still have not determined yet the file type or it is a
      // symlink and we must follow it to get the target file type.
      std::error_code m_ec;
      auto file = detail::FileDescriptor::Create(
          &path_, m_ec, 0,
          FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
          OPEN_EXISTING,
          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
      if (m_ec) {
        return err.report(m_ec);
      } else {
        BY_HANDLE_FILE_INFORMATION info;
        if (!detail::win32_port::GetFileInformationByHandle(file.fd_, &info)) {
          return err.report(detail::capture_errno());
        } else {
          if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
            cached_data_.type = file_type::reparse_file;
          }
          // Check if we have a directory
          else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            cached_data_.type = file_type::directory;
          } else {
            // Call GetFileType to try to figure out precisely (as much as
            // possible) what is the real file type
            switch (detail::win32_port::GetFileType(file.fd_)) {
              case FILE_TYPE_CHAR:
                cached_data_.type = file_type::character;
                break;
              case FILE_TYPE_PIPE:
                cached_data_.type = file_type::fifo;
                break;
              case FILE_TYPE_DISK:
                cached_data_.type = file_type::regular;
              default:
                std::error_code windows_err = detail::capture_errno();
                if (windows_err) {
                  return err.report(windows_err);
                } else {
                  cached_data_.type = file_type::unknown;
                }
            }
          }
        }
      }
      if (cached_data_.symlink) cached_data_.type_resolved = true;
    }
  }
  cached_data_.cache_type = CacheType_::BASIC;
#else   // !ASAP_WINDOWS
  std::error_code m_ec;
  detail::posix_port::StatT full_st;
  file_status st = detail::posix_port::GetFileStatus(path_, full_st, &m_ec);

  if (cached_data_.cache_type == CacheType_::EMPTY) {
    if (m_ec || !status_known(st)) {
      return err.report(m_ec);
    }
    cached_data_.symlink = filesystem::is_symlink(st);
    cached_data_.type = st.type();
    if (cached_data_.symlink) {
      cached_data_.nlink = static_cast<uintmax_t>(full_st.st_nlink);
      cached_data_.symlink_perms = st.permissions();
    }
  }

  if ((cached_data_.symlink) && follow_symlinks) {
    cached_data_.type = st.type();
    cached_data_.non_symlink_perms = st.permissions();
    cached_data_.type_resolved = true;
    cached_data_.extra_resolved = true;
    cached_data_.perms_resolved = true;
  }

  if (asap::filesystem::is_regular_file(st))
    cached_data_.size = static_cast<uintmax_t>(full_st.st_size);

  if (asap::filesystem::exists(st)) {
    // Attempt to extract the mtime, and fail if it's not representable using
    // file_time_type. For now we ignore the error, as we'll report it when
    // the value is actually used.
    std::error_code ignored_ec;
    cached_data_.write_time =
        detail::posix_port::ExtractLastWriteTime(path_, full_st, &ignored_ec);
  }
  cached_data_.cache_type = CacheType_::FULL;
#endif  // ASAP_WINDOWS
}

void directory_entry::UpdateExtraFileInformation(bool follow_symlinks,
                                                 std::error_code *ec) const {
  ErrorHandler<void> err("UpdateExtraFileInformation", ec, &path_);

#if defined(ASAP_WINDOWS)
  ASAP_ASSERT((cached_data_.cache_type == CacheType_::BASIC) ||
              (cached_data_.cache_type == CacheType_::EXTRA));
  // Open the file handle without following symlinks to get the number of hard
  // links pointing to it.
  std::error_code m_ec;
  {
    auto file = detail::FileDescriptor::Create(
        &path_, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (m_ec) {
      return err.report(m_ec);
    }

    BY_HANDLE_FILE_INFORMATION info;
    if (!detail::win32_port::GetFileInformationByHandle(file.fd_, &info)) {
      return err.report(detail::capture_errno());
    }

    cached_data_.nlink = info.nNumberOfLinks;
  }

  // If the file type is a symlink, reopen the file handle and now follow
  // symlinks to get the size and the last write time.
  if (cached_data_.symlink) {
    auto file = detail::FileDescriptor::Create(
        &path_, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (m_ec) {
      return err.report(m_ec);
    }

    BY_HANDLE_FILE_INFORMATION info;
    if (!detail::win32_port::GetFileInformationByHandle(file.fd_, &info)) {
      return err.report(detail::capture_errno());
    }

    // File size
    if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      return err.report(std::errc::is_a_directory,
                        "directory size is not supported");
    }
    cached_data_.size = (static_cast<uintmax_t>(info.nFileSizeHigh)
                         << (sizeof(info.nFileSizeLow) * 8)) +
                        info.nFileSizeLow;

    // Last write time
    FILETIME lwt;
    if (!detail::win32_port::GetFileTime(file.fd_, 0, 0, &lwt))
      return err.report(detail::capture_errno());
    cached_data_.write_time =
        detail::win32_port::FileTimeTypeFromWindowsFileTime(lwt, m_ec);
    if (m_ec) return err.report(m_ec);

    cached_data_.extra_resolved = true;
  }
#else   // !ASAP_WINDOWS
  ASAP_ASSERT(cached_data_.cache_type == CacheType_::FULL);
  if (cached_data_.symlink && follow_symlinks && !cached_data_.extra_resolved) {
    UpdateBasicFileInformation(true, ec);
  }
#endif  // ASAP_WINDOWS

  cached_data_.cache_type = CacheType_::EXTRA;
}

void directory_entry::UpdatePermissionsInformation(bool follow_symlinks,
                                                   std::error_code *ec) const {
  ErrorHandler<void> err("UpdatePermissionsInformation", ec, &path_);

#if defined(ASAP_WINDOWS)
  ASAP_ASSERT((cached_data_.cache_type == CacheType_::EXTRA) ||
              (cached_data_.cache_type == CacheType_::FULL));
  // Open the file handle without following symlinks
  std::error_code m_ec;
  {
    auto file = detail::FileDescriptor::Create(
        &path_, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (m_ec) {
      return err.report(m_ec);
    }

    FILE_ATTRIBUTE_TAG_INFO file_info;
    if (!detail::win32_port::GetFileInformationByHandleEx(
            file.fd_, FileAttributeTagInfo, &file_info, sizeof(file_info))) {
      return err.report(detail::capture_errno());
    }
    if (cached_data_.symlink) {
      cached_data_.symlink_perms = detail::win32_port::GetPermissions(
          path_, file_info.FileAttributes, follow_symlinks, &m_ec);
    } else {
      cached_data_.non_symlink_perms = detail::win32_port::GetPermissions(
          path_, file_info.FileAttributes, follow_symlinks, &m_ec);
    }
    if (m_ec) {
      return err.report(m_ec);
    }
  }

  // If the file type is a symlink, reopen the file handle and now follow
  // symlinks to get the size and the last write time.
  if (cached_data_.symlink && follow_symlinks) {
    auto file = detail::FileDescriptor::Create(
        &path_, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (m_ec) {
      return err.report(m_ec);
    }

    FILE_ATTRIBUTE_TAG_INFO file_info;
    if (!detail::win32_port::GetFileInformationByHandleEx(
            file.fd_, FileAttributeTagInfo, &file_info, sizeof(file_info))) {
      return err.report(detail::capture_errno());
    }
    cached_data_.non_symlink_perms = detail::win32_port::GetPermissions(
        path_, file_info.FileAttributes, follow_symlinks, &m_ec);
    if (m_ec) {
      return err.report(m_ec);
    }
    cached_data_.perms_resolved = true;
  }
#else   // !ASAP_WINDOWS
  ASAP_ASSERT(cached_data_.cache_type == CacheType_::FULL);
  if (cached_data_.symlink && follow_symlinks && !cached_data_.perms_resolved) {
    UpdateBasicFileInformation(true, ec);
  }
#endif  // ASAP_WINDOWS

  cached_data_.cache_type = CacheType_::FULL;
}  // namespace filesystem

file_type directory_entry::GetSymLinkFileType(std::error_code *ec) const {
  if (ec) ec->clear();
  if (cached_data_.cache_type == CacheType_::EMPTY) {
    UpdateBasicFileInformation(false, ec);
    if (ec) return file_type::none;
  }
  if (cached_data_.symlink) {
    return file_type::symlink;
  } else {
    file_status st(cached_data_.type);
    if (!asap::filesystem::exists(st)) {
      HandleError("in directory_entry::GetSymLinkFileType", ec,
                  make_error_code(std::errc::no_such_file_or_directory),
                  /*allow_dne*/ false);
    }
    return cached_data_.type;
  }
}

file_type directory_entry::GetFileType(std::error_code *ec) const {
  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY:
      UpdateBasicFileInformation(true, ec);
      break;
    case CacheType_::BASIC:
      if (!cached_data_.type_resolved) UpdateBasicFileInformation(true, ec);
      break;
    default:
      // Nothing to do
      break;
  }
  if (ec) return file_type::none;
  file_status st(cached_data_.type);
  if (!asap::filesystem::exists(st)) {
    HandleError("in directory_entry::GetSymLinkFileType", ec,
                make_error_code(std::errc::no_such_file_or_directory),
                /*allow_dne*/ false);
  }
  return cached_data_.type;
}

uintmax_t directory_entry::GetSize(std::error_code *ec) const {
  ErrorHandler<uintmax_t> err("UpdateExtraFileInformation", ec, &path_);

  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY: {
      UpdateBasicFileInformation(true, ec);
      // Check that the entry type supports querying the size
      file_status st(cached_data_.type);
      if (asap::filesystem::exists(st) &&
          !asap::filesystem::is_regular_file(st)) {
        std::errc err_kind = asap::filesystem::is_directory(st)
                                 ? std::errc::is_a_directory
                                 : std::errc::not_supported;
        return err.report(err_kind,
                          "can only query size of regular files that do exist");
      }
      // continue to update extra file attributes
    }
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
    /* FALLTHRU */
    case CacheType_::BASIC:
      UpdateExtraFileInformation(true, ec);
      break;
    case CacheType_::EXTRA:
      if (!cached_data_.extra_resolved) UpdateExtraFileInformation(true, ec);
      break;

    default:
      // Nothing to do
      break;
  }

  return cached_data_.size;
}

uintmax_t directory_entry::GetHardLinkCount(std::error_code *ec) const {
  ErrorHandler<uintmax_t> err("GetHardLinkCount", ec, &path_);

  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY:
      UpdateBasicFileInformation(false, ec);
      // continue to update extra file attributes
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::BASIC:
      UpdateExtraFileInformation(false, ec);
      break;

    default:
      // Nothing to do
      break;
  }

  return cached_data_.nlink;
}

file_time_type directory_entry::GetLastWriteTime(std::error_code *ec) const {
  ErrorHandler<file_time_type> err("GetLastWriteTime", ec, &path_);

  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY:
      UpdateBasicFileInformation(true, ec);
      // continue to update extra file attributes
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::BASIC:
      UpdateExtraFileInformation(true, ec);
      break;
    case CacheType_::EXTRA:
      if (!cached_data_.extra_resolved) UpdateExtraFileInformation(true, ec);
      break;

    default:
      // Nothing to do
      break;
  }
  file_status st(cached_data_.type);
  if (asap::filesystem::exists(st) &&
      cached_data_.write_time == file_time_type::min())
    return err.report(std::errc::value_too_large);

  return cached_data_.write_time;
}

file_status directory_entry::GetStatus(std::error_code *ec) const {
  ErrorHandler<file_time_type> err("GetLastWriteTime", ec, &path_);

  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY:
      UpdateBasicFileInformation(true, ec);
      // continue to update extra file attributes
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::BASIC:
      // Don't resolve links for extra information as we may not need it if the
      // caller is just asking for permissions information.
      UpdateExtraFileInformation(false, ec);
      // continue to update permissions
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::EXTRA:
      UpdatePermissionsInformation(true, ec);
      break;
    case CacheType_::FULL:
      if (!cached_data_.perms_resolved) UpdatePermissionsInformation(true, ec);
      break;
#if !defined(__clang__)
    // clang properly sees that the switch covers all enum values and therefore
    // the default case is unnecessary
    default:
      HEDLEY_UNREACHABLE();
#endif  // __clang__
  }
  return file_status(cached_data_.type, cached_data_.non_symlink_perms);
}

file_status directory_entry::GetSymLinkStatus(std::error_code *ec) const {
  ErrorHandler<file_time_type> err("GetLastWriteTime", ec, &path_);

  if (ec) ec->clear();
  switch (cached_data_.cache_type) {
    case CacheType_::EMPTY:
      UpdateBasicFileInformation(false, ec);
      // continue to update extra file attributes
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::BASIC:
      UpdateExtraFileInformation(false, ec);
      // continue to update permissions
#if defined(__clang__)
      [[clang::fallthrough]];
#endif  // __clang__
      /* FALLTHRU */
    case CacheType_::EXTRA:
      UpdatePermissionsInformation(false, ec);
      break;

    default:
      // Nothing to do
      break;
  }
  return file_status(cached_data_.type, cached_data_.symlink_perms);
}

#ifndef ASAP_WINDOWS
std::error_code directory_entry::DoRefresh_impl() const noexcept {
  cached_data_.Reset();
  std::error_code failure_ec;
  UpdateBasicFileInformation(true, &failure_ec);
  return failure_ec;
}
#else
std::error_code directory_entry::DoRefresh_impl() const noexcept {
  cached_data_.Reset();
  std::error_code failure_ec;
  UpdateBasicFileInformation(true, &failure_ec);
  if (failure_ec) return failure_ec;
  UpdateExtraFileInformation(true, &failure_ec);
  if (failure_ec) return failure_ec;
  UpdatePermissionsInformation(true, &failure_ec);
  return failure_ec;
}
#endif

namespace detail {
namespace {

#if !defined(ASAP_WINDOWS)
template <class DirEntT, class = decltype(DirEntT::d_type)>
file_type get_file_type(DirEntT *ent) {
  switch (ent->d_type) {
    case DT_BLK:
      return file_type::block;
    case DT_CHR:
      return file_type::character;
    case DT_DIR:
      return file_type::directory;
    case DT_FIFO:
      return file_type::fifo;
    case DT_LNK:
      return file_type::symlink;
    case DT_REG:
      return file_type::regular;
    case DT_SOCK:
      return file_type::socket;
      // Unlike in lstat, hitting "unknown" here simply means that the
      // underlying filesystem doesn't support d_type. Report is as 'none' so
      // we correctly set the cache to empty.
    case DT_UNKNOWN:
      break;
    default:
      HEDLEY_UNREACHABLE();
  }
  return file_type::none;
}

std::pair<std::string, file_type> posix_readdir(DIR *dir_stream,
                                                std::error_code &ec) {
  struct dirent *dir_entry_ptr = nullptr;
  errno = 0;  // zero errno in order to detect errors
  ec.clear();
  if ((dir_entry_ptr = posix_port::readdir(dir_stream)) == nullptr) {
    if (errno) ec = detail::capture_errno();
    return {};
  } else {
    return {dir_entry_ptr->d_name, get_file_type(dir_entry_ptr)};
  }
}
#else

file_type get_file_type(const WIN32_FIND_DATAW &data) {
  auto attrs = data.dwFileAttributes;
  if (attrs & FILE_ATTRIBUTE_DIRECTORY) return file_type::directory;
  if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
    auto reparseTag = data.dwReserved0;
    return (reparseTag == IO_REPARSE_TAG_SYMLINK
            // Directory junctions are very similar to symlinks, but have some
            // performance and other advantages over symlinks. They can be
            // created from the command line with "mklink /j junction-name
            // target-path".
            || reparseTag == IO_REPARSE_TAG_MOUNT_POINT)
               ? file_type::symlink
               : file_type::reparse_file;
  }
  return file_type::regular;
}

uintmax_t get_file_size(const WIN32_FIND_DATAW &data) {
  return (data.nFileSizeHigh * (MAXDWORD + 1)) + data.nFileSizeLow;
}
file_time_type get_write_time(const WIN32_FIND_DATAW &data) {
  ULARGE_INTEGER tmp;
  auto &time = data.ftLastWriteTime;
  tmp.u.LowPart = time.dwLowDateTime;
  tmp.u.HighPart = time.dwHighDateTime;

  const long WINDOWS_TICK = 10000000;
  const long long NANOSEC_TO_UNIX_EPOCH = 11644473600000000000LL;

  return file_time_type(
      file_time_type::duration(tmp.QuadPart * 100 - NANOSEC_TO_UNIX_EPOCH));
}

#endif

}  // namespace
}  // namespace detail

using detail::ErrorHandler;

#if defined(ASAP_WINDOWS)
class DirectoryStream {
 public:
  DirectoryStream() = delete;
  DirectoryStream &operator=(const DirectoryStream &) = delete;

  DirectoryStream(DirectoryStream &&other) noexcept
      : stream_(other.stream_),
        root_(std::move(other.root_)),
        entry_(std::move(other.entry_)) {
    other.stream_ = INVALID_HANDLE_VALUE;
  }

  DirectoryStream(const path &root, directory_options opts, std::error_code &ec)
      : stream_(INVALID_HANDLE_VALUE), root_(root) {
    auto wdirpath = root.wstring();
    // This form of search will work with all versions of windows
    wdirpath += (wdirpath.empty() || (wdirpath[wdirpath.size() - 1] != L'\\' &&
                                      wdirpath[wdirpath.size() - 1] != L'/' &&
                                      wdirpath[wdirpath.size() - 1] != L':'))
                    ? L"\\*"
                    : L"*";
    stream_ = ::FindFirstFileW(wdirpath.c_str(), &entry_data_);
    // Skip the '.' and '..'
    if (stream_ != INVALID_HANDLE_VALUE) {
      while (!wcscmp(entry_data_.cFileName, L".") ||
             !wcscmp(entry_data_.cFileName, L"..")) {
        if (!::FindNextFileW(stream_, &entry_data_)) {
          close();
          break;
        }
      }
    }
    if (stream_ == INVALID_HANDLE_VALUE) {
      ec = std::error_code(::GetLastError(), std::generic_category());
      const bool ignore_permission_denied =
          bool(opts & directory_options::skip_permission_denied);
      if (ec.value() == ERROR_ACCESS_DENIED) {
        if (ignore_permission_denied)
          ec.clear();
        else
          ec.assign(static_cast<typename std::underlying_type<std::errc>::type>(
                        std::errc::permission_denied),
                    std::generic_category());
      }
      // We consider the situation of no more files available as ok
      if (ec.value() == ERROR_NO_MORE_FILES) ec.clear();
    } else {
      entry_.path_ = root_ / entry_data_.cFileName;
      entry_.cached_data_.type = detail::get_file_type(entry_data_);
      if (entry_.cached_data_.type == file_type::symlink) {
        entry_.cached_data_.symlink = true;
        // FIXME: If the path points to a symbolic link, the WIN32_FIND_DATA
        // buffer contains information about the symbolic link, not the
        // target.
      }
      entry_.cached_data_.cache_type = directory_entry::CacheType_::BASIC;

      // FIXME: Cache more of this
      // directory_entry::CachedData_ cdata;
      // cdata.type = get_file_type(cached_data_);
      // cdata.size = get_file_size(cached_data_);
      // cdata.write_time = get_write_time(cached_data_);
    }
  }

  ~DirectoryStream() noexcept {
    if (stream_ == INVALID_HANDLE_VALUE) return;
    close();
  }

  bool good() const noexcept { return stream_ != INVALID_HANDLE_VALUE; }

  bool advance(std::error_code &ec) {
    while (::FindNextFileW(stream_, &entry_data_)) {
      if (!wcscmp(entry_data_.cFileName, L".") ||
          !wcscmp(entry_data_.cFileName, L".."))
        continue;

      entry_.path_ = root_ / entry_data_.cFileName;
      entry_.cached_data_.type = detail::get_file_type(entry_data_);
      if (entry_.cached_data_.type == file_type::symlink) {
        entry_.cached_data_.symlink = true;
        // FIXME: If the path points to a symbolic link, the WIN32_FIND_DATA
        // buffer contains information about the symbolic link, not the
        // target.
      }
      entry_.cached_data_.cache_type = directory_entry::CacheType_::BASIC;

      // FIXME: Cache more of this
      // directory_entry::CachedData_ cdata;
      // cdata.type = get_file_type(cached_data_);
      // cdata.size = get_file_size(cached_data_);
      // cdata.write_time = get_write_time(cached_data_);
      return true;
    }
    ec = std::error_code(::GetLastError(), std::generic_category());
    close();
    // We consider the situation of no more files available as ok
    if (ec.value() == ERROR_NO_MORE_FILES) ec.clear();
    return false;
  }

 private:
  std::error_code close() noexcept {
    std::error_code ec;
    if (!::FindClose(stream_))
      ec = std::error_code(::GetLastError(), std::generic_category());
    stream_ = INVALID_HANDLE_VALUE;
    return ec;
  }

  HANDLE stream_{INVALID_HANDLE_VALUE};
  WIN32_FIND_DATAW entry_data_;

 public:
  path root_;
  directory_entry entry_;
};
#else
class DirectoryStream {
 public:
  DirectoryStream() = delete;
  DirectoryStream &operator=(const DirectoryStream &) = delete;

  DirectoryStream(DirectoryStream &&other) noexcept
      : stream_(other.stream_),
        root_(std::move(other.root_)),
        entry_(std::move(other.entry_)) {
    other.stream_ = nullptr;
  }

  DirectoryStream(const path &root, directory_options opts, std::error_code &ec)
      : stream_(nullptr), root_(root) {
    if ((stream_ = ::opendir(root.c_str())) == nullptr) {
      ec = detail::capture_errno();
      const auto allow_eacess =
          bool(opts & directory_options::skip_permission_denied);
      if (allow_eacess && ec.value() == EACCES) ec.clear();
      return;
    }
    advance(ec);
  }

  ~DirectoryStream() noexcept {
    if (stream_) close();
  }

  bool good() const noexcept { return stream_ != nullptr; }

  bool advance(std::error_code &ec) {
    while (true) {
      auto entry_data_ = detail::posix_readdir(stream_, ec);
      auto &str = entry_data_.first;
      if (str == "." || str == "..") {
        continue;
      } else if (ec || str.empty()) {
        close();
        return false;
      } else {
        entry_.path_ = root_ / str;
        entry_.cached_data_.type = entry_data_.second;
        entry_.cached_data_.cache_type = directory_entry::CacheType_::BASIC;
        if (entry_.cached_data_.type == file_type::symlink) {
          entry_.cached_data_.symlink = true;
          // FIXME: check if read_dir follows sumlinks or not
        }
        return true;
      }
    }
  }

 private:
  std::error_code close() noexcept {
    std::error_code m_ec;
    if (::closedir(stream_) == -1) m_ec = detail::capture_errno();
    stream_ = nullptr;
    return m_ec;
  }

  DIR *stream_{nullptr};

 public:
  path root_;
  directory_entry entry_;
};
#endif

// directory_iterator

directory_iterator::directory_iterator(const path &p, std::error_code *ec,
                                       directory_options opts) {
  ErrorHandler<void> err("directory_iterator::directory_iterator(...)", ec, &p);

  std::error_code m_ec;
  impl_ = std::make_shared<DirectoryStream>(p, opts, m_ec);
  if (ec) *ec = m_ec;
  if (!impl_->good()) {
    impl_.reset();
    if (m_ec) err.report(m_ec);
  }
}

directory_iterator &directory_iterator::do_increment(std::error_code *ec) {
  ErrorHandler<void> err("directory_iterator::operator++()", ec);
  if (impl_) {
    std::error_code m_ec;
    if (!impl_->advance(m_ec)) {
      path root = std::move(impl_->root_);
      impl_.reset();
      if (m_ec) err.report(m_ec, "at root \"" + root.string() + "\"");
    }
  } else {
    err.report(std::errc::invalid_argument, "invalid iterator");
  }
  return *this;
}

directory_entry const &directory_iterator::dereference() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->entry_;
}

// recursive_directory_iterator

struct recursive_directory_iterator::SharedImpl {
  std::stack<DirectoryStream> dir_stack;
  directory_options options{directory_options::none};
};

recursive_directory_iterator::recursive_directory_iterator(
    const path &p, directory_options opt, std::error_code *ec)
    : impl_(nullptr), recursion_(true) {
  ErrorHandler<void> err("recursive_directory_iterator", ec, &p);

  std::error_code m_ec;
  DirectoryStream new_s(p, opt, m_ec);
  if (m_ec) err.report(m_ec);
  if (m_ec || !new_s.good()) return;

  impl_ = std::make_shared<SharedImpl>();
  impl_->options = opt;
  impl_->dir_stack.push(std::move(new_s));
}

void recursive_directory_iterator::pop_impl(std::error_code *ec) {
  ErrorHandler<void> err("directory_iterator::pop()", ec);
  if (impl_) {
    impl_->dir_stack.pop();
    if (impl_->dir_stack.empty())
      impl_.reset();
    else
      Advance(ec);
  } else {
    err.report(std::errc::invalid_argument, "invalid iterator");
  }
}

directory_options recursive_directory_iterator::options() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->options;
}

int recursive_directory_iterator::depth() const {
  ErrorHandler<int> err("recursive_directory_iterator::depth()", nullptr);
  if (!impl_)
    return err.report(std::errc::invalid_argument, "invalid iterator");

  return static_cast<int>(impl_->dir_stack.size() - 1);
}

const directory_entry &recursive_directory_iterator::dereference() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->dir_stack.top().entry_;
}

recursive_directory_iterator &recursive_directory_iterator::do_increment(
    std::error_code *ec) {
  if (ec) ec->clear();
  if (recursion_pending()) {
    if (TryRecursion(ec) || (ec && *ec)) return *this;
  }
  recursion_ = true;
  Advance(ec);
  return *this;
}

void recursive_directory_iterator::Advance(std::error_code *ec) {
  ErrorHandler<void> err("recursive_directory_iterator::Advance()", ec);
  if (!impl_)
    return err.report(std::errc::invalid_argument, "invalid iterator");

  const directory_iterator end_it;
  auto &stack = impl_->dir_stack;
  std::error_code m_ec;
  while (!stack.empty()) {
    if (stack.top().advance(m_ec)) return;
    if (m_ec) break;
    stack.pop();
  }

  if (m_ec) {
    path root = std::move(stack.top().root_);
    impl_.reset();
    err.report(m_ec, "at root \"{" + root.string() + "}\"");
  } else {
    impl_.reset();
  }
}

bool recursive_directory_iterator::TryRecursion(std::error_code *ec) {
  ErrorHandler<bool> err("recursive_directory_iterator::TryRecursion()", ec);
  if (!impl_)
    return err.report(make_error_code(std::errc::invalid_argument),
                      "invalid iterator");

  auto rec_sym = bool(options() & directory_options::follow_directory_symlink);

  auto &curr_it = impl_->dir_stack.top();

  bool skip_rec = false;
  std::error_code m_ec;
  if (!rec_sym) {
    file_status st(curr_it.entry_.GetSymLinkFileType(&m_ec));
    if (m_ec && status_known(st)) m_ec.clear();
    if (m_ec || is_symlink(st) || !is_directory(st)) skip_rec = true;
  } else {
    file_status st(curr_it.entry_.GetFileType(&m_ec));
    if (m_ec && status_known(st)) m_ec.clear();
    if (m_ec || !is_directory(st)) skip_rec = true;
  }

  if (!skip_rec) {
    DirectoryStream new_it(curr_it.entry_.path(), impl_->options, m_ec);
    if (new_it.good()) {
      impl_->dir_stack.push(std::move(new_it));
      return true;
    }
  }
  if (m_ec) {
    const auto allow_eacess =
        bool(impl_->options & directory_options::skip_permission_denied);
    if (m_ec.value() == EACCES && allow_eacess) {
      if (ec) ec->clear();
    } else {
      path at_ent = std::move(curr_it.entry_.path_);
      impl_.reset();
      err.report(m_ec,
                 "attempting recursion into \"{" + at_ent.string() + "}\"");
    }
  }
  return false;
}

}  // namespace filesystem
}  // namespace asap
