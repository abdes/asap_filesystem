//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#if defined(__clang__)
#pragma clang diagnostic push
// Catch2 uses a lot of macro names that will make clang go crazy
#if !defined(__APPLE__)
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
// Big mess created because of the way spdlog is organizing its source code
// based on header only builds vs library builds. The issue is that spdlog
// places the template definitions in a separate file and explicitly
// instantiates them, so we have no problem at link, but we do have a problem
// with clang (rightfully) complaining that the template definitions are not
// available when the template needs to be instantiated here.
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif  // __clang__

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Assign
// -----------------------------------------------------------------------------

TEST_CASE("Path / assign / operator", "[common][filesystem][path][assign]") {
  SECTION("Using std::string") {
    for (const std::string &s : TEST_PATHS()) {
      path p0(s);
      path p1;
      path p2;

      p1 = s;
      ComparePaths(p0, p1);

      p2 = s.c_str();
      ComparePaths(p0, p2);
    }
  }
  SECTION("Using std::wstring") {
    for (const std::string &s : TEST_PATHS()) {
      path p0 = s;
      path p1;
      path p2;
      std::wstring ws(s.begin(), s.end());

      p1 = ws;
      ComparePaths(p0, p1);

      p2 = ws.c_str();
      ComparePaths(p0, p2);
    }
  }
}

TEST_CASE("Path / assign / assign", "[common][filesystem][path][assign]") {
  SECTION("Using std::string") {
    for (const std::string &s : TEST_PATHS()) {
      path p0 = s;
      path p1;
      path p2;
      path p3;
      path p4;

      p1.assign(s);
      ComparePaths(p0, p1);

      p2.assign(s.begin(), s.end());
      ComparePaths(p0, p2);

      p3.assign(s.c_str());
      ComparePaths(p0, p3);

      p4.assign(s.c_str(), s.c_str() + s.size());
      ComparePaths(p0, p4);
    }
  }
  SECTION("Using std::wstring") {
    for (const std::string &s : TEST_PATHS()) {
      path p0 = s;
      path p1;
      path p2;
      path p3;
      path p4;
      std::wstring ws(s.begin(), s.end());

      p1.assign(ws);
      ComparePaths(p0, p1);

      p2.assign(ws.begin(), ws.end());
      ComparePaths(p0, p2);

      p3.assign(ws.c_str());
      ComparePaths(p0, p3);

      p4.assign(ws.c_str(), ws.c_str() + ws.size());
      ComparePaths(p0, p4);
    }
  }
}

TEST_CASE("Path / assign / copy", "[common][filesystem][path][assign]") {
  for (const path p : TEST_PATHS()) {
    path copy;
    copy = p;
    ComparePaths(p, copy);
  }
}

TEST_CASE("Path / assign / copy move", "[common][filesystem][path][assign]") {
  for (const path p : TEST_PATHS()) {
    path copy = p;
    path move;
    move = std::move(copy);
    ComparePaths(p, move);
  }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__
