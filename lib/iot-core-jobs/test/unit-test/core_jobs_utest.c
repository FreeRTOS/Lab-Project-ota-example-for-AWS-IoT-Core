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

#include "core_jobs.h"
#include "core_json.h"

#include "mock_mqtt_wrapper.h"

/* ===========================   UNITY FIXTURES ============================ */


/* Called before each test method. */
void setUp()
{
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

bool startJob_publishCallback(char * topic,
                          size_t topicLength,
                          uint8_t * message,
                          size_t messageLength,
                          int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY(topic, "$aws/things/thingname/jobs/start-next", strlen("$aws/things/thingname/jobs/start-next"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/jobs/start-next"), topicLength);
    TEST_ASSERT_EQUAL_MEMORY(message, "{\"clientToken\":\"clientToken\"}", strlen("{\"clientToken\":\"clientToken\"}"));
    TEST_ASSERT_EQUAL_INT( strlen("{\"clientToken\":\"clientToken\"}"), messageLength);

    return true;
}

bool updateJob_publishCallback(char * topic,
                          size_t topicLength,
                          uint8_t * message,
                          size_t messageLength,
                          int NumCalls)
{
    TEST_ASSERT_EQUAL_MEMORY(topic, "$aws/things/thingname/jobs/jobId/update", strlen("$aws/things/thingname/jobs/jobId/update"));
    TEST_ASSERT_EQUAL_INT(strlen("$aws/things/thingname/jobs/jobId/update"), topicLength);
    TEST_ASSERT_EQUAL_MEMORY(message, "{\"status\":\"SUCCEEDED\",\"expectedVersion\":\"1.0.1\"}", strlen("{\"status\":\"SUCCEEDED\",\"expectedVersion\":\"1.0.1\"}"));
    TEST_ASSERT_EQUAL_INT( strlen("{\"status\":\"SUCCEEDED\",\"expectedVersion\":\"1.0.1\"}"), messageLength);

    return true;
}

/* ===============================   TESTS   =============================== */

void test_isStartNextAccepted_isStartNextMsg( void )
{
    char * thingName = "thingname";
    char topic[] = "$aws/things/thingname/jobs/start-next/accepted";
    size_t topicLength = strlen(topic);

    mqttWrapper_getThingName_ExpectAnyArgs();
    mqttWrapper_getThingName_ReturnArrayThruPtr_thingNameBuffer(thingName, strlen(thingName));

    bool result = coreJobs_isStartNextAccepted(topic, topicLength);

    TEST_ASSERT_TRUE(result);

}

void test_isStartNextAccepted_isNotStartNextMsg( void )
{
    char * thingName = "thingname";
    char topic[] = "thingname/random/topic";
    size_t topicLength = strlen(topic);

    mqttWrapper_getThingName_ExpectAnyArgs();
    mqttWrapper_getThingName_ReturnArrayThruPtr_thingNameBuffer(thingName, strlen(thingName));

    bool result = coreJobs_isStartNextAccepted(topic, strlen(topic));

    TEST_ASSERT_FALSE(result);
}

void test_isStartNextAccepted_isStartNextMsgForAnotherThing( void )
{
    char * thingName = "thingname";
    char topic[] = "$aws/things/differntThignName/jobs/start-next/accepted";
    size_t topicLength = strlen(topic);

    mqttWrapper_getThingName_ExpectAnyArgs();
    mqttWrapper_getThingName_ReturnArrayThruPtr_thingNameBuffer(thingName, strlen(thingName));

    bool result = coreJobs_isStartNextAccepted(topic, strlen(topic));

    TEST_ASSERT_FALSE(result);
}

void test_isStartNextAccepted_isStartNextMsgForSameLengthThing( void )
{
    char * thingName = "thingname";
    char topic[] = "$aws/things/different/jobs/start-next/accepted";
    size_t topicLength = strlen(topic);

    mqttWrapper_getThingName_ExpectAnyArgs();
    mqttWrapper_getThingName_ReturnArrayThruPtr_thingNameBuffer(thingName, strlen(thingName));

    bool result = coreJobs_isStartNextAccepted(topic, strlen(topic));

    TEST_ASSERT_FALSE(result);
}

void test_isStartNextAccepted_nullTopic( void )
{
    bool result = coreJobs_isStartNextAccepted(NULL, 1U);

    TEST_ASSERT_FALSE(result);
}

void test_isStartNextAccepted_zeroTopicLength( void )
{
    char topic[] = "$aws/things/differntThignName/jobs/start-next/accepted";

    bool result = coreJobs_isStartNextAccepted(topic, 0U);

    TEST_ASSERT_FALSE(result);
}

void test_getJobId_returnsJobId( void )
{
    char * message = "{\"execution\":{\"jobId\":\"identification\",\"jobDocument\":\"document\"}}";
    char * jobId = NULL;

    size_t result = coreJobs_getJobId(message, strlen(message), &jobId);

    TEST_ASSERT_EQUAL(strlen("identification"), result);
    TEST_ASSERT_EQUAL_MEMORY("identification", jobId, result);
}

void test_getJobId_cannotFindJobId( void )
{
    char * message = "{\"execution\":{\"jobDocument\":\"document\"}}";
    char * jobId = NULL;

    size_t result = coreJobs_getJobId(message, strlen(message), &jobId);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobId);
}

void test_getJobId_malformedJson( void )
{
    char * message = "clearlyNotJson";
    char * jobId = NULL;

    size_t result = coreJobs_getJobId(message, strlen(message), &jobId);

TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobId);
}

void test_getJobId_returnsZeroLengthJob_givenNullMessage( void )
{
    char * jobId = NULL;

    size_t result = coreJobs_getJobId(NULL, 10U, &jobId);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobId);

}

