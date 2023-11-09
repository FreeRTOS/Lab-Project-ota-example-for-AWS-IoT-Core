/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "mock_MQTTFileDownloader_cbor.h"

#include "MQTTFileDownloader.h"

#define GET_STREAM_REQUEST_BUFFER_SIZE 256U

/* ============================   TEST GLOBALS ============================= */

char * thingName = "thingname";
size_t thingNameLength = strlen("thingname");
char * streamName = "stream-name";
size_t streamNameLength = strlen("stream-name");
char * getStreamTopic = "get-stream-topic";
size_t getStreamTopicLength = strlen("get-stream-topic");

uint8_t uintResult;
size_t requestLength;
/* ===========================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    uintResult = 10; // An unimplimented MQTTFileDownloaderStatus value
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* =============================   CALLBACKS   ============================= */

bool mqtt_subscribe_stream_json_true(char * topic, size_t topicLength, int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/json", topic, strlen("$aws/things/thingname/streams/stream-name/data/json"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/json"), topicLength);
    return true;
}

bool mqtt_subscribe_stream_json_false(char * topic, size_t topicLength, int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/json", topic, strlen("$aws/things/thingname/streams/stream-name/data/json"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/json"), topicLength);
    return false;
}


bool mqtt_subscribe_stream_cbor_true(char * topic, size_t topicLength, int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/cbor", topic, strlen("$aws/things/thingname/streams/stream-name/data/cbor"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/cbor"), topicLength);
    return true;
}

bool mqtt_subscribe_stream_cbor_false(char * topic, size_t topicLength, int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/cbor", topic, strlen("$aws/things/thingname/streams/stream-name/data/cbor"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/cbor"), topicLength);
    return false;
}

bool mqtt_publish_request_json_true(char * topic,
                          size_t topicLength,
                          uint8_t * message,
                          size_t messageLength,
                          int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY(getStreamTopic, topic, getStreamTopicLength);
    TEST_ASSERT_EQUAL_INT(getStreamTopicLength, topicLength);
    TEST_ASSERT_EQUAL_MEMORY("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}", message, strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}"));
    TEST_ASSERT_EQUAL_INT(strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}"), messageLength);
    return true;
}

bool mqtt_publish_request_json_false(char * topic,
                          size_t topicLength,
                          uint8_t * message,
                          size_t messageLength,
                          int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY(getStreamTopic, topic, getStreamTopicLength);
    TEST_ASSERT_EQUAL_INT(getStreamTopicLength, topicLength);
    TEST_ASSERT_EQUAL_MEMORY("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}", message, strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}"));
    TEST_ASSERT_EQUAL_INT(strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}"), messageLength);
    return false;
}

/* ===============================   TESTS   =============================== */

void test_init_returnsSuccess_forJSONDataType( void )
{
    MqttFileDownloaderContext_t context;

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/json", context.topicStreamData, strlen("$aws/things/thingname/streams/stream-name/data/json"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/json"), context.topicStreamDataLength);
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/get/json", context.topicGetStream, strlen("$aws/things/thingname/streams/stream-name/get/json"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/get/json"), context.topicGetStreamLength);
}

void test_init_returnsSuccess_forCBORDataType( void )
{
    MqttFileDownloaderContext_t context;

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_CBOR);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/data/cbor", context.topicStreamData, strlen("$aws/things/thingname/streams/stream-name/data/cbor"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/data/cbor"), context.topicStreamDataLength);
    TEST_ASSERT_EQUAL_MEMORY("$aws/things/thingname/streams/stream-name/get/cbor", context.topicGetStream, strlen("$aws/things/thingname/streams/stream-name/get/cbor"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/streams/stream-name/get/cbor"), context.topicGetStreamLength);
}

void test_init_returnsBadParam_givenNullContext( void )
{
    uintResult = mqttDownloader_init(NULL, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderBadParameter, uintResult);
}

void test_createGetDataBlockRequest_succeedsForJSONDataType( void )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = GET_STREAM_REQUEST_BUFFER_SIZE;

    requestLength = mqttDownloader_createGetDataBlockRequest(DATA_TYPE_JSON, 4U, 3U, 2U, 1U, getStreamRequest, getStreamRequestLength);
    TEST_ASSERT_EQUAL_INT(strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}"), requestLength);
    TEST_ASSERT_EQUAL_MEMORY(getStreamRequest, "{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}", strlen("{\"s\": 1,\"f\": 4,\"l\": 3,\"o\": 2,\"n\": 1}") );
}

