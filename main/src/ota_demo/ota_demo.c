/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>


#include "jobs.h"
#include "mqtt_wrapper.h"
#include "ota_demo.h"
#include "ota_job_processor.h"
#include "os/ota_os_freertos.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "esp_log.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"

/* Maximum size of the file which can be downloaded */
#define CONFIG_MAX_FILE_SIZE    1310720U
#define NUM_OF_BLOCKS_REQUESTED 1U
#define MAX_THING_NAME_SIZE     128U
#define MAX_JOB_ID_LENGTH       64U
#define MAX_NUM_OF_OTA_DATA_BUFFERS 5U
#define START_JOB_MSG_LENGTH 147U
#define UPDATE_JOB_MSG_LENGTH 48U

MqttFileDownloaderContext_t mqttFileDownloaderContext = { 0 };
static uint32_t numOfBlocksRemaining = 0;
static uint32_t currentBlockOffset = 0;
static uint8_t currentFileId = 0;
static uint32_t totalBytesReceived = 0;
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

static OtaDataEvent_t dataBuffers[MAX_NUM_OF_OTA_DATA_BUFFERS] = { 0 };
static OtaJobEventData_t jobDocBuffer = { 0 };
static SemaphoreHandle_t bufferSemaphore;

static OtaState_t otaAgentState = OtaAgentStateInit;

typedef struct
{
    const esp_partition_t * update_partition;
    esp_ota_handle_t update_handle;
    bool valid_image;
} esp_ota_context_t;

static esp_ota_context_t ota_ctx;

static void finishDownload( void );

static void processOTAEvents( void );

static void requestJobDocumentHandler( void );

static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc );

static bool jobDocumentParser( char * message, size_t messageLength, AfrOtaJobDocumentFields_t *jobFields );

static void initMqttDownloader( AfrOtaJobDocumentFields_t *jobFields );

static OtaDataEvent_t * getOtaDataEventBuffer( void );

static void freeOtaDataEventBuffer( OtaDataEvent_t * const buffer );

static void handleMqttStreamsBlockArrived( uint8_t *data, size_t dataLength );

static void requestDataBlock( void );


static void freeOtaDataEventBuffer( OtaDataEvent_t * const pxBuffer )
{
    if( xSemaphoreTake( bufferSemaphore, portMAX_DELAY ) == pdTRUE )
    {
        pxBuffer->bufferUsed = false;
        ( void ) xSemaphoreGive( bufferSemaphore );
    }
    else
    {
        ESP_LOGE( "OTA_DEMO", "Failed to get buffer semaphore.\n" );
    }
}

static OtaDataEvent_t * getOtaDataEventBuffer( void )
{
    uint32_t ulIndex = 0;
    OtaDataEvent_t * freeBuffer = NULL;

    if( xSemaphoreTake( bufferSemaphore, portMAX_DELAY ) == pdTRUE )
    {
        for( ulIndex = 0; ulIndex < MAX_NUM_OF_OTA_DATA_BUFFERS; ulIndex++ )
        {
            if( dataBuffers[ ulIndex ].bufferUsed == false )
            {
                dataBuffers[ ulIndex ].bufferUsed = true;
                freeBuffer = &dataBuffers[ ulIndex ];
                break;
            }
        }

        ( void ) xSemaphoreGive( bufferSemaphore );
    }
    else
    {
        ESP_LOGE( "OTA_DEMO","Failed to get buffer semaphore. \n" );
    }

    return freeBuffer;
}

void otaDemo_start( void )
{
    OtaEventMsg_t initEvent = { 0 };

    if( !mqttWrapper_isConnected() )
    {
        return;
    }

    bufferSemaphore = xSemaphoreCreateMutex();

    if( bufferSemaphore != NULL )
    {
        memset( dataBuffers, 0x00, sizeof( dataBuffers ) );
    }

    OtaInitEvent_FreeRTOS();

    initEvent.eventId = OtaAgentEventRequestJobDocument;
    OtaSendEvent_FreeRTOS( &initEvent );

    while (otaAgentState != OtaAgentStateStopped ) {
        processOTAEvents();
    }
}

