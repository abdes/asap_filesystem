//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <common/platform.h>

#include <filesystem/fs_path.h>

using asap::filesystem::path;

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
  REQUIRE(str == u8"abc");
  REQUIRE(str == p.u8string());

  auto strw = p.string<wchar_t>();
  REQUIRE(strw == L"abc");
  REQUIRE(strw == p.wstring());
}
