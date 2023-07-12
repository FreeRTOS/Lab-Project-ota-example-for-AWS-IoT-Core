/* License Notification Here */

#ifndef OTA_JOB_HANDLER_H
#define OTA_JOB_HANDLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool handleJobDoc( const char * jobId,
                   const size_t jobIdLength,
                   const char * jobDoc,
                   const size_t jobDocLength );

#endif /* OTA_JOB_HANDLER_H */
