//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <common/flag_ops.h>

#include <deque>
#include <vector>

#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: permissions
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace win32_port {

enum class TrusteeType { owner = 0, group, others, system, admins };
enum class OperationType { read = 0, write, exec };

namespace mapping {
namespace {
const perms posix[3][3] = {
    {perms::owner_read, perms::owner_write, perms::owner_exec},
    {perms::group_read, perms::group_write, perms::group_exec},
    {perms::others_read, perms::others_write, perms::others_exec}};
}

// clang-format off
namespace file {
constexpr unsigned long read_mask = FILE_READ_ATTRIBUTES | 
                                    FILE_READ_DATA |
                                    FILE_READ_EA | 
	                                STANDARD_RIGHTS_READ |
                                    SYNCHRONIZE;

// We are explicitly omitting DELETE, WRITE_DAC and WRITE_OWNER
constexpr unsigned long write_mask = FILE_APPEND_DATA |
	                                 FILE_WRITE_ATTRIBUTES |
	                                 FILE_WRITE_DATA |
                                     FILE_WRITE_EA |
	                                 STANDARD_RIGHTS_WRITE |
	                                 SYNCHRONIZE;

constexpr unsigned long exec_mask = FILE_EXECUTE |
                                    FILE_READ_ATTRIBUTES |
	                                STANDARD_RIGHTS_EXECUTE |
	                                SYNCHRONIZE;
}  // namespace file

namespace directory {

constexpr unsigned long read_mask = FILE_READ_ATTRIBUTES |
                                    FILE_LIST_DIRECTORY |
                                    FILE_READ_EA |
	                                STANDARD_RIGHTS_READ |
                                    SYNCHRONIZE;

// We are explicitly omitting DELETE, WRITE_DAC and WRITE_OWNER
constexpr unsigned long write_mask = FILE_ADD_SUBDIRECTORY |
	                                 FILE_DELETE_CHILD |
	                                 FILE_WRITE_ATTRIBUTES |
                                     FILE_ADD_FILE |
	                                 FILE_WRITE_EA |
	                                 STANDARD_RIGHTS_WRITE |
	                                 SYNCHRONIZE;

constexpr unsigned long exec_mask = FILE_TRAVERSE |
                                    FILE_READ_ATTRIBUTES |
                                    STANDARD_RIGHTS_EXECUTE |
	                                SYNCHRONIZE;
}  // namespace directory
// clang-format on
}  // namespace mapping

// We need this to be exported for testing
auto ASAP_FILESYSTEM_API MapAccessMaskToPerms(ACCESS_MASK mask,
                                              TrusteeType trustee,
                                              file_type type) -> perms {
  if (type == file_type::none || type == file_type::unknown ||
      type == file_type::not_found) {
    return perms::unknown;
  }

  if (mask == 0) {
    return perms::none;
  }

  int op = 0;
  int tr =
      static_cast<typename std::underlying_type<TrusteeType>::type>(trustee);
  auto prm = perms::none;
  if (type == file_type::directory) {
    if (FlagTest(mask, mapping::directory::read_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::read);
      prm |= mapping::posix[tr][op];
    }
    if (FlagTest(mask, mapping::directory::write_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::write);
      prm |= mapping::posix[tr][op];
    }
    if (FlagTest(mask, mapping::directory::exec_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::exec);
      prm |= mapping::posix[tr][op];
    }
  } else {
    if (FlagTest(mask, mapping::file::read_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::read);
      prm |= mapping::posix[tr][op];
    }
    if (FlagTest(mask, mapping::file::write_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::write);
      prm |= mapping::posix[tr][op];
    }
    if (FlagTest(mask, mapping::file::exec_mask)) {
      op = static_cast<typename std::underlying_type<OperationType>::type>(
          OperationType::exec);
      prm |= mapping::posix[tr][op];
    }
  }
  return prm;
}

namespace {

auto ProcessPermissionsFailure(std::error_code m_ec, const path &p,
                               std::error_code *ec = nullptr) -> perms {
  if (m_ec) {
    if (IsNotFoundError(m_ec.value())) {
      return perms(perms::none);
    }
    if (m_ec.value() == ERROR_SHARING_VIOLATION) {
      return perms(perms::unknown);
    }
    if (ec != nullptr) {
      ErrorHandler<perms> err("permissions", ec, &p);
      err.report(m_ec,
                 "failed to determine permissions for the specified path");
    }
  }
  return perms(perms::none);
}

}  // namespace

auto GetSecurityDescriptor(const path &p, std::error_code &ec)
    -> PSECURITY_DESCRIPTOR * {
  ec.clear();

  DWORD dwSize = 0;
  DWORD dwBytesNeeded = 0;

  auto wpath = p.wstring();

  GetFileSecurityW(wpath.c_str(),
                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                       DACL_SECURITY_INFORMATION,
                   nullptr, NULL, &dwBytesNeeded);

  dwSize = dwBytesNeeded;
  auto *pDescriptor = static_cast<PSECURITY_DESCRIPTOR *>(
      LocalAlloc(LMEM_FIXED, dwBytesNeeded));
  if (pDescriptor == nullptr) {
    return nullptr;
  }
  if (GetFileSecurityW(wpath.c_str(),
                       OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                           DACL_SECURITY_INFORMATION,
                       pDescriptor, dwSize, &dwBytesNeeded) == 0) {
    ec = capture_errno();
    LocalFree(pDescriptor);
    return nullptr;
  }
  return pDescriptor;
}

auto GetDaclFromDescriptor(PSECURITY_DESCRIPTOR *pDescriptor,
                           std::error_code &ec) -> PACL {
  ec.clear();

  PACL DACL = nullptr;
  BOOL bDACLPresent = 0;
  BOOL bDACLDefaulted = 0;

  bDACLPresent = 0;
  bDACLDefaulted = 0;
  if (GetSecurityDescriptorDacl(pDescriptor, &bDACLPresent, &DACL,
                                &bDACLDefaulted) == 0) {
    ec = capture_errno();
    return nullptr;
  }

  return DACL;
}

auto GetOwnerPermissions(PSID pSidOwner, PACL DACL, DWORD attr,
                         std::error_code &ec) -> perms {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = nullptr;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_SID;
  trustee.TrusteeType = TRUSTEE_IS_USER;
  trustee.ptstrName = static_cast<LPCH>(pSidOwner);

  ACCESS_MASK mask = 0;
  auto retval = GetEffectiveRightsFromAcl(DACL, &trustee, &mask);
  if (retval != ERROR_SUCCESS) {
    // The GetEffectiveRightsFromAcl function fails and returns
    // ERROR_INVALID_ACL if the specified ACL contains an inherited
    // access-denied ACE. When this happens the last error code is
    // useless and we just need to report this as a permission denied error.
    if (retval == ERROR_INVALID_ACL) {
      ec = std::make_error_code(std::errc::permission_denied);
    } else {
      ec = capture_errno();
    }
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::owner,
      FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_DIRECTORY))
          ? file_type::directory
          : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_READONLY))) {
    FlagClear(prms, perms::owner_read);
  }

  return prms;
}

