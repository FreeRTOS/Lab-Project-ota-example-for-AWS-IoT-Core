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

#include "mock_mqtt_wrapper.h"
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
    mqttWrapper_subscribe_Stub(mqtt_subscribe_stream_json_true);

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
}

void test_init_returnsSuccess_forCBORDataType( void )
{
    MqttFileDownloaderContext_t context;
    mqttWrapper_subscribe_Stub(mqtt_subscribe_stream_cbor_true);

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_CBOR);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
}

void test_init_returnsBadParam_givenNullContext( void )
{
    uintResult = mqttDownloader_init(NULL, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderBadParameter, uintResult);
}

void test_init_returnsPublishFailure_whenSubscribeFails_forJSONDataType( void )
{
    MqttFileDownloaderContext_t context;
    mqttWrapper_subscribe_Stub(mqtt_subscribe_stream_json_false);

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderPublishFailed, uintResult);
}

void test_init_returnsPublishFailure_whenSubscribeFails_forCBORDataType( void )
{
    MqttFileDownloaderContext_t context;
    mqttWrapper_subscribe_Stub(mqtt_subscribe_stream_cbor_false);

    uintResult = mqttDownloader_init(&context, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_CBOR);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderPublishFailed, uintResult);
}

void test_requestDataBlock_succeedsForJSONDataType( void )
{
    MqttFileDownloaderContext_t context;
    context.topicStreamDataLength = 3;
    memcpy(context.topicGetStream, getStreamTopic, getStreamTopicLength);
    context.topicGetStreamLength = getStreamTopicLength;
    context.dataType = DATA_TYPE_JSON;

    mqttWrapper_publish_Stub(mqtt_publish_request_json_true);

    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
}

void test_requestDataBlock_succeedsForCBORDataType( void )
{
    MqttFileDownloaderContext_t context;
    context.topicStreamDataLength = 3;
    memcpy(context.topicGetStream, getStreamTopic, getStreamTopicLength);
    context.topicGetStreamLength = getStreamTopicLength;
    context.dataType = DATA_TYPE_CBOR;

    char * expectedCborMsg = "expected-cbor-request-msg";
    CBOR_Encode_GetStreamRequestMessage_ExpectAndReturn(NULL,
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
    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_messageBuffer();
    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_encodedMessageSize();
    CBOR_Encode_GetStreamRequestMessage_ReturnThruPtr_messageBuffer(expectedCborMsg);

    mqttWrapper_publish_ExpectAndReturn(context.topicGetStream, context.topicGetStreamLength, expectedCborMsg, 0, true);

    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderSuccess, uintResult);
}

void test_requestDataBlock_returnsPublishFailed_forJsonRequest( void )
{
    MqttFileDownloaderContext_t context;
    context.topicStreamDataLength = 3;
    memcpy(context.topicGetStream, getStreamTopic, getStreamTopicLength);
    context.topicGetStreamLength = getStreamTopicLength;
    context.dataType = DATA_TYPE_JSON;

    mqttWrapper_publish_Stub(mqtt_publish_request_json_false);

    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderPublishFailed, uintResult);
}

void test_requestDataBlock_returnsPublishFailed_forCBORRequest( void )
{
    MqttFileDownloaderContext_t context;
    context.topicStreamDataLength = 3;
    memcpy(context.topicGetStream, getStreamTopic, getStreamTopicLength);
    context.topicGetStreamLength = getStreamTopicLength;
    context.dataType = DATA_TYPE_CBOR;

    char * expectedCborMsg = "expected-cbor-request-msg";
    CBOR_Encode_GetStreamRequestMessage_ExpectAndReturn(NULL,
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
    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_messageBuffer();
    CBOR_Encode_GetStreamRequestMessage_IgnoreArg_encodedMessageSize();
    CBOR_Encode_GetStreamRequestMessage_ReturnThruPtr_messageBuffer(expectedCborMsg);

    mqttWrapper_publish_ExpectAndReturn(context.topicGetStream, context.topicGetStreamLength, expectedCborMsg, 0, false);

    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderPublishFailed, uintResult);
}

void test_requestDataBlock_returnsBadParameter_givenNullContext( void )
{
    uintResult = mqttDownloader_requestDataBlock(NULL, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderBadParameter, uintResult);
}

void test_requestDataBlock_returnsDownloaderNotInit_givenZeroStreamDataLength( void )
{
    MqttFileDownloaderContext_t context;
    context.topicStreamDataLength = 0;
    context.topicGetStreamLength = 1;
    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderNotInitialized, uintResult);
}

void test_requestDataBlock_returnsDownloaderNotInit_givenZeroGetStreamLength( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.topicStreamDataLength = 1;
    context.topicGetStreamLength = 0;
    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderNotInitialized, uintResult);
}

void test_requestDataBlock_returnsDownloaderNotInit_givenZeroStreamLengths( void )
{
    MqttFileDownloaderContext_t context = { 0 };
    context.topicStreamDataLength = 0;
    context.topicGetStreamLength = 0;
    uintResult = mqttDownloader_requestDataBlock(&context, 4U, 3U, 2U, 1U);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderNotInitialized, uintResult);
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
