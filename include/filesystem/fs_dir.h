//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <common/assert.h>
#include <filesystem/asap_filesystem_api.h>
#include <filesystem/filesystem.h>

#include <memory> // for std::shared_ptr
#include <utility>

namespace asap {
namespace filesystem {

class DirectoryStream;

// -----------------------------------------------------------------------------
//                               directory_entry
// -----------------------------------------------------------------------------

/*!
@brief Represents a directory entry.

The object stores a path as a member and may also store additional file
attributes (hard link count, status, symlink status file size, and last write
time) during directory iteration.

@see https://en.cppreference.com/w/cpp/filesystem/directory_entry
*/
class ASAP_FILESYSTEM_API directory_entry {
  using path_type = asap::filesystem::path;

public:
  // constructors and destructors
  directory_entry() noexcept = default;
  directory_entry(directory_entry const &) = default;
  directory_entry(directory_entry &&) noexcept = default;

  explicit directory_entry(path_type p) : path_(std::move(p)) {
    std::error_code ec;
    DoRefresh(&ec);
  }

  directory_entry(path_type p, std::error_code &ec) : path_(std::move(p)) { DoRefresh(&ec); }

  ~directory_entry() = default;

  auto operator=(directory_entry const &) -> directory_entry & = default;
  auto operator=(directory_entry &&) noexcept -> directory_entry & = default;

  void assign(path_type const &p) {
    path_ = p;
    DoRefresh();
  }

  void assign(path_type const &p, std::error_code &ec) {
    path_ = p;
    DoRefresh(&ec);
  }

  void replace_filename(path_type const &p) {
    path_.replace_filename(p);
    DoRefresh();
  }

  void replace_filename(path_type const &p, std::error_code &ec) {
    path_.replace_filename(p);
    DoRefresh(&ec);
  }

  void refresh() { DoRefresh(); }