auto GetGroupPermissions(PSID pSidGroup, PACL DACL, DWORD attr,
                         std::error_code &ec) -> perms {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = nullptr;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_SID;
  trustee.TrusteeType = TRUSTEE_IS_GROUP;
  trustee.ptstrName = static_cast<LPCH>(pSidGroup);

  ACCESS_MASK mask = 0;
  auto retval = GetEffectiveRightsFromAcl(DACL, &trustee, &mask);
  if (retval != ERROR_SUCCESS) {
    // The GetEffectiveRightsFromAcl function fails and returns
    // ERROR_INVALID_ACL if the specified ACL contains an inherited
    // access-denied ACE. When this happens the last error code is
    // useless and we just need to report this as a permission denied error.
    if (retval == ERROR_INVALID_ACL) {
      ec = std::make_error_code(std::errc::permission_denied);
    } else {
      ec = capture_errno();
    }
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::group,
      FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_DIRECTORY))
          ? file_type::directory
          : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_READONLY))) {
    FlagClear(prms, perms::group_read);
  }

  return prms;
}

auto GetOthersPermissions(PACL DACL, DWORD attr, std::error_code &ec) -> perms {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = nullptr;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_NAME;
  trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
  trustee.ptstrName = "EVERYONE";

  ACCESS_MASK mask = 0;
  auto retval = GetEffectiveRightsFromAcl(DACL, &trustee, &mask);
  if (retval != ERROR_SUCCESS) {
    // The GetEffectiveRightsFromAcl function fails and returns
    // ERROR_INVALID_ACL if the specified ACL contains an inherited
    // access-denied ACE. When this happens the last error code is
    // useless and we just need to report this as a permission denied error.
    if (retval == ERROR_INVALID_ACL) {
      ec = std::make_error_code(std::errc::permission_denied);
    } else {
      ec = capture_errno();
    }
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::others,
      FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_DIRECTORY))
          ? file_type::directory
          : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, static_cast<DWORD>(FILE_ATTRIBUTE_READONLY))) {
    FlagClear(prms, perms::others_read);
  }

  return prms;
}

