//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <locale>

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <iterator>  // for std::iterator_traits
#include <string>
#include <type_traits>  // for std::enable_if

#include "config.h"
#include "path_traits.h"

#include <filesystem/asap_filesystem_api.h>

namespace asap {
namespace filesystem {

template <typename _OutStr, typename _InChar, typename _Codecvt,
          typename _State, typename _Fn>
bool __do_str_codecvt(const _InChar *__first, const _InChar *__last,
                      _OutStr &__outstr, const _Codecvt &__cvt, _State &__state,
                      size_t &__count, _Fn __fn) {
  if (__first == __last) {
    __outstr.clear();
    __count = 0;
    return true;
  }

  size_t __outchars = 0;
  auto __next = __first;
  const size_t __maxlen = __cvt.max_length() + 1;

  std::codecvt_base::result __result;
  do {
    __outstr.resize(__outstr.size() + (__last - __next) * __maxlen);
    auto __outnext = &__outstr.front() + __outchars;
    auto const __outlast = &__outstr.back() + 1;
    __result = (__cvt.*__fn)(__state, __next, __last, __next, __outnext,
                             __outlast, __outnext);
    __outchars = __outnext - &__outstr.front();
  } while (__result == std::codecvt_base::partial && __next != __last &&
           (__outstr.size() - __outchars) < __maxlen);

  if (__result == std::codecvt_base::error) {
    __count = __next - __first;
    return false;
  }

  if (__result == std::codecvt_base::noconv) {
    __outstr.assign(__first, __last);
    __count = __last - __first;
  } else {
    __outstr.resize(__outchars);
    __count = __next - __first;
  }

  return true;
}

// Convert narrow character string to wide.
template <typename _CharT, typename _Traits, typename _Alloc, typename _State>
inline bool __str_codecvt_in(
    const char *__first, const char *__last,
    std::basic_string<_CharT, _Traits, _Alloc> &__outstr,
    const std::codecvt<_CharT, char, _State> &__cvt, _State &__state,
    size_t &__count) {
  using _Codecvt = std::codecvt<_CharT, char, _State>;
  using _ConvFn = std::codecvt_base::result (_Codecvt::*)(
      _State &, const char *, const char *, const char *&, _CharT *, _CharT *,
      _CharT *&) const;
  _ConvFn __fn = &std::codecvt<_CharT, char, _State>::in;
  return __do_str_codecvt(__first, __last, __outstr, __cvt, __state, __count,
                          __fn);
}

template <typename _CharT, typename _Traits, typename _Alloc, typename _State>
inline bool __str_codecvt_in(
    const char *__first, const char *__last,
    std::basic_string<_CharT, _Traits, _Alloc> &__outstr,
    const std::codecvt<_CharT, char, _State> &__cvt) {
  _State __state = {};
  size_t __n;
  return __str_codecvt_in(__first, __last, __outstr, __cvt, __state, __n);
}

// Convert wide character string to narrow.
template <typename _CharT, typename _Traits, typename _Alloc, typename _State>
inline bool __str_codecvt_out(
    const _CharT *__first, const _CharT *__last,
    std::basic_string<char, _Traits, _Alloc> &__outstr,
    const std::codecvt<_CharT, char, _State> &__cvt, _State &__state,
    size_t &__count) {
  using _Codecvt = std::codecvt<_CharT, char, _State>;
  using _ConvFn = std::codecvt_base::result (_Codecvt::*)(
      _State &, const _CharT *, const _CharT *, const _CharT *&, char *, char *,
      char *&) const;
  _ConvFn __fn = &std::codecvt<_CharT, char, _State>::out;
  return __do_str_codecvt(__first, __last, __outstr, __cvt, __state, __count,
                          __fn);
}

template <typename _CharT, typename _Traits, typename _Alloc, typename _State>
inline bool __str_codecvt_out(
    const _CharT *__first, const _CharT *__last,
    std::basic_string<char, _Traits, _Alloc> &__outstr,
    const std::codecvt<_CharT, char, _State> &__cvt) {
  _State __state = {};
  size_t __n;
  return __str_codecvt_out(__first, __last, __outstr, __cvt, __state, __n);
}

// -----------------------------------------------------------------------------
//                               class path
// -----------------------------------------------------------------------------

class ASAP_FILESYSTEM_API path {
  template <typename Tp1, typename Tp2 = void>
  using IsPathable = std::enable_if_t<
      asap::conjunction<asap::negation<std::is_same<Tp1, path>>,
                        path_traits::IsConstructibleFrom<Tp1, Tp2>>::value>;

