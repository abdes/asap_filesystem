//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

// clang-format off
#include <common/platform.h>
#include <common/config.h>
#include "catch2/catch.hpp"
#if defined(ASAP_POSIX)
# if !defined(ASAP_APPLE) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE ASAP_POSIX_LEVEL  // Request POSIX api
# endif
# include <unistd.h>
#endif
// clang-format on

#if defined(ASAP_WINDOWS)
#include <Windows.h>
#endif

#include <filesystem/filesystem.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <utility>

namespace fs = asap::filesystem;
using asap::filesystem::path;

#define PATH_CHK(p1, p2, fn) REQUIRE((p1).fn() == (p2).fn())

namespace testing {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif  // __clang__

// Catch2 matcher for filesystem_error
class FilesystemErrorMatcher : public Catch::MatcherBase<fs::filesystem_error> {
  std::error_code ec_;
  path p1_;
  path p2_;

 public:
  explicit FilesystemErrorMatcher(std::error_code ec) : ec_(ec) {}
  FilesystemErrorMatcher(std::error_code ec, path p1)
      : ec_(ec), p1_(std::move(p1)) {}
  FilesystemErrorMatcher(std::error_code ec, path p1, path p2)
      : ec_(ec), p1_(std::move(p1)), p2_(std::move(p2)) {}

  // Performs the test for this matcher
  auto match(fs::filesystem_error const &error) const -> bool override {
    return (error.code() == ec_) && (error.path1() == p1_) &&
           (error.path2() == p2_);
  }

  // Produces a string describing what this matcher does. It should
  // include any provided data (the begin/ end in this case) and
  // be written as if it were stating a fact (in the output it will be
  // preceded by the value under test).
  auto describe() const -> std::string override {
    std::ostringstream ss;
    ss << "error code " << ec_;
    if (!p1_.empty()) {
      ss << ", path1 '" << p1_.string() << "'";
    }
    if (!p2_.empty()) {
      ss << ", path2 '" << p2_.string() << "'";
    }
    return ss.str();
  }
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__

// The builder functions
inline auto FilesystemErrorDetail(std::error_code ec)
    -> FilesystemErrorMatcher {
  return FilesystemErrorMatcher(ec);
}
inline auto FilesystemErrorDetail(std::error_code ec, path p1)
    -> FilesystemErrorMatcher {
  return {ec, std::move(p1)};
}
inline auto FilesystemErrorDetail(std::error_code ec, path p1, path p2)
    -> FilesystemErrorMatcher {
  return {ec, std::move(p1), std::move(p2)};
}

inline void ComparePaths(const path &p1, const path &p2) {
  CAPTURE(p1);
  CAPTURE(p2);
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
  CHECK(d1 == d2);
}

inline auto TEST_PATHS() -> std::vector<std::string> {
  return {"",     "/",     "//",       "/.",     "/./",    "/a",
          "/a/",  "/a//",  "/a/b/c/d", "/a//b",  "a",      "a/b",
          "a/b/", "a/b/c", "a/b/c.d",  "a/b/..", "a/b/c.", "a/b/.c"};
}

inline auto root_path() -> path {
#if defined(ASAP_WINDOWS)
  return "c:/";
#else
  return {"/"};
#endif
}

// This is NOT supposed to be a secure way to get a unique name!
// We just need a path that doesn't exist for testing purposes.
inline auto nonexistent_path() -> path {
  path p;
#if defined(ASAP_POSIX)
  char tmp[] = "filesystem-test.XXXXXX";
  int fd = ::mkstemp(tmp);
  if (fd == -1) {
    throw fs::filesystem_error("mkstemp failed",
                               std::error_code(errno, std::generic_category()));
  }
  ::unlink(tmp);
  ::close(fd);
  p = tmp;
#else
  char buf[64];
  static int counter;
#if defined(ASAP_WINDOWS)
  unsigned long pid = static_cast<unsigned long>(GetCurrentProcessId());
#else
  unsigned long pid = static_cast<unsigned long>(::getpid());
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

  scoped_file(path p, adopt_file_t /*unused*/) : path_(std::move(p)) {}

  ~scoped_file() {
    if (!path_.empty()) {
      std::error_code ec;
      fs::permissions(path_, fs::perms::owner_all, ec);
      if (fs::is_directory(path_)) {
        fs::remove_all(path_, ec);
      } else {
        fs::remove(path_, ec);
      }
    }
  }

  path path_{};
};

#if defined(ASAP_WINDOWS)
// Symbolic links without privilege escalation require developer mode and
// windows creator update version (10.0 build 1703).
//
// "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock"
// AllowDevelopmentWithoutDevLicense -PropertyType DWORD -Value 1

inline bool IsDeveloperModeEnabled() {
  HKEY hKey;
  LSTATUS lResult = ::RegOpenKeyExW(
      HKEY_LOCAL_MACHINE,
      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0,
      KEY_READ | KEY_WOW64_64KEY, &hKey);
  if (lResult == ERROR_SUCCESS) {
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nValue(0);
    lResult =
        ::RegQueryValueExW(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL,
                           reinterpret_cast<LPBYTE>(&nValue), &dwBufferSize);
    if (lResult == ERROR_SUCCESS) {
      return (nValue != 0);
    }
  }
  return false;
}
#endif  // ASAP_WINDOWS

}  // namespace testing
