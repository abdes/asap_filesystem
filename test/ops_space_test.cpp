//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

namespace {
bool check(fs::space_info const& s) {
  const std::uintmax_t err = static_cast<uintmax_t>(-1);
  return s.capacity != err || s.free != err || s.available != err;
}
}  // namespace

// -----------------------------------------------------------------------------
//  space
// -----------------------------------------------------------------------------

TEST_CASE("Ops / space", "[common][filesystem][ops][space]") {
  const fs::path root = testing::root_path();
  fs::space_info s = fs::space(root);
  std::error_code ec = make_error_code(std::errc::invalid_argument);
  s = fs::space(root, ec);
  REQUIRE(!ec);
  REQUIRE(check(s));
  REQUIRE(s.capacity >= s.free);

  s = fs::space(testing::nonexistent_path() / ".", ec);
  if (ec)
    REQUIRE(!check(s));
  else
    REQUIRE(check(s));
}

TEST_CASE("Ops / space / capacity > free", "[common][filesystem][ops][space]") {
  fs::space_info s = fs::space(".");
  REQUIRE(check(s));
  REQUIRE(s.capacity >= s.free);
}
