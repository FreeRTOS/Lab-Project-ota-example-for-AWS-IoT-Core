/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdio.h>
#include <string.h>

#include "mqtt_wrapper/mqtt_wrapper.h"
#include "ota_demo.h"

#include "ota_job_handler.h"
#include "ota_job_processor.h"

#define CONFIG_BLOCK_SIZE    256U
#define CONFIG_MAX_FILE_SIZE 65536U

#define MAX_JOB_ID_LENGTH    64U

bool otaDemo_handleJobsStartNextAccepted( const char * jobId,
                                          const size_t jobIdLength,
                                          const char * jobDoc,
                                          const size_t jobDocLength );

static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };
char globalJobId[ MAX_JOB_ID_LENGTH ] = { 0 };

void otaDemo_start( void )
{
    if( isMqttConnected() )
    {
        jobs_startNextPendingJob();
    }
}

/* Implemented for use by the MQTT library */
void otaDemo_handleIncomingMQTTMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength )
{
    bool handled = coreJobsMQTTAPI_handleIncomingMQTTMessage(
        &otaDemo_handleJobsStartNextAccepted,
        topic,
        topicLength,
        message,
        messageLength );
    handled = handled && mqttStreams_handleIncomingMessage( topic,
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
}

/* TODO: Implement for the Jobs library */
bool otaDemo_handleJobsStartNextAccepted( const char * jobId,
                                          const size_t jobIdLength,
                                          const char * jobDoc,
                                          const size_t jobDocLength )
{
    bool handled = false;
    if( globalJobId[ 0 ] == 0 )
    {
        strncpy( globalJobId, jobId, jobIdLength );
        handled = handleJobDoc( jobId, jobIdLength, jobDoc, jobDocLength );
    }
    return handled;
}

/* AFR OTA library callback */
void applicationSuppliedFunction_processAfrOtaDocument(
    AfrOtaJobDocumentFields_t * params )
{
    /* Set to 0 if the filesize is perfectly divisible by the block size */
    uint32_t finalBlockSize = ( params->fileSize % CONFIG_BLOCK_SIZE > 0 ) ? 1
                                                                           : 0;
    uint32_t totalBlocks = params->fileSize / CONFIG_BLOCK_SIZE +
                           finalBlockSize;

    for( int i = 0; i < totalBlocks; i++ )
    {
        mqttStreams_getBlock( params->imageRef,
                              i * CONFIG_BLOCK_SIZE,
                              CONFIG_BLOCK_SIZE );
    }
}

/* Implemented for the MQTT Streams library */
void otaDemo_handleMqttStreamsBlockArrived( MqttStreamDataBlockInfo_t dataBlock )
{
    /* TODO: Add guardrails, this is vulnerable to buffer overwrites */
    /* TODO: How do we know when a block is the final block? */
    memcpy( downloadedData + dataBlock.offset,
            dataBlock.payload,
            dataBlock.blockSize );

    if( globalNumBlocksRemaining == 0 )
    {
        otaDemo_finishDownload();
    }
    else
    {
        globalNumBlocksRemaining--;
    }
}

void otaDemo_finishDownload()
{
    /* TODO: Do something with the completed download */
    /* Start the bootloader */
    jobs_reportJobStatusComplete();
}
