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

template <typename... Args>
constexpr bool nothrow_constructible() {
  return std::is_nothrow_constructible<fs::file_status, Args...>::value;
}

// -----------------------------------------------------------------------------
//  Constructors have noexcept
// -----------------------------------------------------------------------------

TEST_CASE("Ops / file_status / construction / nothrow",
          "[common][filesystem][file_status]") {
  fs::file_status st0;
  REQUIRE(st0.type() == fs::file_type::none);
  REQUIRE(st0.permissions() == fs::perms::unknown);
  static_assert(nothrow_constructible<>(), "");

  fs::file_status st1(fs::file_type::regular);
  REQUIRE(st1.type() == fs::file_type::regular);
  REQUIRE(st1.permissions() == fs::perms::unknown);
  static_assert(nothrow_constructible<fs::file_type>(), "");

  fs::file_status st2(fs::file_type::directory, fs::perms::owner_all);
  REQUIRE(st2.type() == fs::file_type::directory);
  REQUIRE(st2.permissions() == fs::perms::owner_all);
  static_assert(nothrow_constructible<fs::file_type, fs::perms>(), "");

  static_assert(nothrow_constructible<const fs::file_status&>(), "");
  static_assert(nothrow_constructible<fs::file_status>(), "");
}

// -----------------------------------------------------------------------------
//  Check member initialization during construction
// -----------------------------------------------------------------------------

TEST_CASE("Ops / file_status / construction",
          "[common][filesystem][file_status]") {
  fs::file_status st;
  REQUIRE(st.type() == fs::file_type::none);
  REQUIRE(st.permissions() == fs::perms::unknown);

  st.type(fs::file_type::symlink);
  REQUIRE(st.type() == fs::file_type::symlink);
  REQUIRE(st.permissions() == fs::perms::unknown);

  st.permissions(fs::perms::owner_all);
  REQUIRE(st.type() == fs::file_type::symlink);
  REQUIRE(st.permissions() == fs::perms::owner_all);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
