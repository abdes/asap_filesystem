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
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  permissions(p, fs::perms::none);
  std::error_code ec, ec2;

  bool result = fs::is_empty(p, ec);
  REQUIRE(ec == std::make_error_code(std::errc::permission_denied));
  REQUIRE(!result);

  REQUIRE_THROWS_MATCHES(fs::is_empty(p), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, p));

  result = fs::is_empty(p / "f", ec);

  // On windows, by default FILE_TRAVERSE privilege is enabled for all users.
  // https://docs.microsoft.com/en-us/windows/win32/fileio/file-security-and-access-rights
  //
  //  The FILE_TRAVERSE access right can be enforced by removing the
  //  BYPASS_TRAVERSE_CHECKING privilege from users. This is not recommended in
  //  the general case, as many programs do not correctly handle directory
  //  traversal errors. The primary use for the FILE_TRAVERSE access right on
  //  directories is to enable conformance to certain IEEE and ISO POSIX
  //  standards when interoperability with Unix systems is a requirement.
  REQUIRE(((ec.value() ==
            static_cast<typename std::underlying_type<std::errc>::type>(
                std::errc::permission_denied)) ||
           (ec.value() ==
            static_cast<typename std::underlying_type<std::errc>::type>(
                std::errc::no_such_file_or_directory))));
  REQUIRE(!result);

  REQUIRE_THROWS_MATCHES(fs::is_empty(p / "f"), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, p / "f"));
}

TEST_CASE("Ops / is_empty", "[common][filesystem][ops][is_empty]") {
  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
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
}
