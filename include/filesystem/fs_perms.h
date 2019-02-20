//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               perms
// -----------------------------------------------------------------------------

enum class perms : unsigned {
  none = 0,
  owner_read = 0400,
  owner_write = 0200,
  owner_exec = 0100,
  owner_all = 0700,
  group_read = 040,
  group_write = 020,
  group_exec = 010,
  group_all = 070,
  others_read = 04,
  others_write = 02,
  others_exec = 01,
  others_all = 07,
  all = 0777,
  set_uid = 04000,
  set_gid = 00,
  sticky_bit = 00,
  mask = 07777,
  unknown = 0xFFFF,
};

constexpr perms operator&(perms lhs, perms rhs) noexcept {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) & static_cast<utype>(rhs));
}

constexpr perms operator|(perms lhs, perms rhs) noexcept {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) | static_cast<utype>(rhs));
}

constexpr perms operator^(perms lhs, perms rhs) noexcept {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) ^ static_cast<utype>(rhs));
}

constexpr perms operator~(perms lhs) noexcept {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(~static_cast<utype>(lhs));
}

inline perms &operator&=(perms &lhs, perms rhs) noexcept {
  return lhs = lhs & rhs;
}

inline perms &operator|=(perms &lhs, perms rhs) noexcept {
  return lhs = lhs | rhs;
}

inline perms &operator^=(perms &lhs, perms rhs) noexcept {
  return lhs = lhs ^ rhs;
}

// -----------------------------------------------------------------------------
//                               perms_options
// -----------------------------------------------------------------------------

enum class perm_options : unsigned {
  replace = 0x1,
  add = 0x2,
  remove = 0x4,
  nofollow = 0x8
};

constexpr perm_options operator&(perm_options lhs, perm_options rhs) noexcept {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) &
                                   static_cast<utype>(rhs));
}

constexpr perm_options operator|(perm_options lhs, perm_options rhs) noexcept {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) |
                                   static_cast<utype>(rhs));
}

constexpr perm_options operator^(perm_options lhs, perm_options rhs) noexcept {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) ^
                                   static_cast<utype>(rhs));
}

constexpr perm_options operator~(perm_options lhs) noexcept {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(~static_cast<utype>(lhs));
}

inline perm_options &operator&=(perm_options &lhs, perm_options rhs) noexcept {
  return lhs = lhs & rhs;
}

inline perm_options &operator|=(perm_options &lhs, perm_options rhs) noexcept {
  return lhs = lhs | rhs;
}

inline perm_options &operator^=(perm_options &lhs, perm_options rhs) noexcept {
  return lhs = lhs ^ rhs;
}


}  // namespace filesystem
}  // namespace asap
