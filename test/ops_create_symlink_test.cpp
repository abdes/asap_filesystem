//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  create_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_symlink / empty",
          "[common][filesystem][ops][create_symlink]") {
  std::error_code ec, ec2;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test empty path.
  fs::path p;
  create_symlink(tgt, p, ec);
  REQUIRE(ec);
  try {
    create_symlink(tgt, p);
  } catch (const fs::filesystem_error& ex) {
    ec2 = ex.code();
    REQUIRE(ex.path1() == tgt);
    REQUIRE(ex.path2() == p);
  }
  REQUIRE(ec2 == ec);
}

TEST_CASE("Ops / create_symlink", "[common][filesystem][ops][create_symlink]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec, ec2;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test non-existent path
  auto p = testing::nonexistent_path();
  REQUIRE(!exists(p));

  ec = bad_ec;
  create_symlink(tgt, p, ec);  // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));
  remove(p);
  create_symlink(tgt, p);  // create the symlink again
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));

  ec.clear();
  create_symlink(tgt, p, ec);  // Try to create existing symlink
  REQUIRE(ec);
  try {
    create_symlink(tgt, p);
  } catch (const fs::filesystem_error& ex) {
    ec2 = ex.code();
    REQUIRE(ex.path1() == tgt);
    REQUIRE(ex.path2() == p);
  }
  REQUIRE(ec2 == ec);

  remove(p);
}
