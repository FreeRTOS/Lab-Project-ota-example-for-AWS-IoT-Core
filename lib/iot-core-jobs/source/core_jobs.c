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

#include "core_jobs.h"
#include "core_json.h"
#include "mqtt_wrapper.h"

#define TOPIC_BUFFER_SIZE     256U
#define MAX_THING_NAME_LENGTH 128U
/* This accounts for the message fields and client token (128 chars) */
#define START_JOB_MSG_LENGTH  147U
/* This accounts for the message fields and expected version size (up to '999')
 */
#define UPDATE_JOB_MSG_LENGTH 48U

static const char * jobStatusString[ 5U ] = { "QUEUED",
                                              "IN_PROGRESS",
                                              "FAILED",
                                              "SUCCEEDED",
                                              "REJECTED" };

static const size_t jobStatusStringLengths[ 5U ] = {
    sizeof( "QUEUED" ) - 1U,
    sizeof( "IN_PROGRESS" ) - 1U,
    sizeof( "FAILED" ) - 1U,
    sizeof( "SUCCEEDED" ) - 1U,
    sizeof( "REJECTED" ) - 1U
};

static size_t getStartNextPendingJobExecutionTopic( const char * thingname,
                                                    size_t thingnameLength,
                                                    char * buffer,
                                                    size_t bufferSize );
static size_t getStartNextPendingJobExecutionMsg( const char * clientToken,
                                                  size_t clientTokenLength,
                                                  char * buffer,
                                                  size_t bufferSize );
static size_t getUpdateJobExecutionTopic( char * thingname,
                                          size_t thingnameLength,
                                          char * jobId,
                                          size_t jobIdLength,
                                          char * buffer,
                                          size_t bufferSize );
static size_t getUpdateJobExecutionMsg( JobStatus_t status,
                                        char * expectedVersion,
                                        size_t expectedVersionLength,
                                        char * buffer,
                                        size_t bufferSize );

bool isMessageJobStartNextAccepted( const char * topic,
                                    const size_t topicLength )
{
    /* TODO: Inefficient - better implementation shouldn't use snprintf */
    bool isMatch = false;
    char expectedBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    char thingName[ MAX_THING_NAME_LENGTH + 1 ] = { 0 };
    mqttWrapper_getThingName( thingName );
    snprintf( expectedBuffer,
              TOPIC_BUFFER_SIZE,
              "%s%s%s",
              "$aws/things/",
              thingName,
              "/jobs/start-next/accepted" );
    isMatch = strnlen( expectedBuffer, TOPIC_BUFFER_SIZE ) == topicLength;
    isMatch = isMatch && strncmp( expectedBuffer, topic, topicLength ) == 0;
    return isMatch;
}

bool getJobStartNextFields( const uint8_t * message,
                            const size_t messageLength,
                            char ** jobId,
                            size_t * jobIdLength,
                            char ** jobDoc,
                            size_t * jobDocLength )
{
    JSONStatus_t jsonResult = JSONNotFound;
    jsonResult = JSON_Validate( ( char * ) message, messageLength );
    if( jsonResult == JSONSuccess )
    {
        jsonResult = JSON_Search( ( char * ) message,
                                  messageLength,
                                  "execution.jobId",
                                  sizeof( "execution.jobId" ) - 1,
                                  jobId,
                                  jobIdLength );
    }
    if( jsonResult == JSONSuccess )
    {
        jsonResult = JSON_Search( ( char * ) message,
                                  messageLength,
                                  "execution.jobDocument",
                                  sizeof( "execution.jobDocument" ) - 1,
                                  jobDoc,
                                  jobDocLength );
    }
    return jsonResult == JSONSuccess;
}

// Can be called on any incoming MQTT message
// If the incoming MQTT message is intended for an AWS IoT Jobs topic, then
// this function parses the Job doc and distributes it through the Jobs
// chain of responsibilities, then returns true. Returns false otherwise.
bool coreJobs_handleIncomingMQTTMessage(
    const IncomingJobDocHandler_t jobDocHandler,
    const char * topic,
    const size_t topicLength,
    const uint8_t * message,
    size_t messageLength )
{
    bool willHandle = false;
    char * jobId = NULL;
    size_t jobIdLength = 0U;
    char * jobDoc = NULL;
    size_t jobDocLength = 0U;

    willHandle = isMessageJobStartNextAccepted( topic, topicLength );
    willHandle = willHandle && getJobStartNextFields( message,
                                                      messageLength,
                                                      &jobId,
                                                      &jobIdLength,
                                                      &jobDoc,
                                                      &jobDocLength );

    willHandle = willHandle &&
                 ( *jobDocHandler )( jobId, jobIdLength, jobDoc, jobDocLength );
    return willHandle;
}

bool coreJobs_startNextPendingJob( char * thingname,
                                   size_t thingnameLength,
                                   char * clientToken,
                                   size_t clientTokenLength )
{
    bool published = false;
    char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    char messageBuffer[ START_JOB_MSG_LENGTH ] = { 0 };

    size_t
        topicLength = getStartNextPendingJobExecutionTopic( thingname,
                                                            thingnameLength,
                                                            topicBuffer,
                                                            TOPIC_BUFFER_SIZE );

    size_t messageLength = getStartNextPendingJobExecutionMsg(
        clientToken,
        clientTokenLength,
        messageBuffer,
        ( START_JOB_MSG_LENGTH ) );

    if( topicLength > 0 && messageLength > 0 )
    {
        published = mqttWrapper_publish( topicBuffer,
                                 topicLength,
                                 ( uint8_t * ) messageBuffer,
                                 messageLength );
    }

    return published;
}

