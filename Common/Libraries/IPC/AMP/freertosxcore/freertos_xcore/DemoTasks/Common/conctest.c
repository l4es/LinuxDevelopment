/*
 * conctest.c
 *
 *  Created on: 14 Aug 2011
 *      Author: James
 */

#include <stdlib.h>
#include <stdio.h>
#include "xuartlite_l.h"

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "xparameters.h"

/* Task function prototypes */
static portTASK_FUNCTION_PROTO( vConcTestMonitor, pvParameters );
static portTASK_FUNCTION_PROTO( vConcTest0, pvParameters );
static portTASK_FUNCTION_PROTO( vConcTest1, pvParameters );

static volatile portBASE_TYPE calc1Done = 0;
static volatile portBASE_TYPE calc2Done = 0;

static const portBASE_TYPE CHECK_TASK_DELAY_PERIOD = 100;

void vStartConcTestTasks( unsigned portBASE_TYPE uxPriority ) {

	xTaskCreate( 0, vConcTestMonitor, ( signed char * ) "T1", configMINIMAL_STACK_SIZE, NULL, uxPriority, NULL );
	xTaskCreate( 0, vConcTest0, ( signed char * ) "T2", configMINIMAL_STACK_SIZE, NULL, uxPriority, NULL );
	xTaskCreate( 1, vConcTest1, ( signed char * ) "T3", configMINIMAL_STACK_SIZE, NULL, uxPriority, NULL );

}

static portTASK_FUNCTION( vConcTestMonitor, pvParameters )
{

	for (;;) {

		if ((calc1Done == 10000000) && (calc2Done == 10000000)) {

			portTickType ticks = xTaskGetTickCount();
			printf("Calculations complete in %lu ticks!\n", ticks);
			for(;;) {}

		} else {

			vTaskDelay(((systemTaskParameters *)(pvParameters))->taskHandle, CHECK_TASK_DELAY_PERIOD);

		}

	}

}

static portTASK_FUNCTION( vConcTest0, pvParameters )
{

	for (;;) {

		portBASE_TYPE i = 0;

		calc1Done = 0;
		for (i=0;i<10000000;i++) {

			calc1Done++;

		}

		for (;;) {}

	}

}

static portTASK_FUNCTION( vConcTest1, pvParameters )
{

	for (;;) {

		portBASE_TYPE i = 0;

		calc2Done = 0;
		for (i=0;i<10000000;i++) {

			calc2Done++;

		}

		for (;;) {}

	}

}
