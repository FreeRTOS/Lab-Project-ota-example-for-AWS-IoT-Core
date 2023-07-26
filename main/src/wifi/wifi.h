/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#ifndef _PF_WIFI_H_
#define _PF_WIFI_H_

#include <stdbool.h>

void pfWifi_init( void );

bool pfWifi_startNetwork( void );

bool pfWifi_stopNetwork( void );

bool pfWifi_networkConnected( void );

#endif
