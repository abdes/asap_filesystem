//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#pragma clang diagnostic ignored "-Wreserved-identifier"
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif  // __clang__

#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>

#include <catch2/catch.hpp>
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

  REQUIRE_THROWS_MATCHES(
      last_write_time(p), fs::filesystem_error,
      testing::FilesystemErrorDetail(
          std::make_error_code(std::errc::no_such_file_or_directory), p));

  testing::scoped_file file(p);
  REQUIRE(exists(p));
  mtime = last_write_time(p, ec);
  REQUIRE(!ec);
  REQUIRE(mtime <= time_type::clock::now());
  REQUIRE(mtime == last_write_time(p));
}

namespace {
bool approx_equal(time_type file_time, time_type expected) {
  auto delta = expected - file_time;
  if (delta < delta.zero()) delta = -delta;
  return delta < std::chrono::seconds(1);
}
}  // namespace

TEST_CASE("Ops / last_write_time / write",
          "[common][filesystem][ops][last_write_time]") {
  // write times
  auto p = testing::nonexistent_path();

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  testing::scoped_file f(p);
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
  REQUIRE(ec);  // negative seconds since epoch is invalid
  REQUIRE(approx_equal(last_write_time(f.path_), time_type()));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
