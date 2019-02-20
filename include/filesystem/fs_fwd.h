//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <chrono>
#include <cstdint>
#include <system_error>  // for std::std::error_code
#include <type_traits>   // for std::underlying_type

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               classes
// -----------------------------------------------------------------------------

class path;
class filesystem_error;
class directory_entry;
class directory_iterator;
class recursive_directory_iterator;
class file_status;


// -----------------------------------------------------------------------------

using file_time_type = std::chrono::system_clock::time_point;

}  // namespace filesystem
}  // namespace asap
