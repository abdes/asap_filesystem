

#pragma once

#include <memory>  // for std::shared_ptr

#include <common/assert.h>
#include <filesystem/asap_filesystem_api.h>
#include <filesystem/filesystem.h>

namespace asap {
namespace filesystem {

class __dir_stream;

class ASAP_FILESYSTEM_API directory_entry {
  typedef asap::filesystem::path _Path;

 public:
  // constructors and destructors
  directory_entry() noexcept = default;
  directory_entry(directory_entry const&) = default;
  directory_entry(directory_entry&&) noexcept = default;

  explicit directory_entry(_Path const& __p) : __p_(__p) {
    std::error_code __ec;
    __refresh(&__ec);
  }

  directory_entry(_Path const& __p, std::error_code& __ec) : __p_(__p) {
    __refresh(&__ec);
  }

  ~directory_entry() {}

  directory_entry& operator=(directory_entry const&) = default;
  directory_entry& operator=(directory_entry&&) noexcept = default;

  void assign(_Path const& __p) {
    __p_ = __p;
    std::error_code __ec;
    __refresh(&__ec);
  }

  void assign(_Path const& __p, std::error_code& __ec) {
    __p_ = __p;
    __refresh(&__ec);
  }

  void replace_filename(_Path const& __p) {
    __p_.replace_filename(__p);
    std::error_code __ec;
    __refresh(&__ec);
  }

  void replace_filename(_Path const& __p, std::error_code& __ec) {
    __p_ = __p_.parent_path() / __p;
    __refresh(&__ec);
  }

  void refresh() { __refresh(); }

  void refresh(std::error_code& __ec) noexcept { __refresh(&__ec); }

  _Path const& path() const noexcept { return __p_; }

  operator const _Path&() const noexcept { return __p_; }

  bool exists() const {
    return asap::filesystem::exists(file_status{__get_ft()});
  }

  bool exists(std::error_code& __ec) const noexcept {
    return asap::filesystem::exists(file_status{__get_ft(&__ec)});
  }

  bool is_block_file() const { return __get_ft() == file_type::block; }

  bool is_block_file(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::block;
  }

  bool is_character_file() const { return __get_ft() == file_type::character; }

  bool is_character_file(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::character;
  }

  bool is_directory() const { return __get_ft() == file_type::directory; }

  bool is_directory(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::directory;
  }

  bool is_fifo() const { return __get_ft() == file_type::fifo; }

  bool is_fifo(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::fifo;
  }

  bool is_other() const {
    return asap::filesystem::is_other(file_status{__get_ft()});
  }

  bool is_other(std::error_code& __ec) const noexcept {
    return asap::filesystem::is_other(file_status{__get_ft(&__ec)});
  }

  bool is_regular_file() const { return __get_ft() == file_type::regular; }

  bool is_regular_file(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::regular;
  }

  bool is_socket() const { return __get_ft() == file_type::socket; }

  bool is_socket(std::error_code& __ec) const noexcept {
    return __get_ft(&__ec) == file_type::socket;
  }

  bool is_symlink() const { return __get_sym_ft() == file_type::symlink; }

  bool is_symlink(std::error_code& __ec) const noexcept {
    return __get_sym_ft(&__ec) == file_type::symlink;
  }

  uintmax_t file_size() const { return __get_size(); }

  uintmax_t file_size(std::error_code& __ec) const noexcept {
    return __get_size(&__ec);
  }

  uintmax_t hard_link_count() const { return __get_nlink(); }

  uintmax_t hard_link_count(std::error_code& __ec) const noexcept {
    return __get_nlink(&__ec);
  }

  file_time_type last_write_time() const { return __get_write_time(); }

  file_time_type last_write_time(std::error_code& __ec) const noexcept {
    return __get_write_time(&__ec);
  }

  file_status status() const { return __get_status(); }

  file_status status(std::error_code& __ec) const noexcept {
    return __get_status(&__ec);
  }

  file_status symlink_status() const { return __get_symlink_status(); }

  file_status symlink_status(std::error_code& __ec) const noexcept {
    return __get_symlink_status(&__ec);
  }

  bool operator<(directory_entry const& __rhs) const noexcept {
    return __p_ < __rhs.__p_;
  }

  bool operator==(directory_entry const& __rhs) const noexcept {
    return __p_ == __rhs.__p_;
  }

  bool operator!=(directory_entry const& __rhs) const noexcept {
    return __p_ != __rhs.__p_;
  }

  bool operator<=(directory_entry const& __rhs) const noexcept {
    return __p_ <= __rhs.__p_;
  }

