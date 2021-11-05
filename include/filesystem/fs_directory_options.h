//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <type_traits>
namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               directory_options
// -----------------------------------------------------------------------------

/*!
@brief This type represents available options that control the behavior of the
directory_iterator and recursive_directory_iterator.

directory_options satisfies the requirements of BitmaskType (which means the
bitwise operators operator&, operator|, operator^, operator~, operator&=,
operator|=, and operator^= are defined for this type). none represents the empty
bitmask; every other enumerator represents a distinct bitmask element.

@see https://en.cppreference.com/w/cpp/filesystem/directory_options
*/
enum class directory_options : unsigned char {
  none = 0,
  follow_directory_symlink = 1,
  skip_permission_denied = 2
};

constexpr auto operator&(directory_options lhs, directory_options rhs) noexcept
    -> directory_options {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) &
                                        static_cast<utype>(rhs));
}

constexpr auto operator|(directory_options lhs, directory_options rhs) noexcept
    -> directory_options {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) |
                                        static_cast<utype>(rhs));
}

constexpr auto operator^(directory_options lhs, directory_options rhs) noexcept
    -> directory_options {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) ^
                                        static_cast<utype>(rhs));
}

constexpr auto operator~(directory_options lhs) noexcept -> directory_options {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(~static_cast<utype>(lhs));
}

inline auto operator&=(directory_options &lhs, directory_options rhs) noexcept
    -> directory_options & {
  return lhs = lhs & rhs;
}

inline auto operator|=(directory_options &lhs, directory_options rhs) noexcept
    -> directory_options & {
  return lhs = lhs | rhs;
}

inline auto operator^=(directory_options &lhs, directory_options rhs) noexcept
    -> directory_options & {
  return lhs = lhs ^ rhs;
}

}  // namespace filesystem
}  // namespace asap
