/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

#include "api_cfg/core_mqtt_config.h"

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