  bool operator>(directory_entry const& __rhs) const noexcept {
    return __p_ > __rhs.__p_;
  }

  bool operator>=(directory_entry const& __rhs) const noexcept {
    return __p_ >= __rhs.__p_;
  }

 private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  friend class __dir_stream;

  enum _CacheType : unsigned char {
    _Empty,
    _IterSymlink,
    _IterNonSymlink,
    _RefreshSymlink,
    _RefreshSymlinkUnresolved,
    _RefreshNonSymlink
  };

  struct __cached_data {
    uintmax_t __size_;
    uintmax_t __nlink_;
    file_time_type __write_time_;
    perms __sym_perms_;
    perms __non_sym_perms_;
    file_type __type_;
    _CacheType __cache_type_;

    __cached_data() noexcept { __reset(); }

    void __reset() {
      __cache_type_ = _Empty;
      __type_ = file_type::none;
      __sym_perms_ = __non_sym_perms_ = perms::unknown;
      __size_ = __nlink_ = uintmax_t(-1);
      __write_time_ = file_time_type::min();
    }
  };

  static __cached_data __create_iter_result(file_type __ft) {
    __cached_data __data;
    __data.__type_ = __ft;
    __data.__cache_type_ = [&]() {
      switch (__ft) {
        case file_type::none:
          return _Empty;
        case file_type::symlink:
          return _IterSymlink;
        default:
          return _IterNonSymlink;
      }
    }();
    return __data;
  }

  void __assign_iter_entry(_Path&& __p, __cached_data __dt) {
    __p_ = std::move(__p);
    __data_ = __dt;
  }

  ASAP_FILESYSTEM_API
  std::error_code __do_refresh() noexcept;

  static bool __is_dne_error(std::error_code const& __ec) {
    if (!__ec) return true;
    switch (static_cast<std::errc>(__ec.value())) {
      case std::errc::no_such_file_or_directory:
      case std::errc::not_a_directory:
        return true;
      default:
        return false;
    }
  }

  void __handle_error(const char* __msg, std::error_code* __dest_ec,
                      std::error_code const& __ec,
                      bool __allow_dne = false) const {
    if (__dest_ec) {
      *__dest_ec = __ec;
      return;
    }
    if (__ec && (!__allow_dne || !__is_dne_error(__ec)))
      throw filesystem_error(__msg, __p_, __ec);
  }

  void __refresh(std::error_code* __ec = nullptr) {
    __handle_error("in directory_entry::refresh", __ec, __do_refresh(),
                   /*allow_dne*/ true);
  }

  file_type __get_sym_ft(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
        return symlink_status_impl(__p_, __ec).type();
      case _IterSymlink:
      case _RefreshSymlink:
      case _RefreshSymlinkUnresolved:
        if (__ec) __ec->clear();
        return file_type::symlink;
      case _IterNonSymlink:
      case _RefreshNonSymlink:
        file_status __st(__data_.__type_);
        if (__ec && !asap::filesystem::exists(__st))
          *__ec = make_error_code(std::errc::no_such_file_or_directory);
        else if (__ec)
          __ec->clear();
        return __data_.__type_;
    }
    ASAP_UNREACHABLE();
  }

  file_type __get_ft(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterSymlink:
      case _RefreshSymlinkUnresolved:
        return status_impl(__p_, __ec).type();
      case _IterNonSymlink:
      case _RefreshNonSymlink:
      case _RefreshSymlink: {
        file_status __st(__data_.__type_);
        if (__ec && !asap::filesystem::exists(__st))
          *__ec = make_error_code(std::errc::no_such_file_or_directory);
        else if (__ec)
          __ec->clear();
        return __data_.__type_;
      }
    }
    ASAP_UNREACHABLE();
  }

