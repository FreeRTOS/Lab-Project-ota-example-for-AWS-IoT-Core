#ifndef CORE_JOBS_H_
#define CORE_JOBS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef bool ( *IncomingJobDocHandler_t )( const char * jobId,
                                           const size_t jobIdLength,
                                           const char * jobDoc,
                                           const size_t jobDocLength );

// ------------------------ coreJobs API Functions --------------------------
// Called by downstream users of coreJobs (e.g. the OTA library)
bool coreJobs_sendStatusUpdate( const char * jobId, const size_t jobIdLength );

bool coreJobs_checkForJobs();

// ------------------------ MQTT API Functions  --------------------------
// Called by the platform wrapper, implemented by coreJobs

// Can be called on any incoming MQTT message
// If the incoming MQTT message is intended for an AWS IoT Jobs topic, then this
// function parses the Job doc and distributes it through the Jobs chain of
// responsibilities, then returns true. Returns false otherwise.
bool coreJobsMQTTAPI_handleIncomingMQTTMessage(
    const IncomingJobDocHandler_t jobDocHandler,
    const char * topic,
    const size_t topicLength,
    const uint8_t * message,
    size_t messageLength );

#endif
