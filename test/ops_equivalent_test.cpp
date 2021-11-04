//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  equivalent
// -----------------------------------------------------------------------------

TEST_CASE("Ops / equivalent", "[common][filesystem][ops][equivalent]") {
  auto p1 = testing::nonexistent_path();
  auto p2 = testing::nonexistent_path();
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  bool result;

  result = equivalent(p1, p2, ec);
  REQUIRE(ec);
  REQUIRE(!result);

  testing::scoped_file f1(p1);
  ec.clear();
  result = equivalent(p1, p2, ec);
  REQUIRE(ec);  // https://cplusplus.github.io/LWG/issue2937
  REQUIRE(!result);

  testing::scoped_file f2(p2);
  ec.clear();
  result = equivalent(p1, p2, ec);
  REQUIRE(!ec);
  REQUIRE(!result);

  auto p3 = testing::nonexistent_path();
  create_hard_link(p1, p3, ec);
  if (ec) return;  // hard links not supported
  testing::scoped_file f3(p3, testing::scoped_file::adopt_file);

  ec = bad_ec;
  result = equivalent(p1, p3, ec);
  REQUIRE(!ec);
  REQUIRE(result);

  ec = bad_ec;
  result = equivalent(p2, p3, ec);
  REQUIRE(!ec);
  REQUIRE(!result);
}
