//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_path.h>
#include <hedley/hedley.h>

#include <memory>  // for shared_ptr
#include <string>
#include <system_error>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                          class filesystem_error
// -----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif  // __clang__

/*
MSVC will issue a C4275 warning and erors will follow due to the fact that
filesystem_error declares a DLL interface but inherits from a class that does
not declare a DLL interface (in this case std::system_error). We need the DLL
interface for filesystem_error and this works fine on non MSVC compilers.

For MSVC, we don't need to explicitly declare the DLL interface for this
class as it is inheriting from a standard library class.
*/
/*!
@brief Defines an exception object that is thrown on failure by the throwing
overloads of the functions in the filesystem library.

@see https://en.cppreference.com/w/cpp/filesystem/filesystem_error
*/
class
#if !defined(HEDLEY_MSVC_VERSION)
    ASAP_FILESYSTEM_API
#endif
        filesystem_error : public std::system_error {
 public:
  filesystem_error(const std::string& what, std::error_code ec)
      : std::system_error(ec, what),
        data_(std::make_shared<Data>(path(), path())) {
    create_what(0);
  }

  filesystem_error(const std::string& what, const path& p1, std::error_code ec)
      : std::system_error(ec, what), data_(std::make_shared<Data>(p1, path())) {
    create_what(1);
  }

  filesystem_error(const std::string& what, const path& p1, const path& p2,
                   std::error_code ec)
      : std::system_error(ec, what), data_(std::make_shared<Data>(p1, p2)) {
    create_what(2);
  }

  ~filesystem_error() override = default;

  const path& path1() const noexcept { return data_->path1_; }
  const path& path2() const noexcept { return data_->path2_; }
  const char* what() const noexcept override { return data_->what_.c_str(); }

 private:
  ASAP_FILESYSTEM_API void create_what(int num_paths);

  struct Data {
    Data(path p1, path p2) noexcept
        : path1_(std::move(p1)), path2_(std::move(p2)) {}
    path path1_;
    path path2_;
    std::string what_;
  };
  std::shared_ptr<Data> data_;
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif  // __clang__

}  // namespace filesystem
}  // namespace asap
