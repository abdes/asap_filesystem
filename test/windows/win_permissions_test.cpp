//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <catch2/catch.hpp>
#include <random>

#include "../fs_testsuite.h"

namespace asap {
namespace filesystem {
namespace detail {
namespace win32_port {

enum class TrusteeType { OWNER, GROUP, OTHERS };
perms MapAccessMaskToPerms(ACCESS_MASK, TrusteeType, file_type);

}  // namespace win32_port
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

using asap::filesystem::detail::win32_port::MapAccessMaskToPerms;
using asap::filesystem::detail::win32_port::TrusteeType;

// -----------------------------------------------------------------------------
//  dir_iterator
// -----------------------------------------------------------------------------

void TestMappingWithBadType(TrusteeType trustee) {
  std::mt19937 rng;
  rng.seed(std::random_device()());
  std::uniform_int_distribution<std::mt19937::result_type> dist;
  ACCESS_MASK mask = dist(rng);
  auto prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::none);
  REQUIRE(prm == fs::perms::unknown);
  mask = dist(rng);
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::unknown);
  REQUIRE(prm == fs::perms::unknown);
  mask = dist(rng);
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::not_found);
  REQUIRE(prm == fs::perms::unknown);
}

TEST_CASE("Windows / Permissions / Mapping / Bad type",
          "[filesystem][windows][permissions]") {
  // OWNER
  TestMappingWithBadType(TrusteeType::OWNER);
  // GROUP
  TestMappingWithBadType(TrusteeType::GROUP);
  // OTHERS
  TestMappingWithBadType(TrusteeType::OTHERS);
}

void TestMappingFile(TrusteeType trustee) {
  ACCESS_MASK mask = FILE_GENERIC_EXECUTE;
  auto prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::regular);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_exec);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_exec);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_exec);
  }

  mask = FILE_GENERIC_READ;
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::regular);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_read);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_read);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_read);
  }

  mask = FILE_GENERIC_WRITE;
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::regular);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_write);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_write);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_write);
  }
}

TEST_CASE("Windows / Permissions / Mapping / file / owner",
          "[filesystem][windows][permissions]") {
  TestMappingFile(TrusteeType::OWNER);
}

TEST_CASE("Windows / Permissions / Mapping / file / group",
          "[filesystem][windows][permissions]") {
  TestMappingFile(TrusteeType::GROUP);
}

TEST_CASE("Windows / Permissions / Mapping / file / others",
          "[filesystem][windows][permissions]") {
  TestMappingFile(TrusteeType::OTHERS);
}

void TestMappingDirectory(TrusteeType trustee) {
  ACCESS_MASK mask = FILE_TRAVERSE | FILE_READ_ATTRIBUTES |
                     STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE;
  auto prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::directory);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_exec);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_exec);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_exec);
  }

  mask = FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_READ_EA |
         STANDARD_RIGHTS_READ | SYNCHRONIZE;
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::directory);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_read);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_read);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_read);
  }

  mask = FILE_ADD_SUBDIRECTORY | FILE_WRITE_ATTRIBUTES | FILE_ADD_FILE |
         FILE_WRITE_EA | STANDARD_RIGHTS_WRITE | SYNCHRONIZE |
         FILE_DELETE_CHILD;
  prm = MapAccessMaskToPerms(mask, trustee, fs::file_type::directory);
  switch (trustee) {
    case TrusteeType::OWNER:
      REQUIRE(prm == fs::perms::owner_write);
      break;
    case TrusteeType::GROUP:
      REQUIRE(prm == fs::perms::group_write);
      break;
    case TrusteeType::OTHERS:
      REQUIRE(prm == fs::perms::others_write);
  }
}

TEST_CASE("Windows / Permissions / Mapping / dir / owner",
          "[filesystem][windows][permissions]") {
  TestMappingDirectory(TrusteeType::OWNER);
}

TEST_CASE("Windows / Permissions / Mapping / dir / group",
          "[filesystem][windows][permissions]") {
  TestMappingDirectory(TrusteeType::GROUP);
}

TEST_CASE("Windows / Permissions / Mapping / dir / others",
          "[filesystem][windows][permissions]") {
  TestMappingDirectory(TrusteeType::OTHERS);
}

TEST_CASE("Windows / Permissions / Mapping / combo",
          "[filesystem][windows][permissions]") {
  ACCESS_MASK mask =
      FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE;
  auto prm =
      MapAccessMaskToPerms(mask, TrusteeType::OWNER, fs::file_type::regular);
  REQUIRE(prm == (fs::perms::owner_read | fs::perms::owner_write |
                  fs::perms::owner_exec));
}
