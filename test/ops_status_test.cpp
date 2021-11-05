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
  testing::scoped_file sdir(dir, testing::scoped_file::adopt_file);
  fs::create_directory(dir);

  std::error_code ec;
  fs::file_status st = fs::status(dir, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::directory);

  fs::file_status st2 = fs::status(dir);
  REQUIRE(st2.type() == fs::file_type::directory);
}

TEST_CASE("Ops / status / file", "[common][filesystem][ops][status]") {
  fs::path file = testing::nonexistent_path();
  testing::scoped_file sfile(file);

  std::error_code ec;
  fs::file_status st = fs::status(file, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::regular);

  st = fs::status(file, ec);
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
  testing::scoped_file stgt(tgt, testing::scoped_file::adopt_file);
  fs::create_directory(tgt);
  auto lnk = testing::nonexistent_path();
  testing::scoped_file slnk(lnk, testing::scoped_file::adopt_file);
  create_directory_symlink(tgt, lnk, ec);  // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(lnk));
  REQUIRE(is_symlink(lnk));

  ec = bad_ec;
  fs::file_status st = fs::status(lnk, ec);
  REQUIRE(!ec);
  REQUIRE(st.type() == fs::file_type::directory);
}

TEST_CASE("Ops / status / no-permission", "[common][filesystem][ops][status]") {
  fs::path dir = testing::nonexistent_path();
  testing::scoped_file sdir(dir, testing::scoped_file::adopt_file);
  fs::create_directory(dir);

  permissions(dir, fs::perms::none);

  std::error_code ec;
  fs::status(dir, ec);
  REQUIRE(!ec);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
