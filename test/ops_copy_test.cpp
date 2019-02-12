//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  copy
// -----------------------------------------------------------------------------

TEST_CASE("Ops / copy / errors", "[common][filesystem][ops][copy]") {
  auto p = testing::nonexistent_path();
  std::error_code ec;

  REQUIRE(!fs::exists(p));
  fs::copy(p, ".", fs::copy_options::none, ec);
  REQUIRE(ec);

  ec.clear();
  fs::copy(".", ".", fs::copy_options::none, ec);
  REQUIRE(ec);

  testing::scoped_file f(p);
  REQUIRE(fs::is_directory("."));
  REQUIRE(fs::is_regular_file(p));
  ec.clear();
  fs::copy(".", p, fs::copy_options::none, ec);
  REQUIRE(ec);

  auto to = testing::nonexistent_path();
  ec.clear();
  auto opts = fs::copy_options::create_symlinks;
  fs::copy("/", to, opts, ec);
  REQUIRE(ec == std::make_error_code(std::errc::is_a_directory));
  REQUIRE(!exists(to));

  ec.clear();
  opts |= fs::copy_options::recursive;
  fs::copy("/", to, opts, ec);
  REQUIRE(ec == std::make_error_code(std::errc::is_a_directory));
  REQUIRE(!exists(to));
}

TEST_CASE("Ops / copy / symlinks", "[common][filesystem][ops][copy]") {
  // This test case requires symlinks and on windows, this will only work if
  // developer mode is enabled or the test cases are run as administrator.
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();
  std::error_code ec;

  ec = bad_ec;
  fs::create_symlink(".", from, ec);
  REQUIRE(!ec);
  REQUIRE(fs::exists(from));

  ec = bad_ec;
  fs::copy(from, to, fs::copy_options::skip_symlinks, ec);
  REQUIRE(!ec);
  REQUIRE(!fs::exists(to));

  ec = bad_ec;
  fs::copy(from, to, fs::copy_options::skip_symlinks, ec);
  REQUIRE(!ec);
  REQUIRE(!fs::exists(to));

  ec = bad_ec;
  fs::copy(from, to,
           fs::copy_options::skip_symlinks | fs::copy_options::copy_symlinks,
           ec);
  REQUIRE(!ec);
  REQUIRE(!fs::exists(to));

  ec = bad_ec;
  fs::copy(from, to, fs::copy_options::copy_symlinks, ec);
  REQUIRE(!ec);
  REQUIRE(fs::exists(to));
  REQUIRE(is_symlink(to));

  ec.clear();
  fs::copy(from, to, fs::copy_options::copy_symlinks, ec);
  REQUIRE(ec);

  remove(from, ec);
  remove(to, ec);
}

TEST_CASE("Ops / copy / empty file", "[common][filesystem][ops][copy]") {
  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();

  // test empty file
  std::ofstream{from};
  REQUIRE(fs::exists(from));
  REQUIRE(fs::file_size(from) == 0);
  fs::copy(from, to);
  REQUIRE(fs::exists(to));
  REQUIRE(fs::file_size(to) == 0);

  remove(from);
  remove(to);
}

TEST_CASE("Ops / copy / regular file", "[common][filesystem][ops][copy]") {
  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();

  std::ofstream{from} << "Hello, filesystem!";
  REQUIRE(fs::file_size(from) != 0);

  REQUIRE(!fs::exists(to));
  fs::copy(from, to);
  REQUIRE(fs::exists(to));
  REQUIRE(fs::file_size(to) == fs::file_size(from));

  remove(from);
  remove(to);
}

TEST_CASE("Ops / copy / directory", "[common][filesystem][ops][copy]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();
  std::error_code ec;

  create_directories(from / "a/b/c");

  {
    testing::scoped_file f(to);
    copy(from, to, ec);
    REQUIRE(ec);
  }

  testing::scoped_file f1(from / "a/f1");
  std::ofstream{f1.path_} << "file one";
  testing::scoped_file f2(from / "a/b/f2");
  std::ofstream{f2.path_} << "file two";

  copy(from, to, ec);
  REQUIRE(!ec);
  REQUIRE(exists(to));
  REQUIRE(!is_empty(to));
  remove_all(to);

  ec = bad_ec;
  copy(from, to, fs::copy_options::recursive, ec);
  REQUIRE(!ec);
  REQUIRE(exists(to));
  REQUIRE(!is_empty(to));
  REQUIRE(is_regular_file(to / "a/f1"));
  REQUIRE(!is_empty(to / "a/f1"));
  REQUIRE(file_size(from / "a/f1") == file_size(to / "a/f1"));
  REQUIRE(is_regular_file(to / "a/b/f2"));
  REQUIRE(!is_empty(to / "a/b/f2"));
  REQUIRE(file_size(from / "a/b/f2") == file_size(to / "a/b/f2"));
  REQUIRE(is_directory(to / "a/b/c"));
  REQUIRE(is_empty(to / "a/b/c"));

  f1.path_.clear();
  f2.path_.clear();
  remove_all(from, ec);
  remove_all(to, ec);
}

TEST_CASE("Ops / copy / no-op", "[common][filesystem][ops][copy]") {
  auto to = testing::nonexistent_path();
  std::error_code ec = std::make_error_code(std::errc::invalid_argument);

  fs::copy("/", to, fs::copy_options::copy_symlinks, ec);
  REQUIRE(!ec);  // Previous value should be cleared (LWG 2683)
}