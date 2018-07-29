//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <filesystem/path.h>

#ifdef ASAP_WINDOWS
# include <algorithm>
#endif

#include <common/assert.h>
#include <common/platform.h>

namespace fs = asap::filesystem;
using fs::path;

namespace asap {
namespace filesystem {

// -----------------------------------------------------------------------------
//  Assign
// -----------------------------------------------------------------------------

path &path::operator=(path &&p) noexcept {
  pathname_ = std::move(p.pathname_);
  components_ = std::move(p.components_);
  type_ = p.type_;
  p.clear();
  return *this;
}

path &path::operator=(string_type &&__source) {
  return *this = path(std::move(__source));
}

path &path::assign(string_type &&__source) {
  return *this = path(std::move(__source));
}

// -----------------------------------------------------------------------------
//  Query
// -----------------------------------------------------------------------------

bool path::has_stem() const {
  auto ext = FindExtension();
  return ext.first;
}

bool path::has_extension() const {
  auto ext = FindExtension();
  return ext.first && ext.second != string_type::npos && ext.second != 0;
}

bool path::has_root_name() const {
  return (type_ == Type::ROOT_NAME) ||
         (!components_.empty() &&
          components_.begin()->type_ == Type::ROOT_NAME);
}

bool path::has_root_directory() const { return !root_directory().empty(); }

bool path::has_root_path() const { return !root_path().empty(); }

bool path::has_relative_path() const { return !relative_path().empty(); }

bool path::has_parent_path() const {
  return !parent_path().empty();
  ;
}

bool path::has_filename() const { return !filename().empty(); }

bool path::is_absolute() const {
  return (has_root_directory() || (has_root_name() && has_root_directory()));
}

// End Query -------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  Decomposition
// -----------------------------------------------------------------------------

path path::root_name() const {
  path ret;
  if (type_ == Type::ROOT_NAME)
    ret = *this;
  else if (!components_.empty() &&
           components_.begin()->type_ == Type::ROOT_NAME)
    ret = *components_.begin();
  return ret;
}

path path::root_directory() const {
  path ret;
  if (type_ == Type::ROOT_DIR) {
    // There will be one single component which has the simplified root dir '/'.
    // That component does not have components! Test for this case to avoid
    // issues with the recursive descent over parent directories.
    if (!components_.empty())
      ret = *components_.begin();
    else
      ret = *this;
  } else if (type_ == Type::ROOT_NAME) {
    // Check for windows special case of drive letter
    if (pathname_[1] == ':')
      ret = path("");
    else
      ret = path("/");
  } else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) ++it;
    if (it != components_.end() && it->type_ == Type::ROOT_DIR) ret = *it;
  }
  return ret;
}

path path::root_path() const {
  path ret;
  if (type_ == Type::ROOT_NAME || type_ == Type::ROOT_DIR)
    ret = *this;
  else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) {
      ret = *it++;
      if (it != components_.end() && it->type_ == Type::ROOT_DIR) {
        ret.pathname_ += preferred_separator;
        ret.SplitComponents();
      }
    } else if (it->type_ == Type::ROOT_DIR)
      ret = *it;
  }
  return ret;
}

path path::relative_path() const {
  path ret;
  if (type_ == Type::FILENAME)
    ret = *this;
  else if (!components_.empty()) {
    auto it = components_.begin();
    if (it->type_ == Type::ROOT_NAME) ++it;
    if (it != components_.end() && it->type_ == Type::ROOT_DIR) ++it;
    if (it != components_.end()) ret.assign(pathname_.substr(it->pos_));
  }
  return ret;
}

path path::parent_path() const {
  path __ret;
  if (!has_relative_path()) {
    if (type_ == Type::ROOT_DIR)
      __ret = *components_.begin();
    else
      __ret = *this;
  } else if (components_.size() >= 2) {
    for (auto __it = components_.begin(), __end = std::prev(components_.end());
         __it != __end; ++__it) {
      __ret /= *__it;
    }
  }
  return __ret;

  /*
    path ret;
    if (components_.size() < 2) {
      if (type_ == Type::ROOT_DIR) return *this;
      if (type_ == Type::ROOT_NAME) return path("/");
      return ret;
    }
    for (auto it = components_.begin(), end = std::prev(components_.end());
         it != end; ++it) {
      ret /= *it;
    }
    return ret;
  */
}

path path::filename() const {
  if (empty())
    return {};
  else if (type_ == Type::FILENAME)
    return *this;
  else if (type_ == Type::MULTI) {
    if (pathname_.back() == preferred_separator) return {};
    auto &__last = *--end();
    if (__last.type_ == Type::FILENAME) return __last;
  }
  return {};
}

