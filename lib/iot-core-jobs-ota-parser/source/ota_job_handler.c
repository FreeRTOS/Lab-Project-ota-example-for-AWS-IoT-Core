/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core_json.h"

#include "job_parser.h"
#include "ota_job_processor.h"

/**
 * @brief Signals if the job document provided is a FreeRTOS OTA update document
 *
 * @param jobId The JobID from the AWS IoT Job
 * @param jobIdLength The length of the JobID
 * @param jobDoc The job document contained in the AWS IoT Job
 * @param jobDocLength The length of the job document
 * @return true The job document is for a FreeRTOS OTA update
 * @return false The job document is NOT for an FreeRTOS OTA update
 */
bool otaParser_handleJobDoc( OtaDocProcessor_t docCallback,
                             const char * jobId,
                             const size_t jobIdLength,
                             const char * jobDoc,
                             const size_t jobDocLength )
{
    bool docHandled = false;
    JSONStatus_t isFreeRTOSOta = JSONIllegalDocument;
    const char * afrOtaDocHeader;
    size_t afrOtaDocHeaderLength = 0U;

    ( void ) jobId;
    ( void ) jobIdLength;

    if( ( jobDoc != NULL ) && ( jobDocLength > 0U ) )
    {
        /* FreeRTOS OTA updates have a top level "afr_ota" job document key.
         * Check for this to ensure the docuemnt is an FreeRTOS OTA update */
        isFreeRTOSOta = JSON_SearchConst( jobDoc,
                                          jobDocLength,
                                          "afr_ota",
                                          7U,
                                          &afrOtaDocHeader,
                                          &afrOtaDocHeaderLength,
                                          NULL );
    }

    if( isFreeRTOSOta == JSONSuccess )
    {
        const char * fileValue;
        size_t fileValueLength = 0U;
        int fileIndex = 0;
        char files[ 17U ] = "afr_ota.files[0]";
        docHandled = true;

        while( docHandled && ( fileIndex <= 9 ) &&
               ( JSONSuccess == JSON_SearchConst( jobDoc,
                                                  jobDocLength,
                                                  files,
                                                  16U,
                                                  &fileValue,
                                                  &fileValueLength,
                                                  NULL ) ) )
        {
            AfrOtaJobDocumentFields_t fields = { 0 };
            docHandled &= populateJobDocFields( jobDoc,
                                                jobDocLength,
                                                fileIndex,
                                                &fields );

            if( docHandled )
            {
                docCallback( &fields );
            }

            files[ 14U ] = ( char ) ( ++fileIndex + ( int ) '0' );
        }
    }

    return docHandled;
}
