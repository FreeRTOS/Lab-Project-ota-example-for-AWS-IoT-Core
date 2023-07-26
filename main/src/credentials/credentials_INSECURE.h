/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef _PF_CREDENTIALS_H_
#define _PF_CREDENTIALS_H_

#include <stddef.h>

void pfCred_getThingName( char * buffer, size_t * bufferLength );

void pfCred_getSsid( char * buffer, size_t * bufferLength );

void pfCred_getPassphrase( char * buffer, size_t * bufferLength );

void pfCred_getCertificate( char * buffer, size_t * bufferLength );

void pfCred_getPrivateKey( char * buffer, size_t * bufferLength );

void pfCred_getRootCa( char * buffer, size_t * bufferLength );

void pfCred_getEndpoint( char * buffer, size_t * bufferLength );

#endif
