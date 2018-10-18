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

TEST_CASE("Ops / status / directory", "[common][filesystem][ops][status]") {
  fs::path dir = testing::nonexistent_path();
  fs::create_directory(dir);
  testing::scoped_file d(dir, testing::scoped_file::adopt_file);

  std::error_code ec;
  fs::file_status st = fs::status(d.path_, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::directory);

  fs::file_status st2 = fs::status(d.path_);
  REQUIRE(st2.type() == fs::file_type::directory);
}

TEST_CASE("Ops / status / file", "[common][filesystem][ops][status]") {
  fs::path file = testing::nonexistent_path();
  testing::scoped_file f(file);

  std::error_code ec;
  fs::file_status st = fs::status(f.path_, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::regular);

  st = fs::status(f.path_, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::regular);
}

TEST_CASE("Ops / status / symlink", "[common][filesystem][ops][status]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  fs::path tgt = testing::nonexistent_path();
  fs::create_directory(tgt);
  testing::scoped_file d(tgt, testing::scoped_file::adopt_file);
  auto symlink = testing::nonexistent_path();
  create_directory_symlink(tgt, symlink, ec);  // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(symlink));
  REQUIRE(is_symlink(symlink));

  ec = bad_ec;
  fs::file_status st = fs::status(symlink, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::directory);
  remove(symlink);
}

TEST_CASE("Ops / status / no permission", "[common][filesystem][ops][status]") {
  fs::path dir = testing::nonexistent_path();
  fs::create_directory(dir);
  testing::scoped_file d(dir, testing::scoped_file::adopt_file);
  testing::scoped_file f(dir / "file");
  fs::permissions(dir, fs::perms::none);

  std::error_code ec;
  fs::file_status st = fs::status(f.path_, ec);
  REQUIRE(ec.value() == (int)std::errc::permission_denied);
  REQUIRE(st.type() == fs::file_type::none);

  REQUIRE_THROWS_MATCHES(fs::status(f.path_), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, f.path_));

  fs::permissions(dir, fs::perms::owner_all, ec);
}
