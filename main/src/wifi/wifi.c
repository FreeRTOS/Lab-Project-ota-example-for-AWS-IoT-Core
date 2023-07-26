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
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "key_value_store/key_value_store.h"
#include "wifi/wifi.h"

#define WIFI_CONNECTED_BIT         BIT0
#define WIFI_FAIL_BIT              BIT1

#define CONFIG_KEY_WIFI_SSID       "SSID"
#define CONFIG_KEY_WIFI_PASS       "Passphrase"
#define CONFIG_KEY_WIFI_SSID_LEN   ( sizeof( CONFIG_KEY_WIFI_SSID ) - 1U )
#define CONFIG_KEY_WIFI_PASS_LEN   ( sizeof( CONFIG_KEY_WIFI_PASS ) - 1U )
#define CONFIG_WIFI_SSID_BUFF_SIZE ( 32U )
#define CONFIG_WIFI_PASS_BUFF_SIZE ( 64U )

static bool wifiConnected = false;
static EventGroupHandle_t wifiEventGroup;

void stationEventHandler( void * arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void * event_data )
{
    if( event_base == WIFI_EVENT )
    {
        switch( event_id )
        {
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI( "PF_WIFI", "Station connected event" );
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI( "PF_WIFI", "Station disconnected event" );
                wifiConnected = false;
                xEventGroupSetBits( wifiEventGroup, WIFI_FAIL_BIT );
                /* TODO: Do something when we're disconnected */
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI( "PF_WIFI", "Station started event" );
                wifiConnected = false;
                break;
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI( "PF_WIFI", "WiFi scan completed event" );
                break;
            default:
                ESP_LOGE( "PF_WIFI",
                          "Unknown WiFi event_id occurred: %d",
                          ( int ) event_id );
                break;
        }
    }
    else if( event_base == IP_EVENT )
    {
        switch( event_id )
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI( "PF_WIFI", "Station acquired IP address event" );
                xEventGroupSetBits( wifiEventGroup, WIFI_CONNECTED_BIT );
                wifiConnected = true;
                break;
            default:
                ESP_LOGE( "PF_WIFI",
                          "Unknown IP event_id occurred: %d",
                          ( int ) event_id );
                break;
        }
    }
}

void pfWifi_init( void )
{
    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    esp_event_handler_instance_t instanceAnyId = NULL;
    esp_event_handler_instance_t instanceGotIp = NULL;

    wifiEventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK( esp_wifi_init( &initConfig ) );

    ESP_ERROR_CHECK( esp_event_handler_instance_register( WIFI_EVENT,
                                                          ESP_EVENT_ANY_ID,
                                                          &stationEventHandler,
                                                          NULL,
                                                          &instanceAnyId ) );
    ESP_ERROR_CHECK( esp_event_handler_instance_register( IP_EVENT,
                                                          IP_EVENT_STA_GOT_IP,
                                                          &stationEventHandler,
                                                          NULL,
                                                          &instanceGotIp ) );

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

bool pfWifi_startNetwork( void )
{
    bool success = true;
    wifi_config_t wifiConfig = { 0 };
    uint8_t ssid[ CONFIG_WIFI_SSID_BUFF_SIZE ] = { 0 };
    size_t ssidLength = CONFIG_WIFI_SSID_BUFF_SIZE;
    uint8_t passphrase[ CONFIG_WIFI_PASS_BUFF_SIZE ] = { 0 };
    size_t passphraseLength = CONFIG_WIFI_PASS_BUFF_SIZE;

    if( !pfKvs_getKeyValue( CONFIG_KEY_WIFI_SSID,
                            CONFIG_KEY_WIFI_SSID_LEN,
                            ssid,
                            &ssidLength ) )
    {
        ESP_LOGE( "PF_WIFI",
                  "Failed to retrieve configuration:SSID. Please set this "
                  "value." );
        success = false;
    }

    if( success && !pfKvs_getKeyValue( CONFIG_KEY_WIFI_PASS,
                                       CONFIG_KEY_WIFI_PASS_LEN,
                                       passphrase,
                                       &passphraseLength ) )
    {
        ESP_LOGE( "PF_WIFI",
                  "Failed to retrieve configuration:PASSPHRASE. Please set "
                  "this value." );
        success = false;
    }

    if( success )
    {
        memcpy( wifiConfig.sta.ssid, ssid, ssidLength );
        memcpy( wifiConfig.sta.password, passphrase, passphraseLength );
        if( ESP_OK != esp_wifi_set_config( WIFI_IF_STA, &wifiConfig ) )
        {
            ESP_LOGE( "PF_WIFI", "Failed to set WiFi config" );
            success = false;
        }
    }

    if( success )
    {
        EventBits_t waitBits;
        ESP_LOGI( "PF_WIFI",
                  "Connecting to SSID=%.*s",
                  ( int ) ssidLength,
                  ssid );

        xEventGroupClearBits( wifiEventGroup,
                              WIFI_CONNECTED_BIT | WIFI_FAIL_BIT );

        esp_wifi_connect();

        waitBits = xEventGroupWaitBits( wifiEventGroup,
                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                        pdFALSE,
                                        pdFALSE,
                                        portMAX_DELAY );

        if( waitBits & WIFI_CONNECTED_BIT )
        {
            ESP_LOGI( "PF_WIFI",
                      "Successfully connected to SSID=%.*s",
                      ( int ) ssidLength,
                      ssid );
        }
        else
        {
            ESP_LOGI( "PF_WIFI",
                      "Failed to connect to SSID=%.*s",
                      ( int ) ssidLength,
                      ssid );
            success = false;
        }
    }

    return success;
}

bool pfWifi_stopNetwork( void )
{
    esp_err_t disconnectStatus = esp_wifi_disconnect();
    switch( disconnectStatus )
    {
        case ESP_ERR_WIFI_NOT_INIT:
            ESP_LOGE( "PF_WIFI", "Disconnect failed. Wifi not initialized" );
            break;
        case ESP_ERR_WIFI_NOT_STARTED:
            ESP_LOGE( "PF_WIFI", "Disconnect failed. Wifi not started" );
            break;
        case ESP_FAIL:
            ESP_LOGE( "PF_WIFI", "Disconnect failed. Internal error" );
            break;
        case ESP_OK:
            ESP_LOGI( "PF_WIFI", "Wifi disconnected" );
            break;
        default:
            ESP_LOGE( "PF_WIFI", "Disconnect failed. Unknown error" );
            break;
    }

    return disconnectStatus == ESP_OK;
}

bool pfWifi_networkConnected( void )
{
    return wifiConnected;
}
