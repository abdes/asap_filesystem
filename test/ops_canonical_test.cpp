//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;

// -----------------------------------------------------------------------------
//  canonical
// -----------------------------------------------------------------------------

TEST_CASE("Ops / canonical", "[common][filesystem][ops][canonical]") {
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec;
  auto p = testing::nonexistent_path();
  canonical(p, ec);
  REQUIRE(ec);

  create_directory(p);
  auto p2 = canonical(p, ec);
  ComparePaths(p2, fs::current_path() / p);
  REQUIRE(!ec);

  ec = bad_ec;
  p2 = canonical(fs::current_path() / "." / (p.string() + "////././."), ec);
  ComparePaths(p2, fs::current_path() / p);
  REQUIRE(!ec);

  remove(p);

  ec = bad_ec;
  p = fs::current_path();
  p2 = canonical(p, ec);
  ComparePaths(p2, p);
  REQUIRE(!ec);

  // For absolute pths, canonical on windows will add the root name

  ec = bad_ec;
  p = "/";
  p = canonical(p, ec);
  ComparePaths(p, p.root_name() / "/");
  REQUIRE(!ec);

  ec = bad_ec;
  p = "/.";
  p = canonical(p, ec);
  ComparePaths(p, p.root_name() / "/");
  REQUIRE(!ec);

  ec = bad_ec;
  p = "/..";
  p = canonical(p, ec);
  ComparePaths(p, p.root_name() / "/");
  REQUIRE(!ec);

  ec = bad_ec;
  p = "/../.././.";
  p = canonical(p, ec);
  ComparePaths(p, p.root_name() / "/");
  REQUIRE(!ec);
}

TEST_CASE("Ops / canonical / exception",
          "[common][filesystem][ops][canonical]") {
  const fs::path p = testing::nonexistent_path();
  std::error_code ec;
  const fs::path res = canonical(p, ec);
  REQUIRE(ec);
  REQUIRE(res.empty());

  REQUIRE_THROWS_MATCHES(
      canonical(p), fs::filesystem_error,
      testing::FilesystemErrorDetail(ec, p, fs::current_path()));
}

TEST_CASE("Ops / canonical / real use",
          "[common][filesystem][ops][canonical]") {
  auto dir = testing::nonexistent_path();
  fs::create_directory(dir);
  fs::path foo = dir / "foo", bar = dir / "bar";
  fs::create_directory(foo);
  fs::create_directory(bar);
#if defined(ASAP_WINDOWS)
  fs::create_directory_symlink("..\\bar", foo / "baz");
#else
  fs::create_directory_symlink("../bar", foo / "baz");
#endif
  auto dirc = canonical(dir);
  auto barc = canonical(bar);

  auto p1 = fs::canonical(dir / "foo//.///..//./");
  ComparePaths(p1, dirc);
  auto p2 = fs::canonical(dir / "foo//./baz///..//./");
  ComparePaths(p2, dirc);
  auto p3 = fs::canonical(dir / "foo//./baz////./");
  ComparePaths(p3, barc);
  auto p4 = fs::canonical(dir / "foo//./baz///..//./bar");
  ComparePaths(p4, barc);
  auto p5 = fs::canonical(dir / "foo//./baz///..//./bar/");
  ComparePaths(p5, p4);
  auto p6 = fs::canonical(dir / "foo//./baz///..//./bar/.");
  ComparePaths(p6, p4);

  remove_all(dir);
}
