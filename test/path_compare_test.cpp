//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
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
//  Compare
// -----------------------------------------------------------------------------

TEST_CASE("Path / compare / basic", "[common][filesystem][path][compare]") {
  path p("/foo/bar");
  REQUIRE(p.compare(p) == 0);
  REQUIRE(p.compare("/foo//bar") == 0);

  path q("/foo/baz");
  REQUIRE(p.compare(q) < 0);
  REQUIRE(q.compare(p) > 0);

  path r("/foo/bar/.");
  REQUIRE(p.compare(r) < 0);

  REQUIRE(path("a/b/").compare("a/b//") == 0);
}

TEST_CASE("Path / compare / path", "[common][filesystem][path][compare]") {
  const path p0 = "/a/a/b/b";
  for (const path p : TEST_PATHS()) {
    REQUIRE(p.compare(p) == 0);
    int cmp = p.compare(p0);
    if (cmp == 0) {
      REQUIRE(p0.compare(p) == 0);
    } else if (cmp < 0) {
      REQUIRE(p0.compare(p) > 0);
    } else if (cmp > 0) {
      REQUIRE(p0.compare(p) < 0);
    }
  }
}

TEST_CASE("Path / compare / strings", "[common][filesystem][path][assign]") {
  const std::string s0 = "/a/a/b/b";
  const path p0 = s0;
  for (const std::string &s : TEST_PATHS()) {
    path p(s);
    REQUIRE(p.compare(s) == 0);
    REQUIRE(p.compare(s.c_str()) == 0);
    REQUIRE(p.compare(p0) == p.compare(s0));
    REQUIRE(p.compare(p0) == p.compare(s0.c_str()));
  }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
