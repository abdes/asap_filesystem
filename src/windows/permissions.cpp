//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: permissions
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace win32 {

	namespace {
bool equal_extension(wchar_t const *p, wchar_t const (&x1)[5],
                     wchar_t const (&x2)[5]) {
  return (p[0] == x1[0] || p[0] == x2[0]) && (p[1] == x1[1] || p[1] == x2[1]) &&
         (p[2] == x1[2] || p[2] == x2[2]) && (p[3] == x1[3] || p[3] == x2[3]) &&
         p[4] == 0;
}
}  // namespace

perms MakePermissions(const path &p, DWORD attr) {
  // TODO: See if we can get the permissions in a better way
  perms prms = perms::owner_read | perms::group_read | perms::others_read;
  if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
    prms |= perms::owner_write | perms::group_write | perms::others_write;
  auto wext = p.extension().wstring();
  wchar_t const *q = wext.c_str();
  if (equal_extension(q, L".exe", L".EXE") ||
      equal_extension(q, L".com", L".COM") ||
      equal_extension(q, L".bat", L".BAT") ||
      equal_extension(q, L".cmd", L".CMD"))
    prms |= perms::owner_exec | perms::group_exec | perms::others_exec;
  return prms;
}

}  // namespace win32
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
