// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )

{
    static StaticTask_t timerTaskTCB;
    static StackType_t timerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    *ppxTimerTaskTCBBuffer = &timerTaskTCB;
    *ppxTimerTaskStackBuffer = timerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )

{
    static StaticTask_t idleTaskTCB;
    static StackType_t idleTaskStack[ configMINIMAL_STACK_SIZE ];

    *ppxIdleTaskTCBBuffer = &idleTaskTCB;
    *ppxIdleTaskStackBuffer = idleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