path path::stem() const {
  auto ext = FindExtension();
  if (ext.first) {
    if (ext.second == string_type::npos || ext.second == 0)
      return path{*ext.first};
    else
      return path{ext.first->substr(0, ext.second)};
  }
  return {};
}

path path::extension() const {
  auto ext = FindExtension();
  if (ext.first && ext.second != string_type::npos && ext.second != 0)
    return path{ext.first->substr(ext.second)};
  return {};
}

std::pair<const path::string_type *, std::size_t> path::FindExtension() const {
  const string_type *s = nullptr;

  if (type_ == Type::FILENAME)
    s = &pathname_;
  else if (type_ == Type::MULTI && !components_.empty()) {
    const auto &c = components_.back();
    if (c.type_ == Type::FILENAME) s = &c.pathname_;
  }

  if (s) {
    if (auto sz = s->size()) {
      if (sz <= 2 && (*s)[0] == dot) return {s, string_type::npos};
      const auto pos = s->rfind(dot);
      return {s, pos ? pos : string_type::npos};
    }
  }
  return {};
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
        while (pos < len && !IsDirSeparator(pathname_[pos])) ++pos;
        AddRootName(pos);
        if (pos < len)  // also got root directory
          AddRootDir(pos);
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
        return;
      }
      AddRootDir(0);
      ++pos;
    }
  }
#ifdef ASAP_WINDOWS
  else if (len > 1 && pathname_[1] == L':') {
    // got disk designator
    AddRootName(2);
    if (len > 2 && IsDirSeparator(pathname_[2])) AddRootDir(2);
    pos = 2;
  }
#endif

  size_t back = pos;
  while (pos < len) {
    if (IsDirSeparator(pathname_[pos])) {
      if (back != pos) AddFilename(back, pos - back);
      back = ++pos;
    } else
      ++pos;
  }

  if (back != pos)
    AddFilename(back, pos - back);
  else if (IsDirSeparator(pathname_.back())) {
    // [fs.path.itr]/4
    // An empty element, if trailing non-root directory-separator present.
    if (components_.back().type_ == Type::FILENAME) {
      pos = pathname_.size();
      components_.emplace_back(string_type(), Type::FILENAME, pos);
    }
  }

  Trim();
}
// End Decomposition -----------------------------------------------------------

void path::AddRootName(size_t len) {
  components_.emplace_back(pathname_.substr(0, len), Type::ROOT_NAME, 0);
}

void path::AddRootDir(size_t pos) {
  components_.emplace_back(pathname_.substr(pos, 1), Type::ROOT_DIR, pos);
}

void path::AddFilename(size_t pos, size_t len) {
  components_.emplace_back(pathname_.substr(pos, len), Type::FILENAME, pos);
}

void path::Trim() {
  if (components_.size() == 1) {
    type_ = components_.front().type_;
    components_.clear();
  }
}

//
// Iteration
//

path::iterator path::begin() const {
  if (type_ == Type::MULTI) return iterator(this, components_.begin());
  return iterator(this, empty());
}

path::iterator path::end() const {
  if (type_ == Type::MULTI) return iterator(this, components_.end());
  return iterator(this, true);
}

path::iterator &path::iterator::operator++() {
  ASAP_ASSERT(path_ != nullptr);
  if (path_->type_ == Type::MULTI) {
    ASAP_ASSERT(cur_ != path_->components_.end());
    ++cur_;
  } else {
    ASAP_ASSERT(!at_end_);
    at_end_ = true;
  }
  return *this;
}

path::iterator &path::iterator::operator--() {
  ASAP_ASSERT(path_ != nullptr);
  if (path_->type_ == Type::MULTI) {
    ASAP_ASSERT(cur_ != path_->components_.begin());
    --cur_;
  } else {
    ASAP_ASSERT(at_end_);
    at_end_ = false;
  }
  return *this;
}

path::iterator::reference path::iterator::operator*() const {
  ASAP_ASSERT(path_ != nullptr);
  if (path_->type_ == Type::MULTI) {
    ASAP_ASSERT(cur_ != path_->components_.end());
    return *cur_;
  }
  return *path_;
}

bool path::iterator::equals(iterator rhs) const {
  if (path_ != rhs.path_) return false;
  if (path_ == nullptr) return true;
  if (path_->type_ == path::Type::MULTI) return cur_ == rhs.cur_;
  return at_end_ == rhs.at_end_;
}

//------------------------------------------------------------------------------
// Compare
//------------------------------------------------------------------------------

