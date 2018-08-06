//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <fcntl.h>  // for ‘O_RDONLY’, ‘O_NONBLOCK’
#include <time.h>   // for struct timespec
#include <array>
#include <cstdio>  // for BUFSIZ

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#if ASAP_FS_USE_UTIME
#  include <utime.h>
#endif

#include <filesystem/config.h>

// Which method to use for copy file
#if ASAP_FS_USE_SENDFILE
#  include <sys/sendfile.h>
#elif ASAP_FS_USE_COPYFILE
#  include <copyfile.h>
#endif

#include <common/assert.h>

#include <filesystem/filesystem.h>

#include "fs_error.h"

namespace asap {
namespace filesystem {

namespace detail {
namespace {

// -----------------------------------------------------------------------------
//                        detail: options bitset
// -----------------------------------------------------------------------------

template <typename Bitmask>
inline bool is_set(Bitmask obj, Bitmask bits) {
  return (obj & bits) != Bitmask::none;
}

// -----------------------------------------------------------------------------
//                            detail: struct ::stat
// -----------------------------------------------------------------------------

// Use typedef instead of using to avoid gcc warning on struct ::stat not
// declaring anything. (alternative is: "using StatT = struct ::stat;" )
typedef struct ::stat StatT;
typedef struct ::timespec TimeSpec;

#if defined(ASAP_APPLE)
TimeSpec extract_mtime(StatT const& st) { return st.st_mtimespec; }
#else
TimeSpec extract_mtime(StatT const& st) { return st.st_mtim; }
#endif

#if ASAP_FS_USE_UTIME
#  if defined(ASAP_APPLE)
TimeSpec extract_atime(StatT const& st) { return st.st_atimespec; }
#  else
TimeSpec extract_atime(StatT const& st) { return st.st_atim; }
#  endif
#endif

// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

struct FileDescriptor {
  const path& name_;
  int fd_ = -1;
  StatT stat_;
  file_status status_;

  template <class... Args>
  static FileDescriptor create(const path* p, std::error_code& ec,
                               Args... args) {
    ec.clear();
    int fd;
    if ((fd = ::open(p->c_str(), args...)) == -1) {
      ec = capture_errno();
      return FileDescriptor{p};
    }
    return FileDescriptor(p, fd);
  }

  template <class... Args>
  static FileDescriptor create_with_status(const path* p, std::error_code& ec,
                                           Args... args) {
    FileDescriptor fd = create(p, ec, args...);
    if (!ec) fd.refresh_status(ec);

    return fd;
  }

  file_status get_status() const { return status_; }
  StatT const& get_stat() const { return stat_; }

  file_status refresh_status(std::error_code& ec);

  void close() noexcept {
    if (fd_ != -1) ::close(fd_);
    fd_ = -1;
  }

  FileDescriptor(FileDescriptor&& other) noexcept
      : name_(other.name_),
        fd_(other.fd_),
        stat_(other.stat_),
        status_(other.status_) {
    other.fd_ = -1;
    other.status_ = file_status{};
  }

  ~FileDescriptor() { close(); }

  FileDescriptor() = delete;
  FileDescriptor(FileDescriptor const&) = delete;
  FileDescriptor& operator=(FileDescriptor const&) = delete;

 private:
  explicit FileDescriptor(const path* p, int fd = -1) : name_(*p), fd_(fd) {}
};

file_status create_file_status(std::error_code& m_ec, path const& p,
                               const StatT& path_stat, std::error_code* ec);

file_status FileDescriptor::refresh_status(std::error_code& ec) {
  // FD must be open and good.
  status_ = file_status{};
  stat_ = {};
  std::error_code m_ec;
  if (::fstat(fd_, &stat_) == -1) m_ec = capture_errno();
  status_ = detail::create_file_status(m_ec, name_, stat_, &ec);
  return status_;
}

// -----------------------------------------------------------------------------
//                            detail: posix stat
// -----------------------------------------------------------------------------

perms posix_get_perms(const StatT& st) noexcept {
  return static_cast<perms>(st.st_mode) & perms::mask;
}

::mode_t posix_convert_perms(perms prms) {
  return static_cast< ::mode_t>(prms & perms::mask);
}

file_status create_file_status(std::error_code& m_ec, path const& p,
                               const StatT& path_stat, std::error_code* ec) {
  if (ec) *ec = m_ec;
  if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
    return file_status(file_type::not_found);
  } else if (m_ec) {
    ErrorHandler<void> err("posix_stat", ec, &p);
    err.report(m_ec, "failed to determine attributes for the specified path");
    return file_status(file_type::none);
  }
  // else

