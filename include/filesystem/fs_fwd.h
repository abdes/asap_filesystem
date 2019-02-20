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

enum class file_type : signed char {
  none = 0,        // file status has not been evaluated yet, or an error
                   // occurred when evaluating it
  not_found = -1,  // file was not found (this is not considered an error)
  regular = 1,     // a regular file
  directory = 2,   // a directory
  symlink = 3,     // a symbolic link
  block = 4,       // a block special file
  character = 5,   // a character special file
  fifo = 6,        // a FIFO (also known as pipe) file
  socket = 7,      // a socket file
  unknown = 8      // the file exists but its type could not be determined
#if defined(ASAP_WINDOWS)
  ,
  reparse_file = 100
#endif
};

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

/// Bitmask type
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
