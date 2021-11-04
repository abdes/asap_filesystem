//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_error.h"
#include "../fs_portability.h"

#if defined(ASAP_POSIX)

// -----------------------------------------------------------------------------
//                            detail: stat
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace posix_port {

file_status CreateFileStatus(std::error_code &m_ec, path const &p,
                             const posix_port::StatT &path_stat,
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

file_status GetFileStatus(path const &p, posix_port::StatT &path_stat,
                          std::error_code *ec) {
  std::error_code m_ec;
  if (detail::posix_port::stat(p.c_str(), &path_stat) == -1)
    m_ec = capture_errno();
  return CreateFileStatus(m_ec, p, path_stat, ec);
}

file_status GetLinkStatus(path const &p, posix_port::StatT &path_stat,
                          std::error_code *ec) {
  std::error_code m_ec;
  if (detail::posix_port::lstat(p.c_str(), &path_stat) == -1)
    m_ec = capture_errno();
  return CreateFileStatus(m_ec, p, path_stat, ec);
}

}  // namespace posix_port
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_POSIX
