//        Copyright The Authors 8.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//                               file_type
// -----------------------------------------------------------------------------

///
/// @brief Defines constants that indicate a type of a file or directory a path
/// refers to. The value of the enumerators are distinct.
///
/// @see https://en.cppreference.com/w/cpp/filesystem/file_type
///
enum class file_type : signed char {
  none = 0,        // file status has not been evaluated yet, or an error
                   // occurred when evaluating it
  not_found = -1,  // file was not found (this is not considered an error)
  regular = 1,     // a regular file
  directory = 2,   // a directory
  symlink = 3,     // a symbolic link
  block = 4,       // a block special file
  character = 5,   // a character special file
  fifo = 6,        // a FIFO (also known as pipe) file
  socket = 7,      // a socket file
  unknown = 8      // the file exists but its type could not be determined
#if defined(ASAP_WINDOWS)
  ,
  reparse_file = 100  // used for symlinks on windows
#endif
};

}  // namespace filesystem
}  // namespace asap
