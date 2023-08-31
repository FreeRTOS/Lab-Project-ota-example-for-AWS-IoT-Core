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
#include "mqtt_wrapper.h"
#include "ota_demo.h"
#include "transport/transport_wrapper.h"
#include "utils/clock.h"

#define MAX_THING_NAME_SIZE 128U

static TransportInterface_t transport = { 0 };
static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ];

static StaticSemaphore_t MQTTAgentLockBuffer;
static StaticSemaphore_t MQTTStateUpdateLockBuffer;
SemaphoreHandle_t MQTTAgentLock = NULL;
SemaphoreHandle_t MQTTStateUpdateLock = NULL;

static void otaAgentTask( void * parameters );

static void mqttProcessLoopTask( void * parameters );

static void suspendResumeLoopTask( void * parameters );

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo );

static void handleIncomingMQTTMessage( char * topic,
                                       size_t topicLength,
                                       uint8_t * message,
                                       size_t messageLength );

int main( int argc, char * argv[] )
{
    MQTTStatus_t mqttResult;
    MQTTFixedBuffer_t fixedBuffer = { 0 };

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

    fixedBuffer.pBuffer = networkBuffer;
    fixedBuffer.size = 5000U;

    transport_tlsInit( &transport );
    mqttResult = MQTT_Init( &mqttContext,
                            &transport,
                            Clock_GetTimeMs,
                            mqttEventCallback,
                            &fixedBuffer );
    assert( mqttResult == MQTTSuccess );

    xTaskCreate( otaAgentTask, "T_OTA", 6000, ( void * ) argv, 1, NULL );
    xTaskCreate( mqttProcessLoopTask, "T_MQTT", 6000, NULL, 2, NULL );
    xTaskCreate( suspendResumeLoopTask, "T_SUSPEND", 6000, NULL, 2, NULL );

    mqttWrapper_setCoreMqttContext( &mqttContext );
    mqttWrapper_setThingName( argv[ 5 ],
                              strnlen( argv[ 5 ], MAX_THING_NAME_SIZE ) );

    vTaskStartScheduler();

    return 0;
}

static void suspendResumeLoopTask( void * parameters )
{
    ( void ) parameters;
    OtaEventMsg_t testEvent = { 0 };
    OtaState_t curState;
    printf("Suspend resume task \n");
    while( true )
    {
        curState = getOtaAgentState();

        if  (curState != OtaAgentStateStopped && curState >= OtaAgentStateRequestingJob && curState != OtaAgentStateSuspended)
        {
            testEvent.eventId = OtaAgentEventSuspend;
            OtaSendEvent_FreeRTOS( &testEvent );
        }

        vTaskDelay( 10 );

        curState = getOtaAgentState();
        if (curState == OtaAgentStateSuspended)
        {
            testEvent.eventId = OtaAgentEventResume;
            OtaSendEvent_FreeRTOS( &testEvent );
        }
        vTaskDelay( 30 );

    }
}

static void mqttProcessLoopTask( void * parameters )
{
    ( void ) parameters;

    while( true )
    {
        if( mqttWrapper_isConnected() )
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
    char * topic = NULL;
    size_t topicLength = 0U;
    uint8_t * message = NULL;
    size_t messageLength = 0U;

    ( void ) mqttContext;

    if( ( packetInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( deserializedInfo->pPublishInfo != NULL );
        topic = ( char * ) deserializedInfo->pPublishInfo->pTopicName;
        topicLength = deserializedInfo->pPublishInfo->topicNameLength;
        message = ( uint8_t * ) deserializedInfo->pPublishInfo->pPayload;
        messageLength = deserializedInfo->pPublishInfo->payloadLength;
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

    result = mqttWrapper_connect( thingName,
                                  strnlen( thingName, MAX_THING_NAME_SIZE ) );
    assert( result );
    printf( "Successfully connected to IoT Core\n" );

    otaDemo_start();

    for( ;; )
    {
        vTaskDelay( portMAX_DELAY );
    }
}
