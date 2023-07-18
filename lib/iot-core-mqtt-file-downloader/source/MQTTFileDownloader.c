/*
License Info
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

/* Logging configuration for the library. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME "MqttFileDownloader"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL LOG_DEBUG
#endif

/**
 * @brief The number of topic filters to subscribe.
 */
#define mqttFileDownloaderSubscribeTOPIC_COUNT    ( 1 )

/**
 * @brief The number of topic filters to subscribe.
 */
#define mqttFileDownloaderSubscribeRETRY_COUNT    ( 5 )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define mqttFileDownloaderPROCESS_LOOP_TIMEOUT_MS ( 500U )

/**
 * @brief Build a string from a set of strings
 *
 * @param[in] pBuffer Buffer to place the output string in.
 * @param[in] bufferSizeBytes Size of the buffer pointed to by pBuffer.
 * @param[in] strings NULL-terminated array of string pointers.
 * @return size_t Length of the output string, not including the terminator.
 */
static size_t stringBuilder( char * pBuffer,
                             size_t bufferSizeBytes,
                             const char * const strings[] );

/**
 *  @brief Topic strings used by the MQTT downloader.
 *
 * These first few are topic extensions to the dynamic base topic that includes
 * the Thing name.
 */
#define MQTT_API_THINGS    "$aws/things/" /*!< Topic prefix for thing APIs. */
#define MQTT_API_STREAMS   "/streams/"    /*!< Stream API identifier. */
#define MQTT_API_DATA_CBOR "/data/cbor"   /*!< Stream API suffix. */
#define MQTT_API_GET_CBOR  "/get/cbor"    /*!< Stream API suffix. */
#define MQTT_API_DATA_JSON "/data/json"   /*!< JSON DATA Stream API suffix. */
#define MQTT_API_GET_JSON  "/get/json"    /*!< JSON GET Stream API suffix. */

/* NOTE: The format specifiers in this string are placeholders only; the lengths
 * of these strings are used to calculate buffer sizes.
 */
/* Reserved for CBOR data type*/
// static const char pCborStreamDataTopicTemplate[] = MQTT_API_THINGS
// "%s"MQTT_API_STREAMS "%s"MQTT_API_DATA_CBOR; /*!< Topic template to receive
// data over a stream. */ static const char pCborGetStreamTopicTemplate[] =
// MQTT_API_THINGS "%s"MQTT_API_STREAMS "%s"MQTT_API_GET_CBOR;   /*!< Topic
// template to request next data over a stream. */
static const char pJsonStreamDataTopicTemplate[] = MQTT_API_THINGS
    "%s" MQTT_API_STREAMS
    "%s" MQTT_API_DATA_JSON; /*!< Topic template to receive data over a stream.
                              */
static const char pJsonGetStreamTopicTemplate[] = MQTT_API_THINGS
    "%s" MQTT_API_STREAMS "%s" MQTT_API_GET_JSON; /*!< Topic template to request
                                                     next data over a stream. */

/* Maximum lengths for constants used in the ota_mqtt_topic_strings templates.
 * These are used to calculate the static size of buffers used to store MQTT
 * topic and message strings. Each length is in terms of bytes. */
#define U32_MAX_LEN \
    10U  /*!< Maximum number of output digits of an unsigned long value. */
#define JOB_NAME_MAX_LEN                                                    \
    128U /*!< Maximum length of the name of job documents received from the \
            server. */
#define STREAM_NAME_MAX_LEN \
    44U  /*!< Maximum length for the name of MQTT streams. */
#define NULL_CHAR_LEN     1U /*!< Size of a single null character use */
#define MAX_THINGNAME_LEN 128U

#define CONST_STRLEN( s ) ( ( ( uint32_t ) sizeof( s ) ) - 1UL )
#define TOPIC_PLUS_THINGNAME_LEN( topic ) \
    ( CONST_STRLEN( topic ) + MAX_THINGNAME_LEN + NULL_CHAR_LEN )
#define TOPIC_JSON_STREAM_DATA_BUFFER_SIZE                       \
    ( TOPIC_PLUS_THINGNAME_LEN( pJsonStreamDataTopicTemplate ) + \
      STREAM_NAME_MAX_LEN ) /*!< Max buffer size for             \
                               `streams/<stream_name>/data/cbor` topic. */
#define TOPIC_JSON_GET_STREAM_BUFFER_SIZE                       \
    ( TOPIC_PLUS_THINGNAME_LEN( pJsonGetStreamTopicTemplate ) + \
      STREAM_NAME_MAX_LEN )

#define GET_STREAM_REQUEST_BUFFER_SIZE 128U

/*
 * Buffer to store the thing name.
 */
static char pMqttDownloaderThingName[ MAX_THINGNAME_LEN ];

/*
 * Buffer to store the name of the data stream.
 */
static char pMqttDownloaderStreamName[ STREAM_NAME_MAX_LEN ];

/*
 * Buffer to store the topic generated for requesting data stream.
 */
