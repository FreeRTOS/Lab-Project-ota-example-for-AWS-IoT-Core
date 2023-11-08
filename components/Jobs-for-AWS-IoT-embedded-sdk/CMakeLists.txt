# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/Jobs-for-AWS-IoT-embedded-sdk/jobsFilePaths.cmake")

idf_component_register(
  SRCS ${JOBS_SOURCES} ${OTA_HANDLER_SOURCES}
  INCLUDE_DIRS ${JOBS_INCLUDE_PUBLIC_DIRS} ${OTA_HANDLER_INCLUDES}
  PRIV_REQUIRES corejson )
