/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "core_mqtt.h"
#include "mqtt_wrapper/mqtt_wrapper.h"
#include "ota_demo.h"
#include "transport/transport_wrapper.h"
#include "utils/clock.h"

static TransportInterface_t transport = { 0 };
static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ];

static StaticSemaphore_t MQTTAgentLockBuffer;
static StaticSemaphore_t MQTTStateUpdateLockBuffer;
SemaphoreHandle_t MQTTAgentLock = NULL;
SemaphoreHandle_t MQTTStateUpdateLock = NULL;

static void otaAgentTask( void * parameters );

static void mqttProcessLoopTask( void * parameters );

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo );

static void handleIncomingMQTTMessage( char * topic,
                                       size_t topicLength,
                                       uint8_t * message,
                                       size_t messageLength );

int main( int argc, char * argv[] )
{
    if( argc != 6 )
    {
        printf( "Usage: %s certificateFilePath privateKeyFilePath "
                "rootCAFilePath endpoint thingName\n",
                argv[ 0 ] );
        return 1;
    }

    MQTTAgentLock = xSemaphoreCreateRecursiveMutexStatic(
        &MQTTAgentLockBuffer );
    MQTTStateUpdateLock = xSemaphoreCreateMutexStatic(
        &MQTTStateUpdateLockBuffer );

    MQTTFixedBuffer_t fixedBuffer = { .pBuffer = networkBuffer, .size = 5000U };
    transport_tlsInit( &transport );
    MQTTStatus_t mqttResult = MQTT_Init( &mqttContext,
                                         &transport,
                                         Clock_GetTimeMs,
                                         mqttEventCallback,
                                         &fixedBuffer );
    assert( mqttResult == MQTTSuccess );

    xTaskCreate( otaAgentTask, "T_OTA", 6000, ( void * ) argv, 1, NULL );
    xTaskCreate( mqttProcessLoopTask, "T_MQTT", 6000, NULL, 2, NULL );

    setCoreMqttContext( &mqttContext );
    setThingName( argv[ 5 ] );

    vTaskStartScheduler();

    return 0;
}

static void mqttProcessLoopTask( void * parameters )
{
    ( void ) parameters;

    while( true )
    {
        if( isMqttConnected() )
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
    if( ( packetInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( deserializedInfo->pPublishInfo != NULL );
        char * topic = ( char * ) deserializedInfo->pPublishInfo->pTopicName;
        size_t topicLength = deserializedInfo->pPublishInfo->topicNameLength;
        uint8_t * message = ( uint8_t * )
                                deserializedInfo->pPublishInfo->pPayload;
        size_t messageLength = deserializedInfo->pPublishInfo->payloadLength;
        handleIncomingMQTTMessage( topic, topicLength, message, messageLength );
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

static void handleIncomingMQTTMessage( char * topic,
                                       size_t topicLength,
                                       uint8_t * message,
                                       size_t messageLength )

{
    bool messageHandled = otaDemo_handleIncomingMQTTMessage( topic,
                                                             topicLength,
                                                             message,
                                                             messageLength );
    if( !messageHandled )
    {
        printf( "Unhandled incoming PUBLISH received on topic, message: "
                "%.*s\n%.*s\n",
                ( unsigned int ) topicLength,
                topic,
                ( unsigned int ) messageLength,
                ( char * ) message );
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

    result = mqttConnect( thingName );
    assert( result );
    printf( "Successfully connected to IoT Core\n" );

    otaDemo_start();

    for( ;; )
    {
        vTaskDelay( portMAX_DELAY );
    }
}
