//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/filesystem_error.h>
#include <filesystem/fs_path.h>

namespace asap {
namespace filesystem {


filesystem_error::~filesystem_error() {}

void filesystem_error::create_what(int num_paths) {
  const char* base_what = system_error::what();
  data_->what_ = [&]() -> std::string {
    std::string rv("filesystem error: ");
    rv.append(base_what);
    if (num_paths > 0) {
      auto &p1_native = path1().native();
      if (p1_native.empty()) rv.append(" \"\"");
      else rv.append(" [").append(p1_native).append("]");
    }
    if (num_paths > 1) {
      auto &p2_native = path2().native();
      if (p2_native.empty()) rv.append(" \"\"");
      else rv.append(" [").append(p2_native).append("]");
    }
    return rv;
  }();
}

}
}

