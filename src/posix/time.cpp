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
namespace posix {

namespace {
bool FileTimeIsRepresentable(TimeSpec tm) {
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

file_time_type FileTimeTypeFromPosixTimeSpec(TimeSpec tm) {
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

file_time_type ExtractLastWriteTime(const path &p, const StatT &st,
                                       std::error_code *ec) {
  ErrorHandler<file_time_type> err("last_write_time", ec, &p);

  auto ts = detail::posix::ExtractModificationTime(st);
  if (!detail::posix::FileTimeIsRepresentable(ts))
    return err.report(std::errc::value_too_large);
  return detail::posix::FileTimeTypeFromPosixTimeSpec(ts);
}

}  // namespace posix
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_POSIX
