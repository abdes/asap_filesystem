//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <string>
#include <system_error>

#include <common/assert.h>
#include <common/platform.h>
// Silence this warning as fmt lib checks properly for the compiler support of
// string literal operator template before enabling it
#if ASAP_COMPILER_IS_AppleClang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif
#include <fmt/format.h>
#if ASAP_COMPILER_IS_AppleClang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic pop
#endif

#include <filesystem/fs_fwd.h>
#include <filesystem/fs_path.h>
#include <filesystem/filesystem_error.h>


namespace asap {
namespace filesystem {
namespace detail {

// -----------------------------------------------------------------------------
//                        detail: error handling
// -----------------------------------------------------------------------------

inline std::error_code capture_errno() {
  int error;
#if defined(ASAP_WINDOWS)
  error = GetLastError();
#else
  error = errno;
#endif
  ASAP_ASSERT(error != 0);
  return {error, std::generic_category()};
}

template <class T>
T error_value();

template <>
inline constexpr void error_value<void>() {}

template <>
inline bool error_value<bool>() {
  return false;
}

template <>
inline uintmax_t error_value<uintmax_t>() {
  return uintmax_t(-1);
}

template <>
inline int error_value<int>() {
  return -1;
}

template <>
inline constexpr file_time_type error_value<file_time_type>() {
  return file_time_type::min();
}

template <>
inline path error_value<path>() {
  return {};
}

template <class T>
struct ErrorHandler {
  const char *func_name;
  std::error_code *ec = nullptr;
  const path *p1 = nullptr;
  const path *p2 = nullptr;

  ErrorHandler(const char *fname, std::error_code *ec, const path *p1 = nullptr,
               const path *p2 = nullptr)
      : func_name(fname), ec(ec), p1(p1), p2(p2) {
    if (ec) ec->clear();
  }
  ErrorHandler(ErrorHandler const &) = delete;
  ErrorHandler &operator=(ErrorHandler const &) = delete;

  T report(const std::error_code &m_ec) const {
    if (ec) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what = std::string("in ") + func_name;
    switch (bool(p1) + bool(p2)) {
      case 0:
        throw filesystem_error(what, m_ec);
      case 1:
        throw filesystem_error(what, *p1, m_ec);
      case 2:
        throw filesystem_error(what, *p1, *p2, m_ec);
    }
    // Unreachable
    ASAP_UNREACHABLE();
  }

  template <class... Args>
  T report(const std::error_code &m_ec, const char *msg,
           Args const &... args) const {
    if (ec) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what =
        std::string("in ") + func_name + ": " + fmt::format(msg, args...);
    switch (bool(p1) + bool(p2)) {
      case 0:
        throw filesystem_error(what, m_ec);
      case 1:
        throw filesystem_error(what, *p1, m_ec);
      case 2:
        throw filesystem_error(what, *p1, *p2, m_ec);
    }
    // Unreachable
    ASAP_UNREACHABLE();
  }

  T report(std::errc const &err) const { return report(make_error_code(err)); }

  template <class... Args>
  T report(std::errc const &err, const char *msg, Args const &... args) const {
    return report(std::make_error_code(err), msg, args...);
  }
};

}  // namespace detail
}  // namespace filesystem
}  // namespace asap
