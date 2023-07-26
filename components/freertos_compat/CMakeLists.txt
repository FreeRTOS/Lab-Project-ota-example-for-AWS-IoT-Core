# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

set(FREERTOS_COMPONENT_PATH $ENV{IDF_PATH}/components/freertos)

idf_component_register(
  INCLUDE_DIRS
    ${FREERTOS_COMPONENT_PATH}/FreeRTOS-Kernel/include/freertos
    ${FREERTOS_COMPONENT_PATH}/FreeRTOS-Kernel/portable/xtensa/include/freertos
    ${FREERTOS_COMPONENT_PATH}/esp_additions/include/freertos
  REQUIRES freertos)
