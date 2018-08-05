//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  copy_file
// -----------------------------------------------------------------------------

TEST_CASE("Ops / copy_file", "[common][filesystem][ops][copy_file]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();

  // test non-existent file
  bool b = copy_file(from, to, ec);
  REQUIRE(!b);
  REQUIRE(ec);
  REQUIRE(!exists(to));

  // test empty file
  std::ofstream{from};
  REQUIRE(exists(from));
  REQUIRE(file_size(from) == 0);

  b = copy_file(from, to);
  REQUIRE(b);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == 0);
  remove(to);
  REQUIRE(!exists(to));
  b = copy_file(from, to, fs::copy_options::none, ec);
  REQUIRE(b);
  REQUIRE(!ec);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == 0);

  std::ofstream{from} << "Hello, filesystem!";
  REQUIRE(file_size(from) != 0);
  remove(to);
  REQUIRE(!exists(to));
  b = copy_file(from, to);
  REQUIRE(b);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == file_size(from));
  remove(to);
  REQUIRE(!exists(to));
  b = copy_file(from, to);
  REQUIRE(b);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == file_size(from));

  remove(from);
  remove(to);
}