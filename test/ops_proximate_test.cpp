//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

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
