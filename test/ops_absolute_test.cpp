//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  absolute
// -----------------------------------------------------------------------------

TEST_CASE("Ops / absolute / suite", "[common][filesystem][ops][absolute]") {
  for (const path p : TEST_PATHS()) {
    std::error_code ec;
    path abs = absolute(p, ec);
    REQUIRE((ec || abs.is_absolute()));
  }
}

TEST_CASE("Ops / absolute / special cases",
          "[common][filesystem][ops][absolute]") {
  std::error_code ec = make_error_code(std::errc::invalid_argument);
  path root = testing::root_path();
  REQUIRE(absolute(root) == root);
  REQUIRE(absolute(root, ec) == root);
  REQUIRE(!ec);
  // For POSIX-based operating systems, std::filesystem::absolute(p) is
  // equivalent to std::filesystem::current_path() / p
  REQUIRE(absolute(path{}, ec) == fs::current_path() / "");
  REQUIRE(!ec);

#if defined(ASAP_WINDOWS)
  path p1("/");
  REQUIRE(absolute(p1) != p1);
  path p2("/foo");
  REQUIRE(absolute(p2) != p2);
  REQUIRE(absolute(p2) == (absolute(p1) / p2));
  path p3("foo");
  REQUIRE(absolute(p3) != p3);
  path p4("C:\\");
  REQUIRE(absolute(p4) == p4);
#else
  path p1("/");
  REQUIRE(absolute(p1) == p1);
  path p2("/foo");
  REQUIRE(absolute(p2) == p2);
  path p3("foo");
  REQUIRE(absolute(p3) != p3);
  REQUIRE(absolute(p3) == (fs::current_path() / p3));
#endif
}
