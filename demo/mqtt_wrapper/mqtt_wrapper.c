/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <assert.h>
#include <string.h>

#include "mqtt_wrapper/mqtt_wrapper.h"

static MQTTContext_t * globalCoreMqttContext = NULL;

#define MAX_THING_NAME_SIZE 128U
static char globalThingName[ MAX_THING_NAME_SIZE + 1];

void setCoreMqttContext( MQTTContext_t * mqttContext )
{
    globalCoreMqttContext = mqttContext;
}

MQTTContext_t * getCoreMqttContext( void )
{
    assert( globalCoreMqttContext != NULL );
    return globalCoreMqttContext;
}

void setThingName( char * thingName )
{
    strncpy( globalThingName, thingName, MAX_THING_NAME_SIZE );
}

void getThingName( char * thingNameBuffer )
{
    assert( globalThingName[ 0 ] != 0 );
    size_t thingNameLength = strlen(globalThingName);
    memcpy( thingNameBuffer, globalThingName, thingNameLength );
    thingNameBuffer[thingNameLength] = '\0';
}

bool mqttConnect( char * thingName )
{
    assert( globalCoreMqttContext != NULL );
    MQTTConnectInfo_t connectInfo = { 0 };
    MQTTStatus_t mqttStatus = MQTTSuccess;
    bool sessionPresent = false;
    connectInfo.pClientIdentifier = thingName;
    connectInfo.clientIdentifierLength = ( uint16_t ) sizeof( thingName );
    connectInfo.pUserName = NULL;
    connectInfo.userNameLength = 0U;
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;
    connectInfo.keepAliveSeconds = 60U;
    connectInfo.cleanSession = true;
    mqttStatus = MQTT_Connect( globalCoreMqttContext,
                                &connectInfo,
                                NULL,
                                5000U,
                                &sessionPresent );
    return mqttStatus == MQTTSuccess;
}

bool isMqttConnected( void )
{
    assert( globalCoreMqttContext != NULL );
    bool isConnected = globalCoreMqttContext->connectStatus == MQTTConnected;
    return isConnected;
}

bool mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength )
{
    assert( globalCoreMqttContext != NULL );
    bool success = isMqttConnected();
    if( success )
    {
        MQTTStatus_t mqttStatus = MQTTSuccess;
        MQTTPublishInfo_t pubInfo = { .qos = 0,
                                      .retain = false,
                                      .dup = false,
                                      .pTopicName = topic,
                                      .topicNameLength = topicLength,
                                      .pPayload = message,
                                      .payloadLength = messageLength };
        mqttStatus = MQTT_Publish( globalCoreMqttContext,
                                   &pubInfo,
                                   MQTT_GetPacketId( globalCoreMqttContext ) );
        success = mqttStatus == MQTTSuccess;
    }
    return success;
}

bool mqttSubscribe( char * topic, size_t topicLength )
{
    assert( globalCoreMqttContext != NULL );
    bool success = isMqttConnected();
    if( success )
    {
        MQTTStatus_t mqttStatus = MQTTSuccess;
        MQTTSubscribeInfo_t subscribeInfo = { .qos = 0,
                                              .pTopicFilter = topic,
                                              .topicFilterLength = topicLength };
        mqttStatus = MQTT_Subscribe( globalCoreMqttContext,
                                     &subscribeInfo,
                                     1,
                                     MQTT_GetPacketId(
                                         globalCoreMqttContext ) );
        success = mqttStatus == MQTTSuccess;
    }
    return success;
}
