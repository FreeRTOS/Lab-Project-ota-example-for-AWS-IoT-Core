# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

idf_component_register(
  SRCS src/main.c
       src/credentials/credentials_INSECURE.c
       src/transport/transport.c
       src/wifi/wifi.c
       src/key_value_store/key_value_store.c
       src/os/ota_os_freertos.c
       src/ota_demo/ota_demo.c
  PRIV_REQUIRES
    app_update
    bootloader_support
    driver
    efuse
    esp-tls
    esp_wifi
    freertos_compat
    mbedtls
    nvs_flash
    spi_flash
    coremqtt
    corejson
    tinycbor
    backoffalgorithm
    mqtt_wrapper
    iot-core-jobs
    iot-core-jobs-ota-parser
    iot-core-mqtt-file-downloader
  PRIV_INCLUDE_DIRS src)

set_property(
  TARGET ${COMPONENT_LIB}
  APPEND
  PROPERTY LINK_INTERFACE_MULTIPLICITY 3)

  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
