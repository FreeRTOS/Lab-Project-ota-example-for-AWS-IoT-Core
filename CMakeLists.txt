# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(aws_iot_core_ota)

set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -Wno-stringop-truncation")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  FreeRTOS_Kernel
  # hash: sha256-UXge9cJcbTaMlaCbrAVQ03AwQ2WQll5aL8BqFlN4Q/Y=
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
  GIT_TAG 6d65558ba01141d7c50ff6f93a4054cc5f16940e)
FetchContent_Declare(
  coreMQTT
  # hash: sha256-iT+mjlduomUTWYEdpoRwoLxJzI0daPjypxlXlXie7nc=
  GIT_REPOSITORY https://github.com/FreeRTOS/coreMQTT.git
  GIT_TAG d41f4572ff4e91d1d3116093331aa23936038f35)
FetchContent_Declare(
  backoffAlgorithm
  # hash: sha256-R5UqyC431OmQG5NM/hh4QOUdo6T60x+CNv1Zuxfjzhw=
  GIT_REPOSITORY https://github.com/FreeRTOS/backoffAlgorithm.git
  GIT_TAG 12a6d73796f7634085af5a25e92f4a5d1200cca6)
FetchContent_Declare(
  coreJSON
  # hash: sha256-r0lJff61NK2rPtO7Wr6RudFNQiLt1D4M30V7/p60Zi0=
  GIT_REPOSITORY https://github.com/FreeRTOS/coreJSON.git
  GIT_TAG a0cd6122745a879225bf459dd257e79bdd63d37a)
FetchContent_Declare(
  tinycbor
  # hash: sha256-otueOD8tkzbHHVuOBI6krewYoeteRGAAQWSf0r19P5s=
  GIT_REPOSITORY https://github.com/intel/tinycbor.git
  GIT_TAG 9924cfed3b95ad6de299ae675064430fdb886216)

# FreeRTOS-Kernel build options
add_library(freertos_config INTERFACE)
target_include_directories(
  freertos_config SYSTEM
  INTERFACE "${CMAKE_CURRENT_LIST_DIR}/cfg/FreeRTOS-Kernel")

set(FREERTOS_HEAP
    "3"
    CACHE STRING "" FORCE)
set(FREERTOS_PORT
    "GCC_POSIX"
    CACHE STRING "" FORCE)

FetchContent_MakeAvailable(FreeRTOS_Kernel coreMQTT backoffAlgorithm coreJSON
                           tinycbor)

# coreMQTT
include("${coremqtt_SOURCE_DIR}/mqttFilePaths.cmake")
add_library(coreMQTT ${MQTT_SOURCES} ${MQTT_SERIALIZER_SOURCES})
target_include_directories(
  coreMQTT
  PUBLIC ${MQTT_INCLUDE_PUBLIC_DIRS}
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cfg/coreMQTT"
  INTERFACE "${CMAKE_CURRENT_LIST_DIR}/cfg/coreMQTT/api_cfg")
target_link_libraries(coreMQTT PRIVATE freertos_kernel)

# backoffAlgorithm
include("${backoffalgorithm_SOURCE_DIR}/backoffAlgorithmFilePaths.cmake")
add_library(backoffAlgorithm ${BACKOFF_ALGORITHM_SOURCES})
target_include_directories(backoffAlgorithm
                           PUBLIC ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS})

# coreJSON
include("${corejson_SOURCE_DIR}/jsonFilePaths.cmake")
add_library(coreJSON ${JSON_SOURCES})
target_include_directories(coreJSON PUBLIC ${JSON_INCLUDE_PUBLIC_DIRS})

# tinycbor
add_library(
  tinycbor
  "${tinycbor_SOURCE_DIR}/src/cborparser.c"
  "${tinycbor_SOURCE_DIR}/src/cborencoder.c"
  "${tinycbor_SOURCE_DIR}/src/cborencoder_close_container_checked.c")
target_include_directories(tinycbor PUBLIC "${tinycbor_SOURCE_DIR}/src")

# iot-core-jobs
include("${CMAKE_CURRENT_LIST_DIR}/lib/iot-core-jobs/coreJobsFilePaths.cmake")
add_library(iot-core-jobs ${CORE_JOBS_SOURCES})
target_link_libraries(iot-core-jobs PUBLIC coreJSON)
target_include_directories(iot-core-jobs PUBLIC ${CORE_JOBS_INCLUDES})

# iot-core-jobs-ota-parser
include(
  "${CMAKE_CURRENT_LIST_DIR}/lib/iot-core-jobs-ota-parser/otaJobsFilePaths.cmake"
)
add_library(iot-core-jobs-ota-parser ${JOBS_HANDLER_SOURCES})
target_link_libraries(iot-core-jobs-ota-parser PUBLIC coreJSON)
target_include_directories(iot-core-jobs-ota-parser
                           PUBLIC ${JOBS_HANDLER_INCLUDES})

# iot-core-mqtt-file-downloader
include(
  "${CMAKE_CURRENT_LIST_DIR}/lib/iot-core-mqtt-file-downloader/mqttFileDownloaderFilePaths.cmake"
)
add_library(iot-core-mqtt-file-downloader ${MQTT_FILE_DOWNLOADER_SOURCES})
target_link_libraries(iot-core-mqtt-file-downloader PUBLIC coreJSON coreMQTT
                                                           tinycbor)
target_include_directories(iot-core-mqtt-file-downloader
                           PUBLIC ${MQTT_FILE_DOWNLOADER_INCLUDES})

add_executable(
  coreOTA_Demo
  ./demo/main.c
  ./demo/mqtt_wrapper/mqtt_wrapper.c
  ./demo/ota_demo.c
  ./demo/transport/openssl_posix.c
  ./demo/transport/sockets_posix.c
  ./demo/transport/transport_wrapper.c
  ./demo/utils/clock_posix.c
  ./demo/utils/freertos_hooks.c)

target_include_directories(
  coreOTA_Demo
  PUBLIC "${CMAKE_CURRENT_LIST_DIR}/source" "${CMAKE_CURRENT_LIST_DIR}/demo"
         "${CMAKE_CURRENT_LIST_DIR}/cfg")

target_link_libraries(
  coreOTA_Demo
  PRIVATE coreMQTT
          OpenSSL::SSL
          coreJSON
          tinycbor
          backoffAlgorithm
          freertos_kernel
          iot-core-jobs
          iot-core-jobs-ota-parser
          iot-core-mqtt-file-downloader)

find_library(LIBRT rt)
if(LIBRT)
  target_link_libraries(coreOTA_Demo PRIVATE rt)
endif()
