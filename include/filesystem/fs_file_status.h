//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <filesystem/asap_filesystem_api.h>
#include <filesystem/fs_file_type.h>
#include <filesystem/fs_perms.h>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                          class file_status
// -----------------------------------------------------------------------------

/*!
@brief Stores information about the type and permissions of a file.

@see https://en.cppreference.com/w/cpp/filesystem/file_status
*/
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

  ~file_status() = default;

  //@}

  /// @name assignments
  //@{

  auto operator=(const file_status &) noexcept -> file_status & = default;

  auto operator=(file_status &&) noexcept -> file_status & = default;

  //@}

  /// @name observers
  //@{

  auto type() const noexcept -> file_type { return ftype_; }

  auto permissions() const noexcept -> perms { return permissions_; }

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
