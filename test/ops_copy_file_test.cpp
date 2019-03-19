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

TEST_CASE("Ops / copy_file / does not exist",
          "[common][filesystem][ops][copy_file]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  auto to = testing::nonexistent_path();

  // If from refers to a non-existing file, copy_file should fail with an error
  bool b = copy_file(from, to, ec);
  REQUIRE(!b);
  REQUIRE(ec);
  REQUIRE(!exists(to));
}

TEST_CASE("Ops / copy_file / not regular file",
          "[common][filesystem][ops][copy_file]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  testing::scoped_file sfrom(from, testing::scoped_file::adopt_file);
  fs::create_directory(from);
  fs::file_status st = fs::status(from, ec);
  REQUIRE(st.type() != fs::file_type::regular);

  auto to = testing::nonexistent_path();

  // If from refers to a non-regular file, copy_file should fail with an error
  bool b = copy_file(from, to, ec);
  REQUIRE(!b);
  REQUIRE(ec);
  REQUIRE(!exists(to));
}

TEST_CASE("Ops / copy_file / empty", "[common][filesystem][ops][copy_file]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  testing::scoped_file sfrom(from, testing::scoped_file::adopt_file);
  auto to = testing::nonexistent_path();
  testing::scoped_file sto(to, testing::scoped_file::adopt_file);

  // test empty file
  std::ofstream{from};
  REQUIRE(exists(from));
  REQUIRE(file_size(from) == 0);

  bool b = copy_file(from, to);
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
}


TEST_CASE("Ops / copy_file", "[common][filesystem][ops][copy_file]") {
  std::error_code ec;

  auto from = testing::nonexistent_path();
  testing::scoped_file sfrom(from, testing::scoped_file::adopt_file);
  auto to = testing::nonexistent_path();
  testing::scoped_file sto(to, testing::scoped_file::adopt_file);

  std::ofstream{from} << "Hello, filesystem!";
  REQUIRE(file_size(from) != 0);
  remove(to);
  REQUIRE(!exists(to));

  bool b = copy_file(from, to);
  REQUIRE(b);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == file_size(from));
  remove(to);
  REQUIRE(!exists(to));

  b = copy_file(from, to);
  REQUIRE(b);
  REQUIRE(exists(to));
  REQUIRE(file_size(to) == file_size(from));
}
