//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  read_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / read_symlink / dir",
          "[common][filesystem][ops][read_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  auto p = testing::nonexistent_path();
  std::error_code ec;

  fs::read_symlink(p, ec);
  REQUIRE(ec);

  fs::path tgt = ".";
  fs::create_directory_symlink(tgt, p);

  auto result = read_symlink(p, ec);
  REQUIRE(!ec);
  REQUIRE(result == tgt);

  fs::remove(p);
}

TEST_CASE("Ops / read_symlink / file",
          "[common][filesystem][ops][read_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  auto p = testing::nonexistent_path();
  std::error_code ec;

  fs::read_symlink(p, ec);
  REQUIRE(ec);

#if defined(ASAP_WINDOWS)
  fs::path tgt = ".\\symlink-target";
#else
  fs::path tgt = "./symlink-target";
#endif
  fs::create_symlink(tgt, p);

  auto result = read_symlink(p, ec);
  REQUIRE(!ec);
  REQUIRE(result == tgt);

  fs::remove(p);
}
