#ifndef OTA_DEMO_H
#define OTA_DEMO_H

void otaDemo_start( void );

bool otaDemo_handleIncomingMQTTMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength );
#endif
