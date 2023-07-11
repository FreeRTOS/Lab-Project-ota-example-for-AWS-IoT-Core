/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef MQTT_WRAPPER_H
#define MQTT_WRAPPER_H

#include "core_mqtt.h"

void setCoreMqttContext( MQTTContext_t * mqttContext );

MQTTContext_t * getCoreMqttContext( void );

void setThingName( char * thingName );

void getThingName( char * thingNameBuffer );

bool mqttConnect( char * thingName );

bool isMqttConnected( void );

bool mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength );

bool mqttSubscribe( char * topic, size_t topicLength );

#endif
