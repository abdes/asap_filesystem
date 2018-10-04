//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  symlink_status
// -----------------------------------------------------------------------------

TEST_CASE("Ops / symlink_status / dot",
          "[common][filesystem][ops][symlink_status]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec = bad_ec;
  fs::path dot = ".";

  fs::file_status st1 = fs::symlink_status(dot, ec);
  REQUIRE(!ec);
  REQUIRE(st1.type() == fs::file_type::directory);

  fs::file_status st2 = fs::symlink_status(dot);
  REQUIRE(st2.type() == fs::file_type::directory);

  fs::path link = testing::nonexistent_path();
  create_directory_symlink(dot, link);

  st1 = fs::symlink_status(link);
  REQUIRE(st1.type() == fs::file_type::symlink);
  ec = bad_ec;
  st2 = fs::symlink_status(link, ec);
  REQUIRE(!ec);
  REQUIRE(st2.type() == fs::file_type::symlink);
}

TEST_CASE("Ops / symlink_status / non-existing",
          "[common][filesystem][ops][symlink_status]") {
  fs::path p = testing::nonexistent_path();

  std::error_code ec;
  fs::file_status st1 = fs::symlink_status(p, ec);
  REQUIRE(ec);
  REQUIRE(st1.type() == fs::file_type::not_found);

  fs::file_status st2 = fs::symlink_status(p);
  REQUIRE(st2.type() == fs::file_type::not_found);
}

TEST_CASE("Ops / symlink_status / permissions", "[common][filesystem][ops][symlink_status]") {
  fs::path dir = testing::nonexistent_path();
  fs::create_directory(dir);
  testing::scoped_file d(dir, testing::scoped_file::adopt_file);
  testing::scoped_file f(dir / "file");
  fs::permissions(dir, fs::perms::none);
  auto link = testing::nonexistent_path();
  fs::create_symlink(f.path_, link);
  testing::scoped_file l(link, testing::scoped_file::adopt_file);

  std::error_code ec;
  fs::file_status st = fs::symlink_status(f.path_, ec);
  REQUIRE(ec.value() == (int)std::errc::permission_denied);
  REQUIRE(st.type() == fs::file_type::none);

  st = fs::symlink_status(link, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::symlink);

  REQUIRE_THROWS_MATCHES(
      fs::symlink_status(f.path_), fs::filesystem_error,
      testing::FilesystemErrorDetail(
          std::make_error_code(std::errc::permission_denied), f.path_));

  fs::file_status st2 = symlink_status(link);
  REQUIRE(st2.type() == fs::file_type::symlink);

  fs::permissions(dir, fs::perms::owner_all, ec);
}
