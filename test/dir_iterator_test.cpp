//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  dir_iterator
// -----------------------------------------------------------------------------

TEST_CASE("Dir / dir_iterator / dne",
          "[common][filesystem][ops][dir_iterator]") {
  std::error_code ec;

  // Test non-existent path.
  const auto p = testing::nonexistent_path();
  fs::directory_iterator iter(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));
}

TEST_CASE("Dir / dir_iterator / empty",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test empty directory.
  const auto dir = testing::nonexistent_path();
  create_directory(dir, fs::current_path(), ec);
  testing::scoped_file sdir(dir, testing::scoped_file::adopt_file);
  fs::directory_iterator iter(dir, ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::directory_iterator(dir, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));
}

// It is not crystal clear in the C++ standard what is the exact interpretation
// of the follow_directory_symlink in directory_options. Our interpretation is
// that it is not significant for dir_iterator and it is only used for
// dir_recursive_iterator.
//
// This is in line with the default behavior of 'ls' command and with the fact
// that POSIX opendir always follows symlinks in the given path.
TEST_CASE("Dir / dir_iterator / over symlink",
          "[common][filesystem][ops][dir_iterator]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Create a directory and symlink to it
  const auto tgt = testing::nonexistent_path();
  create_directory(tgt, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file stgt(tgt, testing::scoped_file::adopt_file);
  REQUIRE(is_directory(tgt));
  auto lnk = testing::nonexistent_path();
  create_directory_symlink(tgt, lnk, ec);
  REQUIRE(!ec);
  testing::scoped_file slnk(lnk, testing::scoped_file::adopt_file);
  REQUIRE(is_symlink(lnk));

  // Add a file to the directory
  const auto f = testing::nonexistent_path();
  std::ofstream{tgt / f};
  REQUIRE(fs::exists(tgt / f));

  // Iterate over the link with follow_directory_symlink specified and check it
  // finds the file inside
  ec = bad_ec;
  auto iter = fs::directory_iterator(
      lnk, fs::directory_options::follow_directory_symlink, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == lnk / f);
  REQUIRE(iter->is_regular_file());
  ++iter;
  REQUIRE(iter == end(iter));

  // Iterate over the link with follow_directory_symlink not specified and check
  // it finds the file inside
  ec = bad_ec;
  iter = fs::directory_iterator(lnk, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == lnk / f);
  REQUIRE(iter->is_regular_file());
  ++iter;
  REQUIRE(iter == end(iter));
}

TEST_CASE("Dir / dir_iterator / contains single dir",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test non-empty directory with a single entry that is a directory.
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  ec = bad_ec;
  create_directory(p / "x", ec);
  REQUIRE(!ec);
  ec = bad_ec;
  fs::directory_iterator iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == p / "x");
  REQUIRE(iter->is_directory());
  ++iter;
  REQUIRE(iter == end(iter));
}

TEST_CASE("Dir / dir_iterator / contains single file",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test non-empty directory with a single entry that is a directory.
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  const auto f = testing::nonexistent_path();
  std::ofstream{p / f};
  REQUIRE(fs::exists(p / f));

  ec = bad_ec;
  fs::directory_iterator iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == p / f);
  REQUIRE(iter->is_regular_file());
  ++iter;
  REQUIRE(iter == end(iter));
}

TEST_CASE("Dir / dir_iterator / contains symlink",
          "[common][filesystem][ops][dir_iterator]") {
#if defined(ASAP_WINDOWS)
  if (!testing::IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test non-empty directory with a single entry that is a symlink.
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  testing::scoped_file f;
  auto tgt = f.path_;
  auto l = testing::nonexistent_path();
  REQUIRE(!exists(l));

  ec = bad_ec;
  create_symlink(".." / tgt, p / l, ec);  // create the symlink
  REQUIRE(!ec);
  REQUIRE(exists(p / l));
  REQUIRE(is_symlink(p / l));

  ec = bad_ec;
  fs::directory_iterator iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == p / l);
  REQUIRE(iter->is_symlink());
  ++iter;
  REQUIRE(iter == end(iter));
}

#if !(defined(ASAP_WINDOWS))
TEST_CASE("Dir / dir_iterator / no permission",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test inaccessible directory.
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  ec = bad_ec;
  create_directory(p / "x", ec);
  REQUIRE(!ec);

  ec = bad_ec;
  permissions(p, fs::perms::none, ec);
  REQUIRE(!ec);
  auto iter = fs::directory_iterator(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test inaccessible directory, skipping permission denied.
  const auto opts = fs::directory_options::skip_permission_denied;
  ec = bad_ec;
  iter = fs::directory_iterator(p, opts, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));
}
#endif

TEST_CASE("Dir / dir_iterator / ++",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  create_directory(p / "x", ec);
  REQUIRE(!ec);

  // Test post-increment (libstdc++/71005)
  ec = bad_ec;
  auto iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != end(iter));
  const auto entry1 = *iter;
  const auto entry2 = *iter++;
  REQUIRE(entry1 == entry2);
  REQUIRE(entry1.path() == p / "x");
  REQUIRE(iter == end(iter));
}

TEST_CASE("Dir / dir_iterator / noexcept",
          "[common][filesystem][ops][dir_iterator]") {
  auto p = testing::nonexistent_path();
  create_directory(p);
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  create_directory(p / "x");
  fs::directory_iterator it(p), endit;
  REQUIRE(begin(it) == it);
  static_assert(noexcept(begin(it)), "begin is noexcept");
  REQUIRE(end(it) == endit);
  static_assert(noexcept(end(it)), "end is noexcept");
}
