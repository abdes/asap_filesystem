//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <memory>  // for shared_ptr
#include <string>
#include <system_error>

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_path.h>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                          class filesystem_error
// -----------------------------------------------------------------------------

class filesystem_error : public std::system_error {
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
    Data(const path& p1, const path& p2) : path1_(p1), path2_(p2) {}
    path path1_;
    path path2_;
    std::string what_;
  };
  std::shared_ptr<Data> data_;
};

}  // namespace filesystem
}  // namespace asap
