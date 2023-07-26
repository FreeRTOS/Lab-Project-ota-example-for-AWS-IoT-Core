/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef _TRANSPORT_WRAPPER_H_
#define _TRANSPORT_WRAPPER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "transport_interface.h"

void pfTransport_tlsInit( TransportInterface_t * transport );

bool pfTransport_tlsConnect( const char * endpoint, size_t endpointLength );

void pfTransport_tlsDisconnect( void );

#endif