 public:
  //  value_type is the character type used by the operating system API to
  //  represent paths.
#ifdef ASAP_WINDOWS_API
  typedef wchar_t value_type;
  static constexpr value_type slash = L'/';
  static constexpr value_type preferred_separator = L'\\';
  static constexpr value_type dot = L'.';
#else
  typedef char value_type;
  static constexpr value_type slash = '/';
  static constexpr value_type preferred_separator = '/';
  static constexpr value_type dot = '.';
#endif
  typedef std::basic_string<value_type> string_type;

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
  template <typename Source, typename Require = IsPathable<Source>>
  path(const Source &source, format = auto_format)
      : pathname_(convert(range_begin(source), range_end(source))) {
    SplitComponents();
  }

  template <typename InputIterator,
            typename Require = IsPathable<InputIterator>>
  path(InputIterator first, InputIterator last, format = auto_format)
      : pathname_(convert(first, last)) {
    SplitComponents();
  }

  template <class Source, typename Require = IsPathable<Source>>
  path(const Source &source, const std::locale &loc, format = auto_format)
      : pathname_(convert_loc(range_begin(source), range_end(source), loc)) {
    SplitComponents();
  }

  template <class InputIterator, typename Require = IsPathable<InputIterator>>
  path(InputIterator first, InputIterator last, const std::locale &loc,
       format = auto_format)
      : pathname_(convert_loc(first, last, loc)) {
    SplitComponents();
  }

  ~path() = default;

  //@}

  /// @name assignments
  //@{
  path &operator=(const path &__p) = default;
  path &operator=(path &&__p) noexcept;
  path &operator=(string_type &&__source);
  path &assign(string_type &&__source);

  template <typename _Source, typename = IsPathable<_Source>>
  path &operator=(_Source const &__source) {
    return *this = path(__source);
  }

  template <typename _Source, typename = IsPathable<_Source>>
  path &assign(_Source const &__source) {
    return *this = path(__source);
  }

  template <typename _InputIterator,
            typename = IsPathable<_InputIterator, _InputIterator>>
  path &assign(_InputIterator __first, _InputIterator __last) {
    return *this = path(__first, __last);
  }
  //@}

  // appends

  path& operator/=(const path& __p);

  template <class _Source, typename = IsPathable<_Source> >
  path &
  operator/=(_Source const& __source)
  { return Append(path(__source)); }

  template<typename _Source, typename = IsPathable<_Source> >
  path &
  append(_Source const& __source)
  { return Append(path(__source)); }

  template<typename _InputIterator, typename = IsPathable<_InputIterator, _InputIterator>>
  path &
  append(_InputIterator __first, _InputIterator __last)
  { return Append(path(__first, __last)); }

  // concatenation

  path &operator+=(const path &__x);
  path &operator+=(const string_type &__x);
  path &operator+=(const value_type *__x);
  path &operator+=(value_type __x);

  template <typename _Source, typename = IsPathable<_Source> >
  path &operator+=(_Source const &__x) {
    return concat(__x);
  }

  template <typename _CharT, typename = IsPathable<_CharT *>>
  path &operator+=(_CharT __x);

  template <typename _Source, typename = IsPathable<_Source>>
  path &concat(_Source const &__x) {
    return *this += convert(range_begin(__x), range_end(__x));
  }

  template <typename _InputIterator, typename = IsPathable<_InputIterator>>
  path &concat(_InputIterator __first, _InputIterator __last) {
    return *this += convert(__first, __last);
  }

  /// @name Modifiers
  //@{

  void clear() noexcept {
    pathname_.clear();
    SplitComponents();
  }

  path &make_preferred();
  path &remove_filename();
  path &replace_filename(const path &__replacement);
  path &replace_extension(const path &__replacement = path());

  void swap(path &__rhs) noexcept {
	  pathname_.swap(__rhs.pathname_);
	  components_.swap(__rhs.components_);
	  std::swap(type_, __rhs.type_);
  }

