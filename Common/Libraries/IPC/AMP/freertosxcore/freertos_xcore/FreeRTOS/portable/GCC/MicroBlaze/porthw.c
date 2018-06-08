/*
    MicroBlaze(TM) GCC Port for FreeRTOS

    Copyright (C) 2010 Tethers Unlimited, Inc.

    This file is part of the MicroBlaze(TM) GCC port for FreeRTOS.

    The MicroBlaze(TM) GCC port for FreeRTOS is free software: you can
    redistribute it and/or modify it under the terms of the GNU
    General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option)
    any later version.

    The MicroBlaze(TM) GCC port for FreeRTOS is distributed in the hope
    that it will be useful, but WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the MicroBlaze(TM) GCC port for FreeRTOS. If not, see
    <http://www.gnu.org/licenses/>.
    
    MicroBlaze(TM) is a registered trademark of Xilinx, Inc.

    1 tab == 4 spaces!
*/

/**
 * @file porthw.c
 * @ingroup freertosmbport
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Functions that provide the kernel with an interface to the MicroBlaze peripherals.
 * @see porthw.h
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Port/kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "xparameters.h"
#include "multicore.h"

/**
 * @var xIntc
 * @ingroup freertosmbport
 * @brief Global interrupt controller (XIntc) intstance mangaged by the kernel
 * via this port.
 */
static XIntc xIntc;

/**
 * @var xTickTimer
 * @ingroup freertosmbport
 * @brief Global timer (XTmrCtr) intstance mangaged by the kernel via this port
 * to generate the periodic tick for the kernel.
 */
static XTmrCtr xTickTimer;

portBASE_TYPE xPortSetupHardware(void)
{
	portBASE_TYPE xReturn = pdFAIL;
	
#ifdef XPAR_MICROBLAZE_USE_ICACHE 
    microblaze_invalidate_icache();
    microblaze_enable_icache();
#endif
#ifdef XPAR_MICROBLAZE_USE_DCACHE 
    microblaze_invalidate_dcache();
    microblaze_enable_dcache();
#endif

	microblaze_enable_exceptions();

	xReturn = xPortSetupInterruptController();

	return xReturn;

}

portBASE_TYPE xPortSetupInterruptController(void)
{

	XStatus status;
	portBASE_TYPE xReturn = pdFAIL;
	
	/* initialize the interrupt controller */
	status = XIntc_Initialize(&xIntc, portXPS_INTC_DEVICE_ID);

	if (status == XST_SUCCESS) {
	    /* configure the ISR to service all interrupts when a single interrupt occurs */
	    XIntc_SetIntrSvcOption(xIntc.BaseAddress, XIN_SVC_ALL_ISRS_OPTION);
	    
	    /* start the interrupt controller, note that this doesn't actually enable interrupts */
	    status = XIntc_Start(&xIntc, XIN_REAL_MODE);
	    if (status == XST_SUCCESS) {
			xReturn = pdPASS;
	    }
	}
    
    return xReturn;
}

void vPortEnableInterrupt(unsigned portCHAR ucInterruptID)
{
	XIntc_Enable(&xIntc, ucInterruptID);
	microblaze_enable_interrupts();
}

void vPortDisableInterrupt(unsigned portCHAR ucInterruptID)
{
	XIntc_Disable(&xIntc, ucInterruptID);
}

portBASE_TYPE xPortConnectInterruptHandler(unsigned portCHAR ucInterruptID, XInterruptHandler pxHandler, void *pvCallBackRef)
{
	XStatus status;
	portBASE_TYPE xReturn = pdFAIL;

	/* pass the handler information to the interrupt controller */
	status = XIntc_Connect(&xIntc, ucInterruptID, pxHandler, pvCallBackRef);

	if (status == XST_SUCCESS) {
		/* enable the connected interrupt within the interrupt controller */
		vPortEnableInterrupt(ucInterruptID);
		xReturn = pdPASS;
	}

	return xReturn;
}

void vPortDisconnectInterruptHandler(unsigned portCHAR ucInterruptID)
{
	vPortDisableInterrupt(ucInterruptID);
	XIntc_Disconnect(&xIntc, ucInterruptID);
}

portBASE_TYPE xPortSetupTickTimer(XTmrCtr_Handler pxHandler, unsigned portLONG xCounterValue)
{
	XStatus status;
	portBASE_TYPE xReturn = pdFAIL;

	/* initialize the tick timer to periodically generate an interrupt */
    status = XTmrCtr_Initialize(&xTickTimer, portXPS_TIMER_DEVICE_ID);
    if (status == XST_SUCCESS) {
	    XTmrCtr_SetOptions(&xTickTimer, portXPS_TIMER_COUNTER_NUMBER, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);
    	XTmrCtr_SetResetValue(&xTickTimer, portXPS_TIMER_COUNTER_NUMBER, xCounterValue);
		/* register and enable the timer/counter handler with the interrupt controller,
		 * then register our tick handler with the specific timer/counter */
		XTmrCtr_SetHandler(&xTickTimer, pxHandler, &xTickTimer);
		xReturn = xPortConnectInterruptHandler(portXPS_TIMER_INTERRUPT_ID, XTmrCtr_InterruptHandler, &xTickTimer);
		if (xReturn) {
			/* start the timer counting */
			XTmrCtr_Start(&xTickTimer, portXPS_TIMER_COUNTER_NUMBER);
			xReturn = pdPASS;
		}
    }
    
    return xReturn;
}

unsigned portBASE_TYPE xPortGetCurrentCPU() {

	/* Query the USER1 bits in the PVR register to determine the current core's
	 * ID.
	 * */
	pvr_t pvr_data;
	microblaze_get_pvr(&pvr_data);
	unsigned portBASE_TYPE xReturn = (unsigned portBASE_TYPE)((unsigned portCHAR)((pvr_data.pvr[0] >> 0) & 0xFF));

	return xReturn;

}
