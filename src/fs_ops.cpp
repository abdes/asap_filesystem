//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <array>
#include <chrono>
#include <deque>
#include <stack>

#include "fs_portability.h"

namespace asap {
namespace filesystem {

namespace detail {
// -----------------------------------------------------------------------------
//                          detail: options bitset
// -----------------------------------------------------------------------------

template <typename Bitmask>
inline auto is_set(Bitmask obj, Bitmask bits) -> bool {
  return (obj & bits) != Bitmask::none;
}

}  // namespace detail

using detail::capture_errno;
using detail::ErrorHandler;
using detail::FileDescriptor;
#if defined(ASAP_POSIX)
using detail::posix_port::StatT;
#endif

// -----------------------------------------------------------------------------
//                               absolute
// -----------------------------------------------------------------------------

static auto do_absolute_impl(const path &p, path *cwd, std::error_code *ec)
    -> path {
  if (ec != nullptr) {
    ec->clear();
  }
  if (p.is_absolute()
#if defined(ASAP_WINDOWS)
      && p.has_root_name()
#endif
  ) {
    return p;
  }
  *cwd = current_path_impl(ec);
  if ((ec != nullptr) && *ec) {
    return {};
  }
  return (*cwd) / p;
}

auto absolute_impl(const path &p, std::error_code *ec) -> path {
  path cwd;
  return do_absolute_impl(p, &cwd, ec);
}

// -----------------------------------------------------------------------------
//                               canonical
// -----------------------------------------------------------------------------

// TODO(abdessattar): refactor to make this available in one single place
namespace {
constexpr path::value_type dot = '.';

inline auto is_dot(path::value_type c) -> bool { return c == dot; }

inline auto is_dot(const path &path) -> bool {
  const auto &filename = path.native();
  return filename.size() == 1 && is_dot(filename[0]);
}

inline auto is_dotdot(const path &path) -> bool {
  const auto &filename = path.native();
  return filename.size() == 2 && is_dot(filename[0]) && is_dot(filename[1]);
}
}  // namespace

auto canonical_impl(path const &orig_p, std::error_code *ec) -> path {
  path cwd;
  ErrorHandler<path> err("canonical", ec, &orig_p, &cwd);

  path pa = do_absolute_impl(orig_p, &cwd, ec);

#ifndef ASAP_WINDOWS
  // Use OpenGroup realpath()
  char buff[PATH_MAX + 1];
  char *ret = nullptr;
  if ((ret = ::realpath(pa.c_str(), buff)) == nullptr) {
    return err.report(capture_errno());
  }
  return {ret};
#else

  std::error_code m_ec;
  //  if (!exists(pa, m_ec)) {
  //    if (!m_ec) return err.report(std::errc::no_such_file_or_directory);
  //  }
  // else: we know there are (currently) no unresolvable symlink loops

  path result = pa.root_path();

  std::deque<path> cmpts;
  for (const auto &f : pa.relative_path()) {
    cmpts.push_back(f);
  }

  int max_allowed_symlinks = 40;

  while (!cmpts.empty() && !m_ec) {
    path f = std::move(cmpts.front());
    cmpts.pop_front();

    if (f.empty()) {
      // ignore
    } else if (is_dot(f)) {
      if (!is_directory(result, m_ec) && !m_ec) {
        err.report(std::errc::not_a_directory);
      }
    } else if (is_dotdot(f)) {
      auto parent = result.parent_path();
      if (parent.empty()) {
        result = pa.root_path();
      } else {
        result.swap(parent);
      }
    } else {
      result /= f;

      if (is_symlink(result, m_ec)) {
        path link = read_symlink(result, m_ec);
        if (!m_ec) {
          if (--max_allowed_symlinks == 0) {
            err.report(std::errc::too_many_symbolic_link_levels);
          } else {
            if (link.is_absolute()) {
              result = link.root_path();
              link = link.relative_path();
            } else {
              result = result.parent_path();
            }

            cmpts.insert(cmpts.begin(), link.begin(), link.end());
          }
        }
      }
    }
  }

  // Standard says that the canonical path must refer to an existing path.
  if (m_ec || !exists(result, m_ec)) {
    return err.report(std::errc::no_such_file_or_directory);
  }

  return result;
#endif
}

// -----------------------------------------------------------------------------
//                             copy
// -----------------------------------------------------------------------------

#if defined(ASAP_POSIX)
namespace {
auto stat_equivalent(const StatT &st1, const StatT &st2) -> bool {
  return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino);
}
}  // namespace
#endif  // ASAP_POSIX

void copy_impl(const path &from, const path &to, copy_options options,
               std::error_code *ec) {
  ErrorHandler<void> err("copy", ec, &from, &to);

  const bool skip_symlinks =
      detail::is_set(options, copy_options::skip_symlinks);
  const bool create_symlinks =
      detail::is_set(options, copy_options::create_symlinks);
  const bool copy_symlinks =
      detail::is_set(options, copy_options::copy_symlinks);

  file_status f;
  file_status t;
#if defined(ASAP_WINDOWS)
  // TODO(Abdessattar): Combine this with the non-Windows code
  std::error_code m_ec1;
  bool use_lstat = create_symlinks || skip_symlinks || copy_symlinks;
  f = use_lstat ? symlink_status_impl(from, &m_ec1) : status_impl(from, &m_ec1);
  if (m_ec1) {
    return err.report(m_ec1);
  }

  use_lstat = create_symlinks || skip_symlinks;
  t = use_lstat ? symlink_status_impl(to, &m_ec1) : status_impl(to, &m_ec1);
  if (!status_known(t)) {
    return err.report(m_ec1);
  }

  if (!exists(f) || is_other(f) || is_other(t) ||
      (is_directory(f) && is_regular_file(t))) {
    return err.report(std::errc::function_not_supported);
  }
  // TODO(Abdessattar): status will be queried again here. Need optimization.
  if (exists(f) && exists(t)) {
    bool same_file = equivalent_impl(from, to, &m_ec1);
    if (m_ec1) {
      return err.report(m_ec1);
    }
    if (same_file) {
      return err.report(std::errc::function_not_supported);
    }
  }
#else
  std::error_code m_ec1;
  StatT f_st = {};
  // For the souce file, if it's a symlink and the copy_options require to
  // act on the symlink rather than the file pointed to, then use lstat
  bool use_lstat = create_symlinks || skip_symlinks || copy_symlinks;
  f = use_lstat ? detail::posix_port::GetLinkStatus(from, f_st, &m_ec1)
                : detail::posix_port::GetFileStatus(from, f_st, &m_ec1);
  if (m_ec1) {
    return err.report(m_ec1);
  }

  StatT t_st = {};
  use_lstat = create_symlinks || skip_symlinks;
  t = use_lstat ? detail::posix_port::GetLinkStatus(to, t_st, &m_ec1)
                : detail::posix_port::GetFileStatus(to, t_st, &m_ec1);
  if (!status_known(t)) {
    return err.report(m_ec1);
  }

  if (!exists(f) || is_other(f) || is_other(t) ||
      (is_directory(f) && is_regular_file(t)) || stat_equivalent(f_st, t_st)) {
    return err.report(std::errc::function_not_supported);
  }
#endif  // ASAP_WINDOWS

  if (is_symlink(f)) {
    if (skip_symlinks) {
      // do nothing
    } else if (!exists(t)) {
      copy_symlink_impl(from, to, ec);
    } else {
      return err.report(std::errc::file_exists);
    }
    return;
  }
  if (is_regular_file(f)) {
    if (detail::is_set(options, copy_options::directories_only)) {
      // do nothing
    } else if (create_symlinks) {
      create_symlink_impl(from, to, ec);
    } else if (detail::is_set(options, copy_options::create_hard_links)) {
      create_hard_link_impl(from, to, ec);
    } else if (is_directory(t)) {
      copy_file_impl(from, to / from.filename(), options, ec);
    } else {
      copy_file_impl(from, to, options, ec);
    }
    return;
  }
  if (is_directory(f) && create_symlinks) {
    return err.report(std::errc::is_a_directory);
  }
  if (is_directory(f) && (detail::is_set(options, copy_options::recursive) ||
                          copy_options::none == options)) {
    if (!exists(t)) {
      // create directory to with attributes from 'from'.
      create_directory_impl(to, from, ec);
      if ((ec != nullptr) && *ec) {
        return;
      }
    }
    directory_iterator it = ec != nullptr ? directory_iterator(from, *ec)
                                          : directory_iterator(from);
    if ((ec != nullptr) && *ec) {
      return;
    }
    std::error_code m_ec2;
    for (; it != directory_iterator(); it.increment(m_ec2)) {
      if (m_ec2) {
        return err.report(m_ec2);
      }
      copy_impl(it->path(), to / it->path().filename(), options, ec);
      if ((ec != nullptr) && *ec) {
        return;
      }
    }
  }
}

namespace {
#if defined(ASAP_WINDOWS)
auto do_copy_file_win32(FileDescriptor &read_fd, FileDescriptor &write_fd,
                        std::error_code &ec) -> bool {
  auto read_wpath = read_fd.name_.wstring();
  auto write_wpath = write_fd.name_.wstring();
  if (detail::win32_port::CopyFileW(read_wpath.c_str(), write_wpath.c_str(),
                                    1) != 0) {
    ec = capture_errno();
    return false;
  }

  ec.clear();
  return true;
}
#else  // ASAP_WINDOWS
#if ASAP_FS_USE_SENDFILE
// NOTE:
// The LINUX method using sendfile() has a major problem in that it can
// not copy files more than 2GB in size!
// http://man7.org/linux/man-pages/man2/sendfile.2.html
//
//    sendfile() will transfer at most 0x7ffff000 (2,147,479,552) bytes,
//    returning the number of bytes actually transferred. (This is true on
//    both 32-bit and 64-bit systems.)
auto do_copy_file_sendfile(FileDescriptor &read_fd, FileDescriptor &write_fd,
                           std::error_code &ec) -> bool {
  const auto SENDFILE_MAX_BYTES = 0x7ffff000;
  auto count = read_fd.PosixStatus().st_size;
  if (count < 0 || count > SENDFILE_MAX_BYTES) {
    return false;
  }
  do {
    ssize_t res = 0;
    if ((res = detail::linux_port::sendfile(write_fd.fd_, read_fd.fd_, nullptr,
                                            static_cast<size_t>(count))) ==
        -1) {
      ec = capture_errno();
      return false;
    }
    count -= res;
  } while (count > 0);

  ec.clear();

  return true;
}
#elif ASAP_FS_USE_COPYFILE
bool do_copy_file_copyfile(FileDescriptor &read_fd, FileDescriptor &write_fd,
                           std::error_code &ec) {
  struct CopyFileState {
    copyfile_state_t state;
    CopyFileState() { state = copyfile_state_alloc(); }
    ~CopyFileState() { copyfile_state_free(state); }

   private:
    CopyFileState(CopyFileState const &) = delete;
    CopyFileState &operator=(CopyFileState const &) = delete;
  };

  CopyFileState cfs;
  if (detail::apple_port::fcopyfile(read_fd.fd_, write_fd.fd_, cfs.state,
                                    COPYFILE_DATA) < 0) {
    ec = capture_errno();
    return false;
  }

  ec.clear();
  return true;
}
#endif

auto do_copy_file_default(FileDescriptor &read_fd, FileDescriptor &write_fd,
                          std::error_code &ec) -> bool {
  // BUFSIZ defaults to 8192
  // BUFSIZ of  means one chareter at time
  // good values should fit to blocksize, like 1024 or 4096
  // higher values reduce number of system calls
  // constexpr size_t BUFFER_SIZE = 4096;
  char buf[BUFSIZ];
  ssize_t size = 0;

  while ((size = detail::posix_port::read(read_fd.fd_, buf, BUFSIZ)) > 0) {
    if (detail::posix_port::write(write_fd.fd_, buf,
                                  static_cast<size_t>(size)) < 0) {
      ec = capture_errno();
      return false;
    }
  }
  if (size == -1) {
    ec = capture_errno();
    return false;
  }

  ec.clear();
  return true;
}
#endif  // ASAP_WINDOWS

auto do_copy_file(FileDescriptor &from, FileDescriptor &to, std::error_code &ec)
    -> bool {
#if defined(ASAP_WINDOWS)
  return do_copy_file_win32(from, to, ec);
#else  // ASAP_WINDOWS
#if ASAP_FS_USE_SENDFILE
  // Prefer to use sendfile when it is available.
  // Check if sendfile cannot handle the large files and if so, fallback
  // to copyfile() or default implementation.
  if (do_copy_file_sendfile(from, to, ec)) {
    return true;
  }
#elif ASAP_FS_USE_COPYFILE
  if (do_copy_file_copyfile(from, to, ec)) return true;
#endif
  return do_copy_file_default(from, to, ec);
#endif  // ASAP_WINDOWS
}

}  // namespace

auto copy_file_impl(const path &from, const path &to, copy_options options,
                    std::error_code *ec) -> bool {
  ErrorHandler<bool> err("copy_file", ec, &to, &from);

  std::error_code m_ec;

  const bool skip_existing = bool(copy_options::skip_existing & options);
  const bool update_existing = bool(copy_options::update_existing & options);
  const bool overwrite_existing =
      bool(copy_options::overwrite_existing & options);

#if defined(ASAP_WINDOWS)
  // TODO(Abdessattar): refactor code to share portable portables

  auto from_st = status_impl(from, &m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }

  if (!is_regular_file(from_st)) {
    return err.report(make_error_code(std::errc::not_supported));
  }

  auto to_st = status_impl(to, &m_ec);
  if (!status_known(to_st)) {
    return err.report(m_ec);
  }

  const bool to_exists = exists(to_st);
  if (to_exists && !is_regular_file(to_st)) {
    return err.report(std::errc::not_supported);
  }

  if (to_exists && equivalent_impl(from, to, &m_ec)) {
    if (m_ec) {
      return err.report(std::errc::invalid_argument);
    }
    return err.report(std::errc::file_exists);
  }
  if (to_exists && skip_existing) {
    return false;
  }

  bool ShouldCopy = [&]() {
    if (to_exists && update_existing) {
      auto from_time = last_write_time_impl(from, &m_ec);
      if (m_ec) {
        return err.report(m_ec);
      }
      auto to_time = last_write_time_impl(to, &m_ec);
      if (m_ec) {
        return err.report(m_ec);
      }
      return from_time >= to_time;
    }
    if (!to_exists || overwrite_existing) {
      return true;
    }
    return err.report(std::errc::file_exists);
  }();
  if (!ShouldCopy) {
    return false;
  }

  auto from_wpath = from.wstring();
  auto to_wpath = to.wstring();
  if (detail::win32_port::CopyFileW(from_wpath.c_str(), to_wpath.c_str(), 0) ==
      0) {
    return err.report(capture_errno());
  }

  return true;
#else   // ASAP_WINDOWS
  FileDescriptor from_fd =
      FileDescriptor::Create(&from, m_ec, O_RDONLY | O_NONBLOCK);
  if (m_ec) {
    return err.report(m_ec);
  }

  from_fd.RefreshStatus(false, m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }

  auto from_st = from_fd.Status();
  StatT const &from_stat = from_fd.PosixStatus();
  if (!is_regular_file(from_st)) {
    if (!m_ec) {
      m_ec = make_error_code(std::errc::not_supported);
    }
    return err.report(m_ec);
  }

  StatT to_stat_path;
  file_status to_st =
      detail::posix_port::GetFileStatus(to, to_stat_path, &m_ec);
  if (!status_known(to_st)) {
    return err.report(m_ec);
  }

  const bool to_exists = exists(to_st);
  if (to_exists && !is_regular_file(to_st)) {
    return err.report(std::errc::not_supported);
  }

  if (to_exists && stat_equivalent(from_stat, to_stat_path)) {
    return err.report(std::errc::file_exists);
  }

  if (to_exists && skip_existing) {
    return false;
  }

  bool ShouldCopy = [&]() {
    if (to_exists && update_existing) {
      auto from_time = detail::posix_port::ExtractModificationTime(from_stat);
      auto to_time = detail::posix_port::ExtractModificationTime(to_stat_path);
      if (from_time.tv_sec < to_time.tv_sec) {
        return false;
      }
      if (from_time.tv_sec == to_time.tv_sec &&
          from_time.tv_nsec <= to_time.tv_nsec) {
        return false;
      }
      return true;
    }
    if (!to_exists || overwrite_existing) {
      return true;
    }
    return err.report(std::errc::file_exists);
  }();
  if (!ShouldCopy) {
    return false;
  }

  // Don't truncate right away. We may not be opening the file we
  // originally looked at; we'll check this later.
  int to_open_flags = O_WRONLY;
  if (!to_exists) {
    to_open_flags |= O_CREAT;
  }
  FileDescriptor to_fd =
      FileDescriptor::Create(&to, m_ec, to_open_flags, from_stat.st_mode);
  if (m_ec) {
    return err.report(m_ec);
  }

  to_fd.RefreshStatus(false, m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }

  if (to_exists) {
    // Check that the file we initially stat'ed is equivalent to the one
    // we opened.
    // FIXME: report this better.
    if (!stat_equivalent(to_stat_path, to_fd.PosixStatus())) {
      return err.report(std::errc::bad_file_descriptor);
    }

    // Set the permissions and truncate the file we opened.
    if (detail::posix_port::fchmod(to_fd.fd_, from_stat.st_mode) != 0) {
      return err.report(capture_errno());
    }
    if (detail::posix_port::ftruncate(to_fd.fd_, 0) != 0) {
      return err.report(capture_errno());
    }
  }

  if (!do_copy_file(from_fd, to_fd, m_ec)) {
    // FIXME: Remove the dest file if we failed, and it didn't exist
    // previously.
    return err.report(m_ec);
  }

  return true;
#endif  // ASAP_WINDOWS
}

void copy_symlink_impl(const path &existing_symlink, const path &new_symlink,
                       std::error_code *ec) {
  ErrorHandler<void> err("copy_symlink", ec, &existing_symlink, &new_symlink);

  std::error_code m_ec;

  const path real_path(read_symlink_impl(existing_symlink, &m_ec));
  if (m_ec) {
    return err.report(m_ec);
  }

  // NOTE: proposal says you should detect if you should call
  // create_symlink or create_directory_symlink. Although this
  // is not needed with POSIX it is needed with Windows.
#if defined(ASAP_WINDOWS)
  bool is_dir = is_directory(real_path, m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }
  if (is_dir) {
    create_directory_symlink_impl(real_path, new_symlink, ec);
  } else {
    create_symlink_impl(real_path, new_symlink, ec);
  }
#else
  create_symlink_impl(real_path, new_symlink, ec);
#endif
}

// -----------------------------------------------------------------------------
//                             create directory
// -----------------------------------------------------------------------------

auto create_directories_impl(const path &p, std::error_code *ec) -> bool {
  ErrorHandler<bool> err("create_directories", ec, &p);

  if (p.empty()) {
    return err.report(std::errc::invalid_argument);
  }

  std::stack<path> missing;
  path pp = p;

  // NOTE: to work around the differences between the way POSIX systems and
  // Windows handle the status of a path, we need to push all the components of
  // the path (except . and ..) on the missing stack.
  //
  // The issue is that if we try to create the directories in the path
  // "./foo/../bar", POSIX systems stat will require that all components of the
  // path do exist while windows GetFileAttributes will simplify the path into
  // "bar" and checks for bar only, thus losing the other components of the
  // path.
  std::error_code m_ec;
  while (pp.has_filename() /*&& status(pp, m_ec).type() == file_type::not_found*/) {
    m_ec.clear();
    const auto &filename = pp.filename();
    if (!is_dot(filename) && !is_dotdot(filename)) {
      missing.push(pp);
    }
    pp = pp.parent_path();
  }
  if (m_ec) {
    return err.report(m_ec);
  }
  if (missing.empty()) {
    return false;
  }

  // Because some of the components of the path pushed on the missing stack may
  // be already existing, we need to check their status at this point and we
  // also need to track if we have ever created any of these components to
  // properly return the correct return value.
  bool created{false};
  do {
    const path &top = missing.top();
    // Check the status of the current component from the top of the stack and
    // create only if it is not found.
    if (status(top, m_ec).type() == file_type::not_found) {
      created = true;
      create_directory(top, m_ec);
    }
    if (m_ec && is_directory(top)) {
      m_ec.clear();
    }
    missing.pop();
  } while (!missing.empty() && !m_ec);

  if (m_ec) {
    return err.report(m_ec);
  }
  return created && missing.empty();
}

auto create_directory_impl(const path &p, std::error_code *ec) -> bool {
  ErrorHandler<bool> err("create_directory", ec, &p);
#if defined(ASAP_WINDOWS)
  auto wpath = p.wstring();
  if (detail::win32_port::CreateDirectoryW(wpath.c_str(), nullptr) != 0) {
    return true;
  }
  if (detail::win32_port::GetLastError() != ERROR_ALREADY_EXISTS) {
    err.report(capture_errno());
  }
#else
  if (detail::posix_port::mkdir(p.c_str(), static_cast<int>(perms::all)) == 0) {
    return true;
  }
  if (errno != EEXIST) {
    err.report(capture_errno());
  }
#endif

  return false;
}

auto create_directory_impl(path const &p, path const &existing_template,
                           std::error_code *ec) -> bool {
  ErrorHandler<bool> err("create_directory", ec, &p, &existing_template);
#if defined(ASAP_WINDOWS)
  auto wpath = p.wstring();
  auto template_wpath = existing_template.wstring();
  if (detail::win32_port::CreateDirectoryExW(template_wpath.c_str(),
                                             wpath.c_str(), nullptr) == 0) {
    if (detail::win32_port::GetLastError() != ERROR_ALREADY_EXISTS) {
      err.report(capture_errno());
    }
  }
  return true;
#else
  StatT attr_stat;
  std::error_code mec;
  auto st =
      detail::posix_port::GetFileStatus(existing_template, attr_stat, &mec);
  if (!status_known(st)) {
    return err.report(mec);
  }
  if (!is_directory(st)) {
    return err.report(std::errc::not_a_directory,
                      "the specified attribute path is invalid");
  }

  if (detail::posix_port::mkdir(p.c_str(), attr_stat.st_mode) == 0) {
    return true;
  }
  if (errno != EEXIST) {
    err.report(capture_errno());
  }
#endif

  return false;
}

// -----------------------------------------------------------------------------
//                             create symlink
// -----------------------------------------------------------------------------

void create_directory_symlink_impl(path const &target, path const &new_symlink,
                                   std::error_code *ec) {
  ErrorHandler<void> err("create_directory_symlink", ec, &target, &new_symlink);
#if defined(ASAP_WINDOWS)
  auto link_wpath = new_symlink.wstring();
  auto target_wpath = target.wstring();
  if (detail::win32_port::CreateSymbolicLinkW(
          link_wpath.c_str(), target_wpath.c_str(),
          SYMBOLIC_LINK_FLAG_DIRECTORY |
              SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0U) {
    return err.report(capture_errno());
  }
#else
  if (detail::posix_port::symlink(target.c_str(), new_symlink.c_str()) != 0) {
    return err.report(capture_errno());
  }
#endif
}

void create_hard_link_impl(const path &target, const path &new_hard_link,
                           std::error_code *ec) {
  ErrorHandler<void> err("create_hard_link", ec, &target, &new_hard_link);
#if defined(ASAP_WINDOWS)
  auto link_wpath = new_hard_link.wstring();
  auto target_wpath = target.wstring();
  if (detail::win32_port::CreateHardLinkW(link_wpath.c_str(),
                                          target_wpath.c_str(), nullptr) == 0) {
    return err.report(capture_errno());
  }
#else
  if (detail::posix_port::link(target.c_str(), new_hard_link.c_str()) == -1) {
    return err.report(capture_errno());
  }
#endif
}

void create_symlink_impl(path const &target, path const &new_symlink,
                         std::error_code *ec) {
  ErrorHandler<void> err("create_symlink", ec, &target, &new_symlink);
#if defined(ASAP_WINDOWS)
  auto link_wpath = new_symlink.wstring();
  auto target_wpath = target.wstring();
  if (detail::win32_port::CreateSymbolicLinkW(
          link_wpath.c_str(), target_wpath.c_str(),
          SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0U) {
    return err.report(capture_errno());
  }
#else
  if (detail::posix_port::symlink(target.c_str(), new_symlink.c_str()) == -1) {
    return err.report(capture_errno());
  }
#endif
}

// -----------------------------------------------------------------------------
//                              current path
// -----------------------------------------------------------------------------

auto current_path_impl(std::error_code *ec) -> path {
  ErrorHandler<path> err("current_path", ec);
#if defined(ASAP_WINDOWS)
  auto size = detail::win32_port::GetCurrentDirectoryW(0, nullptr);
  auto buff = std::unique_ptr<WCHAR[]>(new WCHAR[size + 1]);
  if (detail::win32_port::GetCurrentDirectoryW(
          static_cast<DWORD>(size), reinterpret_cast<LPWSTR>(buff.get())) ==
      0) {
    return err.report(capture_errno(), "call to GetCurrentDirectoryW failed");
  }
  return {buff.get()};
#else
  auto size = detail::posix_port::pathconf(".", _PC_PATH_MAX);
  ASAP_ASSERT(size > 0);
  auto buff = std::unique_ptr<char[]>(new char[static_cast<size_t>(size + 1)]);
  if (detail::posix_port::getcwd(buff.get(), static_cast<size_t>(size)) ==
      nullptr) {
    return err.report(capture_errno(), "call to getcwd failed");
  }
  return {buff.get()};
#endif
}

void current_path_impl(const path &p, std::error_code *ec) {
  ErrorHandler<void> err("current_path", ec, &p);
#if defined(ASAP_WINDOWS)
  auto wpath = p.wstring();
  if (detail::win32_port::SetCurrentDirectoryW(wpath.c_str()) == 0) {
    err.report(capture_errno());
  }
#else
  if (detail::posix_port::chdir(p.c_str()) == -1) {
    err.report(capture_errno());
  }
#endif
}

// -----------------------------------------------------------------------------
//                               equivalent
// -----------------------------------------------------------------------------

auto equivalent_impl(const path &p1, const path &p2, std::error_code *ec)
    -> bool {
  ErrorHandler<bool> err("equivalent", ec, &p1, &p2);
#if defined(ASAP_WINDOWS)
  std::error_code m_ec;
  auto file1 = detail::FileDescriptor::Create(
      &p1, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }
  auto file2 = detail::FileDescriptor::Create(
      &p2, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }

  if (file1.fd_ == detail::win32_port::invalid_handle_value ||
      file2.fd_ == detail::win32_port::invalid_handle_value) {
    // if one is invalid and the other isn't, then they aren't equivalent,
    // but if both are invalid then it is an error
    if (file1.fd_ == detail::win32_port::invalid_handle_value &&
        file2.fd_ == detail::win32_port::invalid_handle_value) {
      return err.report(std::errc::not_supported,
                        "both paths refer to invalid files");
    }
    return false;
  }
  // at this point, both handles are known to be valid

  BY_HANDLE_FILE_INFORMATION info1;
  BY_HANDLE_FILE_INFORMATION info2;

  if (detail::win32_port::GetFileInformationByHandle(file1.fd_, &info1) == 0) {
    return err.report(capture_errno());
  }
  if (detail::win32_port::GetFileInformationByHandle(file2.fd_, &info2) == 0) {
    return err.report(capture_errno());
  }

  // In theory, volume serial numbers are sufficient to distinguish between
  // devices, but in practice VSN's are sometimes duplicated, so last write
  // time and file size are also checked.
  return info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
         info1.nFileIndexHigh == info2.nFileIndexHigh &&
         info1.nFileIndexLow == info2.nFileIndexLow &&
         info1.nFileSizeHigh == info2.nFileSizeHigh &&
         info1.nFileSizeLow == info2.nFileSizeLow &&
         info1.ftLastWriteTime.dwLowDateTime ==
             info2.ftLastWriteTime.dwLowDateTime &&
         info1.ftLastWriteTime.dwHighDateTime ==
             info2.ftLastWriteTime.dwHighDateTime;
#else
  // https://cplusplus.github.io/LWG/issue2937
  // If either p1 or p2 does not exist, an error is reported.
  std::error_code ec1;
  std::error_code ec2;
  StatT st1 = {};
  StatT st2 = {};
  auto s1 = detail::posix_port::GetFileStatus(p1.native(), st1, &ec1);
  if (!exists(s1)) {
    return err.report(std::errc::not_supported);
  }
  auto s2 = detail::posix_port::GetFileStatus(p2.native(), st2, &ec2);
  if (!exists(s2)) {
    return err.report(std::errc::not_supported);
  }

  return stat_equivalent(st1, st2);
#endif
}

// -----------------------------------------------------------------------------
//                               file_size
// -----------------------------------------------------------------------------

auto file_size_impl(const path &p, std::error_code *ec) -> uintmax_t {
  ErrorHandler<uintmax_t> err("file_size", ec, &p);
#if defined(ASAP_WINDOWS)
  WIN32_FILE_ATTRIBUTE_DATA fad;
  auto wpath = p.wstring();
  if (detail::win32_port::GetFileAttributesExW(
          wpath.c_str(), ::GetFileExInfoStandard, &fad) == 0) {
    return err.report(capture_errno());
  }

  if ((fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return err.report(std::errc::is_a_directory,
                      "directory size is not supported");
  }

  return (static_cast<uintmax_t>(fad.nFileSizeHigh)
          << (sizeof(fad.nFileSizeLow) * 8)) +
         fad.nFileSizeLow;
#else
  std::error_code m_ec;
  StatT st;
  file_status fst = detail::posix_port::GetFileStatus(p, st, &m_ec);
  if (!exists(fst) || !is_regular_file(fst)) {
    std::errc error_kind = is_directory(fst) ? std::errc::is_a_directory
                                             : std::errc::not_supported;
    if (!m_ec) {
      m_ec = make_error_code(error_kind);
    }
    return err.report(m_ec);
  }
  // is_regular_file(p) == true
  return static_cast<uintmax_t>(st.st_size);
#endif
}

// -----------------------------------------------------------------------------
//                              hard_link_count
// -----------------------------------------------------------------------------

auto hard_link_count_impl(const path &p, std::error_code *ec) -> uintmax_t {
  ErrorHandler<uintmax_t> err("hard_link_count", ec, &p);

#if defined(ASAP_WINDOWS)
  // Link count info is only available through GetFileInformationByHandle
  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }

  BY_HANDLE_FILE_INFORMATION info;
  if (detail::win32_port::GetFileInformationByHandle(file.fd_, &info) == 0) {
    return err.report(capture_errno());
  }
  return info.nNumberOfLinks;

#else
  std::error_code m_ec;
  StatT st;
  detail::posix_port::GetFileStatus(p, st, &m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }
  return static_cast<uintmax_t>(st.st_nlink);
#endif
}

// -----------------------------------------------------------------------------
//                               fs_is_empty
// -----------------------------------------------------------------------------

namespace {
auto is_empty_directory(const path &p, std::error_code *ec) -> bool {
  auto it = ec != nullptr ? directory_iterator(p, *ec) : directory_iterator(p);
  if ((ec != nullptr) && *ec) {
    return false;
  }
  return it == directory_iterator{};
}
}  // namespace

auto is_empty_impl(const path &p, std::error_code *ec) -> bool {
  ErrorHandler<bool> err("is_empty", ec, &p);

#if defined(ASAP_WINDOWS)
  WIN32_FILE_ATTRIBUTE_DATA fad;
  auto wpath = p.wstring();
  if (detail::win32_port::GetFileAttributesExW(
          wpath.c_str(), ::GetFileExInfoStandard, &fad) == 0) {
    return err.report(capture_errno());
  }
  return (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U
             ? is_empty_directory(p, ec)
             : ((fad.nFileSizeHigh == 0U) && (fad.nFileSizeLow == 0U));
#else
  std::error_code m_ec;
  StatT pst;
  auto st = detail::posix_port::GetFileStatus(p, pst, &m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }
  if (!is_directory(st) && !is_regular_file(st)) {
    return err.report(std::errc::not_supported);
  }
  if (is_directory(st)) {
    return is_empty_directory(p, ec);
  }
  if (is_regular_file(st)) {
    return static_cast<uintmax_t>(pst.st_size) == 0;
  }

  // Unreachable
  ASAP_UNREACHABLE();
#endif
}

// -----------------------------------------------------------------------------
//                               last_write_time
// -----------------------------------------------------------------------------

auto last_write_time_impl(const path &p, std::error_code *ec)
    -> file_time_type {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);
#if defined(ASAP_WINDOWS)
  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, FILE_READ_ATTRIBUTES,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }

  FILETIME lwt;
  if (detail::win32_port::GetFileTime(file.fd_, nullptr, nullptr, &lwt) == 0) {
    return err.report(capture_errno());
  }
  auto ft = detail::win32_port::FileTimeTypeFromWindowsFileTime(lwt, m_ec);
  return (m_ec ? err.report(m_ec) : ft);
#else
  std::error_code m_ec;
  StatT st;
  detail::posix_port::GetFileStatus(p, st, &m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }
  return detail::posix_port::ExtractLastWriteTime(p, st, ec);
#endif
}

void last_write_time_impl(const path &p, file_time_type new_time,
                          std::error_code *ec) {
  ErrorHandler<void> err("last_write_time", ec, &p);

#if defined(ASAP_WINDOWS)

  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, FILE_WRITE_ATTRIBUTES,
      FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }

  auto wt = detail::win32_port::FileTimeTypeToWindowsFileTime(new_time, m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }

  if (detail::win32_port::SetFileTime(file.fd_, nullptr, nullptr, &wt) == 0) {
    return err.report(capture_errno());
  }
#else  // ASAP_WINDOWS
  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;
  using std::chrono::seconds;

  auto d = new_time.time_since_epoch();
  auto s = duration_cast<seconds>(d);

#if ASAP_FS_USE_UTIMENSAT
  auto ns = duration_cast<nanoseconds>(d - s);
  if (ns < std::chrono::duration<std::int64_t, std::ratio<1, 1000000000> >::
               zero())  // tv_nsec must be non-negative and less than 10e9.
  {
    --s;
    ns += seconds(1);
  }
  if (s < std::chrono::duration<std::int64_t>::zero()) {
    return err.report(std::errc::invalid_argument,
                      "negative number of seconds since epoch");
  }
  detail::TimeSpec ts[2];
  ts[0].tv_sec = 0;
  ts[0].tv_nsec = UTIME_OMIT;
  ts[1].tv_sec = static_cast<std::time_t>(s.count());
  ts[1].tv_nsec = static_cast<std::int64_t>(ns.count());
  if (::utimensat(AT_FDCWD, p.c_str(), ts, 0) != 0) {
    err.report(capture_errno());
  }
#elif ASAP_FS_USE_UTIME
  if (s < s.zero())
    return err.report(std::errc::invalid_argument,
                      "negative number of seconds since epoch");
  utimbuf times;
  times.modtime = s.count();
  std::error_code m_ec;
  StatT st;
  detail::posix_port::GetFileStatus(p, st, &m_ec);
  if (m_ec) return err.report(m_ec);
  // The utime call allows time resolution of 1 second
  times.actime = detail::posix_port::ExtractAccessTime(st).tv_sec;
  if (::utime(p.c_str(), &times)) return err.report(capture_errno());
#else
  return err.report(std::errc::not_supported);
#endif
#endif  // ASAP_WINDOWS
}

// -----------------------------------------------------------------------------
//                               permissions
// -----------------------------------------------------------------------------

void permissions_impl(const path &p, perms prms, perm_options opts,
                      std::error_code *ec) {
  ErrorHandler<void> err("permissions", ec, &p);

  auto has_opt = [&](perm_options o) { return bool(o & opts); };
  const bool resolve_symlinks = !has_opt(perm_options::nofollow);
  const bool add_perms = has_opt(perm_options::add);
  const bool remove_perms = has_opt(perm_options::remove);
  ASAP_ASSERT(
      ((add_perms + remove_perms + has_opt(perm_options::replace)) == 1) &&
      "One and only one of the perm_options constants replace, add, or "
      "remove is present in opts");

  bool set_sym_perms = false;
  prms &= perms::mask;
  if (!resolve_symlinks || (add_perms || remove_perms)) {
    std::error_code m_ec;
    file_status st = resolve_symlinks ? status_impl(p, &m_ec)
                                      : symlink_status_impl(p, &m_ec);
    set_sym_perms = is_symlink(st);
    if (m_ec) {
      return err.report(m_ec);
    }
    ASAP_ASSERT((st.permissions() != perms::unknown) &&
                "Permissions unexpectedly unknown");
    if (add_perms) {
      prms |= st.permissions();
    } else if (remove_perms) {
      prms = st.permissions() & ~prms;
    }
  }

#if defined(ASAP_WINDOWS)
  std::error_code m_ec;
  detail::win32_port::SetPermissions(p, prms, set_sym_perms, &m_ec);
  if (m_ec) {
    return err.report(m_ec);
  }
#else  // ASAP_WINDOWS
  const auto real_perms = static_cast< ::mode_t>(prms & perms::mask);

#if defined(AT_SYMLINK_NOFOLLOW) && defined(AT_FDCWD)
  const int flags = set_sym_perms ? AT_SYMLINK_NOFOLLOW : 0;
  if (::fchmodat(AT_FDCWD, p.c_str(), real_perms, flags) == -1) {
    return err.report(capture_errno());
  }
#else
  if (set_sym_perms) return err.report(std::errc::operation_not_supported);
  if (::chmod(p.c_str(), real_perms) == -1) {
    return err.report(capture_errno());
  }
#endif
#endif  // ASAP_WINDOWS
}

// -----------------------------------------------------------------------------
//                               read_symlink
// -----------------------------------------------------------------------------

auto read_symlink_impl(const path &p, std::error_code *ec) -> path {
#if defined(ASAP_WINDOWS)
  return detail::win32_port::ReadSymlinkFromReparsePoint(p, ec);
#else
  ErrorHandler<path> err("read_symlink", ec, &p);

  auto size = detail::posix_port::pathconf(".", _PC_PATH_MAX);
  ASAP_ASSERT(size > 0);
  auto buff = std::unique_ptr<char[]>(new char[static_cast<size_t>(size + 1)]);
  std::error_code m_ec;
  ::ssize_t ret = 0;
  if ((ret = detail::posix_port::readlink(p.c_str(), buff.get(),
                                          static_cast<size_t>(size))) == -1) {
    return err.report(capture_errno());
  }
  ASAP_ASSERT(ret <= size);
  ASAP_ASSERT(ret > 0);
  buff[static_cast<size_t>(ret)] = 0;
  return {buff.get()};
#endif
}

// -----------------------------------------------------------------------------
//                               remove
// -----------------------------------------------------------------------------

auto remove_impl(const path &p, std::error_code *ec) -> bool {
  ErrorHandler<bool> err("remove", ec, &p);

  // If the path is empty, nothing is deleted but no error is reported.
  if (p.empty()) {
    return false;
  }

#if defined(ASAP_WINDOWS)
  // On windows, deleting files uses a different API then directorues.
  // We need to need to check for that manually here as is_directory()
  // uses status() and follows symlinks, and we don't want that here.

  bool is_dir = false;
  auto wpath = p.wstring();
  auto attrs = detail::win32_port::GetFileAttributesW(wpath.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    // Attempting to remove a non-existing path should return false
    // without failing.
    return (detail::win32_port::GetLastError() == ERROR_FILE_NOT_FOUND)
               ? false
               : err.report(capture_errno());
  }
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
    is_dir = true;
  }
  auto status = is_dir ? detail::win32_port::RemoveDirectoryW(wpath.c_str())
                       : detail::win32_port::DeleteFileW(wpath.c_str());
  if ((status == 0) &&
      detail::win32_port::GetLastError() != ERROR_FILE_NOT_FOUND) {
    err.report(capture_errno());
    return false;
  }

#else
  if (detail::posix_port::remove(p.c_str()) == -1) {
    if (errno != ENOENT) {
      err.report(capture_errno());
    }
    return false;
  }
#endif

