/*
 * AWS IoT Device SDK for Embedded C 202108.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define LIBRARY_LOG_NAME  "Sockets"
#define LIBRARY_LOG_LEVEL LOG_INFO
#include "csdk_logging/logging.h"

/* Standard includes. */
#include <assert.h>
#include <string.h>

/* POSIX sockets includes. */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "sockets_posix.h"

/*-----------------------------------------------------------*/

/**
 * @brief Number of milliseconds in one second.
 */
#define ONE_SEC_TO_MS ( 1000 )

/**
 * @brief Number of microseconds in one millisecond.
 */
#define ONE_MS_TO_US  ( 1000 )

/*-----------------------------------------------------------*/

/**
 * @brief Resolve a host name.
 *
 * @param[in] hostName Server host name.
 * @param[in] hostNameLength Length associated with host name.
 * @param[out] listHead The output parameter to return the list containing
 * resolved DNS records.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_DNS_FAILURE,
 * #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t resolveHostName( const char * hostName,
                                       size_t hostNameLength,
                                       struct addrinfo ** listHead );

/**
 * @brief Traverse list of DNS records until a connection is established.
 *
 * @param[in] listHead List containing resolved DNS records.
 * @param[in] hostName Server host name.
 * @param[in] hostNameLength Length associated with host name.
 * @param[in] port Server port in host-order.
 * @param[out] tcpSocket The output parameter to return the created socket.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t attemptConnection( struct addrinfo * listHead,
                                         const char * hostName,
                                         size_t hostNameLength,
                                         uint16_t port,
                                         int32_t * tcpSocket );

/**
 * @brief Connect to server using the provided address record.
 *
 * @param[in, out] addrInfo Address record of the server.
 * @param[in] port Server port in host-order.
 * @param[in] tcpSocket Socket handle.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t connectToAddress( struct sockaddr * addrInfo,
                                        uint16_t port,
                                        int32_t tcpSocket );

/**
 * @brief Log possible error using errno and return appropriate status.
 *
 * @param[in] errorNumber Error number.
 *
 * @return #SOCKETS_API_ERROR, #SOCKETS_INSUFFICIENT_MEMORY,
 * #SOCKETS_INVALID_PARAMETER on error.
 */
static SocketStatus_t retrieveError( int32_t errorNumber );

/*-----------------------------------------------------------*/

