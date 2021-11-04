//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::ComparePaths;
using testing::TEST_PATHS;

// path::operator/=(const path&)

namespace {
fs::path Append(fs::path l, const fs::path& r) {
  l /= r;
  return l;
}
}  // namespace

// -----------------------------------------------------------------------------
//  Append
// -----------------------------------------------------------------------------

TEST_CASE("Path / append / path", "[common][filesystem][path][append]") {
  ComparePaths(Append("/foo/bar", "/foo/"), "/foo/");

  ComparePaths(Append("baz", "baz"), "baz/baz");
  ComparePaths(Append("baz/", "baz"), "baz/baz");
  ComparePaths(Append("baz", "/foo/bar"), "/foo/bar");
  ComparePaths(Append("baz/", "/foo/bar"), "/foo/bar");

  REQUIRE(Append("", "").empty());
  REQUIRE(!Append("", "rel").is_absolute());

  ComparePaths(Append("dir/", "/file"), "/file");
  ComparePaths(Append("dir/", "file"), "dir/file");

  // path("foo") / ""; // yields "foo/"
  ComparePaths(Append("foo", ""), "foo/");

  // path("foo") / "/bar"; // yields "/bar"
  ComparePaths(Append("foo", "/bar"), "/bar");

  // path("foo") / ""; // yields "foo/"
  ComparePaths(Append("foo", ""), "foo/");

  // path("foo") / "/bar"; // yields "/bar"
  ComparePaths(Append("foo", "/bar"), "/bar");

#if defined(ASAP_WINDOWS)
  ComparePaths(Append("//host", "foo"), "//host/foo");
  ComparePaths(Append("//host/", "foo"), "//host/foo");
  ComparePaths(Append("//host/bar", "foo"), "//host/bar/foo");
  ComparePaths(Append("//host", "/foo"), "//host/foo");
  ComparePaths(Append("//host", "//other/foo"), "//other/foo");
  ComparePaths(Append("//host/bar", "//other/foo"), "//other/foo");

  ComparePaths(Append("c:/foo", "/bar"), "c:/bar");

  // path("c:") / ""; // yields "c:"
  ComparePaths(Append("c:", ""), "c:");

  // path("c:foo") / "/bar"; // yields "c:/bar"
  ComparePaths(Append("c:foo", "/bar"), "c:/bar");

  // path("c:foo") / "c:bar"; // yields "c:foo/bar"
  ComparePaths(Append("c:foo", "c:bar"), "c:foo/bar");

  // path("foo") / "c:/bar"; // yields "c:/bar"
  ComparePaths(Append("foo", "c:/bar"), "c:/bar");

  // path("foo") / "c:"; // yields "c:"
  ComparePaths(Append("foo", "c:"), "c:");
#endif  // ASAP_WINDOWS
}

// path::operator/=(const Source& source)
// path::Append(const Source& source)
// Equivalent to: return operator/=(path(source));

// path::Append(InputIterator first, InputIterator last)
// Equivalent to: return operator/=(path(first, last));

template <typename Char>
void test(const fs::path& p, const Char* s) {
  fs::path expected = p;
  expected /= fs::path(s);

  fs::path oper = p;
  oper /= s;

  fs::path func = p;
  func.append(s);

  std::vector<char> input_range(s, s + std::char_traits<Char>::length(s));
  fs::path range = p;
  range.append(input_range.begin(), input_range.end());

  ComparePaths(oper, expected);
  ComparePaths(func, expected);
  ComparePaths(range, expected);
}

TEST_CASE("Path / append / source", "[common][filesystem][fs::path][append]") {
  test("/foo/bar", "/foo/");

  test("baz", "baz");
  test("baz/", "baz");
  test("baz", "/foo/bar");
  test("baz/", "/foo/bar");

  test("", "");
  test("", "rel");

  test("dir/", "/file");
  test("dir/", "file");

  // C++17 [fs.fs::path.append] p4
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
          "[common][filesystem][fs::path][append]") {
  for (const fs::path p : TEST_PATHS()) {
    for (const fs::path q : TEST_PATHS()) {
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
