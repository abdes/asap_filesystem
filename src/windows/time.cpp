//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: file time utils
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace win32_port {

auto FileTimeTypeFromWindowsFileTime(const FILETIME &ft, std::error_code &ec)
    -> file_time_type {
  ec.clear();

  // Contains a 64-bit value representing the number of 100-nanosecond intervals
  // since January 1, 1601 (UTC).
  ULARGE_INTEGER ulft{{0, 0}};
  ulft.HighPart = ft.dwHighDateTime;
  ulft.LowPart = ft.dwLowDateTime;

  // Convert to EPOCH
  constexpr auto EPOCH_DIFF = 11644473600;
  using rep = typename file_time_type::rep;
  using fs_seconds = std::chrono::duration<rep>;
  return file_time_type(fs_seconds(ulft.QuadPart / 10000000 - EPOCH_DIFF));
}

auto FileTimeTypeToWindowsFileTime(const file_time_type &ft,
                                   std::error_code &ec) -> FILETIME {
  ec.clear();

  FILETIME wt{0, 0};
  auto d = ft.time_since_epoch();
  // Negative seconds since epoch is not allowed
  if (d.count() < 0) {
    ec = make_error_code(std::errc::invalid_argument);
    return wt;
  }
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
  std::uint64_t ll = ns.count() / 100 + 116444736000000000LL;
  wt.dwLowDateTime = static_cast<DWORD>(ll);
  wt.dwHighDateTime = ll >> 32;
  return wt;
}

}  // namespace win32_port
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
