//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <common/platform.h>
#include <common/unicode/convert.h>
#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_path_traits.h>

#include <algorithm>
#include <iomanip>   // for std::quoted
#include <iostream>  // for operator >> and operator <<
#include <iterator>  // for std::iterator_traits
#include <string>
#include <type_traits>  // for std::enable_if

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
  using value_type = char;
  using string_type = std::basic_string<value_type>;
#ifdef ASAP_WINDOWS
  static constexpr value_type preferred_separator = '\\';
#else
  static constexpr value_type preferred_separator = '/';
#endif

  enum class format { native_format, generic_format, auto_format };

  /// @name Constructors and destructor
  //@{

  /// Constructs an empty path.
  path() noexcept {}  // NOLINT

  /*!
   * @brief Copy constructor.
   * @param [in] p a path to copy.
   */
  path(const path &p) = default;

  // NOLINTNEXTLINE
  path(string_type &&source, format /*fmt*/ = format::auto_format)
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
   * @tparam Source the source type
   * @param source the source character sequence
   */
  template <typename Source, typename = IsPathable<Source>>
  path(const Source &source, format /*fmt*/ = format::auto_format)  // NOLINT
      : pathname_(convert(range_begin(source), range_end(source))) {
    SplitComponents();
  }

  template <typename InputIterator, typename = IsPathable<InputIterator>>
  path(InputIterator first, InputIterator last,
       format /*fmt*/ = format::auto_format)
      : pathname_(convert(first, last)) {
    SplitComponents();
  }

  ~path() = default;

  //@}

  /// @name assignments
  //@{
  auto operator=(const path &p) -> path & = default;
  auto operator=(path &&p) noexcept -> path &;
  auto operator=(string_type &&source) -> path &;
  auto assign(string_type &&source) -> path &;

  template <typename Source, typename = IsPathable<Source>>
  auto operator=(Source const &source) -> path & {
    *this = path(source);
    return *this;
  }

  template <typename Source, typename = IsPathable<Source>>
  auto assign(Source const &source) -> path & {
    return *this = path(source);
  }

  template <typename InputIterator,
            typename = IsPathable<InputIterator, InputIterator>>
  auto assign(InputIterator first, InputIterator last) -> path & {
    return *this = path(first, last);
  }
  //@}

  // appends

  auto operator/=(const path &p) -> path &;

  template <class Source, typename = IsPathable<Source>>
  auto operator/=(Source const &source) -> path & {
    return operator/=(path(source));
  }

  template <typename Source, typename = IsPathable<Source>>
  auto append(Source const &source) -> path & {
    return operator/=(path(source));
  }

  template <typename InputIterator,
            typename = IsPathable<InputIterator, InputIterator>>
  auto append(InputIterator first, InputIterator last) -> path & {
    return operator/=(path(first, last));
  }

  // concatenation

  auto operator+=(const path &other) -> path &;
  auto operator+=(const string_type &other) -> path &;
  auto operator+=(const value_type *other) -> path &;
  auto operator+=(value_type other) -> path &;

  template <typename Source, typename = IsPathable<Source>>
  auto operator+=(Source const &other) -> path & {
    return concat(other);
  }

  template <typename CharT, typename = IsPathable<CharT *>>
  auto operator+=(CharT other) -> path & {
    auto *addr = std::addressof(other);
    return concat(addr, addr + 1);
  }

  template <typename Source, typename = IsPathable<Source>>
  auto concat(Source const &other) -> path & {
    return *this += convert(range_begin(other), range_end(other));
  }

  template <typename InputIterator, typename = IsPathable<InputIterator>>
  auto concat(InputIterator first, InputIterator last) -> path & {
    return *this += convert(first, last);
  }

  /// @name Modifiers
  //@{

  void clear() noexcept {
    pathname_.clear();
    SplitComponents();
  }

  auto make_preferred() -> path &;
  auto remove_filename() -> path &;
  auto replace_filename(const path &replacement) -> path &;
  auto replace_extension(const path &replacement = path()) -> path &;

  void swap(path &rhs) noexcept {
    pathname_.swap(rhs.pathname_);
    components_.swap(rhs.components_);
    std::swap(type_, rhs.type_);
  }

  //@}

  // native format observers

  auto native() const noexcept -> const string_type & { return pathname_; }
  auto c_str() const noexcept -> const value_type * {
    return pathname_.c_str();
  }
  // NOLINTNEXTLINE
  operator string_type() const { return pathname_; }

  template <typename CharT, typename Traits = std::char_traits<CharT>,
            typename Allocator = std::allocator<CharT>>
  auto string(const Allocator &alloc = Allocator()) const
      -> std::basic_string<CharT, Traits, Allocator>;

  auto string() const -> std::string;
  auto wstring() const -> std::wstring;
  auto u8string() const -> std::string;

  // generic format observers
  template <typename CharT, typename Traits = std::char_traits<CharT>,
            typename Allocator = std::allocator<CharT>>
  auto generic_string(const Allocator &alloc = Allocator()) const
      -> std::basic_string<CharT, Traits, Allocator>;

  auto generic_string() const -> std::string;
  auto generic_wstring() const -> std::wstring;
  auto generic_u8string() const -> std::string;

  // compare

  auto compare(const path &other) const noexcept -> int;
  auto compare(const string_type &other) const -> int;
  auto compare(const value_type *other) const -> int;

  // generation
  auto lexically_normal() const -> path;
  auto lexically_relative(const path &base) const -> path;

  auto lexically_proximate(const path &base) const -> path {
    path result = this->lexically_relative(base);
    if (result.native().empty()) {
      return *this;
    }
    return result;
  }

  // decomposition

  auto root_name() const -> path;
  auto root_directory() const -> path;
  auto root_path() const -> path;
  auto relative_path() const -> path;
  auto parent_path() const -> path;
  auto filename() const -> path;
  auto stem() const -> path;
  auto extension() const -> path;

  // query

  auto empty() const noexcept -> bool { return pathname_.empty(); }
  auto has_root_name() const -> bool;
  auto has_root_directory() const -> bool;
  auto has_root_path() const -> bool;
  auto has_relative_path() const -> bool;
  auto has_parent_path() const -> bool;
  auto has_filename() const -> bool;
  auto has_stem() const -> bool;
  auto has_extension() const -> bool;
  auto is_absolute() const -> bool;
  auto is_relative() const -> bool { return !is_absolute(); }

  // iterators
  class iterator;
  using const_iterator = iterator;

  auto begin() const -> iterator;
  auto end() const -> iterator;

 private:
  template <typename Source>
  static auto range_begin(Source begin) -> Source {
    return begin;
  }

  struct null_terminated {};

  template <typename Source>
  static auto range_end(Source /*unused*/) -> null_terminated {
    return {};
  }

  template <typename CharT, typename Traits, typename Alloc>
  static auto range_begin(const std::basic_string<CharT, Traits, Alloc> &str)
      -> const CharT * {
    return str.data();
  }

  template <typename CharT, typename Traits, typename Alloc>
  static auto range_end(const std::basic_string<CharT, Traits, Alloc> &str)
      -> const CharT * {
    return str.data() + str.size();
  }

  // Create a basic_string by reading until a null character.
  template <typename InputIterator,
            typename Traits = std::iterator_traits<InputIterator>,
            typename CharT =
                typename std::remove_cv<typename Traits::value_type>::type>
  static auto string_from_iter(InputIterator source)
      -> std::basic_string<CharT> {
    std::basic_string<CharT> str;
    for (CharT ch = *source; ch != CharT(); ch = *++source) {
      str.push_back(ch);
    }
    return str;
  }

  //
  // Convert to string_type (no locale)
  //
  template <typename CharT>
  struct Converter;

  static auto convert(value_type *src, null_terminated /*unused*/)
      -> string_type {
    return {src};
  }

  static auto convert(const value_type *src, null_terminated /*unused*/)
      -> string_type {
    return {src};
  }

  template <typename Iter>
  static auto convert(Iter first, Iter last) -> string_type {
    using iter_value_type = typename std::iterator_traits<Iter>::value_type;
    return Converter<typename std::remove_cv<iter_value_type>::type>::convert(
        first, last);
  }

  template <typename InputIterator>
  static auto convert(InputIterator src, null_terminated /*unused*/)
      -> string_type {
    auto s = string_from_iter(src);
    return convert(s.c_str(), s.c_str() + s.size());
  }

  template <typename CharT, typename Traits, typename Allocator>
  static auto str_convert(const string_type & /*str*/, const Allocator &alloc)
      -> std::basic_string<CharT, Traits, Allocator>;

  static constexpr value_type slash = '/';

  static auto IsDirSeparator(value_type ch) -> bool {
    return ch == slash
#ifdef ASAP_WINDOWS
           || ch == preferred_separator
#endif
        ;
  }

  enum class Type : unsigned char { MULTI, ROOT_NAME, ROOT_DIR, FILENAME };

  path(string_type pathname, Type type)
      : pathname_(std::move(pathname)), type_(type) {}

  auto FindExtension() const -> std::pair<const string_type *, size_t>;

  auto AppendSeparatorIfNeeded() -> string_type::size_type;
  void EraseRedundantSeparator(string_type::size_type sep_pos);

  void SplitComponents();
  void Trim();
  void AddRootName(size_t len);
  void AddRootDir(size_t pos);
  void AddFilename(size_t pos, size_t len);

  template <typename Allocator = std::allocator<value_type>>
  auto make_generic(const Allocator &alloc = Allocator()) const -> string_type;

  string_type pathname_;
  struct Component;
  using List = std::vector<Component>;
  List components_;  // empty unless type_ == Type::MULTI
  Type type_ = Type::MULTI;
};