  void refresh(std::error_code &ec) noexcept {
    try {
      DoRefresh(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
    }
  }

  /// Returns the full path the directory entry refers to.
  auto path() const noexcept -> path_type const & { return path_; }

  /// Returns the full path the directory entry refers to.
  explicit operator const path_type &() const noexcept { return path_; }

  /// Checks whether the pointed-to object exists (follows symlinks).
  auto exists() const -> bool { return asap::filesystem::exists(file_status{GetFileType()}); }

  /// Checks whether the pointed-to object exists (follows symlinks).
  auto exists(std::error_code &ec) const noexcept -> bool {
    return asap::filesystem::exists(file_status{GetFileType(&ec)});
  }

  /// Checks whether the pointed-to object is a block device (follows symlinks).
  auto is_block_file() const -> bool { return GetFileType() == file_type::block; }

  /// Checks whether the pointed-to object is a block device (follows symlinks).
  auto is_block_file(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::block;
  }

  /// Checks whether the pointed-to object is a charcter device (follows
  /// symlinks).
  auto is_character_file() const -> bool { return GetFileType() == file_type::character; }

  /// Checks whether the pointed-to object is a charcter device (follows
  /// symlinks).
  auto is_character_file(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::character;
  }

  /// Checks whether the pointed-to object is a directory (follows symlinks).
  auto is_directory() const -> bool { return GetFileType() == file_type::directory; }

  /// Checks whether the pointed-to object is a directory (follows symlinks).
  auto is_directory(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::directory;
  }

  /// Checks whether the pointed-to object is a FIFO or pipe file (follows
  /// symlinks).
  auto is_fifo() const -> bool { return GetFileType() == file_type::fifo; }

  /// Checks whether the pointed-to object is a FIFO or pipe file (follows
  /// symlinks).
  auto is_fifo(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::fifo;
  }

  /// Checks whether the pointed-to object is an other file (not a regular file,
  /// directory or symlink) (follows symlinks).
  auto is_other() const -> bool { return asap::filesystem::is_other(file_status{GetFileType()}); }

  /// Checks whether the pointed-to object is an other file (not a regular file,
  /// directory or symlink) (follows symlinks).
  auto is_other(std::error_code &ec) const noexcept -> bool {
    return asap::filesystem::is_other(file_status{GetFileType(&ec)});
  }

  /// Checks whether the pointed-to object is a regular file (follows symlinks).
  auto is_regular_file() const -> bool { return GetFileType() == file_type::regular; }

  /// Checks whether the pointed-to object is a regular file (follows symlinks).
  auto is_regular_file(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::regular;
  }

  /// Checks whether the pointed-to object is a named socket (follows symlinks).
  auto is_socket() const -> bool { return GetFileType() == file_type::socket; }

  /// Checks whether the pointed-to object is a named socket (follows symlinks).
  auto is_socket(std::error_code &ec) const noexcept -> bool {
    return GetFileType(&ec) == file_type::socket;
  }

  /// Checks whether the pointed-to object is a symbolic link (does not follow
  /// symlinks).
  auto is_symlink() const -> bool { return GetSymLinkFileType() == file_type::symlink; }

  /// Checks whether the pointed-to object is a symbolic link (does not follow
  /// symlinks).
  auto is_symlink(std::error_code &ec) const noexcept -> bool {
    return GetSymLinkFileType(&ec) == file_type::symlink;
  }

  /// Returns the size of the referred-to filesystem object (follows symlinks).
  auto file_size() const -> uintmax_t { return GetSize(); }

  /// Returns the size of the referred-to filesystem object (follows symlinks).
  auto file_size(std::error_code &ec) const noexcept -> uintmax_t {
    try {
      return GetSize(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  /// Returns the number of hard links for the referred-to filesystem object
  /// (does not follow symlinks).
  auto hard_link_count() const -> uintmax_t { return GetHardLinkCount(); }

  /// Returns the number of hard links for the referred-to filesystem object
  /// (does not follow symlinks).
  auto hard_link_count(std::error_code &ec) const noexcept -> uintmax_t {
    try {
      return GetHardLinkCount(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  /// Returns the last modification time for the referred-to filesystem object
  /// (follows symlinks).
  auto last_write_time() const -> file_time_type { return GetLastWriteTime(); }

  /// Returns the last modification time for the referred-to filesystem object
  /// (follows symlinks).
  auto last_write_time(std::error_code &ec) const noexcept -> file_time_type {
    try {
      return GetLastWriteTime(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  /// Returns status of the entry, as if determined by a status call (symlinks
  /// are followed to their targets).
  auto status() const -> file_status { return GetStatus(); }

  /// Returns status of the entry, as if determined by a status call (symlinks
  /// are followed to their targets).
  auto status(std::error_code &ec) const noexcept -> file_status { return GetStatus(&ec); }

  /// Returns status of the entry, as if determined by a symlink_status call
  /// (symlinks are not followed).
  auto symlink_status() const -> file_status { return GetSymLinkStatus(); }

  /// Returns status of the entry, as if determined by a symlink_status call
  /// (symlinks are not followed).
  auto symlink_status(std::error_code &ec) const noexcept -> file_status {
    return GetSymLinkStatus(&ec);
  }

  auto operator<(directory_entry const &rhs) const noexcept -> bool { return path_ < rhs.path_; }

  auto operator==(directory_entry const &rhs) const noexcept -> bool { return path_ == rhs.path_; }

  auto operator!=(directory_entry const &rhs) const noexcept -> bool { return path_ != rhs.path_; }

  auto operator<=(directory_entry const &rhs) const noexcept -> bool { return path_ <= rhs.path_; }

  auto operator>(directory_entry const &rhs) const noexcept -> bool { return path_ > rhs.path_; }

  auto operator>=(directory_entry const &rhs) const noexcept -> bool { return path_ >= rhs.path_; }

private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  friend class DirectoryStream;

  enum class CacheType_ : unsigned char { EMPTY, BASIC, EXTRA, FULL };

  struct CachedData_ {
    CacheType_ cache_type;

    bool symlink;
    bool type_resolved;
    bool extra_resolved;
    bool perms_resolved;

    // BASIC
    // The filesystem object type (symlinks are followed)
    file_type type;

    // EXTRA
    // Number of hard links (symlinks are NOT followed)
    uintmax_t nlink;
    // Total size, in bytes (symlinks are followed)
    uintmax_t size;
    // Time of last modification (symlinks are followed)
    file_time_type write_time;

    // FULL
    perms symlink_perms;
    perms non_symlink_perms;

    CachedData_() noexcept { Reset(); }

    void Reset() {
      symlink = false;
      type_resolved = extra_resolved = perms_resolved = false;
      cache_type = CacheType_::EMPTY;
      type = file_type::none;
      symlink_perms = non_symlink_perms = perms::unknown;
      size = nlink = uintmax_t(-1);
      write_time = file_time_type::min();
    }
  };

#if 0 // FIXME
  static CachedData_ CreateIterResult(file_type type) {
    CachedData_ data;
    data.type = type;
    data.cache_type = [&]() {
      switch (type) {
        case file_type::none:
          return CacheType_::EMPTY;
        case file_type::symlink:
          return CacheType_::ITER_SYMLINK;
        default:
          return CacheType_::ITER_NON_SYMLINK;
      }
    }();
    return data;
  }
#endif

  void AssignIterEntry(path_type &&p, CachedData_ data) {
    path_ = std::move(p);
    cached_data_ = data;
  }

  auto DoRefresh_impl() const noexcept -> std::error_code;

  static auto IsDoesNotExistError(std::error_code const &ec) -> bool {
    if (!ec) {
      return true;
    }
    switch (static_cast<std::errc>(ec.value())) {
    case std::errc::no_such_file_or_directory:
    case std::errc::not_a_directory:
      return true;
    default:
      return false;
    }
  }

  void HandleError(const char *msg, std::error_code *dest_ec, std::error_code const &ec,
      bool allow_dne = false) const {
    if (dest_ec != nullptr) {
      *dest_ec = ec;
      return;
    }
    if (ec && (!allow_dne || !IsDoesNotExistError(ec))) {
      throw filesystem_error(msg, path_, ec);
    }
  }

  void DoRefresh(std::error_code *ec = nullptr) const {
    HandleError("in directory_entry::DoRefresh", ec, DoRefresh_impl(),
        /*allow_dne*/ true);
  }

  void UpdateBasicFileInformation(bool follow_symlinks, std::error_code *ec = nullptr) const;
  void UpdateExtraFileInformation(bool follow_symlinks, std::error_code *ec = nullptr) const;
  void UpdatePermissionsInformation(bool follow_symlinks, std::error_code *ec = nullptr) const;

  auto GetSymLinkFileType(std::error_code *ec = nullptr) const -> file_type;

  auto GetFileType(std::error_code *ec = nullptr) const -> file_type;

  auto GetStatus(std::error_code *ec = nullptr) const -> file_status;

  auto GetSymLinkStatus(std::error_code *ec = nullptr) const -> file_status;

  auto GetSize(std::error_code *ec = nullptr) const -> uintmax_t;

  auto GetHardLinkCount(std::error_code *ec = nullptr) const -> uintmax_t;

  auto GetLastWriteTime(std::error_code *ec = nullptr) const -> file_time_type;

  path_type path_;
#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  mutable CachedData_ cached_data_;
#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(pop)
#endif
}; // namespace filesystem

class DirectoryEntryProxy_ {
public:
  inline auto operator*() -> directory_entry { return std::move(entry_); }

private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  explicit DirectoryEntryProxy_(directory_entry entry) : entry_(std::move(entry)) {}
  DirectoryEntryProxy_(DirectoryEntryProxy_ &&other) noexcept : entry_(std::move(other.entry_)) {}

  directory_entry entry_;
};

// -----------------------------------------------------------------------------
//                               directory_iterator
// -----------------------------------------------------------------------------

/*!
@brief A LegacyInputIterator that iterates over the directory_entry elements of
a directory (but does not visit the subdirectories). The iteration order is
unspecified, except that each directory entry is visited only once. The special
pathnames dot and dot-dot are skipped.

If the directory_iterator reports an error or is advanced past the last
directory entry, it becomes equal to the default-constructed iterator, also
known as the end iterator. Two end iterators are always equal, dereferencing or
incrementing the end iterator is undefined behavior.

If a file or a directory is deleted or added to the directory tree after the
directory iterator has been created, it is unspecified whether the change would
be observed through the iterator.

@see https://en.cppreference.com/w/cpp/filesystem/directory_iterator
*/
class ASAP_FILESYSTEM_API directory_iterator {
public:
  using value_type = directory_entry;
  using difference_type = ptrdiff_t;
  using pointer = const value_type *;
  using reference = const value_type &;
  using iterator_category = std::input_iterator_tag;

  // ctor & dtor
  directory_iterator() noexcept = default;

  explicit directory_iterator(const path &p) : directory_iterator(p, nullptr) {}

  directory_iterator(const path &p, directory_options opts)
      : directory_iterator(p, nullptr, opts) {}

  directory_iterator(const path &p, std::error_code &ec) : directory_iterator(p, &ec) {}

  directory_iterator(const path &p, directory_options opts, std::error_code &ec)
      : directory_iterator(p, &ec, opts) {}

  directory_iterator(const directory_iterator &) = default;
  directory_iterator(directory_iterator &&) = default;
  auto operator=(const directory_iterator &) -> directory_iterator & = default;

  auto operator=(directory_iterator &&other) noexcept -> directory_iterator & {
    // non-default implementation provided to support self-move assign.
    if (this != &other) {
      impl_ = std::move(other.impl_);
    }
    return *this;
  }

  ~directory_iterator() = default;

  auto operator*() const -> const directory_entry & {
    ASAP_ASSERT(impl_ && "The end iterator cannot be dereferenced");
    return dereference();
  }

  auto operator->() const -> const directory_entry * { return &**this; }

  auto operator++() -> directory_iterator & { return do_increment(); }

  auto operator++(int) -> DirectoryEntryProxy_ {
    DirectoryEntryProxy_ proxy(**this);
    do_increment();
    return proxy;
  }

  auto increment(std::error_code &ec) -> directory_iterator & { return do_increment(&ec); }

private:
  inline friend auto operator==(
      const directory_iterator & /*lhs*/, const directory_iterator & /*rhs*/) noexcept -> bool;

  // construct the dir_stream
  directory_iterator(const path &, std::error_code *, directory_options = directory_options::none);

  auto do_increment(std::error_code *ec = nullptr) -> directory_iterator &;

  auto dereference() const -> const directory_entry &;

#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  std::shared_ptr<DirectoryStream> impl_;
#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(pop)
#endif
};

inline auto operator==(const directory_iterator &lhs, const directory_iterator &rhs) noexcept
    -> bool {
  return lhs.impl_ == rhs.impl_;
}

inline auto operator!=(const directory_iterator &lhs, const directory_iterator &rhs) noexcept
    -> bool {
  return !(lhs == rhs);
}

// enable directory_iterator range-based for statements
inline auto begin(directory_iterator iter) noexcept -> directory_iterator { return iter; }

inline auto end(const directory_iterator & /*unused*/) noexcept -> directory_iterator { return {}; }

// -----------------------------------------------------------------------------
//                        recursive_directory_iterator
// -----------------------------------------------------------------------------

/*!
@brief A LegacyInputIterator that iterates over the directory_entry elements of
a directory, and, recursively, over the entries of all subdirectories. The
iteration order is unspecified, except that each directory entry is visited only
once.

By default, symlinks are not followed, but this can be enabled by specifying the
directory option follow_directory_symlink at construction time.

The special pathnames dot and dot-dot are skipped.

If the recursive_directory_iterator reports an error or is advanced past the
last directory entry of the top-level directory, it becomes equal to the
default-constructed iterator, also known as the end iterator. Two end iterators
are always equal, dereferencing or incrementing the end iterator is undefined
behavior.

If a file or a directory is deleted or added to the directory tree after the
recursive directory iterator has been created, it is unspecified whether the
change would be observed through the iterator.

If the directory structure contains cycles, the end iterator may be unreachable.

@see https://en.cppreference.com/w/cpp/filesystem/recursive_directory_iterator
*/
class ASAP_FILESYSTEM_API recursive_directory_iterator {
public:
  using value_type = directory_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = directory_entry const *;
  using reference = directory_entry const &;
  using iterator_category = std::input_iterator_tag;

  // constructors and destructor

  recursive_directory_iterator() noexcept = default;

  explicit recursive_directory_iterator(
      const path &p, directory_options xoptions = directory_options::none)
      : recursive_directory_iterator(p, xoptions, nullptr) {}

  recursive_directory_iterator(const path &p, directory_options xoptions, std::error_code &ec)
      : recursive_directory_iterator(p, xoptions, &ec) {}

  recursive_directory_iterator(const path &p, std::error_code &ec)
      : recursive_directory_iterator(p, directory_options::none, &ec) {}

  recursive_directory_iterator(const recursive_directory_iterator &) = default;
  recursive_directory_iterator(recursive_directory_iterator &&) = default;

  auto operator=(const recursive_directory_iterator &) -> recursive_directory_iterator & = default;

  auto operator=(recursive_directory_iterator &&other) noexcept -> recursive_directory_iterator & {
    // non-default implementation provided to support self-move assign.
    if (this != &other) {
      impl_ = std::move(other.impl_);
      recursion_ = other.recursion_;
    }
    return *this;
  }

  ~recursive_directory_iterator() = default;

  auto operator*() const -> const directory_entry & { return dereference(); }

  auto operator->() const -> const directory_entry * { return &dereference(); }

  auto operator++() -> recursive_directory_iterator & { return do_increment(); }

  auto operator++(int) -> DirectoryEntryProxy_ {
    DirectoryEntryProxy_ p(**this);
    do_increment();
    return p;
  }

  auto increment(std::error_code &ec) -> recursive_directory_iterator & {
    return do_increment(&ec);
  }

  auto options() const -> directory_options;
  auto depth() const -> int;

  void pop() { pop_impl(); }

  void pop(std::error_code &ec) { pop_impl(&ec); }

  auto recursion_pending() const -> bool { return recursion_; }

  void disable_recursion_pending() { recursion_ = false; }

private:
  recursive_directory_iterator(const path &p, directory_options _opt, std::error_code *ec);

  auto dereference() const -> const directory_entry &;

  auto TryRecursion(std::error_code *ec) -> bool;

  void Advance(std::error_code *ec = nullptr);

  auto do_increment(std::error_code *ec = nullptr) -> recursive_directory_iterator &;

  void pop_impl(std::error_code *ec = nullptr);

  inline friend auto operator==(const recursive_directory_iterator & /*lhs*/,
      const recursive_directory_iterator & /*rhs*/) noexcept -> bool;

  struct SharedImpl;
#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(disable : 4251)
#endif
  std::shared_ptr<SharedImpl> impl_;
#if defined(HEDLEY_MSVC_VERSION)
#pragma warning(pop)
#endif
  bool recursion_{};
}; // class recursive_directory_iterator

inline auto operator==(const recursive_directory_iterator &lhs,
    const recursive_directory_iterator &rhs) noexcept -> bool {
  return lhs.impl_ == rhs.impl_;
}

inline auto operator!=(const recursive_directory_iterator &lhs,
    const recursive_directory_iterator &rhs) noexcept -> bool {
  return !(lhs == rhs);
}
// enable recursive_directory_iterator range-based for statements
inline auto begin(recursive_directory_iterator iter) noexcept -> recursive_directory_iterator {
  return iter;
}

inline auto end(const recursive_directory_iterator & /*unused*/) noexcept
    -> recursive_directory_iterator {
  return {};
}

} // namespace filesystem
} // namespace asap
