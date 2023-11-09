/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * @file MQTTFileDownloader.c
 * @brief MQTT file streaming APIs.
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
                             const char * streamName,
                             size_t streamNameLength,
                             const char * thingName,
                             size_t thingNameLength,
                             const char * apiSuffix );

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
static MQTTFileDownloaderStatus_t handleCborMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  const uint8_t * message,
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
static MQTTFileDownloaderStatus_t handleJsonMessage( uint8_t * decodedData,
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
                             const char * streamName,
                             size_t streamNameLength,
                             const char * thingName,
                             size_t thingNameLength,
                             const char * apiSuffix )
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

    ( void ) memset( streamNameBuff, ( int32_t ) '\0', STREAM_NAME_MAX_LEN + 1U );
    ( void ) memcpy( streamNameBuff, streamName, streamNameLength );

    ( void ) memset( thingNameBuff, ( int32_t ) '\0', MAX_THINGNAME_LEN + 1U );
    ( void ) memcpy( thingNameBuff, thingName, thingNameLength );

    topicParts[ 1 ] = ( const char * ) thingNameBuff;
    topicParts[ 3 ] = ( const char * ) streamNameBuff;
    topicParts[ 4 ] = ( const char * ) apiSuffix;

    topicLen = ( uint16_t ) stringBuilder( topicBuffer,
                                           topicBufferLen,
                                           topicParts );

    return topicLen;
}

MQTTFileDownloaderStatus_t mqttDownloader_init( MqttFileDownloaderContext_t * context,
                                                const char * streamName,
                                                size_t streamNameLength,
                                                const char * thingName,
                                                size_t thingNameLength,
                                                DataType_t dataType )
{
    const char * streamDataApiSuffix = NULL;
    const char * getStreamApiSuffix = NULL;
    MQTTFileDownloaderStatus_t initStatus = MQTTFileDownloaderSuccess;

    if ( ( streamName == NULL ) || ( streamNameLength == 0 ) ||
         ( thingName == NULL) || (thingNameLength == 0) || ( context == NULL ))
    {
        initStatus = MQTTFileDownloaderBadParameter;
    }

    if( initStatus == MQTTFileDownloaderSuccess )
    {
        /* Initializing MQTT File Downloader context */
        ( void ) memset( context->topicStreamData, ( int32_t ) '\0', TOPIC_STREAM_DATA_BUFFER_SIZE );
        ( void ) memset( context->topicGetStream, ( int32_t ) '\0', TOPIC_GET_STREAM_BUFFER_SIZE );
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
        if( context->topicStreamDataLength == 0U )
        {
            initStatus = MQTTFileDownloaderInitFailed;
        }

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
        if( context->topicGetStreamLength == 0U )
        {
            initStatus = MQTTFileDownloaderInitFailed;
        }

    }

    return initStatus;
}

size_t mqttDownloader_createGetDataBlockRequest(
    DataType_t dataType,
    uint16_t fileId,
    uint32_t blockSize,
    uint16_t blockOffset,
    uint32_t numberOfBlocksRequested,
    char * getStreamRequest,
    size_t getStreamRequestLength )
{
    size_t requestLength = 0U;
    /*
     * Get stream request format
     *
     *   "{ \"s\" : 1, \"f\": 1, \"l\": 256, \"o\": 0, \"n\": 1 }";
     */
    if (( getStreamRequestLength >= GET_STREAM_REQUEST_BUFFER_SIZE ) &&
        ( getStreamRequest != NULL ) )
    {
        ( void ) memset( getStreamRequest, ( int32_t ) '\0', GET_STREAM_REQUEST_BUFFER_SIZE );

        if( dataType == DATA_TYPE_JSON )
        {

            /* coverity[misra_c_2012_rule_21_6_violation] */
            ( void ) snprintf( getStreamRequest,
                    GET_STREAM_REQUEST_BUFFER_SIZE,
                    "{"
                    "\"s\": 1,"
                    "\"f\": %u,"
                    "\"l\": %lu,"
                    "\"o\": %u,"
                    "\"n\": %lu"
                    "}",
                    fileId,
                    blockSize,
                    blockOffset,
                    numberOfBlocksRequested );

            requestLength = strnlen( getStreamRequest,
                                          GET_STREAM_REQUEST_BUFFER_SIZE );
        }
        else
        {
            ( void ) CBOR_Encode_GetStreamRequestMessage( ( uint8_t * ) getStreamRequest,
                                                GET_STREAM_REQUEST_BUFFER_SIZE,
                                                &requestLength,
                                                "rdy",
                                                fileId,
                                                blockSize,
                                                blockOffset,
                                                /* coverity[misra_c_2012_rule_7_4_violation] */
                                                ( const uint8_t * ) "MQ==",
                                                strlen( "MQ==" ),
                                                numberOfBlocksRequested );
        }
    }
    return requestLength;
}

