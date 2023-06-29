// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

#include "api_cfg/core_mqtt_config.h"

#define LIBRARY_LOG_NAME  "coreMQTT"
#define LIBRARY_LOG_LEVEL LOG_WARN
#include "../csdk_logging/logging.h"

#define MQTT_RECV_POLLING_TIMEOUT_MS    ( 1000 )
#define MQTT_COMMAND_CONTEXTS_POOL_SIZE ( 10 )

#endif
