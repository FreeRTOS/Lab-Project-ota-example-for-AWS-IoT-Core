/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

/* Standard includes. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MQTTFileDownloader.h"
#include "MQTTFileDownloader_base64.h"
#include "MQTTFileDownloader_cbor.h"
#include "core_json.h"
#include "mqtt_wrapper.h"

#define GET_STREAM_REQUEST_BUFFER_SIZE 256U

/**
 * @brief Build a string from a set of strings
 *
 * @param[in] buffer Buffer to place the output string in.
 * @param[in] bufferSizeBytes Size of the buffer pointed to by buffer.
 * @param[in] strings NULL-terminated array of string pointers.
 * @return size_t Length of the output string, not including the terminator.
 */
static size_t stringBuilder( char * buffer,
                             size_t bufferSizeBytes,
                             const char * const strings[] );

/**
 * @brief Create MQTT topics from common substrings and input variables.
 *
 * @param[out] topicBuffer Buffer to place the output topic in.
 * @param[in] topicBufferLen Length of topicBuffer.
 * @param[in] streamName Name of the MQTT stream.
 * @param[in] streamNameLength Length of the MQTT stream name.
 * @param[in] thingName Name of the thing.
 * @param[in] thingNameLength Length of the thing name of the Device.
 * @param[in] apiSuffix Last part of the MQTT topic.
 *
 * @return uint16_t Length of the MQTT topic, not including the terminator.
 */
static uint16_t createTopic( char * topicBuffer,
                             size_t topicBufferLen,
                             char * streamName,
                             size_t streamNameLength,
                             char * thingName,
                             size_t thingNameLength,
                             char * apiSuffix );

/**
 * @brief Handles and decodes the received message in CBOR format.
 *
 * @param[in] message Incoming MQTT message received.
 * @param[in] messageLength Length of the MQTT message received.
 * @param[out] decodedData Buffer to place the decoded data in.
 * @param[out] decodedDataLength Length of decoded data.
 *
 * @return uint8_t returns appropriate MQTT File Downloader Status.
 */
static uint8_t handleCborMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  uint8_t * message,
                                  size_t messageLength );

/**
 * @brief Handles and decodes the received message in JSON format.
 *
 * @param[in] message Incoming MQTT message received.
 * @param[in] messageLength Length of the MQTT message received.
 * @param[out] decodedData Buffer to place the decoded data in.
 * @param[out] decodedDataLength Length of decoded data.
 *
 * @return uint8_t returns appropriate MQTT File Downloader Status.
 */
static uint8_t handleJsonMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  uint8_t * message,
                                  size_t messageLength );

static size_t stringBuilder( char * buffer,
                             size_t bufferSizeBytes,
                             const char * const strings[] )
{
    size_t curLen = 0;
    size_t i;
    size_t thisLength = 0;

    buffer[ 0 ] = '\0';

    for( i = 0; strings[ i ] != NULL; i++ )
    {
        thisLength = strlen( strings[ i ] );

        if( ( thisLength + curLen + 1U ) > bufferSizeBytes )
        {
            curLen = 0;
            break;
        }

        ( void ) strncat( buffer, strings[ i ], bufferSizeBytes - curLen - 1U );
        curLen += thisLength;
    }

    buffer[ curLen ] = '\0';

    return curLen;
}

static uint16_t createTopic( char * topicBuffer,
                             size_t topicBufferLen,
                             char * streamName,
                             size_t streamNameLength,
                             char * thingName,
                             size_t thingNameLength,
                             char * apiSuffix )
{
    uint16_t topicLen = 0;
    char streamNameBuff[ STREAM_NAME_MAX_LEN + 1 ];
    char thingNameBuff[ MAX_THINGNAME_LEN + 1 ];

    /* NULL-terminated list of topic string parts. */
    const char * topicParts[] = { MQTT_API_THINGS,
                                  NULL, /* Thing Name not available at compile
                                           time, initialized below. */
                                  MQTT_API_STREAMS,
                                  NULL, /* Stream Name not available at compile
                                           time, initialized below.*/
                                  NULL,
                                  NULL };

    memset( streamNameBuff, '\0', STREAM_NAME_MAX_LEN + 1 );
    memcpy( streamNameBuff, streamName, streamNameLength );

    memset( thingNameBuff, '\0', MAX_THINGNAME_LEN + 1 );
    memcpy( thingNameBuff, thingName, thingNameLength );

    topicParts[ 1 ] = ( const char * ) thingNameBuff;
    topicParts[ 3 ] = ( const char * ) streamNameBuff;
    topicParts[ 4 ] = ( const char * ) apiSuffix;

    topicLen = ( uint16_t ) stringBuilder( topicBuffer,
                                           topicBufferLen,
                                           topicParts );

    return topicLen;
}

