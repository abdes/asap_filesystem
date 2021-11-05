//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <type_traits>
namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               copy_options
// -----------------------------------------------------------------------------

/*!
@brief This type represents available options that control the behavior of
the copy() and copy_file() function.

copy_options satisfies the requirements of BitmaskType (which means the
bitwise operators operator&, operator|, operator^, operator~, operator&=,
operator|=, and operator^= are defined for this type). none represents the
empty bitmask; every other enumerator represents a distinct bitmask element.

@see https://en.cppreference.com/w/cpp/filesystem/copy_options
*/
enum class copy_options : unsigned short {
  none = 0,
  skip_existing = 1,
  overwrite_existing = 2,
  update_existing = 4,
  recursive = 8,
  copy_symlinks = 16,
  skip_symlinks = 32,
  directories_only = 64,
  create_symlinks = 128,
  create_hard_links = 256
};

constexpr auto operator&(copy_options lhs, copy_options rhs) noexcept
    -> copy_options {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) &
                                   static_cast<utype>(rhs));
}

constexpr auto operator|(copy_options lhs, copy_options rhs) noexcept
    -> copy_options {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) |
                                   static_cast<utype>(rhs));
}

constexpr auto operator^(copy_options lhs, copy_options rhs) noexcept
    -> copy_options {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) ^
                                   static_cast<utype>(rhs));
}

constexpr auto operator~(copy_options lhs) noexcept -> copy_options {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(~static_cast<utype>(lhs));
}

inline auto operator&=(copy_options &lhs, copy_options rhs) noexcept
    -> copy_options & {
  return lhs = lhs & rhs;
}

inline auto operator|=(copy_options &lhs, copy_options rhs) noexcept
    -> copy_options & {
  return lhs = lhs | rhs;
}

inline auto operator^=(copy_options &lhs, copy_options rhs) noexcept
    -> copy_options & {
  return lhs = lhs ^ rhs;
}

}  // namespace filesystem
}  // namespace asap
