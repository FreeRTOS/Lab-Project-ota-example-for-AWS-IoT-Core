/*
 * Copyright of Amazon Web Services, Inc. (AWS) 2022
 *
 * This code is licensed under the AWS Intellectual Property License, which can
 * be found here: https://aws.amazon.com/legal/aws-ip-license-terms/; provided
 * that AWS grants you a limited, royalty-free, revocable, non-exclusive,
 * non-sublicensable, non-transferrable license to modify the code for internal
 * testing purposes. Your receipt of this code is subject to any non-disclosure
 * (or similar) agreement between you and AWS.
 */

#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

#include "api_cfg/core_mqtt_config.h"

#define LIBRARY_LOG_NAME  "coreMQTT"
#define LIBRARY_LOG_LEVEL LOG_WARN
#include "../csdk_logging/logging.h"

#define MQTT_RECV_POLLING_TIMEOUT_MS    ( 1000 )
#define MQTT_COMMAND_CONTEXTS_POOL_SIZE ( 10 )

#endif
