//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

using time_type = fs::file_time_type;

// -----------------------------------------------------------------------------
//  last_write_time
// -----------------------------------------------------------------------------

TEST_CASE("Ops / last_write_time / read",
          "[common][filesystem][ops][last_write_time]") {
  // read times

  auto p = testing::nonexistent_path();
  std::error_code ec;
  time_type mtime = last_write_time(p, ec);
  REQUIRE(ec);
  REQUIRE(ec == std::make_error_code(std::errc::no_such_file_or_directory));
#if __cpp_exceptions
  bool caught = false;
  try {
    mtime = last_write_time(p);
  } catch (std::system_error const& e) {
    caught = true;
    ec = e.code();
  }
  REQUIRE(caught);
  REQUIRE(ec);
  REQUIRE(ec == std::make_error_code(std::errc::no_such_file_or_directory));
#endif

  testing::scoped_file file(p);
  REQUIRE(exists(p));
  mtime = last_write_time(p, ec);
  REQUIRE(!ec);
  REQUIRE(mtime <= time_type::clock::now());
  REQUIRE(mtime == last_write_time(p));

  auto end_of_time = time_type::duration::max();
  auto last_second =
      std::chrono::duration_cast<std::chrono::seconds>(end_of_time).count();
  if (last_second > std::numeric_limits<std::time_t>::max())
    return;  // can't test overflow

    // TODO change this
#if 1  //_GLIBCXX_USE_UTIMENSAT
  struct ::timespec ts[2];
  ts[0].tv_sec = 0;
  ts[0].tv_nsec = UTIME_NOW;
  ts[1].tv_sec = std::numeric_limits<std::time_t>::max() - 1;
  ts[1].tv_nsec = 0;
  REQUIRE(!::utimensat(AT_FDCWD, p.c_str(), ts, 0));
#elif _GLIBCXX_HAVE_UTIME_H
  ::utimbuf times;
  times.modtime = std::numeric_limits<std::time_t>::max() - 1;
  times.actime = std::numeric_limits<std::time_t>::max() - 1;
  REQUIRE(!::utime(p.string().c_str(), &times));
#else
  return;
#endif

  mtime = last_write_time(p, ec);
// TODO: Check/fix this
#ifdef FAILED_TEST
  REQUIRE(ec);
  REQUIRE(ec == std::make_error_code(std::errc::value_too_large));
  REQUIRE(mtime == time_type::min());
#endif

// TODO: Check/fix this
#ifdef FAILED_TEST
  caught = false;
  try {
    mtime = last_write_time(p);
  } catch (std::system_error const& e) {
    caught = true;
    ec = e.code();
  }
  REQUIRE(caught);
  REQUIRE(ec);
  REQUIRE(ec == std::make_error_code(std::errc::value_too_large));
#endif
}

bool approx_equal(time_type file_time, time_type expected) {
  auto delta = expected - file_time;
  if (delta < delta.zero()) delta = -delta;
  return delta < std::chrono::seconds(1);
}

TEST_CASE("Ops / last_write_time / write",
          "[common][filesystem][ops][last_write_time]") {
  // write times

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  testing::scoped_file f;
  std::error_code ec;
  time_type time;

  time = last_write_time(f.path_);
  ec = bad_ec;
  last_write_time(f.path_, time, ec);
  REQUIRE(!ec);
  REQUIRE(approx_equal(last_write_time(f.path_), time));

  ec = bad_ec;
  time -= std::chrono::milliseconds(1000 * 60 * 10 + 15);
  last_write_time(f.path_, time, ec);
  REQUIRE(!ec);
  REQUIRE(approx_equal(last_write_time(f.path_), time));

  ec = bad_ec;
  time += std::chrono::milliseconds(1000 * 60 * 20 + 15);
  last_write_time(f.path_, time, ec);
  REQUIRE(!ec);
  REQUIRE(approx_equal(last_write_time(f.path_), time));

  ec = bad_ec;
  time = time_type();
  last_write_time(f.path_, time, ec);
  REQUIRE(!ec);
  REQUIRE(approx_equal(last_write_time(f.path_), time));

  ec = bad_ec;
  time -= std::chrono::milliseconds(1000 * 60 * 10 + 15);
  last_write_time(f.path_, time, ec);
  REQUIRE(!ec);
  REQUIRE(approx_equal(last_write_time(f.path_), time));
}