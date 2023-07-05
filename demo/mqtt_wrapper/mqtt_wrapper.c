#include <assert.h>

#include "mqtt_wrapper/mqtt_wrapper.h"

static MQTTContext_t * globalCoreMqttContext = NULL;

void setCoreMqttContext( MQTTContext_t * mqttContext )
{
    globalCoreMqttContext = mqttContext;
}

MQTTContext_t * getCoreMqttContext( void )
{
    assert( globalCoreMqttContext != NULL );
    return globalCoreMqttContext;
}

bool mqttConnect( char * thingName )
{
    assert( globalCoreMqttContext != NULL );
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
    return MQTTSuccess == MQTT_Connect( globalCoreMqttContext,
                                        &connectInfo,
                                        NULL,
                                        5000U,
                                        &sessionPresent );
}

bool isMqttConnected( void )
{
    assert( globalCoreMqttContext != NULL );
    return globalCoreMqttContext->connectStatus == MQTTConnected;
}

bool mqttPublish( char * topic,
                  size_t topicLength,
                  uint8_t * message,
                  size_t messageLength )
{
    assert( globalCoreMqttContext != NULL );
    if( !isMqttConnected() )
    {
        return false;
    }
    MQTTPublishInfo_t pubInfo = { .qos = 0,
                                  .retain = false,
                                  .dup = false,
                                  .pTopicName = topic,
                                  .topicNameLength = topicLength,
                                  .pPayload = message,
                                  .payloadLength = messageLength };
    return MQTTSuccess ==
           MQTT_Publish( globalCoreMqttContext,
                         &pubInfo,
                         MQTT_GetPacketId( globalCoreMqttContext ) );
}

bool mqttSubscribe( char * topic, size_t topicLength )
{
    assert( globalCoreMqttContext != NULL );
    if( !isMqttConnected() )
    {
        return false;
    }
    MQTTSubscribeInfo_t subscribeInfo = { .qos = 0,
                                          .pTopicFilter = topic,
                                          .topicFilterLength = topicLength };
    return MQTTSuccess ==
           MQTT_Subscribe( globalCoreMqttContext,
                           &subscribeInfo,
                           1,
                           MQTT_GetPacketId( globalCoreMqttContext ) );
}
