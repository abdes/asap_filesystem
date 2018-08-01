//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <common/assert.h>
#include <fmt/format.h>

#include <filesystem/filesystem.h>

namespace asap {
namespace filesystem {



namespace {

// -----------------------------------------------------------------------------
//                              error handling
// -----------------------------------------------------------------------------

std::error_code capture_errno() {
  ASAP_ASSERT(errno != 0);
  return {errno, std::generic_category()};
}

template<class T>
T error_value();
template<>
constexpr void error_value<void>() {}
template<>
bool error_value<bool>() {
  return false;
}
template<>
uintmax_t error_value<uintmax_t>() {
  return uintmax_t(-1);
}
template<>
constexpr file_time_type error_value<file_time_type>() {
  return file_time_type::min();
}
template<>
path error_value<path>() {
  return {};
}

template<class T>
struct ErrorHandler {
  const char *func_name;
  std::error_code *ec = nullptr;
  const path *p1 = nullptr;
  const path *p2 = nullptr;

  ErrorHandler(const char *fname, std::error_code *ec, const path *p1 = nullptr,
               const path *p2 = nullptr)
      : func_name(fname), ec(ec), p1(p1), p2(p2) {
    if (ec)
      ec->clear();
  }
  ErrorHandler(ErrorHandler const &) = delete;
  ErrorHandler &operator=(ErrorHandler const &) = delete;

