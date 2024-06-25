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
#include "jobs.h"
#include "mqtt_wrapper.h"
#include "ota_demo.h"
#include "ota_job_processor.h"
#include "os/ota_os_freertos.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define CONFIG_MAX_FILE_SIZE           65536U
#define NUM_OF_BLOCKS_REQUESTED        1U
#define START_JOB_MSG_LENGTH           147U
#define MAX_THING_NAME_SIZE            128U
#define MAX_JOB_ID_LENGTH              64U
#define UPDATE_JOB_MSG_LENGTH          48U
#define MAX_NUM_OF_OTA_DATA_BUFFERS    5U

MqttFileDownloaderContext_t mqttFileDownloaderContext = { 0 };
static uint32_t numOfBlocksRemaining = 0;
static uint8_t currentFileId = 0;
static uint32_t totalBytesReceived = 0;
static uint8_t * blockBitmap;
static uint32_t totalBlocks = 0;
static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

static OtaDataEvent_t dataBuffers[ MAX_NUM_OF_OTA_DATA_BUFFERS ] = { 0 };
static OtaJobEventData_t jobDocBuffer = { 0 };
static SemaphoreHandle_t bufferSemaphore;

static OtaState_t otaAgentState = OtaAgentStateInit;

static void finishDownload( void );
static void processOTAEvents( void );
static void requestJobDocumentHandler( void );
static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc );
static bool jobDocumentParser( char * message,
                               size_t messageLength,
                               AfrOtaJobDocumentFields_t * jobFields );
static void initMqttDownloader( AfrOtaJobDocumentFields_t * jobFields );
static OtaDataEvent_t * getOtaDataEventBuffer( void );
static void freeOtaDataEventBuffer( OtaDataEvent_t * const buffer );
static void handleMqttStreamsBlockArrived( int32_t blockId,
                                           uint8_t * data,
                                           size_t dataLength );
static void requestDataBlock( uint32_t startingBlock,
                              uint32_t numberOfBlocks );
static bool isBlockNeeded( uint32_t blockId );
static void markBlockDownloaded( uint32_t blockId );
static uint32_t findNextBlockToRequest();
static uint32_t findSuccessiveBlocksToRequest( uint32_t startingBlock );


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
        printf( "Failed to get buffer semaphore. \n" );
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

    while( otaAgentState != OtaAgentStateStopped )
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

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * AWS IoT Jobs library:
     * Creates the topic string for a StartNextPendingJobExecution request.
     * It used to check if any pending jobs are available.
     */
    Jobs_StartNext( topicBuffer,
                    TOPIC_BUFFER_SIZE,
                    thingName,
                    thingNameLength,
                    &topicLength );

    /*
     * AWS IoT Jobs library:
     * Creates the message string for a StartNextPendingJobExecution request.
     * It will be sent on the topic created in the previous step.
     */
    size_t messageLength = Jobs_StartNextMsg( "test",
                                              4U,
                                              messageBuffer,
                                              START_JOB_MSG_LENGTH );

    mqttWrapper_publish( topicBuffer,
                         topicLength,
                         ( uint8_t * ) messageBuffer,
                         messageLength );
}

static void initMqttDownloader( AfrOtaJobDocumentFields_t * jobFields )
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    numOfBlocksRemaining = jobFields->fileSize /
                           mqttFileDownloader_CONFIG_BLOCK_SIZE;
    numOfBlocksRemaining += ( jobFields->fileSize %
                              mqttFileDownloader_CONFIG_BLOCK_SIZE > 0 ) ? 1 : 0;
    currentFileId = jobFields->fileId;
    totalBytesReceived = 0;
    int bitmapSize = ( numOfBlocksRemaining + ( 8 - 1 ) ) / 8;
    blockBitmap = ( uint8_t * ) calloc( bitmapSize, sizeof( uint8_t ) );
    totalBlocks = numOfBlocksRemaining;

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
                         DATA_TYPE_JSON );
}

static bool receivedJobDocumentHandler( OtaJobEventData_t * jobDoc )
{
    bool parseJobDocument = false;
    bool handled = false;
    char * jobId;
    size_t jobIdLength = 0U;
    AfrOtaJobDocumentFields_t jobFields = { 0 };

    /*
     * AWS IoT Jobs library:
     * Extracting the job ID from the received OTA job document.
     */
    jobIdLength = Jobs_GetJobId( ( char * ) jobDoc->jobData, jobDoc->jobDataLength, ( const char ** ) &jobId );

    if( jobIdLength )
    {
        if( strncmp( globalJobId, jobId, jobIdLength ) )
        {
            parseJobDocument = true;
            strncpy( globalJobId, jobId, jobIdLength );
        }
        else
        {
            handled = true;
        }
    }

    if( parseJobDocument )
    {
        handled = jobDocumentParser( ( char * ) jobDoc->jobData, jobDoc->jobDataLength, &jobFields );

        if( handled )
        {
            initMqttDownloader( &jobFields );
        }
    }

    return handled;
}

