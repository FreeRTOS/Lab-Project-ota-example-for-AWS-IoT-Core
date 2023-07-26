# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/coreJSON/jsonFilePaths.cmake")

idf_component_register(SRCS ${JSON_SOURCES}
                       INCLUDE_DIRS ${JSON_INCLUDE_PUBLIC_DIRS})