  return true;
}  // namespace filesystem

// -----------------------------------------------------------------------------
//                               remove_all
// -----------------------------------------------------------------------------

namespace {

auto do_remove_all_impl(path const &p, std::error_code &ec) -> uintmax_t {
  const auto npos = static_cast<uintmax_t>(-1);
  const file_status st = symlink_status_impl(p, &ec);
  if (ec) {
    return npos;
  }
  uintmax_t count = 1;
  if (is_directory(st)) {
    for (directory_iterator it(p, ec); !ec && it != directory_iterator();
         it.increment(ec)) {
      auto other_count = do_remove_all_impl(it->path(), ec);
      if (ec) {
        return npos;
      }
      count += other_count;
    }
    if (ec) {
      return npos;
    }
  }
  if (!remove_impl(p, &ec)) {
    return npos;
  }
  return count;
}

}  // end namespace

auto remove_all_impl(const path &p, std::error_code *ec) -> uintmax_t {
  ErrorHandler<uintmax_t> err("remove_all", ec, &p);

  // If the path is empty, nothing is deleted but no error is reported.
  if (p.empty()) {
    return 0;
  }

  std::error_code mec;
  auto count = do_remove_all_impl(p, mec);
  if (mec) {
    if (mec == std::errc::no_such_file_or_directory) {
      return 0;
    }
    return err.report(mec);
  }
  return count;
}

