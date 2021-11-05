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
//  Append
// -----------------------------------------------------------------------------

TEST_CASE("Ops / current_path / ec", "[common][filesystem][ops]") {
  fs::path cwd = fs::current_path();
  std::error_code ec;
  fs::path cwd2 = fs::current_path(ec);
  REQUIRE(cwd == cwd2);
}

TEST_CASE("Ops / current_path", "[common][filesystem][ops]") {
  auto oldwd = fs::current_path();
  auto tmpdir = fs::temp_directory_path();
  current_path(tmpdir);
  REQUIRE(canonical(fs::current_path()) == canonical(tmpdir));
  std::error_code ec;
  current_path(oldwd, ec);
  REQUIRE(canonical(fs::current_path()) == canonical(oldwd));
  REQUIRE(canonical(fs::current_path(ec)) == canonical(oldwd));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
