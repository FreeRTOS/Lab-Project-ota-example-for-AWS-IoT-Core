# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/iot-core-jobs/coreJobsFilePaths.cmake")

idf_component_register(
  SRCS ${CORE_JOBS_SOURCES}
  INCLUDE_DIRS ${CORE_JOBS_INCLUDES}
  PRIV_REQUIRES corejson mqtt_wrapper)
