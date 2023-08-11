cmake_minimum_required(VERSION 3.16.0)
project(
  "Core Jobs Unit Tests"
  VERSION 1.0.0
  LANGUAGES C)

# Include filepaths for source and include.
include(${MODULE_ROOT_DIR}/coreJobsFilePaths.cmake)
include("${corejson_SOURCE_DIR}/jsonFilePaths.cmake")
include("${coremqtt_SOURCE_DIR}/mqttFilePaths.cmake")

# ====================  Define your project name (edit) ========================
set(project_name "coreota_jobs")

# =====================  Create your mock here  (edit)  ========================

# list the files to mock here
list(APPEND mock_list 
    "${MODULE_ROOT_DIR}/test/unit-test/dependencies/mqtt_wrapper.h")

# list the directories your mocks need
list(APPEND mock_include_list
 "${MODULE_ROOT_DIR}/source/include"
 "${MODULE_ROOT_DIR}/test/unit-test/dependencies/coreMQTT/"
 "${MODULE_ROOT_DIR}/test/unit-test/dependencies/coreMQTT/api_cfg"
 ${MQTT_INCLUDE_PUBLIC_DIRS}
 )

# list the definitions of your mocks to control what to be included
list(APPEND mock_define_list "")

# ================= Create the library under test here (edit) ==================

# list the files you would like to test here
list(APPEND real_source_files ${CORE_JOBS_SOURCES} ${JSON_SOURCES} ${MQTT_SOURCES})
# list the directories the module under test includes
list(APPEND real_include_directories . 
    ${CORE_JOBS_INCLUDES}
    ${JSON_INCLUDE_PUBLIC_DIRS}
    ${MQTT_INCLUDE_PUBLIC_DIRS}
    "${MODULE_ROOT_DIR}/test/unit-test/dependencies/"
    "${MODULE_ROOT_DIR}/test/unit-test/dependencies/coreMQTT/"
    "${MODULE_ROOT_DIR}/test/unit-test/dependencies/coreMQTT/api_cfg")

# =====================  Create UnitTest Code here (edit)  =====================

# list the directories your test needs to include
list(APPEND test_include_directories . ${CORE_JOBS_INCLUDES})

# =============================  (end edit)  ===================================

# Create core_jobs unit test
set(real_name "core_jobs_real")
set(utest_name "core_jobs_utest")
set(utest_source "core_jobs_utest.c")
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