static MQTTFileDownloaderStatus_t handleCborMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  const uint8_t * message,
                                  size_t messageLength )
{
    bool cborDecodeRet = false;
    int32_t fileId = 0;
    int32_t blockId = 0;
    int32_t blockSize = 0;
    uint8_t * payload = decodedData;
    size_t payloadSize = mqttFileDownloader_CONFIG_BLOCK_SIZE;
    MQTTFileDownloaderStatus_t handleStatus = MQTTFileDownloaderSuccess;

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
        handleStatus = MQTTFileDownloaderDataDecodingFailed;
    }

    return handleStatus;
}

static MQTTFileDownloaderStatus_t handleJsonMessage( uint8_t * decodedData,
                                  size_t * decodedDataLength,
                                  uint8_t * message,
                                  size_t messageLength )
{
    const char dataQuery[] = "p";
    size_t dataQueryLength = sizeof( dataQuery ) - 1U;
    char * dataValue;
    size_t dataValueLength;
    JSONStatus_t result = JSONSuccess;
    MQTTFileDownloaderStatus_t handleStatus = MQTTFileDownloaderSuccess;

    Base64Status_t base64Status = Base64Success;

    result = JSON_Search( ( char * ) message,
                          messageLength,
                          dataQuery,
                          dataQueryLength,
                          &dataValue,
                          &dataValueLength );

    if( result != JSONSuccess )
    {
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
            handleStatus = MQTTFileDownloaderDataDecodingFailed;
        }
    }

    return handleStatus;
}

MQTTFileDownloaderStatus_t mqttDownloader_isDataBlockReceived( const MqttFileDownloaderContext_t * context,
                                                               const char * topic,
                                                               size_t topicLength )
{
    MQTTFileDownloaderStatus_t status = MQTTFileDownloaderFailure;

    if( ( topic == NULL ) || ( topicLength == 0 ) )
    {
        status = MQTTFileDownloaderBadParameter;
    }
    else if( ( topicLength == context->topicStreamDataLength ) &&
        ( 0 == strncmp( context->topicStreamData, topic, topicLength ) ) )
    {
        status = MQTTFileDownloaderSuccess;
    }
    else
    {
        /* Empty MISRA body */
    }

    return status;
}

MQTTFileDownloaderStatus_t mqttDownloader_processReceivedDataBlock(
    const MqttFileDownloaderContext_t * context,
    uint8_t * message,
    size_t messageLength,
    uint8_t * data,
    size_t * dataLength )
{
    MQTTFileDownloaderStatus_t decodingStatus = MQTTFileDownloaderFailure;
    if ( ( message != NULL ) && ( messageLength != 0 ) && ( data != NULL ) && ( dataLength != 0 ))
    {
        ( void ) memset( data, ( int32_t ) '\0', mqttFileDownloader_CONFIG_BLOCK_SIZE );
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
    }

    return decodingStatus;
}
