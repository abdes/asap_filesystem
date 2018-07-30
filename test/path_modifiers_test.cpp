//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <filesystem/fs_path.h>
#include "testsuite_fs.h"

using asap::filesystem::path;
using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Modifiers
// -----------------------------------------------------------------------------

TEST_CASE("Path / modifiers / clear", "[common][filesystem][path][modifiers]") {
  for (path p : TEST_PATHS) {
    path empty;
    p.clear();
    REQUIRE(p.empty());
    ComparePaths(p, empty);
  }
}

TEST_CASE("Path / modifiers / make_preferred",
          "[common][filesystem][path][modifiers]") {
#ifdef ASAP_WINDOWS
  REQUIRE(path("foo/bar").make_preferred() == "foo\bar");
#else
  REQUIRE(path("foo/bar").make_preferred() == "foo/bar");
#endif
}

TEST_CASE("Path / modifiers / remove_file_name / special",
          "[common][filesystem][path][modifiers]") {
  ComparePaths(path("foo/bar").remove_filename(), "foo/");
  ComparePaths(path("foo/").remove_filename(), "foo/");
  ComparePaths(path("/foo").remove_filename(), "/");
  ComparePaths(path("/").remove_filename(), "/");
}

TEST_CASE("Path / modifiers / remove_file_name / suite",
          "[common][filesystem][path][modifiers]") {
  for (path p : TEST_PATHS) {
    path p2(p);
    p2.remove_filename();
    p2 /= p.filename();
    ComparePaths(p2, p);
  }
}

TEST_CASE("Path / modifiers / replace_extension / special",
          "[common][filesystem][path][modifiers]") {
  ComparePaths(path("/foo.txt").replace_extension("cpp"), "/foo.cpp");
  ComparePaths(path("/foo.txt").replace_extension(".cpp"), "/foo.cpp");
  ComparePaths(path("/").replace_extension("bar"), "/.bar");
}

TEST_CASE("Path / modifiers / replace_extension / suite",
          "[common][filesystem][path][modifiers]") {
  for (path p : TEST_PATHS) {
    path p2(p);
    ComparePaths(p2.replace_extension(p2.extension()), p);
  }
}

TEST_CASE("Path / modifiers / replace_filename / special",
          "[common][filesystem][path][modifiers]") {
  ComparePaths(path("/foo").replace_filename("bar"), "/bar");
  ComparePaths(path("/").replace_filename("bar"), "/bar");
}

TEST_CASE("Path / modifiers / replace_filename / suite",
          "[common][filesystem][path][modifiers]") {
  for (path p : TEST_PATHS) {
    path p2(p);
    p2.replace_filename(p.filename());
    ComparePaths(p2, p);
  }
}

TEST_CASE("Path / modifiers / swap", "[common][filesystem][path][modifiers]") {
  const path p("/foo/bar");
  path p1;
  path p2 = p;
  p1.swap(p2);
  REQUIRE(p2.empty());
  ComparePaths(p1, p);
}
