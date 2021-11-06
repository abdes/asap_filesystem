//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_copy_options.h>
#include <filesystem/fs_file_status.h>
#include <filesystem/fs_path.h>

#include "filesystem/fs_file_time_type.h"

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                                 types
// -----------------------------------------------------------------------------

struct space_info {
  std::uintmax_t capacity;
  std::uintmax_t free;
  std::uintmax_t available;
};

// -----------------------------------------------------------------------------
//                               operations
// -----------------------------------------------------------------------------

ASAP_FILESYSTEM_API
auto absolute_impl(const path &p, std::error_code *ec = nullptr) -> path;
ASAP_FILESYSTEM_API
auto canonical_impl(const path &p, std::error_code *ec = nullptr) -> path;
ASAP_FILESYSTEM_API
void copy_impl(const path &from, const path &to, copy_options opt,
               std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
auto copy_file_impl(const path &from, const path &to, copy_options opt,
                    std::error_code *ec = nullptr) -> bool;
ASAP_FILESYSTEM_API
void copy_symlink_impl(const path &existing_symlink, const path &new_symlink,
                       std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
auto create_directories_impl(const path &p, std::error_code *ec = nullptr)
    -> bool;
ASAP_FILESYSTEM_API
auto create_directory_impl(const path &p, std::error_code *ec = nullptr)
    -> bool;
ASAP_FILESYSTEM_API
auto create_directory_impl(const path &p, const path &existing_template,
                           std::error_code *ec = nullptr) -> bool;
ASAP_FILESYSTEM_API
void create_directory_symlink_impl(const path &target, const path &new_symlink,
                                   std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void create_hard_link_impl(const path &target, const path &new_hard_link,
                           std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void create_symlink_impl(const path &target, const path &new_symlink,
                         std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
auto current_path_impl(std::error_code *ec = nullptr) -> path;
ASAP_FILESYSTEM_API
void current_path_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
auto equivalent_impl(const path &p, const path &, std::error_code *ec = nullptr)
    -> bool;
ASAP_FILESYSTEM_API
auto file_size_impl(const path &p, std::error_code *ec = nullptr) -> uintmax_t;
ASAP_FILESYSTEM_API
auto hard_link_count_impl(const path &p, std::error_code *ec = nullptr)
    -> uintmax_t;
ASAP_FILESYSTEM_API
auto is_empty_impl(const path &p, std::error_code *ec = nullptr) -> bool;
ASAP_FILESYSTEM_API
auto last_write_time_impl(const path &p, std::error_code *ec = nullptr)
    -> file_time_type;
ASAP_FILESYSTEM_API
void last_write_time_impl(const path &p, file_time_type new_time,
                          std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void permissions_impl(const path &p, perms, perm_options,
                      std::error_code * = nullptr);
ASAP_FILESYSTEM_API
auto read_symlink_impl(const path &p, std::error_code *ec = nullptr) -> path;
ASAP_FILESYSTEM_API
auto remove_impl(const path &p, std::error_code *ec = nullptr) -> bool;
ASAP_FILESYSTEM_API
auto remove_all_impl(const path &p, std::error_code *ec = nullptr) -> uintmax_t;
ASAP_FILESYSTEM_API
void rename_impl(const path &from, const path &to,
                 std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void resize_file_impl(const path &p, uintmax_t size,
                      std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
auto space_impl(const path &p, std::error_code *ec = nullptr) -> space_info;
ASAP_FILESYSTEM_API
auto status_impl(const path &p, std::error_code *ec = nullptr) -> file_status;
ASAP_FILESYSTEM_API
auto symlink_status_impl(const path &p, std::error_code *ec = nullptr)
    -> file_status;
ASAP_FILESYSTEM_API
auto temp_directory_path_impl(std::error_code *ec = nullptr) -> path;
ASAP_FILESYSTEM_API
auto weakly_canonical_impl(path const &p, std::error_code *ec = nullptr)
    -> path;

inline auto current_path() -> path { return current_path_impl(); }

inline auto current_path(std::error_code &ec) -> path {
  return current_path_impl(&ec);
}

inline void current_path(const path &p) { current_path_impl(p); }

inline void current_path(const path &p, std::error_code &ec) noexcept {
  current_path_impl(p, &ec);
}

inline auto absolute(const path &p) -> path { return absolute_impl(p); }

inline auto absolute(const path &p, std::error_code &ec) -> path {
  return absolute_impl(p, &ec);
}

inline auto canonical(const path &p) -> path { return canonical_impl(p); }

inline auto canonical(const path &p, std::error_code &ec) -> path {
  return canonical_impl(p, &ec);
}

inline void copy(const path &from, const path &to) {
  copy_impl(from, to, copy_options::none);
}

inline void copy(const path &from, const path &to, std::error_code &ec) {
  copy_impl(from, to, copy_options::none, &ec);
}

inline void copy(const path &from, const path &to, copy_options opt) {
  copy_impl(from, to, opt);
}

inline void copy(const path &from, const path &to, copy_options opt,
                 std::error_code &ec) {
  copy_impl(from, to, opt, &ec);
}

inline auto copy_file(const path &from, const path &to) -> bool {
  return copy_file_impl(from, to, copy_options::none);
}

inline auto copy_file(const path &from, const path &to, std::error_code &ec)
    -> bool {
  return copy_file_impl(from, to, copy_options::none, &ec);
}

inline auto copy_file(const path &from, const path &to, copy_options opt)
    -> bool {
  return copy_file_impl(from, to, opt);
}

inline auto copy_file(const path &from, const path &to, copy_options opt,
                      std::error_code &ec) -> bool {
  return copy_file_impl(from, to, opt, &ec);
}

inline void copy_symlink(const path &existing_symlink,
                         const path &new_symlink) {
  copy_symlink_impl(existing_symlink, new_symlink);
}

inline void copy_symlink(const path &existing_symlink, const path &new_symlink,
                         std::error_code &ec) noexcept {
  copy_symlink_impl(existing_symlink, new_symlink, &ec);
}

inline auto create_directories(const path &p) -> bool {
  return create_directories_impl(p);
}

inline auto create_directories(const path &p, std::error_code &ec) -> bool {
  return create_directories_impl(p, &ec);
}

inline auto create_directory(const path &p) -> bool {
  return create_directory_impl(p);
}

inline auto create_directory(const path &p, std::error_code &ec) noexcept
    -> bool {
  return create_directory_impl(p, &ec);
}

inline auto create_directory(const path &p, const path &attributes) -> bool {
  return create_directory_impl(p, attributes);
}

inline auto create_directory(const path &p, const path &attributes,
                             std::error_code &ec) noexcept -> bool {
  return create_directory_impl(p, attributes, &ec);
}

inline void create_directory_symlink(const path &to, const path &new_symlink) {
  create_directory_symlink_impl(to, new_symlink);
}

inline void create_directory_symlink(const path &to, const path &new_symlink,
                                     std::error_code &ec) noexcept {
  create_directory_symlink_impl(to, new_symlink, &ec);
}

inline void create_hard_link(const path &to, const path &new_hardlink) {
  create_hard_link_impl(to, new_hardlink);
}

inline void create_hard_link(const path &to, const path &new_hardlink,
                             std::error_code &ec) noexcept {
  create_hard_link_impl(to, new_hardlink, &ec);
}

inline void create_symlink(const path &to, const path &new_symlink) {
  create_symlink_impl(to, new_symlink);
}

inline void create_symlink(const path &to, const path &new_symlink,
                           std::error_code &ec) noexcept {
  return create_symlink_impl(to, new_symlink, &ec);
}

inline auto status_known(file_status status) noexcept -> bool {
  return status.type() != file_type::none;
}

inline auto exists(file_status status) noexcept -> bool {
  return status_known(status) && status.type() != file_type::not_found;
}

inline auto exists(const path &p) -> bool { return exists(status_impl(p)); }

inline auto exists(const path &p, std::error_code &ec) noexcept -> bool {
  auto status = status_impl(p, &ec);
  if (status_known(status)) {
    ec.clear();
  }
  return exists(status);
}

inline auto equivalent(const path &p1, const path &p2) -> bool {
  return equivalent_impl(p1, p2);
}

inline auto equivalent(const path &p1, const path &p2,
                       std::error_code &ec) noexcept -> bool {
  return equivalent_impl(p1, p2, &ec);
}

inline auto file_size(const path &p) -> uintmax_t { return file_size_impl(p); }

inline auto file_size(const path &p, std::error_code &ec) noexcept
    -> uintmax_t {
  return file_size_impl(p, &ec);
}

inline auto hard_link_count(const path &p) -> uintmax_t {
  return hard_link_count_impl(p);
}

inline auto hard_link_count(const path &p, std::error_code &ec) noexcept
    -> uintmax_t {
  return hard_link_count_impl(p, &ec);
}

inline auto is_block_file(file_status status) noexcept -> bool {
  return status.type() == file_type::block;
}

inline auto is_block_file(const path &p) -> bool {
  return is_block_file(status_impl(p));
}

inline auto is_block_file(const path &p, std::error_code &ec) noexcept -> bool {
  return is_block_file(status_impl(p, &ec));
}

inline auto is_character_file(file_status status) noexcept -> bool {
  return status.type() == file_type::character;
}

inline auto is_character_file(const path &p) -> bool {
  return is_character_file(status_impl(p));
}

inline auto is_character_file(const path &p, std::error_code &ec) noexcept
    -> bool {
  return is_character_file(status_impl(p, &ec));
}

inline auto is_directory(file_status status) noexcept -> bool {
  return status.type() == file_type::directory;
}

inline auto is_directory(const path &p) -> bool {
  return is_directory(status_impl(p));
}

inline auto is_directory(const path &p, std::error_code &ec) noexcept -> bool {
  return is_directory(status_impl(p, &ec));
}

inline auto is_empty(const path &p) -> bool { return is_empty_impl(p); }

inline auto is_empty(const path &p, std::error_code &ec) -> bool {
  return is_empty_impl(p, &ec);
}

inline auto is_fifo(file_status status) noexcept -> bool {
  return status.type() == file_type::fifo;
}
inline auto is_fifo(const path &p) -> bool { return is_fifo(status_impl(p)); }

inline auto is_fifo(const path &p, std::error_code &ec) noexcept -> bool {
  return is_fifo(status_impl(p, &ec));
}

inline auto is_regular_file(file_status status) noexcept -> bool {
  return status.type() == file_type::regular;
}

inline auto is_regular_file(const path &p) -> bool {
  return is_regular_file(status_impl(p));
}

inline auto is_regular_file(const path &p, std::error_code &ec) noexcept
    -> bool {
  return is_regular_file(status_impl(p, &ec));
}

inline auto is_socket(file_status status) noexcept -> bool {
  return status.type() == file_type::socket;
}

inline auto is_socket(const path &p) -> bool {
  return is_socket(status_impl(p));
}

inline auto is_socket(const path &p, std::error_code &ec) noexcept -> bool {
  return is_socket(status_impl(p, &ec));
}

inline auto is_symlink(file_status status) noexcept -> bool {
  return status.type() == file_type::symlink;
}

inline auto is_symlink(const path &p) -> bool {
  return is_symlink(symlink_status_impl(p));
}

inline auto is_symlink(const path &p, std::error_code &ec) noexcept -> bool {
  return is_symlink(symlink_status_impl(p, &ec));
}

inline auto is_other(file_status status) noexcept -> bool {
  return exists(status) && !is_regular_file(status) && !is_directory(status) &&
         !is_symlink(status);
}

inline auto is_other(const path &p) -> bool { return is_other(status_impl(p)); }

inline auto is_other(const path &p, std::error_code &ec) noexcept -> bool {
  return is_other(status_impl(p, &ec));
}

inline auto last_write_time(const path &p) -> file_time_type {
  return last_write_time_impl(p);
}

inline auto last_write_time(const path &p, std::error_code &ec) noexcept
    -> file_time_type {
  return last_write_time_impl(p, &ec);
}

inline void last_write_time(const path &p, file_time_type file_time) {
  last_write_time_impl(p, file_time);
}

inline void last_write_time(const path &p, file_time_type file_time,
                            std::error_code &ec) noexcept {
  last_write_time_impl(p, file_time, &ec);
}

inline void permissions(const path &p, perms permissions,
                        perm_options options = perm_options::replace) {
  permissions_impl(p, permissions, options);
}

inline void permissions(const path &p, perms permissions,
                        std::error_code &ec) noexcept {
  permissions_impl(p, permissions, perm_options::replace, &ec);
}

inline void permissions(const path &p, perms permissions, perm_options options,
                        std::error_code &ec) {
  permissions_impl(p, permissions, options, &ec);
}

inline auto proximate(const path &p, const path &base, std::error_code &ec)
    -> path {
  path tmp = weakly_canonical_impl(p, &ec);
  if (ec) {
    return {};
  }
  path tmp_base = weakly_canonical_impl(base, &ec);
  if (ec) {
    return {};
  }
  return tmp.lexically_proximate(tmp_base);
}

inline auto proximate(const path &p, std::error_code &ec) -> path {
  return proximate(p, current_path(), ec);
}

inline auto proximate(const path &p, const path &base = current_path())
    -> path {
  return weakly_canonical_impl(p).lexically_proximate(
      weakly_canonical_impl(base));
}

inline auto read_symlink(const path &p) -> path { return read_symlink_impl(p); }

inline auto read_symlink(const path &p, std::error_code &ec) -> path {
  return read_symlink_impl(p, &ec);
}

inline auto relative(const path &p, const path &base, std::error_code &ec)
    -> path {
  path tmp = weakly_canonical_impl(p, &ec);
  if (ec) {
    return {};
  }
  path tmpbase = weakly_canonical_impl(base, &ec);
  if (ec) {
    return {};
  }
  return tmp.lexically_relative(tmpbase);
}

inline auto relative(const path &p, std::error_code &ec) -> path {
  return relative(p, current_path(), ec);
}

inline auto relative(const path &p, const path &base = current_path()) -> path {
  return weakly_canonical_impl(p).lexically_relative(
      weakly_canonical_impl(base));
}

inline auto remove(const path &p) -> bool { return remove_impl(p); }

inline auto remove(const path &p, std::error_code &ec) noexcept -> bool {
  return remove_impl(p, &ec);
}

inline auto remove_all(const path &p) -> uintmax_t {
  return remove_all_impl(p);
}

inline auto remove_all(const path &p, std::error_code &ec) -> uintmax_t {
  return remove_all_impl(p, &ec);
}

inline void rename(const path &from, const path &to) {
  return rename_impl(from, to);
}

inline void rename(const path &from, const path &to,
                   std::error_code &ec) noexcept {
  return rename_impl(from, to, &ec);
}

inline void resize_file(const path &p, uintmax_t new_size) {
  return resize_file_impl(p, new_size);
}

inline void resize_file(const path &p, uintmax_t new_size,
                        std::error_code &ec) noexcept {
  return resize_file_impl(p, new_size, &ec);
}

inline auto space(const path &p) -> space_info { return space_impl(p); }

inline auto space(const path &p, std::error_code &ec) noexcept -> space_info {
  return space_impl(p, &ec);
}

inline auto status(const path &p) -> file_status { return status_impl(p); }

inline auto status(const path &p, std::error_code &ec) noexcept -> file_status {
  return status_impl(p, &ec);
}

inline auto symlink_status(const path &p) -> file_status {
  return symlink_status_impl(p);
}

inline auto symlink_status(const path &p, std::error_code &ec) noexcept
    -> file_status {
  return symlink_status_impl(p, &ec);
}

inline auto temp_directory_path() -> path { return temp_directory_path_impl(); }

inline auto temp_directory_path(std::error_code &ec) -> path {
  return temp_directory_path_impl(&ec);
}

inline auto weakly_canonical(path const &p) -> path {
  return weakly_canonical_impl(p);
}

inline auto weakly_canonical(path const &p, std::error_code &ec) -> path {
  return weakly_canonical_impl(p, &ec);
}

}  // namespace filesystem
}  // namespace asap
