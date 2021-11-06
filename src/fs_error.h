//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

// clang-format off
#include <string>
#include <system_error>

#include <common/platform.h>
#include <common/assert.h>

#if defined(ASAP_WINDOWS)
#include <Windows.h>
#endif

#include <filesystem/fs_file_type.h>
#include <filesystem/fs_file_time_type.h>
#include <filesystem/fs_perms.h>
#include <filesystem/fs_path.h>
#include <filesystem/filesystem_error.h>
// clang-format on

namespace asap {
namespace filesystem {
namespace detail {

// -----------------------------------------------------------------------------
//                        detail: error handling
// -----------------------------------------------------------------------------

inline auto capture_errno() -> std::error_code {
  int error = 0;
#if defined(ASAP_WINDOWS)
  error = ::GetLastError();
#else
  error = errno;
#endif
  ASAP_ASSERT(error != 0);
  return {error, std::generic_category()};
}

template <class T>
auto error_value() -> T;

template <>
inline constexpr void error_value<void>() {}

template <>
inline auto error_value<bool>() -> bool {
  return false;
}

template <>
inline auto error_value<uintmax_t>() -> uintmax_t {
  return uintmax_t(-1);
}

template <>
inline auto error_value<int>() -> int {
  return -1;
}

template <>
inline constexpr auto error_value<file_time_type>() -> file_time_type {
  return file_time_type::min();
}

template <>
inline auto error_value<path>() -> path {
  return {};
}

template <>
inline auto error_value<file_type>() -> file_type {
  return file_type::none;
}

template <>
inline auto error_value<perms>() -> perms {
  return perms::unknown;
}

template <class T>
struct ErrorHandler {
  const char *func_name;
  std::error_code *ec = nullptr;
  const path *p1 = nullptr;
  const path *p2 = nullptr;

  ErrorHandler(const char *fname, std::error_code *eec,
               const path *pp1 = nullptr, const path *pp2 = nullptr)
      : func_name(fname), ec(eec), p1(pp1), p2(pp2) {
    if (ec != nullptr) {
      ec->clear();
    }
  }
  ErrorHandler(ErrorHandler const &) = delete;
  auto operator=(ErrorHandler const &) -> ErrorHandler & = delete;

  auto report(const std::error_code &m_ec) const -> T {
    if (ec != nullptr) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what = std::string("in ") + func_name;
    if (p1 != nullptr) {
      if (p2 != nullptr) {
        throw filesystem_error(what, *p1, *p2, m_ec);
      }
      throw filesystem_error(what, *p1, m_ec);
    }
    throw filesystem_error(what, m_ec);

    // Unreachable
    ASAP_UNREACHABLE();
  }

  auto report(const std::error_code &m_ec, const std::string &msg) const -> T {
    if (ec != nullptr) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what =
        std::string("in ").append(func_name).append(": ").append(msg);
    if (p1 != nullptr) {
      if (p2 != nullptr) {
        throw filesystem_error(what, *p1, *p2, m_ec);
      }
      throw filesystem_error(what, *p1, m_ec);
    }
    throw filesystem_error(what, m_ec);

    // Unreachable
    ASAP_UNREACHABLE();
  }

  auto report(std::errc const &err) const -> T {
    return report(make_error_code(err));
  }

  auto report(std::errc const &err, std::string msg) const -> T {
    return report(make_error_code(err), msg);
  }
};

}  // namespace detail
}  // namespace filesystem
}  // namespace asap
