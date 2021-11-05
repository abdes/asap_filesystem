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

#include <filesystem/fs_path.h>

#include <catch2/catch.hpp>

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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
