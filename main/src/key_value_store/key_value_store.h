/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef _PF_KEY_VALUE_STORE_H_
#define _PF_KEY_VALUE_STORE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool pfKvs_writeKeyValue( const char * key,
                          size_t keyLength,
                          const uint8_t * value,
                          size_t valueLength );

bool pfKvs_getKeyValue( const char * key,
                        size_t keyLength,
                        uint8_t * value,
                        size_t * valueLength );

#endif
