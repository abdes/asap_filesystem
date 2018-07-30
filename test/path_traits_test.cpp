//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <filesystem/fs_path.h>

using asap::filesystem::path_traits::IsConstructibleFrom;
using asap::filesystem::path_traits::IsEncodedChar;
using asap::filesystem::path_traits::IsPathableIter;

using asap::filesystem::path;

// -----------------------------------------------------------------------------
//  Traits
// -----------------------------------------------------------------------------

TEST_CASE("Path / traits", "[common][filesystem][path]") {
  REQUIRE(IsEncodedChar<wchar_t>::value);
  REQUIRE(IsPathableIter<std::string::iterator>::value);
  REQUIRE(IsConstructibleFrom<std::string, void>::value);
  REQUIRE(
      IsConstructibleFrom<std::string::iterator, std::string::iterator>::value);
  REQUIRE(IsConstructibleFrom<char *, void>::value);

  std::string s = "ABC";
  path p(s);
}

// -----------------------------------------------------------------------------
//  QUERY
// -----------------------------------------------------------------------------