void test_createGetDataBlockRequest_succeedsForCBORDataType( void )
{
    char getStreamRequest[ GET_STREAM_REQUEST_BUFFER_SIZE ];
    size_t getStreamRequestLength = GET_STREAM_REQUEST_BUFFER_SIZE;

    char *encodedMessage = "expected-message";
    size_t expectedCborSize = 9999U;

    CBOR_Encode_GetStreamRequestMessage_ExpectAndReturn(getStreamRequest,
        GET_STREAM_REQUEST_BUFFER_SIZE,
        NULL,
        "rdy",
        4,
        3,
        2,
        ( const uint8_t * ) "MQ==",
        strlen( "MQ==" ),
        1,
        true);

    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_encodedMessageSize();
    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_messageBuffer();
    CBOR_Encode_GetStreamRequestMessage_ReturnThruPtr_encodedMessageSize(&expectedCborSize);
    CBOR_Encode_GetStreamRequestMessage_ReturnThruPtr_messageBuffer(encodedMessage);

    requestLength = mqttDownloader_createGetDataBlockRequest(DATA_TYPE_CBOR, 4U, 3U, 2U, 1U, getStreamRequest, getStreamRequestLength);
    TEST_ASSERT_EQUAL(expectedCborSize, requestLength);
    TEST_ASSERT_EQUAL_MEMORY(encodedMessage, "expected-message", strlen(encodedMessage));
}

void test_isDataBlockReceived_returnTrue( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    memcpy(context.topicStreamData, "topic", strlen("topic"));
    context.topicStreamDataLength = strlen("topic");
    TEST_ASSERT_TRUE(mqttDownloader_isDataBlockReceived(&context, "topic", strlen("topic")));
}

void test_isDataBlockReceived_returnsFalse_whenTopicIsDifferent( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    memcpy(context.topicStreamData, "topic", strlen("topic"));
    context.topicStreamDataLength = strlen("topic");
    TEST_ASSERT_FALSE(mqttDownloader_isDataBlockReceived(&context, "different-topic", strlen("topic")));
}

void test_isDataBlockReceived_returnsFalse_whenLengthIsDifferent( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    memcpy(context.topicStreamData, "topic", strlen("topic"));
    context.topicStreamDataLength = strlen("topic");
    TEST_ASSERT_FALSE(mqttDownloader_isDataBlockReceived(&context, "topic", 10));
}

void test_isDataBlockReceived_returnsFalse_whenTopicAndLengthIsDifferent( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    memcpy(context.topicStreamData, "topic", strlen("topic"));
    context.topicStreamDataLength = strlen("topic");
    TEST_ASSERT_FALSE(mqttDownloader_isDataBlockReceived(&context, "completely-different-topic", strlen("completely-different-topic")));
}

void test_processReceivedDataBlock_processesJSONBlock( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.dataType = DATA_TYPE_JSON;

    uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
    size_t dataLength = 0;

    bool result = mqttDownloader_processReceivedDataBlock(&context, "{\"p\": \"dGVzdA==\"}", strlen("{\"p\": \"dGVzdA==\"}"), decodedData, &dataLength);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(4, dataLength);
}

void test_processReceivedDataBlock_invalidJSONBlock( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.dataType = DATA_TYPE_JSON;

    uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
    size_t dataLength = 0;

    bool result = mqttDownloader_processReceivedDataBlock(&context, "{\"wrongKey\": \"dGVzdA==\"}", strlen("{\"wrongKey\": \"dGVzdA==\"}"), decodedData, &dataLength);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, dataLength);
}

void test_processReceivedDataBlock_invalidEncodingJSONBlock( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.dataType = DATA_TYPE_JSON;

    uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
    size_t dataLength = 0;

    bool result = mqttDownloader_processReceivedDataBlock(&context, "{\"p\": \"notEncoded\"}", strlen("{\"p\": \"notEncoded\"}"), decodedData, &dataLength);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, dataLength);
}

void test_processReceivedDataBlock_processesCBORBlock( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.dataType = DATA_TYPE_CBOR;

    char * validCBORMsg = "aValidCBORMsg";
    size_t expectedProcessedDataLength = 12345U;
    uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
    size_t dataLength = 0;

    CBOR_Decode_GetStreamResponseMessage_ExpectAndReturn(validCBORMsg, strlen(validCBORMsg), NULL, NULL, NULL, NULL, NULL, true);
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_fileId();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_blockSize();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_blockId();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_payload();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_payloadSize();
    CBOR_Decode_GetStreamResponseMessage_ReturnThruPtr_payloadSize(&expectedProcessedDataLength);

    bool result = mqttDownloader_processReceivedDataBlock(&context, validCBORMsg, strlen(validCBORMsg), decodedData, &dataLength);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(expectedProcessedDataLength, dataLength);
}

void test_processReceivedDataBlock_invalidCBORBlock( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.dataType = DATA_TYPE_CBOR;

    char * invalidCBORMsg = "invalidCBORMsg";
    size_t notExpectedProcessedDataLength = 12345U;
    uint8_t decodedData[ mqttFileDownloader_CONFIG_BLOCK_SIZE ];
    size_t dataLength = 0;

    CBOR_Decode_GetStreamResponseMessage_ExpectAndReturn(invalidCBORMsg, strlen(invalidCBORMsg), NULL, NULL, NULL, NULL, NULL, false);
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_fileId();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_blockSize();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_blockId();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_payload();
    CBOR_Decode_GetStreamResponseMessage_IgnoreArg_payloadSize();
    CBOR_Decode_GetStreamResponseMessage_ReturnThruPtr_payloadSize(&notExpectedProcessedDataLength);

    bool result = mqttDownloader_processReceivedDataBlock(&context, invalidCBORMsg, strlen(invalidCBORMsg), decodedData, &dataLength);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, dataLength);
}
