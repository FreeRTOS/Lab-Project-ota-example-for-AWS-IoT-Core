# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(aws_iot_core_ota)

set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -Werror")
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
  # hash: sha256-uIX8FFrTlzfyGY4ViQo7ylbtAW6uubazfK+5hAWNfwA=
  GIT_REPOSITORY https://github.com/FreeRTOS/coreMQTT.git
  GIT_TAG ec3380e5f1a0d7b0cdcf5e023a4adbeac571fc2b)
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
FetchContent_Declare(
  mbedtls
  # hash: sha256-+M36NvFe4gw2PRbld/2JV3yBGrqK6soWcmrSEkUNcrc=
  GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
  GIT_TAG v3.2.1
  SOURCE_SUBDIR cmake)

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

# mbedTLS build options
set(ENABLE_TESTING
    OFF
    CACHE INTERNAL "" FORCE)
set(ENABLE_PROGRAMS
    OFF
    CACHE INTERNAL "" FORCE)

FetchContent_MakeAvailable(FreeRTOS_Kernel coreMQTT backoffAlgorithm coreJSON
                           tinycbor)

FetchContent_GetProperties(mbedtls)
if(NOT mbedtls_POPULATED)
  FetchContent_Populate(mbedtls)
  add_subdirectory(${mbedtls_SOURCE_DIR} ${mbedtls_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# coreMQTT
include("${coremqtt_SOURCE_DIR}/mqttFilePaths.cmake")
add_library(coreMQTT ${MQTT_SOURCES} ${MQTT_SERIALIZER_SOURCES})
target_include_directories(
  coreMQTT
  PUBLIC ${MQTT_INCLUDE_PUBLIC_DIRS}
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cfg/coreMQTT"
  INTERFACE "${CMAKE_CURRENT_LIST_DIR}/cfg/coreMQTT/api_cfg")

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

add_executable(coreOTA ./source/main.c ./source/transport/transport.c ./source/transport/openssl_posix.c ./source/transport/sockets_posix.c)

target_include_directories(coreOTA PUBLIC "${CMAKE_CURRENT_LIST_DIR}/source" "${CMAKE_CURRENT_LIST_DIR}/cfg")

target_link_libraries(
  coreOTA PRIVATE coreMQTT freertos_kernel OpenSSL::SSL coreJSON tinycbor
                  backoffAlgorithm MbedTLS::mbedx509)

find_library(LIBRT rt)
if(LIBRT)
  target_link_libraries(coreOTA PRIVATE rt)
endif()