// -----------------------------------------------------------------------------
//                               rename
// -----------------------------------------------------------------------------

void rename_impl(const path &from, const path &to, std::error_code *ec) {
  ErrorHandler<void> err("rename", ec, &from, &to);
  if (::rename(from.c_str(), to.c_str()) == -1) {
    err.report(capture_errno());
  }
}

// -----------------------------------------------------------------------------
//                               resize_file
// -----------------------------------------------------------------------------

void resize_file_impl(const path &p, uintmax_t size, std::error_code *ec) {
  ErrorHandler<void> err("resize_file", ec, &p);
#if defined(ASAP_WINDOWS)
  std::error_code m_ec;
  auto file = detail::FileDescriptor::Create(
      &p, m_ec, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, nullptr);
  if (m_ec) {
    return err.report(m_ec);
  }
  LARGE_INTEGER pos;
  pos.QuadPart = size;
  if (detail::win32_port::SetFilePointerEx(file.fd_, pos, nullptr,
                                           FILE_BEGIN) == 0) {
    return err.report(capture_errno());
  }
  if (detail::win32_port::SetEndOfFile(file.fd_) == 0) {
    return err.report(capture_errno());
  }
#else
  if (::truncate(p.c_str(), static_cast< ::off_t>(size)) == -1) {
    return err.report(capture_errno());
  }
#endif
}

