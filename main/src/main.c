/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include "FreeRTOS.h"
#include "credentials/credentials_INSECURE.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "task.h"
#include "wifi/wifi.h"

#include "core_mqtt.h"
#include "mqtt_wrapper.h"
#include "ota_demo/ota_demo.h"
#include "transport/transport.h"
#include "MQTTFileDownloader.h"

static StaticSemaphore_t MQTTAgentLockBuffer;
static StaticSemaphore_t MQTTStateUpdateLockBuffer;
SemaphoreHandle_t MQTTAgentLock = NULL;
SemaphoreHandle_t MQTTStateUpdateLock = NULL;

static MQTTContext_t mqttContext = { 0 };
static uint8_t networkBuffer[ 5000U ] = { 0 };
static TransportInterface_t transport = { 0 };

void mainTask( void * pvParamters );

static uint32_t getTimeMs( void );

static void mqttEventCallback( MQTTContext_t * mqttContext,
                               MQTTPacketInfo_t * packetInfo,
                               MQTTDeserializedInfo_t * deserializedInfo );

static void handleIncomingMQTTMessage( char * topic,
                                       size_t topicLength,
                                       uint8_t * message,
                                       size_t messageLength );
static void mqttProcessLoopTask( void * parameters );

int app_main( void )
{
    MQTTFixedBuffer_t fixedBuffer = { 0 };
    MQTTStatus_t mqttResult;
    nvs_flash_init();
    pfWifi_init();

    MQTTAgentLock = xSemaphoreCreateRecursiveMutexStatic(
        &MQTTAgentLockBuffer );
    MQTTStateUpdateLock = xSemaphoreCreateMutexStatic(
        &MQTTStateUpdateLockBuffer );

    fixedBuffer.pBuffer = networkBuffer;
    fixedBuffer.size = 5000U;

    pfTransport_tlsInit( &transport );
    mqttResult = MQTT_Init( &mqttContext,
                            &transport,
                            getTimeMs,
                            mqttEventCallback,
                            &fixedBuffer );
    assert( mqttResult == MQTTSuccess );

    char thingName[ 128U + 1 ] = { 0 };
    size_t thingNameSize = 129U;
    pfCred_getThingName( thingName, &thingNameSize );

    mqttWrapper_setCoreMqttContext( &mqttContext );
    mqttWrapper_setThingName( thingName, thingNameSize );

    pfWifi_startNetwork();

    xTaskCreate( mainTask, "MAIN", 12000, NULL, 1, NULL );
    xTaskCreate( mqttProcessLoopTask, "MQTT", 6000, NULL, 1, NULL );

    return 0;
}

void mainTask( void * pvParamters )
{
    ( void ) pvParamters;

    ESP_LOGE( "MAIN", "Main task started: v1.0.0" );
    char endpoint[ 256U ] = { 0 };
    size_t endpointLength = 256U;
    pfCred_getEndpoint( endpoint, &endpointLength );
    vTaskDelay( pdMS_TO_TICKS( 100 ) );

    bool result = pfTransport_tlsConnect( endpoint, endpointLength );
    assert( result );

    char thingName[ MAX_THINGNAME_LEN + 1U ] = { 0 };
    size_t thingNameSize = 0U;
    mqttWrapper_getThingName( thingName, &thingNameSize );
    result = mqttWrapper_connect(thingName, thingNameSize);
    assert( result );
    ESP_LOGE( "MAIN", "Connected to IoT Core" );

    otaDemo_start();

    while( true )
    {
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}

static void mqttProcessLoopTask( void * parameters )
{
    ( void ) parameters;

    while( true )
    {
        if( mqttWrapper_isConnected() )
        {
            MQTTStatus_t status = MQTT_ProcessLoop( mqttWrapper_getCoreMqttContext() );

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

static uint32_t getTimeMs( void )
{
    return ( uint32_t ) xTaskGetTickCount() * 1000 / configTICK_RATE_HZ;
}
