//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <common/platform.h>

#if defined(ASAP_WINDOWS)
# include <Windows.h>
#endif

#if defined(ASAP_POSIX)
# include <unistd.h>
#endif

#include <cstdio>
#include <fstream>

#include <filesystem/filesystem.h>

namespace fs = asap::filesystem;
using asap::filesystem::path;

#define PATH_CHK(p1, p2, fn) REQUIRE((p1).fn() == (p2).fn())

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

inline path root_path() {
#if defined(ASAP_WINDOWS)
  return "c:/";
#else
  return "/";
#endif
}

// This is NOT supposed to be a secure way to get a unique name!
// We just need a path that doesn't exist for testing purposes.
inline path nonexistent_path() {
  path p;
#if defined(_GNU_SOURCE) || _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200112L
  char tmp[] = "filesystem-test.XXXXXX";
  int fd = ::mkstemp(tmp);
  if (fd == -1)
    throw fs::filesystem_error("mkstemp failed",
                               std::error_code(errno, std::generic_category()));
  ::unlink(tmp);
  ::close(fd);
  p = tmp;
#else
  char buf[64];
  static int counter;
#if defined(ASAP_WINDOWS)
  unsigned long pid = static_cast<unsigned long>(GetCurrentProcessId());
#elif defined(ASAP_POSIX)
  unsigned long pid = static_cast<unsigned long>(::getpid());
#else
  ASAP_ASSERT_FAIL("")
#endif
  std::snprintf(buf, 64, "filesystem-test.%d.%lu", counter++, pid);
  p = buf;
#endif
  return p;
}

// RAII helper to remove a file on scope exit.
struct scoped_file {
  enum adopt_file_t { adopt_file };

  explicit scoped_file(const path &p = nonexistent_path()) : path_(p) {
    std::ofstream{p.c_str()};
  }

  scoped_file(path p, adopt_file_t) : path_(p) {}

  ~scoped_file() {
    if (!path_.empty()) fs::remove(path_);
  }

  path path_{};
};

}  // namespace testing