void otaDemo_processEvents()
{
    if (otaAgentState != OtaAgentStateStopped )
    {
        processOTAEvents();
    }
}

OtaState_t getOtaAgentState()
{
    return otaAgentState;
}

static void requestJobDocumentHandler()
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;
    char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    char messageBuffer[ START_JOB_MSG_LENGTH ] = { 0 };
    size_t topicLength = 0U;
    size_t messageLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * AWS IoT Jobs library:
     * Creates the topic string for a StartNextPendingJobExecution request.
     * It used to check if any pending jobs are available.
     */
    Jobs_StartNext(topicBuffer, TOPIC_BUFFER_SIZE, thingName, thingNameLength, &topicLength);

    /*
     * AWS IoT Jobs library:
     * Creates the message string for a StartNextPendingJobExecution request.
     * It will be sent on the topic created in the previous step.
     */
    messageLength= Jobs_StartNextMsg("test", 4U, messageBuffer, 147U );

    mqttWrapper_publish(topicBuffer,
                        topicLength,
                        ( uint8_t * ) messageBuffer,
                        messageLength);

    ESP_LOGE( "OTA_DEMO","Start next pending job request sent. \n");
}

static void initMqttDownloader( AfrOtaJobDocumentFields_t *jobFields )
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    numOfBlocksRemaining = jobFields->fileSize /
                            mqttFileDownloader_CONFIG_BLOCK_SIZE;
    numOfBlocksRemaining += ( jobFields->fileSize %
                            mqttFileDownloader_CONFIG_BLOCK_SIZE > 0 ) ? 1 : 0;
    currentFileId = jobFields->fileId;
    currentBlockOffset = 0;
    totalBytesReceived = 0;

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * MQTT streams Library:
     * Initializing the MQTT streams downloader. Passing the
     * parameters extracted from the AWS IoT OTA jobs document
     * using OTA jobs parser.
     */
    mqttDownloader_init( &mqttFileDownloaderContext,
                        jobFields->imageRef,
                        jobFields->imageRefLen,
                        thingName,
                        thingNameLength,
                        DATA_TYPE_CBOR );

    mqttWrapper_subscribe( mqttFileDownloaderContext.topicStreamData,
                            mqttFileDownloaderContext.topicStreamDataLength );
}

bool createFileForRx( void )
{
    bool created = false;

    const esp_partition_t * update_partition = esp_ota_get_next_update_partition( NULL );

    if( update_partition == NULL )
    {
        ESP_LOGE( "OTA_DEMO", "Failed to find update partition. \n" );
        return created;
    }

    ESP_LOGE( "OTA_DEMO", "Writing to partition subtype %d at offset 0x%"PRIx32"",
               update_partition->subtype, update_partition->address );

    esp_ota_handle_t update_handle;
    esp_err_t err = esp_ota_begin( update_partition, OTA_SIZE_UNKNOWN, &update_handle );

    if( err != ESP_OK )
    {
        ESP_LOGE( "OTA_DEMO", "esp_ota_begin failed (%d)", err  );
        return created;
    }

    memset(&ota_ctx, '0', sizeof(esp_ota_context_t));
    ota_ctx.update_partition = update_partition;
    ota_ctx.update_handle = update_handle;

    ota_ctx.valid_image = false;
    created = true;

    ESP_LOGE( "OTA_DEMO", "esp_ota_begin succeeded" );

    return created;
}


static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc )
{
    bool parseJobDocument = false;
    bool handled = false;
    char * jobId = NULL;
    size_t jobIdLength = 0U;
    AfrOtaJobDocumentFields_t jobFields = { 0 };

    /*
     * AWS IoT Jobs library:
     * Extracting the job ID from the received OTA job document.
     */
    jobIdLength = Jobs_GetJobId( (char *)jobDoc->jobData, jobDoc->jobDataLength, &jobId );

    if ( jobIdLength )
    {
        if ( strncmp( globalJobId, jobId, jobIdLength ) )
        {
            parseJobDocument = true;
            strncpy( globalJobId, jobId, jobIdLength );
        }
        else
        {
            handled = true;
        }
    }

    if ( parseJobDocument )
    {
        handled = jobDocumentParser( (char * )jobDoc->jobData, jobDoc->jobDataLength, &jobFields );
        if (handled)
        {
            initMqttDownloader( &jobFields );
            createFileForRx();
        }
    }

    return handled;
}

