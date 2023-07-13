/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
