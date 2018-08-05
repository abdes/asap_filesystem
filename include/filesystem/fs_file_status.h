//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_fwd.h>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                          class file_status
// -----------------------------------------------------------------------------

class ASAP_FILESYSTEM_API file_status {
 public:
  /// @name Constructors and destructor
  //@{

  file_status() noexcept : file_status(file_type::none) {}

  explicit file_status(file_type ftype,
                       perms permissions = perms::unknown) noexcept
      : ftype_(ftype), permissions_(permissions) {}

  file_status(const file_status &) noexcept = default;
  file_status(file_status &&) noexcept = default;

  ~file_status() {}

  //@}

  /// @name assignments
  //@{

  file_status &operator=(const file_status &) noexcept = default;

  file_status &operator=(file_status &&) noexcept = default;

  //@}

  /// @name observers
  //@{

  file_type type() const noexcept { return ftype_; }

  perms permissions() const noexcept { return permissions_; }

  //@}

  /// @name modifiers
  //@{

  void type(file_type ftype) noexcept { ftype_ = ftype; }
  
  void permissions(perms permissions) noexcept { permissions_ = permissions; }

  //@}

 private:
  file_type ftype_;
  perms permissions_;
};

}  // namespace filesystem
}  // namespace asap
