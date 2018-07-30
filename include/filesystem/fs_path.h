//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <iomanip>   // for std::quoted
#include <iostream>  // for operator >> and operator <<
#include <iterator>  // for std::iterator_traits
#include <string>
#include <type_traits>  // for std::enable_if

#include <common/platform.h>
#include <common/unicode/convert.h>

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_path_traits.h>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               class path
// -----------------------------------------------------------------------------

class ASAP_FILESYSTEM_API path {
 private:
  template <typename Tp1, typename Tp2 = void>
  using IsPathable = std::enable_if_t<
      asap::conjunction<asap::negation<std::is_same<Tp1, path>>,
                        path_traits::IsConstructibleFrom<Tp1, Tp2>>::value>;

 public:
  // value_type is the character type used by this implementation to represent
  // the path internally. We follow the utf8-everywhere philosophy and we
  // exclusively use UTF-8 on any platform for the internal representation.
  // https://utf8everywhere.org/
  typedef char value_type;
  typedef std::basic_string<value_type> string_type;
#ifdef ASAP_WINDOWS
  static constexpr value_type preferred_separator = '\\';
#else
  static constexpr value_type preferred_separator = '/';
#endif

  enum format { native_format, generic_format, auto_format };

  /// @name Constructors and destructor
  //@{

  /// Constructs an empty path.
  path() noexcept {};

  /*!
   * @brief Copy constructor.
   * @param [in] p a path to copy.
   */
  path(const path &p) = default;

  path(string_type &&source, format = auto_format)
      : pathname_(std::move(source)) {
    SplitComponents();
  }

  /*!
   * @brief Move constructor.
   * Constructs a copy of p, p is left in valid but unspecified state.
   * @param [in] p a path to copy.
   */
  path(path &&p) noexcept : pathname_(std::move(p.pathname_)) {
    SplitComponents();
    p.clear();
  }

  /*!
   * @brief Constructs the path from a character sequence provided by source,
   * which is a pointer or an input iterator to a null-terminated character/wide
   * character sequence or an std::basic_string.
   * @tparam Source
   * @param source
   */
  template <typename Source, typename = IsPathable<Source>>
  path(const Source &source, format = auto_format)
      : pathname_(convert(range_begin(source), range_end(source))) {
    SplitComponents();
  }

  template <typename InputIterator, typename = IsPathable<InputIterator>>
  path(InputIterator first, InputIterator last, format = auto_format)
      : pathname_(convert(first, last)) {
    SplitComponents();
  }

  ~path() = default;

  //@}

  /// @name assignments
  //@{
  path &operator=(const path &p) = default;
  path &operator=(path &&p) noexcept;
  path &operator=(string_type &&source);
  path &assign(string_type &&source);

  template <typename Source, typename = IsPathable<Source>>
  path &operator=(Source const &source) {
    *this = path(source);
    return *this;
  }

  template <typename Source, typename = IsPathable<Source>>
  path &assign(Source const &source) {
    return *this = path(source);
  }

  template <typename InputIterator,
            typename = IsPathable<InputIterator, InputIterator>>
  path &assign(InputIterator first, InputIterator last) {
    return *this = path(first, last);
  }
  //@}

  // appends

  path &operator/=(const path &p);

  template <class Source, typename = IsPathable<Source>>
  path &operator/=(Source const &source) {
    return Append(path(source));
  }

  template <typename Source, typename = IsPathable<Source>>
  path &append(Source const &source) {
    return Append(path(source));
  }

  template <typename InputIterator,
            typename = IsPathable<InputIterator, InputIterator>>
  path &append(InputIterator first, InputIterator last) {
    return Append(path(first, last));
  }

  // concatenation

  path &operator+=(const path &other);
  path &operator+=(const string_type &other);
  path &operator+=(const value_type *other);
  path &operator+=(value_type other);

  template <typename Source, typename = IsPathable<Source>>
  path &operator+=(Source const &other) {
    return concat(other);
  }

