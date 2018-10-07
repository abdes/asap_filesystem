//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)


#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: FileDescriptor
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {

const FileDescriptor::fd_type FileDescriptor::invalid_value =
    win32::invalid_handle_value;

file_status FileDescriptor::RefreshStatus(std::error_code &ec) {
  // FD must be open and good.
  status_ = file_status{};

  // TODO: implement status refresh for windows

  return status_;
}

}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
