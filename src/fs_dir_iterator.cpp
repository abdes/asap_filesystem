//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//===------------------ directory_iterator.cpp ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <common/config.h>

#if defined(ASAP_WINDOWS)
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include <cerrno>
#include <stack>

#include <filesystem/filesystem.h>
#include "fs_error.h"

namespace asap {
namespace filesystem {

namespace detail {
namespace {

#if !defined(ASAP_WINDOWS)
template<class DirEntT, class = decltype(DirEntT::d_type)>
file_type get_file_type(DirEntT *ent) {
  switch (ent->d_type) {
    case DT_BLK:return file_type::block;
    case DT_CHR:return file_type::character;
    case DT_DIR:return file_type::directory;
    case DT_FIFO:return file_type::fifo;
    case DT_LNK:return file_type::symlink;
    case DT_REG:return file_type::regular;
    case DT_SOCK:return file_type::socket;
      // Unlike in lstat, hitting "unknown" here simply means that the
      // underlying filesystem doesn't support d_type. Report is as 'none' so we
      // correctly set the cache to empty.
    case DT_UNKNOWN:break;
  }
  return file_type::none;
}

std::pair<std::string, file_type> posix_readdir(DIR *dir_stream,
                                                std::error_code &ec) {
  struct dirent *dir_entry_ptr = nullptr;
  errno = 0;  // zero errno in order to detect errors
  ec.clear();
  if ((dir_entry_ptr = ::readdir(dir_stream)) == nullptr) {
    if (errno) ec = detail::capture_errno();
    return {};
  } else {
    return {dir_entry_ptr->d_name, get_file_type(dir_entry_ptr)};
  }
}
#else

file_type get_file_type(const WIN32_FIND_DATA &data) {
  // auto attrs = data.dwFileAttributes;
  // FIXME Windows implementation of get_file_type()
  return file_type::unknown;
}
uintmax_t get_file_size(const WIN32_FIND_DATA &data) {
  return (data.nFileSizeHigh * (MAXDWORD + 1)) + data.nFileSizeLow;
}
file_time_type get_write_time(const WIN32_FIND_DATA &data) {
  ULARGE_INTEGER tmp;
  auto &time = data.ftLastWriteTime;
  tmp.u.LowPart = time.dwLowDateTime;
  tmp.u.HighPart = time.dwHighDateTime;
  return file_time_type(file_time_type::duration(tmp.QuadPart));
}

#endif

}  // namespace
}  // namespace detail

using detail::ErrorHandler;

#if defined(ASAP_WINDOWS)
class DirectoryStream {
 public:
  DirectoryStream() = delete;
  DirectoryStream &operator=(const DirectoryStream &) = delete;

  DirectoryStream(DirectoryStream &&__ds) noexcept
      : __stream_(__ds.__stream_),
        __root_(std::move(__ds.__root_)),
        __entry_(std::move(__ds.__entry_)) {
    __ds.__stream_ = INVALID_HANDLE_VALUE;
  }

  DirectoryStream(const path &root, directory_options opts, std::error_code &ec)
      : __stream_(INVALID_HANDLE_VALUE), __root_(root) {
    __stream_ = ::FindFirstFileEx(root.c_str(), &cached_data_);
    if (__stream_ == INVALID_HANDLE_VALUE) {
      ec = std::error_code(::GetLastError(), std::generic_category());
      const bool ignore_permission_denied =
          bool(opts & directory_options::skip_permission_denied);
      if (ignore_permission_denied && ec.value() == ERROR_ACCESS_DENIED)
        ec.clear();
      return;
    }
  }

  ~DirectoryStream() noexcept {
    if (__stream_ == INVALID_HANDLE_VALUE) return;
    close();
  }

  bool good() const noexcept { return __stream_ != INVALID_HANDLE_VALUE; }

