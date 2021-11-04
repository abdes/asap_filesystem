//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#pragma once

#include <common/traits/logical.h>
#include <hedley/hedley.h>

#include <list>
#include <string>
#include <vector>

namespace asap {
namespace filesystem {

namespace path_traits {

template <typename CharT, typename Ch = typename std::remove_const<CharT>::type>
using IsEncodedChar =
    asap::disjunction<std::is_same<Ch, char>, std::is_same<Ch, wchar_t>,
                      std::is_same<Ch, char16_t>, std::is_same<Ch, char32_t>>;

template <typename Iter, typename Iter_traits = std::iterator_traits<Iter>>
using IsPathableIter =
    asap::conjunction<IsEncodedChar<typename Iter_traits::value_type>,
                      std::is_base_of<std::input_iterator_tag,
                                      typename Iter_traits::iterator_category>>;
#if HEDLEY_HAS_WARNING("-Wunused-template")
HEDLEY_DIAGNOSTIC_PUSH
HEDLEY_PRAGMA(clang diagnostic ignored "-Wunused-template")
#endif

template <typename Iter>
static IsPathableIter<Iter> IsPathable(Iter, int);

template <typename CharT, typename Traits, typename Alloc>
static IsEncodedChar<CharT> IsPathable(
    const std::basic_string<CharT, Traits, Alloc> &, int);

template <typename Unknown>
static std::false_type IsPathable(const Unknown &, ...);

#if HEDLEY_HAS_WARNING("-Wunused-template")
HEDLEY_DIAGNOSTIC_POP
#endif

template <typename Tp1, typename Tp2>
struct IsConstructibleFrom;

template <typename Iter>
struct IsConstructibleFrom<Iter, Iter> : IsPathableIter<Iter> {};

template <typename Source>
struct IsConstructibleFrom<Source, void>
    : decltype(IsPathable(std::declval<Source>(), 0)) {};

}  // namespace path_traits
}  // namespace filesystem
}  // namespace asap
