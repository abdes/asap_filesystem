//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

// -----------------------------------------------------------------------------
//  read_symlink
// -----------------------------------------------------------------------------

#if defined(ASAP_WINDOWS)
TEST_CASE("Ops / read_symlink/ unsupported",
          "[common][filesystem][ops][read_symlink]") {
  auto p = testing::nonexistent_path();
  REQUIRE_THROWS_MATCHES(
      read_symlink(p), fs::filesystem_error,
      testing::FilesystemErrorDetail(
          std::make_error_code(std::errc::not_supported), p));
}
#else

TEST_CASE("Ops / read_symlink", "[common][filesystem][ops][read_symlink]") {
  auto p = testing::nonexistent_path();
  std::error_code ec;

  fs::read_symlink(p, ec);
  REQUIRE(ec);

  fs::path tgt = ".";
  fs::create_symlink(tgt, p);

  auto result = read_symlink(p, ec);
  REQUIRE(!ec);
  REQUIRE(result == tgt);

  fs::remove(p);
}

#endif