static void requestDataBlock( void )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = 0U;

    /*
     * MQTT streams Library:
     * Creating the Get data block request. MQTT streams library only
     * creates the get block request. To publish the request, MQTT libraries
     * like coreMQTT are required.
     */
    getStreamRequestLength = mqttDownloader_createGetDataBlockRequest( mqttFileDownloaderContext.dataType,
                                        currentFileId,
                                        mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                        currentBlockOffset,
                                        NUM_OF_BLOCKS_REQUESTED,
                                        getStreamRequest,
                                        &getStreamRequestLength );

    mqttWrapper_publish( mqttFileDownloaderContext.topicGetStream,
                         mqttFileDownloaderContext.topicGetStreamLength,
                         ( uint8_t * ) getStreamRequest,
                         getStreamRequestLength );
}

static void processOTAEvents() {
    OtaEventMsg_t recvEvent = { 0 };
    OtaEvent_t recvEventId = 0;
    OtaEventMsg_t nextEvent = { 0 };

    OtaReceiveEvent_FreeRTOS(&recvEvent);
    recvEventId = recvEvent.eventId;
    ESP_LOGE( "OTA_DEMO","Received Event is %d \n", recvEventId);
    ESP_LOGE( "OTA_DEMO","Current state is %d \n", otaAgentState);

    switch (recvEventId)
    {
    case OtaAgentEventRequestJobDocument:
        ESP_LOGE( "OTA_DEMO","Request Job Document event Received \n");
        ESP_LOGE( "OTA_DEMO","-------------------------------------\n");
        if (otaAgentState == OtaAgentStateSuspended)
        {
            ESP_LOGE( "OTA_DEMO","OTA-Agent is in Suspend State. Hence dropping Request Job document event. \n");
            break;
        }
        requestJobDocumentHandler();
        otaAgentState = OtaAgentStateRequestingJob;
        break;
    case OtaAgentEventReceivedJobDocument:
        ESP_LOGE( "OTA_DEMO","Received Job Document event Received \n");
        ESP_LOGE( "OTA_DEMO","-------------------------------------\n");

        if (otaAgentState == OtaAgentStateSuspended)
        {
            ESP_LOGE( "OTA_DEMO","OTA-Agent is in Suspend State. Hence dropping Job Document. \n");
            break;
        }

        if ( receivedJobDocumentHandler(recvEvent.jobEvent) )
        {
            nextEvent.eventId = OtaAgentEventRequestFileBlock;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
        else
        {
            ESP_LOGE( "OTA_DEMO","This is not an OTA job \n");
            vTaskDelay( pdMS_TO_TICKS( 5000UL ) );
            nextEvent.eventId = OtaAgentEventRequestJobDocument;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
        otaAgentState = OtaAgentStateCreatingFile;
        break;
    case OtaAgentEventRequestFileBlock:
        ESP_LOGE( "OTA_DEMO","Request File Block event Received \n");
        ESP_LOGE( "OTA_DEMO","-----------------------------------\n");
        if (otaAgentState == OtaAgentStateSuspended)
        {
            ESP_LOGE( "OTA_DEMO","OTA-Agent is in Suspend State. Hence dropping Request file block event. \n");
            break;
        }
        otaAgentState = OtaAgentStateRequestingFileBlock;
        requestDataBlock();
        break;
    case OtaAgentEventReceivedFileBlock:
        ESP_LOGE( "OTA_DEMO","Received File Block event Received \n");
        ESP_LOGE( "OTA_DEMO","---------------------------------------\n");
        if (otaAgentState == OtaAgentStateSuspended)
        {
            ESP_LOGE( "OTA_DEMO","OTA-Agent is in Suspend State. Hence dropping File Block. \n");
            freeOtaDataEventBuffer(recvEvent.dataEvent);
            break;
        }
        uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
        size_t decodedDataLength = 0;
        /*
         * MQTT streams Library:
         * Extracting and decoding the received data block from the incoming MQTT message.
         */
        mqttDownloader_processReceivedDataBlock(
            &mqttFileDownloaderContext,
            recvEvent.dataEvent->data,
            recvEvent.dataEvent->dataLength,
            decodedData,
            &decodedDataLength );
        handleMqttStreamsBlockArrived(decodedData, decodedDataLength);
        freeOtaDataEventBuffer(recvEvent.dataEvent);
        ESP_LOGE( "OTA_DEMO","Received block number %lu \n", currentBlockOffset);
        numOfBlocksRemaining--;
        currentBlockOffset++;

        if( numOfBlocksRemaining == 0 )
        {
            nextEvent.eventId = OtaAgentEventCloseFile;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
        else
        {
            nextEvent.eventId = OtaAgentEventRequestFileBlock;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
        break;
    case OtaAgentEventCloseFile:
        ESP_LOGE( "OTA_DEMO","Close file event Received \n");
        ESP_LOGE( "OTA_DEMO","-----------------------\n");
        finishDownload();
        otaAgentState = OtaAgentStateStopped;
        break;
    case OtaAgentEventSuspend:
        ESP_LOGE( "OTA_DEMO","Suspend Event Received \n");
        ESP_LOGE( "OTA_DEMO","-----------------------\n");
        otaAgentState = OtaAgentStateSuspended;
        break;
    case OtaAgentEventResume:
        ESP_LOGE( "OTA_DEMO","Resume Event Received \n");
        ESP_LOGE( "OTA_DEMO","---------------------\n");
        otaAgentState = OtaAgentStateRequestingJob;
        nextEvent.eventId = OtaAgentEventRequestJobDocument;
        OtaSendEvent_FreeRTOS( &nextEvent );
    default:
        break;
    }
}

/* Implemented for use by the MQTT library */
bool otaDemo_handleIncomingMQTTMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength )
{
    ESP_LOGE( "OTA_DEMO","Handling Incoming MQTT message \n");
    OtaEventMsg_t nextEvent = { 0 };
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * AWS IoT Jobs library:
     * Checks if a message comes from the start-next/accepted reserved topic.
     */
    bool handled = Jobs_IsStartNextAccepted( topic, topicLength, thingName, thingNameLength );

    if( handled )
    {
        memset(jobDocBuffer.jobData, '0', JOB_DOC_SIZE);
        memcpy(jobDocBuffer.jobData, message, messageLength);
        nextEvent.jobEvent = &jobDocBuffer;
        jobDocBuffer.jobDataLength = messageLength;
        nextEvent.eventId = OtaAgentEventReceivedJobDocument;
        OtaSendEvent_FreeRTOS( &nextEvent );
    }
    else
    {
        /*
         * MQTT streams Library:
         * Checks if the incoming message contains the requested data block. It is performed by
         * comparing the incoming MQTT message topic with MQTT streams topics.
         */
        handled = mqttDownloader_isDataBlockReceived(&mqttFileDownloaderContext, topic, topicLength);
        if (handled)
        {
            nextEvent.eventId = OtaAgentEventReceivedFileBlock;
            OtaDataEvent_t * dataBuf = getOtaDataEventBuffer();
            memcpy(dataBuf->data, message, messageLength);
            nextEvent.dataEvent = dataBuf;
            dataBuf->dataLength = messageLength;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
    }

    if( !handled )
    {
        ESP_LOGE( "OTA_DEMO", "Unrecognized incoming MQTT message received on topic: "
                "%.*s\nMessage: %.*s\n",
                ( unsigned int ) topicLength,
                topic,
                ( unsigned int ) messageLength,
                ( char * ) message );
    }
    return handled;
}

static bool jobDocumentParser( char * message, size_t messageLength, AfrOtaJobDocumentFields_t *jobFields )
{
    char * jobDoc;
    size_t jobDocLength = 0U;
    int8_t fileIndex = -1;

    /*
     * AWS IoT Jobs library:
     * Extracting the OTA job document from the jobs message recevied from AWS IoT core.
     */
    jobDocLength = Jobs_GetJobDocument( message, messageLength, &jobDoc );

    if( jobDocLength != 0U )
    {
        fileIndex = 0;
        do
        {
            /*
             * AWS IoT Jobs library:
             * Parsing the OTA job document to extract all of the parameters needed to download
             * the new firmware.
             */
            fileIndex = otaParser_parseJobDocFile( jobDoc,
                                                jobDocLength,
                                                fileIndex,
                                                jobFields );
        } while( fileIndex > 0 );
    }

    // File index will be -1 if an error occured, and 0 if all files were
    // processed
    return fileIndex == 0;
}

/* Stores the received data blocks in the flash partition reserved for OTA */
static void handleMqttStreamsBlockArrived(
    uint8_t *data, size_t dataLength )
{
    assert( ( totalBytesReceived + dataLength ) <
            CONFIG_MAX_FILE_SIZE );

    esp_err_t ret = esp_ota_write_with_offset( ota_ctx.update_handle, data, dataLength, totalBytesReceived );

    if( ret != ESP_OK )
    {
        ESP_LOGE( "OTA_DEMO", "Couldn't flash at the offset %"PRIu32"", totalBytesReceived );
        return;
    }

    totalBytesReceived += dataLength;

}

/* Activates the new firmware image */
bool activateNewImage( void )
{
    bool activated = false;

    if( esp_ota_end( ota_ctx.update_handle ) != ESP_OK )
    {
        ESP_LOGE( "OTA_DEMO", "esp_ota_end failed!" );
        esp_partition_erase_range( ota_ctx.update_partition, 0, ota_ctx.update_partition->size );
    }

    esp_err_t err = esp_ota_set_boot_partition( ota_ctx.update_partition );

    if( err != ESP_OK )
    {
        ESP_LOGE( "OTA_DEMO", "esp_ota_set_boot_partition failed (%d)!", err  );
        esp_partition_erase_range( ota_ctx.update_partition, 0, ota_ctx.update_partition->size );
    }
    else
    {
        activated = true;
        ESP_LOGE( "OTA_DEMO", "Image successfully activated \n" );
    }

    return activated;
}

static void finishDownload()
{

    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;
    char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    size_t topicBufferLength = 0U;
    char messageBuffer[ UPDATE_JOB_MSG_LENGTH  ] = { 0 };

    bool status = activateNewImage();

    if ( status )
    {
        mqttWrapper_getThingName( thingName, &thingNameLength );

        /*
         * AWS IoT Jobs library:
         * Creating the MQTT topic to update the status of OTA job.
         */
        Jobs_Update(topicBuffer, TOPIC_BUFFER_SIZE, thingName, thingNameLength, globalJobId, strnlen( globalJobId, 64U ), &topicBufferLength);

        /*
         * AWS IoT Jobs library:
         * Creating the message which contains the status of OTA job.
         * It will be published on the topic created in the previous step.
         */
        size_t messageBufferLength = Jobs_UpdateMsg(Succeeded, "2", 1U, messageBuffer, 48U);

        mqttWrapper_publish(topicBuffer,
                            topicBufferLength,
                        ( uint8_t * ) messageBuffer,
                        messageBufferLength);
        ESP_LOGE( "OTA_DEMO", "\033[1;32mOTA Completed successfully!\033[0m\n" );
    }
    else
    {
        ESP_LOGE( "OTA_DEMO", "\033[1;32mOTA Completed failed!\033[0m\n" );
    }

    globalJobId[ 0 ] = 0U;

    vTaskDelay( pdMS_TO_TICKS( 1000UL ) );
    esp_restart();
}
