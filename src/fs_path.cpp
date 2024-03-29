//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/fs_path.h>

#ifdef ASAP_WINDOWS
#include <algorithm>
#endif

#include <common/assert.h>
#include <common/platform.h>

namespace fs = asap::filesystem;
using fs::path;

namespace asap {
namespace filesystem {

namespace {
constexpr path::value_type dot = '.';
}

void path::SplitComponents() {
  components_.clear();
  if (pathname_.empty()) {
    type_ = Type::FILENAME;
    return;
  }
  type_ = Type::MULTI;

  size_t pos = 0;
  const size_t len = pathname_.size();

  // look for root name or root directory
  if (IsDirSeparator(pathname_[0])) {
#ifdef ASAP_WINDOWS
    // look for root name, such as "//foo"
    if (len > 2 && pathname_[1] == pathname_[0]) {
      if (!IsDirSeparator(pathname_[2])) {
        // got root name, find its end
        pos = 3;
        while (pos < len && !IsDirSeparator(pathname_[pos])) {
          ++pos;
        }
        AddRootName(pos);
        if (pos < len) {  // also got root directory
          AddRootDir(pos);
        }
      } else {
        // got something like "///foo" which is just a root directory
        // composed of multiple redundant directory separators
        AddRootDir(0);
      }
    } else
#endif
    {
      // got root directory
      if (pathname_.find_first_not_of('/') == string_type::npos) {
        // entire path is just slashes
        type_ = Type::ROOT_DIR;
        AddRootDir(0);
        Trim();
        return;
      }
      AddRootDir(0);
      ++pos;
    }
  }
#ifdef ASAP_WINDOWS
  else if (len > 1 && pathname_[1] == ':') {
    // got disk designator
    AddRootName(2);
    if (len > 2 && IsDirSeparator(pathname_[2])) {
      AddRootDir(2);
    }
    pos = 2;
  }
#endif

  size_t back = pos;
  while (pos < len) {
    if (IsDirSeparator(pathname_[pos])) {
      if (back != pos) {
        AddFilename(back, pos - back);
      }
      back = ++pos;
    } else {
      ++pos;
    }
  }

  if (back != pos) {
    AddFilename(back, pos - back);
  } else if (IsDirSeparator(pathname_.back())) {
    // [fs.path.itr]/4
    // An empty element, if trailing non-root directory-separator present.
    if (components_.back().type_ == Type::FILENAME) {
      pos = pathname_.size();
      components_.emplace_back(string_type(), Type::FILENAME, pos);
    }
  }

  Trim();
}

void path::Trim() {
  if (components_.size() == 1) {
    auto &component = components_.front();
    type_ = component.type_;
    pathname_ = component.pathname_;
    components_.clear();
  }
}

// -----------------------------------------------------------------------------
//  Assign
// -----------------------------------------------------------------------------

auto path::operator=(path &&p) noexcept -> path & {
  pathname_ = std::move(p.pathname_);
  components_ = std::move(p.components_);
  type_ = p.type_;
  p.clear();
  return *this;
}

auto path::operator=(string_type &&source) -> path & {
  return *this = path(std::move(source));
}

auto path::assign(string_type &&source) -> path & {
  return *this = path(std::move(source));
}

// -----------------------------------------------------------------------------
//  Query
// -----------------------------------------------------------------------------

auto path::has_stem() const -> bool {
  auto ext = FindExtension();
  return ext.first != nullptr;
}

auto path::has_extension() const -> bool {
  auto ext = FindExtension();
  return (ext.first != nullptr) && ext.second != string_type::npos &&
         ext.second != 0;
}

auto path::has_root_name() const -> bool {
  return (type_ == Type::ROOT_NAME) ||
         (!components_.empty() &&
          components_.begin()->type_ == Type::ROOT_NAME);
}

auto path::has_root_directory() const -> bool {
  return !root_directory().empty();
}

auto path::has_root_path() const -> bool { return !root_path().empty(); }

auto path::has_relative_path() const -> bool {
  return !relative_path().empty();
}

auto path::has_parent_path() const -> bool { return !parent_path().empty(); }

auto path::has_filename() const -> bool { return !filename().empty(); }

auto path::is_absolute() const -> bool {
// NOTE: //foo is absolute because we can't express relative paths on top of it
// without appending a separator.
#if defined(ASAP_WINDOWS)
  if (has_root_name()) {
    auto rn = root_name().pathname_;
    if (IsDirSeparator(rn[0]) && IsDirSeparator(rn[1])) {
      return true;
    }
  }
  return has_root_directory();
#else
  // //foo is absolute because we can't express relative paths on top of it
  // without appending a separator.
  return (has_root_directory() || has_root_name());
#endif
}

// End Query -------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  Decomposition
// -----------------------------------------------------------------------------

auto path::root_name() const -> path {
  path ret;
  if (type_ == Type::ROOT_NAME) {
    ret = *this;
  } else if (!components_.empty() &&
             components_.begin()->type_ == Type::ROOT_NAME) {
    ret = *components_.begin();
  }
  return ret;
}

auto path::root_directory() const -> path {
  path ret;
  if (type_ == Type::ROOT_DIR) {
    ret = *this;
  } else if (type_ == Type::ROOT_NAME) {
    // Check for windows special case of drive letter
    if (pathname_[1] == ':') {
      ret = path("");
    }
  } else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) {
      ++it;
    }
    if (it != components_.end() && it->type_ == Type::ROOT_DIR) {
      ret = *it;
    }
  }
  return ret;
}

