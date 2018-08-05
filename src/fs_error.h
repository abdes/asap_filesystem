//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <string>
#include <system_error>

#include <common/assert.h>
#include <common/platform.h>
#include <fmt/format.h>

#include <filesystem/fs_path.h>

namespace asap {
namespace filesystem {

namespace detail {
namespace {

// -----------------------------------------------------------------------------
//                        detail: error handling
// -----------------------------------------------------------------------------

inline std::error_code capture_errno() {
  ASAP_ASSERT(errno != 0);
  return {errno, std::generic_category()};
}

template <class T>
T error_value();

template <>
constexpr void error_value<void>() {}

template <>
bool error_value<bool>() {
  return false;
}

template <>
uintmax_t error_value<uintmax_t>() {
  return uintmax_t(-1);
}

template <>
constexpr file_time_type error_value<file_time_type>() {
  return file_time_type::min();
}

template <>
path error_value<path>() {
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
    ASAP_ASSERT_FAIL();
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#endif
#if ASAP_COMPILER_IS_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
  }
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic pop
#endif
#if ASAP_COMPILER_IS_GNU
#pragma GCC diagnostic pop
#endif

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
    ASAP_ASSERT_FAIL();
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#endif
#if ASAP_COMPILER_IS_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
  }
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic pop
#endif
#if ASAP_COMPILER_IS_GNU
#pragma GCC diagnostic pop
#endif

  T report(std::errc const &err) const { return report(make_error_code(err)); }

  template <class... Args>
  T report(std::errc const &err, const char *msg, Args const &... args) const {
    return report(std::make_error_code(err), msg, args...);
  }
};

}  // namespace
}  // namespace detail
}  // namespace filesystem
}  // namespace asap