  //@}

  // native format observers

  const string_type &native() const noexcept { return pathname_; }
  const value_type *c_str() const noexcept { return pathname_.c_str(); }
  operator string_type() const { return pathname_; }

  template <typename _CharT, typename _Traits = std::char_traits<_CharT>,
            typename _Allocator = std::allocator<_CharT>>
  std::basic_string<_CharT, _Traits, _Allocator> string(
      const _Allocator &__a = _Allocator()) const;

  std::string string() const;
  std::wstring wstring() const;
  std::string u8string() const;
  std::u16string u16string() const;
  std::u32string u32string() const;

  // generic format observers
  template <typename _CharT, typename _Traits = std::char_traits<_CharT>,
            typename _Allocator = std::allocator<_CharT>>
  std::basic_string<_CharT, _Traits, _Allocator> generic_string(
      const _Allocator &__a = _Allocator()) const;

  std::string generic_string() const;
  std::wstring generic_wstring() const;
  std::string generic_u8string() const;
  std::u16string generic_u16string() const;
  std::u32string generic_u32string() const;

  // compare

  int compare(const path& __p) const noexcept;
  int compare(const string_type& __s) const;
  int compare(const value_type* __s) const;

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
  static Source range_begin(Source __begin) {
    return __begin;
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
  template <typename _InputIterator,
            typename _Traits = std::iterator_traits<_InputIterator>,
            typename _CharT =
                typename std::remove_cv<typename _Traits::value_type>::type>
  static std::basic_string<_CharT> string_from_iter(_InputIterator __source) {
    std::basic_string<_CharT> __str;
    for (_CharT __ch = *__source; __ch != _CharT(); __ch = *++__source)
      __str.push_back(__ch);
    return __str;
  }

  inline void swap(path &__lhs, path &__rhs) noexcept { __lhs.swap(__rhs); }

  size_t hash_value(const path &__p) noexcept;

  //
  // Convert to string_type (no locale)
  //
  template <typename CharT>
  struct Converter;

  static string_type convert(value_type *__src, null_terminated) {
    return string_type(__src);
  }

  static string_type convert(const value_type *__src, null_terminated) {
    return string_type(__src);
  }

  template <typename _Iter>
  static string_type convert(_Iter __first, _Iter __last) {
    using iter_value_type = typename std::iterator_traits<_Iter>::value_type;
    return Converter<typename std::remove_cv<iter_value_type>::type>::convert(
        __first, __last);
  }

  template <typename InputIterator>
  static string_type convert(InputIterator src, null_terminated) {
    auto s = string_from_iter(src);
    return convert(s.c_str(), s.c_str() + s.size());
  }

  static string_type convert_loc(const char *__first, const char *__last,
                                 const std::locale &__loc);

  template <typename _Iter>
  static string_type convert_loc(_Iter __first, _Iter __last,
                                 const std::locale &__loc) {
    const std::string __str(__first, __last);
    return convert_loc(__str.data(), __str.data() + __str.size(), __loc);
  }

  template <typename _InputIterator>
  static string_type convert_loc(_InputIterator __src, null_terminated,
                                 const std::locale &__loc) {
    std::string __s = string_from_iter(__src);
    return convert_loc(__s.data(), __s.data() + __s.size(), __loc);
  }

  template <typename _CharT, typename _Traits, typename _Allocator>
  static std::basic_string<_CharT, _Traits, _Allocator> str_convert(
      const string_type &, const _Allocator &__a);

 private:
  static bool IsDirSeparator(value_type ch) {
    return ch == slash || ch == preferred_separator;
  }

  enum class Type : unsigned char { MULTI, ROOT_NAME, ROOT_DIR, FILENAME };

  path(string_type pathname, Type type)
      : pathname_(std::move(pathname)), type_(type) {
    //__glibcxx_assert(!empty());
    //__glibcxx_assert(type_ != Type::MULTI);
  }
  std::pair<const string_type *, size_t> FindExtension() const;

  path& Append(path __p);
  string_type::size_type AppendSeparatorIfNeeded();
  void EraseRedundantSeparator(string_type::size_type sep_pos);

  void SplitComponents();
  void Trim();
  void AddRootName(size_t len);
  void AddRootDir(size_t pos);
  void AddFilename(size_t pos, size_t len);

  string_type pathname_;
  struct Component;
  using List = std::vector<Component>;
  List components_;  // empty unless type_ == Type::MULTI
  Type type_ = Type::MULTI;
};

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
  iterator operator++(int) {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }

