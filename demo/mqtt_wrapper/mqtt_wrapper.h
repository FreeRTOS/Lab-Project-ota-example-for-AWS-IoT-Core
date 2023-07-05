#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H

#include "core_mqtt.h"

void setCoreMqttContext( MQTTContext_t * mqttContext );

MQTTContext_t * getCoreMqttContext( void );

bool mqttConnect( char * thingName );

bool isMqttConnected( void );

bool mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength );

bool mqttSubscribe( char * topic, size_t topicLength );

#endif
