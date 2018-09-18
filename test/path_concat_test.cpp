//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Compare
// -----------------------------------------------------------------------------

TEST_CASE("Path / concat / basic", "[common][filesystem][path][concat]") {
  const path p("/foo/bar");

  path pp = p;
  pp += p;
  ComparePaths(pp, "/foo/bar/foo/bar");

  path q("foo/bar");

  path qq = q;
  qq += q;
  ComparePaths(qq, "foo/barfoo/bar");

  q += p;
  ComparePaths(q, "foo/bar/foo/bar");
}

TEST_CASE("Path / concat / path", "[common][filesystem][path][compare]") {
  for (path p : TEST_PATHS) {
    auto prior_native = p.native();
    path x("//blah/di/blah");
    p += x;
    REQUIRE(p.native() == prior_native + x.native());
  }
}

TEST_CASE("Path / concat / strings", "[common][filesystem][path][assign]") {
  path p("/");
  p += "foo";
  REQUIRE(p.filename().generic_string() == "foo");
  p += "bar";
  REQUIRE(p.filename().generic_string() == "foobar");
  p += '/';
  REQUIRE(p.parent_path().generic_string() == "/foobar");
  REQUIRE(p.filename().generic_string() == "");
  p += "baz.txt";
  REQUIRE(p.filename().generic_string() == "baz.txt");
  p.concat("/dir/");
  REQUIRE(p.parent_path() == path("/foobar/baz.txt/dir"));
  REQUIRE(p.filename().generic_string() == "");
  const char file[] = "file";
  std::vector<char> input(file, file + 4);
  p.concat(input.begin(), input.end());
  REQUIRE(p.filename().generic_string() == file);
}