static void requestDataBlock( uint32_t startingBlock,
                              uint32_t numberOfBlocks )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = 0U;

    printf( "Requesting blocks %u-%u\n", startingBlock, startingBlock + numberOfBlocks );

    /*
     * MQTT streams Library:
     * Creating the Get data block request. MQTT streams library only
     * creates the get block request. To publish the request, MQTT libraries
     * like coreMQTT are required.
     */
    getStreamRequestLength = mqttDownloader_createGetDataBlockRequest( mqttFileDownloaderContext.dataType,
                                                                       currentFileId,
                                                                       mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                                                       startingBlock,
                                                                       numberOfBlocks,
                                                                       getStreamRequest,
                                                                       GET_STREAM_REQUEST_BUFFER_SIZE );

    mqttWrapper_publish( mqttFileDownloaderContext.topicGetStream,
                         mqttFileDownloaderContext.topicGetStreamLength,
                         ( uint8_t * ) getStreamRequest,
                         getStreamRequestLength );
}


static void processOTAEvents()
{
    OtaEventMsg_t recvEvent = { 0 };
    OtaEvent_t recvEventId = 0;
    OtaEventMsg_t nextEvent = { 0 };
    uint32_t startingBlock = 0;
    uint32_t numberOfBlocksToRequest = 0;
    int32_t fileId = 0;
    int32_t blockId = 0;
    int32_t blockSize = 0;

    OtaReceiveEvent_FreeRTOS( &recvEvent );
    recvEventId = recvEvent.eventId;
    printf( "Received Event is %d \n", recvEventId );

    switch( recvEventId )
    {
        case OtaAgentEventRequestJobDocument:
            printf( "Request Job Document event Received \n" );
            printf( "-------------------------------------\n" );
            requestJobDocumentHandler();
            otaAgentState = OtaAgentStateRequestingJob;
            break;

        case OtaAgentEventReceivedJobDocument:
            printf( "Received Job Document event Received \n" );
            printf( "-------------------------------------\n" );

            if( otaAgentState == OtaAgentStateSuspended )
            {
                printf( "OTA-Agent is in Suspend State. Hence dropping Job Document. \n" );
                break;
            }

            if( receivedJobDocumentHandler( recvEvent.jobEvent ) )
            {
                printf( "Received OTA Job. \n" );
                nextEvent.eventId = OtaAgentEventRequestFileBlock;
                OtaSendEvent_FreeRTOS( &nextEvent );
            }
            else
            {
                printf( "This is not an OTA job \n" );
            }

            otaAgentState = OtaAgentStateCreatingFile;
            break;

        case OtaAgentEventRequestFileBlock:
            otaAgentState = OtaAgentStateRequestingFileBlock;
            printf( "Request File Block event Received \n" );
            printf( "-----------------------------------\n" );
            /* Find the block to request in the bitmap */
            startingBlock = findNextBlockToRequest();
            /* Find any other blocks after that starting block which can be requested (up to the configured number of blocks). */
            numberOfBlocksToRequest = findSuccessiveBlocksToRequest( startingBlock );

            if( startingBlock == 0 )
            {
                printf( "Starting The Download. \n" );
            }

            requestDataBlock( startingBlock, numberOfBlocksToRequest );
            break;

        case OtaAgentEventReceivedFileBlock:
            printf( "Received File Block event Received \n" );
            printf( "---------------------------------------\n" );

            if( otaAgentState == OtaAgentStateSuspended )
            {
                printf( "OTA-Agent is in Suspend State. Hence dropping File Block. \n" );
                freeOtaDataEventBuffer( recvEvent.dataEvent );
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
                &fileId,
                &blockId,
                &blockSize,
                decodedData,
                &decodedDataLength );
            handleMqttStreamsBlockArrived( blockId, decodedData, decodedDataLength );
            freeOtaDataEventBuffer( recvEvent.dataEvent );

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
            printf( "Close file event Received \n" );
            printf( "-----------------------\n" );
            printf( "Downloaded Data %s \n", ( char * ) downloadedData );
            finishDownload();
            otaAgentState = OtaAgentStateStopped;
            break;

        case OtaAgentEventSuspend:
            printf( "Suspend Event Received \n" );
            printf( "-----------------------\n" );
            otaAgentState = OtaAgentStateSuspended;
            break;

        case OtaAgentEventResume:
            printf( "Resume Event Received \n" );
            printf( "---------------------\n" );
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
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * AWS IoT Jobs library:
     * Checks if a message comes from the start-next/accepted reserved topic.
     */
    bool handled = Jobs_IsStartNextAccepted( topic,
                                             topicLength,
                                             thingName,
                                             thingNameLength );

    if( handled )
    {
        memcpy( jobDocBuffer.jobData, message, messageLength );
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
        handled = mqttDownloader_isDataBlockReceived( &mqttFileDownloaderContext, topic, topicLength );

        if( handled )
        {
            nextEvent.eventId = OtaAgentEventReceivedFileBlock;
            OtaDataEvent_t * dataBuf = getOtaDataEventBuffer();
            memcpy( dataBuf->data, message, messageLength );
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

static bool jobDocumentParser( char * message,
                               size_t messageLength,
                               AfrOtaJobDocumentFields_t * jobFields )
{
    char * jobDoc;
    size_t jobDocLength = 0U;
    int8_t fileIndex = 0;

    /*
     * AWS IoT Jobs library:
     * Extracting the OTA job document from the jobs message recevied from AWS IoT core.
     */
    jobDocLength = Jobs_GetJobDocument( message, messageLength, ( const char ** ) &jobDoc );

    if( jobDocLength != 0U )
    {
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

    /* File index will be -1 if an error occured, and 0 if all files were
     * processed */
    return fileIndex == 0;
}

/* Stores the received data blocks in the flash partition reserved for OTA */
static void handleMqttStreamsBlockArrived( int32_t blockId,
                                           uint8_t * data,
                                           size_t dataLength )
{
    assert( ( totalBytesReceived + dataLength ) <
            CONFIG_MAX_FILE_SIZE );

    /* Check the bitmap and copy it into the correct position in the file if it is not already there */
    if( isBlockNeeded( blockId ) )
    {
        printf( "Downloaded block %u. Remaining blocks to download: %u. \n", blockId, numOfBlocksRemaining );

        memcpy( downloadedData + ( blockId * mqttFileDownloader_CONFIG_BLOCK_SIZE ), data, dataLength );

        totalBytesReceived += dataLength;
        markBlockDownloaded( blockId );
        numOfBlocksRemaining--;
    }
    else
    {
        printf( "Received already downloaded block: %u\n", blockId );
    }
}

static void finishDownload()
{
    /* TODO: Do something with the completed download */
    /* Start the bootloader */
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;
    char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    size_t topicBufferLength = 0U;
    char messageBuffer[ UPDATE_JOB_MSG_LENGTH ] = { 0 };

    mqttWrapper_getThingName( thingName, &thingNameLength );

    /*
     * AWS IoT Jobs library:
     * Creating the MQTT topic to update the status of OTA job.
     */
    Jobs_Update( topicBuffer,
                 TOPIC_BUFFER_SIZE,
                 thingName,
                 thingNameLength,
                 globalJobId,
                 strnlen( globalJobId, 1000U ),
                 &topicBufferLength );

    /*
     * AWS IoT Jobs library:
     * Creating the message which contains the status of OTA job.
     * It will be published on the topic created in the previous step.
     */
    size_t messageBufferLength = Jobs_UpdateMsg( Succeeded,
                                                 "2",
                                                 1U,
                                                 messageBuffer,
                                                 UPDATE_JOB_MSG_LENGTH );

    mqttWrapper_publish( topicBuffer,
                         topicBufferLength,
                         ( uint8_t * ) messageBuffer,
                         messageBufferLength );
    printf( "\033[1;32mOTA Completed successfully!\033[0m\n" );
    globalJobId[ 0 ] = 0U;
}

static bool isBlockNeeded( uint32_t blockId )
{
    uint32_t byteIndex = blockId >> 3;
    uint32_t bitIndex = blockId & 0x7;

    return ( blockBitmap[ byteIndex ] & ( 1 << bitIndex ) ) == 0;
}
static void markBlockDownloaded( uint32_t blockId )
{
    uint32_t byteIndex = blockId / 8;
    uint32_t bitIndex = blockId % 8;

    blockBitmap[ byteIndex ] |= ( 1 << bitIndex );
}

static uint32_t findNextBlockToRequest()
{
    uint32_t blockId = 0;

    while( blockId <= totalBlocks && !isBlockNeeded( blockId ) )
    {
        blockId++;
    }

    return blockId;
}

static uint32_t findSuccessiveBlocksToRequest( uint32_t startingBlock )
{
    uint32_t blockId = startingBlock;
    uint32_t blocksToRequest = 0;

    while( blockId <= totalBlocks && blocksToRequest < NUM_OF_BLOCKS_REQUESTED && isBlockNeeded( blockId ) )
    {
        blocksToRequest++;
        blockId++;
    }

    return blocksToRequest;
}
