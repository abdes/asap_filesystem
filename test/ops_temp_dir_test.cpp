//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>

#include <stdlib.h>

#include "fs_testsuite.h"

void clean_env() {
#if defined(ASAP_WINDOWS)
  ::_putenv("TMP=");
  ::_putenv("TEMP=");
#else
  ::unsetenv("TMPDIR");
  ::unsetenv("TMP");
  ::unsetenv("TEMPDIR");
  ::unsetenv("TEMP");
#endif
}

bool set_env(const char* name, std::string value) {
#if defined(ASAP_WINDOWS)
  std::string s = name;
  s += '=';
  s += value;
  return !::_putenv(s.c_str());
#else
  return !::setenv(name, value.c_str(), 1);
#endif
}

// -----------------------------------------------------------------------------
//  temp_dir
// -----------------------------------------------------------------------------

TEST_CASE("Ops / temp_dir / default (/tmp)",
          "[common][filesystem][ops][temp_dir]") {
  clean_env();

  if (!fs::exists("/tmp")) return;  // just give up

  std::error_code ec = make_error_code(std::errc::invalid_argument);
  fs::path p1 = fs::temp_directory_path(ec);
  REQUIRE(!ec);
  REQUIRE(exists(p1));

  fs::path p2 = fs::temp_directory_path();
  REQUIRE(p1 == p2);
}

TEST_CASE("Ops / temp_dir / TMP", "[common][filesystem][ops][temp_dir]") {
  clean_env();

  if (!set_env("TMP", testing::nonexistent_path().string()))
    return;  // just give up

  std::error_code ec;
  fs::path p = fs::temp_directory_path(ec);
  REQUIRE(ec);
  REQUIRE(p == fs::path());

  REQUIRE_THROWS_MATCHES(fs::temp_directory_path(), fs::filesystem_error,
                         testing::FilesystemErrorDetail(ec));

  std::error_code ec2;
  try {
    p = fs::temp_directory_path();
  } catch (const fs::filesystem_error& e) {
    ec2 = e.code();
  }
  REQUIRE(ec2 == ec);
}

TEST_CASE("Ops / temp_dir / permission",
          "[common][filesystem][ops][temp_dir]") {
  auto p = testing::nonexistent_path();
  testing::scoped_file sp(p, testing::scoped_file::adopt_file);
  create_directories(p / "tmp");
  permissions(p, fs::perms::none);
#if defined(ASAP_WINDOWS)
  // Use TMP as it is the first env variable to be checked, making sure that
  // it will be the value used to return a temporary path
  set_env("TMP", (p / "tmp").string());
#else
  // Use TMPDIR as it is the first env variable to be checked, making sure that
  // it will be the value used to return a temporary path
  set_env("TMPDIR", (p / "tmp").string());
#endif
  std::error_code ec;
  auto r = fs::temp_directory_path(ec);
  REQUIRE(ec == std::make_error_code(std::errc::permission_denied));
  REQUIRE(r == fs::path());

  std::error_code ec2;
  try {
    fs::temp_directory_path();
  } catch (const fs::filesystem_error& e) {
    ec2 = e.code();
  }
  REQUIRE(ec2 == ec);

  permissions(p / "tmp", fs::perms::owner_all);
}

TEST_CASE("Ops / temp_dir / not a directory",
          "[common][filesystem][ops][temp_dir]") {
  testing::scoped_file f;
#if defined(ASAP_WINDOWS)
  // Use TMP as it is the first env variable to be checked, making sure that
  // it will be the value used to return a temporary path
  set_env("TMP", f.path_.string());
#else
  // Use TMPDIR as it is the first env variable to be checked, making sure that
  // it will be the value used to return a temporary path
  set_env("TMPDIR", f.path_.string());
#endif
  std::error_code ec;
  auto r = fs::temp_directory_path(ec);
  REQUIRE(ec == std::make_error_code(std::errc::not_a_directory));
  REQUIRE(r == fs::path());

  std::error_code ec2;
  try {
    fs::temp_directory_path();
  } catch (const fs::filesystem_error& e) {
    ec2 = e.code();
  }
  REQUIRE(ec2 == ec);
}