// -----------------------------------------------------------------------------
//                               space
// -----------------------------------------------------------------------------

auto space_impl(const path &p, std::error_code *ec) -> space_info {
  ErrorHandler<void> err("space", ec, &p);
  space_info si = {static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1),
                   static_cast<uintmax_t>(-1)};

#if defined(ASAP_WINDOWS)
  path dir = absolute(p);
  dir.remove_filename();
  auto pathname = dir.wstring();
  ;
  ULARGE_INTEGER bytes_avail = {};
  ULARGE_INTEGER bytes_total = {};
  ULARGE_INTEGER bytes_free = {};
  if (detail::win32_port::GetDiskFreeSpaceExW(pathname.c_str(), &bytes_avail,
                                              &bytes_total, &bytes_free) != 0) {
    if (bytes_total.QuadPart != 0) {
      si.capacity = bytes_total.QuadPart;
    }
    if (bytes_free.QuadPart != 0) {
      si.free = bytes_free.QuadPart;
    }
    if (bytes_avail.QuadPart != 0) {
      si.available = bytes_avail.QuadPart;
    }
    return si;
  }
  err.report(capture_errno());
  return si;

#else
  struct statvfs m_svfs = {};
  if (::statvfs(p.c_str(), &m_svfs) != -1) {
    // Multiply with overflow checking.
    auto do_mult = [&](uintmax_t &out, uintmax_t other) {
      out = other * m_svfs.f_frsize;
      if (other == 0 || out / other != m_svfs.f_frsize) {
        out = static_cast<uintmax_t>(-1);
      }
    };
    do_mult(si.capacity, m_svfs.f_blocks);
    do_mult(si.free, m_svfs.f_bfree);
    do_mult(si.available, m_svfs.f_bavail);
    return si;
  }
  err.report(capture_errno());
  return si;

