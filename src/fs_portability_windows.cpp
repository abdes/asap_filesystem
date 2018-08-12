//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "fs_portability.h"
#include "fs_error.h"


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
  constexpr auto EPOCH_DIFF = 116444736000000000LL;
  ULARGE_INTEGER ulft;
  ulft.HighPart = ft.dwHighDateTime;
  ulft.LowPart = ft.dwLowDateTime;

  using namespace std::chrono;
  using rep = typename file_time_type::rep;
  using fs_duration = typename file_time_type::duration;
  using fs_seconds = duration<rep>;
  using fs_nanoseconds = duration<rep, std::nano>;

  // Convert to EPOCH
  ulft.QuadPart -= EPOCH_DIFF;

  auto secs = ulft.QuadPart / 10000000LL;
  auto nsecs = ulft.QuadPart - secs * 1000000000LL;
  return file_time_type(
      duration_cast<fs_duration>(fs_seconds(secs) + fs_nanoseconds(nsecs)));
}

}  // namespace win32
#endif // ASAP_WINDOWS


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

#endif // ASAP_WINDOWS


}  // namespace detail
}  // namespace filesystem
}  // namespace asap
