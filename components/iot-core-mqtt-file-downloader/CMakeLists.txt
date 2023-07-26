# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/../../lib/iot-core-mqtt-file-downloader/mqttFileDownloaderFilePaths.cmake")

idf_component_register(
  SRCS ${MQTT_FILE_DOWNLOADER_SOURCES}
  INCLUDE_DIRS ${MQTT_FILE_DOWNLOADER_INCLUDES}
  PRIV_REQUIRES corejson mqtt_wrapper tinycbor)