#endif
}

// -----------------------------------------------------------------------------
//                               status
// -----------------------------------------------------------------------------

// https://docs.microsoft.com/en-us/windows/desktop/FileIO/symbolic-link-effects-on-file-systems-functions

auto status_impl(const path &p, std::error_code *ec) -> file_status {
#if defined(ASAP_WINDOWS)
  ErrorHandler<void> err("status_impl", ec, &p);

  auto wpath = p.wstring();
  auto attrs = detail::win32_port::GetFileAttributesW(wpath.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return detail::win32_port::ProcessStatusFailure(capture_errno(), p, ec);
  }

  std::error_code m_ec;

  // Handle the case of reparse point.
  // Since GetFileAttributesW does not resolve symlinks, try to open the file
  // handle to discover if it exists.
  if ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
    // Becasue we do not specify the flag FILE_FLAG_OPEN_REPARSE_POINT, symlinks
    // will be followed and the target is opened.
    auto file1 = detail::FileDescriptor::Create(
        &p, m_ec, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (m_ec) {
      return detail::win32_port::ProcessStatusFailure(m_ec, p, ec);
    }
    // Get the file attributes from its handle
    FILE_ATTRIBUTE_TAG_INFO file_info;
    if (detail::win32_port::GetFileInformationByHandleEx(
            file1.fd_, FileAttributeTagInfo, &file_info, sizeof(file_info)) ==
        0) {
      return detail::win32_port::ProcessStatusFailure(capture_errno(), p, ec);
    }
    attrs = file_info.FileAttributes;
  }

  auto prms = detail::win32_port::GetPermissions(p, attrs, true, &m_ec);
  if (m_ec) {
    return detail::win32_port::ProcessStatusFailure(m_ec, p, ec);
  }

  return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0U
             ? file_status(file_type::directory, prms)
             : file_status(file_type::regular, prms);
#else
  StatT path_stat;
  return detail::posix_port::GetFileStatus(p, path_stat, ec);
#endif
}

