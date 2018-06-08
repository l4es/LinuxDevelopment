/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.


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

/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include "task.h"

/* Standard demo includes. */
#include "synctest.h"
#include "conctest.h"
#include "porthw.h"

/*-----------------------------------------------------------*/

int main( void )
{

	/* EXTERNAL RAM INTERRUPT HANDLER BUG WORKAROUND
	 * See: http://forums.xilinx.com/t5/EDK-and-Platform-Studio/MIcroblaze-running-from-external-SDRAM-and-interrupts/td-p/26685
	 * */
	int x=(int)&_interrupt_handler;
	*(int*)(0x10)=0xb0000000 | (((x) & 0xFFFF0000) >> 16);
	*(int*)(0x14)=0xb8080000 | (((x) & 0xFFFF));

	xPortSetupHardware();

	/* Create parallel tasks. */
	vStartSyncTestTasks( tskIDLE_PRIORITY + 1 ); // Uncomment this line for 2-task sync test
	//vStartQuadSyncTestTasks(tskIDLE_PRIORITY + 1); // Uncomment this line for 4-task sync test
	//vStartConcTestTasks( tskIDLE_PRIORITY + 1 ); // Uncomment this line for conc test tasks

	/* Start the scheduler itself. */
	vTaskStartScheduler();

    /* Should never get here unless there was not enough heap space to create
	the idle and other system tasks. */
    return 0;
}

void vApplicationIdleHook( void )
{

	/* Background functions go here */

}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Can be implemented if required, but not required in this
	environment and running this demo. */

}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( void )
{
	/* Can be implemented if required, but not required in this
	environment and running this demo. */
}