  file_status __get_status(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterNonSymlink:
      case _IterSymlink:
      case _RefreshSymlinkUnresolved:
        return status_impl(__p_, __ec);
      case _RefreshNonSymlink:
      case _RefreshSymlink:
        return file_status(__get_ft(__ec), __data_.__non_sym_perms_);
    }
    ASAP_UNREACHABLE();
  }

  file_status __get_symlink_status(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterNonSymlink:
      case _IterSymlink:
        return symlink_status_impl(__p_, __ec);
      case _RefreshNonSymlink:
        return file_status(__get_sym_ft(__ec), __data_.__non_sym_perms_);
      case _RefreshSymlink:
      case _RefreshSymlinkUnresolved:
        return file_status(__get_sym_ft(__ec), __data_.__sym_perms_);
    }
    ASAP_UNREACHABLE();
  }

  uintmax_t __get_size(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterNonSymlink:
      case _IterSymlink:
      case _RefreshSymlinkUnresolved:
        return asap::filesystem::file_size_impl(__p_, __ec);
      case _RefreshSymlink:
      case _RefreshNonSymlink: {
        std::error_code __m_ec;
        file_status __st(__get_ft(&__m_ec));
        __handle_error("in directory_entry::file_size", __ec, __m_ec);
        if (asap::filesystem::exists(__st) &&
            !asap::filesystem::is_regular_file(__st)) {
          std::errc __err_kind = asap::filesystem::is_directory(__st)
                                     ? std::errc::is_a_directory
                                     : std::errc::not_supported;
          __handle_error("in directory_entry::file_size", __ec,
                         make_error_code(__err_kind));
        }
        return __data_.__size_;
      }
    }
    ASAP_UNREACHABLE();
  }

  uintmax_t __get_nlink(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterNonSymlink:
      case _IterSymlink:
      case _RefreshSymlinkUnresolved:
        return asap::filesystem::hard_link_count_impl(__p_, __ec);
      case _RefreshSymlink:
      case _RefreshNonSymlink: {
        std::error_code __m_ec;
        (void)__get_ft(&__m_ec);
        __handle_error("in directory_entry::hard_link_count", __ec, __m_ec);
        return __data_.__nlink_;
      }
    }
    ASAP_UNREACHABLE();
  }

  file_time_type __get_write_time(std::error_code* __ec = nullptr) const {
    switch (__data_.__cache_type_) {
      case _Empty:
      case _IterNonSymlink:
      case _IterSymlink:
      case _RefreshSymlinkUnresolved:
        return asap::filesystem::last_write_time_impl(__p_, __ec);
      case _RefreshSymlink:
      case _RefreshNonSymlink: {
        std::error_code __m_ec;
        file_status __st(__get_ft(&__m_ec));
        __handle_error("in directory_entry::last_write_time", __ec, __m_ec);
        if (asap::filesystem::exists(__st) &&
            __data_.__write_time_ == file_time_type::min())
          __handle_error("in directory_entry::last_write_time", __ec,
                         make_error_code(std::errc::value_too_large));
        return __data_.__write_time_;
      }
    }
    ASAP_UNREACHABLE();
  }

 private:
  _Path __p_;
  __cached_data __data_;
};

class __dir_element_proxy {
 public:
  inline directory_entry operator*() { return std::move(__elem_); }

 private:
  friend class directory_iterator;
  friend class recursive_directory_iterator;
  explicit __dir_element_proxy(directory_entry const& __e) : __elem_(__e) {}
  __dir_element_proxy(__dir_element_proxy&& __o)
      : __elem_(std::move(__o.__elem_)) {}
  directory_entry __elem_;
};

class ASAP_FILESYSTEM_API directory_iterator {
 public:
  typedef directory_entry value_type;
  typedef ptrdiff_t difference_type;
  typedef value_type const* pointer;
  typedef value_type const& reference;
  typedef std::input_iterator_tag iterator_category;

 public:
  // ctor & dtor
  directory_iterator() noexcept {}

  explicit directory_iterator(const path& __p)
      : directory_iterator(__p, nullptr) {}

  directory_iterator(const path& __p, directory_options __opts)
      : directory_iterator(__p, nullptr, __opts) {}

  directory_iterator(const path& __p, std::error_code& __ec)
      : directory_iterator(__p, &__ec) {}

  directory_iterator(const path& __p, directory_options __opts,
                     std::error_code& __ec)
      : directory_iterator(__p, &__ec, __opts) {}

  directory_iterator(const directory_iterator&) = default;
  directory_iterator(directory_iterator&&) = default;
  directory_iterator& operator=(const directory_iterator&) = default;

  directory_iterator& operator=(directory_iterator&& __o) noexcept {
    // non-default implementation provided to support self-move assign.
    if (this != &__o) {
      __imp_ = std::move(__o.__imp_);
    }
    return *this;
  }

  ~directory_iterator() = default;

  const directory_entry& operator*() const {
    ASAP_ASSERT(__imp_ && "The end iterator cannot be dereferenced");
    return __dereference();
  }

  const directory_entry* operator->() const { return &**this; }

  directory_iterator& operator++() { return __increment(); }

  __dir_element_proxy operator++(int) {
    __dir_element_proxy __p(**this);
    __increment();
    return __p;
  }

  directory_iterator& increment(std::error_code& __ec) {
    return __increment(&__ec);
  }

 private:
  inline friend bool operator==(const directory_iterator& __lhs,
                                const directory_iterator& __rhs) noexcept;

  // construct the dir_stream
  ASAP_FILESYSTEM_API
  directory_iterator(const path&, std::error_code*,
                     directory_options = directory_options::none);