static char pRxStreamTopic[ TOPIC_JSON_STREAM_DATA_BUFFER_SIZE ];

/*
 * Buffer to store the topic generated for requesting data stream.
 */
static char pGetStreamTopic[ TOPIC_JSON_GET_STREAM_BUFFER_SIZE ];
/*
 * MqttDownloader data type configured.
 */
static uint8_t ucMqttDownloaderDataType;

static size_t stringBuilder( char * pBuffer,
                             size_t bufferSizeBytes,
                             const char * const strings[] )
{
    size_t curLen = 0;
    size_t i;
    size_t thisLength = 0;

    pBuffer[ 0 ] = '\0';

    for( i = 0; strings[ i ] != NULL; i++ )
    {
        thisLength = strlen( strings[ i ] );

        /* Assert if there is not enough buffer space. */

        assert( ( thisLength + curLen + 1U ) <= bufferSizeBytes );

        ( void ) strncat( pBuffer,
                          strings[ i ],
                          bufferSizeBytes - curLen - 1U );
        curLen += thisLength;
    }

    pBuffer[ curLen ] = '\0';

    return curLen;
}

uint8_t mqttDownloader_init( char * pStreamName,
                             size_t streamNameLength,
                             char * pThingName,
                             uint8_t ucDataType )
{
    uint16_t topicLen = 0;

    memset( pMqttDownloaderThingName, '\0', MAX_THINGNAME_LEN );
    memcpy( pMqttDownloaderThingName, pThingName, strlen( pThingName ) );

    memset( pMqttDownloaderStreamName, '\0', STREAM_NAME_MAX_LEN );
    memcpy( pMqttDownloaderStreamName, pStreamName, streamNameLength );

    ucMqttDownloaderDataType = ucDataType;

    /* Initializing DATA topic name */

    memset( pRxStreamTopic, '\0', TOPIC_JSON_STREAM_DATA_BUFFER_SIZE );

    /* NULL-terminated list of topic string parts. */
    const char * pDataTopicParts[] = {
        MQTT_API_THINGS,
        NULL, /* Thing Name not available at compile time, initialized below. */
        MQTT_API_STREAMS,
        NULL, /* Stream Name not available at compile time, initialized below.
               */
        NULL,
        NULL
    };

    pDataTopicParts[ 1 ] = ( const char * ) pMqttDownloaderThingName;
    pDataTopicParts[ 3 ] = ( const char * ) pMqttDownloaderStreamName;

    if( ucDataType == DATA_TYPE_JSON )
    {
        pDataTopicParts[ 4 ] = MQTT_API_DATA_JSON;
    }
    else
    {
        pDataTopicParts[ 4 ] = MQTT_API_DATA_CBOR;
    }

    topicLen = ( uint16_t ) stringBuilder( pRxStreamTopic,
                                           sizeof( pRxStreamTopic ),
                                           pDataTopicParts );

    printf( "Data topic is %s\n", pRxStreamTopic );

    assert( ( topicLen > 0U ) && ( topicLen < sizeof( pRxStreamTopic ) ) );

    /* Initializing Get Stream topic name */
    topicLen = 0;
    memset( pGetStreamTopic, '\0', TOPIC_JSON_GET_STREAM_BUFFER_SIZE );

    const char * pGetStreamTopicParts[] = {
        MQTT_API_THINGS,
        NULL, /* Thing Name not available at compile time, initialized below. */
        MQTT_API_STREAMS,
        NULL, /* Stream Name not available at compile time, initialized below.
               */
        NULL,
        NULL
    };

    pGetStreamTopicParts[ 1 ] = ( const char * ) pMqttDownloaderThingName;
    pGetStreamTopicParts[ 3 ] = ( const char * ) pMqttDownloaderStreamName;

    if( ucDataType == DATA_TYPE_JSON )
    {
        pGetStreamTopicParts[ 4 ] = MQTT_API_GET_JSON;
    }
    else
    {
        pGetStreamTopicParts[ 4 ] = MQTT_API_GET_CBOR;
    }

    topicLen = ( uint16_t ) stringBuilder( pGetStreamTopic,
                                           sizeof( pGetStreamTopic ),
                                           pGetStreamTopicParts );

    printf( "Get Stream topic is %s\n", pGetStreamTopic );

    assert( ( topicLen > 0U ) && ( topicLen < sizeof( pGetStreamTopic ) ) );

    mqttWrapper_subscribe( pRxStreamTopic, topicLen );

    return 0;
}

