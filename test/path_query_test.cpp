//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <filesystem/path.h>
#include "testsuite_fs.h"

using asap::filesystem::path;
using testing::TEST_PATHS;


// -----------------------------------------------------------------------------
//  QUERY
// -----------------------------------------------------------------------------

TEST_CASE("Path / query / empty", "[common][filesystem][path][query]") {
  for (const std::string& s : TEST_PATHS) {
    REQUIRE( s.empty() == path(s).empty() );
  }
}

TEST_CASE("Path / query / has_extension", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_extension() == !p.extension().empty() );
  }
}

TEST_CASE("Path / query / has_filename", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_filename() == !p.filename().empty() );
  }
}

TEST_CASE("Path / query / has_parent_path", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_parent_path() == !p.parent_path().empty() );
  }
}

TEST_CASE("Path / query / has_relative_path", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_relative_path() == !p.relative_path().empty() );
  }
}

TEST_CASE("Path / query / has_root_directory", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_root_directory() == !p.root_directory().empty() );
  }
}

TEST_CASE("Path / query / has_root_name", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_root_name() == !p.root_name().empty() );
  }
}

TEST_CASE("Path / query / has_root_path", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_root_path() == !p.root_path().empty() );
  }
}

TEST_CASE("Path / query / has_stem", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.has_stem() == !p.stem().empty() );
  }
}

TEST_CASE("Path / query / is_absolute", "[common][filesystem][path][query]") {
#ifdef ASAP_WINDOWS_API
  const bool is_posix = false;
#else
  const bool is_posix = true;
#endif

  REQUIRE( path("/").is_absolute() == is_posix );
  REQUIRE( path("/foo").is_absolute() == is_posix );
  REQUIRE( path("/foo/").is_absolute() == is_posix );
  REQUIRE( path("/foo/bar").is_absolute() == is_posix );
  REQUIRE( path("/foo/bar/").is_absolute() == is_posix );
  REQUIRE( ! path("foo").is_absolute() );
  REQUIRE( ! path("foo/").is_absolute() );
  REQUIRE( ! path("foo/bar").is_absolute() );
  REQUIRE( ! path("foo/bar/").is_absolute() );
  REQUIRE( ! path("c:").is_absolute() );
  REQUIRE( ! path("c:foo").is_absolute() );
  REQUIRE( ! path("c:foo/").is_absolute() );
  REQUIRE( ! path("c:foo/bar").is_absolute() );
  REQUIRE( ! path("c:foo/bar/").is_absolute() );
  REQUIRE( path("c:/").is_absolute() == !is_posix );
  REQUIRE( path("c:/foo").is_absolute() == !is_posix );
  REQUIRE( path("c:/foo/").is_absolute() == !is_posix );
  REQUIRE( path("c:/foo/bar").is_absolute() == !is_posix );
  REQUIRE( path("c:/foo/bar/").is_absolute() == !is_posix );
}

TEST_CASE("Path / query / is_relative", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS) {
    REQUIRE( p.is_relative() == !p.is_absolute() );
  }
}
