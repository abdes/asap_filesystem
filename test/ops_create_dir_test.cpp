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
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  create_directories
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_directories",
          "[common][filesystem][ops][create_directories]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test empty path.
  bool b = fs::create_directories("", ec);
  REQUIRE(ec);
  REQUIRE(!b);

  // Test existing path.
  ec = bad_ec;
  b = fs::create_directories(fs::current_path(), ec);
  REQUIRE(!ec);
  REQUIRE(!b);

  // Test non-existent path.
  const auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);

  ec = bad_ec;
  b = fs::create_directories(p, ec);
  REQUIRE(!ec);
  REQUIRE(b);
  REQUIRE(is_directory(p));

  ec = bad_ec;
  b = fs::create_directories(p / ".", ec);
  REQUIRE(!ec);
  REQUIRE(!b);

  ec = bad_ec;
  b = fs::create_directories(p / "..", ec);
  REQUIRE(!ec);
  REQUIRE(!b);

  ec = bad_ec;
  b = fs::create_directories(p / "d1/d2/d3", ec);
  REQUIRE(!ec);
  REQUIRE(b);
  REQUIRE(is_directory(p / "d1/d2/d3"));

  ec = bad_ec;
  b = fs::create_directories(p / "./d4/../d5", ec);
  REQUIRE(!ec);
  REQUIRE(b);
  REQUIRE(is_directory(p / "./d4/../d5"));
}

// -----------------------------------------------------------------------------
//  create_directory
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_directory",
          "[common][filesystem][ops][create_directory]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  // Test empty path.
  fs::path p;
  bool b = create_directory(p, ec);
  REQUIRE(ec);
  REQUIRE(!b);

  // Test non-existent path
  p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  REQUIRE(!exists(p));

  ec = bad_ec;
  b = create_directory(p, ec);  // create the directory once
  REQUIRE(!ec);
  REQUIRE(b);
  REQUIRE(exists(p));

  // Test existing path
  ec = bad_ec;
  b = create_directory(p, ec);
  REQUIRE(!ec);
  REQUIRE(!b);
  b = create_directory(p);
  REQUIRE(!b);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
