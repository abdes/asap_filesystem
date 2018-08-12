//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/filesystem_error.h>
#include <filesystem/fs_path.h>

#include <fmt/format.h>

namespace asap {
namespace filesystem {

void filesystem_error::create_what(int num_paths) {
  const char *base_what = std::system_error::what();
  data_->what_ = [&]() -> std::string {
    const char *p1 = path1().native().empty() ? "\"\"" : path1().c_str();
    const char *p2 = path2().native().empty() ? "\"\"" : path2().c_str();
    switch(num_paths) {
      default:
        return fmt::format("filesystem error: {}", base_what);
      case 1:
        return fmt::format("filesystem error: {} [{}]", base_what, p1);
      case 2:
        return fmt::format("filesystem error: {} [{}] [{}]", base_what, p1, p2);
    }
  }();
}

}  // namespace filesystem
}  // namespace asap
