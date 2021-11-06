# ~~~
#        Copyright The Authors 2018.
#    Distributed under the 3-Clause BSD License.
#    (See accompanying file LICENSE or copy at
#   https://opensource.org/licenses/BSD-3-Clause)
# ~~~

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

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")

set(public_headers "fs_testsuite.h")

if(WIN32)
  set(platform_specific_test_sources "windows/win_permissions_test.cpp")
else()
  set(platform_specific_test_sources)
endif()

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
    "ops_copy_symlink_test.cpp"
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
    "dir_recursive_iterator.cpp"
    "dir_pop_test.cpp"
    # platformspecific
    ${platform_specific_test_sources}
    "main.cpp"
    ${public_headers})

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
  ${compile_options})

set_target_properties(${target} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# ------------------------------------------------------------------------------
# Add support for (optional) code quality tools
# ------------------------------------------------------------------------------

asap_configure_sanitizers(${target})
