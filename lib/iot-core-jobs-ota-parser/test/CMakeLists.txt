cmake_minimum_required(VERSION 3.16.0)

project(
  "AWS IoT Jobs OTA Handler Tests"
  VERSION 1.0.0
  LANGUAGES C)

# Allow the project to be organized into folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure options to always show in CMake GUI.
option(
  BUILD_CLONE_SUBMODULES
  "Set this to ON to automatically clone any required Git submodules. When OFF, submodules must be manually cloned."
  OFF)

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT DEFINED CMAKE_C_STANDARD_REQUIRED)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# Do not allow in-source build.
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
  message(
    FATAL_ERROR
      "In-source build is not allowed. Please build in a separate directory, such as ${PROJECT_SOURCE_DIR}/build."
  )
endif()

# Set global path variables.
get_filename_component(__MODULE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.."
                       ABSOLUTE)
set(MODULE_ROOT_DIR
    ${__MODULE_ROOT_DIR}
    CACHE INTERNAL "coreOTA-Jobs-Handler repository root.")

# ====================================  Test Configuration
# ========================================

# Define a CMock resource path.
set(CMOCK_DIR
    ${MODULE_ROOT_DIR}/test/lib/CMock
    CACHE INTERNAL "CMock library source directory.")

# Include CMock build configuration.
include(lib/cmock_build.cmake)

# Check if the CMock source directory exists, and if not present, clone the
# submodule if BUILD_CLONE_SUBMODULES configuration is enabled.
if(NOT EXISTS ${CMOCK_DIR}/src)
  # Attempt to clone CMock.
  if(${BUILD_CLONE_SUBMODULES})
    clone_cmock()
  else()
    message(
      FATAL_ERROR
        "The required submodule CMock does not exist. Either clone it manually, or set BUILD_CLONE_SUBMODULES to 1 to automatically clone it during build."
    )
  endif()
endif()

# Add unit test and coverage configuration.

# Use CTest utility for managing test runs. This has to be added BEFORE defining
# test targets with add_test()
enable_testing()

# Add build targets for CMock and Unit, required for unit testing.
add_cmock_targets()

# Add function to enable CMock based tests and coverage.
include(${MODULE_ROOT_DIR}/tools/cmock/create_test.cmake)

# Include build configuration for unit tests.
add_subdirectory(unit-test)

# ==================================== Coverage Analysis configuration
# ========================================

# Add a target for running coverage on tests.
add_custom_target(
  coverage
  COMMAND ${CMAKE_COMMAND} -DCMOCK_DIR=${CMOCK_DIR} -P
          ${MODULE_ROOT_DIR}/tools/cmock/coverage.cmake
  DEPENDS cmock unity
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