auto path::root_path() const -> path {
  path ret;
  if (type_ == Type::ROOT_NAME || type_ == Type::ROOT_DIR) {
    ret = *this;
  } else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) {
      ret = *it++;
      if (it != components_.end() && it->type_ == Type::ROOT_DIR) {
        ret.pathname_ += preferred_separator;
        ret.SplitComponents();
      }
    } else if (it->type_ == Type::ROOT_DIR) {
      ret = *it;
    }
  }
  return ret;
}

auto path::relative_path() const -> path {
  path ret;
  if (type_ == Type::FILENAME) {
    ret = *this;
  } else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) {
      ++it;
    }
    if (it != components_.end() && it->type_ == Type::ROOT_DIR) {
      ++it;
    }
    if (it != components_.end()) {
      ret.assign(pathname_.substr(it->pos_));
    }
  }
  return ret;
}

auto path::parent_path() const -> path {
  if (!has_relative_path()) {
    return *this;
  }

  if (type_ == Type::MULTI) {
    ASAP_ASSERT(!components_.empty());
    path ret = *this;
    // Remove the end component
    // Note that if the end component is the empty path, we need to remove
    // the component preceding it as well. The empty path as a component
    // indicates that the previous component is a directory and it was followed
    // with a separator in the path name.
    auto last = std::prev(ret.components_.end());
    if (last->pathname_.empty()) {
      last = std::prev(last);
    }
    ret.components_.erase(last, ret.components_.end());
    ret.pathname_.clear();
    auto components_size = ret.components_.size();
    auto component_index = 1U;
    for (const auto &comp : ret.components_) {
      ret.pathname_.append(comp.pathname_);
      if (component_index < components_size) {
        ret.AppendSeparatorIfNeeded();
      }
      ++component_index;
    }
    return ret;
  }
  return {};
}

auto path::filename() const -> path {
  if (empty()) {
    return {};
  }
  if (type_ == Type::FILENAME) {
    return *this;
  }
  if (type_ == Type::MULTI) {
    if (pathname_.back() == preferred_separator) {
      return {};
    }
    const auto &last = *--end();
    if (last.type_ == Type::FILENAME) {
      return last;
    }
  }
  return {};
}

