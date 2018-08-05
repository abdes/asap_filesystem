//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

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
