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
//  Native string format
// -----------------------------------------------------------------------------

TEST_CASE("Path / native / native", "[common][filesystem][path][native]") {
  using string_type = std::basic_string<path::value_type>;
  const string_type s{'a', 'b', 'c'};
  path p(s);

  REQUIRE(p.native() == s);
  REQUIRE(p.c_str() == s);
  REQUIRE(static_cast<string_type>(p) == s);

  string_type s2 = p;  // implicit conversion
  REQUIRE(s2 == p.native());
}

TEST_CASE("Path / native / strings", "[common][filesystem][path][native]") {
  const char* s = "abc";
  path p(s);

  auto str = p.string();
  REQUIRE(str == "abc");
  REQUIRE(str == p.u8string());

  auto strw = p.string<wchar_t>();
  REQUIRE(strw == L"abc");
  REQUIRE(strw == p.wstring());
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
