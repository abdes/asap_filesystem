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

namespace {
auto check(fs::space_info const &s) -> bool {
  const auto err = static_cast<uintmax_t>(-1);
  return s.capacity != err || s.free != err || s.available != err;
}
} // namespace

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
  if (ec) {
    REQUIRE(!check(s));
  } else {
    REQUIRE(check(s));
  }
}

TEST_CASE("Ops / space / capacity > free", "[common][filesystem][ops][space]") {
  fs::space_info s = fs::space(".");
  REQUIRE(check(s));
  REQUIRE(s.capacity >= s.free);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
