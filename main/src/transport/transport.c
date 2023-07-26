/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include "transport/transport.h"
#include "assert.h"
#include "credentials/credentials_INSECURE.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TRANSPORT_TIMEOUT_MS ( 4000U )
#define MQTT_BROKER_PORT     8883
#define MAX_CERT_LENGTH      ( 4096U )

struct NetworkContext
{
    esp_tls_t * pTlsContext;
};

static int32_t tlsRecv( NetworkContext_t * pNetworkContext,
                        void * pBuffer,
                        size_t bytesToRecv );

static int32_t tlsSend( NetworkContext_t * pNetworkContext,
                        const void * pBuffer,
                        size_t bytesToSend );

static NetworkContext_t networkContext = { 0 };

static int32_t tlsRecv( NetworkContext_t * pNetworkContext,
                        void * pBuffer,
                        size_t bytesToRecv )
{
    ssize_t tlsStatus;

    tlsStatus = esp_tls_conn_read( pNetworkContext->pTlsContext,
                                   pBuffer,
                                   bytesToRecv );

    /* Mark these set of errors as a timeout. The libraries may retry read
     * on these errors. */
    if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        tlsStatus = 0;
    }

    return tlsStatus;
}

static int32_t tlsSend( NetworkContext_t * pNetworkContext,
                        const void * pBuffer,
                        size_t bytesToSend )
{
    ssize_t tlsStatus = 0;
    size_t bytesWritten = 0;
    TickType_t timeoutTicks = pdMS_TO_TICKS( TRANSPORT_TIMEOUT_MS );
    TimeOut_t numTicks;

    vTaskSetTimeOutState( &numTicks );

    while( ( tlsStatus >= 0 ) && ( bytesWritten < bytesToSend ) )
    {
        tlsStatus = esp_tls_conn_write( pNetworkContext->pTlsContext,
                                        pBuffer,
                                        bytesToSend );
        if( tlsStatus > 0 )
        {
            bytesWritten += tlsStatus;
        }

        if( xTaskCheckForTimeOut( &numTicks, &timeoutTicks ) != pdFALSE )
        {
            ESP_LOGE( "PF_TRANSPORT",
                      "Timed out trying to send whole bytes through transport "
                      "interface, bytes sent = %d.",
                      bytesWritten );
            break;
        }
    }

    if( tlsStatus >= 0 )
    {
        /** Return total number of bytes sent. */
        tlsStatus = bytesWritten;
    }
    else if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
             ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
             ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        /* Mark these set of errors as a timeout. The libraries may retry read
         * on these errors. */
        tlsStatus = 0;
    }
    else
    {
        /* Empty Else */
    }

    return tlsStatus;
}

void pfTransport_tlsInit( TransportInterface_t * transport )
{
    *transport = ( TransportInterface_t ) {
        .recv = tlsRecv,
        .send = tlsSend,
        .pNetworkContext = &networkContext,
    };
}

bool pfTransport_tlsConnect( const char * endpoint, size_t endpointLength )
{
    int networkStatus = -1;

    char * cert = malloc( MAX_CERT_LENGTH + 1 );
    char * pkey = malloc( MAX_CERT_LENGTH + 1 );
    char * rootCA = malloc( MAX_CERT_LENGTH + 1 );

    assert( cert != NULL );
    assert( pkey != NULL );
    assert( rootCA != NULL );

    size_t certLength = MAX_CERT_LENGTH;
    size_t privateKeyLength = MAX_CERT_LENGTH;
    size_t rootCALength = MAX_CERT_LENGTH;

    pfCred_getCertificate( cert, &certLength );
    cert[ certLength ] = '\0';

    pfCred_getPrivateKey( pkey, &privateKeyLength );
    pkey[ privateKeyLength ] = '\0';

    pfCred_getRootCa( rootCA, &rootCALength );
    rootCA[ rootCALength ] = '\0';

    esp_tls_cfg_t tls_cfg = { .cacert_buf = ( const unsigned char * ) rootCA,
                              .cacert_bytes = rootCALength + 1,
                              .clientcert_buf = ( const unsigned char * ) cert,
                              .clientcert_bytes = certLength + 1,
                              .clientkey_buf = ( const unsigned char * ) pkey,
                              .clientkey_bytes = privateKeyLength + 1,
                              .non_block = true,
                              .timeout_ms = TRANSPORT_TIMEOUT_MS };

    networkContext.pTlsContext = esp_tls_init();
    assert( networkContext.pTlsContext != NULL );

    /* Attempt to create a mutually authenticated TLS connection. */
    ESP_LOGD( "PF_TRANSPORT", "Starting the TLS connection" );

    networkStatus = esp_tls_conn_new_sync( endpoint,
                                           endpointLength,
                                           MQTT_BROKER_PORT,
                                           &( tls_cfg ),
                                           networkContext.pTlsContext );

    free( cert );
    free( pkey );
    free( rootCA );

    return networkStatus == 1;
}

void pfTransport_tlsDisconnect( void )
{
    ESP_LOGD( "PF_TRANSPORT", "Disconnecting the TLS connection" );
    esp_tls_conn_destroy( networkContext.pTlsContext );
    networkContext.pTlsContext = NULL;
}
