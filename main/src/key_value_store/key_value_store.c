/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <string.h>

#include "assert.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "key_value_store/key_value_store.h"
#include "nvs.h"
#include "nvs_flash.h"

#define CONFIG_MAX_KEY_LENGTH ( 16U )

bool pfKvs_writeKeyValue( const char * key,
                          size_t keyLength,
                          const uint8_t * value,
                          size_t valueLength )
{
    nvs_handle_t xNVS;
    bool returnValue = false;
    char keyBuff[ CONFIG_MAX_KEY_LENGTH + 1U ] = { 0 };

    assert( keyLength <= CONFIG_MAX_KEY_LENGTH );
    memcpy( keyBuff, key, keyLength );

    if( ESP_OK == nvs_open( "config", NVS_READWRITE, &xNVS ) )
    {
        if( ESP_OK == nvs_set_blob( xNVS, keyBuff, value, valueLength ) )
        {
            returnValue = true;
            nvs_commit( xNVS );
        }
        else
        {
            ESP_LOGI( "PF_KVS", "Set Str Failed" );
        }
        nvs_close( xNVS );
    }
    else
    {
        ESP_LOGI( "PF_KVS", "NVS Open Failed" );
    }
    return returnValue;
}

bool pfKvs_getKeyValue( const char * key,
                        size_t keyLength,
                        uint8_t * value,
                        size_t * valueLength )
{
    nvs_handle_t xNVS;
    BaseType_t returnValue = pdFAIL;

    if( ESP_OK == nvs_open( "config", NVS_READWRITE, &xNVS ) )
    {
        if( ESP_OK == nvs_get_blob( xNVS, key, value, valueLength ) )
        {
            returnValue = pdPASS;
        }
        else
        {
            ESP_LOGI( "PF_KVS", "Get Failed %s", key );
        }
        nvs_close( xNVS );
    }
    else
    {
        ESP_LOGI( "PF_KVS", "Open Failed %s", key );
    }
    return returnValue;
}
