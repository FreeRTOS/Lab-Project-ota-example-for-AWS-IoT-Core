cmake_minimum_required(VERSION 3.16.0)
project(
  "AWS IoT Jobs OTA Handler Unit Tests"
  VERSION 1.0.0
  LANGUAGES C)
# A hacky copy of
# https://github.com/FreeRTOS/coreMQTT/blob/main/test/unit-test/CMakeLists.txt

# Include filepaths for source and include.
include( ${MODULE_ROOT_DIR}/otaJobsFilePaths.cmake )
include("${corejson_SOURCE_DIR}/jsonFilePaths.cmake")

# ====================  Define your project name (edit) ========================
set(project_name "coreota_jobs_handler")

# =====================  Create your mock here  (edit)  ========================

# list the files to mock here
list(APPEND mock_list "${MODULE_ROOT_DIR}/source/include/job_parser.h")

# list the directories your mocks need
list(APPEND mock_include_list "${MODULE_ROOT_DIR}/source/include")
# list the definitions of your mocks to control what to be included
list(APPEND mock_define_list "")

# ================= Create the library under test here (edit) ==================

# list the files you would like to test here
list(APPEND real_source_files ${JOBS_HANDLER_SOURCES} ${JSON_SOURCES})
# list the directories the module under test includes
list(APPEND real_include_directories . ${JOBS_HANDLER_INCLUDES}
     ${JSON_INCLUDE_PUBLIC_DIRS})

# =====================  Create UnitTest Code here (edit)  =====================

# list the directories your test needs to include
list(APPEND test_include_directories . ${JOBS_HANDLER_INCLUDES})

# =============================  (end edit)  ===================================

# Create ota_job_handler unit test
set(real_name "ota_job_handler_real")
set(utest_name "ota_job_handler_utest")
set(utest_source "ota_job_handler_utest.c")
set(mock_name "${project_name}_mock")
set(real_name "${project_name}_real")

create_mock_list(
  ${mock_name} "${mock_list}" "${MODULE_ROOT_DIR}/tools/cmock/project.yml"
  "${mock_include_list}" "${mock_define_list}")

create_real_library(${real_name} "${real_source_files}"
                    "${real_include_directories}" "${mock_name}")

list(APPEND utest_link_list -l${mock_name} lib${real_name}.a)

list(APPEND utest_dep_list ${real_name})

create_test(${utest_name} ${utest_source} "${utest_link_list}"
            "${utest_dep_list}" "${test_include_directories}")

# Create job parser unit test
set(real_name "job_parser_real")
set(utest_name "job_parser_utest")
set(utest_source "job_parser_utest.c")
# No need to redefine mocks since we do not need any

set(utest_link_list "")
list(APPEND utest_dep_list ${real_name})

create_real_library(${real_name} "${real_source_files}"
                    "${real_include_directories}" "")

# Redefine the linked files to ignore the mock files
list(APPEND utest_link_list lib${real_name}.a)

create_test(${utest_name} ${utest_source} "${utest_link_list}"
            "${utest_dep_list}" "${test_include_directories}")
