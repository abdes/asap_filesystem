//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <common/platform.h>

#include <filesystem/path.h>

using asap::filesystem::path;

// -----------------------------------------------------------------------------
//  Generic string format
// -----------------------------------------------------------------------------

TEST_CASE("Path / generic / string", "[common][filesystem][path][generic]") {
  REQUIRE(path().generic_string() == "");
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