auto path::stem() const -> path {
  auto ext = FindExtension();
  if (ext.first != nullptr) {
    if (ext.second == string_type::npos || ext.second == 0) {
      return path{*ext.first};
    }
    return path{ext.first->substr(0, ext.second)};
  }
  return {};
}

auto path::extension() const -> path {
  auto ext = FindExtension();
  if ((ext.first != nullptr) && ext.second != string_type::npos &&
      ext.second != 0) {
    return path{ext.first->substr(ext.second)};
  }
  return {};
}

auto path::FindExtension() const
    -> std::pair<const path::string_type *, std::size_t> {
  const string_type *s = nullptr;

  if (type_ == Type::FILENAME) {
    s = &pathname_;
  } else if (type_ == Type::MULTI && !components_.empty()) {
    const auto &c = components_.back();
    if (c.type_ == Type::FILENAME) {
      s = &c.pathname_;
    }
  }

  if (s != nullptr) {
    if (auto sz = s->size()) {
      if (sz <= 2 && (*s)[0] == dot) {
        return {s, string_type::npos};
      }
      const auto pos = s->rfind(dot);
      return {s, pos != 0U ? pos : string_type::npos};
    }
  }
  return {};
}

// End Decomposition -----------------------------------------------------------

void path::AddRootName(size_t len) {
  auto rootname = pathname_.substr(0, len);
#if defined(ASAP_WINDOWS)
  // Replace separator with preferred separator '\'
  std::replace(rootname.begin(), rootname.end(), slash, preferred_separator);
#endif
  components_.emplace_back(rootname, Type::ROOT_NAME, 0);
}

void path::AddRootDir(size_t pos) {
  auto rootdir = pathname_.substr(pos, 1);
#if defined(ASAP_WINDOWS)
  // Replace separator with preferred separator '\'
  if (rootdir[0] == slash) {
    rootdir[0] = preferred_separator;
  }
#endif
  components_.emplace_back(rootdir, Type::ROOT_DIR, pos);
}

void path::AddFilename(size_t pos, size_t len) {
  components_.emplace_back(pathname_.substr(pos, len), Type::FILENAME, pos);
}

//
// Iteration
//

auto path::begin() const -> path::iterator {
  if (type_ == Type::MULTI) {
    return {this, components_.begin()};
  }
  return {this, empty()};
}

auto path::end() const -> path::iterator {
  if (type_ == Type::MULTI) {
    return {this, components_.end()};
  }
  return {this, true};
}

auto path::iterator::operator++() -> path::iterator & {
  ASAP_ASSERT(path_ != nullptr);
  if ((path_ != nullptr) && (path_->type_ == Type::MULTI)) {
    ASAP_ASSERT(cur_ != path_->components_.end());
    ++cur_;
  } else {
    ASAP_ASSERT(!at_end_);
    at_end_ = true;
  }
  return *this;
}

auto path::iterator::operator--() -> path::iterator & {
  ASAP_ASSERT(path_ != nullptr);
  if ((path_ != nullptr) && (path_->type_ == Type::MULTI)) {
    ASAP_ASSERT(cur_ != path_->components_.begin());
    --cur_;
  } else {
    ASAP_ASSERT(at_end_);
    at_end_ = false;
  }
  return *this;
}

auto path::iterator::operator*() const -> path::iterator::reference {
  ASAP_ASSERT(path_ != nullptr);
  if ((path_ != nullptr) && (path_->type_ == Type::MULTI)) {
    ASAP_ASSERT(cur_ != path_->components_.end());
    return *cur_;
  }
  return *path_;
}

auto path::iterator::equals(iterator rhs) const -> bool {
  if (path_ != rhs.path_) {
    return false;
  }
  if (path_ == nullptr) {
    return true;
  }
  if (path_->type_ == path::Type::MULTI) {
    return cur_ == rhs.cur_;
  }
  return at_end_ == rhs.at_end_;
}

