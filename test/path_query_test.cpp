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

using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  QUERY
// -----------------------------------------------------------------------------

TEST_CASE("Path / query / empty", "[common][filesystem][path][query]") {
  for (const std::string& s : TEST_PATHS()) {
    REQUIRE(s.empty() == path(s).empty());
  }
}

TEST_CASE("Path / query / has_extension", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_extension() == !p.extension().empty());
  }
}

TEST_CASE("Path / query / has_filename", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_filename() == !p.filename().empty());
  }
}

TEST_CASE("Path / query / has_parent_path",
          "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_parent_path() == !p.parent_path().empty());
  }
}

TEST_CASE("Path / query / has_relative_path",
          "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_relative_path() == !p.relative_path().empty());
  }
}

TEST_CASE("Path / query / has_root_directory",
          "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_root_directory() == !p.root_directory().empty());
  }
}

TEST_CASE("Path / query / has_root_name", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_root_name() == !p.root_name().empty());
  }
}

TEST_CASE("Path / query / has_root_path", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_root_path() == !p.root_path().empty());
  }
}

TEST_CASE("Path / query / has_stem", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.has_stem() == !p.stem().empty());
  }
}

TEST_CASE("Path / query / is_absolute", "[common][filesystem][path][query]") {
#if !defined(ASAP_WINDOWS)
  REQUIRE(path("/").is_absolute());
  REQUIRE(path("/foo").is_absolute());
  REQUIRE(path("/foo/").is_absolute());
  REQUIRE(path("/foo/bar").is_absolute());
  REQUIRE(path("/foo/bar/").is_absolute());
#endif

  REQUIRE(!path("foo").is_absolute());
  REQUIRE(!path("foo/").is_absolute());
  REQUIRE(!path("foo/bar").is_absolute());
  REQUIRE(!path("foo/bar/").is_absolute());
  REQUIRE(path("//foo").is_absolute());
  REQUIRE(path("//foo/").is_absolute());
  REQUIRE(path("//foo/bar").is_absolute());

#ifdef ASAP_WINDOWS
  REQUIRE(path("/").is_absolute());
  REQUIRE(path("/foo").is_absolute());
  REQUIRE(path("/foo/").is_absolute());
  REQUIRE(path("/foo/bar").is_absolute());
  REQUIRE(path("/foo/bar/").is_absolute());
  REQUIRE(!path("c:").is_absolute());
  REQUIRE(!path("c:foo").is_absolute());
  REQUIRE(!path("c:foo/").is_absolute());
  REQUIRE(!path("c:foo/bar").is_absolute());
  REQUIRE(!path("c:foo/bar/").is_absolute());
  REQUIRE(path("c:/").is_absolute());
  REQUIRE(path("c:/foo").is_absolute());
  REQUIRE(path("c:/foo/").is_absolute());
  REQUIRE(path("c:/foo/bar").is_absolute());
  REQUIRE(path("c:/foo/bar/").is_absolute());
#endif
}

TEST_CASE("Path / query / is_relative", "[common][filesystem][path][query]") {
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.is_relative() == !p.is_absolute());
  }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
