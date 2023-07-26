/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef LOGGING_STACK_H
#define LOGGING_STACK_H

#define LOG_NONE  0U
#define LOG_ERROR 1U
#define LOG_WARN  2U
#define LOG_INFO  3U
#define LOG_DEBUG 4U

#ifndef LIBRARY_LOG_NAME
    #error "Please define LIBRARY_LOG_NAME for the compilation unit."
#endif

#ifndef LIBRARY_LOG_LEVEL
    #error "Please define LIBRARY_LOG_LEVEL for the compilation unit."
#elif LIBRARY_LOG_LEVEL == LOG_NONE
    #define LOG_LOCAL_LEVEL ESP_LOG_NONE
#elif LIBRARY_LOG_LEVEL == LOG_ERROR
    #define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#elif LIBRARY_LOG_LEVEL == LOG_WARN
    #define LOG_LOCAL_LEVEL ESP_LOG_WARN
#elif LIBRARY_LOG_LEVEL == LOG_INFO
    #define LOG_LOCAL_LEVEL ESP_LOG_INFO
#elif LIBRARY_LOG_LEVEL == LOG_DEBUG
    #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#else
    #error "Please define LIBRARY_LOG_LEVEL to a valid value."
#endif

#include "esp_log.h"

#define LOG_UNPACK( ... )      __VA_ARGS__
#define LOG_GENERIC( fn, arg ) fn( LIBRARY_LOG_NAME, LOG_UNPACK arg )

#define LogError( arg )        LOG_GENERIC( ESP_LOGE, arg )
#define LogWarn( arg )         LOG_GENERIC( ESP_LOGW, arg )
#define LogInfo( arg )         LOG_GENERIC( ESP_LOGI, arg )
#define LogDebug( arg )        LOG_GENERIC( ESP_LOGD, arg )

#endif
