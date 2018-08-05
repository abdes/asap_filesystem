//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  remove
// -----------------------------------------------------------------------------

TEST_CASE("Ops / remove", "[common][filesystem][ops][remove]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  bool n;

  n = fs::remove("", ec);
  REQUIRE(!ec);  // This seems odd, but is what the standard requires.
  REQUIRE(!n);

  auto p = testing::nonexistent_path();
  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(!n);

  auto link = testing::nonexistent_path();
  create_symlink(p, link);  // dangling symlink
  ec = bad_ec;
  n = remove(link, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(symlink_status(link)));

  testing::scoped_file f(p);
  create_symlink(p, link);
  ec = bad_ec;
  n = remove(link, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(symlink_status(link)));  // The symlink is removed, but
  REQUIRE(exists(p));                      // its target is not.

  ec = bad_ec;
  n = remove(p, ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(symlink_status(p)));

  const auto dir = testing::nonexistent_path();
  create_directories(dir / "a/b");
  ec.clear();
  n = remove(dir / "a", ec);
  REQUIRE(ec);
  REQUIRE(!n);
  REQUIRE(exists(dir / "a/b"));

  permissions(dir, fs::perms::none, ec);
  if (!ec) {
    ec.clear();
    n = remove(dir / "a/b", ec);
    REQUIRE(ec);
    REQUIRE(!n);
    permissions(dir, fs::perms::owner_all, ec);
  }

  ec = bad_ec;
  n = remove(dir / "a/b", ec);
  REQUIRE(!ec);
  REQUIRE(n);
  REQUIRE(!exists(dir / "a/b"));

  remove(dir / "a", ec);
  remove(dir, ec);
}

// -----------------------------------------------------------------------------
//  remove_all
// -----------------------------------------------------------------------------

TEST_CASE("Ops / remove_all", "[common][filesystem][ops][remove_all]") {
  std::error_code ec;
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::uintmax_t n;

  n = fs::remove_all("", ec);
  REQUIRE(!ec);  // This seems odd, but is what the standard requires.
  REQUIRE(n == 0);

  auto p = testing::nonexistent_path();
  ec = bad_ec;
  n = remove_all(p, ec);
  REQUIRE(!ec);
  REQUIRE(n == 0);

  auto link = testing::nonexistent_path();
  create_symlink(p, link);  // dangling symlink
  ec = bad_ec;
  n = remove_all(link, ec);
  REQUIRE(!ec);
  REQUIRE(n == 1);
  REQUIRE(!exists(symlink_status(link)));  // DR 2721

  testing::scoped_file f(p);
  create_symlink(p, link);
  ec = bad_ec;
  n = remove_all(link, ec);
  REQUIRE(!ec);
  REQUIRE(n == 1);
  REQUIRE(!exists(symlink_status(link)));  // The symlink is removed, but
  REQUIRE(exists(p));                      // its target is not.

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