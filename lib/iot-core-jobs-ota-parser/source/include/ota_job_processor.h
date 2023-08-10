/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef OTA_JOB_PROCESSOR_H
#define OTA_JOB_PROCESSOR_H

#include <stdint.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct AfrOtaJobDocumentFields
{
    char * signature;
    size_t signatureLen;
    char * filepath;
    size_t filepathLen;
    char * certfile;
    size_t certfileLen;
    char * authScheme;
    size_t authSchemeLen;
    char * imageRef;
    size_t imageRefLen;
    uint32_t fileId;
    uint32_t fileSize;
    uint32_t fileType;
} AfrOtaJobDocumentFields_t;

typedef void ( *OtaDocProcessor_t )( AfrOtaJobDocumentFields_t * params );

int otaParser_parseJobDocFile( const char * jobDoc,
                                   const size_t jobDocLength,
                                   const uint8_t fileIndex,
                                   AfrOtaJobDocumentFields_t * fields );

#endif /*OTA_JOB_PROCESSOR_H*/