// TODO(Abdessattar): check what we need to do here for symlinks
auto GetPermissions(const path &p, DWORD attr, bool /*follow_symlinks*/,
                    std::error_code *ec) -> perms {
  ErrorHandler<perms> err("permissions", ec, &p);
  std::error_code m_ec;

  PSID pSidOwner = nullptr;
  PSID pSidGroup = nullptr;
  PACL DACL = nullptr;
  PSECURITY_DESCRIPTOR pSecurityDescriptor = nullptr;

  auto wpath = p.wstring();

  if (GetNamedSecurityInfoW(wpath.c_str(), SE_FILE_OBJECT,
                            OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                            &pSidOwner, &pSidGroup, &DACL, nullptr,
                            &pSecurityDescriptor) != ERROR_SUCCESS) {
    return err.report(capture_errno());
  }
  std::unique_ptr<VOID, decltype(&::LocalFree)> ptr_secDesc(pSecurityDescriptor,
                                                            ::LocalFree);

  //
  // Map permissions
  //

  perms prms = perms::none;

  // For the owner
  prms |= GetOwnerPermissions(pSidOwner, DACL, attr, m_ec);
  if (m_ec) {
    return ProcessPermissionsFailure(m_ec, p, ec);
  }

  // For the primary group
  if (EqualSid(pSidOwner, pSidGroup) == 0) {
    prms |= GetGroupPermissions(pSidGroup, DACL, attr, m_ec);
    if (m_ec) {
      return ProcessPermissionsFailure(m_ec, p, ec);
    }
  }

  // For others
  prms |= GetOthersPermissions(DACL, attr, m_ec);
  if (m_ec) {
    return ProcessPermissionsFailure(m_ec, p, ec);
  }

  return prms;
}

auto MakeAce(TrusteeType trustee, PSID pSid, DWORD access, ACCESS_MODE mode)
    -> PEXPLICIT_ACCESS {
  auto *pEA = static_cast<PEXPLICIT_ACCESS>(
      LocalAlloc(LMEM_FIXED, sizeof(EXPLICIT_ACCESS)));
  if (pEA != nullptr) {
    ZeroMemory(pEA, sizeof(EXPLICIT_ACCESS));

    pEA->grfAccessPermissions = access;
    pEA->grfAccessMode = mode;
    pEA->grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    switch (trustee) {
      case TrusteeType::system:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        pEA->Trustee.ptstrName = "System";
        break;
      case TrusteeType::admins:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        pEA->Trustee.ptstrName = "Administrators";
        break;
      case TrusteeType::others:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        pEA->Trustee.ptstrName = "Everyone";
        break;
      case TrusteeType::owner:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_SID;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_USER;
        pEA->Trustee.ptstrName = static_cast<LPCH>(pSid);
        break;
      case TrusteeType::group:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_SID;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        pEA->Trustee.ptstrName = static_cast<LPCH>(pSid);
        break;
    }
  }
  return pEA;
}

