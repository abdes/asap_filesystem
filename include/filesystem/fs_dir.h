

#pragma once

#include <memory>  // for std::shared_ptr

#include <common/assert.h>
#include <filesystem/asap_filesystem_api.h>
#include <filesystem/filesystem.h>

namespace asap {
namespace filesystem {

class DirectoryStream;

class ASAP_FILESYSTEM_API directory_entry {
  typedef asap::filesystem::path path_type;

 public:
  // constructors and destructors
  directory_entry() noexcept = default;
  directory_entry(directory_entry const &) = default;
  directory_entry(directory_entry &&) noexcept = default;

  explicit directory_entry(path_type const &__p) : path_(__p) {
    std::error_code ec;
    DoRefresh(&ec);
  }

  directory_entry(path_type const &__p, std::error_code &ec) : path_(__p) {
    DoRefresh(&ec);
  }

  ~directory_entry() = default;

  directory_entry &operator=(directory_entry const &) = default;
  directory_entry &operator=(directory_entry &&) noexcept = default;

  void assign(path_type const &__p) {
    path_ = __p;
    std::error_code ec;
    DoRefresh(&ec);
  }

  void assign(path_type const &__p, std::error_code &ec) {
    path_ = __p;
    DoRefresh(&ec);
  }

  void replace_filename(path_type const &__p) {
    path_.replace_filename(__p);
    std::error_code ec;
    DoRefresh(&ec);
  }

  void replace_filename(path_type const &__p, std::error_code &ec) {
    path_ = path_.parent_path() / __p;
    DoRefresh(&ec);
  }

  void refresh() { DoRefresh(); }

