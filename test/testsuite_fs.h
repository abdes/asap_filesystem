//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/fs_path.h>

using asap::filesystem::path;

#define PATH_CHK(p1, p2, fn) REQUIRE(p1.fn() == p2.fn())

namespace testing {

inline void ComparePaths(const path &p1, const path &p2) {
  PATH_CHK(p1, p2, native);
  PATH_CHK(p1, p2, string);
  PATH_CHK(p1, p2, empty);
  PATH_CHK(p1, p2, has_root_path);
  PATH_CHK(p1, p2, has_root_name);
  PATH_CHK(p1, p2, has_root_directory);
  PATH_CHK(p1, p2, has_relative_path);
  PATH_CHK(p1, p2, has_parent_path);
  PATH_CHK(p1, p2, has_filename);
  PATH_CHK(p1, p2, has_stem);
  PATH_CHK(p1, p2, has_extension);
  PATH_CHK(p1, p2, is_absolute);
  PATH_CHK(p1, p2, is_relative);
  auto d1 = std::distance(p1.begin(), p1.end());
  auto d2 = std::distance(p2.begin(), p2.end());
  REQUIRE(d1 == d2);
}

const std::string TEST_PATHS[] = {
    "",     "/",     "//",       "/.",     "/./",    "/a",
    "/a/",  "/a//",  "/a/b/c/d", "/a//b",  "a",      "a/b",
    "a/b/", "a/b/c", "a/b/c.d",  "a/b/..", "a/b/c.", "a/b/.c"};

}  // namespace testing
