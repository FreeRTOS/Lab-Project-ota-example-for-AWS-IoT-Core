#include <stdio.h>
#include <string.h>

#include "core_jobs.h"
#include "core_json.h"
#include "mqtt_wrapper/mqtt_wrapper.h"

#define TOPIC_BUFFER_SIZE     256U
#define MAX_THING_NAME_LENGTH 128U
/* This accounts for the message fields and client token (128 chars) */
#define START_JOB_MSG_LENGTH  147U
/* This accounts for the message fields and expected version size (up to '999')
 */
#define UPDATE_JOB_MSG_LENGTH 48U

static const char * jobStatusString[ 5U ] = { "Queued",
                                              "InProgress",
                                              "Failed",
                                              "Succeeded",
                                              "Rejected" };

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
    getThingName( thingName );
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
                                  strlen( "execution.jobId" ),
                                  jobId,
                                  jobIdLength );
    }
    if( jsonResult == JSONSuccess )
    {
        jsonResult = JSON_Search( ( char * ) message,
                                  messageLength,
                                  "execution.jobDocument",
                                  strlen( "execution.jobDocument" ),
                                  jobDoc,
                                  jobDocLength );
    }
    return jsonResult == JSONSuccess;
}

// Can be called on any incoming MQTT message
// If the incoming MQTT message is intended for an AWS IoT Jobs topic, then
// this function parses the Job doc and distributes it through the Jobs
// chain of responsibilities, then returns true. Returns false otherwise.
bool coreJobsMQTTAPI_handleIncomingMQTTMessage(
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
        published = mqttPublish( topicBuffer,
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
        published = mqttPublish( topicBuffer,
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
    /* TODO - Assert on buffer size being at least the size of the topic */
    size_t topicLength = 12U;
    strncpy( buffer, "$aws/things/", bufferSize );
    strncpy( buffer + topicLength, thingname, bufferSize - topicLength );
    topicLength += thingnameLength;
    strncpy( buffer + topicLength,
             "/jobs/start-next",
             bufferSize - topicLength );

    return topicLength + 16U;
}

static size_t getStartNextPendingJobExecutionMsg( const char * clientToken,
                                                  size_t clientTokenLength,
                                                  char * buffer,
                                                  size_t bufferSize )
{
    size_t messageLength = 16U;
    strncpy( buffer, "{\"clientToken\":\"", bufferSize );
    strncpy( buffer + messageLength, clientToken, clientTokenLength );
    messageLength += clientTokenLength;
    strncpy( buffer + messageLength, "\"}", bufferSize - messageLength );

    return messageLength + 2U;
}

static size_t getUpdateJobExecutionTopic( char * thingname,
                                          size_t thingnameLength,
                                          char * jobId,
                                          size_t jobIdLength,
                                          char * buffer,
                                          size_t bufferSize )
{
    /* TODO - Assert on buffer size being at least the size of the topic */
    size_t topicLength = 12U;
    strncpy( buffer, "$aws/things/", bufferSize );
    strncpy( buffer + topicLength, thingname, thingnameLength );
    topicLength += thingnameLength;
    strncpy( buffer + topicLength, "/jobs/", bufferSize - topicLength );
    topicLength += 6U;
    strncpy( buffer + topicLength, jobId, jobIdLength );
    topicLength += jobIdLength;
    strncpy( buffer + topicLength, "/update", bufferSize - topicLength );

    return topicLength + 7U;
}

static size_t getUpdateJobExecutionMsg( JobStatus_t status,
                                        char * expectedVersion,
                                        size_t expectedVersionLength,
                                        char * buffer,
                                        size_t bufferSize )
{
    size_t messageLength = 11U;
    strncpy( buffer, "{\"status\":\"", bufferSize );
    strncpy( buffer + messageLength,
             jobStatusString[ status ],
             strlen(jobStatusString[status]) );
    messageLength += strlen( jobStatusString[ status ] );
    strncpy( buffer + messageLength,
             "\",\"expectedVersion\":\"",
             bufferSize - messageLength );
    messageLength += 21U;
    strncpy( buffer + messageLength,
             expectedVersion,
             expectedVersionLength );
    messageLength += expectedVersionLength;
    strncpy( buffer, "\"}", bufferSize - messageLength );

    return messageLength + 2U;
}
