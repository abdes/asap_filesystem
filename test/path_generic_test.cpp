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
//  Generic string format
// -----------------------------------------------------------------------------

TEST_CASE("Path / generic / string", "[common][filesystem][path][generic]") {
  REQUIRE(path().generic_string().empty());
  REQUIRE(path("/").generic_string() == "/");
  REQUIRE(path("////").generic_string() == "/");
#ifdef ASAP_WINDOWS
  REQUIRE(path("//a").generic_string() == "//a");
  REQUIRE(path("//a/").generic_string() == "//a/");
  REQUIRE(path("//a/b").generic_string() == "//a/b");
#else
  REQUIRE(path("//a").generic_string() == "/a");
  REQUIRE(path("//a/").generic_string() == "/a/");
  REQUIRE(path("//a/b").generic_string() == "/a/b");
#endif
  REQUIRE(path("/a//b").generic_string() == "/a/b");
  REQUIRE(path("/a//b/").generic_string() == "/a/b/");
  REQUIRE(path("/a//b//").generic_string() == "/a/b/");
  REQUIRE(path("/a//b//.").generic_string() == "/a/b/.");
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
