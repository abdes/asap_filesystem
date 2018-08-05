//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  dir_iterator
// -----------------------------------------------------------------------------

TEST_CASE("Ops / dir_iterator", "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test non-existent path.
  const auto p = testing::nonexistent_path();
  fs::directory_iterator iter(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test empty directory.
  create_directory(p, fs::current_path(), ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));

  // Test non-empty directory.
  ec = bad_ec;
  create_directory(p / "x", ec);
  REQUIRE(!ec);
  ec = bad_ec;
  iter = fs::directory_iterator(p, ec);
  REQUIRE(!ec);
  REQUIRE(iter != fs::directory_iterator());
  REQUIRE(iter->path() == p / "x");
  ++iter;
  REQUIRE(iter == end(iter));

#if !(defined(ASAP_WINDOWS))
  // Test inaccessible directory.
  ec = bad_ec;
  permissions(p, fs::perms::none, ec);
  REQUIRE(!ec);
  iter = fs::directory_iterator(p, ec);
  REQUIRE(ec);
  REQUIRE(iter == end(iter));

  // Test inaccessible directory, skipping permission denied.
  const auto opts = fs::directory_options::skip_permission_denied;
  ec = bad_ec;
  iter = fs::directory_iterator(p, opts, ec);
  REQUIRE(!ec);
  REQUIRE(iter == end(iter));
#endif

  permissions(p, fs::perms::owner_all, ec);
  remove_all(p, ec);
}

TEST_CASE("Ops / dir_iterator / ++",
          "[common][filesystem][ops][dir_iterator]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  const auto p = testing::nonexistent_path();
  ec = bad_ec;
  create_directory(p, fs::current_path(), ec);
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

  remove_all(p, ec);
}

TEST_CASE("Ops / dir_iterator / noexcept",
          "[common][filesystem][ops][dir_iterator]") {
  auto p = testing::nonexistent_path();
  create_directory(p);
  create_directory(p / "x");
  fs::directory_iterator it(p), endit;
  REQUIRE(begin(it) == it);
  static_assert(noexcept(begin(it)), "begin is noexcept");
  REQUIRE(end(it) == endit);
  static_assert(noexcept(end(it)), "end is noexcept");
}