  bool advance(std::error_code &ec) {
    while (::FindNextFile(__stream_, &cached_data_)) {
      if (!strcmp(cached_data_.cFileName, ".") || strcmp(cached_data_.cFileName, ".."))
        continue;
      // FIXME: Cache more of this
      // directory_entry::CachedData_ cdata;
      // cdata.type = get_file_type(cached_data_);
      // cdata.size = get_file_size(cached_data_);
      // cdata.write_time = get_write_time(cached_data_);
      __entry_.AssignIterEntry(
          __root_ / cached_data_.cFileName,
          directory_entry::CreateIterResult(detail::get_file_type(cached_data_)));
      return true;
    }
    ec = std::error_code(::GetLastError(), std::generic_category());
    close();
    return false;
  }

 private:
  std::error_code close() noexcept {
    std::error_code ec;
    if (!::FindClose(__stream_))
      ec = std::error_code(::GetLastError(), std::generic_category());
    __stream_ = INVALID_HANDLE_VALUE;
    return ec;
  }

  HANDLE __stream_{INVALID_HANDLE_VALUE};
  WIN32_FIND_DATA cached_data_;

 public:
  path __root_;
  directory_entry __entry_;
};
#else
class DirectoryStream {
 public:
  DirectoryStream() = delete;
  DirectoryStream &operator=(const DirectoryStream &) = delete;

  DirectoryStream(DirectoryStream &&other) noexcept
      : __stream_(other.__stream_),
        __root_(std::move(other.__root_)),
        __entry_(std::move(other.__entry_)) {
    other.__stream_ = nullptr;
  }

  DirectoryStream(const path &root, directory_options opts, std::error_code &ec)
      : __stream_(nullptr), __root_(root) {
    if ((__stream_ = ::opendir(root.c_str())) == nullptr) {
      ec = detail::capture_errno();
      const auto allow_eacess =
          bool(opts & directory_options::skip_permission_denied);
      if (allow_eacess && ec.value() == EACCES) ec.clear();
      return;
    }
    advance(ec);
  }

  ~DirectoryStream() noexcept {
    if (__stream_) close();
  }

  bool good() const noexcept { return __stream_ != nullptr; }

  bool advance(std::error_code &ec) {
    while (true) {
      auto str_type_pair = detail::posix_readdir(__stream_, ec);
      auto &str = str_type_pair.first;
      if (str == "." || str == "..") {
        continue;
      } else if (ec || str.empty()) {
        close();
        return false;
      } else {
        __entry_.AssignIterEntry(
            __root_ / str,
            directory_entry::CreateIterResult(str_type_pair.second));
        return true;
      }
    }
  }

 private:
  std::error_code close() noexcept {
    std::error_code m_ec;
    if (::closedir(__stream_) == -1) m_ec = detail::capture_errno();
    __stream_ = nullptr;
    return m_ec;
  }

  DIR *__stream_{nullptr};

 public:
  path __root_;
  directory_entry __entry_;
};
#endif

// directory_iterator

directory_iterator::directory_iterator(const path &p, std::error_code *ec,
                                       directory_options opts) {
  ErrorHandler<void> err("directory_iterator::directory_iterator(...)", ec, &p);

  std::error_code m_ec;
  impl_ = std::make_shared<DirectoryStream>(p, opts, m_ec);
  if (ec) *ec = m_ec;
  if (!impl_->good()) {
    impl_.reset();
    if (m_ec) err.report(m_ec);
  }
}

directory_iterator &directory_iterator::do_increment(std::error_code *ec) {
  ErrorHandler<void> err("directory_iterator::operator++()", ec);
  if (impl_) {
    std::error_code m_ec;
    if (!impl_->advance(m_ec)) {
      path root = std::move(impl_->__root_);
      impl_.reset();
      if (m_ec) err.report(m_ec, "at root \"{}\"", root.string());
    }
  } else {
    err.report(std::errc::invalid_argument, "invalid iterator");
  }
  return *this;
}

directory_entry const &directory_iterator::dereference() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->__entry_;
}

// recursive_directory_iterator

struct recursive_directory_iterator::SharedImpl {
  std::stack<DirectoryStream> __stack_;
  directory_options __options_{directory_options::none};
};

recursive_directory_iterator::recursive_directory_iterator(
    const path &p, directory_options opt, std::error_code *ec)
    : impl_(nullptr), recursion_(true) {
  ErrorHandler<void> err("recursive_directory_iterator", ec, &p);

  std::error_code m_ec;
  DirectoryStream new_s(p, opt, m_ec);
  if (m_ec) err.report(m_ec);
  if (m_ec || !new_s.good()) return;

  impl_ = std::make_shared<SharedImpl>();
  impl_->__options_ = opt;
  impl_->__stack_.push(std::move(new_s));
}

void recursive_directory_iterator::pop_impl(std::error_code *ec) {
  ErrorHandler<void> err("directory_iterator::pop()", ec);
  if (impl_) {
    impl_->__stack_.pop();
    if (impl_->__stack_.empty())
      impl_.reset();
    else
      Advance(ec);
  } else {
    err.report(std::errc::invalid_argument, "invalid iterator");
  }
}

directory_options recursive_directory_iterator::options() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->__options_;
}

