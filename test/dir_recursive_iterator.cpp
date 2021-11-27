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

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  dir_recursive_iterator
// -----------------------------------------------------------------------------

TEST_CASE("Dir / dir_recursive_iterator", "[common][filesystem][ops][dir_recursive_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test non-existent path.
  const auto p = testing::nonexistent_path();
  testing::scoped_file sfrom(p, testing::scoped_file::adopt_file);

  fs::recursive_directory_iterator iter(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test empty directory.
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::recursive_directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));

  // Test non-empty directory.
  ec = bad_ec;
  create_directories(p / "d1/d2", ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::recursive_directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != end(iter));
  REQUIRE(iter->path() == p / "d1");
  ++iter;
  REQUIRE(iter->path() == p / "d1/d2");
  ++iter;
  REQUIRE(iter == end(iter));

#if !defined(ASAP_WINDOWS)
  // Test inaccessible directory.
  ec = bad_ec;
  permissions(p, fs::perms::none, ec);
  REQUIRE(!ec);
  iter = fs::recursive_directory_iterator(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test inaccessible directory, skipping permission denied.
  const auto opts = fs::directory_options::skip_permission_denied;
  iter = fs::recursive_directory_iterator(p, opts, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));

  // Test inaccessible sub-directory.
  ec = bad_ec;
  permissions(p, fs::perms::owner_all, ec);
  REQUIRE(!ec);
  ec = bad_ec;
  permissions(p / "d1/d2", fs::perms::none, ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::recursive_directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != end(iter));
  REQUIRE(iter->path() == p / "d1");
  ++iter; // should recurse into d1
  REQUIRE(iter != end(iter));
  REQUIRE(iter->path() == p / "d1/d2");
  iter.increment(ec); // should fail to recurse into p/d1/d2
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test inaccessible sub-directory, skipping permission denied.
  ec = bad_ec;
  iter = fs::recursive_directory_iterator(p, opts, ec);
  REQUIRE(!ec);
  REQUIRE(iter != end(iter));
  REQUIRE(iter->path() == p / "d1");
  ++iter; // should recurse into d1
  REQUIRE(iter != end(iter));
  REQUIRE(iter->path() == p / "d1/d2");
  ec = bad_ec;
  iter.increment(ec); // should fail to recurse into p/d1/d2, so skip it
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));
#endif

  permissions(p / "d1/d2", fs::perms::owner_all, ec);
  remove_all(p, ec);
}

TEST_CASE(
    "Dir / dir_recursive_iterator / ++", "[common][filesystem][ops][dir_recursive_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directories(p / "d1/d2", ec);
  REQUIRE(!ec);

  // Test post-increment (libstdc++/71005)
  ec = bad_ec;
  auto iter = fs::recursive_directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != end(iter));
  const auto entry1 = *iter;
  const auto entry2 = *iter++;
  REQUIRE(entry1 == entry2);
  REQUIRE(entry1.path() == p / "d1");
  const auto entry3 = *iter;
  const auto entry4 = *iter++;
  REQUIRE(entry3 == entry4);
  REQUIRE(entry3.path() == p / "d1/d2");
  REQUIRE(iter == end(iter));

  remove_all(p, ec);
}

TEST_CASE("Dir / dir_recursive_iterator / noexcept",
    "[common][filesystem][ops][dir_recursive_iterator]") {
  auto p = testing::nonexistent_path();
  create_directory(p);
  create_directory(p / "x");
  fs::recursive_directory_iterator it(p);
  fs::recursive_directory_iterator endit;
  REQUIRE(begin(it) == it);
  static_assert(noexcept(begin(it)), "begin is noexcept");
  REQUIRE(end(it) == endit);
  static_assert(noexcept(end(it)), "end is noexcept");

  std::error_code ec;
  remove_all(p, ec);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
