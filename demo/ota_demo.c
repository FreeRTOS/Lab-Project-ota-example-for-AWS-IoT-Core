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

static void handleMqttStreamsBlockArrivedCallback(
    MqttFileDownloaderDataBlockInfo_t * dataBlock );
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

        mqttWrapper_getThingName( thingName, &thingNameLength );
        coreJobs_startNextPendingJob( thingName,
                                      strnlen( thingName, MAX_THING_NAME_SIZE ),
                                      "test",
                                      4U );
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
        handled = coreJobs_isStartNextAccepted( topic, topicLength );

        if( handled )
        {
            handled = jobHandlerChain( ( char * ) message, messageLength );

            printf( "Handled? %d", handled );
        }
        else
        {
            handled = mqttDownloader_handleIncomingMessage(
                &mqttFileDownloaderContext,
                &handleMqttStreamsBlockArrivedCallback,
                topic,
                topicLength,
                message,
                messageLength );
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
        handled = coreJobs_isJobUpdateStatus( topic,
                                              topicLength,
                                              ( const char * ) &globalJobId,
                                              strnlen( globalJobId,
                                                       MAX_JOB_ID_LENGTH ),
                                              Accepted );

        if( handled )
        {
            printf( "Job was accepted! Clearing Job ID.\n" );
            globalJobId[ 0 ] = 0;
        }
        else
        {
            handled = coreJobs_isJobUpdateStatus( topic,
                                                  topicLength,
                                                  ( const char * ) &globalJobId,
                                                  strnlen( globalJobId,
                                                           MAX_JOB_ID_LENGTH ),
                                                  Rejected );
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

    jobDocLength = coreJobs_getJobDocument( message, messageLength, &jobDoc );
    jobIdLength = coreJobs_getJobId( message, messageLength, &jobId );

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
                         DATA_TYPE_JSON );

    /* Request the first block */
    mqttDownloader_requestDataBlock( &mqttFileDownloaderContext,
                                     currentFileId,
                                     mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                     currentBlockOffset,
                                     NUM_OF_BLOCKS_REQUESTED );
}

/* Implemented for the MQTT Streams library */
static void handleMqttStreamsBlockArrivedCallback(
    MqttFileDownloaderDataBlockInfo_t * dataBlock )
{
    assert( ( totalBytesReceived + dataBlock->payloadLength ) <
            CONFIG_MAX_FILE_SIZE );

    memcpy( downloadedData + totalBytesReceived,
            dataBlock->payload,
            dataBlock->payloadLength );

    totalBytesReceived += dataBlock->payloadLength;
    numOfBlocksRemaining--;

    if( numOfBlocksRemaining == 0 )
    {
        printf( "Downloaded Data %s \n", ( char * ) downloadedData );
        finishDownload();
    }
    else
    {
        currentBlockOffset++;
        mqttDownloader_requestDataBlock( &mqttFileDownloaderContext,
                                         currentFileId,
                                         mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                         currentBlockOffset,
                                         NUM_OF_BLOCKS_REQUESTED );
    }
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
}
