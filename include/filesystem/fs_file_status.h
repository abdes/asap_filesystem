//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                          class file_status
// -----------------------------------------------------------------------------

class ASAP_FILESYSTEM_API file_status {
 public:
  // constructors
  file_status() noexcept : file_status(file_type::none) {}

  explicit file_status(file_type __ft, perms __prms = perms::unknown) noexcept
      : __ft_(__ft),
        __prms_(__prms) {}

  file_status(const file_status &) noexcept = default;
  file_status(file_status &&) noexcept = default;

  ~file_status() {}

  file_status &operator=(const file_status &) noexcept = default;
  file_status &operator=(file_status &&) noexcept = default;

  // observers

  file_type
  type() const noexcept { return __ft_; }

  perms
  permissions() const noexcept { return __prms_; }

  // modifiers

  void type(file_type __ft) noexcept { __ft_ = __ft; }

  void permissions(perms __p) noexcept { __prms_ = __p; }

 private:
  file_type __ft_;
  perms __prms_;
};

}
}

