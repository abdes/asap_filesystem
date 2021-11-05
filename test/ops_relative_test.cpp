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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
