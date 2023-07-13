/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "MQTTFileDownloader.h"
#include "core_jobs.h"
#include "mqtt_wrapper.h"
#include "ota_demo.h"
#include "ota_job_processor.h"

#define CONFIG_BLOCK_SIZE       256U
#define CONFIG_MAX_FILE_SIZE    65536U
#define NUM_OF_BLOCKS_REQUESTED 1U
#define MAX_THING_NAME_SIZE     128U
#define MAX_JOB_ID_LENGTH       64U

static uint32_t numOfBlocksRemaining = 0;
static uint32_t currentBlockOffset = 0;
static uint8_t currentFileId = 0;
static uint32_t totalBytesReceived = 0;
static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

static bool handleJobsStartNextAcceptedCallback( const char * jobId,
                                                 const size_t jobIdLength,
                                                 const char * jobDoc,
                                                 const size_t jobDocLength );
static void handleMqttStreamsBlockArrivedCallback(
    MqttFileDownloaderDataBlockInfo_t * dataBlock );
static void processOtaDocumentCallback( AfrOtaJobDocumentFields_t * params );
static void finishDownload();

void otaDemo_start( void )
{
    if( mqttWrapper_isConnected() )
    {
        char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
        mqttWrapper_getThingName( thingName );
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
    bool handled = coreJobs_handleIncomingMQTTMessage(
        &handleJobsStartNextAcceptedCallback,
        topic,
        topicLength,
        message,
        messageLength );

    handled = handled || mqttDownloader_handleIncomingMessage(
                             &handleMqttStreamsBlockArrivedCallback,
                             topic,
                             topicLength,
                             message,
                             messageLength );
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

/* TODO: Implement for the Jobs library */
static bool handleJobsStartNextAcceptedCallback( const char * jobId,
                                                 const size_t jobIdLength,
                                                 const char * jobDoc,
                                                 const size_t jobDocLength )
{
    bool handled = false;
    if( globalJobId[ 0 ] == 0 )
    {
        strncpy( globalJobId, jobId, jobIdLength );
        handled = otaParser_handleJobDoc( &processOtaDocumentCallback,
                                          jobId,
                                          jobIdLength,
                                          jobDoc,
                                          jobDocLength );
    }
    return handled;
}

/* AFR OTA library callback */
static void processOtaDocumentCallback( AfrOtaJobDocumentFields_t * params )
{
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    mqttWrapper_getThingName( thingName );

    numOfBlocksRemaining = params->fileSize / CONFIG_BLOCK_SIZE;
    numOfBlocksRemaining += ( params->fileSize % CONFIG_BLOCK_SIZE > 0 ) ? 1
                                                                         : 0;
    currentFileId = params->fileId;
    currentBlockOffset = 0;
    totalBytesReceived = 0;
    /* Initalize the File downloader */
    mqttDownloader_init( params->imageRef,
                         params->imageRefLen,
                         thingName,
                         DATA_TYPE_JSON );

    /* Request the first block */
    mqttDownloader_requestDataBlock( currentFileId,
                                     CONFIG_BLOCK_SIZE,
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
        mqttDownloader_requestDataBlock( currentFileId,
                                         CONFIG_BLOCK_SIZE,
                                         currentBlockOffset,
                                         NUM_OF_BLOCKS_REQUESTED );
    }
}

static void finishDownload()
{
    /* TODO: Do something with the completed download */
    /* Start the bootloader */
    char thingName[ MAX_THING_NAME_SIZE + 1 ] = { 0 };
    mqttWrapper_getThingName( thingName );
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
