/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef CORE_JOBS_H_
#define CORE_JOBS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_JOB_ID_LENGTH       64U

typedef enum JobStatus
{
    Queued,
    InProgress,
    Failed,
    Succeeded,
    Rejected
} JobStatus_t;

typedef enum JobUpdateStatus
{
    JobUpdateStatus_Accepted,
    JobUpdateStatus_Rejected
} JobUpdateStatus_t;

typedef bool ( *IncomingJobDocHandler_t )( const char * jobId,
                                           const size_t jobIdLength,
                                           const char * jobDoc,
                                           const size_t jobDocLength );

/* ------------------------ coreJobs API Functions -------------------------- */
/* Called by downstream users of coreJobs (e.g. the OTA library) */
bool coreJobs_sendStatusUpdate( const char * jobId, const size_t jobIdLength );

bool coreJobs_checkForJobs();

/**
 * @brief Starts the next pending job vended by IoT core
 *
 * @param thingname IoT 'Thing' name
 * @param thingnameLength IoT 'Thing' name length
 * @param clientToken String used to match correspondence
 * @param clientTokenLength Length of clientToken
 * @return true Message to start next pending job was published
 * @return false Messsage to start next pending job was not published
 */
bool coreJobs_startNextPendingJob( char * thingname,
                                   size_t thingnameLength,
                                   char * clientToken,
                                   size_t clientTokenLength );

/**
 * @brief Updates the specified job for the thing to the specified status
 *
 * @param thingname IoT 'Thing' name
 * @param thingnameLength Thingname length
 * @param jobId IoT Core job identifier
 * @param jobIdLength Job identifier length
 * @param status Job status
 * @param expectedVersion Expected current version of the job execution
 * @param expectedVersionLength Length of expectedVersion
 * @return true Message to update job status was published
 * @return false Messsage to update job status was not published
 */
bool coreJobs_updateJobStatus( char * thingname,
                               size_t thingnameLength,
                               char * jobId,
                               size_t jobIdLength,
                               JobStatus_t status,
                               char * expectedVersion,
                               size_t expectedVersionLength );

/**
 * @brief Retrieves the job ID from a given message (if applicable)
 * 
 * @param message [In] A JSON formatted message which
 * @param messageLength [In] The length of the message
 * @param jobId [Out] The job ID
 * @return size_t The job ID length
 */
size_t coreJobs_getJobId(const char * message, size_t messageLength, char ** jobId);

/**
 * @brief Retrieves the job document from a given message (if applicable)
 * 
 * @param message [In] A JSON formatted message which
 * @param messageLength [In] The length of the message
 * @param jobDoc [Out] The job document
 * @return size_t The length of the job document
 */
size_t coreJobs_getJobDocument(const char * message, size_t messageLength, char ** jobDoc);

/**
 * @brief Checks if a message comes from the start-next/accepted reserved topic
 * 
 * @param topic The topic to check against
 * @param topicLength The expected topic length 
 * @return true If the topic is the start-next/accepted topic
 * @return false If the topic is not the start-next/accepted topic
 */
bool coreJobs_isStartNextAccepted( const char * topic,
                                   const size_t topicLength );

/**
 * @brief Checks if a message comes from the update/accepted reserved topic
 * 
 * @param topic The topic to check against
 * @param topicLength The expected topic length 
 * @param jobId Corresponding Job ID which the update was accepted for
 * @param jobIdLength The Job ID length
 * @param expectedStatus The job update status reported by AWS IoT Jobs
 * @return true If the topic is the update/<expectedStatus> topic
 * @return false If the topic is not the update/<expectedStatus> topic
 */
bool coreJobs_isJobUpdateStatus( const char * topic,
                                const size_t topicLength,
                                const char * jobId,
                                const size_t jobIdLength,
                                JobUpdateStatus_t expectedStatus );

#endif
