//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if (__clang_major__ >= 13) && !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif // __clang__

#include <catch2/catch.hpp>
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  create_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_symlink / empty", "[common][filesystem][ops][create_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif // ASAP_WINDOWS

  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test empty path.
  fs::path p;
  create_symlink(tgt, p, ec);
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(
      create_symlink(tgt, p), fs::filesystem_error, testing::FilesystemErrorDetail(ec, tgt, p));
}

TEST_CASE("Ops / create_symlink", "[common][filesystem][ops][create_symlink]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif // ASAP_WINDOWS

  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test non-existent path
  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  REQUIRE(!exists(p));

  ec = bad_ec;
  create_symlink(tgt, p, ec); // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));
  remove(p);
  create_symlink(tgt, p); // create the symlink again
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));

  ec.clear();
  create_symlink(tgt, p, ec); // Try to create existing symlink
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(
      create_symlink(tgt, p), fs::filesystem_error, testing::FilesystemErrorDetail(ec, tgt, p));
}

// -----------------------------------------------------------------------------
//  create_directory_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_directory_symlink / empty",
    "[common][filesystem][ops][create_directory_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif // ASAP_WINDOWS
  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test empty path.
  fs::path p;
  create_directory_symlink(tgt, p, ec);
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(create_directory_symlink(tgt, p), fs::filesystem_error,
      testing::FilesystemErrorDetail(ec, tgt, p));
}

TEST_CASE("Ops / create_directory_symlink", "[common][filesystem][ops][create_directory_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled())
    return;
#endif // ASAP_WINDOWS
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  std::error_code ec2;

  fs::path tgt = testing::nonexistent_path();
  fs::create_directory(tgt);
  testing::scoped_file stgt(tgt, testing::scoped_file::adopt_file);

  // Test non-existent path
  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  REQUIRE(!exists(p));

  ec = bad_ec;
  create_directory_symlink(tgt, p, ec); // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));
  remove(p);
  create_directory_symlink(tgt, p); // create the symlink again
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));

  ec.clear();
  create_symlink(tgt, p, ec); // Try to create existing symlink
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(
      create_symlink(tgt, p), fs::filesystem_error, testing::FilesystemErrorDetail(ec, tgt, p));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
