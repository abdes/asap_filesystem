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

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  weakly_canonical
// -----------------------------------------------------------------------------

TEST_CASE("Ops / weakly_canonical",
          "[common][filesystem][ops][weakly_canonical]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  auto dir = testing::nonexistent_path();
  testing::scoped_file sdir(dir, testing::scoped_file::adopt_file);
  fs::create_directory(dir);
  const auto dirc = canonical(dir);
  fs::path foo = dir / "foo";
  fs::path bar = dir / "bar";
  fs::create_directory(foo);
  fs::create_directory(bar);
  fs::create_directory(bar / "baz");
#if defined(ASAP_WINDOWS)
  fs::create_directory_symlink("..\\bar", foo / "bar");
#else
  fs::create_directory_symlink("../bar", foo / "bar");
#endif

  auto p = fs::weakly_canonical(dir / "foo//./bar///../biz/.");
  REQUIRE(p == dirc / "biz/");
  p = fs::weakly_canonical(dir / "foo/.//bar/././baz/.");
  REQUIRE(p == dirc / "bar/baz");
  p = fs::weakly_canonical(fs::current_path() / dir / "bar//../foo/bar/baz");
  REQUIRE(p == dirc / "bar/baz");

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  ec = bad_ec;
  p = fs::weakly_canonical(dir / "foo//./bar///../biz/.", ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "biz/");
  ec = bad_ec;
  p = fs::weakly_canonical(dir / "foo/.//bar/././baz/.", ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "bar/baz");
  ec = bad_ec;
  p = fs::weakly_canonical(fs::current_path() / dir / "bar//../foo/bar/baz",
                           ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "bar/baz");
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
