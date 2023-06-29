// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "core_mqtt.h"
#include "transport/transport.h"

#define NANOSECONDS_PER_MILLISECOND \
    ( 1000000L ) /**< @brief Nanoseconds per millisecond. */
#define MILLISECONDS_PER_SECOND \
    ( 1000L ) /**< @brief Milliseconds per second. */

static TransportInterface_t transport = { 0 };
static bool sessionPresent = false;
static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ];

static uint32_t Clock_GetTimeMs( void )
{
    int64_t timeMs;
    struct timespec timeSpec;

    /* Get the MONOTONIC time. */
    ( void ) clock_gettime( CLOCK_MONOTONIC, &timeSpec );

    /* Calculate the milliseconds from timespec. */
    timeMs = ( timeSpec.tv_sec * MILLISECONDS_PER_SECOND ) +
             ( timeSpec.tv_nsec / NANOSECONDS_PER_MILLISECOND );

    /* Libraries need only the lower 32 bits of the time in milliseconds, since
     * this function is used only for calculating the time difference.
     * Also, the possible overflows of this time value are handled by the
     * libraries. */
    return ( uint32_t ) timeMs;
}

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo )
{
    ( void ) mqttContext;
    ( void ) packetInfo;
    ( void ) deserializedInfo;
    printf( "MQTT event callback triggered\n" );
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

    bool result = transport_tlsConnect( certificateFilePath,
                                        privateKeyFilePath,
                                        rootCAFilePath,
                                        endpoint );
    assert( result );

    MQTTConnectInfo_t connectInfo = { 0 };
    connectInfo.pClientIdentifier = thingName;
    connectInfo.clientIdentifierLength = ( uint16_t ) sizeof( thingName );
    connectInfo.pUserName = NULL;
    connectInfo.userNameLength = 0U;
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;
    connectInfo.keepAliveSeconds = 60U;
    connectInfo.cleanSession = true;
    mqttResult = MQTT_Connect( &mqttContext,
                               &connectInfo,
                               NULL,
                               5000U,
                               &sessionPresent );
    assert( mqttResult == MQTTSuccess );
    printf( "Successfully connected to IoT Core\n" );

    MQTT_ProcessLoop( &mqttContext, 10000U );

    return 0;
}
