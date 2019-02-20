//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <chrono>

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               file_time_type
// -----------------------------------------------------------------------------


using file_time_type = std::chrono::system_clock::time_point;

}  // namespace filesystem
}  // namespace asap
