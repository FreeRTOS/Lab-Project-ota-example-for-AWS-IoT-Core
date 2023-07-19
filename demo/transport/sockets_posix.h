/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef SOCKETS_POSIX_H_
#define SOCKETS_POSIX_H_

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/* Transport interface include. */
#include "transport_interface.h"

/**
 * @brief TCP Connect / Disconnect return status.
 */
typedef enum SocketStatus
{
    SOCKETS_SUCCESS = 0,         /**< Function successfully completed. */
    SOCKETS_INVALID_PARAMETER,   /**< At least one parameter was invalid. */
    SOCKETS_INSUFFICIENT_MEMORY, /**< Insufficient memory required to
                                    establish connection. */
    SOCKETS_API_ERROR,      /**< A call to a system API resulted in an internal
                               error. */
    SOCKETS_DNS_FAILURE,    /**< Resolving hostname of server failed. */
    SOCKETS_CONNECT_FAILURE /**< Initial connection to the server failed. */
} SocketStatus_t;

/**
 * @brief Information on the remote server for connection setup.
 */
typedef struct ServerInfo
{
    const char * hostName; /**< @brief Server host name. */
    size_t hostNameLength; /**< @brief Length of the server host name. */
    uint16_t port;         /**< @brief Server port in host-order. */
} ServerInfo_t;

/**
 * @brief Establish a connection to server.
 *
 * @param[out] tcpSocket The output parameter to return the created socket
 * descriptor.
 * @param[in] serverInfo Server connection info.
 * @param[in] sendTimeoutMs Timeout for transport send.
 * @param[in] recvTimeoutMs Timeout for transport recv.
 *
 * @note A timeout of 0 means infinite timeout.
 *
 * @return #SOCKETS_SUCCESS if successful;
 * #SOCKETS_INVALID_PARAMETER, #SOCKETS_DNS_FAILURE,
 * #SOCKETS_CONNECT_FAILURE on error.
 */
SocketStatus_t Sockets_Connect( int32_t * tcpSocket,
                                const ServerInfo_t * serverInfo,
                                uint32_t sendTimeoutMs,
                                uint32_t recvTimeoutMs );

/**
 * @brief End connection to server.
 *
 * @param[in] tcpSocket The socket descriptor.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_INVALID_PARAMETER on
 * error.
 */
SocketStatus_t Sockets_Disconnect( int32_t tcpSocket );

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* ifndef SOCKETS_POSIX_H_ */
