//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include "fs_testsuite.h"

using testing::TEST_PATHS;

// -----------------------------------------------------------------------------
//  DECOMPOSE
// -----------------------------------------------------------------------------

TEST_CASE("Path / decompose / extension",
          "[common][filesystem][path][decompose]") {
  REQUIRE(path("/foo/bar.txt").extension() == path(".txt"));
  REQUIRE(path("/foo/bar.baz.txt").extension() == path(".txt"));
  REQUIRE(path(".bar.baz.txt").extension() == path(".txt"));

  REQUIRE(path(".profile").extension() == path(""));
  REQUIRE(path(".profile.old").extension() == path(".old"));
  REQUIRE(path("..abc").extension() == path(".abc"));
  REQUIRE(path("...abc").extension() == path(".abc"));
  REQUIRE(path("abc..def").extension() == path(".def"));
  REQUIRE(path("abc...def").extension() == path(".def"));
  REQUIRE(path("abc.").extension() == path("."));
  REQUIRE(path("abc..").extension() == path("."));
  REQUIRE(path("abc.d.").extension() == path("."));
  REQUIRE(path("..").extension() == path(""));
  REQUIRE(path(".").extension() == path(""));

  REQUIRE(path().extension() == path());
}

TEST_CASE("Path / decompose / stem + extension = filename",
          "[common][filesystem][path][decompose]") {
  for (const path p : TEST_PATHS) {
    auto stem = p.stem();
    auto ext = p.extension();
    auto file = p.filename();
    CAPTURE(p);
    CAPTURE(stem);
    CAPTURE(ext);
    CAPTURE(file);
    REQUIRE(stem.native() + ext.native() == file.native());
  }
}

TEST_CASE("Path / decompose / filename",
          "[common][filesystem][path][decompose]") {
  for (const path p : TEST_PATHS) {
    CAPTURE(p);
    path f = p.filename();
    if (p.empty())
      REQUIRE(f.empty());
    else {
      const path back = *--p.end();
      if (back.has_root_path()) {
        REQUIRE(f.empty());
      }
    }
  }
}

TEST_CASE("Path / decompose / filename special cases",
          "[common][filesystem][path][decompose]") {
  // [fs.path.decompose] p7
  REQUIRE(path("/foo/bar.txt").filename() == "bar.txt");
  REQUIRE(path("/foo/bar").filename() == "bar");
  REQUIRE(path("/foo/bar/").filename() == "");
  REQUIRE(path("/").filename() == "");
#ifdef ASAP_WINDOWS
  REQUIRE(path("//host").filename() == "");
#else
  REQUIRE(path("//host").filename() == "host");
#endif
  REQUIRE(path(".").filename() == ".");
  REQUIRE(path("..").filename() == "..");
}

TEST_CASE("Path / decompose / parent_path basic",
          "[common][filesystem][path][decompose]") {
  REQUIRE(path("/var/tmp/example.txt").parent_path() == path("/var/tmp"));
  REQUIRE(path("/var/tmp/.").parent_path() == path("/var/tmp"));
  REQUIRE(path("/var").parent_path() == path("/"));
  REQUIRE(path("/").parent_path() == path("/"));
  REQUIRE(path("//").parent_path() == path("/"));
  REQUIRE(path("foo").parent_path() == path(""));
  REQUIRE(path("foo/bar").parent_path() == path("foo"));
  REQUIRE(path("/foo/bar").parent_path() == path("/foo"));
  REQUIRE(path("/foo/").parent_path() == path("/"));
  REQUIRE(path("/foo/bar/.").parent_path() == path("/foo/bar"));
#ifdef ASAP_WINDOWS
  REQUIRE(path("//foo").parent_path() == path("//foo"));
  REQUIRE(path("c:").parent_path() == "c:");
  REQUIRE(path("c:\\").parent_path() == "c:\\");
  REQUIRE(path("c:\\foo").parent_path() == "c:\\");
  REQUIRE(path("c:/foo").parent_path() == "c:/");
#endif
}

TEST_CASE("Path / decompose / relative_path basic",
          "[common][filesystem][path][decompose]") {
  path p1 = "foo";
  REQUIRE(p1.relative_path() == p1);
  path p2 = "foo/bar";
  REQUIRE(p2.relative_path() == p2);
  path p3 = "/foo/bar";
  REQUIRE(p3.relative_path() == p2);
}

TEST_CASE("Path / decompose / relative_path",
          "[common][filesystem][path][decompose]") {
  for (const path p : TEST_PATHS) {
    bool after_root = false;
    const path prel = p.relative_path();
    REQUIRE(!prel.has_root_name());
    path rel;
    for (const auto &cmpt : p) {
      if (!cmpt.has_root_path()) after_root = true;
      if (after_root) rel /= cmpt;
    }
    if (prel != rel) std::cout << prel << ' ' << rel << '\n';
    REQUIRE(prel == rel);
  }
}

