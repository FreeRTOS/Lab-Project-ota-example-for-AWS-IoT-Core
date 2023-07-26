# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/iot-core-jobs-ota-parser/otaJobsFilePaths.cmake")

idf_component_register(
  SRCS ${JOBS_HANDLER_SOURCES}
  INCLUDE_DIRS ${JOBS_HANDLER_INCLUDES}
  PRIV_REQUIRES corejson)