//------------------------------------------------------------------------------
// Compare
//------------------------------------------------------------------------------

namespace {
template <typename Iter1, typename Iter2>
auto do_compare(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2) -> int {
  int cmpt = 1;
  while (begin1 != end1 && begin2 != end2) {
    if (begin1->native() < begin2->native()) {
      return -cmpt;
    }
    if (begin1->native() > begin2->native()) {
      return +cmpt;
    }
    ++begin1;
    ++begin2;
    ++cmpt;
  }
  if (begin1 == end1) {
    if (begin2 == end2) {
      return 0;
    }
    return -cmpt;
  }
  return +cmpt;
}
}  // namespace

auto path::compare(const path &other) const noexcept -> int {
  struct CmptRef {
    const path *ptr;
    auto native() const noexcept -> const string_type & {
      return ptr->native();
    }
  };

  if (empty() && other.empty()) {
    return 0;
  }
  if (type_ == Type::MULTI && other.type_ == Type::MULTI) {
    return do_compare(components_.begin(), components_.end(),
                      other.components_.begin(), other.components_.end());
  }
  if (type_ == Type::MULTI) {
    CmptRef c[1] = {{&other}};
    return do_compare(components_.begin(), components_.end(), c, c + 1);
  }
  if (other.type_ == Type::MULTI) {
    CmptRef c[1] = {{this}};
    return do_compare(c, c + 1, other.components_.begin(),
                      other.components_.end());
  }
  return pathname_.compare(other.pathname_);
}
auto path::compare(const string_type &other) const -> int {
  return compare(path(other));
}

auto path::compare(const value_type *other) const -> int {
  return compare(path(other));
}

// End Compare -----------------------------------------------------------------

//
// Append
//

auto path::operator/=(const path &p) -> path & {
  // NOTE: the standard is not clear when it comes to handling root names.
  // The following examples were specified for windows:
  //   path("foo") / "c:/bar"; // yields "c:/bar"
  //   path("foo") / "c:";       // yields "c:"
  //   path("c:") / "";          // yields "c:"
  //   path("c:foo") / "/bar";   // yields "c:/bar"
  //   path("c:foo") / "c:bar";  // yields "c:foo/bar"
  //
  // But...
  //
  // 1) If p.is_absolute() || (p.has_root_name() && p.root_name() !=
  // root_name()), then replaces the current path with p as if by operator=(p)
  // and finishes.
  //   * Otherwise, if p.has_root_directory(), then removes any root directory
  //     and the entire relative path from the generic format pathname of *this
  //   * Otherwise, if has_filename() || (!has_root_directory() &&
  //   is_absolute()),
  //     then appends path::preferred_separator to the generic format of *this
  //   * Either way, then appends the native format pathname of p, omitting any
  //     root-name from its generic format, to the native format of *this.

  if ((p.is_absolute() && !this->has_root_name()) ||
      (p.has_root_name() && p.root_name() != this->root_name())) {
    return operator=(p);
  }

  auto lhs = pathname_;
  bool add_sep = false;

  if (p.has_root_directory()) {
    // Remove any root directory and relative path
    if (type_ != Type::ROOT_NAME) {
      if (!components_.empty() &&
          components_.front().type_ == Type::ROOT_NAME) {
        lhs = components_.front().pathname_;
      } else {
        lhs = {};
      }
    }
  } else if (has_filename() || (!has_root_directory() && is_absolute())) {
    add_sep = true;
  }

  auto rhs = p.pathname_;
  // Omit any root-name from the generic format pathname:
  if (p.type_ == Type::ROOT_NAME) {
    rhs = {};
  } else if (!p.components_.empty() &&
             p.components_.front().type_ == Type::ROOT_NAME) {
    rhs.erase(0, p.components_.front().pathname_.size());
  }

  const size_t len = lhs.size() + static_cast<size_t>(add_sep) + rhs.size();
  const size_t maxcmpts = components_.size() + p.components_.size();
  if (pathname_.capacity() < len || components_.capacity() < maxcmpts) {
    // Construct new path and swap (strong exception-safety guarantee).
    string_type tmp;
    tmp.reserve(len);
    tmp = lhs;
    if (add_sep) {
      tmp += preferred_separator;
    }
    tmp += rhs;
    path newp(std::move(tmp));
    swap(newp);
  } else {
    pathname_ = lhs;
    if (add_sep) {
      pathname_ += preferred_separator;
    }
    pathname_ += rhs;
    SplitComponents();
  }
  return *this;
}