auto symlink_status_impl(const path &p, std::error_code *ec) -> file_status {
#if defined(ASAP_WINDOWS)
  if (ec != nullptr) {
    ec->clear();
  }
  auto wpath = p.wstring();
  DWORD attr(detail::win32_port::GetFileAttributesW(wpath.c_str()));
  if (attr == INVALID_FILE_ATTRIBUTES) {
    return detail::win32_port::ProcessStatusFailure(capture_errno(), p, ec);
  }

  std::error_code m_ec;
  auto prms = detail::win32_port::GetPermissions(p, attr, false, &m_ec);
  if (m_ec) {
    return detail::win32_port::ProcessStatusFailure(m_ec, p, ec);
  }

  if ((attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0U)
    return detail::win32_port::IsReparsePointSymlink(p, ec)
               ? file_status(file_type::symlink, prms)
               : file_status(file_type::reparse_file, prms);

  return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0U
             ? file_status(file_type::directory, prms)
             : file_status(file_type::regular, prms);
#else
  StatT path_stat;
  return detail::posix_port::GetLinkStatus(p, path_stat, ec);
#endif
}

// -----------------------------------------------------------------------------
//                               temp_directory_path
// -----------------------------------------------------------------------------

/**
 * @brief Returns the directory location suitable for temporary files.
 *
 * On POSIX systems, the path may be the one specified in the environment
 * variables TMPDIR, TMP, TEMP, TEMPDIR, and, if none of them are specified, the
 * path "/tmp" is returned.
 *
 * TMPDIR is the canonical environment variable in Unix and POSIX[1] that should
 * be used to specify a temporary directory for scratch space. Most Unix
 * programs will honor this setting and use its value to denote the scratch area
 * for temporary files instead of the common default of /tmp[2][3] or /var/tmp.
 *
 * Other forms sometimes accepted are TEMP, TEMPDIR and TMP, but these
 * alternatives are used more commonly by non-POSIX operating systems or
 * non-conformant programs.
 *
 * TMPDIR is specified in various Unix and similar standards, e.g. per the
 * Single UNIX Specification.
 *
 * On Windows systems, the path is typically the one returned by GetTempPath
 * function which checks for the existence of environment variables in the
 * following order and uses the first path found: TMP, TEMP, USERPROFILE, the
 * Windows directory.
 *
 * @return A directory suitable for temporary files. When successful, the path
 * is guaranteed to exist and to be a directory.
 *
 * @throws If the std::error_code* parameter is null, throws filesystem_error on
 * underlying OS API errors or if the temp directory path does not exist or is
 * not a directory, constructed with path to be returned as the first path
 * argument and the OS error code as the error code argument. If the
 * std::error_code* parameter is not null, it is set to the OS API error code if
 * an OS API call fails, std::errc::not_a_directory if the temp directory path
 * does not exist or is not a directory, otherwise it is cleared with ec.clear()
 * if no errors occur.
 */
auto temp_directory_path_impl(std::error_code *ec) -> path {
  ErrorHandler<path> err("temp_directory_path", ec);

  path p;
#ifdef ASAP_WINDOWS
  auto buff = std::unique_ptr<WCHAR[]>(new WCHAR[MAX_PATH + 1]);
  auto gtp_ret = detail::win32_port::GetTempPathW(
      static_cast<DWORD>(MAX_PATH), reinterpret_cast<LPWSTR>(buff.get()));
  if (gtp_ret > MAX_PATH || gtp_ret == 0) {
    return err.report(capture_errno(), "call to GetTempPathW failed");
  }

  p = path(buff.get());
#else
  const char *env_paths[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR"};
  const char *ret = nullptr;

  for (auto &ep : env_paths) {
    if ((ret = ::getenv(ep)) != nullptr) {
      break;
    }
  }
  if (ret == nullptr) {
    ret = "/tmp";
  }
  p = path(ret);
#endif

  std::error_code status_ec;
  file_status st = status(p, status_ec);
  if (!status_known(st)) {
    return err.report(status_ec, "cannot access path \"{" + p.string() + "}\"");
  }
  if (!exists(st) || !is_directory(st)) {
    return err.report(std::errc::not_a_directory,
                      "path \"{" + p.string() + "}\" is not a directory");
  }
  return p;
}

// -----------------------------------------------------------------------------
//                               weakly_canonical
// -----------------------------------------------------------------------------

auto weakly_canonical_impl(const path &p, std::error_code *ec) -> path {
  ErrorHandler<path> err("weakly_canonical", ec, &p);

  if (p.empty()) {
    return canonical_impl("", ec);
  }

  path result;
  std::error_code m_ec;
  file_status st = status_impl(p, &m_ec);
  if (!status_known(st)) {
    return err.report(m_ec);
  }
  if (exists(st)) {
    return canonical_impl(p, ec);
  }

  path tmp;
  auto iter = p.begin();
  auto end = p.end();
  // find leading elements of p that exist:
  while (iter != end) {
    tmp = result / *iter;
    st = status_impl(tmp, &m_ec);
    if (exists(st)) {
      swap(result, tmp);
    } else {
      if (status_known(st)) {
        m_ec.clear();
      }
      break;
    }
    ++iter;
  }
  // canonicalize:
  if (!m_ec && !result.empty()) {
    result = canonical_impl(result, &m_ec);
  }
  if (m_ec) {
    result.clear();
  } else {
    // append the non-existing elements:
    while (iter != end) {
      result /= *iter++;
    }
    // normalize:
    result = result.lexically_normal();
  }
  return result;
}

}  // namespace filesystem
}  // namespace asap
