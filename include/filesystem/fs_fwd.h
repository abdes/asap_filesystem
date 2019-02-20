//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <chrono>
#include <cstdint>
#include <system_error>  // for std::std::error_code
#include <type_traits>   // for std::underlying_type

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               classes
// -----------------------------------------------------------------------------

class path;
class filesystem_error;
class directory_entry;
class directory_iterator;
class recursive_directory_iterator;
class file_status;


// -----------------------------------------------------------------------------

enum class directory_options : unsigned char {
  none = 0,
  follow_directory_symlink = 1,
  skip_permission_denied = 2
};

constexpr directory_options operator&(directory_options lhs,
                                      directory_options rhs) noexcept {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) &
                                        static_cast<utype>(rhs));
}

constexpr directory_options operator|(directory_options lhs,
                                      directory_options rhs) noexcept {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) |
                                        static_cast<utype>(rhs));
}

constexpr directory_options operator^(directory_options lhs,
                                      directory_options rhs) noexcept {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(static_cast<utype>(lhs) ^
                                        static_cast<utype>(rhs));
}

constexpr directory_options operator~(directory_options lhs) noexcept {
  using utype = typename std::underlying_type<directory_options>::type;
  return static_cast<directory_options>(~static_cast<utype>(lhs));
}

inline directory_options &operator&=(directory_options &lhs,
                                     directory_options rhs) noexcept {
  return lhs = lhs & rhs;
}

inline directory_options &operator|=(directory_options &lhs,
                                     directory_options rhs) noexcept {
  return lhs = lhs | rhs;
}

inline directory_options &operator^=(directory_options &lhs,
                                     directory_options rhs) noexcept {
  return lhs = lhs ^ rhs;
}

// -----------------------------------------------------------------------------

using file_time_type = std::chrono::system_clock::time_point;

}  // namespace filesystem
}  // namespace asap
