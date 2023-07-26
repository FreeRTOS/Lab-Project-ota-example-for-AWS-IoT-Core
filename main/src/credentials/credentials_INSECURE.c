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
#include "credentials/credentials_INSECURE.h"
#include "esp_err.h"
#include "esp_log.h"
#include "key_value_store/key_value_store.h"

#define INSECURE_CONFIG_KEY_PRIVATEKEY "PrivateKey"
#define CONFIG_KEY_CERT                "Certificate"
#define CONFIG_KEY_ROOTCA              "RootCA"
#define CONFIG_KEY_THINGNAME           "ThingName"
#define CONFIG_KEY_WIFI_SSID           "SSID"
#define CONFIG_KEY_WIFI_PASS           "Passphrase"
#define CONFIG_KEY_ENDPOINT            "Endpoint"

#define INSECURE_CONFIG_KEY_PRIVATEKEY_LEN \
    ( sizeof( INSECURE_CONFIG_KEY_PRIVATEKEY ) - 1U )
#define CONFIG_KEY_CERT_LEN      ( sizeof( CONFIG_KEY_CERT ) - 1U )
#define CONFIG_KEY_ROOTCA_LEN    ( sizeof( CONFIG_KEY_ROOTCA ) - 1U )
#define CONFIG_KEY_THINGNAME_LEN ( sizeof( CONFIG_KEY_THINGNAME ) - 1U )
#define CONFIG_KEY_WIFI_SSID_LEN ( sizeof( CONFIG_KEY_WIFI_SSID ) - 1U )
#define CONFIG_KEY_WIFI_PASS_LEN ( sizeof( CONFIG_KEY_WIFI_PASS ) - 1U )
#define CONFIG_KEY_ENDPOINT_LEN  ( sizeof( CONFIG_KEY_ENDPOINT ) - 1U )

void pfCred_getThingName( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable ThingName for testing purposes
    // Production devices should not have a configurable ThingName
    if( !pfKvs_getKeyValue( CONFIG_KEY_THINGNAME,
                            CONFIG_KEY_THINGNAME_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getSsid( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable certificate for testing purposes
    if( !pfKvs_getKeyValue( CONFIG_KEY_WIFI_SSID,
                            CONFIG_KEY_WIFI_SSID_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getPassphrase( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable certificate for testing purposes
    if( !pfKvs_getKeyValue( CONFIG_KEY_WIFI_PASS,
                            CONFIG_KEY_WIFI_PASS_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getCertificate( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable certificate for testing purposes
    if( !pfKvs_getKeyValue( CONFIG_KEY_CERT,
                            CONFIG_KEY_CERT_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getPrivateKey( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable private key for testing purposes
    // A production device's private key should be on a HSM and inaccessible
    if( !pfKvs_getKeyValue( INSECURE_CONFIG_KEY_PRIVATEKEY,
                            INSECURE_CONFIG_KEY_PRIVATEKEY_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getRootCa( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable certificate for testing purposes
    if( !pfKvs_getKeyValue( CONFIG_KEY_ROOTCA,
                            CONFIG_KEY_ROOTCA_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}

void pfCred_getEndpoint( char * buffer, size_t * bufferLength )
{
    // The example platform has a configurable certificate for testing purposes
    if( !pfKvs_getKeyValue( CONFIG_KEY_ENDPOINT,
                            CONFIG_KEY_ENDPOINT_LEN,
                            ( uint8_t * ) buffer,
                            bufferLength ) )
    {
        ESP_LOGE( "PF_CRED", "Device not provisioned." );
        assert( false && "not provisioned" );
    }
}