void test_getJobId_returnsZeroLengthJob_givenZeroMessageLength( void )
{
    char * message = "{\"execution\":{\"jobId\":\"identification\",\"jobDocument\":\"document\"}}";
    char * jobId = NULL;

    size_t result = coreJobs_getJobId(message, 0U, &jobId);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobId);
}

void test_getJobDocument_returnsDoc( void )
{
    char * message = "{\"execution\":{\"jobId\":\"identification\",\"jobDocument\":\"document\"}}";
    char * jobDocument = NULL;

    size_t result = coreJobs_getJobDocument(message, strlen(message), &jobDocument);

    TEST_ASSERT_EQUAL(strlen("document"), result);
    TEST_ASSERT_EQUAL_MEMORY("document", jobDocument, result);
}

void test_getJobDocument_cannotFindDoc( void )
{
    char * message = "{\"execution\":{\"jobId\":\"identification\"}}";
    char * jobDocument = NULL;

    size_t result = coreJobs_getJobDocument(message, strlen(message), &jobDocument);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobDocument);
}

void test_getJobDocument_malformedJson( void )
{
    char * message = "clearlyNotJson";
    char * jobDocument = NULL;

    size_t result = coreJobs_getJobDocument(message, strlen(message), &jobDocument);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobDocument);
}

void test_getJobDocument_returnsZeroLengthJob_givenNullMessage( void )
{
    char * jobDocument = NULL;

    size_t result = coreJobs_getJobDocument(NULL, 10U, &jobDocument);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobDocument);
}

void test_getJobDocument_returnsZeroLengthJob_givenZeroMessageLength( void )
{
    char * message = "{\"execution\":{\"jobId\":\"identification\",\"jobDocument\":\"document\"}}";
    char * jobDocument = NULL;

    size_t result = coreJobs_getJobDocument(message, 0U, &jobDocument);

    TEST_ASSERT_EQUAL(0U, result);
    TEST_ASSERT_NULL(jobDocument);
}

void test_startNextPendingJob_startsJob( void )
{
    char * thingName = "thingname";
    char * clientToken = "clientToken";

    mqttWrapper_publish_Stub(startJob_publishCallback);

    bool result = coreJobs_startNextPendingJob(thingName, (size_t) strlen(thingName), clientToken, (size_t) strlen(clientToken));

    TEST_ASSERT_TRUE(result);
}

