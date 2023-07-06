#include <stdio.h>
#include <string.h>

#include "core_jobs.h"
#include "core_json.h"
#include "mqtt_wrapper/mqtt_wrapper.h"

#define TOPIC_BUFFER_SIZE     256U
#define MAX_THING_NAME_LENGTH 128U

bool isMessageJobStartNextAccepted( const char * topic,
                                    const size_t topicLength )
{
    /* TODO: Inefficient - better implementation shouldn't use snprintf */
    bool isMatch = false;
    char expectedBuffer[ TOPIC_BUFFER_SIZE + 1 ] = { 0 };
    char thingName[ MAX_THING_NAME_LENGTH + 1 ] = { 0 };
    getThingName( thingName, MAX_THING_NAME_LENGTH );
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
                            char * jobId,
                            size_t * jobIdLength,
                            char * jobDoc,
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
                                  &jobId,
                                  jobIdLength );
    }
    if( jsonResult == JSONSuccess )
    {
        jsonResult = JSON_Search( ( char * ) message,
                                  messageLength,
                                  "execution.jobDocument",
                                  strlen( "execution.jobDocument" ),
                                  &jobDoc,
                                  jobDocLength );
    }
    return true;
}

// Can be called on any incoming MQTT message
// If the incoming MQTT message is intended for an AWS IoT Jobs topic, then
// this function parses the Job doc and distributes it through the Jobs
// chain of responsibilities, then returns true. Returns false otherwise.
bool coreJobsMQTTAPI_handleIncomingMQTTMessage(
    const IncomingJobDocHandler_t * jobDocHandler,
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
                                                      jobId,
                                                      &jobIdLength,
                                                      jobDoc,
                                                      &jobDocLength );

    willHandle = willHandle &&
                 ( *jobDocHandler )( jobId, jobIdLength, jobDoc, jobDocLength );
    return willHandle;
}