auto path::AppendSeparatorIfNeeded() -> path::string_type::size_type {
  if (!pathname_.empty() &&
#ifdef ASAP_WINDOWS
      *(pathname_.end() - 1) != ':' &&
#endif
      !IsDirSeparator(*(pathname_.end() - 1))) {
    string_type::size_type tmp(pathname_.size());
    pathname_ += preferred_separator;
    return tmp;
  }
  return 0;
}

auto hash_value(const path &p) noexcept -> std::size_t {
  // [path.non-member]
  // "If for two paths, p1 == p2 then hash_value(p1) == hash_value(p2)."
  // Equality works as if by traversing the range [begin(), end()), meaning
  // e.g. path("a//b") == path("a/b"), so we cannot simply hash pathname_
  // but need to iterate over individual elements. Use the hash_combine from
  // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3876.pdf
  size_t seed = 0;
  for (const auto &x : p) {
    seed ^= std::hash<path::string_type>()(x.native()) + 0x9e3779b9 +
            (seed << 6) + (seed >> 2);
  }
  return seed;
}

//
//  MODIFIERS
//

auto path::make_preferred() -> path & {
#ifdef ASAP_WINDOWS
  std::replace(pathname_.begin(), pathname_.end(), slash, preferred_separator);
#endif
  return *this;
}

auto path::remove_filename() -> path & {
  if (type_ == Type::MULTI) {
    if (!components_.empty()) {
      auto cmpt = std::prev(components_.end());
      if (cmpt->type_ == Type::FILENAME && !cmpt->empty()) {
        pathname_.erase(cmpt->pos_);
        auto prev = std::prev(cmpt);
        if (prev->type_ == Type::ROOT_DIR || prev->type_ == Type::ROOT_NAME) {
          components_.erase(cmpt);
          Trim();
        } else {
          cmpt->clear();
        }
      }
    }
  } else if (type_ == Type::FILENAME) {
    clear();
  }
  return *this;
}

auto path::replace_filename(const path &replacement) -> path & {
  remove_filename();
  operator/=(replacement);
  return *this;
}

auto path::replace_extension(const path &replacement) -> path & {
  auto ext = FindExtension();
  // Any existing extension() is removed
  if ((ext.first != nullptr) && ext.second != string_type::npos) {
    if (ext.first == &pathname_) {
      pathname_.erase(ext.second);
    } else {
      const auto &back = components_.back();
      if (ext.first != &back.pathname_) {
        throw(std::logic_error("path::replace_extension failed"));
      }
      pathname_.erase(back.pos_ + ext.second);
    }
  }
  // If replacement is not empty and does not begin with a dot character,
  // a dot character is appended
  if (!replacement.empty() && replacement.native()[0] != dot) {
    pathname_ += dot;
  }
  operator+=(replacement);
  return *this;
}

namespace {
inline auto is_dot(fs::path::value_type c) -> bool { return c == dot; }

inline auto is_dot(const fs::path &path) -> bool {
  const auto &filename = path.native();
  return filename.size() == 1 && is_dot(filename[0]);
}

inline auto is_dotdot(const fs::path &path) -> bool {
  const auto &filename = path.native();
  return filename.size() == 2 && is_dot(filename[0]) && is_dot(filename[1]);
}
}  // namespace