  template <typename CharT, typename = IsPathable<CharT *>>
  path &operator+=(CharT other) {
    auto *addr = std::addressof(other);
    return concat(addr, addr + 1);
  }

  template <typename Source, typename = IsPathable<Source>>
  path &concat(Source const &other) {
    return *this += convert(range_begin(other), range_end(other));
  }

  template <typename InputIterator, typename = IsPathable<InputIterator>>
  path &concat(InputIterator first, InputIterator last) {
    return *this += convert(first, last);
  }

  /// @name Modifiers
  //@{

  void clear() noexcept {
    pathname_.clear();
    SplitComponents();
  }

  path &make_preferred();
  path &remove_filename();
  path &replace_filename(const path &replacement);
  path &replace_extension(const path &replacement = path());

  void swap(path &rhs) noexcept {
    pathname_.swap(rhs.pathname_);
    components_.swap(rhs.components_);
    std::swap(type_, rhs.type_);
  }

  //@}

  // native format observers

  const string_type &native() const noexcept { return pathname_; }
  const value_type *c_str() const noexcept { return pathname_.c_str(); }
  operator string_type() const { return pathname_; }

  template <typename CharT, typename Traits = std::char_traits<CharT>,
            typename Allocator = std::allocator<CharT>>
  std::basic_string<CharT, Traits, Allocator> string(
      const Allocator &alloc = Allocator()) const;

  std::string string() const;
  std::wstring wstring() const;
  std::string u8string() const;

  // generic format observers
  template <typename CharT, typename Traits = std::char_traits<CharT>,
            typename Allocator = std::allocator<CharT>>
  std::basic_string<CharT, Traits, Allocator> generic_string(
      const Allocator &alloc = Allocator()) const;

  std::string generic_string() const;
  std::wstring generic_wstring() const;
  std::string generic_u8string() const;

  // compare

  int compare(const path &other) const noexcept;
  int compare(const string_type &other) const;
  int compare(const value_type *other) const;

  // decomposition

  path root_name() const;
  path root_directory() const;
  path root_path() const;
  path relative_path() const;
  path parent_path() const;
  path filename() const;
  path stem() const;
  path extension() const;

  // query

  bool empty() const noexcept { return pathname_.empty(); }
  bool has_root_name() const;
  bool has_root_directory() const;
  bool has_root_path() const;
  bool has_relative_path() const;
  bool has_parent_path() const;
  bool has_filename() const;
  bool has_stem() const;
  bool has_extension() const;
  bool is_absolute() const;
  bool is_relative() const { return !is_absolute(); }

  // iterators
  class iterator;
  typedef iterator const_iterator;

  iterator begin() const;
  iterator end() const;

 private:
  template <typename Source>
  static Source range_begin(Source begin) {
    return begin;
  }

  struct null_terminated {};

  template <typename Source>
  static null_terminated range_end(Source) {
    return {};
  }

  template <typename CharT, typename Traits, typename Alloc>
  static const CharT *range_begin(
      const std::basic_string<CharT, Traits, Alloc> &str) {
    return str.data();
  }

  template <typename CharT, typename Traits, typename Alloc>
  static const CharT *range_end(
      const std::basic_string<CharT, Traits, Alloc> &str) {
    return str.data() + str.size();
  }

  // Create a basic_string by reading until a null character.
  template <typename InputIterator,
            typename Traits = std::iterator_traits<InputIterator>,
            typename CharT =
                typename std::remove_cv<typename Traits::value_type>::type>
  static std::basic_string<CharT> string_from_iter(InputIterator source) {
    std::basic_string<CharT> str;
    for (CharT ch = *source; ch != CharT(); ch = *++source) str.push_back(ch);
    return str;
  }

  //
  // Convert to string_type (no locale)
  //
  template <typename CharT>
  struct Converter;

  static string_type convert(value_type *src, null_terminated) {
    return string_type(src);
  }

  static string_type convert(const value_type *src, null_terminated) {
    return string_type(src);
  }

