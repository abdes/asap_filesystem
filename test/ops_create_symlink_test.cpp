//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <fstream>

#include "fs_testsuite.h"

using testing::ComparePaths;

#if defined(ASAP_WINDOWS)
// Symbolic links without privilege escalation require developer mode and
// windows creator update version (10.0 build 1703).
//
// "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock"
// AllowDevelopmentWithoutDevLicense -PropertyType DWORD -Value 1

bool IsDeveloperModeEnabled() {
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

// -----------------------------------------------------------------------------
//  create_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_symlink / empty",
          "[common][filesystem][ops][create_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test empty path.
  fs::path p;
  create_symlink(tgt, p, ec);
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(create_symlink(tgt, p), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, tgt, p));
}

TEST_CASE("Ops / create_symlink", "[common][filesystem][ops][create_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec, ec2;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test non-existent path
  auto p = testing::nonexistent_path();
  REQUIRE(!exists(p));

  ec = bad_ec;
  create_symlink(tgt, p, ec);  // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));
  remove(p);
  create_symlink(tgt, p);  // create the symlink again
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));

  ec.clear();
  create_symlink(tgt, p, ec);  // Try to create existing symlink
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(create_symlink(tgt, p), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, tgt, p));

  remove(p);
}

// -----------------------------------------------------------------------------
//  create_directory_symlink
// -----------------------------------------------------------------------------

TEST_CASE("Ops / create_directory_symlink / empty",
          "[common][filesystem][ops][create_directory_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  std::error_code ec;
  testing::scoped_file f;
  auto tgt = f.path_;

  // Test empty path.
  fs::path p;
  create_directory_symlink(tgt, p, ec);
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(create_directory_symlink(tgt, p), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, tgt, p));
}

TEST_CASE("Ops / create_directory_symlink",
          "[common][filesystem][ops][create_directory_symlink]") {
#if defined(ASAP_WINDOWS)
  if (!IsDeveloperModeEnabled()) return;
#endif  // ASAP_WINDOWS
  const std::error_code bad_ec = make_error_code(std::errc::invalid_argument);
  std::error_code ec, ec2;
  testing::scoped_file f;

  fs::path tgt = testing::nonexistent_path();
  fs::create_directory(tgt);
  testing::scoped_file d(tgt, testing::scoped_file::adopt_file);

  // Test non-existent path
  auto p = testing::nonexistent_path();
  REQUIRE(!exists(p));

  ec = bad_ec;
  create_directory_symlink(tgt, p, ec);  // create the symlink once
  REQUIRE(!ec);
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));
  remove(p);
  create_directory_symlink(tgt, p);  // create the symlink again
  REQUIRE(exists(p));
  REQUIRE(is_symlink(p));

  ec.clear();
  create_symlink(tgt, p, ec);  // Try to create existing symlink
  REQUIRE(ec);
  REQUIRE_THROWS_MATCHES(create_symlink(tgt, p), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec, tgt, p));

  remove(p);
}
