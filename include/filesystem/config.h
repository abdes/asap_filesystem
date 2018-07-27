//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <common/platform.h>

#ifdef ASAP_WINDOWS
#define ASAP_WINDOWS_API
#else
#define ASAP_POSIX_API
#endif
