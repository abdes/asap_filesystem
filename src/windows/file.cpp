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
  // Get the file attributes from its handle
  FILE_ATTRIBUTE_TAG_INFO file_info;
  if (!detail::win32::GetFileInformationByHandleEx(
          fd_, FileAttributeTagInfo, &file_info, sizeof(file_info))) {
    return detail::win32::ProcessStatusFailure(capture_errno(), name_, &ec);
  } else {
    return (file_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
               ? file_status(file_type::directory,
                             detail::win32::MakePermissions(
                                 name_, file_info.FileAttributes))
               : file_status(file_type::regular,
                             detail::win32::MakePermissions(
                                 name_, file_info.FileAttributes));
  }
}

}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
