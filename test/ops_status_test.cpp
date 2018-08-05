//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  status
// -----------------------------------------------------------------------------

TEST_CASE("Ops / status / dot", "[common][filesystem][ops][status]") {
  std::error_code ec = make_error_code(std::errc::invalid_argument);
  fs::path dot = ".";

  fs::file_status st1 = fs::status(dot, ec);
  REQUIRE(!ec);
  REQUIRE(st1.type() == fs::file_type::directory);

  fs::file_status st2 = fs::status(dot);
  REQUIRE(st2.type() == fs::file_type::directory);
}

TEST_CASE("Ops / status / non-existing", "[common][filesystem][ops][status]") {
  fs::path p = testing::nonexistent_path();

  std::error_code ec;
  fs::file_status st1 = fs::status(p, ec);
  REQUIRE(ec);
  REQUIRE(st1.type() == fs::file_type::not_found);

  fs::file_status st2 = fs::status(p);
  REQUIRE(st2.type() == fs::file_type::not_found);
}

TEST_CASE("Ops / status", "[common][filesystem][ops][status]") {
  fs::path dir = testing::nonexistent_path();
  fs::create_directory(dir);
  testing::scoped_file d(dir, testing::scoped_file::adopt_file);
  testing::scoped_file f(dir / "file");
  fs::permissions(dir, fs::perms::none);

  std::error_code ec;
  fs::file_status st = fs::status(f.path_, ec);
  REQUIRE(ec.value() == (int)std::errc::permission_denied);
  REQUIRE(st.type() == fs::file_type::none);

  bool caught = false;
  std::error_code ec2;
  fs::path p, p2;
  try {
    fs::symlink_status(f.path_);
  } catch (const fs::filesystem_error& e) {
    caught = true;
    p = e.path1();
    p2 = e.path2();
    ec2 = e.code();
  }
  REQUIRE(caught);
  REQUIRE(ec2 == ec);
  REQUIRE(p == f.path_);
  REQUIRE(p2.empty());

  fs::permissions(dir, fs::perms::owner_all, ec);
}
