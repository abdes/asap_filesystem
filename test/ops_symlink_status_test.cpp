//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif  // __clang__

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  symlink_status
// -----------------------------------------------------------------------------

TEST_CASE("Ops / symlink_status / dot",
          "[common][filesystem][ops][symlink_status]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec = bad_ec;
  fs::path dot = ".";

  fs::file_status st1 = fs::symlink_status(dot, ec);
  REQUIRE(!ec);
  REQUIRE(st1.type() == fs::file_type::directory);

  fs::file_status st2 = fs::symlink_status(dot);
  REQUIRE(st2.type() == fs::file_type::directory);

  fs::path link = testing::nonexistent_path();
  testing::scoped_file sfrom(link, testing::scoped_file::adopt_file);
  create_directory_symlink(dot, link);

  st1 = fs::symlink_status(link);
  REQUIRE(st1.type() == fs::file_type::symlink);
  ec = bad_ec;
  st2 = fs::symlink_status(link, ec);
  REQUIRE(!ec);
  REQUIRE(st2.type() == fs::file_type::symlink);
}

TEST_CASE("Ops / symlink_status / non-existing",
          "[common][filesystem][ops][symlink_status]") {
  fs::path p = testing::nonexistent_path();

  std::error_code ec;
  fs::file_status st1 = fs::symlink_status(p, ec);
  REQUIRE(ec);
  REQUIRE(st1.type() == fs::file_type::not_found);

  fs::file_status st2 = fs::symlink_status(p);
  REQUIRE(st2.type() == fs::file_type::not_found);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