// TODO(Abdessattar): check what we need to do here for symlinks
void SetPermissions(const path &p, perms prms, bool /*follow_symlinks*/,
                    std::error_code *ec) {
  ErrorHandler<void> err("permissions", ec, &p);

  DWORD errVal = 0;
  PSID pSidOwner = nullptr;
  PSID pSidGroup = nullptr;
  PSECURITY_DESCRIPTOR pSecurityDescriptor = nullptr;
  auto wpath = p.wstring();

  errVal = GetNamedSecurityInfoW(
      wpath.c_str(), SE_FILE_OBJECT,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION, &pSidOwner,
      &pSidGroup, nullptr, nullptr, &pSecurityDescriptor);
  if (errVal != ERROR_SUCCESS) {
    return err.report({static_cast<int>(errVal), std::system_category()});
  }
  std::unique_ptr<VOID, decltype(&::LocalFree)> ptr_secDesc(pSecurityDescriptor,
                                                            ::LocalFree);

  using ea_unique_ptr =
      std::unique_ptr<EXPLICIT_ACCESS, decltype(&::LocalFree)>;
  std::deque<ea_unique_ptr> aces;
  PEXPLICIT_ACCESS pEA = nullptr;
  ea_unique_ptr ptr_EA(pEA, ::LocalFree);

  //
  // Everyone
  //

  DWORD access = 0L;
  if (FlagTest(prms, perms::others_read)) {
    FlagSet(access, GENERIC_READ);
  }
  if (FlagTest(prms, perms::others_write)) {
    FlagSet(access,
            static_cast<DWORD>(GENERIC_WRITE | FILE_DELETE_CHILD | DELETE));
  }
  if (FlagTest(prms, perms::others_exec)) {
    FlagSet(access, static_cast<DWORD>(GENERIC_EXECUTE));
  }

  if (access != 0) {
    pEA = MakeAce(TrusteeType::others, nullptr, access, GRANT_ACCESS);
    if (pEA == nullptr) {
      return err.report(capture_errno());
    }

    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  //
  // Group
  // Only if the group SID is not the same than the owner SID
  //

  // Always check the mmore generalized scope (Everyone) for allow/deny on
  // same access rights specified for group to avoid situation where an access
  // right is not granted with the group ace but is unfortunately granted by
  // the Everyone ace.
  // To avoid this, we will explicitly deny anything that is not allowed for
  // group but allowed for Everyone.

  if (EqualSid(pSidOwner, pSidGroup) == 0) {
    // Allow
    access = 0;
    if (FlagTest(prms, perms::group_read) &&
        !FlagTest(prms, perms::others_read)) {
      FlagSet(access, GENERIC_READ);
    }
    if (FlagTest(prms, perms::group_write) &&
        !FlagTest(prms, perms::others_write)) {
      FlagSet(access,
              static_cast<DWORD>(GENERIC_WRITE | FILE_DELETE_CHILD | DELETE));
    }
    if (FlagTest(prms, perms::group_exec) &&
        !FlagTest(prms, perms::others_exec)) {
      FlagSet(access, static_cast<DWORD>(GENERIC_EXECUTE));
    }

    if (access != 0) {
      pEA = MakeAce(TrusteeType::group, pSidGroup, access, GRANT_ACCESS);
      if (pEA == nullptr) {
        return err.report(capture_errno());
      }

      ptr_EA.reset(pEA);
      aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
    }

    // Deny
    access = 0;
    if (!FlagTest(prms, perms::group_read) &&
        FlagTest(prms, perms::others_read)) {
      FlagSet(access, GENERIC_READ);
    }
    if (!FlagTest(prms, perms::group_write) &&
        FlagTest(prms, perms::others_write)) {
      FlagSet(access,
              static_cast<DWORD>(GENERIC_WRITE | FILE_DELETE_CHILD | DELETE));
    }
    if (!FlagTest(prms, perms::group_exec) &&
        FlagTest(prms, perms::others_exec)) {
      FlagSet(access, static_cast<DWORD>(GENERIC_EXECUTE));
    }

    if (access != 0) {
      pEA = MakeAce(TrusteeType::group, pSidGroup, access, DENY_ACCESS);
      if (pEA == nullptr) {
        return err.report(capture_errno());
      }

      ptr_EA.reset(pEA);
      aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
    }
  }

  // Owner

  // Allow
  access = 0;
  if (FlagTest(prms, perms::owner_read) &&
      !(FlagTest(prms, perms::group_read) ||
        FlagTest(prms, perms::others_read))) {
    FlagSet(access, GENERIC_READ);
  }
  if (FlagTest(prms, perms::owner_write) &&
      !(FlagTest(prms, perms::group_write) ||
        FlagTest(prms, perms::others_write))) {
    FlagSet(access,
            static_cast<DWORD>(GENERIC_WRITE | FILE_DELETE_CHILD | DELETE));
  }
  if (FlagTest(prms, perms::owner_exec) &&
      !(FlagTest(prms, perms::group_exec) ||
        FlagTest(prms, perms::others_exec))) {
    FlagSet(access, static_cast<DWORD>(GENERIC_EXECUTE));
  }

  if (access != 0) {
    pEA = MakeAce(TrusteeType::owner, pSidOwner, access, GRANT_ACCESS);
    if (pEA == nullptr) {
      return err.report(capture_errno());
    }

    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  // Deny
  access = 0;
  if (!FlagTest(prms, perms::owner_read) &&
      (FlagTest(prms, perms::group_read) ||
       FlagTest(prms, perms::others_read))) {
    FlagSet(access, GENERIC_READ);
  }
  if (!FlagTest(prms, perms::owner_write) &&
      (FlagTest(prms, perms::group_write) ||
       FlagTest(prms, perms::others_write))) {
    FlagSet(access,
            static_cast<DWORD>(GENERIC_WRITE | FILE_DELETE_CHILD | DELETE));
  }
  if (!FlagTest(prms, perms::owner_exec) &&
      (FlagTest(prms, perms::group_exec) ||
       FlagTest(prms, perms::others_exec))) {
    FlagSet(access, static_cast<DWORD>(GENERIC_EXECUTE));
  }

  if (access != 0) {
    pEA = MakeAce(TrusteeType::group, pSidOwner, access, DENY_ACCESS);
    if (pEA == nullptr) {
      return err.report(capture_errno());
    }

    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  if (aces.empty()) {
    // Add the special ACEs for Everyone to deny everything
    pEA = MakeAce(TrusteeType::others, nullptr, GENERIC_ALL, DENY_ACCESS);
    if (pEA == nullptr) {
      return err.report(capture_errno());
    }
    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  // Create a new ACL that contains the new ACEs.
  auto ordered_aces = std::vector<EXPLICIT_ACCESS>();
  for (auto &i : aces) {
    ordered_aces.push_back(*(i));
  }

  PACL pACL = nullptr;
  auto acl_size = ordered_aces.size();
  errVal = SetEntriesInAcl(static_cast<ULONG>(acl_size),
                           (acl_size > 0) ? &ordered_aces[0] : nullptr, nullptr,
                           &pACL);
  if (errVal != ERROR_SUCCESS) {
    return err.report({static_cast<int>(errVal), std::system_category()});
  }
  std::unique_ptr<ACL, decltype(&::LocalFree)> ptr_ACL(pACL, ::LocalFree);

  errVal = SetNamedSecurityInfoW(
      const_cast<LPWSTR>(wpath.c_str()), SE_FILE_OBJECT,
      DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr,
      nullptr, pACL, nullptr);
  if (errVal != ERROR_SUCCESS) {
    return err.report({static_cast<int>(errVal), std::system_category()});
  }
}

}  // namespace win32_port
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
