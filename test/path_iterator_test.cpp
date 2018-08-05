//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <algorithm>
#include <vector>

#include "fs_testsuite.h"

using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Iterator
// -----------------------------------------------------------------------------

TEST_CASE("Path / iterator / components",
          "[common][filesystem][path][iterator]") {
  for (const path p : TEST_PATHS) {
    CAPTURE(p);
    if (p.empty())
      REQUIRE(std::distance(p.begin(), p.end()) == 0);
    else
      REQUIRE(std::distance(p.begin(), p.end()) != 0);

    for (const path &cmpt : p) {
      if (cmpt.empty())
        REQUIRE(std::distance(cmpt.begin(), cmpt.end()) == 0);
      else
        REQUIRE(std::distance(cmpt.begin(), cmpt.end()) == 1);
    }
  }
}

TEST_CASE("Path / iterator / traversal",
          "[common][filesystem][path][iterator]") {
  path p;
  REQUIRE(p.begin() == p.end());

  std::vector<path> v, v2;

  p = "/";
  v.assign(p.begin(), p.end());
  v2 = {"/"};
  REQUIRE(v == v2);

  p = "filename";
  v.assign(p.begin(), p.end());
  v2 = {"filename"};
  REQUIRE(v == v2);

  p = "dir/.";
  v.assign(p.begin(), p.end());
  v2 = {"dir", "."};
  REQUIRE(v == v2);

  p = "dir/";
  v.assign(p.begin(), p.end());
  v2 = {"dir", ""};
  REQUIRE(v == v2);

  p = "//rootname/dir/.";
  v.assign(p.begin(), p.end());
#ifdef ASAP_WINDOWS
  v2 = {"//rootname", "/", "dir", "."};
#else
  v2 = {"/", "rootname", "dir", "."};
#endif
  REQUIRE(v == v2);

  p = "//rootname/dir/";
  v.assign(p.begin(), p.end());
#ifdef ASAP_WINDOWS
  v2 = {"//rootname", "/", "dir", ""};
#else
  v2 = {"/", "rootname", "dir", ""};
#endif
  REQUIRE(v == v2);

  p = "//rootname/dir/filename";
  v.assign(p.begin(), p.end());
#ifdef ASAP_WINDOWS
  v2 = {"//rootname", "/", "dir", "filename"};
#else
  v2 = {"/", "rootname", "dir", "filename"};
#endif
  REQUIRE(v == v2);

  p = "c:relative/path";
  v.assign(p.begin(), p.end());
#ifdef ASAP_WINDOWS
  v2 = {"c:", "relative", "path"};
#else
  v2 = {"c:relative", "path"};
#endif
  REQUIRE(v == v2);

  p = "c:/absolute/path";
  v.assign(p.begin(), p.end());
#ifdef ASAP_WINDOWS
  v2 = {"c:", "/", "absolute", "path"};
#else
  v2 = {"c:", "absolute", "path"};
#endif
  REQUIRE(v == v2);
}

TEST_CASE("Path / iterator / reverse", "[common][filesystem][path][iterator]") {
  using reverse_iterator = std::reverse_iterator<path::iterator>;
  std::vector<path> fwd, rev;

  for (const path p : TEST_PATHS) {
    const auto begin = p.begin(), end = p.end();
    fwd.assign(begin, end);
    rev.assign(reverse_iterator(end), reverse_iterator(begin));
    REQUIRE(fwd.size() == rev.size());
    REQUIRE(std::equal(fwd.begin(), fwd.end(), rev.rbegin()));
  }
}

TEST_CASE("Path / iterator / special cases",
          "[common][filesystem][path][iterator]") {
  path paths[] = {"single", "multiple/elements", "trailing/slash/", "/."};
  for (const path p : paths) {
    for (auto iter = p.begin(); iter != p.end(); ++iter) {
      auto iter2 = iter;
      ++iter;
      iter2++;
      REQUIRE(iter2 == iter);
      --iter;
      iter2--;
      REQUIRE(iter2 == iter);
    }
  }
}