  template <typename _Iter>
  static string_type convert(_Iter first, _Iter last) {
    using iter_value_type = typename std::iterator_traits<_Iter>::value_type;
    return Converter<typename std::remove_cv<iter_value_type>::type>::convert(
        first, last);
  }

  template <typename InputIterator>
  static string_type convert(InputIterator src, null_terminated) {
    auto s = string_from_iter(src);
    return convert(s.c_str(), s.c_str() + s.size());
  }

  template <typename CharT, typename Traits, typename Allocator>
  static std::basic_string<CharT, Traits, Allocator> str_convert(
      const string_type &, const Allocator &alloc);

 private:
  static constexpr value_type slash = '/';
  static constexpr value_type dot = '.';

  static bool IsDirSeparator(value_type ch) {
    return ch == slash
#ifdef ASAP_WINDOWS
           || ch == preferred_separator
#endif
        ;
  }

  enum class Type : unsigned char { MULTI, ROOT_NAME, ROOT_DIR, FILENAME };

  path(string_type pathname, Type type)
      : pathname_(std::move(pathname)), type_(type) {}

  std::pair<const string_type *, size_t> FindExtension() const;

  path &Append(path p);
  string_type::size_type AppendSeparatorIfNeeded();
  void EraseRedundantSeparator(string_type::size_type sep_pos);

  void SplitComponents();
  void Trim();
  void AddRootName(size_t len);
  void AddRootDir(size_t pos);
  void AddFilename(size_t pos, size_t len);

  template <typename Allocator = std::allocator<value_type>>
  string_type make_generic(const Allocator &alloc = Allocator()) const;

  string_type pathname_;
  struct Component;
  using List = std::vector<Component>;
  List components_;  // empty unless type_ == Type::MULTI
  Type type_ = Type::MULTI;
};

inline void swap(path &lhs, path &rhs) noexcept { lhs.swap(rhs); }

size_t ASAP_FILESYSTEM_API hash_value(const path &p) noexcept;

/// An iterator for the components of a path
class ASAP_FILESYSTEM_API path::iterator {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = path;
  using reference = const path &;
  using pointer = const path *;
  using iterator_category = std::bidirectional_iterator_tag;

  iterator() : path_(nullptr), cur_(), at_end_() {}

  iterator(const iterator &) = default;
  iterator &operator=(const iterator &) = default;

  reference operator*() const;
  pointer operator->() const { return std::addressof(**this); }

  iterator &operator++();
  const iterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  iterator &operator--();
  const iterator operator--(int) {
    auto tmp = *this;
    --*this;
    return tmp;
  }

  friend bool operator==(const iterator &lhs, const iterator &rhs) {
    return lhs.equals(rhs);
  }

  friend bool operator!=(const iterator &lhs, const iterator &rhs) {
    return !lhs.equals(rhs);
  }

 private:
  friend class path;

  iterator(const path *path, path::List::const_iterator iter)
      : path_(path), cur_(iter), at_end_() {}

  iterator(const path *path, bool at_end)
      : path_(path), cur_(), at_end_(at_end) {}

  bool equals(iterator) const;

  const path *path_;
  path::List::const_iterator cur_;
  bool at_end_;  // only used when type != MULTI
};

