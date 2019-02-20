//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               copy_options
// -----------------------------------------------------------------------------

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

constexpr copy_options operator&(copy_options lhs, copy_options rhs) noexcept {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) &
                                   static_cast<utype>(rhs));
}

constexpr copy_options operator|(copy_options lhs, copy_options rhs) noexcept {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) |
                                   static_cast<utype>(rhs));
}

constexpr copy_options operator^(copy_options lhs, copy_options rhs) noexcept {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(static_cast<utype>(lhs) ^
                                   static_cast<utype>(rhs));
}

constexpr copy_options operator~(copy_options lhs) noexcept {
  using utype = typename std::underlying_type<copy_options>::type;
  return static_cast<copy_options>(~static_cast<utype>(lhs));
}

inline copy_options &operator&=(copy_options &lhs, copy_options rhs) noexcept {
  return lhs = lhs & rhs;
}

inline copy_options &operator|=(copy_options &lhs, copy_options rhs) noexcept {
  return lhs = lhs | rhs;
}

inline copy_options &operator^=(copy_options &lhs, copy_options rhs) noexcept {
  return lhs = lhs ^ rhs;
}

}  // namespace filesystem
}  // namespace asap