auto path::lexically_normal() const -> path {
  /*
  C++17 [fs.path.generic] p6
  - If the path is empty, stop.
  - Replace each slash character in the root-name with a preferred-separator.
  - Replace each directory-separator with a preferred-separator.
  - Remove each dot filename and any immediately following directory-separator.
  - As long as any appear, remove a non-dot-dot filename immediately followed
    by a directory-separator and a dot-dot filename, along with any immediately
    following directory-separator.
  - If there is a root-directory, remove all dot-dot filenames and any
    directory-separators immediately following them.
  - If the last filename is dot-dot, remove any trailing directory-separator.
  - If the path is empty, add a dot.
  */
  path ret;
  // If the path is empty, stop.
  if (empty()) {
    return ret;
  }
  for (const auto &p : *this) {
#ifdef ASAP_WINDOWS
    // Replace each slash character in the root-name
    if (p.type_ == Type::ROOT_NAME || p.type_ == Type::ROOT_DIR) {
      string_type s = p.native();
      std::replace(s.begin(), s.end(), slash, preferred_separator);
      ret /= s;
      continue;
    }
#endif
    if (is_dotdot(p)) {
      if (ret.has_filename()) {
        // remove a non-dot-dot filename immediately followed by /..
        if (!is_dotdot(ret.filename())) {
          ret.remove_filename();
        } else {
          ret /= p;
        }
      } else if (!ret.has_relative_path()) {
        // remove a dot-dot filename immediately after root-directory
        if (!ret.has_root_directory()) {
          ret /= p;
        }
      } else {
        // Got a path with a relative path (i.e. at least one non-root
        // element) and no filename at the end (i.e. empty last element),
        // so must have a trailing slash. See what is before it.
        auto elem = std::prev(ret.end(), 2);
        if (elem->has_filename() && !is_dotdot(*elem)) {
          // Remove the filename before the trailing slash
          // (equiv. to ret = ret.parent_path().remove_filename())
          if (elem == ret.begin()) {
            ret.clear();
          } else {
            ret.pathname_.erase(elem.cur_->pos_);
            // Do we still have a trailing slash?
            if (std::prev(elem)->type_ == Type::FILENAME) {
              ret.components_.erase(elem.cur_);
            } else {
              ret.components_.erase(elem.cur_, ret.components_.end());
            }
          }
        } else {  // ???
          ret /= p;
        }
      }
    } else if (is_dot(p)) {
      ret /= path();
    } else {
      ret /= p;
    }
  }

  if (ret.components_.size() >= 2) {
    auto back = std::prev(ret.components_.end());
    // If the last filename is dot-dot, ...
    if (back->empty() && is_dotdot(*std::prev(back))) {
      // ... remove any trailing directory-separator.
      ret.components_.erase(back);
      ret.pathname_.erase(std::prev(ret.pathname_.end()));
    }
  }
  // If the path is empty, add a dot.
  else if (ret.empty()) {
    ret = ".";
  }

  return ret;
}

auto path::lexically_relative(const path &base) const -> path {
  path ret;
  if (root_name() != base.root_name()) {
    return ret;
  }
  if (is_absolute() != base.is_absolute()) {
    return ret;
  }
  if (!has_root_directory() && base.has_root_directory()) {
    return ret;
  }
  auto p = std::mismatch(begin(), end(), base.begin(), base.end());
  auto a = p.first;
  auto b = p.second;
  if (a == end() && b == base.end()) {
    ret = ".";
  } else {
    int n = 0;
    for (; b != base.end(); ++b) {
      const path &bp = *b;
      if (is_dotdot(bp)) {
        --n;
      } else if (!is_dot(bp)) {
        ++n;
      }
    }
    if (n >= 0) {
      const path dotdot("..");
      while ((n--) != 0) {
        ret /= dotdot;
      }
      for (; a != end(); ++a) {
        ret /= *a;
      }
    }
  }
  return ret;
}

}  // namespace filesystem
}  // namespace asap
