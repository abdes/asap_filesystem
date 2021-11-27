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

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  proximate
// -----------------------------------------------------------------------------

TEST_CASE("Ops / proximate", "[common][filesystem][ops][proximate]") {
  ComparePaths(fs::proximate("/a/d", "/a/b/c"), "../../d");
  ComparePaths(fs::proximate("/a/b/c", "/a/d"), "../b/c");
  ComparePaths(fs::proximate("a/b/c", "a"), "b/c");
  ComparePaths(fs::proximate("a/b/c", "a/b/c/x/y"), "../..");
  ComparePaths(fs::proximate("a/b/c", "a/b/c"), ".");
  ComparePaths(fs::proximate("a/b", "c/d"), "../../a/b");

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec = bad_ec;
  ComparePaths(fs::proximate("/a/d", "/a/b/c", ec), "../../d");
  REQUIRE(!ec);
  ec = bad_ec;
  ComparePaths(fs::proximate("/a/b/c", "/a/d", ec), "../b/c");
  REQUIRE(!ec);
  ec = bad_ec;
  ComparePaths(fs::proximate("a/b/c", "a", ec), "b/c");
  REQUIRE(!ec);
  ec = bad_ec;
  ComparePaths(fs::proximate("a/b/c", "a/b/c/x/y", ec), "../..");
  REQUIRE(!ec);
  ec = bad_ec;
  ComparePaths(fs::proximate("a/b/c", "a/b/c", ec), ".");
  REQUIRE(!ec);
  ec = bad_ec;
  ComparePaths(fs::proximate("a/b", "c/d", ec), "../../a/b");
  REQUIRE(!ec);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
