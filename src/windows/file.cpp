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
    win32_port::invalid_handle_value;

file_status FileDescriptor::RefreshStatus(bool follow_symlinks,
                                          std::error_code &ec) {
  // Get the file attributes from its handle
  FILE_ATTRIBUTE_TAG_INFO file_info;
  if (!detail::win32_port::GetFileInformationByHandleEx(
          fd_, FileAttributeTagInfo, &file_info, sizeof(file_info))) {
    return detail::win32_port::ProcessStatusFailure(capture_errno(), name_,
                                                    &ec);
  } else {
    std::error_code m_ec;
    auto prms = detail::win32_port::GetPermissions(
        name_, file_info.FileAttributes, follow_symlinks, &m_ec);
    if (m_ec) {
      return detail::win32_port::ProcessStatusFailure(m_ec, name_, &ec);
    }
    return (file_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
               ? file_status(file_type::directory, prms)
               : file_status(file_type::regular, prms);
  }
}

}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
