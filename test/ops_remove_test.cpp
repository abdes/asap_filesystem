//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
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
//  remove
// -----------------------------------------------------------------------------

TEST_CASE("Ops / remove / nonexistent path",
          "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  auto p = testing::nonexistent_path();
  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(!n);
}

TEST_CASE("Ops / remove / empty path", "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  ec = bad_ec;
  n = fs::remove("", ec);
  REQUIRE(!ec);  // This seems odd, but is what the standard requires.
  REQUIRE(!n);
}

TEST_CASE("Ops / remove / file", "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  auto p = testing::nonexistent_path();
  testing::scoped_file f(p);
  REQUIRE(exists(p));

  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(n);
}

TEST_CASE("Ops / remove / empty directory",
          "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, ec);
  REQUIRE(!ec);
  REQUIRE(exists(p));

  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(n);
}

TEST_CASE("Ops / remove / symlink to nonexistent path",
          "[common][filesystem][ops][remove]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  auto p = testing::nonexistent_path();
  auto link = testing::nonexistent_path();
  create_directory_symlink(p, link);  // dangling symlink
  ec = bad_ec;
  n = remove(link, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(symlink_status(link)));
}

TEST_CASE("Ops / remove / symlink to actual path",
          "[common][filesystem][ops][remove]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  auto p = testing::nonexistent_path();
  auto link = testing::nonexistent_path();

  testing::scoped_file f(p);
  create_symlink(p, link);
  ec = bad_ec;
  n = remove(link, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(symlink_status(link)));  // The symlink is removed, but
  REQUIRE(exists(p));                      // its target is not.
}

TEST_CASE("Ops / remove / non-empty directory",
          "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  const auto dir = testing::nonexistent_path();
  create_directories(dir / "a/b");
  ec.clear();
  n = remove(dir / "a", ec);
  REQUIRE(ec);
  REQUIRE(!n);
  REQUIRE(exists(dir / "a/b"));

  ec = bad_ec;
  n = remove(dir / "a/b", ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(dir / "a/b"));

  remove(dir / "a", ec);
  remove(dir, ec);
}

// FIXME: this is not correct on POSIX
// To remove a file, you need write permissions on the containing directory
// permissions on the file itself are not sued
TEST_CASE("Ops / remove / file permissions",
          "[common][filesystem][ops][remove]") {
#if 0   // FIXME
  //  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n = false;

  const auto p = testing::nonexistent_path();
  testing::scoped_file f(p);
  REQUIRE(exists(p));

  permissions(p, fs::perms::none, ec);
  if (!ec) {
    ec.clear();
    n = remove(p, ec);
    REQUIRE(ec);
    REQUIRE(!n);
    permissions(p, fs::perms::owner_all, ec);
  }

  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(p));
#endif  // FIXME
}

// FIXME: need to add test cases for permission scenarios involving files and
// directories, including permissions on parent directory

// -----------------------------------------------------------------------------
//  remove_all
// -----------------------------------------------------------------------------

TEST_CASE("Ops / remove_all / empty path",
          "[common][filesystem][ops][remove_all]") {
  std::error_code ec;
  std::uintmax_t n = 0;

  n = fs::remove_all("", ec);
  REQUIRE(!ec);  // This seems odd, but is what the standard requires.
  REQUIRE(n == 0);
}

TEST_CASE("Ops / remove_all / nonexistent path",
          "[common][filesystem][ops][remove_all]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::uintmax_t n = 0;

  auto p = testing::nonexistent_path();
  ec = bad_ec;
  n = remove_all(p, ec);
  REQUIRE(!ec);
  REQUIRE(n == 0);
}

TEST_CASE("Ops / remove_all / symlink to nonexistent path",
          "[common][filesystem][ops][remove_all]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::uintmax_t n = 0;

  auto p = testing::nonexistent_path();
  auto link = testing::nonexistent_path();
  create_symlink(p, link);  // dangling symlink
  ec = bad_ec;
  n = remove_all(link, ec);
  REQUIRE(!ec);
  REQUIRE(n == 1);
  REQUIRE(!exists(symlink_status(link)));  // DR 2721
}

TEST_CASE("Ops / remove_all / symlink to actual path",
          "[common][filesystem][ops][remove_all]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::uintmax_t n = 0;

  auto p = testing::nonexistent_path();
  auto link = testing::nonexistent_path();
  testing::scoped_file f(p);
  create_symlink(p, link);
  ec = bad_ec;
  n = remove_all(link, ec);
  REQUIRE(!ec);
  REQUIRE(n == 1);
  REQUIRE(!exists(symlink_status(link)));  // The symlink is removed, but
  REQUIRE(exists(p));                      // its target is not.
}

TEST_CASE("Ops / remove_all / tree", "[common][filesystem][ops][remove_all]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::uintmax_t n = 0;

  const auto dir = testing::nonexistent_path();
  create_directories(dir / "a/b/c");
  ec = bad_ec;
  n = remove_all(dir / "a", ec);
  REQUIRE(!ec);
  REQUIRE(n == 3);
  REQUIRE(exists(dir));
  REQUIRE(!exists(dir / "a"));

  create_directories(dir / "a/b/c");
  testing::scoped_file a1(dir / "a/1");
  testing::scoped_file a2(dir / "a/2");
  testing::scoped_file b1(dir / "a/b/1");
  testing::scoped_file b2(dir / "a/b/2");
  ec = bad_ec;
  n = remove_all(dir, ec);
  REQUIRE(!ec);
  REQUIRE(n == 8);
  REQUIRE(!exists(dir));

  a1.path_.clear();
  a2.path_.clear();
  b1.path_.clear();
  b2.path_.clear();
}

TEST_CASE("Ops / remove_all / multiple levels",
          "[common][filesystem][ops][remove_all]") {
  const auto dir = testing::nonexistent_path();
  create_directories(dir / "a/b/c");
  std::uintmax_t n = remove_all(dir / "a");
  REQUIRE(n == 3);
  REQUIRE(exists(dir));
  REQUIRE(!exists(dir / "a"));

  n = remove_all(dir / "a");
  REQUIRE(n == 0);
  REQUIRE(exists(dir));

  n = remove_all(dir);
  REQUIRE(n == 1);
  REQUIRE(!exists(dir));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