  file_status fs_tmp;
  auto const mode = path_stat.st_mode;
  if (S_ISLNK(mode))
    fs_tmp.type(file_type::symlink);
  else if (S_ISREG(mode))
    fs_tmp.type(file_type::regular);
  else if (S_ISDIR(mode))
    fs_tmp.type(file_type::directory);
  else if (S_ISBLK(mode))
    fs_tmp.type(file_type::block);
  else if (S_ISCHR(mode))
    fs_tmp.type(file_type::character);
  else if (S_ISFIFO(mode))
    fs_tmp.type(file_type::fifo);
  else if (S_ISSOCK(mode))
    fs_tmp.type(file_type::socket);
  else
    fs_tmp.type(file_type::unknown);

  fs_tmp.permissions(posix_get_perms(path_stat));
  return fs_tmp;
}

file_status posix_stat(path const& p, StatT& path_stat, std::error_code* ec) {
  std::error_code m_ec;
  if (::stat(p.c_str(), &path_stat) == -1) m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_status posix_stat(path const& p, std::error_code* ec) {
  StatT path_stat;
  return posix_stat(p, path_stat, ec);
}

file_status posix_lstat(path const& p, StatT& path_stat, std::error_code* ec) {
  std::error_code m_ec;
  if (::lstat(p.c_str(), &path_stat) == -1) m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_status posix_lstat(path const& p, std::error_code* ec) {
  StatT path_stat;
  return posix_lstat(p, path_stat, ec);
}

bool posix_ftruncate(const FileDescriptor& fd, size_t to_size,
                     std::error_code& ec) {
  if (::ftruncate(fd.fd_, to_size) == -1) {
    ec = capture_errno();
    return true;
  }
  ec.clear();
  return false;
}

bool posix_fchmod(const FileDescriptor& fd, const StatT& st,
                  std::error_code& ec) {
  if (::fchmod(fd.fd_, st.st_mode) == -1) {
    ec = capture_errno();
    return true;
  }
  ec.clear();
  return false;
}

bool stat_equivalent(const StatT& st1, const StatT& st2) {
  return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino);
}

}  // namespace
}  // namespace detail

using detail::capture_errno;
using detail::ErrorHandler;
using detail::FileDescriptor;
using detail::StatT;

// -----------------------------------------------------------------------------
//                               absolute
// -----------------------------------------------------------------------------

static path do_absolute_impl(const path& p, path* cwd, std::error_code* ec) {
  if (ec) ec->clear();
  if (p.is_absolute()) return p;
  *cwd = current_path_impl(ec);
  if (ec && *ec) return {};
  return (*cwd) / p;
}

path absolute_impl(const path& p, std::error_code* ec) {
  path cwd;
  return do_absolute_impl(p, &cwd, ec);
}

// -----------------------------------------------------------------------------
//                               canonical
// -----------------------------------------------------------------------------

path canonical_impl(path const& orig_p, std::error_code* ec) {
  path cwd;
  ErrorHandler<path> err("canonical", ec, &orig_p, &cwd);

  path pa = do_absolute_impl(orig_p, &cwd, ec);

#ifndef ASAP_WINDOWS
  // Use OpenGroup realpath()
  char buff[PATH_MAX + 1];
  char* ret;
  if ((ret = ::realpath(pa.c_str(), buff)) == nullptr)
    return err.report(capture_errno());
  return {ret};
#else
  // TODO: windows implementation of canonical()
  return err.report(std::errc::not_supported);
#endif
}

// -----------------------------------------------------------------------------
//                             copy
// -----------------------------------------------------------------------------

void copy_impl(const path& from, const path& to, copy_options options,
               std::error_code* ec) {
  ErrorHandler<void> err("copy", ec, &from, &to);

  const bool skip_symlinks =
      detail::is_set(options, copy_options::skip_symlinks);
  const bool create_symlinks =
      detail::is_set(options, copy_options::create_symlinks);
  const bool copy_symlinks =
      detail::is_set(options, copy_options::copy_symlinks);

  std::error_code m_ec1;
  StatT f_st = {};
  // For the souce file, if it's a symlink and the copy_options require to act
  // on the symlink rather than the file pointed to, then use lstat
  bool use_lstat = create_symlinks || skip_symlinks || copy_symlinks;
  const file_status f = use_lstat ? detail::posix_lstat(from, f_st, &m_ec1)
                                  : detail::posix_stat(from, f_st, &m_ec1);
  if (m_ec1) return err.report(m_ec1);

  StatT t_st = {};
  use_lstat = create_symlinks || skip_symlinks;
  const file_status t = use_lstat ? detail::posix_lstat(to, t_st, &m_ec1)
                                  : detail::posix_stat(to, t_st, &m_ec1);
  if (not status_known(t)) return err.report(m_ec1);

  if (!exists(f) || is_other(f) || is_other(t) ||
      (is_directory(f) && is_regular_file(t)) ||
      detail::stat_equivalent(f_st, t_st)) {
    return err.report(std::errc::function_not_supported);
  }

  if (ec) ec->clear();

  if (is_symlink(f)) {
    if (skip_symlinks) {
      // do nothing
    } else if (not exists(t)) {
      copy_symlink_impl(from, to, ec);
    } else {
      return err.report(std::errc::file_exists);
    }
    return;
  } else if (is_regular_file(f)) {
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
  } else if (is_directory(f) && create_symlinks) {
    return err.report(std::errc::is_a_directory);
  } else if (is_directory(f) &&
             (detail::is_set(options, copy_options::recursive) ||
              copy_options::none == options)) {
    if (!exists(t)) {
      // create directory to with attributes from 'from'.
      create_directory_impl(to, from, ec);
      if (ec && *ec) {
        return;
      }
    }
    directory_iterator it =
        ec ? directory_iterator(from, *ec) : directory_iterator(from);
    if (ec && *ec) {
      return;
    }
    std::error_code m_ec2;
    for (; it != directory_iterator(); it.increment(m_ec2)) {
      if (m_ec2) {
        return err.report(m_ec2);
      }
      copy_impl(it->path(), to / it->path().filename(), options, ec);
      if (ec && *ec) {
        return;
      }
    }
  }
}

namespace {

#if ASAP_FS_USE_SENDFILE
// NOTE:
// The LINUX method using sendfile() has a major problem in that it can not
// copy files more than 2GB in size!
// http://man7.org/linux/man-pages/man2/sendfile.2.html
//
//    sendfile() will transfer at most 0x7ffff000 (2,147,479,552) bytes,
//    returning the number of bytes actually transferred. (This is true on
//    both 32-bit and 64-bit systems.)
bool do_copy_file_sendfile(FileDescriptor& read_fd, FileDescriptor& write_fd,
                           std::error_code& ec) {
  size_t count = read_fd.get_stat().st_size;
  do {
    ssize_t res;
    if ((res = ::sendfile(write_fd.fd, read_fd.fd, nullptr, count)) == -1) {
      ec = capture_errno();
      return false;
    }
    count -= res;
  } while (count > 0);

  ec.clear();

  return true;
}
#elif ASAP_FS_USE_COPYFILE
bool do_copy_file_copyfile(FileDescriptor& read_fd, FileDescriptor& write_fd,
                           std::error_code& ec) {
  struct CopyFileState {
    copyfile_state_t state;
    CopyFileState() { state = copyfile_state_alloc(); }
    ~CopyFileState() { copyfile_state_free(state); }

   private:
    CopyFileState(CopyFileState const&) = delete;
    CopyFileState& operator=(CopyFileState const&) = delete;
  };

  CopyFileState cfs;
  if (fcopyfile(read_fd.fd_, write_fd.fd_, cfs.state, COPYFILE_DATA) < 0) {
    ec = capture_errno();
    return false;
  }

  ec.clear();
  return true;
}
#endif

#if !ASAP_FS_USE_SENDFILE && !ASAP_FS_USE_COPYFILE
bool do_copy_file_default(FileDescriptor& read_fd, FileDescriptor& write_fd,
                          std::error_code& ec) {
  // BUFSIZ defaults to 8192
  // BUFSIZ of  means one chareter at time
  // good values should fit to blocksize, like 1024 or 4096
  // higher values reduce number of system calls
  // constexpr size_t BUFFER_SIZE = 4096;
  char buf[BUFSIZ];
  ssize_t size;

  while ((size = ::read(read_fd.fd_, buf, BUFSIZ)) > 0) {
    if (::write(write_fd.fd_, buf, static_cast<size_t>(size)) < 0) {
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
#endif

bool do_copy_file(FileDescriptor& from, FileDescriptor& to,
                  std::error_code& ec) {
#if ASAP_FS_USE_SENDFILE
  return do_copy_file_sendfile(from, to, ec);
#elif ASAP_FS_USE_COPYFILE
  return do_copy_file_copyfile(from, to, ec);
#else
  return do_copy_file_default(from, to, ec);
#endif
}

}  // namespace

bool copy_file_impl(const path& from, const path& to, copy_options options,
                    std::error_code* ec) {
  ErrorHandler<bool> err("copy_file", ec, &to, &from);

  std::error_code m_ec;
  FileDescriptor from_fd =
      FileDescriptor::create_with_status(&from, m_ec, O_RDONLY | O_NONBLOCK);
  if (m_ec) return err.report(m_ec);

  auto from_st = from_fd.get_status();
  StatT const& from_stat = from_fd.get_stat();
  if (!is_regular_file(from_st)) {
    if (not m_ec) m_ec = make_error_code(std::errc::not_supported);
    return err.report(m_ec);
  }

  const bool skip_existing = bool(copy_options::skip_existing & options);
  const bool update_existing = bool(copy_options::update_existing & options);
  const bool overwrite_existing =
      bool(copy_options::overwrite_existing & options);

  StatT to_stat_path;
  file_status to_st = detail::posix_stat(to, to_stat_path, &m_ec);
  if (!status_known(to_st)) return err.report(m_ec);

  const bool to_exists = exists(to_st);
  if (to_exists && !is_regular_file(to_st))
    return err.report(std::errc::not_supported);

  if (to_exists && detail::stat_equivalent(from_stat, to_stat_path))
    return err.report(std::errc::file_exists);

  if (to_exists && skip_existing) return false;

  bool ShouldCopy = [&]() {
    if (to_exists && update_existing) {
      auto from_time = detail::extract_mtime(from_stat);
      auto to_time = detail::extract_mtime(to_stat_path);
      if (from_time.tv_sec < to_time.tv_sec) return false;
      if (from_time.tv_sec == to_time.tv_sec &&
          from_time.tv_nsec <= to_time.tv_nsec)
        return false;
      return true;
    }
    if (!to_exists || overwrite_existing) return true;
    return err.report(std::errc::file_exists);
  }();
  if (!ShouldCopy) return false;

  // Don't truncate right away. We may not be opening the file we originally
  // looked at; we'll check this later.
  int to_open_flags = O_WRONLY;
  if (!to_exists) to_open_flags |= O_CREAT;
  FileDescriptor to_fd = FileDescriptor::create_with_status(
      &to, m_ec, to_open_flags, from_stat.st_mode);
  if (m_ec) return err.report(m_ec);

  if (to_exists) {
    // Check that the file we initially stat'ed is equivalent to the one
    // we opened.
    // FIXME: report this better.
    if (!detail::stat_equivalent(to_stat_path, to_fd.get_stat()))
      return err.report(std::errc::bad_file_descriptor);

    // Set the permissions and truncate the file we opened.
    if (detail::posix_fchmod(to_fd, from_stat, m_ec)) return err.report(m_ec);
    if (detail::posix_ftruncate(to_fd, 0, m_ec)) return err.report(m_ec);
  }

  if (!do_copy_file(from_fd, to_fd, m_ec)) {
    // FIXME: Remove the dest file if we failed, and it didn't exist previously.
    return err.report(m_ec);
  }

  return true;
}

void copy_symlink_impl(const path& existing_symlink, const path& new_symlink,
                       std::error_code* ec) {
  const path real_path(read_symlink_impl(existing_symlink, ec));
  if (ec && *ec) {
    return;
  }
  // NOTE: proposal says you should detect if you should call
  // create_symlink or create_directory_symlink. I don't think this
  // is needed with POSIX
  create_symlink_impl(real_path, new_symlink, ec);
}

// -----------------------------------------------------------------------------
//                             create directory
// -----------------------------------------------------------------------------

bool create_directories_impl(const path& p, std::error_code* ec) {
  ErrorHandler<bool> err("create_directories", ec, &p);

  std::error_code m_ec;
  auto const st = detail::posix_stat(p, &m_ec);
  if (!status_known(st))
    return err.report(m_ec);
  else if (is_directory(st))
    return false;
  else if (exists(st))
    return err.report(std::errc::file_exists);

  const path parent = p.parent_path();
  if (!parent.empty()) {
    const file_status parent_st = status(parent, m_ec);
    if (not status_known(parent_st)) return err.report(m_ec);
    if (not exists(parent_st)) {
      create_directories_impl(parent, ec);
      if (ec && *ec) {
        return false;
      }
    }
  }
  return create_directory_impl(p, ec);
}

bool create_directory_impl(const path& p, std::error_code* ec) {
  ErrorHandler<bool> err("create_directory", ec, &p);

  if (::mkdir(p.c_str(), static_cast<int>(perms::all)) == 0) return true;
  if (errno != EEXIST) err.report(capture_errno());
  return false;
}

bool create_directory_impl(path const& p, path const& attributes,
                           std::error_code* ec) {
  ErrorHandler<bool> err("create_directory", ec, &p, &attributes);

  StatT attr_stat;
  std::error_code mec;
  auto st = detail::posix_stat(attributes, attr_stat, &mec);
  if (!status_known(st)) return err.report(mec);
  if (!is_directory(st))
    return err.report(std::errc::not_a_directory,
                      "the specified attribute path is invalid");

  if (::mkdir(p.c_str(), attr_stat.st_mode) == 0) return true;
  if (errno != EEXIST) err.report(capture_errno());
  return false;
}

// -----------------------------------------------------------------------------
//                             create symlink
// -----------------------------------------------------------------------------

void create_directory_symlink_impl(path const& from, path const& to,
                                   std::error_code* ec) {
  ErrorHandler<void> err("create_directory_symlink", ec, &from, &to);
  if (::symlink(from.c_str(), to.c_str()) != 0)
    return err.report(capture_errno());
}

void create_hard_link_impl(const path& from, const path& to,
                           std::error_code* ec) {
  ErrorHandler<void> err("create_hard_link", ec, &from, &to);
  if (::link(from.c_str(), to.c_str()) == -1)
    return err.report(capture_errno());
}

void create_symlink_impl(path const& from, path const& to,
                         std::error_code* ec) {
  ErrorHandler<void> err("create_symlink", ec, &from, &to);
  if (::symlink(from.c_str(), to.c_str()) == -1)
    return err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                              current path
// -----------------------------------------------------------------------------

path current_path_impl(std::error_code* ec) {
  ErrorHandler<path> err("current_path", ec);

  auto size = ::pathconf(".", _PC_PATH_MAX);
  ASAP_ASSERT(size > 0);

  auto buff = std::unique_ptr<char[]>(new char[size + 1]);
  if (::getcwd(buff.get(), static_cast<size_t>(size)) == nullptr)
    return err.report(capture_errno(), "call to getcwd failed");

  return {buff.get()};
}

void current_path_impl(const path& p, std::error_code* ec) {
  ErrorHandler<void> err("current_path", ec, &p);
  if (::chdir(p.c_str()) == -1) err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                               equivalent
// -----------------------------------------------------------------------------

bool equivalent_impl(const path& p1, const path& p2, std::error_code* ec) {
  ErrorHandler<bool> err("equivalent", ec, &p1, &p2);

  // https://cplusplus.github.io/LWG/issue2937
  // If either p1 or p2 does not exist, an error is reported.
  std::error_code ec1, ec2;
  StatT st1 = {}, st2 = {};
  auto s1 = detail::posix_stat(p1.native(), st1, &ec1);
  if (!exists(s1)) return err.report(std::errc::not_supported);
  auto s2 = detail::posix_stat(p2.native(), st2, &ec2);
  if (!exists(s2)) return err.report(std::errc::not_supported);

  return detail::stat_equivalent(st1, st2);
}

// -----------------------------------------------------------------------------
//                               file_size
// -----------------------------------------------------------------------------

uintmax_t file_size_impl(const path& p, std::error_code* ec) {
  ErrorHandler<uintmax_t> err("file_size", ec, &p);

  std::error_code m_ec;
  StatT st;
  file_status fst = detail::posix_stat(p, st, &m_ec);
  if (!exists(fst) || !is_regular_file(fst)) {
    std::errc error_kind = is_directory(fst) ? std::errc::is_a_directory
                                             : std::errc::not_supported;
    if (!m_ec) m_ec = make_error_code(error_kind);
    return err.report(m_ec);
  }
  // is_regular_file(p) == true
  return static_cast<uintmax_t>(st.st_size);
}

// -----------------------------------------------------------------------------
//                              hard_link_count
// -----------------------------------------------------------------------------

uintmax_t hard_link_count_impl(const path& p, std::error_code* ec) {
  ErrorHandler<uintmax_t> err("hard_link_count", ec, &p);

  std::error_code m_ec;
  StatT st;
  detail::posix_stat(p, st, &m_ec);
  if (m_ec) return err.report(m_ec);
  return static_cast<uintmax_t>(st.st_nlink);
}

// -----------------------------------------------------------------------------
//                               fs_is_empty
// -----------------------------------------------------------------------------

bool is_empty_impl(const path &p, std::error_code *ec) {
  ErrorHandler<bool> err("is_empty", ec, &p);

  std::error_code m_ec;
  StatT pst;
  auto st = detail::posix_stat(p, pst, &m_ec);
  if (m_ec)
    return err.report(m_ec);
  else if (!is_directory(st) && !is_regular_file(st))
    return err.report(std::errc::not_supported);
  else if (is_directory(st)) {
    auto it = ec ? directory_iterator(p, *ec) : directory_iterator(p);
    if (ec && *ec) return false;
    return it == directory_iterator{};
  } else if (is_regular_file(st))
    return static_cast<uintmax_t>(pst.st_size) == 0;

  // Unreachable
  ASAP_UNREACHABLE();
}

// -----------------------------------------------------------------------------
//                               last_write_time
// -----------------------------------------------------------------------------

namespace detail {
namespace {
bool ft_is_representable(TimeSpec tm) {
  using namespace std::chrono;
  if (tm.tv_sec >= 0) {
    return tm.tv_sec < seconds::max().count() ||
           (tm.tv_sec == seconds::max().count() &&
            tm.tv_nsec <= nanoseconds::max().count());
  } else if (tm.tv_sec == (seconds::min().count() - 1)) {
    return tm.tv_nsec >= nanoseconds::min().count();
  } else {
    return tm.tv_sec >= seconds::min().count();
  }
}

file_time_type ft_convert_from_timespec(TimeSpec tm) {
  using namespace std::chrono;
  using rep = typename file_time_type::rep;
  using fs_duration = typename file_time_type::duration;
  using fs_seconds = duration<rep>;
  using fs_nanoseconds = duration<rep, std::nano>;

  if (tm.tv_sec >= 0 || tm.tv_nsec == 0) {
    return file_time_type(
        fs_seconds(tm.tv_sec) +
        duration_cast<fs_duration>(fs_nanoseconds(tm.tv_nsec)));
  } else {  // tm.tv_sec < 0
    auto adj_subsec =
        duration_cast<fs_duration>(fs_seconds(1) - fs_nanoseconds(tm.tv_nsec));
    auto Dur = fs_seconds(tm.tv_sec + 1) - adj_subsec;
    return file_time_type(Dur);
  }
}

}  // namespace
}  // namespace detail

static file_time_type extract_last_write_time(const path& p, const StatT& st,
                                              std::error_code* ec) {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);

  auto ts = detail::extract_mtime(st);

  if (!detail::ft_is_representable(ts))
    return err.report(std::errc::value_too_large);

  return detail::ft_convert_from_timespec(ts);
}

file_time_type last_write_time_impl(const path& p, std::error_code* ec) {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);

  std::error_code m_ec;
  StatT st;
  detail::posix_stat(p, st, &m_ec);
  if (m_ec) return err.report(m_ec);
  return extract_last_write_time(p, st, ec);
}

void last_write_time_impl(const path& p, file_time_type new_time,
                          std::error_code* ec) {
  ErrorHandler<void> err("last_write_time", ec, &p);

  using namespace std::chrono;

  auto d = new_time.time_since_epoch();
  auto s = duration_cast<seconds>(d);

#if ASAP_FS_USE_UTIMENSAT
  auto ns = duration_cast<nanoseconds>(d - s);
  if (ns < ns.zero())  // tv_nsec must be non-negative and less than 10e9.
  {
    --s;
    ns += seconds(1);
  }
  detail::TimeSpec ts[2];
  ts[0].tv_sec = 0;
  ts[0].tv_nsec = UTIME_OMIT;
  ts[1].tv_sec = static_cast<std::time_t>(s.count());
  ts[1].tv_nsec = static_cast<long>(ns.count());
  if (::utimensat(AT_FDCWD, p.c_str(), ts, 0)) err.report(capture_errno());
#elif ASAP_FS_USE_UTIME
  utimbuf times;
  times.modtime = s.count();
  std::error_code m_ec;
  StatT st;
  detail::posix_stat(p, st, &m_ec);
  if (m_ec) return err.report(m_ec);
  // The utime call allows time resolution of 1 second
  times.actime = detail::extract_atime(st).tv_sec;
  if (::utime(p.c_str(), &times)) return err.report(capture_errno());
#else
  return err.report(std::errc::not_supported);
#endif
}

// -----------------------------------------------------------------------------
//                               permissions
// -----------------------------------------------------------------------------

void permissions_impl(const path& p, perms prms, perm_options opts,
                      std::error_code* ec) {
  ErrorHandler<void> err("permissions", ec, &p);

  auto has_opt = [&](perm_options o) { return bool(o & opts); };
  const bool resolve_symlinks = !has_opt(perm_options::nofollow);
  const bool add_perms = has_opt(perm_options::add);
  const bool remove_perms = has_opt(perm_options::remove);
  ASAP_ASSERT(
      ((add_perms + remove_perms + has_opt(perm_options::replace)) == 1) &&
      "One and only one of the perm_options constants replace, add, or remove "
      "is present in opts");

  bool set_sym_perms = false;
  prms &= perms::mask;
  if (!resolve_symlinks || (add_perms || remove_perms)) {
    std::error_code m_ec;
    file_status st = resolve_symlinks ? detail::posix_stat(p, &m_ec)
                                      : detail::posix_lstat(p, &m_ec);
    set_sym_perms = is_symlink(st);
    if (m_ec) return err.report(m_ec);
    ASAP_ASSERT((st.permissions() != perms::unknown) &&
                "Permissions unexpectedly unknown");
    if (add_perms)
      prms |= st.permissions();
    else if (remove_perms)
      prms = st.permissions() & ~prms;
  }
  const auto real_perms = detail::posix_convert_perms(prms);

#if defined(AT_SYMLINK_NOFOLLOW) && defined(AT_FDCWD)
  const int flags = set_sym_perms ? AT_SYMLINK_NOFOLLOW : 0;
  if (::fchmodat(AT_FDCWD, p.c_str(), real_perms, flags) == -1) {
    return err.report(capture_errno());
  }
#else
  if (set_sym_perms) return err.report(errc::operation_not_supported);
  if (::chmod(p.c_str(), real_perms) == -1) {
    return err.report(capture_errno());
  }
#endif
}

// -----------------------------------------------------------------------------
//                               read_symlink
// -----------------------------------------------------------------------------

path read_symlink_impl(const path& p, std::error_code* ec) {
  ErrorHandler<path> err("read_symlink", ec, &p);

  char buff[PATH_MAX + 1];
  std::error_code m_ec;
  ::ssize_t ret;
  if ((ret = ::readlink(p.c_str(), buff, PATH_MAX)) == -1) {
    return err.report(capture_errno());
  }
  ASAP_ASSERT(ret <= PATH_MAX);
  ASAP_ASSERT(ret > 0);
  buff[ret] = 0;
  return {buff};
}

// -----------------------------------------------------------------------------
//                               remove
// -----------------------------------------------------------------------------

bool remove_impl(const path& p, std::error_code* ec) {
  ErrorHandler<bool> err("remove", ec, &p);
  if (::remove(p.c_str()) == -1) {
    if (errno != ENOENT) err.report(capture_errno());
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
//                               remove_all
// -----------------------------------------------------------------------------

namespace {

uintmax_t do_remove_all_impl(path const& p, std::error_code& ec) {
  const auto npos = static_cast<uintmax_t>(-1);
  const file_status st = symlink_status_impl(p, &ec);
  if (ec) return npos;
  uintmax_t count = 1;
  if (is_directory(st)) {
    for (directory_iterator it(p, ec); !ec && it != directory_iterator();
         it.increment(ec)) {
      auto other_count = do_remove_all_impl(it->path(), ec);
      if (ec) return npos;
      count += other_count;
    }
    if (ec) return npos;
  }
  if (!remove_impl(p, &ec)) return npos;
  return count;
}

}  // end namespace

uintmax_t remove_all_impl(const path& p, std::error_code* ec) {
  ErrorHandler<uintmax_t> err("remove_all", ec, &p);

  std::error_code mec;
  auto count = do_remove_all_impl(p, mec);
  if (mec) {
    if (mec == std::errc::no_such_file_or_directory) return 0;
    return err.report(mec);
  }
  return count;
}

// -----------------------------------------------------------------------------
//                               rename
// -----------------------------------------------------------------------------

void rename_impl(const path& from, const path& to, std::error_code* ec) {
  ErrorHandler<void> err("rename", ec, &from, &to);
  if (::rename(from.c_str(), to.c_str()) == -1) err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                               resize_file
// -----------------------------------------------------------------------------

void resize_file_impl(const path& p, uintmax_t size, std::error_code* ec) {
  ErrorHandler<void> err("resize_file", ec, &p);
  if (::truncate(p.c_str(), static_cast< ::off_t>(size)) == -1)
    return err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                               space
// -----------------------------------------------------------------------------

space_info space_impl(const path& p, std::error_code* ec) {
  ErrorHandler<void> err("space", ec, &p);
  space_info si;
  struct statvfs m_svfs = {};
  if (::statvfs(p.c_str(), &m_svfs) == -1) {
    err.report(capture_errno());
    si.capacity = si.free = si.available = static_cast<uintmax_t>(-1);
    return si;
  }
  // Multiply with overflow checking.
  auto do_mult = [&](uintmax_t& out, uintmax_t other) {
    out = other * m_svfs.f_frsize;
    if (other == 0 || out / other != m_svfs.f_frsize)
      out = static_cast<uintmax_t>(-1);
  };
  do_mult(si.capacity, m_svfs.f_blocks);
  do_mult(si.free, m_svfs.f_bfree);
  do_mult(si.available, m_svfs.f_bavail);
  return si;
}

// -----------------------------------------------------------------------------
//                               status
// -----------------------------------------------------------------------------

file_status status_impl(const path& p, std::error_code* ec) {
  return detail::posix_stat(p, ec);
}

file_status symlink_status_impl(const path& p, std::error_code* ec) {
  return detail::posix_lstat(p, ec);
}

// -----------------------------------------------------------------------------
//                               temp_directory_path
// -----------------------------------------------------------------------------

path temp_directory_path_impl(std::error_code* ec) {
  ErrorHandler<path> err("temp_directory_path", ec);

  path p;
#ifdef ASAP_WINDOWS
  // TODO: Windows implementation of temp_directory_path_impl
  // GetTempPathW
  // GetLastError
  return err.report(std::errc::not_supported);
#else
  const char* env_paths[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR"};
  const char* ret = nullptr;

  for (auto& ep : env_paths)
    if ((ret = ::getenv(ep))) break;
  if (ret == nullptr) ret = "/tmp";
  p = path(ret);
#endif

  std::error_code status_ec;
  file_status st = status(p, status_ec);
  if (!status_known(st))
    return err.report(status_ec, "cannot access path \"{}\"", p.string());

  if (!exists(st) || !is_directory(st))
    return err.report(std::errc::not_a_directory,
                      "path \"{}\" is not a directory", p.string());

  return p;
}

// -----------------------------------------------------------------------------
//                               weakly_canonical
// -----------------------------------------------------------------------------

path weakly_canonical_impl(const path& p, std::error_code* ec) {
  ErrorHandler<path> err("weakly_canonical", ec, &p);

  if (p.empty()) return canonical_impl("", ec);

  path result;
  std::error_code m_ec;
  file_status st = status_impl(p, &m_ec);
  if (!status_known(st)) {
    return err.report(m_ec);
  } else if (exists(st)) {
    return canonical_impl(p, ec);
  }

  path tmp;
  auto iter = p.begin(), end = p.end();
  // find leading elements of p that exist:
  while (iter != end) {
    tmp = result / *iter;
    st = status_impl(tmp, &m_ec);
    if (exists(st))
      swap(result, tmp);
    else {
      if (status_known(st)) m_ec.clear();
      break;
    }
    ++iter;
  }
  // canonicalize:
  if (!m_ec && !result.empty()) result = canonical_impl(result, &m_ec);
  if (m_ec)
    result.clear();
  else {
    // append the non-existing elements:
    while (iter != end) result /= *iter++;
    // normalize:
    result = result.lexically_normal();
  }
  return result;
}


// -----------------------------------------------------------------------------
//                           directory entry definitions
// -----------------------------------------------------------------------------

#ifndef ASAP_WINDOWS
std::error_code directory_entry::Refresh_impl() noexcept {
  cached_data_.Reset();
  std::error_code failure_ec;

  StatT full_st;
  file_status st = detail::posix_lstat(path_, full_st, &failure_ec);
  if (!status_known(st)) {
    cached_data_.Reset();
    return failure_ec;
  }

  if (!asap::filesystem::exists(st) || !asap::filesystem::is_symlink(st)) {
    cached_data_.cache_type = CacheType_::REFRESH_NON_SYMLINK;
    cached_data_.type = st.type();
    cached_data_.non_symlink_perms = st.permissions();
  } else { // we have a symlink
    cached_data_.symlink_perms = st.permissions();
    // Get the information about the linked entity.
    // Ignore errors from stat, since we don't want errors regarding symlink
    // resolution to be reported to the user.
    std::error_code ignored_ec;
    st = detail::posix_stat(path_, full_st, &ignored_ec);

    cached_data_.type = st.type();
    cached_data_.non_symlink_perms = st.permissions();

    // If we failed to resolve the link, then only partially populate the
    // cache.
    if (!status_known(st)) {
      cached_data_.cache_type = CacheType_::REFRESH_SYMLINK_UNRESOLVED;
      return std::error_code{};
    }
    // Otherwise, we resolved the link, potentially as not existing.
    // That's OK.
    cached_data_.cache_type = CacheType_::REFRESH_SYMLINK;
  }

  if (asap::filesystem::is_regular_file(st))
    cached_data_.size = static_cast<uintmax_t>(full_st.st_size);

  if (asap::filesystem::exists(st)) {
    cached_data_.nlink = static_cast<uintmax_t>(full_st.st_nlink);

    // Attempt to extract the mtime, and fail if it's not representable using
    // file_time_type. For now we ignore the error, as we'll report it when
    // the value is actually used.
    std::error_code ignored_ec;
    cached_data_.write_time =
        extract_last_write_time(path_, full_st, &ignored_ec);
  }

  return failure_ec;
}
#else
std::error_code directory_entry::__do_refresh() noexcept {
  cached_data_.__reset();
  std::error_code failure_ec;

  file_status st = asap::filesystem::symlink_status(path_, failure_ec);
  if (!status_known(st)) {
    cached_data_.__reset();
    return failure_ec;
  }

  if (!asap::filesystem::exists(st) || !asap::filesystem::is_symlink(st)) {
    cached_data_.cache_type = directory_entry::_RefreshNonSymlink;
    cached_data_.type = st.type();
    cached_data_.non_symlink_perms = st.permissions();
  } else { // we have a symlink
    cached_data_.symlink_perms = st.permissions();
    // Get the information about the linked entity.
    // Ignore errors from stat, since we don't want errors regarding symlink
    // resolution to be reported to the user.
    std::error_code ignored_ec;
    st = asap::filesystem::status(path_, ignored_ec);

    cached_data_.type = st.type();
    cached_data_.non_symlink_perms = st.permissions();

    // If we failed to resolve the link, then only partially populate the
    // cache.
    if (!status_known(st)) {
      cached_data_.cache_type = directory_entry::_RefreshSymlinkUnresolved;
      return std::error_code{};
    }
    cached_data_.cache_type = directory_entry::_RefreshSymlink;
  }

  // FIXME: This is currently broken, and the implementation only a placeholder.
  // We need to cache last_write_time, file_size, and hard_link_count here before
  // the implementation actually works.

  return failure_ec;
}
#endif

}  // namespace filesystem
}  // namespace asap
