#        Copyright The Authors 2018.
#    Distributed under the 3-Clause BSD License.
#    (See accompanying file LICENSE or copy at
#   https://opensource.org/licenses/BSD-3-Clause)
 

# Meta information about the project
set(META_MODULE_NAME                "filesystem")
set(META_MODULE_DESCRIPTION         "A module to provide the functionality of std::filesystem from C++ 17")
set(META_MODULE_GITHUB_REPO         "asap_filesystem")
set(META_MODULE_VERSION_MAJOR       "0")
set(META_MODULE_VERSION_MINOR       "1")
set(META_MODULE_VERSION_PATCH       "1")
set(META_MODULE_VERSION_REVISION    "${MODULE_GIT_REV}")
set(META_MODULE_VERSION             "${META_MODULE_VERSION_MAJOR}.${META_MODULE_VERSION_MINOR}.${META_MODULE_VERSION_PATCH}")
set(META_MODULE_NAME_VERSION        "${META_MODULE_PROJECT_NAME} v${META_MODULE_VERSION}")
message("=> [module: filesystem ${META_MODULE_VERSION}]")

# ------------------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------------------

set(IDE_FOLDER "Libraries")

# Project options

# This module's specific Environment detection
include(CheckIncludeFileCXX)
check_include_file_cxx("copyfile.h" ASAP_HAVE_COPYFILE_H)
check_include_file_cxx("utime.h" ASAP_HAVE_UTIME_H)


# ------------------------------------------------------------------------------
# External dependencies
# ------------------------------------------------------------------------------

# find_package(THIRDPARTY REQUIRED)


# ==============================================================================
# Build instructions
# ==============================================================================

# Target name
set(target asap_filesystem)
set(target_export filesystem)

# Exit here if required dependencies are not met


# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include/filesystem")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(public_headers
  "include/filesystem/filesystem.h"
  "include/filesystem/fs_path_traits.h"
  "include/filesystem/fs_path.h"
  "include/filesystem/filesystem_error.h"
  "include/filesystem/fs_file_type.h"
  "include/filesystem/fs_copy_options.h"
  "include/filesystem/fs_perms.h"
  "include/filesystem/fs_directory_options.h"
  "include/filesystem/fs_fwd.h"
  "include/filesystem/fs_dir.h"
  "include/filesystem/fs_ops.h"
  )
if (WIN32)
  set(platform_specific_sources
    "src/windows/file.cpp"
    "src/windows/time.cpp"
    "src/windows/stat.cpp"
    "src/windows/permissions.cpp"
    )
else ()
  set(platform_specific_sources
    "src/posix/file.cpp"
    "src/posix/time.cpp"
    "src/posix/stat.cpp"
    )
endif ()

set(sources
  "src/fs_path.cpp"
  "src/fs_dir_iterator.cpp"
  "src/fs_ops.cpp"
  "src/filesystem_error.cpp"
  ${platform_specific_sources}
  "src/fs_error.h"
  "src/fs_portability.h"
  ${public_headers}
  )

# Group source files
set(header_group "Header Files (API)")
set(source_group "Source Files")
source_group_by_path(${include_path} "\\\\.h$|\\\\.hpp$"
  ${header_group} ${public_headers})
source_group_by_path(${source_path} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$"
  ${source_group} ${sources})


# ------------------------------------------------------------------------------
# Include dirs
# ------------------------------------------------------------------------------

set(public_include_dirs)
set(private_include_dirs)

# ------------------------------------------------------------------------------
# Compile definitions / options
# ------------------------------------------------------------------------------

set(private_compile_definitions)
set(private_compile_options)

# ------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------

set(public_libraries ${META_PROJECT_NAME}::common)

# ------------------------------------------------------------------------------
# Create targets
# ------------------------------------------------------------------------------

asap_library(
  TARGET
  ${target}
  EXPORT_NAME
  ${target_export}
  SOURCES
  ${sources}
  PUBLIC_INCLUDE_DIRS
  ${public_include_dirs}
  PRIVATE_INCLUDE_DIRS
  ${private_include_dirs}
  PUBLIC_LIBRARIES
  ${public_libraries}
  PRIVATE_COMPILE_DEFINITIONS
  ${private_compile_definitions}
  PRIVATE_COMPILE_OPTIONS
  ${private_compile_options}
)


# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if (OPTION_BUILD_TESTS)
  add_subdirectory(test)
endif()

# ------------------------------------------------------------------------------
# Code generation
# ------------------------------------------------------------------------------

# Generate config-header
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/filesystem/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/filesystem/config.h
)


# ------------------------------------------------------------------------------
# Code analysis
# ------------------------------------------------------------------------------

enable_cppcheck()
tidy_target(${target})


# ------------------------------------------------------------------------------
# API Documentation
# ------------------------------------------------------------------------------

configure_doxyfile(${target}
  "\"C++17 filesystem backport Module\""
  "\"A module to provide the functionality of std::filesystem from C++ 17\""
  "${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include")

add_doxygen_target(${target})

add_sphinx_target(${target})

install(
  DIRECTORY ${DOXYGEN_BUILD_DIR}/${target}
  DESTINATION ${INSTALL_DOC}
  COMPONENT docs
)
