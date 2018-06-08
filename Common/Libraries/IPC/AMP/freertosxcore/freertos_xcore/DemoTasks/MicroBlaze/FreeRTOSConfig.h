/*
    FreeRTOS V7.0.0 - Copyright (C) 2011 Real Time Engineers Ltd.


	FreeRTOS supports many tools and architectures. V7.0.0 is sponsored by:
	Atollic AB - Atollic provides professional embedded systems development
	tools for C/C++ development, code analysis and test automation.
	See http://www.atollic.com


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "xparameters.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#define configSTDOUT_STARTUP_MSG	1   /* Print a message to STDOUT once the scheduler has been started */
#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			1	/* Used as an opportunity to free blocked packets. */
#define configUSE_TICK_HOOK			0
#define configUSE_MALLOC_FAILED_HOOK	1
#define configCHECK_FOR_STACK_OVERFLOW	1	/* Stick a break-point on this function to double check task stacks. */
#define configCPU_CLOCK_HZ			( ( unsigned long ) XPAR_MICROBLAZE_FREQ )
#define configTICK_RATE_HZ			( ( portTickType ) 100 )
#define configMAX_PRIORITIES		( ( unsigned portBASE_TYPE ) 8 )
#define configMINIMAL_STACK_SIZE	8192
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 0xA00000 ) )
#define configMAX_TASK_NAME_LEN		( 5 )
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_TRACE_FACILITY	0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* OS Functionality. */
#define configUSE_RECURSIVE_MUTEXES		1
#define configUSE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES	1
#define configUSE_TIMERS				0
#define configTIMER_TASK_STACK_DEPTH	configMINIMAL_STACK_SIZE

/* Common Demo Test configuration. */
#define configTIMER_TASK_PRIORITY		7
#define configTIMER_QUEUE_LENGTH		10

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetSchedulerState	1

/* Target Specific Configuration. */
#define configSYSINTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define configSYSINTC_BASEADDR			XPAR_XPS_INTC_0_BASEADDR

#define configSYSTMR_DEVICE_ID			XPAR_XPS_TIMER_0_DEVICE_ID
#define configSYSTMR_BASEADDR			XPAR_XPS_TIMER_0_BASEADDR
#define configSYSTMR_INTR_ID			XPAR_INTC_0_TMRCTR_0_VEC_ID

#define configUART_DEVICE_ID			XPAR_UARTLITE_1_DEVICE_ID
#define configUART_INTR_ID				XPAR_INTC_0_UARTLITE_1_VEC_ID
//#define configLED_DEVICE_ID				XPAR_LEDS_8BIT_DEVICE_ID

#define configMSR_ENABLE_ICACHE 		XPAR_MICROBLAZE_0_USE_ICACHE
#define configMSR_ENABLE_DCACHE 		XPAR_MICROBLAZE_0_USE_DCACHE

#define portXPS_TIMER_INTERRUPT_ID		XPAR_XPS_INTC_0_XPS_TIMER_0_INTERRUPT_INTR
#define portISR_SIGNAL_ADDRESS			XPAR_XPS_INTC_0_BASEADDR
#define portISR_SIGNAL_MASK				0x00000001
#define portXPS_INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID

#define portGENERATE_ISR_SIGNAL			1

#endif /* FREERTOS_CONFIG_H */
