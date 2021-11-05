//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_portability.h"

#if defined(ASAP_POSIX)

// -----------------------------------------------------------------------------
//                            detail: file time utils
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace posix_port {

namespace {
// We know this may create an overflow and also that the tests may always be
// true. This is here for cases where the representation of
// std::chrono::duration is smaller than the TimeSpec representation.
HEDLEY_DIAGNOSTIC_PUSH
#if defined(HEDLEY_GCC_VERSION)
HEDLEY_PRAGMA(GCC diagnostic ignored "-Woverflow")
#endif  // HEDLEY_GCC_VERSION
#if defined(__clang__)
HEDLEY_PRAGMA(clang diagnostic ignored "-Wtautological-type-limit-compare")
#endif  // __clang__
auto FileTimeIsRepresentable(TimeSpec tm) -> bool {
  using std::chrono::nanoseconds;
  using std::chrono::seconds;
  if (tm.tv_sec >= 0) {
    return tm.tv_sec <= seconds::max().count();
  }
  if (tm.tv_sec == (seconds::min().count() - 1)) {
    return tm.tv_nsec >= nanoseconds::min().count();
  }
  return tm.tv_sec >= seconds::min().count();
}
HEDLEY_DIAGNOSTIC_POP
}  // namespace

auto FileTimeTypeFromPosixTimeSpec(TimeSpec tm) -> file_time_type {
  using std::chrono::duration;
  using std::chrono::duration_cast;

  using rep = typename file_time_type::rep;
  using fs_duration = typename file_time_type::duration;
  using fs_seconds = duration<rep>;
  using fs_nanoseconds = duration<rep, std::nano>;

  if (tm.tv_sec >= 0 || tm.tv_nsec == 0) {
    return file_time_type(
        fs_seconds(tm.tv_sec) +
        duration_cast<fs_duration>(fs_nanoseconds(tm.tv_nsec)));
  }  // tm.tv_sec < 0
  auto adj_subsec =
      duration_cast<fs_duration>(fs_seconds(1) - fs_nanoseconds(tm.tv_nsec));
  auto Dur = fs_seconds(tm.tv_sec + 1) - adj_subsec;
  return file_time_type(Dur);
}

auto ExtractLastWriteTime(const path &p, const StatT &st, std::error_code *ec)
    -> file_time_type {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);

  auto ts = detail::posix_port::ExtractModificationTime(st);
  if (!detail::posix_port::FileTimeIsRepresentable(ts)) {
    return err.report(std::errc::value_too_large);
  }
  return detail::posix_port::FileTimeTypeFromPosixTimeSpec(ts);
}

}  // namespace posix_port
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_POSIX
