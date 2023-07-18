/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * @file clock_posix.c
 * @brief Implementation of the functions in clock.h for POSIX systems.
 */

/* POSIX include. Allow the default POSIX header to be overridden. */
#ifdef POSIX_TIME_HEADER
    #include POSIX_TIME_HEADER
#else
    #include <time.h>
#endif

/* Platform clock include. */
#include "clock.h"

/*
 * Time conversion constants.
 */
#define NANOSECONDS_PER_MILLISECOND \
    ( 1000000L ) /**< @brief Nanoseconds per millisecond. */
#define MILLISECONDS_PER_SECOND \
    ( 1000L ) /**< @brief Milliseconds per second. */

/*-----------------------------------------------------------*/

uint32_t Clock_GetTimeMs( void )
{
    int64_t timeMs;
    struct timespec timeSpec;

    /* Get the MONOTONIC time. */
    ( void ) clock_gettime( CLOCK_MONOTONIC, &timeSpec );

    /* Calculate the milliseconds from timespec. */
    timeMs = ( timeSpec.tv_sec * MILLISECONDS_PER_SECOND ) +
             ( timeSpec.tv_nsec / NANOSECONDS_PER_MILLISECOND );

    /* Libraries need only the lower 32 bits of the time in milliseconds, since
     * this function is used only for calculating the time difference.
     * Also, the possible overflows of this time value are handled by the
     * libraries. */
    return ( uint32_t ) timeMs;
}

/*-----------------------------------------------------------*/

void Clock_SleepMs( uint32_t sleepTimeMs )
{
    /* Convert parameter to timespec. */
    struct timespec sleepTime = { 0 };

    sleepTime.tv_sec = ( ( time_t ) sleepTimeMs /
                         ( time_t ) MILLISECONDS_PER_SECOND );
    sleepTime.tv_nsec = ( ( int64_t ) sleepTimeMs % MILLISECONDS_PER_SECOND ) *
                        NANOSECONDS_PER_MILLISECOND;

    /* High resolution sleep. */
    ( void ) nanosleep( &sleepTime, NULL );
}