uint8_t mqttDownloader_requestDataBlock( uint16_t usFileId,
                                         uint32_t ulBlockSize,
                                         uint16_t usBlockOffset,
                                         uint32_t ulNumberOfBlocksRequested )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];

    memset( getStreamRequest, '\0', GET_STREAM_REQUEST_BUFFER_SIZE );

    /*
     * Get stream request format
     *
     *   "{ \"s\" : 1, \"f\": 1, \"l\": 256, \"o\": 0, \"n\": 1 }";
     */
    if( ucMqttDownloaderDataType == DATA_TYPE_JSON )
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
                  usFileId,
                  ulBlockSize,
                  usBlockOffset,
                  ulNumberOfBlocksRequested );
        mqttWrapper_publish( pGetStreamTopic,
                             strlen( pGetStreamTopic ),
                             ( uint8_t * ) getStreamRequest,
                             strlen( getStreamRequest ) );
    }
    else
    {
        size_t encodedMessageSize = 0;

        OTA_CBOR_Encode_GetStreamRequestMessage( ( uint8_t * ) getStreamRequest,
                                                 GET_STREAM_REQUEST_BUFFER_SIZE,
                                                 &encodedMessageSize,
                                                 "rdy",
                                                 usFileId,
                                                 ulBlockSize,
                                                 usBlockOffset,
                                                 ( const uint8_t * ) "MQ==",
                                                 strlen( "MQ==" ),
                                                 ulNumberOfBlocksRequested );
        mqttWrapper_publish( pGetStreamTopic,
                             strlen( pGetStreamTopic ),
                             ( uint8_t * ) getStreamRequest,
                             encodedMessageSize );
    }

    return 0;
}

bool mqttDownloader_handleIncomingMessage( MqttFileBlockHandler_t blockCallback,
                                        char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength )
{
    /* Process incoming Publish. */
    printf( "MQTT streams handling incoming message \n" );

    /* Verify the received publish is for the we have subscribed to. */
    if( ( topicLength == strlen( pRxStreamTopic ) ) &&
        ( 0 == strncmp( pRxStreamTopic, topic, topicLength ) ) )
    {
        printf( "Incoming Publish Topic Length: %lu Name: %.*s matches "
                "subscribed topic.\r\n"
                "Incoming Publish Message length: %lu Message: %.*s\r\n",
                topicLength,
                ( int ) topicLength,
                topic,
                messageLength,
                ( int ) messageLength,
                ( char * ) message );

        char decodedData[ 1024 ];
        size_t decodedDataLength = 0;
        memset( decodedData, '\0', sizeof( char ) * 1024 );
        MqttFileDownloaderDataBlockInfo_t dataBlock;

        if( ucMqttDownloaderDataType == DATA_TYPE_JSON )
        {
            JSONStatus_t result;
            char dataQuery[] = "p";
            size_t dataQueryLength = sizeof( dataQuery ) - 1;
            char * dataValue;
            size_t dataValueLength;

            result = JSON_Search( ( char * ) message,
                                  messageLength,
                                  dataQuery,
                                  dataQueryLength,
                                  &dataValue,
                                  &dataValueLength );

            if( result == JSONSuccess )
            {
                printf( "Found: %s -> %s\n", dataQuery, dataValue );

                Base64Status_t base64Status = Base64Success;

                base64Status = base64_Decode( ( uint8_t * ) decodedData,
                                             1024,
                                             &decodedDataLength,
                                             ( const uint8_t * ) dataValue,
                                             dataValueLength );

                if( base64Status != Base64Success )
                {
                    /* Stop processing on error. */
                    printf( "Failed to decode Base64 data: "
                            "base64_Decode returned error: "
                            "error=%d",
                            ( int ) base64Status );
                    return true;
                }
                else
                {
                    printf( "Data value length: %lu actual len: %lu \n",
                            dataValueLength,
                            decodedDataLength );
                    printf( "Extracted: [ %s ]\n", decodedData );
                    dataBlock.payload = ( uint8_t * ) decodedData;
                    dataBlock.payloadLength = decodedDataLength;
                }
            }
        }
        else
        {
            int32_t pFileId = 0;
            int32_t pBlockId = 0;
            int32_t pBlockSize = 0;
            uint8_t * pPayload = ( uint8_t * ) decodedData;
            size_t pPayloadSize = 1024;
            bool cborDecodeRet = false;
            cborDecodeRet = OTA_CBOR_Decode_GetStreamResponseMessage(
                message,
                messageLength,
                &pFileId,
                &pBlockId, /* CBOR requires pointer to int and our block indices
                              never exceed 31 bits. */
                &pBlockSize, /* CBOR requires pointer to int and our block sizes
                                never exceed 31 bits. */
                &pPayload,   /* This payload gets malloc'd by
                                OTA_CBOR_Decode_GetStreamResponseMessage(). We
                                must free it. */
                &pPayloadSize );

            if( cborDecodeRet == true )
            {
                // printf("CBor: payload length: %lu\n", pPayloadSize);
                dataBlock.payload = ( uint8_t * ) pPayload;
                dataBlock.payloadLength = pPayloadSize;
            }
            else
            {
                printf( "Decoding CBOR failed \n" );
            }
        }

        blockCallback( &dataBlock );
    }
    else
    {
        printf( "Incoming Publish Topic Name: %s does not match subscribed "
                "topic.\r\n",
                topic );
        return false;
    }

    return true;
}