void test_startNextPendingJob_returnsFalse_givenNullThingname( void )
{
    char * thingName = NULL;
    char * clientToken = "clientToken";

    bool result = coreJobs_startNextPendingJob(thingName, 10U, clientToken, (size_t) strlen(clientToken));

    TEST_ASSERT_FALSE(result);
}

void test_startNextPendingJob_returnsFalse_givenNullClientToken( void )
{
    char * thingName = "thingname";
    char * clientToken = NULL;

    bool result = coreJobs_startNextPendingJob(thingName, (size_t) strlen(thingName), clientToken, 1U);

    TEST_ASSERT_FALSE(result);
}

void test_startNextPendingJob_returnsFalse_gienZeroThingnameLength( void )
{
    char * thingName = "thingname";
    char * clientToken = "clientToken";

    bool result = coreJobs_startNextPendingJob(thingName, 0U, clientToken, (size_t) strlen(clientToken));

    TEST_ASSERT_FALSE(result);
}

void test_startNextPendingJob_returnsFalse_givenZeroClientTokenLength( void )
{
    char * thingName = "thingname";
    char * clientToken = "clientToken";

    bool result = coreJobs_startNextPendingJob(thingName, (size_t) strlen(thingName), clientToken, 0U);

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_updatesStatus( void )
{
    char * thingName = "thingname";
    char * jobId = "jobId";
    char * expectedVersion = "1.0.1";

    mqttWrapper_publish_Stub(updateJob_publishCallback);

    bool result = coreJobs_updateJobStatus(thingName,
            (size_t) strlen(thingName),
            jobId,
            (size_t) strlen(jobId),
            Succeeded,
            expectedVersion,
            (size_t) strlen(expectedVersion));

    TEST_ASSERT_TRUE(result);
}

void test_updateJobStatus_returnsFalse_givenNullThingname( void )
{
    char * thingName = NULL;
    char * jobId = "jobId";
    char * expectedVersion = "1.0.1";

    bool result = coreJobs_updateJobStatus(thingName,
            1U,
            jobId,
            (size_t) strlen(jobId),
            Succeeded,
            expectedVersion,
            (size_t) strlen(expectedVersion));

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_returnsFalse_givenNullJobId( void )
{
    char * thingName = "thingname";
    char * jobId = NULL;
    char * expectedVersion = "1.0.1";

    bool result = coreJobs_updateJobStatus(thingName,
            (size_t) strlen(thingName),
            jobId,
            1U,
            Succeeded,
            expectedVersion,
            (size_t) strlen(expectedVersion));

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_returnsFalse_givenNullVersion( void )
{
    char * thingName = "thingname";
    char * jobId = "jobId";
    char * expectedVersion = NULL;

    bool result = coreJobs_updateJobStatus(thingName,
            (size_t) strlen(thingName),
            jobId,
            (size_t) strlen(jobId),
            Succeeded,
            expectedVersion,
            1U);

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_returnsFalse_givenZeroThingnameLength( void )
{
    char * thingName = "thingname";
    char * jobId = "jobId";
    char * expectedVersion = "1.0.1";

    bool result = coreJobs_updateJobStatus(thingName,
            0U,
            jobId,
            (size_t) strlen(jobId),
            Succeeded,
            expectedVersion,
            (size_t) strlen(expectedVersion));

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_returnsFalse_givenZeroJobIdLength( void )
{
    char * thingName = "thingname";
    char * jobId = "jobId";
    char * expectedVersion = "1.0.1";

    bool result = coreJobs_updateJobStatus(thingName,
            (size_t) strlen(thingName),
            jobId,
            0U,
            Succeeded,
            expectedVersion,
            (size_t) strlen(expectedVersion));

    TEST_ASSERT_FALSE(result);
}

void test_updateJobStatus_returnsFalse_givenZeroVersionLength( void )
{
    char * thingName = "thingname";
    char * jobId = "jobId";
    char * expectedVersion = "1.0.1";

    bool result = coreJobs_updateJobStatus(thingName,
            (size_t) strlen(thingName),
            jobId,
            (size_t) strlen(jobId),
            Succeeded,
            expectedVersion,
            0U);

    TEST_ASSERT_FALSE(result);
}