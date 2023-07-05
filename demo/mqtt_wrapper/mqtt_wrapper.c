#include <assert.h>

#include "mqtt_wrapper/mqtt_wrapper.h"

static MQTTContext_t * globalCoreMqttContext = NULL;

void initDemoGlobals( MQTTContext_t * mqttContext )
{
    globalCoreMqttContext = mqttContext;
}

MQTTContext_t * getCoreMqttContext()
{
    return globalCoreMqttContext;
}

MQTTStatus_t mqttConnect( char * thingName )
{
    MQTTConnectInfo_t connectInfo = { 0 };
    bool sessionPresent = false;
    connectInfo.pClientIdentifier = thingName;
    connectInfo.clientIdentifierLength = ( uint16_t ) sizeof( thingName );
    connectInfo.pUserName = NULL;
    connectInfo.userNameLength = 0U;
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;
    connectInfo.keepAliveSeconds = 60U;
    connectInfo.cleanSession = true;
    return MQTT_Connect( globalCoreMqttContext,
                         &connectInfo,
                         NULL,
                         5000U,
                         &sessionPresent );
}

void mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength )
{
    assert( globalCoreMqttContext != NULL );
    MQTTPublishInfo_t pubInfo = { .qos = 0,
                                  .retain = false,
                                  .dup = false,
                                  .pTopicName = topic,
                                  .topicNameLength = topicLength,
                                  .pPayload = message,
                                  .payloadLength = messageLength };
    MQTT_Publish( globalCoreMqttContext,
                  &pubInfo,
                  MQTT_GetPacketId( globalCoreMqttContext ) );
}

void mqttSubscribe( char * topic, size_t topicLength )
{
    assert( globalCoreMqttContext != NULL );
    MQTTSubscribeInfo_t subscribeInfo = { .qos = 0,
                                          .pTopicFilter = topic,
                                          .topicFilterLength = topicLength };
    MQTT_Subscribe( globalCoreMqttContext,
                    &subscribeInfo,
                    1,
                    MQTT_GetPacketId( globalCoreMqttContext ) );
}
