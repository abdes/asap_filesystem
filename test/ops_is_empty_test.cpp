//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  is_empty
// -----------------------------------------------------------------------------

TEST_CASE("Ops / is_empty / permissions",
          "[common][filesystem][ops][is_empty]") {
  auto p = testing::nonexistent_path();
  create_directory(p);
  permissions(p, fs::perms::none);
  std::error_code ec, ec2;

  bool result = fs::is_empty(p, ec);
  REQUIRE(ec == std::make_error_code(std::errc::permission_denied));
  REQUIRE(!result);

  try {
    fs::is_empty(p);
  } catch (const fs::filesystem_error& e) {
    ec2 = e.code();
  }
  REQUIRE(ec2 == ec);

  result = fs::is_empty(p / "f", ec);
  REQUIRE(ec == std::make_error_code(std::errc::permission_denied));
  REQUIRE(!result);

  try {
    fs::is_empty(p / "f");
  } catch (const fs::filesystem_error& e) {
    ec2 = e.code();
  }
  REQUIRE(ec2 == ec);

  permissions(p, fs::perms::owner_all, ec);
  remove_all(p, ec);
}

TEST_CASE("Ops / is_empty", "[common][filesystem][ops][is_empty]") {
  auto p = testing::nonexistent_path();
  create_directory(p);
  std::error_code ec, bad_ec = make_error_code(std::errc::invalid_argument);
  bool empty;

  ec = bad_ec;
  empty = is_empty(p, ec);
  REQUIRE(!ec);
  REQUIRE(empty);
  empty = is_empty(p);
  REQUIRE(empty);

  testing::scoped_file f(p / "f");
  ec = bad_ec;
  empty = is_empty(f.path_, ec);
  REQUIRE(!ec);
  REQUIRE(empty);
  empty = is_empty(f.path_);
  REQUIRE(empty);

  std::ofstream{f.path_} << "data";
  ec = bad_ec;
  empty = is_empty(p, ec);
  REQUIRE(!ec);
  REQUIRE(!empty);
  empty = is_empty(p);
  REQUIRE(!empty);

  ec = bad_ec;
  empty = is_empty(p, ec);
  REQUIRE(!ec);
  REQUIRE(!empty);
  empty = is_empty(p);
  REQUIRE(!empty);

  f.path_.clear();
  remove_all(p, ec);
}