inline void swap(path &lhs, path &rhs) noexcept { lhs.swap(rhs); }

auto ASAP_FILESYSTEM_API hash_value(const path &p) noexcept -> size_t;

/// An iterator for the components of a path
class ASAP_FILESYSTEM_API path::iterator {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = path;
  using reference = const path &;
  using pointer = const path *;
  using iterator_category = std::bidirectional_iterator_tag;

  iterator() = default;

  iterator(const iterator &) = default;
  auto operator=(const iterator &) -> iterator & = default;

  auto operator*() const -> reference;
  auto operator->() const -> pointer { return std::addressof(**this); }

  auto operator++() -> iterator &;
  auto operator++(int) -> iterator {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  auto operator--() -> iterator &;
  auto operator--(int) -> iterator {
    auto tmp = *this;
    --*this;
    return tmp;
  }

  friend auto operator==(const iterator &lhs, const iterator &rhs) -> bool {
    return lhs.equals(rhs);
  }

  friend auto operator!=(const iterator &lhs, const iterator &rhs) -> bool {
    return !lhs.equals(rhs);
  }

 private:
  friend class path;

  iterator(const path *path, path::List::const_iterator iter)
      : path_(path), cur_(iter) {}

  iterator(const path *path, bool at_end) : path_(path), at_end_(at_end) {}

  auto equals(iterator) const -> bool;

  const path *path_{};
  path::List::const_iterator cur_;
  bool at_end_{};  // only used when type != MULTI
};

inline auto operator<(const path &lhs, const path &rhs) noexcept -> bool {
  return lhs.compare(rhs) < 0;
}

inline auto operator<=(const path &lhs, const path &rhs) noexcept -> bool {
  return !(rhs < lhs);
}

inline auto operator>(const path &lhs, const path &rhs) noexcept -> bool {
  return rhs < lhs;
}

inline auto operator>=(const path &lhs, const path &rhs) noexcept -> bool {
  return !(lhs < rhs);
}

inline auto operator==(const path &lhs, const path &rhs) noexcept -> bool {
  return lhs.compare(rhs) == 0;
}

inline auto operator!=(const path &lhs, const path &rhs) noexcept -> bool {
  return !(lhs == rhs);
}

//
// Concat
//

inline auto path::operator+=(const path &other) -> path & {
  return operator+=(other.native());
}

inline auto path::operator+=(const string_type &other) -> path & {
  pathname_ += other;
  SplitComponents();
  return *this;
}

inline auto path::operator+=(const value_type *other) -> path & {
  pathname_ += other;
  SplitComponents();
  return *this;
}

inline auto path::operator+=(value_type other) -> path & {
  pathname_ += other;
  SplitComponents();
  return *this;
}

/// Append one path to another
inline auto operator/(const path &lhs, const path &rhs) -> path {
  path result(lhs);
  result /= rhs;
  return result;
}

/// Write a path to a stream
inline auto operator<<(std::ostream &os, const path &p) -> std::ostream & {
  os << std::quoted(p.string(), '"', '\\');
  return os;
}
template <typename CharT, typename Traits>
auto operator<<(std::basic_ostream<CharT, Traits> &os, const path &p)
    -> std::basic_ostream<CharT, Traits> & {
  auto tmp = p.string<CharT, Traits>();
  os << std::quoted<CharT, Traits>(tmp, CharT('"'), CharT('\\'));
  return os;
}

/// Read a path from a stream
template <typename CharT, typename Traits>
auto operator>>(std::basic_istream<CharT, Traits> &is, path &p)
    -> std::basic_istream<CharT, Traits> & {
  std::basic_string<CharT, Traits> tmp;
  if (is >> std::quoted<CharT, Traits>(tmp, CharT('"'), CharT('\\'))) {
    p = std::move(tmp);
  }
  return is;
}

struct path::Component : path {
  Component(string_type pathname, Type type, size_t pos) noexcept
      : path(std::move(pathname), type), pos_(pos) {}

