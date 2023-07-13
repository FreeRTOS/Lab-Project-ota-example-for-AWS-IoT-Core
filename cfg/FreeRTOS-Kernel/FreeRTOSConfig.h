/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <assert.h>
#include <pthread.h>

#define configASSERT                         assert

#define configMINIMAL_STACK_SIZE             ( PTHREAD_STACK_MIN )
#define configMAX_PRIORITIES                 ( 32 )
#define configUSE_PREEMPTION                 1
#define configUSE_IDLE_HOOK                  0
#define configUSE_TICK_HOOK                  0
#define configUSE_16_BIT_TICKS               0
#define configUSE_RECURSIVE_MUTEXES          1
#define configUSE_MUTEXES                    1
#define configUSE_TIMERS                     1
#define configUSE_COUNTING_SEMAPHORES        1
#define configMAX_TASK_NAME_LEN              ( 12 )
#define configTIMER_TASK_PRIORITY            ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH             20
#define configTIMER_TASK_STACK_DEPTH         ( configMINIMAL_STACK_SIZE * 2 )
#define configSUPPORT_STATIC_ALLOCATION      1
#define configSTACK_DEPTH_TYPE               uint32_t
#define configTICK_RATE_HZ                   ( 100 )

#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_xTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetIdleTaskHandle       1
#define INCLUDE_xTaskAbortDelay              1
#define INCLUDE_xQueueGetMutexHolder         1
#define INCLUDE_xTaskGetHandle               1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_uxTaskGetStackHighWaterMark2 1
#define INCLUDE_eTaskGetState                1
#define INCLUDE_xTimerPendFunctionCall       1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTaskGetCurrentTaskHandle    1

#endif
