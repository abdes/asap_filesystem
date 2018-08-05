#        Copyright The Authors 2018.
#    Distributed under the 3-Clause BSD License.
#    (See accompanying file LICENSE or copy at
#   https://opensource.org/licenses/BSD-3-Clause)

# ------------------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------------------

set(IDE_FOLDER "Tests")

# ------------------------------------------------------------------------------
# External dependencies
# ------------------------------------------------------------------------------

# find_package(THIRDPARTY REQUIRED)


# ==============================================================================
# Build instructions
# ==============================================================================

# Target name
set(target asap_filesystem_test)

# Exit here if required dependencies are not met
message(STATUS "Test Program ${target}")


# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(public_headers
  "fs_testsuite.h"
  )

set(sources
  "path_traits_test.cpp"
  "path_append_test.cpp"
  "path_assign_test.cpp"
  "path_compare_test.cpp"
  "path_concat_test.cpp"
  "path_construct_test.cpp"
  "path_decompose_test.cpp"
  "path_generation_test.cpp"
  "path_generic_test.cpp"
  "path_iostream_test.cpp"
  "path_iterator_test.cpp"
  "path_modifiers_test.cpp"
  "path_native_test.cpp"
  "path_nonmembers_test.cpp"
  "path_query_test.cpp"
  # operations
  "file_status_test.cpp"
  "ops_absolute_test.cpp"
  "ops_canonical_test.cpp"
  "ops_copy_test.cpp"
  "ops_copy_file_test.cpp"
  "ops_create_dir_test.cpp"
  "ops_create_symlink_test.cpp"
  "ops_current_path_test.cpp"
  "ops_equivalent_test.cpp"
  "ops_exists_test.cpp"
  "ops_file_size_test.cpp"
  "ops_is_empty_test.cpp"
  "ops_last_write_time_test.cpp"
  "ops_permissions_test.cpp"
  "ops_proximate_test.cpp"
  "ops_read_symlink_test.cpp"
  "ops_relative_test.cpp"
  "ops_remove_test.cpp"
  "ops_space_test.cpp"
  "ops_status_test.cpp"
  "ops_symlink_status_test.cpp"
  "ops_temp_dir_test.cpp"
  "ops_weakly_canonical_test.cpp"
  # directory iterators
  "dir_iterator_test.cpp"
  "dir_reverse_iterator_test.cpp"
  "dir_pop_test.cpp"
  "main.cpp"
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

set(include_dirs)

# ------------------------------------------------------------------------------
# Compile definitions / options
# ------------------------------------------------------------------------------

set(compile_definitions)
set(compile_options)

# ------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------

set(libraries ${META_PROJECT_NAME}::filesystem)

# ------------------------------------------------------------------------------
# Create targets
# ------------------------------------------------------------------------------

asap_test_executable(
  TARGET
  ${target}
  SOURCES
  ${sources}
  INCLUDE_DIRS
  ${include_dirs}
  LIBRARIES
  ${libraries}
  COMPILE_DEFINITIONS
  ${compile_definitions}
  COMPILE_OPTIONS
  ${compile_options}
)
