// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Transport includes. */
#include "transport/openssl_posix.h"
#include "transport_wrapper.h"

static NetworkContext_t networkContext = { 0 };
static OpensslParams_t opensslParams = { 0 };

#define TRANSPORT_TIMEOUT_MS ( 750U )

#define MAX_FILE_SIZE        4096U

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

bool transport_tlsConnect( char * certificateFilePath,
                           char * privateKeyFilePath,
                           char * rootCAFilePath,
                           char * endpoint )
{
    ServerInfo_t serverInfo;
    OpensslCredentials_t opensslCredentials = { 0 };
    OpensslStatus_t opensslStatus = OPENSSL_SUCCESS;
    char certificate[ MAX_FILE_SIZE ] = { 0 };
    char rootCA[ MAX_FILE_SIZE ] = { 0 };
    char privateKey[ MAX_FILE_SIZE ] = { 0 };

    FILE * certificateFile = fopen( certificateFilePath, "r" );
    if( certificateFile == NULL )
    {
        printf( "Error opening certificate file: %s\n", certificateFilePath );
        assert( false );
    }
    size_t certificateLength = fread( certificate,
                                      sizeof( char ),
                                      MAX_FILE_SIZE,
                                      certificateFile );
    fclose( certificateFile );
    certificate[ certificateLength ] = '\0';

    FILE * privateKeyFile = fopen( privateKeyFilePath, "r" );
    if( privateKeyFile == NULL )
    {
        printf( "Error opening private key file: %s\n", privateKeyFilePath );
        assert( false );
    }
    size_t privateKeyLength = fread( privateKey,
                                     sizeof( char ),
                                     MAX_FILE_SIZE,
                                     privateKeyFile );
    fclose( privateKeyFile );
    privateKey[ privateKeyLength ] = '\0';

    FILE * rootCAFile = fopen( rootCAFilePath, "r" );
    if( rootCAFile == NULL )
    {
        printf( "Error opening root CA file: %s\n", rootCAFilePath );
        assert( false );
    }
    size_t rootCALength = fread( rootCA,
                                 sizeof( char ),
                                 MAX_FILE_SIZE,
                                 rootCAFile );
    fclose( rootCAFile );
    rootCA[ rootCALength ] = '\0';

    opensslCredentials.sniHostName = endpoint;
    opensslCredentials.pClientCertBuffer = certificate;
    opensslCredentials.clientCertLength = certificateLength;
    opensslCredentials.pRootCaBuffer = rootCA;
    opensslCredentials.rootCaLength = rootCALength;
    opensslCredentials.pPrivateKeyBuffer = privateKey;
    opensslCredentials.privateKeyLength = privateKeyLength;

    serverInfo.pHostName = endpoint;
    serverInfo.hostNameLength = strlen( endpoint );
    serverInfo.port = 8883U;

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
