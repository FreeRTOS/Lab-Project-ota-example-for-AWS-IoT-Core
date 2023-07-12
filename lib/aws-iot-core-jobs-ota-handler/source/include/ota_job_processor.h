/* License Notification Here */

#ifndef OTA_JOB_PROCESSOR_H
#define OTA_JOB_PROCESSOR_H

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

void applicationSuppliedFunction_processAfrOtaDocument( AfrOtaJobDocumentFields_t * params );

#endif /*OTA_JOB_PROCESSOR_H*/
