//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <filesystem/path.h>
#include "testsuite_fs.h"

using asap::filesystem::path;
using testing::ComparePaths;
using testing::TEST_PATHS;

// path::operator/=(const path&)

path append(path l, const path& r) {
  l /= r;
  return l;
}

// -----------------------------------------------------------------------------
//  Append
// -----------------------------------------------------------------------------

TEST_CASE("Path / append / path", "[common][filesystem][path][append]") {
  ComparePaths(append("/foo/bar", "/foo/"), "/foo/");

#ifndef ASAP_WINDOWS_API
  ComparePaths(append("baz", "baz"), "baz/baz");
#else
  ComparePaths(append("baz", "baz"), "baz\\baz");
#endif
  ComparePaths(append("baz/", "baz"), "baz/baz");
  ComparePaths(append("baz", "/foo/bar"), "/foo/bar");
  ComparePaths(append("baz/", "/foo/bar"), "/foo/bar");

  REQUIRE(append("", "").empty());
  REQUIRE(!append("", "rel").is_absolute());

  ComparePaths(append("dir/", "/file"), "/file");
  ComparePaths(append("dir/", "file"), "dir/file");

#ifdef ASAP_WINDOWS_API
  ComparePaths(append("c:/foo", "/bar"), "c:/bar");
#endif
  // C++17 [fs.path.append] p4
#ifndef ASAP_WINDOWS_API
  ComparePaths(append("//host", "foo"), "//host/foo");

  ComparePaths(append("//host/", "foo"), "//host/foo");

  // path("foo") / ""; // yields "foo/"
  ComparePaths(append("foo", ""), "foo/");

  // path("foo") / "/bar"; // yields "/bar"
  ComparePaths(append("foo", "/bar"), "/bar");
#else
  ComparePaths(append("//host", "foo"), "//host\\foo");

  ComparePaths(append("//host/", "foo"), "//host/foo");

  // path("foo") / ""; // yields "foo/"
  ComparePaths(append("foo", ""), "foo\\");

  // path("foo") / "/bar"; // yields "/bar"
  ComparePaths(append("foo", "/bar"), "/bar");

  // path("foo") / "c:/bar"; // yields "c:/bar"
  ComparePaths(append("foo", "c:/bar"), "c:/bar");

  // path("foo") / "c:"; // yields "c:"
  ComparePaths(append("foo", "c:"), "c:");

  // path("c:") / ""; // yields "c:"
  ComparePaths(append("c:", ""), "c:");

  // path("c:foo") / "/bar"; // yields "c:/bar"
  ComparePaths(append("c:foo", "/bar"), "c:/bar");

  // path("c:foo") / "c:bar"; // yields "c:foo/bar"
  ComparePaths(append("foo", "c:\\bar"), "c:\\bar");
#endif
}

// path::operator/=(const Source& source)
// path::append(const Source& source)
// Equivalent to: return operator/=(path(source));

// path::append(InputIterator first, InputIterator last)
// Equivalent to: return operator/=(path(first, last));

template <typename Char>
void test(const path& p, const Char* s) {
  path expected = p;
  expected /= path(s);

  path oper = p;
  oper /= s;

  path func = p;
  func.append(s);

  std::vector<char> input_range(s, s + std::char_traits<Char>::length(s));
  path range = p;
  range.append(input_range.begin(), input_range.end());

  ComparePaths(oper, expected);
  ComparePaths(func, expected);
  ComparePaths(range, expected);
}

TEST_CASE("Path / append / source", "[common][filesystem][path][append]") {
  test("/foo/bar", "/foo/");

  test("baz", "baz");
  test("baz/", "baz");
  test("baz", "/foo/bar");
  test("baz/", "/foo/bar");

  test("", "");
  test("", "rel");

  test("dir/", "/file");
  test("dir/", "file");

  // C++17 [fs.path.append] p4
  test("//host", "foo");
  test("//host/", "foo");
  test("foo", "");
  test("foo", "/bar");
  test("foo", "c:/bar");
  test("foo", "c:");
  test("c:", "");
  test("c:foo", "/bar");
  test("foo", "c:\\bar");
}

TEST_CASE("Path / append / source / TEST_PATHS",
          "[common][filesystem][path][append]") {
  for (const path p : TEST_PATHS) {
    for (const path q : TEST_PATHS) {
      test(p, q.c_str());
    }
  }
}

// TODO: figure later wstring vs string correct impl
/*
TEST_CASE("Path / append / source / wstring",
          "[common][filesystem][path][append]") {
  test(  "foo", L"/bar" );
  test( L"foo",  "/bar" );
  test( L"foo", L"/bar" );
}
*/