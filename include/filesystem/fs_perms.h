//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <type_traits>
namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               perms
// -----------------------------------------------------------------------------

/*!
@brief This type represents file access permissions.

perms satisfies the requirements
of BitmaskType (which means the bitwise operators operator&, operator|,
operator^, operator~, operator&=, operator|=, and operator^= are defined for
this type).

@see https://en.cppreference.com/w/cpp/filesystem/perms
 */
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

constexpr auto operator&(perms lhs, perms rhs) noexcept -> perms {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) & static_cast<utype>(rhs));
}

constexpr auto operator|(perms lhs, perms rhs) noexcept -> perms {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) | static_cast<utype>(rhs));
}

constexpr auto operator^(perms lhs, perms rhs) noexcept -> perms {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<utype>(lhs) ^ static_cast<utype>(rhs));
}

constexpr auto operator~(perms lhs) noexcept -> perms {
  using utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(~static_cast<utype>(lhs));
}

inline auto operator&=(perms &lhs, perms rhs) noexcept -> perms & {
  return lhs = lhs & rhs;
}

inline auto operator|=(perms &lhs, perms rhs) noexcept -> perms & {
  return lhs = lhs | rhs;
}

inline auto operator^=(perms &lhs, perms rhs) noexcept -> perms & {
  return lhs = lhs ^ rhs;
}

// -----------------------------------------------------------------------------
//                               perms_options
// -----------------------------------------------------------------------------

/*!
@brief This type represents available options that control the behavior of the
function permissions().

perm_options satisfies the requirements of BitmaskType (which means the bitwise
operators operator&, operator|, operator^, operator~, operator&=, operator|=,
and operator^= are defined for this type).

@see https://en.cppreference.com/w/cpp/filesystem/perm_options
*/
enum class perm_options : unsigned {
  replace = 0x1,
  add = 0x2,
  remove = 0x4,
  nofollow = 0x8
};

constexpr auto operator&(perm_options lhs, perm_options rhs) noexcept
    -> perm_options {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) &
                                   static_cast<utype>(rhs));
}

constexpr auto operator|(perm_options lhs, perm_options rhs) noexcept
    -> perm_options {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) |
                                   static_cast<utype>(rhs));
}

constexpr auto operator^(perm_options lhs, perm_options rhs) noexcept
    -> perm_options {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(static_cast<utype>(lhs) ^
                                   static_cast<utype>(rhs));
}

constexpr auto operator~(perm_options lhs) noexcept -> perm_options {
  using utype = typename std::underlying_type<perm_options>::type;
  return static_cast<perm_options>(~static_cast<utype>(lhs));
}

inline auto operator&=(perm_options &lhs, perm_options rhs) noexcept
    -> perm_options & {
  return lhs = lhs & rhs;
}

inline auto operator|=(perm_options &lhs, perm_options rhs) noexcept
    -> perm_options & {
  return lhs = lhs | rhs;
}

inline auto operator^=(perm_options &lhs, perm_options rhs) noexcept
    -> perm_options & {
  return lhs = lhs ^ rhs;
}

}  // namespace filesystem
}  // namespace asap
