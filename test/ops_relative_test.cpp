//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  relative
// -----------------------------------------------------------------------------

TEST_CASE("Ops / relative", "[common][filesystem][ops][relative]") {
  auto p = testing::nonexistent_path();
  auto q = testing::nonexistent_path();

  auto r = relative(p, q);
  REQUIRE(r == ".." / p);

  r = relative(p, p / q);
  REQUIRE(r == "..");

  r = relative(p / q, p);
  REQUIRE(r == q);

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  ec = bad_ec;
  r = relative(p, q, ec);
  REQUIRE(!ec);
  REQUIRE(r == ".." / p);

  ec = bad_ec;
  r = relative(p, p / q, ec);
  REQUIRE(!ec);
  REQUIRE(r == "..");

  ec = bad_ec;
  r = relative(p / q, p, ec);
  REQUIRE(!ec);
  REQUIRE(r == q);
}
