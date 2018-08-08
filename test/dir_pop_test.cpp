//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  dir_pop
// -----------------------------------------------------------------------------

TEST_CASE("Dir / dir_pop / error", "[common][filesystem][ops][dir_pop]") {
  std::error_code ec;
  fs::recursive_directory_iterator dir;
  dir.pop(ec);  // This is undefined, but our implementation
  REQUIRE(ec);  // checks and returns an error.
  REQUIRE(dir == end(dir));

  std::error_code ec2;
  try {
    dir.pop();
  } catch (const fs::filesystem_error& ex) {
    ec2 = ex.code();
  }
  REQUIRE(ec2 == ec);
}

TEST_CASE("Dir / dir_pop / simple", "[common][filesystem][ops][dir_pop]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  const auto p = testing::nonexistent_path();
  create_directories(p / "d1/d2/d3");
  for (int i = 0; i < 3; ++i) {
    fs::recursive_directory_iterator dir(p);
    REQUIRE(dir != end(dir));
    std::advance(dir, i);
    REQUIRE(dir != end(dir));
    REQUIRE(dir.depth() == i);
    ec = bad_ec;
    dir.pop(ec);
    REQUIRE(!ec);
    REQUIRE(dir == end(dir));

    dir = fs::recursive_directory_iterator(p);
    std::advance(dir, i);
    REQUIRE(dir != end(dir));
    REQUIRE(dir.depth() == i);
    dir.pop();
    REQUIRE(dir == end(dir));
  }
  remove_all(p, ec);
}

TEST_CASE("Dir / dir_pop / complex", "[common][filesystem][ops][dir_pop]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  const auto p = testing::nonexistent_path();
  create_directories(p / "d1/d2/d3");
  create_directories(p / "d1/d2/e3");
  create_directories(p / "d1/e2/d3");
  for (int i = 0; i < 3; ++i) {
    fs::recursive_directory_iterator dir(p);
    std::advance(dir, i);
    REQUIRE(dir != end(dir));
    int expected_depth = i;
    REQUIRE(dir.depth() == expected_depth);
    ec = bad_ec;
    dir.pop(ec);
    REQUIRE(!ec);
    if (dir != end(dir)) REQUIRE(dir.depth() == (expected_depth - 1));

    dir = fs::recursive_directory_iterator(p);
    std::advance(dir, i);
    REQUIRE(dir != end(dir));
    REQUIRE(dir.depth() == i);
    dir.pop();
    if (dir != end(dir)) REQUIRE(dir.depth() == (i - 1));
  }
  remove_all(p, ec);
}
