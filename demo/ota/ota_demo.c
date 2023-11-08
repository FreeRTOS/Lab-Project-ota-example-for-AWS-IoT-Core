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

#define CONFIG_MAX_FILE_SIZE    65536U
#define NUM_OF_BLOCKS_REQUESTED 1U
#define MAX_THING_NAME_SIZE     128U
#define MAX_JOB_ID_LENGTH       64U

MqttFileDownloaderContext_t mqttFileDownloaderContext = { 0 };
static uint32_t numOfBlocksRemaining = 0;
static uint32_t currentBlockOffset = 0;
static uint8_t currentFileId = 0;
static uint32_t totalBytesReceived = 0;
static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

static void handleMqttStreamsBlockArrived( uint8_t * data, size_t dataLength );
static void processJobFile( AfrOtaJobDocumentFields_t * params );
static void finishDownload();
static bool jobMetadataHandlerChain( char * topic, size_t topicLength );
static bool jobHandlerChain( char * message, size_t messageLength );

void otaDemo_start( void )
{
    if( mqttWrapper_isConnected() )
    {
        char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
        size_t thingNameLength = 0U;
        char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
        char messageBuffer[ 147U ] = { 0 };
        size_t topicLength = 0U;
        mqttWrapper_getThingName( thingName, &thingNameLength );

        Jobs_StartNext(topicBuffer, TOPIC_BUFFER_SIZE, thingName, thingNameLength, &topicLength);

        size_t messageLength = Jobs_StartNextMsg("test", 4U, messageBuffer, 147U );

        mqttWrapper_publish(topicBuffer,
                            topicLength,
                            ( uint8_t * ) messageBuffer,
                            messageLength);

        

    }
}

/* Implemented for use by the MQTT library */
bool otaDemo_handleIncomingMQTTMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength )
{
    bool handled = false;

    handled = jobMetadataHandlerChain( topic, topicLength );

    if( !handled )
    {
        char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
        size_t thingNameLength = 0U;

        mqttWrapper_getThingName( thingName, &thingNameLength );
        handled = Jobs_IsStartNextAccepted( topic, topicLength, thingName, thingNameLength );

        if( handled )
        {
            handled = jobHandlerChain( ( char * ) message, messageLength );

            printf( "Handled? %d", handled );
        }
        else
        {
            handled = mqttDownloader_isDataBlockReceived( &mqttFileDownloaderContext,
                                                      topic,
                                                      topicLength );
            if( handled )
            {
                uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
                size_t decodedDataLength = 0;
                handled = mqttDownloader_processReceivedDataBlock(
                    &mqttFileDownloaderContext,
                    message,
                    messageLength,
                    decodedData,
                    &decodedDataLength );
                handleMqttStreamsBlockArrived( decodedData, decodedDataLength );
            }
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

static bool jobMetadataHandlerChain( char * topic, size_t topicLength )
{
    bool handled = false;

    if( globalJobId[ 0 ] != 0 )
    {
        char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
        size_t thingNameLength = 0U;

        mqttWrapper_getThingName( thingName, &thingNameLength );
        handled = Jobs_IsJobUpdateStatus( topic,
                                              topicLength,
                                              ( const char * ) &globalJobId,
                                              strnlen( globalJobId,
                                                       MAX_JOB_ID_LENGTH ),
                                              thingName,
                                              thingNameLength,
                                              JobUpdateStatus_Accepted );

        if( handled )
        {
            printf( "Job was accepted! Clearing Job ID.\n" );
            globalJobId[ 0 ] = 0;
        }
        else
        {
            handled = Jobs_IsJobUpdateStatus( topic,
                                                  topicLength,
                                                  ( const char * ) &globalJobId,
                                                  strnlen( globalJobId,
                                                           MAX_JOB_ID_LENGTH ),
                                                  thingName,
                                                  thingNameLength,
                                                  JobUpdateStatus_Rejected );
        }

        if( handled )
        {
            printf( "Job was rejected! Clearing Job ID.\n" );
            globalJobId[ 0 ] = 0;
        }
    }

    return handled;
}

static bool jobHandlerChain( char * message, size_t messageLength )
{
    char * jobDoc;
    size_t jobDocLength = 0U;
    char * jobId;
    size_t jobIdLength = 0U;
    int8_t fileIndex = 0;

    jobDocLength = Jobs_GetJobDocument( message, messageLength, &jobDoc );
    jobIdLength = Jobs_GetJobId( message, messageLength, &jobId );

    if( globalJobId[ 0 ] == 0 )
    {
        strncpy( globalJobId, jobId, jobIdLength );
    }

    if( jobDocLength != 0U && jobIdLength != 0U )
    {
        AfrOtaJobDocumentFields_t jobFields = { 0 };

        do
        {
            fileIndex = otaParser_parseJobDocFile( jobDoc,
                                                   jobDocLength,
                                                   fileIndex,
                                                   &jobFields );

            if( fileIndex >= 0 )
            {
                processJobFile( &jobFields );
            }
        } while( fileIndex > 0 );
    }

    // File index will be -1 if an error occured, and 0 if all files were
    // processed
    return fileIndex == 0;
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

/* AFR OTA library callback */
static void processJobFile( AfrOtaJobDocumentFields_t * params )
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    size_t thingNameLength = 0U;

    mqttWrapper_getThingName( thingName, &thingNameLength );

    numOfBlocksRemaining = params->fileSize /
                           mqttFileDownloader_CONFIG_BLOCK_SIZE;
    numOfBlocksRemaining += ( params->fileSize %
                                  mqttFileDownloader_CONFIG_BLOCK_SIZE >
                              0 )
                                ? 1
                                : 0;
    currentFileId = params->fileId;
    currentBlockOffset = 0;
    totalBytesReceived = 0;
    /* Initialize the File downloader */
    mqttDownloader_init( &mqttFileDownloaderContext,
                         params->imageRef,
                         params->imageRefLen,
                         thingName,
                         thingNameLength,
                         DATA_TYPE_CBOR );

    mqttWrapper_subscribe( mqttFileDownloaderContext.topicStreamData,
                            mqttFileDownloaderContext.topicStreamDataLength );

    /* Request the first block */
    requestDataBlock();
}

/* Implemented for the MQTT Streams library */
static void handleMqttStreamsBlockArrived( uint8_t * data, size_t dataLength )
{
    assert( ( totalBytesReceived + dataLength ) < CONFIG_MAX_FILE_SIZE );

    memcpy( downloadedData + totalBytesReceived, data, dataLength );

    totalBytesReceived += dataLength;
    numOfBlocksRemaining--;

    if( numOfBlocksRemaining == 0 )
    {
        printf( "Downloaded Data %s \n", ( char * ) downloadedData );
        finishDownload();
    }
    else
    {
        currentBlockOffset++;
        requestDataBlock();
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
    char messageBuffer[ 48U ] = { 0 };

    mqttWrapper_getThingName( thingName, &thingNameLength );
    
    Jobs_Update(topicBuffer, TOPIC_BUFFER_SIZE, thingName, thingNameLength, globalJobId, strnlen( globalJobId, 1000U ), &topicBufferLength);

    size_t messageBufferLength = Jobs_UpdateMsg(Succeeded, "2", 1U, messageBuffer, 48U);

    mqttWrapper_publish(topicBuffer,
                        topicBufferLength,
                        ( uint8_t * ) messageBuffer,
                        messageBufferLength);

    
    printf( "\033[1;32mOTA Completed successfully!\033[0m\n" );
}