TEST_CASE("Path / decompose / root_directory special cases",
          "[common][filesystem][path][decompose]") {
  path p1 = "foo/bar";
  REQUIRE(p1.root_directory() == path());
  path p2 = "/foo/bar";
  REQUIRE(p2.root_directory() == path("/"));
  path p3 = "//foo";
#if !defined(ASAP_WINDOWS)
  REQUIRE(p3.root_directory() == path("/"));
#else
  REQUIRE(p3.root_directory() == path());
#endif
  path p4 = "///foo";
  REQUIRE(p4.root_directory() == path("/"));
  path p5 = "//";
  REQUIRE(p5.root_directory() == path("/"));
  path p6 = "/";
  REQUIRE(p6.root_directory() == path("/"));

#if defined(ASAP_WINDOWS)
  REQUIRE(path("c:/").root_directory() == path("/"));
  REQUIRE(path("c:/foo//").root_directory() == path("/"));
  REQUIRE(path("c:foo").root_directory() == path(""));
  REQUIRE(path("c:foo/").root_directory() == path(""));
  REQUIRE(path("c:foo/bar//").root_directory() == path(""));
  REQUIRE(path("//host/").root_directory() == path("/"));
  REQUIRE(path("//host/foo").root_directory() == path("/"));
#endif
}

TEST_CASE("Path / decompose / root_directory",
          "[common][filesystem][path][decompose]") {
  for (const path p : TEST_PATHS) {
    CAPTURE(p);
    path rootdir = p.root_directory();
    REQUIRE(!rootdir.has_relative_path());
    if (!rootdir.empty()) REQUIRE(rootdir.string() == path("/"));
  }
}

TEST_CASE("Path / decompose / root_name",
          "[common][filesystem][path][decompose]") {
#ifdef ASAP_WINDOWS
  REQUIRE(path("//").root_name().empty());
  REQUIRE(path("//foo").root_name() == "//foo");
  REQUIRE(path("//foo/bar").root_name() == "//foo");
  REQUIRE(path("///foo").root_name().empty());
  REQUIRE(path("c:/foo").root_name() == "c:");
  REQUIRE(path("c:foo").root_name() == "c:");
#else
  REQUIRE(path("//").root_name().empty());
  REQUIRE(path("//foo").root_name().empty());
  REQUIRE(path("//foo/bar").root_name().empty());
  REQUIRE(path("///foo").root_name().empty());
  REQUIRE(path("c:/foo").root_name().empty());
  REQUIRE(path("c:foo").root_name().empty());
#endif
}

TEST_CASE("Path / decompose / root_path basic",
          "[common][filesystem][path][decompose]") {
  path p1 = "foo/bar";
  REQUIRE(p1.root_path() == path());
  path p2 = "/foo/bar";
  REQUIRE(p2.root_path() == path("/"));
  path p3 = "//foo/bar";
#ifdef ASAP_WINDOWS
  REQUIRE(p3.root_path() == path("//foo/"));
#else
  REQUIRE(p3.root_path() == path("/"));
#endif
#ifdef ASAP_WINDOWS
  path p4 = "c:/foo/bar";
  REQUIRE(p4.root_path() == path("c:/"));
#endif
}

TEST_CASE("Path / decompose / stem", "[common][filesystem][path][decompose]") {
  REQUIRE(path("/foo/bar.txt").stem() == path("bar"));
  path p = "foo.bar.baz.tar";
  std::vector<std::string> v;
  for (; !p.extension().empty(); p = p.stem())
    v.push_back(p.extension().string());
  REQUIRE(v.at(0) == ".tar");
  REQUIRE(v.at(1) == ".baz");
  REQUIRE(v.at(2) == ".bar");

  REQUIRE(path(".profile").stem() == path(".profile"));
  REQUIRE(path(".profile.old").stem() == path(".profile"));
  REQUIRE(path("..abc").stem() == path("."));
  REQUIRE(path("...abc").stem() == path(".."));
  REQUIRE(path("abc..def").stem() == path("abc."));
  REQUIRE(path("abc...def").stem() == path("abc.."));
  REQUIRE(path("abc.").stem() == path("abc"));
  REQUIRE(path("abc..").stem() == path("abc."));
  REQUIRE(path("abc.d.").stem() == path("abc.d"));
  REQUIRE(path("..").stem() == path(".."));
  REQUIRE(path(".").stem() == path("."));
  REQUIRE(path("/").stem() == path());

  REQUIRE(path().stem() == path());
}
