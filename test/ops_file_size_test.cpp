//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>
#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  file_size
// -----------------------------------------------------------------------------

TEST_CASE("Ops / file_size / special", "[common][filesystem][ops][file_size]") {
  std::error_code ec;
  auto size = fs::file_size(".", ec);
  REQUIRE(ec == std::errc::is_a_directory);
  REQUIRE(size == static_cast<std::uintmax_t>(-1));

  REQUIRE_THROWS_MATCHES(
      fs::file_size("."), fs::filesystem_error,
      testing::FilesystemErrorDetail(
          std::make_error_code(std::errc::is_a_directory), "."));
}

TEST_CASE("Ops / file_size / not existing",
          "[common][filesystem][ops][exists]") {
  fs::path p = testing::nonexistent_path();

  std::error_code ec;
  auto size = fs::file_size(p, ec);
  REQUIRE(ec);
  REQUIRE(size == static_cast<std::uintmax_t>(-1));

  try {
    size = fs::file_size(p);
    ec.clear();
  } catch (const fs::filesystem_error& e) {
    ec = e.code();
  }
  REQUIRE(ec);
  REQUIRE(size == static_cast<std::uintmax_t>(-1));
}