inline bool operator<(const path &lhs, const path &rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

inline bool operator<=(const path &lhs, const path &rhs) noexcept {
  return !(rhs < lhs);
}

inline bool operator>(const path &lhs, const path &rhs) noexcept {
  return rhs < lhs;
}

inline bool operator>=(const path &lhs, const path &rhs) noexcept {
  return !(lhs < rhs);
}

inline bool operator==(const path &lhs, const path &rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

inline bool operator!=(const path &lhs, const path &rhs) noexcept {
  return !(lhs == rhs);
}

//
// Concat
//

inline path &path::operator+=(const path &other) {
  return operator+=(other.native());
}

inline path &path::operator+=(const string_type &other) {
  pathname_ += other;
  SplitComponents();
  return *this;
}

inline path &path::operator+=(const value_type *other) {
  pathname_ += other;
  SplitComponents();
  return *this;
}

inline path &path::operator+=(value_type other) {
  pathname_ += other;
  SplitComponents();
  return *this;
}

/// Append one path to another
inline path operator/(const path &lhs, const path &rhs) {
  path result(lhs);
  result /= rhs;
  return result;
}

/// Write a path to a stream
inline std::ostream &operator<<(std::ostream &os, const path &p) {
  os << std::quoted(p.string(), '"', '\\');
  return os;
}
template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits> &operator<<(
    std::basic_ostream<CharT, Traits> &os, const path &p) {
  auto tmp = p.string<CharT, Traits>();
  os << std::quoted<CharT, Traits>(tmp, CharT('"'), CharT('\\'));
  return os;
}

/// Read a path from a stream
template <typename CharT, typename Traits>
std::basic_istream<CharT, Traits> &operator>>(
    std::basic_istream<CharT, Traits> &is, path &p) {
  std::basic_string<CharT, Traits> tmp;
  if (is >> std::quoted<CharT, Traits>(tmp, CharT('"'), CharT('\\')))
    p = std::move(tmp);
  return is;
}

struct path::Component : path {
  Component(string_type pathname, Type type, size_t pos)
      : path(std::move(pathname), type), pos_(pos) {}

  size_t pos_;
};

// specialize Converter for degenerate 'noconv' case
template <>
struct path::Converter<path::value_type> {
  template <typename Iter>
  static string_type convert(Iter first, Iter last) {
    return string_type{first, last};
  }
};

template <typename CharT>
struct path::Converter {
  static string_type convert(const CharT *first, const CharT *last) {
    return nowide::narrow(first, last);
  }

  static string_type convert(CharT *first, CharT *last) {
    return convert(const_cast<const CharT *>(first),
                   const_cast<const CharT *>(last));
  }

  template <typename _Iter>
  static string_type convert(_Iter first, _Iter last) {
    const std::basic_string<CharT> str(first, last);
    return convert(str.data(), str.data() + str.size());
  }
};

template <typename CharT, typename Traits, typename Allocator>
std::basic_string<CharT, Traits, Allocator> path::str_convert(
    const string_type &str, const Allocator &alloc) {
  if (str.empty()) return std::basic_string<CharT, Traits, Allocator>(alloc);

  return nowide::widen<Allocator>(str, alloc);
}

template <typename CharT, typename Traits, typename Allocator>
inline std::basic_string<CharT, Traits, Allocator> path::string(
    const Allocator &alloc) const {
  if (std::is_same<CharT, value_type>::value)
    return {pathname_.begin(), pathname_.end(), alloc};
  else
    return str_convert<CharT, Traits>(pathname_, alloc);
}

inline std::string path::string() const { return pathname_; }

inline std::wstring path::wstring() const { return string<wchar_t>(); }

inline std::string path::u8string() const { return pathname_; }

template <typename Allocator>
inline path::string_type path::make_generic(const Allocator &alloc) const {
  string_type str(alloc);

  if (type_ == Type::ROOT_DIR)
    str.assign(1, slash);
  else {
    str.reserve(pathname_.size());
    bool add_slash = false;
    for (auto &elem : *this) {
      if (add_slash) str += slash;
      str += elem.pathname_;
      add_slash = elem.type_ == Type::FILENAME;
    }
  }
  return str;
}

template <typename CharT, typename Traits, typename Allocator>
inline std::basic_string<CharT, Traits, Allocator> path::generic_string(
    const Allocator &alloc) const {
  auto str = make_generic<Allocator>(alloc);
  return nowide::widen<Allocator>(str, alloc);
}

inline std::string path::generic_string() const { return make_generic(); }

inline std::wstring path::generic_wstring() const {
  return generic_string<wchar_t>();
}

inline std::string path::generic_u8string() const { return generic_string(); }

}  // namespace filesystem
}  // namespace asap
