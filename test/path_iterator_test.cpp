//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if (__clang_major__ >= 13) && !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif // __clang__

#include <algorithm>
#include <array>
#include <catch2/catch.hpp>
#include <vector>

#include "fs_testsuite.h"

using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Iterator
// -----------------------------------------------------------------------------

TEST_CASE("Path / iterator / components", "[common][filesystem][path][iterator]") {
  for (const path p : TEST_PATHS()) {
    CAPTURE(p);
    if (p.empty()) {
      REQUIRE(std::distance(p.begin(), p.end()) == 0);
    } else {
      REQUIRE(std::distance(p.begin(), p.end()) != 0);
    }

    for (const path &cmpt : p) {
      if (cmpt.empty()) {
        REQUIRE(std::distance(cmpt.begin(), cmpt.end()) == 0);
      } else {
        REQUIRE(std::distance(cmpt.begin(), cmpt.end()) == 1);
      }
    }
  }
}

TEST_CASE("Path / iterator / traversal", "[common][filesystem][path][iterator]") {
  path p;
  REQUIRE(p.begin() == p.end());

  std::vector<path> v;
  std::vector<path> v2;

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
  std::vector<path> fwd;
  std::vector<path> rev;

  for (const path p : TEST_PATHS()) {
    const auto begin = p.begin();
    const auto end = p.end();
    fwd.assign(begin, end);
    rev.assign(reverse_iterator(end), reverse_iterator(begin));
    REQUIRE(fwd.size() == rev.size());
    REQUIRE(std::equal(fwd.begin(), fwd.end(), rev.rbegin()));
  }
}

TEST_CASE("Path / iterator / special cases", "[common][filesystem][path][iterator]") {
  std::array<path, 4> paths{"single", "multiple/elements", "trailing/slash/", "/."};
  for (const path &p : paths) {
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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