  iterator &operator--();
  iterator operator--(int) {
    auto __tmp = *this;
    --*this;
    return __tmp;
  }

  friend bool operator==(const iterator &__lhs, const iterator &__rhs) {
    return __lhs.equals(__rhs);
  }

  friend bool operator!=(const iterator &__lhs, const iterator &__rhs) {
    return !__lhs.equals(__rhs);
  }

 private:
  friend class path;

  iterator(const path *__path, path::List::const_iterator __iter)
      : path_(__path), cur_(__iter), at_end_() {}

  iterator(const path *__path, bool __at_end)
      : path_(__path), cur_(), at_end_(__at_end) {}

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


inline path&
path::operator+=(const path& __p)
{
  return operator+=(__p.native());
}

inline path&
path::operator+=(const string_type& __x)
{
  pathname_ += __x;
  SplitComponents();
  return *this;
}

inline path&
path::operator+=(const value_type* __x)
{
  pathname_ += __x;
  SplitComponents();
  return *this;
}

inline path&
path::operator+=(value_type __x)
{
  pathname_ += __x;
  SplitComponents();
  return *this;
}

template <typename _CharT, typename = path::IsPathable<_CharT *>>
inline path&
path::operator+=(_CharT __x)
{
	auto* __addr = std::addressof(__x);
	return concat(__addr, __addr + 1);
}


/// Append one path to another
inline path operator/(const path &__lhs, const path &__rhs) {
  path __result(__lhs);
  __result /= __rhs;
  return __result;
}

/// Write a path to a stream
template <typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits> &operator<<(
    std::basic_ostream<_CharT, _Traits> &__os, const path &__p) {
  auto __tmp = __p.string<_CharT, _Traits>();
  __os << std::quoted<_CharT, _Traits>(__tmp, _CharT('"'), _CharT('\\'));
  return __os;
}

/// Read a path from a stream
template <typename _CharT, typename _Traits>
std::basic_istream<_CharT, _Traits> &operator>>(
    std::basic_istream<_CharT, _Traits> &__is, path &__p) {
  std::basic_string<_CharT, _Traits> __tmp;
  if (__is >> std::quoted<_CharT, _Traits>(__tmp, _CharT('"'), _CharT('\\')))
    __p = std::move(__tmp);
  return __is;
}

struct path::Component : path {
  Component(string_type pathname, Type type, size_t pos)
      : path(std::move(pathname), type), pos_(pos) {}

  Component() : pos_(-1) {}

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

template <typename _CharT>
struct path::Converter {
#ifdef ASAP_WINDOWS_API
  static string_type wconvert(const char *__f, const char *__l, std::true_type) {
    using _Cvt = std::codecvt<wchar_t, char, mbstate_t>;
    const auto &__cvt = std::use_facet<_Cvt>(std::locale{});
    std::wstring __wstr;
    if (__str_codecvt_in(__f, __l, __wstr, __cvt)) return __wstr;
	//TODO: replace
	::abort();
	/*
    _GLIBCXX_THROW_OR_ABORT(
        filesystem_error("Cannot convert character sequence",
                         std::make_error_code(errc::illegal_byte_sequence)));
						 */
  }

  static string_type wconvert(const _CharT *__f, const _CharT *__l,
                              std::false_type) {
    std::codecvt_utf8<_CharT> __cvt;
    std::string __str;
    if (__str_codecvt_out(__f, __l, __str, __cvt)) {
      const char *__f2 = __str.data();
      const char *__l2 = __f2 + __str.size();
      std::codecvt_utf8<wchar_t> __wcvt;
      std::wstring __wstr;
      if (__str_codecvt_in(__f2, __l2, __wstr, __wcvt)) return __wstr;
    }
    _GLIBCXX_THROW_OR_ABORT(
        filesystem_error("Cannot convert character sequence",
                         std::make_error_code(errc::illegal_byte_sequence)));
  }