  void refresh(std::error_code &ec) noexcept {
    try {
      DoRefresh(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  path_type const &path() const noexcept { return path_; }

  operator const path_type &() const noexcept { return path_; }

  bool exists() const {
    return asap::filesystem::exists(file_status{GetFileType()});
  }

  bool exists(std::error_code &ec) const noexcept {
    return asap::filesystem::exists(file_status{GetFileType(&ec)});
  }

  bool is_block_file() const { return GetFileType() == file_type::block; }

  bool is_block_file(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::block;
  }

  bool is_character_file() const {
    return GetFileType() == file_type::character;
  }

  bool is_character_file(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::character;
  }

  bool is_directory() const { return GetFileType() == file_type::directory; }

  bool is_directory(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::directory;
  }

  bool is_fifo() const { return GetFileType() == file_type::fifo; }

  bool is_fifo(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::fifo;
  }

  bool is_other() const {
    return asap::filesystem::is_other(file_status{GetFileType()});
  }

  bool is_other(std::error_code &ec) const noexcept {
    return asap::filesystem::is_other(file_status{GetFileType(&ec)});
  }

  bool is_regular_file() const { return GetFileType() == file_type::regular; }

  bool is_regular_file(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::regular;
  }

  bool is_socket() const { return GetFileType() == file_type::socket; }

  bool is_socket(std::error_code &ec) const noexcept {
    return GetFileType(&ec) == file_type::socket;
  }

  bool is_symlink() const { return GetSymLinkFileType() == file_type::symlink; }

  bool is_symlink(std::error_code &ec) const noexcept {
    return GetSymLinkFileType(&ec) == file_type::symlink;
  }

  uintmax_t file_size() const { return GetSize(); }

  uintmax_t file_size(std::error_code &ec) const noexcept {
    try {
      return GetSize(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  uintmax_t hard_link_count() const { return GetHardLinkCount(); }

  uintmax_t hard_link_count(std::error_code &ec) const noexcept {
    try {
      return GetHardLinkCount(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  file_time_type last_write_time() const { return GetLastWriteTime(); }

  file_time_type last_write_time(std::error_code &ec) const noexcept {
    try {
      return GetLastWriteTime(&ec);
    } catch (...) {
      // The above will never throw when we pass a non null ec
      ASAP_UNREACHABLE();
    }
  }

  file_status status() const { return GetStatus(); }

  file_status status(std::error_code &ec) const noexcept {
    return GetStatus(&ec);
  }

  file_status symlink_status() const { return GetSymLinkStatus(); }

  file_status symlink_status(std::error_code &ec) const noexcept {
    return GetSymLinkStatus(&ec);
  }

  bool operator<(directory_entry const &__rhs) const noexcept {
    return path_ < __rhs.path_;
  }

  bool operator==(directory_entry const &__rhs) const noexcept {
    return path_ == __rhs.path_;
  }

  bool operator!=(directory_entry const &__rhs) const noexcept {
    return path_ != __rhs.path_;
  }

  bool operator<=(directory_entry const &__rhs) const noexcept {
    return path_ <= __rhs.path_;
  }

  bool operator>(directory_entry const &__rhs) const noexcept {
    return path_ > __rhs.path_;
  }

  bool operator>=(directory_entry const &__rhs) const noexcept {
    return path_ >= __rhs.path_;
  }

 private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  friend class DirectoryStream;

  enum class CacheType_ : unsigned char {
    EMPTY,
    ITER_SYMLINK,
    ITER_NON_SYMLINK,
    REFRESH_SYMLINK,
    REFRESH_SYMLINK_UNRESOLVED,
    REFRESH_NON_SYMLINK
  };

  struct CachedData_ {
    // clang-format off
    uintmax_t       size;              // Total size, in bytes
    uintmax_t       nlink;             // Number of hard links
    file_time_type  write_time;        // Time of last modification
    perms           symlink_perms;     // Symbolic link permissions
    perms           non_symlink_perms; //
    file_type       type;              //
    CacheType_      cache_type;        //
    // clang-format on

    CachedData_() noexcept { Reset(); }

    void Reset() {
      cache_type = CacheType_::EMPTY;
      type = file_type::none;
      symlink_perms = non_symlink_perms = perms::unknown;
      size = nlink = uintmax_t(-1);
      write_time = file_time_type::min();
    }
  };

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

  void AssignIterEntry(path_type &&__p, CachedData_ __dt) {
    path_ = std::move(__p);
    cached_data_ = __dt;
  }

  std::error_code DoRefresh_impl() noexcept;

  static bool IsDoesNotExistError(std::error_code const &ec) {
    if (!ec) return true;
    switch (static_cast<std::errc>(ec.value())) {
      case std::errc::no_such_file_or_directory:
      case std::errc::not_a_directory:
        return true;
      default:
        return false;
    }
  }

  void HandleError(const char *__msg, std::error_code *__dest_ec,
                   std::error_code const &ec, bool __allow_dne = false) const {
    if (__dest_ec) {
      *__dest_ec = ec;
      return;
    }
    if (ec && (!__allow_dne || !IsDoesNotExistError(ec)))
      throw filesystem_error(__msg, path_, ec);
  }

  void DoRefresh(std::error_code *ec = nullptr) {
    HandleError("in directory_entry::refresh", ec, DoRefresh_impl(),
                /*allow_dne*/ true);
  }

  file_type GetSymLinkFileType(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
        return symlink_status_impl(path_, ec).type();

      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        if (ec) ec->clear();
        return file_type::symlink;

      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::REFRESH_NON_SYMLINK:
        file_status __st(cached_data_.type);
        if (ec && !asap::filesystem::exists(__st))
          *ec = make_error_code(std::errc::no_such_file_or_directory);
        else if (ec)
          ec->clear();
        return cached_data_.type;
    }
    ASAP_UNREACHABLE();
  }

  file_type GetFileType(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return status_impl(path_, ec).type();

      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::REFRESH_NON_SYMLINK:
      case CacheType_::REFRESH_SYMLINK: {
        file_status __st(cached_data_.type);
        if (ec && !asap::filesystem::exists(__st))
          *ec = make_error_code(std::errc::no_such_file_or_directory);
        else if (ec)
          ec->clear();
        return cached_data_.type;
      }
    }
    ASAP_UNREACHABLE();
  }

  file_status GetStatus(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return status_impl(path_, ec);

      case CacheType_::REFRESH_NON_SYMLINK:
      case CacheType_::REFRESH_SYMLINK:
        return file_status(GetFileType(ec), cached_data_.non_symlink_perms);
    }
    ASAP_UNREACHABLE();
  }

  file_status GetSymLinkStatus(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::ITER_SYMLINK:
        return symlink_status_impl(path_, ec);

      case CacheType_::REFRESH_NON_SYMLINK:
        return file_status(GetSymLinkFileType(ec),
                           cached_data_.non_symlink_perms);

      case CacheType_::REFRESH_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return file_status(GetSymLinkFileType(ec), cached_data_.symlink_perms);
    }
    ASAP_UNREACHABLE();
  }

  uintmax_t GetSize(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return asap::filesystem::file_size_impl(path_, ec);

      case CacheType_::REFRESH_SYMLINK:
      case CacheType_::REFRESH_NON_SYMLINK: {
        std::error_code __m_ec;
        file_status __st(GetFileType(&__m_ec));
        HandleError("in directory_entry::file_size", ec, __m_ec);
        if (asap::filesystem::exists(__st) &&
            !asap::filesystem::is_regular_file(__st)) {
          std::errc __err_kind = asap::filesystem::is_directory(__st)
                                     ? std::errc::is_a_directory
                                     : std::errc::not_supported;
          HandleError("in directory_entry::file_size", ec,
                      make_error_code(__err_kind));
        }
        return cached_data_.size;
      }
    }
    ASAP_UNREACHABLE();
  }

  uintmax_t GetHardLinkCount(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return asap::filesystem::hard_link_count_impl(path_, ec);

      case CacheType_::REFRESH_SYMLINK:
      case CacheType_::REFRESH_NON_SYMLINK: {
        std::error_code __m_ec;
        (void)GetFileType(&__m_ec);
        HandleError("in directory_entry::hard_link_count", ec, __m_ec);
        return cached_data_.nlink;
      }
    }
    ASAP_UNREACHABLE();
  }

  file_time_type GetLastWriteTime(std::error_code *ec = nullptr) const {
    switch (cached_data_.cache_type) {
      case CacheType_::EMPTY:
      case CacheType_::ITER_NON_SYMLINK:
      case CacheType_::ITER_SYMLINK:
      case CacheType_::REFRESH_SYMLINK_UNRESOLVED:
        return asap::filesystem::last_write_time_impl(path_, ec);

      case CacheType_::REFRESH_SYMLINK:
      case CacheType_::REFRESH_NON_SYMLINK: {
        std::error_code __m_ec;
        file_status __st(GetFileType(&__m_ec));
        HandleError("in directory_entry::last_write_time", ec, __m_ec);
        if (asap::filesystem::exists(__st) &&
            cached_data_.write_time == file_time_type::min())
          HandleError("in directory_entry::last_write_time", ec,
                      make_error_code(std::errc::value_too_large));
        return cached_data_.write_time;
      }
    }
    ASAP_UNREACHABLE();
  }

 private:
  path_type path_;
  CachedData_ cached_data_;
};

class DirectoryEntryProxy_ {
 public:
  inline directory_entry operator*() { return std::move(entry_); }

 private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  explicit DirectoryEntryProxy_(directory_entry const &entry) : entry_(entry) {}
  DirectoryEntryProxy_(DirectoryEntryProxy_ &&other) noexcept
      : entry_(std::move(other.entry_)) {}

  directory_entry entry_;
};

class ASAP_FILESYSTEM_API directory_iterator {
 public:
  typedef directory_entry value_type;
  typedef ptrdiff_t difference_type;
  typedef value_type const *pointer;
  typedef value_type const &reference;
  typedef std::input_iterator_tag iterator_category;

 public:
  // ctor & dtor
  directory_iterator() noexcept = default;

  explicit directory_iterator(const path &__p)
      : directory_iterator(__p, nullptr) {}

  directory_iterator(const path &__p, directory_options __opts)
      : directory_iterator(__p, nullptr, __opts) {}

  directory_iterator(const path &__p, std::error_code &ec)
      : directory_iterator(__p, &ec) {}

  directory_iterator(const path &__p, directory_options __opts,
                     std::error_code &ec)
      : directory_iterator(__p, &ec, __opts) {}

  directory_iterator(const directory_iterator &) = default;
  directory_iterator(directory_iterator &&) = default;
  directory_iterator &operator=(const directory_iterator &) = default;

  directory_iterator &operator=(directory_iterator &&__o) noexcept {
    // non-default implementation provided to support self-move assign.
    if (this != &__o) {
      impl_ = std::move(__o.impl_);
    }
    return *this;
  }

  ~directory_iterator() = default;

  const directory_entry &operator*() const {
    ASAP_ASSERT(impl_ && "The end iterator cannot be dereferenced");
    return dereference();
  }

  const directory_entry *operator->() const { return &**this; }

  directory_iterator &operator++() { return do_increment(); }

  DirectoryEntryProxy_ operator++(int) {
    DirectoryEntryProxy_ proxy(**this);
    do_increment();
    return proxy;
  }

  directory_iterator &increment(std::error_code &ec) {
    return do_increment(&ec);
  }

 private:
  inline friend bool operator==(const directory_iterator &,
                                const directory_iterator &) noexcept;

  // construct the dir_stream
  directory_iterator(const path &, std::error_code *,
                     directory_options = directory_options::none);

  directory_iterator &do_increment(std::error_code *ec = nullptr);

  const directory_entry &dereference() const;

 private:
  std::shared_ptr<DirectoryStream> impl_;
};

inline bool operator==(const directory_iterator &lhs,
                       const directory_iterator &rhs) noexcept {
  return lhs.impl_ == rhs.impl_;
}

inline bool operator!=(const directory_iterator &lhs,
                       const directory_iterator &rhs) noexcept {
  return !(lhs == rhs);
}

// enable directory_iterator range-based for statements
inline directory_iterator begin(directory_iterator iter) noexcept {
  return iter;
}

inline directory_iterator end(const directory_iterator &) noexcept {
  return directory_iterator();
}

class ASAP_FILESYSTEM_API recursive_directory_iterator {
 public:
  using value_type = directory_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = directory_entry const *;
  using reference = directory_entry const &;
  using iterator_category = std::input_iterator_tag;

 public:
  // constructors and destructor

  recursive_directory_iterator() noexcept : recursion_(false) {}

  explicit recursive_directory_iterator(
      const path &__p, directory_options __xoptions = directory_options::none)
      : recursive_directory_iterator(__p, __xoptions, nullptr) {}

  recursive_directory_iterator(const path &__p, directory_options __xoptions,
                               std::error_code &ec)
      : recursive_directory_iterator(__p, __xoptions, &ec) {}

  recursive_directory_iterator(const path &__p, std::error_code &ec)
      : recursive_directory_iterator(__p, directory_options::none, &ec) {}

  recursive_directory_iterator(const recursive_directory_iterator &) = default;
  recursive_directory_iterator(recursive_directory_iterator &&) = default;

  recursive_directory_iterator &operator=(
      const recursive_directory_iterator &) = default;

  recursive_directory_iterator &operator=(
      recursive_directory_iterator &&__o) noexcept {
    // non-default implementation provided to support self-move assign.
    if (this != &__o) {
      impl_ = std::move(__o.impl_);
      recursion_ = __o.recursion_;
    }
    return *this;
  }

  ~recursive_directory_iterator() = default;

  const directory_entry &operator*() const { return dereference(); }

  const directory_entry *operator->() const { return &dereference(); }

  recursive_directory_iterator &operator++() { return do_increment(); }

  DirectoryEntryProxy_ operator++(int) {
    DirectoryEntryProxy_ __p(**this);
    do_increment();
    return __p;
  }

  recursive_directory_iterator &increment(std::error_code &ec) {
    return do_increment(&ec);
  }

  directory_options options() const;
  int depth() const;

  void pop() { pop_impl(); }

  void pop(std::error_code &ec) { pop_impl(&ec); }

  bool recursion_pending() const { return recursion_; }

  void disable_recursion_pending() { recursion_ = false; }

 private:
  recursive_directory_iterator(const path &__p, directory_options __opt,
                               std::error_code *ec);

  const directory_entry &dereference() const;

  bool TryRecursion(std::error_code *ec);

  void Advance(std::error_code *ec = nullptr);

  recursive_directory_iterator &do_increment(std::error_code *ec = nullptr);

  void pop_impl(std::error_code *ec = nullptr);

  inline friend bool operator==(const recursive_directory_iterator &,
                                const recursive_directory_iterator &) noexcept;

  struct SharedImpl;
  std::shared_ptr<SharedImpl> impl_;
  bool recursion_;
};  // class recursive_directory_iterator

inline bool operator==(const recursive_directory_iterator &lhs,
                       const recursive_directory_iterator &rhs) noexcept {
  return lhs.impl_ == rhs.impl_;
}

inline bool operator!=(const recursive_directory_iterator &lhs,
                       const recursive_directory_iterator &rhs) noexcept {
  return !(lhs == rhs);
}
// enable recursive_directory_iterator range-based for statements
inline recursive_directory_iterator begin(
    recursive_directory_iterator iter) noexcept {
  return iter;
}

inline recursive_directory_iterator end(
    const recursive_directory_iterator &) noexcept {
  return recursive_directory_iterator();
}

}  // namespace filesystem
}  // namespace asap
