#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H

#include "core_mqtt.h"

void initDemoGlobals( MQTTContext_t * mqttContext );

MQTTStatus_t mqttConnect( char * thingName );

void mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength );

void mqttSubscribe( char * topic, size_t topicLength );

#endif
