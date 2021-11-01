//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

#include <deque>
#include <vector>

#include <common/flag_ops.h>

#include "../fs_portability.h"

#if defined(ASAP_WINDOWS)

// -----------------------------------------------------------------------------
//                            detail: permissions
// -----------------------------------------------------------------------------

namespace asap {
namespace filesystem {
namespace detail {
namespace win32 {

enum class TrusteeType { owner = 0, group, others, system, admins };
enum class OperationType { read = 0, write, exec };

namespace mapping {
namespace {
perms posix[3][3] = {
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
perms ASAP_FILESYSTEM_API MapAccessMaskToPerms(ACCESS_MASK mask,
                                               TrusteeType trustee,
                                               file_type type) {
  if (type == file_type::none || type == file_type::unknown ||
      type == file_type::not_found) {
    return perms::unknown;
  }

  if (mask == 0) return perms::none;

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

perms ProcessPermissionsFailure(std::error_code m_ec, const path &p,
                                std::error_code *ec = nullptr) {
  if (m_ec) {
    if (IsNotFoundError(m_ec.value())) {
      return perms(perms::none);
    } else if (m_ec.value() == ERROR_SHARING_VIOLATION) {
      return perms(perms::unknown);
    } else {
      if (ec) {
        ErrorHandler<perms> err("permissions", ec, &p);
        err.report(m_ec,
                   "failed to determine permissions for the specified path");
      }
    }
  }
  return perms(perms::none);
}

}  // namespace

PSECURITY_DESCRIPTOR *GetSecurityDescriptor(const path &p,
                                            std::error_code &ec) {
  ec.clear();

  DWORD dwSize = 0;
  DWORD dwBytesNeeded = 0;

  auto wpath = p.wstring();

  GetFileSecurityW(wpath.c_str(),
                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                       DACL_SECURITY_INFORMATION,
                   NULL, NULL, &dwBytesNeeded);

  dwSize = dwBytesNeeded;
  PSECURITY_DESCRIPTOR *pDescriptor =
      (PSECURITY_DESCRIPTOR *)LocalAlloc(LMEM_FIXED, dwBytesNeeded);
  if (pDescriptor == NULL) return NULL;
  if (!GetFileSecurityW(wpath.c_str(),
                        OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                        pDescriptor, dwSize, &dwBytesNeeded)) {
    ec = capture_errno();
    LocalFree(pDescriptor);
    return NULL;
  }
  return pDescriptor;
}

PACL GetDaclFromDescriptor(PSECURITY_DESCRIPTOR *pDescriptor,
                           std::error_code &ec) {
  ec.clear();

  PACL DACL = NULL;
  BOOL bDACLPresent = false;
  BOOL bDACLDefaulted = false;

  bDACLPresent = false;
  bDACLDefaulted = false;
  if (GetSecurityDescriptorDacl(pDescriptor, &bDACLPresent, &DACL,
                                &bDACLDefaulted) == false) {
    ec = capture_errno();
    return NULL;
  }

  return DACL;
}

perms GetOwnerPermissions(PSID pSidOwner, PACL DACL, DWORD attr,
                          std::error_code &ec) {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = NULL;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_SID;
  trustee.TrusteeType = TRUSTEE_IS_USER;
  trustee.ptstrName = (LPCH)(pSidOwner);

  ACCESS_MASK mask = 0;
  if (GetEffectiveRightsFromAcl(DACL, &trustee, &mask) != ERROR_SUCCESS) {
    ec = capture_errno();
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::owner,
      FlagTest(attr, (DWORD)FILE_ATTRIBUTE_DIRECTORY) ? file_type::directory
                                                      : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, (DWORD)FILE_ATTRIBUTE_READONLY)) {
    FlagClear(prms, perms::owner_read);
  }

  return prms;
}

perms GetGroupPermissions(PSID pSidGroup, PACL DACL, DWORD attr,
                          std::error_code &ec) {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = NULL;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_SID;
  trustee.TrusteeType = TRUSTEE_IS_GROUP;
  trustee.ptstrName = (LPCH)(pSidGroup);

  ACCESS_MASK mask = 0;
  if (GetEffectiveRightsFromAcl(DACL, &trustee, &mask) != ERROR_SUCCESS) {
    ec = capture_errno();
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::group,
      FlagTest(attr, (DWORD)FILE_ATTRIBUTE_DIRECTORY) ? file_type::directory
                                                      : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, (DWORD)FILE_ATTRIBUTE_READONLY)) {
    FlagClear(prms, perms::group_read);
  }