  size_t pos_;
};

// specialize Converter for degenerate 'noconv' case
template <>
struct path::Converter<path::value_type> {
  template <typename Iter>
  static auto convert(Iter first, Iter last) -> string_type {
    return string_type{first, last};
  }
};

template <typename CharT>
struct path::Converter {
  static auto convert(const CharT *first, const CharT *last) -> string_type {
    return nowide::narrow(first, last);
  }

  static auto convert(CharT *first, CharT *last) -> string_type {
    return convert(const_cast<const CharT *>(first),
                   const_cast<const CharT *>(last));
  }

  template <typename Iter>
  static auto convert(Iter first, Iter last) -> string_type {
    const std::basic_string<CharT> str(first, last);
    return convert(str.data(), str.data() + str.size());
  }
};

template <typename CharT, typename Traits, typename Allocator>
auto path::str_convert(const string_type &str, const Allocator &alloc)
    -> std::basic_string<CharT, Traits, Allocator> {
  if (str.empty()) {
    return std::basic_string<CharT, Traits, Allocator>(alloc);
  }

  return nowide::widen<Allocator>(str, alloc);
}

template <typename CharT, typename Traits, typename Allocator>
inline auto path::string(const Allocator &alloc) const
    -> std::basic_string<CharT, Traits, Allocator> {
  if (std::is_same<CharT, value_type>::value) {
    return {pathname_.begin(), pathname_.end(), alloc};
  }
  return str_convert<CharT, Traits>(pathname_, alloc);
}

inline auto path::string() const -> std::string { return pathname_; }

inline auto path::wstring() const -> std::wstring { return string<wchar_t>(); }

inline auto path::u8string() const -> std::string { return pathname_; }

template <typename Allocator>
inline auto path::make_generic(const Allocator &alloc) const
    -> path::string_type {
  string_type str(alloc);

  str.reserve(pathname_.size());
  bool add_slash = false;
  for (const auto &elem : *this) {
    if (elem.type_ == Type::ROOT_NAME) {
      auto rootname = elem.pathname_;
      std::replace(rootname.begin(), rootname.end(), '\\', '/');
      str += rootname;
    } else if (elem.type_ == Type::ROOT_DIR) {
      str += slash;
    } else {
      if (add_slash) {
        str += slash;
      }
      str += elem.pathname_;
      add_slash = elem.type_ == Type::FILENAME;
    }
  }

  return str;
}

template <typename CharT, typename Traits, typename Allocator>
inline auto path::generic_string(const Allocator &alloc) const
    -> std::basic_string<CharT, Traits, Allocator> {
  auto str = make_generic<Allocator>(alloc);
  return nowide::widen<Allocator>(str, alloc);
}

inline auto path::generic_string() const -> std::string {
  return make_generic();
}

inline auto path::generic_wstring() const -> std::wstring {
  return generic_string<wchar_t>();
}

inline auto path::generic_u8string() const -> std::string {
  return generic_string();
}

}  // namespace filesystem
}  // namespace asap