/* TODO - Defend agains NULL streamName or thingName and 0 lengths*/
uint8_t mqttDownloader_init( MqttFileDownloaderContext_t * context,
                             char * streamName,
                             size_t streamNameLength,
                             char * thingName,
                             size_t thingNameLength,
                             uint8_t dataType )
{
    char * streamDataApiSuffix = NULL;
    char * getStreamApiSuffix = NULL;
    bool subscribeStatus = false;
    uint8_t initStatus = MQTTFileDownloaderSuccess;

    if( context == NULL )
    {
        initStatus = MQTTFileDownloaderBadParameter;
    }

    if( initStatus == MQTTFileDownloaderSuccess )
    {
        /* Initializing MQTT File Downloader context */
        memset( context->topicStreamData, '\0', TOPIC_STREAM_DATA_BUFFER_SIZE );
        memset( context->topicGetStream, '\0', TOPIC_GET_STREAM_BUFFER_SIZE );
        context->topicStreamDataLength = 0U;
        context->topicGetStreamLength = 0U;
        context->dataType = dataType;

        if( dataType == DATA_TYPE_JSON )
        {
            streamDataApiSuffix = MQTT_API_DATA_JSON;
        }
        else
        {
            streamDataApiSuffix = MQTT_API_DATA_CBOR;
        }

        context->topicStreamDataLength = createTopic(
            context->topicStreamData,
            TOPIC_STREAM_DATA_BUFFER_SIZE,
            streamName,
            streamNameLength,
            thingName,
            thingNameLength,
            streamDataApiSuffix );
        if( context->topicStreamDataLength == 0 )
        {
            initStatus = MQTTFileDownloaderInitFailed;
        }

        printf( "Data topic is %s\n", context->topicStreamData );
    }

    if( initStatus == MQTTFileDownloaderSuccess )
    {
        if( dataType == DATA_TYPE_JSON )
        {
            getStreamApiSuffix = MQTT_API_GET_JSON;
        }
        else
        {
            getStreamApiSuffix = MQTT_API_GET_CBOR;
        }

        context
            ->topicGetStreamLength = createTopic( context->topicGetStream,
                                                  TOPIC_GET_STREAM_BUFFER_SIZE,
                                                  streamName,
                                                  streamNameLength,
                                                  thingName,
                                                  thingNameLength,
                                                  getStreamApiSuffix );
        if( context->topicGetStreamLength == 0 )
        {
            initStatus = MQTTFileDownloaderInitFailed;
        }

        printf( "Get stream topic is %s\n", context->topicGetStream );
    }

    if( initStatus == MQTTFileDownloaderSuccess )
    {
        subscribeStatus = mqttWrapper_subscribe( context->topicStreamData,
                                                 context
                                                     ->topicStreamDataLength );

        initStatus = subscribeStatus ? MQTTFileDownloaderSuccess
                                     : MQTTFileDownloaderPublishFailed;
    }

    return initStatus;
}

uint8_t mqttDownloader_requestDataBlock( MqttFileDownloaderContext_t * context,
                                         uint16_t fileId,
                                         uint32_t blockSize,
                                         uint16_t blockOffset,
                                         uint32_t numberOfBlocksRequested )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = 0U;
    bool publishStatus = false;
    uint8_t requestStatus = MQTTFileDownloaderSuccess;

    memset( getStreamRequest, '\0', GET_STREAM_REQUEST_BUFFER_SIZE );

    if( context == NULL )
    {
        requestStatus = MQTTFileDownloaderBadParameter;
    }
    else if( ( context->topicStreamDataLength == 0 ) ||
        ( context->topicGetStreamLength == 0 ) )
    {
        requestStatus = MQTTFileDownloaderNotInitialized;
    }

    if( requestStatus == MQTTFileDownloaderSuccess )
    {
        /*
         * Get stream request format
         *
         *   "{ \"s\" : 1, \"f\": 1, \"l\": 256, \"o\": 0, \"n\": 1 }";
         */
        if( context->dataType == DATA_TYPE_JSON )
        {
            snprintf( getStreamRequest,
                      GET_STREAM_REQUEST_BUFFER_SIZE,
                      "{"
                      "\"s\": 1,"
                      "\"f\": %u,"
                      "\"l\": %u,"
                      "\"o\": %u,"
                      "\"n\": %u"
                      "}",
                      fileId,
                      blockSize,
                      blockOffset,
                      numberOfBlocksRequested );

            getStreamRequestLength = strnlen( getStreamRequest,
                                              GET_STREAM_REQUEST_BUFFER_SIZE );
        }
        else
        {
            size_t encodedMessageSize = 0;

            CBOR_Encode_GetStreamRequestMessage( ( uint8_t * ) getStreamRequest,
                                                 GET_STREAM_REQUEST_BUFFER_SIZE,
                                                 &encodedMessageSize,
                                                 "rdy",
                                                 fileId,
                                                 blockSize,
                                                 blockOffset,
                                                 ( const uint8_t * ) "MQ==",
                                                 strlen( "MQ==" ),
                                                 numberOfBlocksRequested );

            getStreamRequestLength = encodedMessageSize;
        }
    }

    if( requestStatus == MQTTFileDownloaderSuccess )
    {
        publishStatus = mqttWrapper_publish( context->topicGetStream,
                                             context->topicGetStreamLength,
                                             ( uint8_t * ) getStreamRequest,
                                             getStreamRequestLength );

        requestStatus = publishStatus ? MQTTFileDownloaderSuccess
                                      : MQTTFileDownloaderPublishFailed;
    }

    return requestStatus;
}

