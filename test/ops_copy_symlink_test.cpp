//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  copy_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / copy_symlink / does not exist",
          "[common][filesystem][ops][copy_symlink]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();

  copy_symlink(from, to, ec);
  REQUIRE(ec);
  REQUIRE(!exists(to));
}

TEST_CASE("Ops / copy_symlink / not symlink",
          "[common][filesystem][ops][copy_symlink]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  testing::scoped_file from_f(from);
  REQUIRE(exists(from));
  REQUIRE(!fs::is_symlink(from));

  auto to = testing::nonexistent_path();

  // If from refers to a non-symlink, copy_symlink should fail with an error
  copy_symlink(from, to, ec);
  REQUIRE(ec);
  REQUIRE(!exists(to));
}

TEST_CASE("Ops / copy_symlink / link to file",
          "[common][filesystem][ops][copy_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  auto from = testing::nonexistent_path();
  testing::scoped_file sfrom(from, testing::scoped_file::adopt_file);
  REQUIRE(!exists(from));
  create_symlink(tgt, from, ec);  // create the symlink
  REQUIRE(!ec);
  REQUIRE(exists(from));
  REQUIRE(is_symlink(from));

  // copy the symlink and check that the copy is a symlink to the same target
  auto to = testing::nonexistent_path();
  testing::scoped_file sto(to, testing::scoped_file::adopt_file);
  copy_symlink(from, to, ec);
  REQUIRE(!ec);
  REQUIRE(exists(to));
  REQUIRE(is_symlink(to));

  auto to_tgt = read_symlink(to, ec);
  REQUIRE(!ec);
  REQUIRE(tgt == to_tgt);
}

TEST_CASE("Ops / copy_symlink / link to directory",
          "[common][filesystem][ops][copy_symlink]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;

  fs::path tgt = testing::nonexistent_path();
  testing::scoped_file stgt(tgt, testing::scoped_file::adopt_file);
  fs::create_directory(tgt);

  auto from = testing::nonexistent_path();
  testing::scoped_file sfrom(from, testing::scoped_file::adopt_file);
  REQUIRE(!exists(from));
  create_directory_symlink(tgt, from, ec);  // create the symlink
  REQUIRE(!ec);
  REQUIRE(exists(from));
  REQUIRE(is_symlink(from));

  // copy the symlink and check that the copy is a symlink to the same target
  auto to = testing::nonexistent_path();
  testing::scoped_file sto(to, testing::scoped_file::adopt_file);
  copy_symlink(from, to, ec);
  REQUIRE(!ec);
  REQUIRE(exists(to));
  REQUIRE(is_symlink(to));

  auto to_tgt = read_symlink(to, ec);
  REQUIRE(!ec);
  REQUIRE(tgt == to_tgt);
}