/* TODO - Update input parameters, there are too many currently */
bool coreJobs_updateJobStatus( char * thingname,
                               size_t thingnameLength,
                               char * jobId,
                               size_t jobIdLength,
                               JobStatus_t status,
                               char * expectedVersion,
                               size_t expectedVersionLength )
{
    bool published = false;
    char topicBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    char messageBuffer[ UPDATE_JOB_MSG_LENGTH ] = { 0 };

    size_t topicLength = getUpdateJobExecutionTopic( thingname,
                                                     thingnameLength,
                                                     jobId,
                                                     jobIdLength,
                                                     topicBuffer,
                                                     TOPIC_BUFFER_SIZE );

    size_t messageLength = getUpdateJobExecutionMsg( status,
                                                     expectedVersion,
                                                     expectedVersionLength,
                                                     messageBuffer,
                                                     UPDATE_JOB_MSG_LENGTH );

    if( topicLength > 0 && messageLength > 0 )
    {
        published = mqttWrapper_publish( topicBuffer,
                                 topicLength,
                                 ( uint8_t * ) messageBuffer,
                                 messageLength );
    }

    return published;
}

static size_t getStartNextPendingJobExecutionTopic( const char * thingname,
                                                    size_t thingnameLength,
                                                    char * buffer,
                                                    size_t bufferSize )
{
    assert( bufferSize >= 28U + thingnameLength );

    size_t topicLength = sizeof( "$aws/things/" ) - 1;
    memcpy( buffer, "$aws/things/", sizeof( "$aws/things/" ) - 1 );
    memcpy( buffer + topicLength, thingname, thingnameLength );
    topicLength += thingnameLength;
    memcpy( buffer + topicLength,
            "/jobs/start-next",
            sizeof( "/jobs/start-next" ) - 1 );

    return topicLength + sizeof( "/jobs/start-next" ) - 1;
}

static size_t getStartNextPendingJobExecutionMsg( const char * clientToken,
                                                  size_t clientTokenLength,
                                                  char * buffer,
                                                  size_t bufferSize )
{
    assert( bufferSize >= 18U + clientTokenLength );

    size_t messageLength = sizeof( "{\"clientToken\":\"" ) - 1;
    memcpy( buffer,
            "{\"clientToken\":\"",
            sizeof( "{\"clientToken\":\"" ) - 1 );
    memcpy( buffer + messageLength, clientToken, clientTokenLength );
    messageLength += clientTokenLength;
    memcpy( buffer + messageLength, "\"}", sizeof( "\"}" ) - 1 );

    return messageLength + sizeof( "\"}" ) - 1;
}

static size_t getUpdateJobExecutionTopic( char * thingname,
                                          size_t thingnameLength,
                                          char * jobId,
                                          size_t jobIdLength,
                                          char * buffer,
                                          size_t bufferSize )
{
    assert( bufferSize >= 25U + thingnameLength + jobIdLength );

    size_t topicLength = sizeof( "$aws/things/" ) - 1;
    memcpy( buffer, "$aws/things/", sizeof( "$aws/things/" ) - 1 );
    memcpy( buffer + topicLength, thingname, thingnameLength );
    topicLength += thingnameLength;
    memcpy( buffer + topicLength, "/jobs/", sizeof( "/jobs/" ) - 1 );
    topicLength += sizeof( "/jobs/" ) - 1;
    memcpy( buffer + topicLength, jobId, jobIdLength );
    topicLength += jobIdLength;
    memcpy( buffer + topicLength, "/update", sizeof( "/update" ) - 1 );

    return topicLength + sizeof( "/update" ) - 1;
}

static size_t getUpdateJobExecutionMsg( JobStatus_t status,
                                        char * expectedVersion,
                                        size_t expectedVersionLength,
                                        char * buffer,
                                        size_t bufferSize )
{
    assert( bufferSize >=
            34U + expectedVersionLength + jobStatusStringLengths[ status ] );

    size_t messageLength = sizeof( "{\"status\":\"" ) - 1;
    memcpy( buffer, "{\"status\":\"", sizeof( "{\"status\":\"" ) - 1 );
    memcpy( buffer + messageLength,
            jobStatusString[ status ],
            jobStatusStringLengths[ status ] );

    messageLength += jobStatusStringLengths[ status ];
    memcpy( buffer + messageLength,
            "\",\"expectedVersion\":\"",
            sizeof( "\",\"expectedVersion\":\"" ) - 1 );
    messageLength += sizeof( "\",\"expectedVersion\":\"" ) - 1;
    memcpy( buffer + messageLength, expectedVersion, expectedVersionLength );
    messageLength += expectedVersionLength;
    memcpy( buffer + messageLength, "\"}", sizeof( "\"}" ) - 1 );

    return messageLength + sizeof( "\"}" ) - 1;
}
