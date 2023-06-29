// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "core_mqtt.h"
#include "transport_wrapper.h"
#include "utils/clock.h"

static TransportInterface_t transport = { 0 };
static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ];

static void otaAgentTask( void * parameters );

static void mqttProcessLoopTask( void * parameters );

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo );

static MQTTStatus_t mqttConnect( char * thingName );

int main( int argc, char * argv[] )
{
    if( argc != 6 )
    {
        printf( "Usage: %s certificateFilePath privateKeyFilePath "
                "rootCAFilePath endpoint thingName\n",
                argv[ 0 ] );
        return 1;
    }

    MQTTFixedBuffer_t fixedBuffer = { .pBuffer = networkBuffer, .size = 5000U };
    transport_tlsInit( &transport );
    MQTTStatus_t mqttResult = MQTT_Init( &mqttContext,
                                         &transport,
                                         Clock_GetTimeMs,
                                         mqttEventCallback,
                                         &fixedBuffer );
    assert( mqttResult == MQTTSuccess );

    xTaskCreate( otaAgentTask, "T_OTA", 6000, ( void * ) argv, 1, NULL );
    xTaskCreate( mqttProcessLoopTask, "T_PROCESS", 6000, NULL, 2, NULL );

    vTaskStartScheduler();

    return 0;
}

static MQTTStatus_t mqttConnect( char * thingName )
{
    MQTTConnectInfo_t connectInfo = { 0 };
    bool sessionPresent = false;
    connectInfo.pClientIdentifier = thingName;
    connectInfo.clientIdentifierLength = ( uint16_t ) sizeof( thingName );
    connectInfo.pUserName = NULL;
    connectInfo.userNameLength = 0U;
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;
    connectInfo.keepAliveSeconds = 60U;
    connectInfo.cleanSession = true;
    return MQTT_Connect( &mqttContext,
                         &connectInfo,
                         NULL,
                         5000U,
                         &sessionPresent );
}

static void mqttProcessLoopTask( void * parameters )
{
    ( void ) parameters;

    while( true )
    {
        if( mqttContext.connectStatus == MQTTConnected )
        {
            MQTTStatus_t status = MQTT_ProcessLoop( &mqttContext );

            if( status == MQTTRecvFailed )
            {
                printf( "ERROR: MQTT Receive failed. Closing connection.\n" );
                exit( 1 );
            }
        }
        vTaskDelay( 10 );
    }
}

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo )
{
    ( void ) mqttContext;
    printf( "MQTT event callback triggered\n" );

    if( ( packetInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( deserializedInfo->pPublishInfo != NULL );
        char * topic = ( char * ) deserializedInfo->pPublishInfo->pTopicName;
        size_t topicLength = deserializedInfo->pPublishInfo->topicNameLength;
        uint8_t * message = ( uint8_t * )
                                deserializedInfo->pPublishInfo->pPayload;
        size_t messageLength = deserializedInfo->pPublishInfo->payloadLength;

        printf( "PUBLISH received with packet id: %u, on topic: %.*s\n",
                ( unsigned int ) deserializedInfo->packetIdentifier,
                ( unsigned int ) topicLength,
                topic );
        // TODO: Do something with the message
        ( void ) message;
        ( void ) messageLength;
    }
    else
    {
        /* Handle other packets. */
        switch( packetInfo->type )
        {
            case MQTT_PACKET_TYPE_PUBACK:
                printf( "PUBACK received with packet id: %u\n",
                        ( unsigned int ) deserializedInfo->packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_SUBACK:
                printf( "SUBACK received with packet id: %u\n",
                        ( unsigned int ) deserializedInfo->packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
                printf( "UNSUBACK received with packet id: %u\n",
                        ( unsigned int ) deserializedInfo->packetIdentifier );
                break;
            default:
                printf( "Error: Unknown packet type received:(%02x).\n",
                        packetInfo->type );
        }
    }
}

static void otaAgentTask( void * parameters )
{
    char ** commandLineArgs = ( char ** ) parameters;
    char * certificateFilePath = commandLineArgs[ 1 ];
    char * privateKeyFilePath = commandLineArgs[ 2 ];
    char * rootCAFilePath = commandLineArgs[ 3 ];
    char * endpoint = commandLineArgs[ 4 ];
    char * thingName = commandLineArgs[ 5 ];

    bool result = transport_tlsConnect( certificateFilePath,
                                        privateKeyFilePath,
                                        rootCAFilePath,
                                        endpoint );
    assert( result );

    MQTTStatus_t mqttResult = mqttConnect( thingName );
    assert( mqttResult == MQTTSuccess );
    printf( "Successfully connected to IoT Core\n" );

    MQTTPublishInfo_t pubInfo = {
        .qos = 0,
        .retain = false,
        .dup = false,
        .pTopicName = "test_topic_publish",
        .topicNameLength = strlen( "test_topic_publish" ),
        .pPayload = "This is my payload!",
        .payloadLength = strlen( "This is my payload!" )
    };
    MQTT_Publish( &mqttContext, &pubInfo, MQTT_GetPacketId( &mqttContext ) );

    MQTTSubscribeInfo_t subscribeInfo = { .qos = 0,
                                          .pTopicFilter = "test_topic",
                                          .topicFilterLength = strlen(
                                              "test_topic" ) };
    MQTT_Subscribe( &mqttContext,
                    &subscribeInfo,
                    1,
                    MQTT_GetPacketId( &mqttContext ) );

    while( true )
    {
    }
}
