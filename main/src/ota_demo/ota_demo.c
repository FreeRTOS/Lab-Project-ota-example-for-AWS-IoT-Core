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

#include "MQTTFileDownloader.h"
#include "core_jobs.h"
#include "mqtt_wrapper.h"
#include "ota_demo.h"
#include "ota_job_processor.h"
#include "os/ota_os_freertos.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "esp_log.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"

#define CONFIG_MAX_FILE_SIZE    1310720U
#define NUM_OF_BLOCKS_REQUESTED 1U
#define MAX_THING_NAME_SIZE     128U
#define MAX_JOB_ID_LENGTH       64U
#define MAX_NUM_OF_OTA_DATA_BUFFERS 5U

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
    bool ret = false;
    mqttWrapper_getThingName( thingName, &thingNameLength );
    ret = coreJobs_startNextPendingJob( thingName,
                                  strnlen( thingName, MAX_THING_NAME_SIZE ),
                                  "test",
                                   4U );
    ESP_LOGE( "OTA_DEMO","Start next pending job return %d \n", ret);
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

    /* Initialize the File downloader */
    mqttDownloader_init( &mqttFileDownloaderContext,
                        jobFields->imageRef,
                        jobFields->imageRefLen,
                        thingName,
                        thingNameLength,
                        DATA_TYPE_CBOR );
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

    LogInfo( ( "esp_ota_begin succeeded" ) );

    return created;
}


static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc )
{
    bool parseJobDocument = false;
    bool handled = false;
    char * jobId = NULL;
    size_t jobIdLength = 0U;
    AfrOtaJobDocumentFields_t jobFields = { 0 };

    jobIdLength = coreJobs_getJobId( (char *)jobDoc->jobData, jobDoc->jobDataLength, &jobId );

    /* TODO: Change length to the length of global job id */
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

static void processOTAEvents() {
    OtaEventMsg_t recvEvent = { 0 };
    OtaEvent_t recvEventId = 0;
    OtaEventMsg_t nextEvent = { 0 };

    OtaReceiveEvent_FreeRTOS(&recvEvent);
    recvEventId = recvEvent.eventId;
    ESP_LOGE( "OTA_DEMO","Received Event is %d \n", recvEventId);

    switch (recvEventId)
    {
    case OtaAgentEventRequestJobDocument:
        ESP_LOGE( "OTA_DEMO","Request Job Document event Received \n");
        ESP_LOGE( "OTA_DEMO","-------------------------------------\n");
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
        otaAgentState = OtaAgentStateRequestingFileBlock;
        ESP_LOGE( "OTA_DEMO","Request File Block event Received \n");
        ESP_LOGE( "OTA_DEMO","-----------------------------------\n");
        mqttDownloader_requestDataBlock( &mqttFileDownloaderContext,
                                        currentFileId,
                                        mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                        currentBlockOffset,
                                        NUM_OF_BLOCKS_REQUESTED );
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
    bool handled = coreJobs_isStartNextAccepted( topic, topicLength );

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

    jobDocLength = coreJobs_getJobDocument( message, messageLength, &jobDoc );

    if( jobDocLength != 0U )
    {
        fileIndex = 0;
        do
        {
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

/* Implemented for the MQTT Streams library */
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
    /* TODO: Do something with the completed download */
    /* Start the bootloader */
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    bool status = activateNewImage();

    if ( status )
    {
        mqttWrapper_getThingName( thingName, &thingNameLength );
        coreJobs_updateJobStatus( thingName,
                              strnlen( thingName, MAX_THING_NAME_SIZE ),
                              globalJobId,
                              strnlen( globalJobId, 64U ), /* TODO: True
                                                                strnlen */
                              Succeeded,
                              "2",
                              1U );
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