static uint8_t handleCborMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  uint8_t * message,
                                  size_t messageLength )
{
    bool cborDecodeRet = false;
    int32_t fileId = 0;
    int32_t blockId = 0;
    int32_t blockSize = 0;
    uint8_t * payload = decodedData;
    size_t payloadSize = mqttFileDownloader_CONFIG_BLOCK_SIZE;
    uint8_t handleStatus = MQTTFileDownloaderSuccess;

    cborDecodeRet = CBOR_Decode_GetStreamResponseMessage( message,
                                                          messageLength,
                                                          &fileId,
                                                          &blockId,
                                                          &blockSize,
                                                          &payload,
                                                          &payloadSize );

    if( cborDecodeRet )
    {
        *decodedDataLength = payloadSize;
    }
    else
    {
        printf( "Failed to decode CBOR data." );
        handleStatus = MQTTFileDownloaderDataDecodingFailed;
    }

    return handleStatus;
}

static uint8_t handleJsonMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  uint8_t * message,
                                  size_t messageLength )
{
    char dataQuery[] = "p";
    size_t dataQueryLength = sizeof( dataQuery ) - 1;
    char * dataValue;
    size_t dataValueLength;
    JSONStatus_t result = JSONSuccess;
    uint8_t handleStatus = MQTTFileDownloaderSuccess;

    Base64Status_t base64Status = Base64Success;

    result = JSON_Search( ( char * ) message,
                          messageLength,
                          dataQuery,
                          dataQueryLength,
                          &dataValue,
                          &dataValueLength );

    if( result != JSONSuccess )
    {
        printf( "Failed to parse JSON data." );
        handleStatus = MQTTFileDownloaderDataDecodingFailed;
    }

    if( handleStatus == MQTTFileDownloaderSuccess )
    {
        base64Status = base64_Decode( decodedData,
                                      mqttFileDownloader_CONFIG_BLOCK_SIZE,
                                      decodedDataLength,
                                      ( const uint8_t * ) dataValue,
                                      dataValueLength );

        if( base64Status != Base64Success )
        {
            printf( "Failed to decode Base64 data. Error code =%d",
                    ( int ) base64Status );
            handleStatus = MQTTFileDownloaderDataDecodingFailed;
        }
    }

    return handleStatus;
}

/* TODO - Handle NULL topic or zero topic length */
bool mqttDownloader_isDataBlockReceived( MqttFileDownloaderContext_t * context,
                                         char * topic,
                                         size_t topicLength )
{
    bool handled = false;

    if( ( topicLength == context->topicStreamDataLength ) &&
        ( 0 == strncmp( context->topicStreamData, topic, topicLength ) ) )
    {
        handled = true;
    }
    else
    {
        handled = false;
        printf( "Incoming Publish Topic Name: %s does not match subscribed "
                "topic.\r\n",
                topic );
    }

    return handled;
}

bool mqttDownloader_processReceivedDataBlock(
    MqttFileDownloaderContext_t * context,
    uint8_t * message,
    size_t messageLength,
    uint8_t * data,
    size_t * dataLength )
{
    bool processed = false;
    uint8_t decodingStatus = MQTTFileDownloaderSuccess;

    printf( "MQTT streams handling incoming message \n" );
    printf( "Incoming data block %.*s\n", ( int ) messageLength, message );

    memset( data, '\0', mqttFileDownloader_CONFIG_BLOCK_SIZE );

    if( context->dataType == DATA_TYPE_JSON )
    {
        decodingStatus = handleJsonMessage( data,
                                            dataLength,
                                            message,
                                            messageLength );
    }
    else
    {
        decodingStatus = handleCborMessage( data,
                                            dataLength,
                                            message,
                                            messageLength );
    }

    if( decodingStatus != MQTTFileDownloaderSuccess )
    {
        printf( "Failed to decode the data received \n" );
    }
    else
    {
        processed = true;
    }

    return processed;
}