  T report(const std::error_code &m_ec) const {
    if (ec) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what = std::string("in ") + func_name;
    switch (bool(p1) +bool(p2)) {
      case 0:throw filesystem_error(what, m_ec);
      case 1:throw filesystem_error(what, *p1, m_ec);
      case 2:throw filesystem_error(what, *p1, *p2, m_ec);
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

  template<class... Args>
  T report(const std::error_code &m_ec,
           const char *msg,
           Args const &... args) const {
    if (ec) {
      *ec = m_ec;
      return error_value<T>();
    }
    std::string what =
        std::string("in ") + func_name + ": " + fmt::format(msg, args...);
    switch (bool(p1) +bool(p2)) {
      case 0:throw filesystem_error(what, m_ec);
      case 1:throw filesystem_error(what, *p1, m_ec);
      case 2:throw filesystem_error(what, *p1, *p2, m_ec);
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

  T report(std::errc const &err) const { return report(make_error_code(err)); }

  template<class... Args>
  T report(std::errc const &err, const char *msg, Args const &... args) const {
    return report(std::make_error_code(err), msg, args...);
  }
};


// -----------------------------------------------------------------------------
//                              posix stat
// -----------------------------------------------------------------------------

using StatT = struct ::stat;

perms posix_get_perms(const StatT& st) noexcept {
  return static_cast<perms>(st.st_mode) & perms::mask;
}

::mode_t posix_convert_perms(perms prms) {
  return static_cast< ::mode_t>(prms & perms::mask);
}


file_status create_file_status(std::error_code& m_ec, path const& p,
                               const StatT& path_stat, std::error_code* ec) {
  if (ec)
    *ec = m_ec;
  if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
    return file_status(file_type::not_found);
  } else if (m_ec) {
    ErrorHandler<void> err("posix_stat", ec, &p);
    err.report(m_ec, "failed to determine attributes for the specified path");
    return file_status(file_type::none);
  }
  // else

  file_status fs_tmp;
  auto const mode = path_stat.st_mode;
  if (S_ISLNK(mode))
    fs_tmp.type(file_type::symlink);
  else if (S_ISREG(mode))
    fs_tmp.type(file_type::regular);
  else if (S_ISDIR(mode))
    fs_tmp.type(file_type::directory);
  else if (S_ISBLK(mode))
    fs_tmp.type(file_type::block);
  else if (S_ISCHR(mode))
    fs_tmp.type(file_type::character);
  else if (S_ISFIFO(mode))
    fs_tmp.type(file_type::fifo);
  else if (S_ISSOCK(mode))
    fs_tmp.type(file_type::socket);
  else
    fs_tmp.type(file_type::unknown);

  fs_tmp.permissions(posix_get_perms(path_stat));
  return fs_tmp;
}


file_status posix_stat(path const& p, StatT& path_stat, std::error_code* ec) {
  std::error_code m_ec;
  if (::stat(p.c_str(), &path_stat) == -1)
    m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_status posix_stat(path const& p, std::error_code* ec) {
  StatT path_stat;
  return posix_stat(p, path_stat, ec);
}

file_status posix_lstat(path const& p, StatT& path_stat, std::error_code* ec) {
  std::error_code m_ec;
  if (::lstat(p.c_str(), &path_stat) == -1)
    m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

file_status posix_lstat(path const& p, std::error_code* ec) {
  StatT path_stat;
  return posix_lstat(p, path_stat, ec);
}

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


// -----------------------------------------------------------------------------
//                               canonical
// -----------------------------------------------------------------------------

path canonical_impl(path const& orig_p, std::error_code* ec) {
  path cwd;
  ErrorHandler<path> err("canonical", ec, &orig_p, &cwd);

  path pa = do_absolute_impl(orig_p, &cwd, ec);

#ifndef ASAP_WINDOWS
  // Use OpenGroup realpath()
  char buff[PATH_MAX + 1];
  char* ret;
  if ((ret = ::realpath(pa.c_str(), buff)) == nullptr)
    return err.report(capture_errno());
  return {ret};
#else
  // TODO: windows implementation of canonical()
  return err.report(std::errc::not_supported);
#endif
}



// -----------------------------------------------------------------------------
//                              current path
// -----------------------------------------------------------------------------

path current_path_impl(std::error_code *ec) {
  ErrorHandler<path> err("current_path", ec);

  auto size = ::pathconf(".", _PC_PATH_MAX);
  ASAP_ASSERT(size > 0);

  auto buff = std::unique_ptr<char[]>(new char[size + 1]);
  if (::getcwd(buff.get(), static_cast<size_t>(size)) == nullptr)
    return err.report(capture_errno(), "call to getcwd failed");

  return {buff.get()};
}

void current_path_impl(const path &p, std::error_code *ec) {
  ErrorHandler<void> err("current_path", ec, &p);
  if (::chdir(p.c_str()) == -1)
    err.report(capture_errno());
}


// -----------------------------------------------------------------------------
//                               remove
// -----------------------------------------------------------------------------


bool remove_impl(const path& p, std::error_code* ec) {
  ErrorHandler<bool> err("remove", ec, &p);
  if (::remove(p.c_str()) == -1) {
    if (errno != ENOENT)
      err.report(capture_errno());
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
//                               remove_all
// -----------------------------------------------------------------------------

/*
namespace {

uintmax_t do_remove_all_impl(path const& p, std::error_code& ec) {
  const auto npos = static_cast<uintmax_t>(-1);
  const file_status st = symlink_status_impl(p, &ec);
  if (ec)
    return npos;
  uintmax_t count = 1;
  if (is_directory(st)) {
    for (directory_iterator it(p, ec); !ec && it != directory_iterator();
         it.increment(ec)) {
      auto other_count = remove_all_impl(it->path(), ec);
      if (ec)
        return npos;
      count += other_count;
    }
    if (ec)
      return npos;
  }
  if (!remove_impl(p, &ec))
    return npos;
  return count;
}

} // end namespace

uintmax_t remove_all_impl(const path& p, std::error_code* ec) {
  ErrorHandler<uintmax_t> err("remove_all", ec, &p);

  std::error_code mec;
  auto count = do_remove_all_impl(p, mec);
  if (mec) {
    if (mec == std::errc::no_such_file_or_directory)
      return 0;
    return err.report(mec);
  }
  return count;
}
 */

// -----------------------------------------------------------------------------
//                               rename
// -----------------------------------------------------------------------------

void rename_impl(const path& from, const path& to, std::error_code* ec) {
  ErrorHandler<void> err("rename", ec, &from, &to);
  if (::rename(from.c_str(), to.c_str()) == -1)
    err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                               resize_file
// -----------------------------------------------------------------------------

void resize_file_impl(const path& p, uintmax_t size, std::error_code* ec) {
  ErrorHandler<void> err("resize_file", ec, &p);
  if (::truncate(p.c_str(), static_cast< ::off_t>(size)) == -1)
    return err.report(capture_errno());
}

// -----------------------------------------------------------------------------
//                               space
// -----------------------------------------------------------------------------

space_info __space(const path& p, std::error_code* ec) {
  ErrorHandler<void> err("space", ec, &p);
  space_info si;
  struct statvfs m_svfs = {};
  if (::statvfs(p.c_str(), &m_svfs) == -1) {
    err.report(capture_errno());
    si.capacity = si.free = si.available = static_cast<uintmax_t>(-1);
    return si;
  }
  // Multiply with overflow checking.
  auto do_mult = [&](uintmax_t& out, uintmax_t other) {
    out = other * m_svfs.f_frsize;
    if (other == 0 || out / other != m_svfs.f_frsize)
      out = static_cast<uintmax_t>(-1);
  };
  do_mult(si.capacity, m_svfs.f_blocks);
  do_mult(si.free, m_svfs.f_bfree);
  do_mult(si.available, m_svfs.f_bavail);
  return si;
}


// -----------------------------------------------------------------------------
//                               status
// -----------------------------------------------------------------------------


file_status status_impl(const path& p, std::error_code* ec) {
  return posix_stat(p, ec);
}

file_status symlink_status_impl(const path& p, std::error_code* ec) {
  return posix_lstat(p, ec);
}



// -----------------------------------------------------------------------------
//                               temp_directory_path
// -----------------------------------------------------------------------------

path temp_directory_path_impl(std::error_code *ec) {
  ErrorHandler<path> err("temp_directory_path", ec);

  path p;
#ifdef ASAP_WINDOWS
  // GetTempPathW
  // GetLastError
#else
  const char *env_paths[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR"};
  const char *ret = nullptr;

  for (auto &ep : env_paths)
    if ((ret = ::getenv(ep)))
      break;
  if (ret == nullptr)
    ret = "/tmp";
  p = path(ret);
#endif

  std::error_code status_ec;
  file_status st = status(p, status_ec);
  if (!status_known(st))
    return err.report(status_ec, "cannot access path \"{}\"", p.string());

  if (!exists(st) || !is_directory(st))
    return err.report(std::errc::not_a_directory, "path \"{}\" is not a directory",
                      p.string());

  return p;
}


// -----------------------------------------------------------------------------
//                               weakly_canonical
// -----------------------------------------------------------------------------

path weakly_canonical_impl(const path& p, std::error_code* ec) {
  ErrorHandler<path> err("weakly_canonical", ec, &p);

  if (p.empty())
    return canonical_impl("", ec);

  path result;
  std::error_code m_ec;
  file_status st = status_impl(p, &m_ec);
  if (!status_known(st)) {
    return err.report(m_ec);
  } else if (exists(st)) {
    return canonical_impl(p, ec);
  }

  path tmp;
  auto iter = p.begin(), end = p.end();
  // find leading elements of p that exist:
  while (iter != end)
  {
    tmp = result / *iter;
    st = status_impl(tmp, &m_ec);
    if (exists(st)) swap(result, tmp);
    else {
      if (status_known(st)) m_ec.clear();
      break;
    }
    ++iter;
  }
  // canonicalize:
  if (!m_ec && !result.empty())
    result = canonical_impl(result, &m_ec);
  if (m_ec) result.clear();
  else {
    // append the non-existing elements:
    while (iter != end)
      result /= *iter++;
    // normalize:
    result = result.lexically_normal();
  }
  return result;
}


}
}
