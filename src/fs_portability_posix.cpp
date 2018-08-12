//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <chrono>

#include "fs_error.h"
#include "fs_portability.h"

namespace asap {
namespace filesystem {
namespace detail {

// -----------------------------------------------------------------------------
//                          detail: file time utils
// -----------------------------------------------------------------------------

#if defined(ASAP_POSIX)
namespace posix {

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

}  // namespace posix
#endif  // ASAP_POSIX

// -----------------------------------------------------------------------------
//                            detail: posix stat
// -----------------------------------------------------------------------------

#if defined(ASAP_POSIX)
namespace posix {

namespace {

file_status create_file_status(std::error_code &m_ec, path const &p,
                               const posix::StatT &path_stat,
                               std::error_code *ec) {
  if (ec) *ec = m_ec;
  if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
    return file_status(file_type::not_found);
  } else if (m_ec) {
    ErrorHandler<void> err("file_stat", ec, &p);
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

  fs_tmp.permissions(static_cast<perms>(path_stat.st_mode) & perms::mask);
  return fs_tmp;
}

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

}  // namespace

file_status file_stat(path const &p, posix::StatT &path_stat,
                      std::error_code *ec) {
  std::error_code m_ec;
  if (detail::posix::stat(p.c_str(), &path_stat) == -1) m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_status link_stat(path const &p, posix::StatT &path_stat,
                      std::error_code *ec) {
  std::error_code m_ec;
  if (detail::posix::lstat(p.c_str(), &path_stat) == -1) m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_time_type extract_last_write_time(const path &p, const StatT &st,
                                       std::error_code *ec) {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);

  auto ts = detail::posix::extract_mtime(st);
  if (!detail::posix::ft_is_representable(ts))
    return err.report(std::errc::value_too_large);
  return detail::posix::ft_convert_from_timespec(ts);
}

}  // namespace posix
#endif  // ASAP_POSIX


// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

#if defined(ASAP_POSIX)

const FileDescriptor::fd_type FileDescriptor::invalid_value =
    posix::invalid_fd_value;

file_status FileDescriptor::refresh_status(std::error_code &ec) {
  // FD must be open and good.
  status_ = file_status{};
  stat_ = {};
  std::error_code m_ec;
  if (posix::fstat(fd_, &stat_) == -1) m_ec = capture_errno();
  status_ = detail::posix::create_file_status(m_ec, name_, stat_, &ec);
  return status_;
}

#endif  // ASAP_POSIX

}  // namespace detail
}  // namespace filesystem
}  // namespace asap
