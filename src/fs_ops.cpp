//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <unistd.h>

#include <common/assert.h>
#include <fmt/format.h>

#include <filesystem/filesystem.h>

namespace asap {
namespace filesystem {


// -----------------------------------------------------------------------------
//                              error handling
// -----------------------------------------------------------------------------

namespace {

std::error_code capture_errno() {
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
  const char* func_name;
  std::error_code* ec = nullptr;
  const path* p1 = nullptr;
  const path* p2 = nullptr;

  ErrorHandler(const char* fname, std::error_code* ec, const path* p1 = nullptr,
               const path* p2 = nullptr)
      : func_name(fname), ec(ec), p1(p1), p2(p2) {
    if (ec)
      ec->clear();
  }
  ErrorHandler(ErrorHandler const&) = delete;
  ErrorHandler& operator=(ErrorHandler const&) = delete;

  T report(const std::error_code& m_ec) const {
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
  }
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic pop
#endif

  template <class... Args>
  T report(const std::error_code& m_ec, const char* msg, Args const&... args) const {
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
  }
#if ASAP_COMPILER_IS_Clang || ASAP_COMPILER_IS_AppleClang
#pragma clang diagnostic pop
#endif

  T report(std::errc const& err) const { return report(make_error_code(err)); }

  template <class... Args>
  T report(std::errc const& err, const char* msg, Args const&... args) const {
    return report(make_error_code(err), msg, args...);
  }
};

}


// -----------------------------------------------------------------------------
//                              current path
// -----------------------------------------------------------------------------

path current_path_impl(std::error_code* ec) {
  ErrorHandler<path> err("current_path", ec);

  auto size = ::pathconf(".", _PC_PATH_MAX);
  ASAP_ASSERT(size > 0);

  auto buff = std::unique_ptr<char[]>(new char[size + 1]);
  char* ret;
  if ((ret = ::getcwd(buff.get(), static_cast<size_t>(size))) == nullptr)
    return err.report(capture_errno(), "call to getcwd failed");

  return {buff.get()};
}

void current_path_impl(const path& p, std::error_code* ec) {
  ErrorHandler<void> err("current_path", ec, &p);
  if (::chdir(p.c_str()) == -1)
    err.report(capture_errno());
}


// -----------------------------------------------------------------------------
//                               absolute
// -----------------------------------------------------------------------------

static path do_absolute_impl(const path &p, path *cwd, std::error_code *ec) {
  if (ec)
    ec->clear();
  if (p.is_absolute())
    return p;
  *cwd = current_path_impl(ec);
  if (ec && *ec)
    return {};
  return (*cwd) / p;
}

path absolute_impl(const path &p, std::error_code *ec) {
  path cwd;
  return do_absolute_impl(p, &cwd, ec);
}

}
}
