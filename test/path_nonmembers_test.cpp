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

using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Non members
// -----------------------------------------------------------------------------

// operator/(const path&, const path&)
// Equivalent to: return path(lhs) /= rhs;

namespace {
void test(const path& lhs, const path& rhs) {
  ComparePaths(lhs / rhs, path(lhs) /= rhs);
}
}  // namespace

TEST_CASE("Path / nonmembers / '/' / special",
          "[common][filesystem][path][nonmembers]") {
  test("/foo/bar", "/foo/");

  test("baz", "baz");
  test("baz/", "baz");
  test("baz", "/foo/bar");
  test("baz/", "/foo/bar");

  test("", "");
  test("", "rel");

  test("dir/", "/file");
  test("dir/", "file");

  // C++17 [fs.path.append] p4
  test("//host", "foo");
  test("//host/", "foo");
  test("foo", "");
  test("foo", "/bar");
  test("foo", "c:/bar");
  test("foo", "c:");
  test("c:", "");
  test("c:foo", "/bar");
  test("foo", "c:\\bar");
}

TEST_CASE("Path / nonmembers / '/' / suite",
          "[common][filesystem][path][nonmembers]") {
  for (path p : TEST_PATHS()) {
    CAPTURE(p);
    for (path q : TEST_PATHS()) {
      CAPTURE(q);
      test(p, q);
    }
  }
}

TEST_CASE("Path / nonmembers / hash_value / sanity",
          "[common][filesystem][path][nonmembers]") {
  REQUIRE(hash_value(path("a//b")) == hash_value(path("a/b")));
  REQUIRE(hash_value(path("a/")) == hash_value(path("a//")));
}

TEST_CASE("Path / nonmembers / hash_value / suite",
          "[common][filesystem][path][nonmembers]") {
  for (path p : TEST_PATHS()) {
    path pp = p.native();
    REQUIRE(hash_value(p) == hash_value(pp));
  }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
