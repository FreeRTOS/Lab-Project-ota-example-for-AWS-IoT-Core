/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * @file clock.h
 * @brief Time-related functions used by demos and tests in this SDK.
 */

#ifndef CLOCK_H_
#define CLOCK_H_

/* Standard includes. */
#include <stdint.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/**
 * @brief The timer query function.
 *
 * This function returns the elapsed time.
 *
 * @return Time in milliseconds.
 */
uint32_t Clock_GetTimeMs( void );

/**
 * @brief Millisecond sleep function.
 *
 * @param[in] sleepTimeMs milliseconds to sleep.
 */
void Clock_SleepMs( uint32_t sleepTimeMs );

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* ifndef CLOCK_H_ */
