//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>

#include <filesystem/fs_file_status.h>
#include <filesystem/fs_path.h>

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
path absolute_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
path canonical_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void copy_impl(const path &from, const path &to, copy_options opt,
               std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool copy_file_impl(const path &from, const path &to, copy_options opt,
                    std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void copy_symlink_impl(const path &existing_symlink, const path &new_symlink,
                       std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool create_directories_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool create_directory_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool create_directory_impl(const path &p, const path &attributes,
                           std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void create_directory_symlink_impl(const path &to, const path &new_symlink,
                                   std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void create_hard_link_impl(const path &to, const path &new_hard_link,
                           std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void create_symlink_impl(const path &to, const path &new_symlink,
                         std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
path current_path_impl(std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void current_path_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool equivalent_impl(const path &p, const path &,
                     std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
uintmax_t file_size_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
uintmax_t hard_link_count_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool is_empty_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
file_time_type last_write_time_impl(const path &p,
                                    std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void last_write_time_impl(const path &p, file_time_type new_time,
                          std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void permissions_impl(const path &p, perms, perm_options,
                      std::error_code * = nullptr);
ASAP_FILESYSTEM_API
path read_symlink_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
bool remove_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
uintmax_t remove_all_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void rename_impl(const path &from, const path &to,
                 std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
void resize_file_impl(const path &p, uintmax_t size,
                      std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
space_info space_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
file_status status_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
file_status symlink_status_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
path system_complete_impl(const path &p, std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
path temp_directory_path_impl(std::error_code *ec = nullptr);
ASAP_FILESYSTEM_API
path weakly_canonical_impl(path const &p, std::error_code *ec = nullptr);

inline path current_path() { return current_path_impl(); }

inline path current_path(std::error_code &ec) { return current_path_impl(&ec); }

inline void current_path(const path &p) { current_path_impl(p); }

inline void current_path(const path &p, std::error_code &ec) noexcept {
  current_path_impl(p, &ec);
}

inline path absolute(const path &p) { return absolute_impl(p); }

inline path absolute(const path &p, std::error_code &ec) {
  return absolute_impl(p, &ec);
}

inline path canonical(const path &p) { return canonical_impl(p); }

inline path canonical(const path &p, std::error_code &ec) {
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

inline bool copy_file(const path &from, const path &to) {
  return copy_file_impl(from, to, copy_options::none);
}

inline bool copy_file(const path &from, const path &to, std::error_code &ec) {
  return copy_file_impl(from, to, copy_options::none, &ec);
}

inline bool copy_file(const path &from, const path &to, copy_options opt) {
  return copy_file_impl(from, to, opt);
}

inline bool copy_file(const path &from, const path &to, copy_options opt,
                      std::error_code &ec) {
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

inline bool create_directories(const path &p) {
  return create_directories_impl(p);
}

inline bool create_directories(const path &p, std::error_code &ec) {
  return create_directories_impl(p, &ec);
}

inline bool create_directory(const path &p) { return create_directory_impl(p); }

inline bool create_directory(const path &p, std::error_code &ec) noexcept {
  return create_directory_impl(p, &ec);
}

inline bool create_directory(const path &p, const path &attributes) {
  return create_directory_impl(p, attributes);
}

inline bool create_directory(const path &p, const path &attributes,
                             std::error_code &ec) noexcept {
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

inline bool status_known(file_status status) noexcept {
  return status.type() != file_type::none;
}

inline bool exists(file_status status) noexcept {
  return status_known(status) && status.type() != file_type::not_found;
}

inline bool exists(const path &p) { return exists(status_impl(p)); }

inline bool exists(const path &p, std::error_code &ec) noexcept {
  auto status = status_impl(p, &ec);
  if (status_known(status)) ec.clear();
  return exists(status);
}

inline bool equivalent(const path &p1, const path &p2) {
  return equivalent_impl(p1, p2);
}

inline bool equivalent(const path &p1, const path &p2,
                       std::error_code &ec) noexcept {
  return equivalent_impl(p1, p2, &ec);
}

inline uintmax_t file_size(const path &p) { return file_size_impl(p); }

inline uintmax_t file_size(const path &p, std::error_code &ec) noexcept {
  return file_size_impl(p, &ec);
}

inline uintmax_t hard_link_count(const path &p) {
  return hard_link_count_impl(p);
}

inline uintmax_t hard_link_count(const path &p, std::error_code &ec) noexcept {
  return hard_link_count_impl(p, &ec);
}

inline bool is_block_file(file_status status) noexcept {
  return status.type() == file_type::block;
}

inline bool is_block_file(const path &p) {
  return is_block_file(status_impl(p));
}

inline bool is_block_file(const path &p, std::error_code &ec) noexcept {
  return is_block_file(status_impl(p, &ec));
}

inline bool is_character_file(file_status status) noexcept {
  return status.type() == file_type::character;
}

inline bool is_character_file(const path &p) {
  return is_character_file(status_impl(p));
}

inline bool is_character_file(const path &p, std::error_code &ec) noexcept {
  return is_character_file(status_impl(p, &ec));
}

inline bool is_directory(file_status status) noexcept {
  return status.type() == file_type::directory;
}

inline bool is_directory(const path &p) { return is_directory(status_impl(p)); }

inline bool is_directory(const path &p, std::error_code &ec) noexcept {
  return is_directory(status_impl(p, &ec));
}

inline bool is_empty(const path &p) { return is_empty_impl(p); }

inline bool is_empty(const path &p, std::error_code &ec) {
  return is_empty_impl(p, &ec);
}

inline bool is_fifo(file_status status) noexcept {
  return status.type() == file_type::fifo;
}
inline bool is_fifo(const path &p) { return is_fifo(status_impl(p)); }

inline bool is_fifo(const path &p, std::error_code &ec) noexcept {
  return is_fifo(status_impl(p, &ec));
}

inline bool is_regular_file(file_status status) noexcept {
  return status.type() == file_type::regular;
}

inline bool is_regular_file(const path &p) {
  return is_regular_file(status_impl(p));
}

inline bool is_regular_file(const path &p, std::error_code &ec) noexcept {
  return is_regular_file(status_impl(p, &ec));
}

inline bool is_socket(file_status status) noexcept {
  return status.type() == file_type::socket;
}

inline bool is_socket(const path &p) { return is_socket(status_impl(p)); }

inline bool is_socket(const path &p, std::error_code &ec) noexcept {
  return is_socket(status_impl(p, &ec));
}

inline bool is_symlink(file_status status) noexcept {
  return status.type() == file_type::symlink;
}

inline bool is_symlink(const path &p) {
  return is_symlink(symlink_status_impl(p));
}

inline bool is_symlink(const path &p, std::error_code &ec) noexcept {
  return is_symlink(symlink_status_impl(p, &ec));
}

inline bool is_other(file_status status) noexcept {
  return exists(status) && !is_regular_file(status) && !is_directory(status) &&
         !is_symlink(status);
}

inline bool is_other(const path &p) { return is_other(status_impl(p)); }

inline bool is_other(const path &p, std::error_code &ec) noexcept {
  return is_other(status_impl(p, &ec));
}

inline file_time_type last_write_time(const path &p) {
  return last_write_time_impl(p);
}

inline file_time_type last_write_time(const path &p,
                                      std::error_code &ec) noexcept {
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

inline path proximate(const path &p, const path &base, std::error_code &ec) {
  path tmp = weakly_canonical_impl(p, &ec);
  if (ec) return {};
  path tmp_base = weakly_canonical_impl(base, &ec);
  if (ec) return {};
  return tmp.lexically_proximate(tmp_base);
}

inline path proximate(const path &p, std::error_code &ec) {
  return proximate(p, current_path(), ec);
}

inline path proximate(const path &p, const path &base = current_path()) {
  return weakly_canonical_impl(p).lexically_proximate(
      weakly_canonical_impl(base));
}

inline path read_symlink(const path &p) { return read_symlink_impl(p); }

inline path read_symlink(const path &p, std::error_code &ec) {
  return read_symlink_impl(p, &ec);
}

inline path relative(const path &p, const path &base, std::error_code &ec) {
  path tmp = weakly_canonical_impl(p, &ec);
  if (ec) return path();
  path tmpbase = weakly_canonical_impl(base, &ec);
  if (ec) return path();
  return tmp.lexically_relative(tmpbase);
}

inline path relative(const path &p, std::error_code &ec) {
  return relative(p, current_path(), ec);
}

inline path relative(const path &p, const path &base = current_path()) {
  return weakly_canonical_impl(p).lexically_relative(
      weakly_canonical_impl(base));
}

inline bool remove(const path &p) { return remove_impl(p); }

inline bool remove(const path &p, std::error_code &ec) noexcept {
  return remove_impl(p, &ec);
}

inline uintmax_t remove_all(const path &p) { return remove_all_impl(p); }

inline uintmax_t remove_all(const path &p, std::error_code &ec) {
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

inline space_info space(const path &p) { return space_impl(p); }

inline space_info space(const path &p, std::error_code &ec) noexcept {
  return space_impl(p, &ec);
}

inline file_status status(const path &p) { return status_impl(p); }

inline file_status status(const path &p, std::error_code &ec) noexcept {
  return status_impl(p, &ec);
}

inline file_status symlink_status(const path &p) {
  return symlink_status_impl(p);
}

inline file_status symlink_status(const path &p, std::error_code &ec) noexcept {
  return symlink_status_impl(p, &ec);
}

inline path temp_directory_path() { return temp_directory_path_impl(); }

inline path temp_directory_path(std::error_code &ec) {
  return temp_directory_path_impl(&ec);
}

inline path weakly_canonical(path const &p) { return weakly_canonical_impl(p); }

inline path weakly_canonical(path const &p, std::error_code &ec) {
  return weakly_canonical_impl(p, &ec);
}

}  // namespace filesystem
}  // namespace asap
