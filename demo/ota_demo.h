#ifndef OTA_DEMO_H
#define OTA_DEMO_H

//TODO - Remove me when CoreJobs integration is complete
typedef bool ( *IncomingJobDocHandler_t )( const char * jobId,
                                           const size_t jobIdLength,
                                           const char * jobDoc,
                                           const size_t jobDocLength );

void otaDemo_start( void );

#endif
