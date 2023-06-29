/*
 * Copyright of Amazon Web Services, Inc. (AWS) 2022
 *
 * This code is licensed under the AWS Intellectual Property License, which can
 * be found here: https://aws.amazon.com/legal/aws-ip-license-terms/; provided
 * that AWS grants you a limited, royalty-free, revocable, non-exclusive,
 * non-sublicensable, non-transferrable license to modify the code for internal
 * testing purposes. Your receipt of this code is subject to any non-disclosure
 * (or similar) agreement between you and AWS.
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
