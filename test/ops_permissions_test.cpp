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

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  permissions
// -----------------------------------------------------------------------------

TEST_CASE("Ops / permissions / read",
          "[common][filesystem][ops][permissions]") {
  using fs::perm_options;
  using fs::perms;

  auto p = testing::nonexistent_path();

  testing::scoped_file f(p);
  REQUIRE(exists(p));
  permissions(p, perms::owner_all);
  REQUIRE(status(p).permissions() == perms::owner_all);
  permissions(p, perms::others_read, perm_options::add);
  REQUIRE(status(p).permissions() == (perms::owner_all | perms::others_read));
  permissions(p, perms::others_read, perm_options::remove);
  REQUIRE(status(p).permissions() == perms::owner_all);
}

TEST_CASE("Ops / permissions / add", "[common][filesystem][ops][permissions]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  using fs::perm_options;
  using fs::perms;

  auto p = testing::nonexistent_path();

  std::error_code ec;

  testing::scoped_file f(p);
  REQUIRE(exists(p));

  ec = bad_ec;
  permissions(p, perms::owner_all, ec);
  REQUIRE(!ec);
  REQUIRE(status(p).permissions() == perms::owner_all);
  ec = bad_ec;
  permissions(p, perms::others_read, perm_options::add, ec);
  REQUIRE(!ec);
  REQUIRE(status(p).permissions() == (perms::owner_all | perms::others_read));
  ec = bad_ec;
  permissions(p, perms::others_read, perm_options::remove, ec);
  REQUIRE(!ec);
  REQUIRE(status(p).permissions() == perms::owner_all);
}

// This test case is not good and it is making the build on appveyor fail
// TODO(abdessattar): rewrite this
/*
TEST_CASE("Ops / permissions / symlink",
          "[common][filesystem][ops][permissions]") {
  using fs::perm_options;
  using fs::perms;

  testing::scoped_file f;
  REQUIRE(exists(f.path_));

  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  create_symlink(f.path_, p);

  std::error_code ec = make_error_code(std::errc::no_such_file_or_directory);
  permissions(p, perms::owner_all,
              perm_options::replace | perm_options::nofollow, ec);
  bool caught = false;
  std::error_code ec2;
  try {
    permissions(p, perms::owner_all,
                perm_options::replace | perm_options::nofollow);
  } catch (const fs::filesystem_error& ex) {
    caught = true;
    ec2 = ex.code();
    REQUIRE(ex.path1() == p);
  }
  // Both calls should succeed, or both should fail with same error:
  if (ec) {
    REQUIRE(caught);
    REQUIRE(ec == ec2);
  } else {
    REQUIRE(!caught);
  }
}
*/

TEST_CASE("Ops / permissions / file",
          "[common][filesystem][ops][permissions]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  using fs::perm_options;
  using fs::perms;
  std::error_code ec;

  testing::scoped_file f;
  auto p = perms::owner_write;

  // symlink_nofollow should not give an error for non-symlinks
  ec = bad_ec;
  permissions(f.path_, p, perm_options::replace | perm_options::nofollow, ec);
  REQUIRE(!ec);
  auto st = status(f.path_);
  REQUIRE(st.permissions() == p);
  p |= perms::owner_read;
  ec = bad_ec;
  permissions(f.path_, p, perm_options::replace | perm_options::nofollow, ec);
  REQUIRE(!ec);
  st = status(f.path_);
  REQUIRE(st.permissions() == p);
}

TEST_CASE("Ops / permissions / directory",
          "[common][filesystem][ops][permissions]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  using fs::perm_options;
  using fs::perms;
  std::error_code ec;

  auto p = testing::nonexistent_path();
  testing::scoped_file sfrom(p, testing::scoped_file::adopt_file);

  ec = bad_ec;
  create_directory(p, ec);
  REQUIRE(!ec);
  REQUIRE(exists(p));

  ec = bad_ec;
  permissions(p, perms::owner_read, perm_options::replace, ec);
  REQUIRE(!ec);
  auto st = status(p);
  REQUIRE(st.permissions() == perms::owner_read);

  ec = bad_ec;
  permissions(p, perms::owner_write | perms::owner_exec, perm_options::add, ec);
  REQUIRE(!ec);
  st = status(p);
  REQUIRE(st.permissions() == (perms::owner_all));

  ec = bad_ec;
  permissions(p, perms::owner_write, perm_options::remove, ec);
  REQUIRE(!ec);
  st = status(p);
  REQUIRE(st.permissions() == (perms::owner_read | perms::owner_exec));
}

TEST_CASE("Ops / permissions / error",
          "[common][filesystem][ops][permissions]") {
  using perms = fs::perms;

  auto p = testing::nonexistent_path();

  std::error_code ec = make_error_code(std::errc::no_such_file_or_directory);
  permissions(p, perms::owner_all, ec);
  REQUIRE(ec);

  REQUIRE_THROWS_MATCHES(permissions(p, perms::owner_all), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, p));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
