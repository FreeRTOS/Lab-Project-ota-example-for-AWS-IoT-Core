// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

#include "api_cfg/core_mqtt_config.h"

#define LIBRARY_LOG_NAME  "coreMQTT"
#define LIBRARY_LOG_LEVEL LOG_WARN
#include "../csdk_logging/logging.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define MQTT_RECV_POLLING_TIMEOUT_MS    ( 1000 )
#define MQTT_COMMAND_CONTEXTS_POOL_SIZE ( 10 )

extern SemaphoreHandle_t MQTTAgentLock;
extern SemaphoreHandle_t MQTTStateUpdateLock;
#define MQTT_PRE_SEND_HOOK( context )                            \
    if( MQTTAgentLock != NULL )                                  \
    {                                                            \
        xSemaphoreTakeRecursive( MQTTAgentLock, portMAX_DELAY ); \
    }
#define MQTT_POST_SEND_HOOK( context )            \
    if( MQTTAgentLock != NULL )                   \
    {                                             \
        xSemaphoreGiveRecursive( MQTTAgentLock ); \
    }
#define MQTT_PRE_STATE_UPDATE_HOOK( pContext )                \
    if( MQTTStateUpdateLock != NULL )                         \
    {                                                         \
        xSemaphoreTake( MQTTStateUpdateLock, portMAX_DELAY ); \
    }
#define MQTT_POST_STATE_UPDATE_HOOK( pContext ) \
    if( MQTTStateUpdateLock != NULL )           \
    {                                           \
        xSemaphoreGive( MQTTStateUpdateLock );  \
    }

#endif