  static string_type convert(const _CharT *__f, const _CharT *__l) {
    return wconvert(__f, __l, std::is_same<_CharT, char>{});
  }
#else
  static string_type convert(const _CharT *__f, const _CharT *__l) {
    std::codecvt_utf8<_CharT> __cvt;
    std::string __str;
    if (__str_codecvt_out(__f, __l, __str, __cvt)) return __str;

    // TODO: REPLACE
    ::abort();
    /*
    _GLIBCXX_THROW_OR_ABORT(filesystem_error(
        "Cannot convert character sequence",
        std::make_error_code(errc::illegal_byte_sequence)));
        */
  }
#endif

  static string_type convert(_CharT *__f, _CharT *__l) {
    return convert(const_cast<const _CharT *>(__f),
                   const_cast<const _CharT *>(__l));
  }

  template <typename _Iter>
  static string_type convert(_Iter __first, _Iter __last) {
    const std::basic_string<_CharT> __str(__first, __last);
    return convert(__str.data(), __str.data() + __str.size());
  }
  /*
    template<typename _Iter, typename _Cont>
    static string_type
    convert(__gnu_cxx::__normal_iterator<_Iter, _Cont> __first,
               __gnu_cxx::__normal_iterator<_Iter, _Cont> __last)
    { return convert(__first.base(), __last.base()); }*/
};

template <typename _CharT, typename _Traits, typename _Allocator>
std::basic_string<_CharT, _Traits, _Allocator> path::str_convert(
    const string_type &__str, const _Allocator &__a) {
  if (__str.size() == 0)
    return std::basic_string<_CharT, _Traits, _Allocator>(__a);

  // TODO: REPLACE
  return {__str.begin(), __str.end(), __a};
}

template <typename _CharT, typename _Traits, typename _Allocator>
inline std::basic_string<_CharT, _Traits, _Allocator> path::string(
    const _Allocator &__a) const {
  if (std::is_same<_CharT, value_type>::value)
    return {pathname_.begin(), pathname_.end(), __a};
  else
    return str_convert<_CharT, _Traits>(pathname_, __a);
}

inline std::string path::string() const { return string<char>(); }

inline std::wstring path::wstring() const { return string<wchar_t>(); }

inline std::string path::u8string() const {
#ifdef ASAP_WINDOWS_API
  std::string __str;
  // convert from native encoding to UTF-8
  std::codecvt_utf8<value_type> __cvt;
  const value_type *__first = pathname_.data();
  const value_type *__last = __first + pathname_.size();
  if (__str_codecvt_out(__first, __last, __str, __cvt)) return __str;
  // TODO: replace
  ::abort();
  /*
  _GLIBCXX_THROW_OR_ABORT(
      filesystem_error("Cannot convert character sequence",
                       std::make_error_code(errc::illegal_byte_sequence)));
					   */
#else
  return pathname_;
#endif
}

inline std::u16string path::u16string() const { return string<char16_t>(); }

inline std::u32string path::u32string() const { return string<char32_t>(); }

template <typename _CharT, typename _Traits, typename _Allocator>
inline std::basic_string<_CharT, _Traits, _Allocator> path::generic_string(
    const _Allocator &__a) const {
  string_type __str(__a);

  if (type_ == Type::ROOT_DIR)
    __str.assign(1, slash);
  else {
    __str.reserve(pathname_.size());
    bool __add_slash = false;
    for (auto &__elem : *this) {
      if (__add_slash) __str += slash;
      __str += __elem.pathname_;
      __add_slash = __elem.type_ == Type::FILENAME;
    }
  }

  if (std::is_same<_CharT, value_type>::value)
    return {__str.begin(), __str.end(), __a};
  else
    return str_convert<_CharT, _Traits>(__str, __a);
}

inline std::string path::generic_string() const {
  return generic_string<char>();
}

#if _GLIBCXX_USE_WCHAR_T
inline std::wstring path::generic_wstring() const {
  return generic_string<wchar_t>();
}
#endif

inline std::string path::generic_u8string() const { return generic_string(); }

inline std::u16string path::generic_u16string() const {
  return generic_string<char16_t>();
}

inline std::u32string path::generic_u32string() const {
  return generic_string<char32_t>();
}

}  // namespace filesystem
}  // namespace asap
