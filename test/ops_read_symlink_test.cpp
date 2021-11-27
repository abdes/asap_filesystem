//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if (__clang_major__ >= 13) && !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif // __clang__

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  read_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / read_symlink / dir", "[common][filesystem][ops][read_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif

  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  std::error_code ec;

  fs::read_symlink(p, ec);
  REQUIRE(ec);

  fs::path tgt = ".";
  fs::create_directory_symlink(tgt, p);

  auto result = read_symlink(p, ec);
  REQUIRE(!ec);
  REQUIRE(result == tgt);
}

TEST_CASE("Ops / read_symlink / file", "[common][filesystem][ops][read_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif

  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  std::error_code ec;

  fs::read_symlink(p, ec);
  REQUIRE(ec);

#if defined(ASAP_WINDOWS)
  fs::path tgt = ".\\symlink-target";
#else
  fs::path tgt = "./symlink-target";
#endif
  fs::create_symlink(tgt, p);

  auto result = read_symlink(p, ec);
  REQUIRE(!ec);
  REQUIRE(result == tgt);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
