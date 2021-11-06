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
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  exists
// -----------------------------------------------------------------------------

TEST_CASE("Ops / exists / special", "[common][filesystem][ops][exists]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  const path root = testing::root_path();

  REQUIRE(exists(root));
  REQUIRE(exists(root / "."));
  REQUIRE(exists(path{"."}));
  REQUIRE(exists(path{".."}));
  REQUIRE(exists(fs::current_path()));

  std::error_code ec;
  ec = bad_ec;
  REQUIRE(exists(root, ec));
  REQUIRE(!ec);
  ec = bad_ec;
  REQUIRE(exists(root / ".", ec));
  REQUIRE(!ec);
  ec = bad_ec;
  REQUIRE(exists(path{"."}, ec));
  REQUIRE(!ec);
  ec = bad_ec;
  REQUIRE(exists(path{".."}, ec));
  REQUIRE(!ec);
  ec = bad_ec;
  REQUIRE(exists(fs::current_path(), ec));
  REQUIRE(!ec);
}

TEST_CASE("Ops / exists / not", "[common][filesystem][ops][exists]") {
  path rel = testing::nonexistent_path();
  REQUIRE(!exists(rel));

  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  REQUIRE(!exists(rel, ec));
  REQUIRE(!ec);
}

TEST_CASE("Ops / exists / not (absolute)",
          "[common][filesystem][ops][exists]") {
  path abs = absolute(testing::nonexistent_path());
  REQUIRE(!exists(abs));

  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  REQUIRE(!exists(abs, ec));
  REQUIRE(!ec);
}

TEST_CASE("Ops / exists / permissions", "[common][filesystem][ops][exists]") {
  using fs::perm_options;
  using fs::perms;
  path p = testing::nonexistent_path();
  create_directory(p);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  permissions(p, perms::all, perm_options::remove);

  auto unr = p / "unreachable";
  std::error_code ec;
  REQUIRE(!exists(unr, ec));

  // On windows, by default FILE_TRAVERSE privilege is enabled for all users.
  // https://docs.microsoft.com/en-us/windows/win32/fileio/file-security-and-access-rights
  //
  //  The FILE_TRAVERSE access right can be enforced by removing the
  //  BYPASS_TRAVERSE_CHECKING privilege from users. This is not recommended in
  //  the general case, as many programs do not correctly handle directory
  //  traversal errors. The primary use for the FILE_TRAVERSE access right on
  //  directories is to enable conformance to certain IEEE and ISO POSIX
  //  standards when interoperability with Unix systems is a requirement.

#if !defined(ASAP_WINDOWS)
  REQUIRE(ec == std::errc::permission_denied);
  ec.clear();

  REQUIRE_THROWS_MATCHES(
      exists(unr), fs::filesystem_error,
      testing::FilesystemErrorDetail(
          std::make_error_code(std::errc::permission_denied), unr));
#endif  // ASAP_WINDOWS
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