  ASAP_FILESYSTEM_API
  directory_iterator& __increment(std::error_code* __ec = nullptr);

  ASAP_FILESYSTEM_API
  const directory_entry& __dereference() const;

 private:
  std::shared_ptr<__dir_stream> __imp_;
};

inline bool operator==(const directory_iterator& __lhs,
                       const directory_iterator& __rhs) noexcept {
  return __lhs.__imp_ == __rhs.__imp_;
}

inline bool operator!=(const directory_iterator& __lhs,
                       const directory_iterator& __rhs) noexcept {
  return !(__lhs == __rhs);
}

// enable directory_iterator range-based for statements
inline directory_iterator begin(directory_iterator __iter) noexcept {
  return __iter;
}

inline directory_iterator end(const directory_iterator&) noexcept {
  return directory_iterator();
}

class ASAP_FILESYSTEM_API recursive_directory_iterator {
 public:
  using value_type = directory_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = directory_entry const*;
  using reference = directory_entry const&;
  using iterator_category = std::input_iterator_tag;

 public:
  // constructors and destructor

  recursive_directory_iterator() noexcept : __rec_(false) {}

  explicit recursive_directory_iterator(
      const path& __p, directory_options __xoptions = directory_options::none)
      : recursive_directory_iterator(__p, __xoptions, nullptr) {}

  recursive_directory_iterator(const path& __p, directory_options __xoptions,
                               std::error_code& __ec)
      : recursive_directory_iterator(__p, __xoptions, &__ec) {}

  recursive_directory_iterator(const path& __p, std::error_code& __ec)
      : recursive_directory_iterator(__p, directory_options::none, &__ec) {}

  recursive_directory_iterator(const recursive_directory_iterator&) = default;
  recursive_directory_iterator(recursive_directory_iterator&&) = default;

  recursive_directory_iterator& operator=(const recursive_directory_iterator&) =
      default;

  recursive_directory_iterator& operator=(
      recursive_directory_iterator&& __o) noexcept {
    // non-default implementation provided to support self-move assign.
    if (this != &__o) {
      __imp_ = std::move(__o.__imp_);
      __rec_ = __o.__rec_;
    }
    return *this;
  }

  ~recursive_directory_iterator() = default;

  const directory_entry& operator*() const { return __dereference(); }

  const directory_entry* operator->() const { return &__dereference(); }

  recursive_directory_iterator& operator++() { return __increment(); }

  __dir_element_proxy operator++(int) {
    __dir_element_proxy __p(**this);
    __increment();
    return __p;
  }

  recursive_directory_iterator& increment(std::error_code& __ec) {
    return __increment(&__ec);
  }

  ASAP_FILESYSTEM_API directory_options options() const;
  ASAP_FILESYSTEM_API int depth() const;

  void pop() { __pop(); }

  void pop(std::error_code& __ec) { __pop(&__ec); }

  bool recursion_pending() const { return __rec_; }

  void disable_recursion_pending() { __rec_ = false; }

 private:
  recursive_directory_iterator(const path& __p, directory_options __opt,
                               std::error_code* __ec);

  ASAP_FILESYSTEM_API
  const directory_entry& __dereference() const;

  ASAP_FILESYSTEM_API
  bool __try_recursion(std::error_code* __ec);

  ASAP_FILESYSTEM_API
  void __advance(std::error_code* __ec = nullptr);

  ASAP_FILESYSTEM_API
  recursive_directory_iterator& __increment(std::error_code* __ec = nullptr);

  ASAP_FILESYSTEM_API
  void __pop(std::error_code* __ec = nullptr);

  inline friend bool operator==(const recursive_directory_iterator&,
                                const recursive_directory_iterator&) noexcept;

  struct __shared_imp;
  std::shared_ptr<__shared_imp> __imp_;
  bool __rec_;
};  // class recursive_directory_iterator

inline bool operator==(const recursive_directory_iterator& __lhs,
                       const recursive_directory_iterator& __rhs) noexcept {
  return __lhs.__imp_ == __rhs.__imp_;
}

inline bool operator!=(const recursive_directory_iterator& __lhs,
                       const recursive_directory_iterator& __rhs) noexcept {
  return !(__lhs == __rhs);
}
// enable recursive_directory_iterator range-based for statements
inline recursive_directory_iterator begin(
    recursive_directory_iterator __iter) noexcept {
  return __iter;
}

inline recursive_directory_iterator end(
    const recursive_directory_iterator&) noexcept {
  return recursive_directory_iterator();
}

}  // namespace filesystem
}  // namespace asap
