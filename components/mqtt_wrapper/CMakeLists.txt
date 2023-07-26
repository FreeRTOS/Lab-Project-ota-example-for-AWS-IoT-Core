# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

idf_component_register(
  SRCS ${CMAKE_CURRENT_LIST_DIR}/../../lib/mqtt_wrapper/mqtt_wrapper.c
  INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../lib/mqtt_wrapper
  REQUIRES coremqtt)
