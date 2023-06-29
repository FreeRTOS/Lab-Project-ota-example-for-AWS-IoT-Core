// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "core_mqtt.h"
#include "transport_wrapper.h"
#include "utils/clock.h"

static TransportInterface_t transport = { 0 };
static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ];

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo )
{
    ( void ) mqttContext;
    ( void ) packetInfo;
    ( void ) deserializedInfo;
    printf( "MQTT event callback triggered\n" );
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

int main( int argc, char * argv[] )
{
    if( argc != 6 )
    {
        printf( "Usage: %s certificateFilePath privateKeyFilePath "
                "rootCAFilePath endpoint thingName\n",
                argv[ 0 ] );
        return 1;
    }

    char * certificateFilePath = argv[ 1 ];
    char * privateKeyFilePath = argv[ 2 ];
    char * rootCAFilePath = argv[ 3 ];
    char * endpoint = argv[ 4 ];
    char * thingName = argv[ 5 ];

    MQTTFixedBuffer_t fixedBuffer = { .pBuffer = networkBuffer, .size = 5000U };
    transport_tlsInit( &transport );
    MQTTStatus_t mqttResult = MQTT_Init( &mqttContext,
                                         &transport,
                                         Clock_GetTimeMs,
                                         mqttEventCallback,
                                         &fixedBuffer );
    assert( mqttResult == MQTTSuccess );

    bool result = transport_tlsConnect( certificateFilePath,
                                        privateKeyFilePath,
                                        rootCAFilePath,
                                        endpoint );
    assert( result );

    mqttResult = mqttConnect( thingName );
    assert( mqttResult == MQTTSuccess );
    printf( "Successfully connected to IoT Core\n" );

    MQTT_ProcessLoop( &mqttContext );

    return 0;
}
