# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/coreMQTT/mqttFilePaths.cmake")

idf_component_register(
  SRCS ${MQTT_SOURCES} ${MQTT_SERIALIZER_SOURCES}
  INCLUDE_DIRS ${MQTT_INCLUDE_PUBLIC_DIRS} ../../main/cfg/coreMQTT
  REQUIRES freertos_compat)
