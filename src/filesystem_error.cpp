//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/filesystem_error.h>
#include <filesystem/fs_path.h>

#include <sstream>

namespace asap {
namespace filesystem {

void filesystem_error::create_what(int num_paths) {
  std::ostringstream ostr;
  const char *base_what = std::system_error::what();
  ostr << "filesystem error: " << base_what;
  if (num_paths == 1) {
    const char *p1 = path1().native().empty() ? "\"\"" : path1().c_str();
    ostr << " [" << p1 << "]";
  }
  if (num_paths == 2) {
    const char *p2 = path2().native().empty() ? "\"\"" : path2().c_str();
    ostr << " [" << p2 << "]";
  }
  data_->what_ = ostr.str();
}

}  // namespace filesystem
}  // namespace asap
