#include <stdio.h>
#include <string.h>

#include "mqtt_wrapper/mqtt_wrapper.h"
#include "ota_demo.h"

#define CONFIG_BLOCK_SIZE    256U
#define CONFIG_MAX_FILE_SIZE 65536U

static uint8_t downloadedData[ CONFIG_MAX_FILE_SIZE ] = { 0 };

void otaDemo_start( void )
{
    if( isMqttConnected() )
    {
        jobs_startNextPendingJob();
    }
}

// Implemented for use by the MQTT library
void otaDemo_handleIncomingMQTTMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength )
{
    bool handled = jobs_handleIncomingMessage( topic,
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

// TODO: Implement for the Jobs library
void otaDemo_handleJobsStartNextAccepted( JobInfo_t jobInfo )
{
    bool handled = afrOta_parseJobDoc( jobInfo );
}

// Implemented for the AFR OTA library
void otaDemo_handleOtaStart( OtaInfo_t otaInfo )
{
    // TODO: Populate with the actual MQTT Streams API
    uint32_t offset = 0;
    uint32_t blockSize = CONFIG_BLOCK_SIZE;

    for( int i = 0; i < NUMBER_OF_BLOCKS; i++ )
    {
        mqttStreams_getBlock( otaInfo.streamId, i * blockSize, blockSize );
    }
}

// Implemented for the MQTT Streams library
void otaDemo_handleMqttStreamsBlockArrived( MqttStreamDataBlockInfo_t dataBlock )
{
    // TODO: Add guardrails, this is vulnerable to buffer overwrites
    // TODO: How do we know when a block is the final block?
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
    // TODO: Do something with the completed download
    // Start the bootloader
    jobs_reportJobStatusComplete();
}
