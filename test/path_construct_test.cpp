//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <cstring>

#include <filesystem/fs_path.h>
#include "testsuite_fs.h"

using asap::filesystem::path;
using testing::ComparePaths;
using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  Construction
// -----------------------------------------------------------------------------

TEST_CASE("Path / construct / default",
          "[common][filesystem][path][construct]") {
  path p;
  CHECK(p.empty());
  CHECK(!p.has_root_path());
  CHECK(!p.has_root_name());
  CHECK(!p.has_root_directory());
  CHECK(!p.has_relative_path());
  CHECK(!p.has_parent_path());
  CHECK(!p.has_filename());
  CHECK(!p.has_stem());
  CHECK(!p.has_extension());
  CHECK(!p.is_absolute());
  CHECK(p.is_relative());
  CHECK(std::distance(p.begin(), p.end()) == 0);
}

TEST_CASE("Path / construct / copy", "[common][filesystem][path][construct]") {
  for (const path p : TEST_PATHS) {
    path copy = p;
    ComparePaths(p, copy);
  }
}

TEST_CASE("Path / construct / move", "[common][filesystem][path][construct]") {
  for (const path p : TEST_PATHS) {
    path copy = p;
    path move = std::move(copy);
    ComparePaths(p, move);
  }
}

TEST_CASE("Path / construct / path(string_type&&, format)",
          "[common][filesystem][path][construct]") {
  // path(string_type&&, format)
  auto s = [&]() -> path::string_type { return path("foo/bar").native(); };
  path p0(s());
  path p1(s(), path::auto_format);
  REQUIRE(p1 == p0);
  path p2(s(), path::native_format);
  REQUIRE(p2 == p0);
  path p3(s(), path::generic_format);
  REQUIRE(p3 == p0);
}

TEST_CASE("Path / construct / path(const Source&, format)",
          "[common][filesystem][path][construct]") {
  // path(const Source&, format)
  SECTION("Source = path::string_type") {
    const path::string_type s = path("foo/bar").native();
    path p0(s);
    path p1(s, path::auto_format);
    REQUIRE(p1 == p0);
    path p2(s, path::native_format);
    REQUIRE(p2 == p0);
    path p3(s, path::generic_format);
    REQUIRE(p3 == p0);
  }
  SECTION("Source = std::string") {
    const std::string s = "foo/bar";
    path p0(s);
    path p1(s, path::auto_format);
    REQUIRE(p1 == p0);
    path p2(s, path::native_format);
    REQUIRE(p2 == p0);
    path p3(s, path::generic_format);
    REQUIRE(p3 == p0);
  }
  SECTION("Source = path::wstring") {
    // path(const Source&, format)
    const std::wstring s = L"foo/bar";
    path p0(s);
    path p1(s, path::auto_format);
    REQUIRE(p1 == p0);
    path p2(s, path::native_format);
    REQUIRE(p2 == p0);
    path p3(s, path::generic_format);
    REQUIRE(p3 == p0);
  }
  SECTION("Source = const char *") {
    // path(const Source&, format)
    const char *s = "foo/bar";
    path p0(s);
    path p1(s, path::auto_format);
    REQUIRE(p1 == p0);
    path p2(s, path::native_format);
    REQUIRE(p2 == p0);
    path p3(s, path::generic_format);
    REQUIRE(p3 == p0);
  }
}

TEST_CASE("Path / construct / path(InputIterator, InputIterator, format)",
          "[common][filesystem][path][construct]") {
  // path(InputIterator, InputIterator, format)
  const char s[] = "foo/bar";
  const std::vector<char> c(s, s + strlen(s));
  auto c0 = c;
  path p0(std::begin(c0), std::end(c0));
  auto c1 = c;
  path p1(std::begin(c1), std::end(c1), path::auto_format);
  REQUIRE(p1 == p0);
  auto c2 = c;
  path p2(std::begin(c2), std::end(c2), path::native_format);
  REQUIRE(p2 == p0);
  auto c3 = c;
  path p3(std::begin(c3), std::end(c3), path::generic_format);
  REQUIRE(p3 == p0);
}

TEST_CASE("Path / construct / range", "[common][filesystem][path][construct]") {
  for (auto &s : TEST_PATHS) {
    path p1 = s;
    path p2(s.begin(), s.end());
    path p3(s.c_str());
    path p4(s.c_str(), s.c_str() + s.size());

    ComparePaths(p1, p2);
    ComparePaths(p1, p3);
    ComparePaths(p1, p4);

    std::wstring ws(s.begin(), s.end());
    path p5 = ws;
    path p6(ws.begin(), ws.end());
    path p7(ws.c_str());
    path p8(ws.c_str(), ws.c_str() + ws.size());

    ComparePaths(p1, p5);
    ComparePaths(p1, p6);
    ComparePaths(p1, p7);
    ComparePaths(p1, p8);

    // Test with input iterators and const value_types

    std::vector<char> r1((char *)s.c_str(), (char *)s.c_str() + s.size());
    path p9(r1.begin(), r1.end());
    ComparePaths(p1, p9);

    std::vector<char> r2(
        (char *)s.c_str(),
        (char *)s.c_str() + s.size() + 1);  // includes null-terminator
    path p10(r2.begin());
    ComparePaths(p1, p10);

    std::vector<char> r3(s.c_str(), s.c_str() + s.size());
    path p11(r3.begin(), r3.end());
    ComparePaths(p1, p11);

    std::vector<char> r4(s.c_str(),
                         s.c_str() + s.size() + 1);  // includes null-terminator
    path p12(r4.begin());
    ComparePaths(p1, p12);

    // Test with input iterators and const value_types
    std::vector<wchar_t> r5((wchar_t *)ws.c_str(),
                            (wchar_t *)ws.c_str() + ws.size());
    path p13(r5.begin(), r5.end());
    ComparePaths(p1, p13);

    std::vector<wchar_t> r6(
        (wchar_t *)ws.c_str(),
        (wchar_t *)ws.c_str() + ws.size() + 1);  // includes null-terminator
    path p14(r6.begin());
    ComparePaths(p1, p14);

    std::vector<wchar_t> r7(ws.c_str(), ws.c_str() + ws.size());
    path p15(r7.begin(), r7.end());
    ComparePaths(p1, p15);

    std::vector<wchar_t> r8(
        ws.c_str(), ws.c_str() + ws.size() + 1);  // includes null-terminator
    path p16(r8.begin());
    ComparePaths(p1, p16);
  }
}
