// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stddef.h>

/* Transport includes. */
#include "transport/transport.h"
#include "transport/openssl_posix.h"

static NetworkContext_t networkContext = { 0 };
static OpensslParams_t opensslParams = { 0 };

#define TRANSPORT_TIMEOUT_MS ( 750U )

void transport_tlsInit( TransportInterface_t * transport )
{
    transport->send = Openssl_Send;
    transport->recv = Openssl_Recv;
    transport->pNetworkContext = &networkContext;

    OPENSSL_init_crypto( OPENSSL_INIT_ADD_ALL_CIPHERS |
                             OPENSSL_INIT_ADD_ALL_DIGESTS |
                             OPENSSL_INIT_LOAD_CONFIG,
                         NULL );
}

bool transport_tlsConnect( const char * endpoint, size_t endpointLength )
{
    ServerInfo_t serverInfo;
    OpensslCredentials_t opensslCredentials = { 0 };
    OpensslStatus_t opensslStatus = OPENSSL_SUCCESS;

    serverInfo.pHostName = endpoint;
    serverInfo.hostNameLength = endpointLength;
    serverInfo.port = 8883U;
    /*
    char pCert[ CERT_BUFF_SIZE ] = { 0 };
    size_t certSize = CERT_BUFF_SIZE;
    char pRootCA[ ROOTCA_BUFF_SIZE ] = { 0 };
    size_t rootCASize = ROOTCA_BUFF_SIZE;
    char pPrivateKey[ CERT_BUFF_SIZE ] = { 0 };
    size_t keyLength = CERT_BUFF_SIZE;

    // TODO: Get all the credentials

    opensslCredentials.sniHostName = endpoint;
    opensslCredentials.pClientCertBuffer = pCert;
    opensslCredentials.clientCertLength = certSize;
    opensslCredentials.pRootCaBuffer = pRootCA;
    opensslCredentials.rootCaLength = rootCASize;
    opensslCredentials.pPrivateKeyBuffer = pPrivateKey;
    opensslCredentials.privateKeyLength = keyLength;
    */

    networkContext.pParams = &opensslParams;

    opensslStatus = Openssl_Connect( &networkContext,
                                     &serverInfo,
                                     &opensslCredentials,
                                     TRANSPORT_TIMEOUT_MS,
                                     TRANSPORT_TIMEOUT_MS );

    return opensslStatus == OPENSSL_SUCCESS;
}

void transport_tlsDisconnect( void )
{
    if( networkContext.pParams != NULL )
    {
        ( void ) Openssl_Disconnect( &networkContext );
    }
}