static SocketStatus_t resolveHostName( const char * hostName,
                                       size_t hostNameLength,
                                       struct addrinfo ** listHead )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    int32_t dnsStatus = -1;
    struct addrinfo hints;

    assert( hostName != NULL );
    assert( hostNameLength > 0 );

    /* Unused parameter. These parameters are used only for logging. */
    ( void ) hostNameLength;

    /* Add hints to retrieve only TCP sockets in getaddrinfo. */
    ( void ) memset( &hints, 0, sizeof( hints ) );

    /* Address family of either IPv4 or IPv6. */
    hints.ai_family = AF_UNSPEC;
    /* TCP Socket. */
    hints.ai_socktype = ( int32_t ) SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Perform a DNS lookup on the given host name. */
    dnsStatus = getaddrinfo( hostName, NULL, &hints, listHead );

    if( dnsStatus != 0 )
    {
        LogError( ( "Failed to resolve DNS: Hostname=%.*s, ErrorCode=%d.",
                    ( int32_t ) hostNameLength,
                    hostName,
                    dnsStatus ) );
        returnStatus = SOCKETS_DNS_FAILURE;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t connectToAddress( struct sockaddr * addrInfo,
                                        uint16_t port,
                                        int32_t tcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    int32_t connectStatus = 0;
    char resolvedIpAddr[ INET6_ADDRSTRLEN ];
    socklen_t addrInfoLength;
    uint16_t netPort = 0;
    struct sockaddr_in * ipv4Address;
    struct sockaddr_in6 * ipv6Address;

    assert( addrInfo != NULL );
    assert( addrInfo->sa_family == AF_INET || addrInfo->sa_family == AF_INET6 );
    assert( tcpSocket >= 0 );

    /* Convert port from host byte order to network byte order. */
    netPort = htons( port );

    if( addrInfo->sa_family == ( sa_family_t ) AF_INET )
    {
        /* MISRA Rule 11.3 flags the following line for casting a pointer of
         * a object type to a pointer of a different object type. This rule
         * is suppressed because casting from a struct sockaddr pointer to
         * a struct sockaddr_in pointer is supported in POSIX and is used
         * to obtain the IP address from the address record. */
        /* coverity[misra_c_2012_rule_11_3_violation] */
        ipv4Address = ( struct sockaddr_in * ) addrInfo;
        /* Store IPv4 in string to log. */
        ipv4Address->sin_port = netPort;
        addrInfoLength = ( socklen_t ) sizeof( struct sockaddr_in );
        ( void ) inet_ntop( ( int32_t ) addrInfo->sa_family,
                            &ipv4Address->sin_addr,
                            resolvedIpAddr,
                            ( socklen_t ) sizeof( resolvedIpAddr ) );
    }
    else
    {
        /* MISRA Rule 11.3 flags the following line for casting a pointer of
         * a object type to a pointer of a different object type. This rule
         * is suppressed because casting from a struct sockaddr pointer to
         * a struct sockaddr_in6 pointer is supported in POSIX and is used
         * to obtain the IPv6 address from the address record. */
        /* coverity[misra_c_2012_rule_11_3_violation] */
        ipv6Address = ( struct sockaddr_in6 * ) addrInfo;
        /* Store IPv6 in string to log. */
        ipv6Address->sin6_port = netPort;
        addrInfoLength = ( socklen_t ) sizeof( struct sockaddr_in6 );
        ( void ) inet_ntop( ( int32_t ) addrInfo->sa_family,
                            &ipv6Address->sin6_addr,
                            resolvedIpAddr,
                            ( socklen_t ) sizeof( resolvedIpAddr ) );
    }

    LogDebug( ( "Attempting to connect to server using the resolved IP address:"
                " IP address=%s.",
                resolvedIpAddr ) );

    /* Attempt to connect. */
    connectStatus = connect( tcpSocket, addrInfo, addrInfoLength );

    if( connectStatus == -1 )
    {
        LogWarn( ( "Failed to connect to server using the resolved IP address: "
                   "IP address=%s. Error: %s",
                   resolvedIpAddr,
                   strerror( errno ) ) );
        ( void ) close( tcpSocket );
        returnStatus = SOCKETS_CONNECT_FAILURE;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t attemptConnection( struct addrinfo * listHead,
                                         const char * hostName,
                                         size_t hostNameLength,
                                         uint16_t port,
                                         int32_t * tcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_CONNECT_FAILURE;
    const struct addrinfo * index = NULL;

    assert( listHead != NULL );
    assert( hostName != NULL );
    assert( hostNameLength > 0 );
    assert( tcpSocket != NULL );

    /* Unused parameters when logging is disabled. */
    ( void ) hostName;
    ( void ) hostNameLength;

    LogDebug( ( "Attempting to connect to: Host=%.*s.",
                ( int32_t ) hostNameLength,
                hostName ) );

    /* Attempt to connect to one of the retrieved DNS records. */
    for( index = listHead; index != NULL; index = index->ai_next )
    {
        *tcpSocket = socket( index->ai_family,
                             index->ai_socktype,
                             index->ai_protocol );

        if( *tcpSocket == -1 )
        {
            continue;
        }

        /* Attempt to connect to a resolved DNS address of the host. */
        returnStatus = connectToAddress( index->ai_addr, port, *tcpSocket );

        /* If connected to an IP address successfully, exit from the loop. */
        if( returnStatus == SOCKETS_SUCCESS )
        {
            break;
        }
    }

    if( returnStatus == SOCKETS_SUCCESS )
    {
        LogDebug( ( "Established TCP connection: Server=%.*s.",
                    ( int32_t ) hostNameLength,
                    hostName ) );
    }
    else
    {
        LogError( ( "Could not connect to any resolved IP address from %.*s.",
                    ( int32_t ) hostNameLength,
                    hostName ) );
    }

    freeaddrinfo( listHead );

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t retrieveError( int32_t errorNumber )
{
    SocketStatus_t returnStatus = SOCKETS_API_ERROR;

    LogError( ( "A transport error occured: %s.", strerror( errorNumber ) ) );

    if( ( errorNumber == ENOMEM ) || ( errorNumber == ENOBUFS ) )
    {
        returnStatus = SOCKETS_INSUFFICIENT_MEMORY;
    }
    else if( ( errorNumber == ENOTSOCK ) || ( errorNumber == EDOM ) ||
             ( errorNumber == EBADF ) )
    {
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else. */
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

SocketStatus_t Sockets_Connect( int32_t * tcpSocket,
                                const ServerInfo_t * serverInfo,
                                uint32_t sendTimeoutMs,
                                uint32_t recvTimeoutMs )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    struct addrinfo * listHead = NULL;
    struct timeval transportTimeout;
    int32_t setTimeoutStatus = -1;

    if( serverInfo == NULL )
    {
        LogError( ( "Parameter check failed: serverInfo is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( serverInfo->hostName == NULL )
    {
        LogError( ( "Parameter check failed: serverInfo->hostName is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( tcpSocket == NULL )
    {
        LogError( ( "Parameter check failed: tcpSocket is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( serverInfo->hostNameLength == 0UL )
    {
        LogError( ( "Parameter check failed: hostNameLength must be greater "
                    "than 0." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else. */
    }

    if( returnStatus == SOCKETS_SUCCESS )
    {
        returnStatus = resolveHostName( serverInfo->hostName,
                                        serverInfo->hostNameLength,
                                        &listHead );
    }

    if( returnStatus == SOCKETS_SUCCESS )
    {
        returnStatus = attemptConnection( listHead,
                                          serverInfo->hostName,
                                          serverInfo->hostNameLength,
                                          serverInfo->port,
                                          tcpSocket );
    }

    /* Set the send timeout. */
    if( returnStatus == SOCKETS_SUCCESS )
    {
        transportTimeout.tv_sec = ( ( ( int64_t ) sendTimeoutMs ) /
                                    ONE_SEC_TO_MS );
        transportTimeout.tv_usec = ( ONE_MS_TO_US *
                                     ( ( ( int64_t ) sendTimeoutMs ) %
                                       ONE_SEC_TO_MS ) );

        setTimeoutStatus = setsockopt( *tcpSocket,
                                       SOL_SOCKET,
                                       SO_SNDTIMEO,
                                       &transportTimeout,
                                       ( socklen_t ) sizeof(
                                           transportTimeout ) );

        if( setTimeoutStatus < 0 )
        {
            LogError( ( "Setting socket send timeout failed." ) );
            returnStatus = retrieveError( errno );
        }
    }

    /* Set the receive timeout. */
    if( returnStatus == SOCKETS_SUCCESS )
    {
        transportTimeout.tv_sec = ( ( ( int64_t ) recvTimeoutMs ) /
                                    ONE_SEC_TO_MS );
        transportTimeout.tv_usec = ( ONE_MS_TO_US *
                                     ( ( ( int64_t ) recvTimeoutMs ) %
                                       ONE_SEC_TO_MS ) );

        setTimeoutStatus = setsockopt( *tcpSocket,
                                       SOL_SOCKET,
                                       SO_RCVTIMEO,
                                       &transportTimeout,
                                       ( socklen_t ) sizeof(
                                           transportTimeout ) );

        if( setTimeoutStatus < 0 )
        {
            LogError( ( "Setting socket receive timeout failed." ) );
            returnStatus = retrieveError( errno );
        }
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

SocketStatus_t Sockets_Disconnect( int32_t tcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;

    if( tcpSocket > 0 )
    {
        ( void ) shutdown( tcpSocket, SHUT_RDWR );
        ( void ) close( tcpSocket );
    }
    else
    {
        LogError( ( "Parameter check failed: tcpSocket was negative." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/