int recursive_directory_iterator::depth() const {
  ErrorHandler<int> err("recursive_directory_iterator::depth()", nullptr);
  if (!impl_) return err.report(std::errc::invalid_argument, "invalid iterator");

  return static_cast<int>(impl_->__stack_.size() - 1);
}

const directory_entry &recursive_directory_iterator::dereference() const {
  ASAP_ASSERT(impl_ && "attempt to dereference an invalid iterator");
  return impl_->__stack_.top().__entry_;
}

recursive_directory_iterator &recursive_directory_iterator::do_increment(
    std::error_code *ec) {
  if (ec) ec->clear();
  if (recursion_pending()) {
    if (TryRecursion(ec) || (ec && *ec)) return *this;
  }
  recursion_ = true;
  Advance(ec);
  return *this;
}

void recursive_directory_iterator::Advance(std::error_code *ec) {
  ErrorHandler<void> err("recursive_directory_iterator::Advance()", ec);
  if (!impl_) return err.report(std::errc::invalid_argument, "invalid iterator");

  const directory_iterator end_it;
  auto &stack = impl_->__stack_;
  std::error_code m_ec;
  while (!stack.empty()) {
    if (stack.top().advance(m_ec)) return;
    if (m_ec) break;
    stack.pop();
  }

  if (m_ec) {
    path root = std::move(stack.top().__root_);
    impl_.reset();
    err.report(m_ec, "at root \"{}\"", root.string());
  } else {
    impl_.reset();
  }
}

bool recursive_directory_iterator::TryRecursion(std::error_code *ec) {
  ErrorHandler<bool> err("recursive_directory_iterator::TryRecursion()", ec);
  if (!impl_) return err.report(make_error_code(std::errc::invalid_argument), "invalid iterator");

  auto rec_sym = bool(options() & directory_options::follow_directory_symlink);

  auto &curr_it = impl_->__stack_.top();

  bool skip_rec = false;
  std::error_code m_ec;
  if (!rec_sym) {
    file_status st(curr_it.__entry_.GetSymLinkFileType(&m_ec));
    if (m_ec && status_known(st)) m_ec.clear();
    if (m_ec || is_symlink(st) || !is_directory(st)) skip_rec = true;
  } else {
    file_status st(curr_it.__entry_.GetFileType(&m_ec));
    if (m_ec && status_known(st)) m_ec.clear();
    if (m_ec || !is_directory(st)) skip_rec = true;
  }

  if (!skip_rec) {
    DirectoryStream new_it(curr_it.__entry_.path(), impl_->__options_, m_ec);
    if (new_it.good()) {
      impl_->__stack_.push(std::move(new_it));
      return true;
    }
  }
  if (m_ec) {
    const auto allow_eacess =
        bool(impl_->__options_ & directory_options::skip_permission_denied);
    if (m_ec.value() == EACCES && allow_eacess) {
      if (ec) ec->clear();
    } else {
      path at_ent = std::move(curr_it.__entry_.path_);
      impl_.reset();
      err.report(m_ec, "attempting recursion into \"{}\"", at_ent.string());
    }
  }
  return false;
}

}  // namespace filesystem
}  // namespace asap
