//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_portability.h"

#if defined(ASAP_POSIX)

// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {

const FileDescriptor::fd_type FileDescriptor::invalid_value =
    posix_port::invalid_fd_value;

auto FileDescriptor::RefreshStatus(bool follow_symlinks, std::error_code &ec)
    -> file_status {
  // FD must be open and good.
  status_ = file_status{};
  stat_ = {};
  std::error_code m_ec;

  if (follow_symlinks) {
    if (posix_port::stat(name_.c_str(), &stat_) == -1) {
      m_ec = capture_errno();
    }
  } else {
    if (posix_port::lstat(name_.c_str(), &stat_) == -1) {
      m_ec = capture_errno();
    }
  }

  status_ = detail::posix_port::CreateFileStatus(m_ec, name_, stat_, &ec);
  return status_;
}

}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_POSIX
