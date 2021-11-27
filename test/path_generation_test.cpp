//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if (__clang_major__ >= 13) && !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif // __clang__

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Generation - normal
// -----------------------------------------------------------------------------

TEST_CASE("Path / generation / normal", "[common][filesystem][path][generation]") {
  // Empty stays empty
  CHECK(path().lexically_normal() == "");

  // dot
  CHECK(path(".").lexically_normal() == ".");

  // dotdot
  // 7. If the last filename is dot-dot, remove any trailing
  // directory-separator.
  CHECK(path("..").lexically_normal() == "..");
  CHECK(path("../").lexically_normal() == "..");
  CHECK(path("../../").lexically_normal() == "../..");
  CHECK(path(".././../.").lexically_normal() == "../..");
  CHECK(path(".././.././").lexically_normal() == "../..");

  //
  CHECK(path("/").lexically_normal() == "/");
  CHECK(path("//").lexically_normal() == "//");
  CHECK(path("/foo").lexically_normal() == "/foo");
  CHECK(path("/foo/").lexically_normal() == "/foo/");
  CHECK(path("/foo/.").lexically_normal() == "/foo/");
  CHECK(path("/foo/bar/..").lexically_normal() == "/foo/");
  CHECK(path("/foo/..").lexically_normal() == "/");

  CHECK(path("/.").lexically_normal() == "/");
  CHECK(path("/./").lexically_normal() == "/");
  CHECK(path("/./.").lexically_normal() == "/");
  CHECK(path("/././").lexically_normal() == "/");
  CHECK(path("/././.").lexically_normal() == "/");

  // 4. Remove each dot filename and any immediately following
  // directory-separator.
  // 8. If the path is empty, add a dot.
  CHECK(path("./").lexically_normal() == ".");
  CHECK(path("./.").lexically_normal() == ".");
  CHECK(path("././").lexically_normal() == ".");
  CHECK(path("././.").lexically_normal() == ".");
  CHECK(path("./././").lexically_normal() == ".");
  CHECK(path("./././.").lexically_normal() == ".");

  CHECK(path("foo/..").lexically_normal() == ".");
  CHECK(path("foo/../").lexically_normal() == ".");
  CHECK(path("foo/../..").lexically_normal() == "..");

#if defined(ASAP_WINDOWS)
  CHECK(path("c:bar/..").lexically_normal() == "c:");
  CHECK(path("c:").lexically_normal() == "c:");
#endif
  CHECK(path("//host/bar/..").lexically_normal() == "//host/");
  CHECK(path("//host").lexically_normal() == "//host");

  CHECK(path("foo/../foo/..").lexically_normal() == ".");
  CHECK(path("foo/../foo/../..").lexically_normal() == "..");
  CHECK(path("../foo/../foo/..").lexically_normal() == "..");
  CHECK(path("../.f/../f").lexically_normal() == "../f");
  CHECK(path("../f/../.f").lexically_normal() == "../.f");

  // 6) If there is root-directory, remove all dot-dots and any
  // directory-separators immediately following them.
  CHECK(path("/..").lexically_normal() == "/");
  CHECK(path("/../").lexically_normal() == "/");
  CHECK(path("/../foo").lexically_normal() == "/foo");
  CHECK(path("/../..").lexically_normal() == "/");
  CHECK(path("/../../").lexically_normal() == "/");
  CHECK(path("/../../foo/").lexically_normal() == "/foo/");

  CHECK(path("foo/./bar/..").lexically_normal() == "foo/");
  CHECK(path("foo/.///bar/../").lexically_normal() == "foo/");

  CHECK(path("foo/../bar").lexically_normal() == "bar");
  CHECK(path("../foo/../bar").lexically_normal() == "../bar");
  CHECK(path("foo/../").lexically_normal() == ".");

  CHECK(path("./a/b/c/../.././b/c").lexically_normal() == "a/b/c");
  CHECK(path("/a/b/c/../.././b/c").lexically_normal() == "/a/b/c");
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
