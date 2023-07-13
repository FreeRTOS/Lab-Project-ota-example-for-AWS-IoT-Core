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

#include <assert.h>
#include <string.h>

#include "mqtt_wrapper.h"

static MQTTContext_t * globalCoreMqttContext = NULL;

#define MAX_THING_NAME_SIZE 128U
static char globalThingName[ MAX_THING_NAME_SIZE + 1 ];

void mqttWrapper_setCoreMqttContext( MQTTContext_t * mqttContext )
{
    globalCoreMqttContext = mqttContext;
}

MQTTContext_t * mqttWrapper_getCoreMqttContext( void )
{
    assert( globalCoreMqttContext != NULL );
    return globalCoreMqttContext;
}

void mqttWrapper_setThingName( char * thingName )
{
    strncpy( globalThingName, thingName, MAX_THING_NAME_SIZE );
}

void mqttWrapper_getThingName( char * thingNameBuffer )
{
    assert( globalThingName[ 0 ] != 0 );
    size_t thingNameLength = strlen( globalThingName );
    memcpy( thingNameBuffer, globalThingName, thingNameLength );
    thingNameBuffer[ thingNameLength ] = '\0';
}

bool mqttWrapper_connect( char * thingName )
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

bool mqttWrapper_isConnected( void )
{
    assert( globalCoreMqttContext != NULL );
    bool isConnected = globalCoreMqttContext->connectStatus == MQTTConnected;
    return isConnected;
}

bool mqttWrapper_publish( char * topic,
                          size_t topicLength,
                          uint8_t * message,
                          size_t messageLength )
{
    assert( globalCoreMqttContext != NULL );
    bool success = mqttWrapper_isConnected();
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

bool mqttWrapper_subscribe( char * topic, size_t topicLength )
{
    assert( globalCoreMqttContext != NULL );
    bool success = mqttWrapper_isConnected();
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
