//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  weakly_canonical
// -----------------------------------------------------------------------------

TEST_CASE("Ops / weakly_canonical",
          "[common][filesystem][ops][weakly_canonical]") {
  auto dir = testing::nonexistent_path();
  fs::create_directory(dir);
  const auto dirc = canonical(dir);
  fs::path foo = dir / "foo", bar = dir / "bar";
  fs::create_directory(foo);
  fs::create_directory(bar);
  fs::create_directory(bar / "baz");
#if defined(ASAP_WINDOWS)
  fs::create_directory_symlink("..\\bar", foo / "bar");
#else
  fs::create_directory_symlink("../bar", foo / "bar");
#endif

  auto p = fs::weakly_canonical(dir / "foo//./bar///../biz/.");
  REQUIRE(p == dirc / "biz/");
  p = fs::weakly_canonical(dir / "foo/.//bar/././baz/.");
  REQUIRE(p == dirc / "bar/baz");
  p = fs::weakly_canonical(fs::current_path() / dir / "bar//../foo/bar/baz");
  REQUIRE(p == dirc / "bar/baz");

  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;

  ec = bad_ec;
  p = fs::weakly_canonical(dir / "foo//./bar///../biz/.", ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "biz/");
  ec = bad_ec;
  p = fs::weakly_canonical(dir / "foo/.//bar/././baz/.", ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "bar/baz");
  ec = bad_ec;
  p = fs::weakly_canonical(fs::current_path() / dir / "bar//../foo/bar/baz",
                           ec);
  REQUIRE(!ec);
  REQUIRE(p == dirc / "bar/baz");

  fs::remove_all(dir, ec);
}
