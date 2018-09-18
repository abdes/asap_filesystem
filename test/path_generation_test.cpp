//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Generation - normal
// -----------------------------------------------------------------------------

void ComparePathsNormal(path p, std::string expected) {
#if defined(ASAP_WINDOWS)
  for (auto& c : expected)
    if (c == '/') c = '\\';
#endif
  ComparePaths(p, expected);
}

TEST_CASE("Path / generation / normal",
          "[common][filesystem][path][generation]") {
  // C++17 [fs.path.gen] p2
  ComparePathsNormal(path("foo/./bar/..").lexically_normal(), "foo/");
  ComparePathsNormal(path("foo/.///bar/../").lexically_normal(), "foo/");

  ComparePathsNormal(path("foo/../bar").lexically_normal(), "bar");
  ComparePathsNormal(path("../foo/../bar").lexically_normal(), "../bar");
  ComparePathsNormal(path("foo/../").lexically_normal(), ".");
  ComparePathsNormal(path("../../").lexically_normal(), "../..");
  ComparePathsNormal(path("../").lexically_normal(), "..");
  ComparePathsNormal(path("./").lexically_normal(), ".");
  ComparePathsNormal(path().lexically_normal(), "");

  ComparePathsNormal(path("/..").lexically_normal(), "/");

  // PR libstdc++/82777
  ComparePathsNormal(path("./a/b/c/../.././b/c").lexically_normal(), "a/b/c");
  ComparePathsNormal(path("/a/b/c/../.././b/c").lexically_normal(), "/a/b/c");
}