  return prms;
}

perms GetOthersPermissions(PACL DACL, DWORD attr, std::error_code &ec) {
  ec.clear();

  TRUSTEE_A trustee;
  trustee.pMultipleTrustee = NULL;
  trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  trustee.TrusteeForm = TRUSTEE_IS_NAME;
  trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
  trustee.ptstrName = "EVERYONE";

  ACCESS_MASK mask = 0;
  if (GetEffectiveRightsFromAcl(DACL, &trustee, &mask) != ERROR_SUCCESS) {
    ec = capture_errno();
    return perms::none;
  }

  perms prms = MapAccessMaskToPerms(
      mask, TrusteeType::others,
      FlagTest(attr, (DWORD)FILE_ATTRIBUTE_DIRECTORY) ? file_type::directory
                                                      : file_type::regular);

  // Test for read-only files and eventually remove read rights
  if (FlagTest(attr, (DWORD)FILE_ATTRIBUTE_READONLY)) {
    FlagClear(prms, perms::others_read);
  }

  return prms;
}

perms GetPermissions(const path &p, DWORD attr, bool follow_symlinks,
                     std::error_code *ec) {
  ErrorHandler<perms> err("permissions", ec, &p);
  std::error_code m_ec;

  PSID pSidOwner = NULL;
  PSID pSidGroup = NULL;
  PACL DACL = NULL;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;

  auto wpath = p.wstring();

  if (GetNamedSecurityInfoW(wpath.c_str(), SE_FILE_OBJECT,
                            OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                            &pSidOwner, &pSidGroup, &DACL, NULL,
                            &pSecurityDescriptor) != ERROR_SUCCESS) {
    // TODO: DEBUG CODE
    std::cout << "GetPermissions (" << wpath
              << ") failed, error code : " << ::GetLastError() << std::endl;
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
  if (m_ec) return ProcessPermissionsFailure(m_ec, p, ec);

  // For the primary group
  if (!EqualSid(pSidOwner, pSidGroup)) {
    prms |= GetGroupPermissions(pSidGroup, DACL, attr, m_ec);
    if (m_ec) return ProcessPermissionsFailure(m_ec, p, ec);
  }

  // For others
  prms |= GetOthersPermissions(DACL, attr, m_ec);
  if (m_ec) return ProcessPermissionsFailure(m_ec, p, ec);

  return prms;
}

PEXPLICIT_ACCESS MakeAce(TrusteeType trustee, PSID pSid, DWORD access,
                         ACCESS_MODE mode) {
  PEXPLICIT_ACCESS pEA =
      (PEXPLICIT_ACCESS)LocalAlloc(LMEM_FIXED, sizeof(EXPLICIT_ACCESS));
  if (pEA != NULL) {
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
        pEA->Trustee.ptstrName = (LPCH)pSid;
        break;
      case TrusteeType::group:
        pEA->Trustee.TrusteeForm = TRUSTEE_IS_SID;
        pEA->Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        pEA->Trustee.ptstrName = (LPCH)pSid;
        break;
    }
  }
  return pEA;
}

void SetPermissions(const path &p, perms prms, bool follow_symlinks,
                    std::error_code *ec) {
  ErrorHandler<void> err("permissions", ec, &p);

  DWORD errVal = 0;
  PSID pSidOwner;
  PSID pSidGroup;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  auto wpath = p.wstring();

  errVal = GetNamedSecurityInfoW(
      wpath.c_str(), SE_FILE_OBJECT,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION, &pSidOwner,
      &pSidGroup, NULL, NULL, &pSecurityDescriptor);
  if (errVal != ERROR_SUCCESS) {
    return err.report({(int)errVal, std::system_category()});
  }
  std::unique_ptr<VOID, decltype(&::LocalFree)> ptr_secDesc(pSecurityDescriptor,
                                                            ::LocalFree);

  using ea_unique_ptr =
      std::unique_ptr<EXPLICIT_ACCESS, decltype(&::LocalFree)>;
  std::deque<ea_unique_ptr> aces;
  PEXPLICIT_ACCESS pEA = NULL;
  ea_unique_ptr ptr_EA(pEA, ::LocalFree);

  //
  // Everyone
  //

  DWORD access = 0L;
  if (FlagTest(prms, perms::others_read)) FlagSet(access, (DWORD)GENERIC_READ);
  if (FlagTest(prms, perms::others_write))
    FlagSet(access, (DWORD)GENERIC_WRITE | FILE_DELETE_CHILD | DELETE);
  if (FlagTest(prms, perms::others_exec))
    FlagSet(access, (DWORD)GENERIC_EXECUTE);

  if (access != 0) {
    pEA = MakeAce(TrusteeType::others, NULL, access, GRANT_ACCESS);
    if (pEA == NULL) return err.report(capture_errno());

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

  if (!EqualSid(pSidOwner, pSidGroup)) {
    // Allow
    access = 0;
    if (FlagTest(prms, perms::group_read) &&
        !FlagTest(prms, perms::others_read))
      FlagSet(access, (DWORD)GENERIC_READ);
    if (FlagTest(prms, perms::group_write) &&
        !FlagTest(prms, perms::others_write))
      FlagSet(access, (DWORD)GENERIC_WRITE | FILE_DELETE_CHILD | DELETE);
    if (FlagTest(prms, perms::group_exec) &&
        !FlagTest(prms, perms::others_exec))
      FlagSet(access, (DWORD)GENERIC_EXECUTE);

    if (access != 0) {
      pEA = MakeAce(TrusteeType::group, pSidGroup, access, GRANT_ACCESS);
      if (pEA == NULL) return err.report(capture_errno());

      ptr_EA.reset(pEA);
      aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
    }

    // Deny
    access = 0;
    if (!FlagTest(prms, perms::group_read) &&
        FlagTest(prms, perms::others_read))
      FlagSet(access, (DWORD)GENERIC_READ);
    if (!FlagTest(prms, perms::group_write) &&
        FlagTest(prms, perms::others_write))
      FlagSet(access, (DWORD)GENERIC_WRITE | FILE_DELETE_CHILD | DELETE);
    if (!FlagTest(prms, perms::group_exec) &&
        FlagTest(prms, perms::others_exec))
      FlagSet(access, (DWORD)GENERIC_EXECUTE);

    if (access != 0) {
      pEA = MakeAce(TrusteeType::group, pSidGroup, access, DENY_ACCESS);
      if (pEA == NULL) return err.report(capture_errno());

      ptr_EA.reset(pEA);
      aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
    }
  }

  // Owner

  // Allow
  access = 0;
  if (FlagTest(prms, perms::owner_read) &&
      !(FlagTest(prms, perms::group_read) ||
        FlagTest(prms, perms::others_read)))
    FlagSet(access, (DWORD)GENERIC_READ);
  if (FlagTest(prms, perms::owner_write) &&
      !(FlagTest(prms, perms::group_write) ||
        FlagTest(prms, perms::others_write)))
    FlagSet(access, (DWORD)GENERIC_WRITE | FILE_DELETE_CHILD | DELETE);
  if (FlagTest(prms, perms::owner_exec) &&
      !(FlagTest(prms, perms::group_exec) ||
        FlagTest(prms, perms::others_exec)))
    FlagSet(access, (DWORD)GENERIC_EXECUTE);

  if (access != 0) {
    pEA = MakeAce(TrusteeType::owner, pSidOwner, access, GRANT_ACCESS);
    if (pEA == NULL) return err.report(capture_errno());

    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  // Deny
  access = 0;
  if (!FlagTest(prms, perms::owner_read) &&
      (FlagTest(prms, perms::group_read) || FlagTest(prms, perms::others_read)))
    FlagSet(access, (DWORD)GENERIC_READ);
  if (!FlagTest(prms, perms::owner_write) &&
      (FlagTest(prms, perms::group_write) ||
       FlagTest(prms, perms::others_write)))
    FlagSet(access, (DWORD)GENERIC_WRITE | FILE_DELETE_CHILD | DELETE);
  if (!FlagTest(prms, perms::owner_exec) &&
      (FlagTest(prms, perms::group_exec) || FlagTest(prms, perms::others_exec)))
    FlagSet(access, (DWORD)GENERIC_EXECUTE);

  if (access != 0) {
    pEA = MakeAce(TrusteeType::group, pSidOwner, access, DENY_ACCESS);
    if (pEA == NULL) return err.report(capture_errno());

    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  if (aces.empty()) {
    // Add the special ACEs for Everyone to deny everything
    pEA = MakeAce(TrusteeType::others, NULL, GENERIC_ALL, DENY_ACCESS);
    if (pEA == NULL) return err.report(capture_errno());
    ptr_EA.reset(pEA);
    aces.emplace_front(std::forward<ea_unique_ptr>(ptr_EA));
  }

  // Create a new ACL that contains the new ACEs.
  auto ordered_aces = std::vector<EXPLICIT_ACCESS>();
  for (auto &i : aces) {
    ordered_aces.push_back(*(i.get()));
  }

  PACL pACL = NULL;
  auto acl_size = ordered_aces.size();
  errVal = SetEntriesInAcl(acl_size, (acl_size > 0) ? &ordered_aces[0] : NULL,
                           NULL, &pACL);
  if (errVal != ERROR_SUCCESS) {
    return err.report({(int)errVal, std::system_category()});
  }
  std::unique_ptr<ACL, decltype(&::LocalFree)> ptr_ACL(pACL, ::LocalFree);

  errVal = SetNamedSecurityInfoW(
      (LPWSTR)(wpath.c_str()), SE_FILE_OBJECT,
      DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL,
      NULL, pACL, NULL);
  if (errVal != ERROR_SUCCESS) {
    return err.report({(int)errVal, std::system_category()});
  }
}

}  // namespace win32
}  // namespace detail
}  // namespace filesystem
}  // namespace asap

#endif  // ASAP_WINDOWS
