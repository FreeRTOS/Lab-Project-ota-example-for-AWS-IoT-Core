#ifndef MQTT_FILE_DOWNLOADER_H
#define MQTT_FILE_DOWNLOADER_H


#include <stdbool.h>
#include <stdint.h>

/*
 * Enum contains all the data types supported.
*/
enum DataType {
  DATA_TYPE_JSON,
  DATA_TYPE_CBOR
};

/*
 * Structure to contain the data block information. 
*/
typedef struct MqttFileDownloaderDataBlockInfo {
    uint8_t *payload;
    size_t payloadLength;
} MqttFileDownloaderDataBlockInfo_t;

 /**
  * Initializes the MQTT file downloader.
  * Creates the topic name the DATA and Get Stream Data topics
  *
  * @param[in] pxMQTTContext MQTT context pointer.
  * @param[in] pStreamName Stream name to download the file.
  * @param[in] pThingName Thing name of the Device.
  */
uint8_t ucMqttFileDownloaderInit(char * pStreamName, size_t streamNameLength, char *pThingName, uint8_t ucDataType);

/**
 * Request the Data blocks from MQTT Streams.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 * @param[in] usFileId File Id of the file to be downloaded from MQTT Streams.
 * @param[in] ulBlockSize Requested size of block.
 * @param[in] usBlockOffset Block Offset.
 * @param[in] ulNumberOfBlocksRequested Number of Blocks requested per request.
 */
uint8_t ucRequestDataBlock(uint16_t usFileId,
                            uint32_t ulBlockSize,
                            uint16_t usBlockOffset,
                            uint32_t ulNumberOfBlocksRequested);

/**
 * @brief Process incoming Publish message.
 *
 * @param[in] pxPublishInfo is a pointer to structure containing deserialized
 * Publish message.
 */
bool mqttStreams_handleIncomingMessage( char * topic,
                                        size_t topicLength,
                                        uint8_t * message,
                                        size_t messageLength );

/**
 * @brief Function to update variable TopicFilterContext with status
 * information from Subscribe ACK. Called by the event callback after processing
 * an incoming SUBACK packet.
 *
 * @param[in] Server response to the subscription request.
 */
//void prvUpdateSubAckStatus(MQTTPacketInfo_t* pxPacketInfo);

#endif // #ifndef MQTT_FILE_DOWNLOADER_H

 