namespace {
template <typename Iter1, typename Iter2>
int do_compare(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2) {
  int cmpt = 1;
  while (begin1 != end1 && begin2 != end2) {
    if (begin1->native() < begin2->native()) return -cmpt;
    if (begin1->native() > begin2->native()) return +cmpt;
    ++begin1;
    ++begin2;
    ++cmpt;
  }
  if (begin1 == end1) {
    if (begin2 == end2) return 0;
    return -cmpt;
  }
  return +cmpt;
}
}  // namespace

int path::compare(const path &other) const noexcept {
  struct CmptRef {
    const path *ptr;
    const string_type &native() const noexcept { return ptr->native(); }
  };

  if (empty() && other.empty())
    return 0;
  else if (type_ == Type::MULTI && other.type_ == Type::MULTI)
    return do_compare(components_.begin(), components_.end(),
                      other.components_.begin(), other.components_.end());
  else if (type_ == Type::MULTI) {
    CmptRef c[1] = {{&other}};
    return do_compare(components_.begin(), components_.end(), c, c + 1);
  } else if (other.type_ == Type::MULTI) {
    CmptRef c[1] = {{this}};
    return do_compare(c, c + 1, other.components_.begin(),
                      other.components_.end());
  } else
    return pathname_.compare(other.pathname_);
}
int path::compare(const string_type &other) const {
  return compare(path(other));
}

int path::compare(const value_type *other) const {
  return compare(path(other));
}

// End Compare -----------------------------------------------------------------

//
// Append
//

path &path::operator/=(const path &p) {
  if (p.is_absolute())
    operator=(p);
  else {
    if (this == &p)  // self-append
    {
      path rhs(p);
      AppendSeparatorIfNeeded();
      pathname_.append(rhs.pathname_);
    } else {
      AppendSeparatorIfNeeded();
      pathname_.append(p.pathname_);
    }
    SplitComponents();
  }
  return *this;
}

path &path::Append(path p) {
  if (p.is_absolute())
    operator=(std::move(p));
  else {
    AppendSeparatorIfNeeded();
    pathname_.append(p.pathname_);
    SplitComponents();
  }
  return *this;
}

path::string_type::size_type path::AppendSeparatorIfNeeded() {
  if (!pathname_.empty() &&
#ifdef ASAP_WINDOWS_API
      *(pathname_.end() - 1) != ':' &&
#endif
      !IsDirSeparator(*(pathname_.end() - 1))) {
    string_type::size_type tmp(pathname_.size());
    pathname_ += preferred_separator;
    return tmp;
  }
  return 0;
}

void path::EraseRedundantSeparator(string_type::size_type sep_pos) {
  if (sep_pos                              // a separator was added
      && sep_pos < pathname_.size()        // and something was appended
      && (pathname_[sep_pos + 1] == slash  // and it was also separator
#ifdef BOOST_WINDOWS_API
          || m_pathname[sep_pos + 1] ==
                 preferred_separator  // or preferred_separator
#endif
          )) {
    pathname_.erase(sep_pos, 1);
  }  // erase the added separator
}

std::size_t hash_value(const path &p) noexcept {
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

path &path::make_preferred() {
#ifdef ASAP_WINDOWS
  std::replace(pathname_.begin(), pathname_.end(), slash, preferred_separator);
#endif
  return *this;
}

path &path::remove_filename() {
  if (type_ == Type::MULTI) {
    if (!components_.empty()) {
      auto cmpt = std::prev(components_.end());
      if (cmpt->type_ == Type::FILENAME && !cmpt->empty()) {
        pathname_.erase(cmpt->pos_);
        auto prev = std::prev(cmpt);
        if (prev->type_ == Type::ROOT_DIR || prev->type_ == Type::ROOT_NAME) {
          components_.erase(cmpt);
          Trim();
        } else
          cmpt->clear();
      }
    }
  } else if (type_ == Type::FILENAME)
    clear();
  return *this;
}

path &path::replace_filename(const path &replacement) {
  remove_filename();
  operator/=(replacement);
  return *this;
}

path &path::replace_extension(const path &replacement) {
  auto ext = FindExtension();
  // Any existing extension() is removed
  if (ext.first && ext.second != string_type::npos) {
    if (ext.first == &pathname_)
      pathname_.erase(ext.second);
    else {
      const auto &back = components_.back();
      if (ext.first != &back.pathname_)
        throw(std::logic_error("path::replace_extension failed"));
      pathname_.erase(back.pos_ + ext.second);
    }
  }
  // If replacement is not empty and does not begin with a dot character,
  // a dot character is appended
  if (!replacement.empty() && replacement.native()[0] != dot) pathname_ += dot;
  operator+=(replacement);
  return *this;
}

}  // namespace filesystem
}  // namespace asap
