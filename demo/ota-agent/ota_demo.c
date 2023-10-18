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

#define CONFIG_MAX_FILE_SIZE    65536U
#define NUM_OF_BLOCKS_REQUESTED 1U
#define MAX_THING_NAME_SIZE     128U
#define MAX_JOB_ID_LENGTH       64U
#define MAX_NUM_OF_OTA_DATA_BUFFERS 5U

MqttFileDownloaderContext_t mqttFileDownloaderContext = { 0 };
static uint32_t numOfBlocksRemaining = 0;
static uint32_t currentBlockOffset = 0;
static uint8_t currentFileId = 0;
static uint32_t totalBytesReceived = 0;
static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

static OtaDataEvent_t dataBuffers[MAX_NUM_OF_OTA_DATA_BUFFERS] = { 0 };
static OtaJobEventData_t jobDocBuffer = { 0 };
static SemaphoreHandle_t bufferSemaphore;

static OtaState_t otaAgentState = OtaAgentStateInit;

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
        printf( "Failed to get buffer semaphore.\n" );
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
        printf("Failed to get buffer semaphore. \n" );
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

OtaState_t getOtaAgentState()
{
    return otaAgentState;
}

static void requestJobDocumentHandler()
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );
    coreJobs_startNextPendingJob( thingName,
                                  strnlen( thingName, MAX_THING_NAME_SIZE ),
                                  "test",
                                   4U );
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
                        DATA_TYPE_JSON );
}

static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc )
{
    bool parseJobDocument = false;
    bool handled = false;
    char * jobId;
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
        }
    }

    return handled;
}

static void requestDataBlock( void )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = 0U;

    getStreamRequestLength = mqttDownloader_createGetDataBlockRequest( mqttFileDownloaderContext.dataType,
                                        currentFileId,
                                        mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                        currentBlockOffset,
                                        NUM_OF_BLOCKS_REQUESTED,
                                        getStreamRequest );

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
    printf("Received Event is %d \n", recvEventId);

    switch (recvEventId)
    {
    case OtaAgentEventRequestJobDocument:
        printf("Request Job Document event Received \n");
        printf("-------------------------------------\n");
        requestJobDocumentHandler();
        otaAgentState = OtaAgentStateRequestingJob;
        break;
    case OtaAgentEventReceivedJobDocument:
        printf("Received Job Document event Received \n");
        printf("-------------------------------------\n");

        if (otaAgentState == OtaAgentStateSuspended)
        {
            printf("OTA-Agent is in Suspend State. Hence dropping Job Document. \n");
            break;
        }

        if ( receivedJobDocumentHandler(recvEvent.jobEvent) )
        {
            nextEvent.eventId = OtaAgentEventRequestFileBlock;
            OtaSendEvent_FreeRTOS( &nextEvent );
        }
        else
        {
            printf("This is not an OTA job \n");
        }
        otaAgentState = OtaAgentStateCreatingFile;
        break;
    case OtaAgentEventRequestFileBlock:
        otaAgentState = OtaAgentStateRequestingFileBlock;
        printf("Request File Block event Received \n");
        printf("-----------------------------------\n");
        requestDataBlock();
        break;
    case OtaAgentEventReceivedFileBlock:
        printf("Received File Block event Received \n");
        printf("---------------------------------------\n");
        if (otaAgentState == OtaAgentStateSuspended)
        {
            printf("OTA-Agent is in Suspend State. Hence dropping File Block. \n");
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
        printf("Close file event Received \n");
        printf("-----------------------\n");
        printf( "Downloaded Data %s \n", ( char * ) downloadedData );
        finishDownload();
        otaAgentState = OtaAgentStateStopped;
        break;
    case OtaAgentEventSuspend:
        printf("Suspend Event Received \n");
        printf("-----------------------\n");
        otaAgentState = OtaAgentStateSuspended;
        break;
    case OtaAgentEventResume:
        printf("Resume Event Received \n");
        printf("---------------------\n");
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
    OtaEventMsg_t nextEvent = { 0 };
    bool handled = coreJobs_isStartNextAccepted( topic, topicLength );

    if( handled )
    {
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
        printf( "Unrecognized incoming MQTT message received on topic: "
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
    int8_t fileIndex = 0;

    jobDocLength = coreJobs_getJobDocument( message, messageLength, &jobDoc );

    if( jobDocLength != 0U )
    {
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

    memcpy( downloadedData + totalBytesReceived,
            data,
            dataLength );

    totalBytesReceived += dataLength;

}

static void finishDownload()
{
    /* TODO: Do something with the completed download */
    /* Start the bootloader */
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );
    coreJobs_updateJobStatus( thingName,
                              strnlen( thingName, MAX_THING_NAME_SIZE ),
                              globalJobId,
                              strnlen( globalJobId, 1000U ), /* TODO: True
                                                                strnlen */
                              Succeeded,
                              "2",
                              1U );
    printf( "\033[1;32mOTA Completed successfully!\033[0m\n" );
    globalJobId[ 0 ] = 0U;
}
