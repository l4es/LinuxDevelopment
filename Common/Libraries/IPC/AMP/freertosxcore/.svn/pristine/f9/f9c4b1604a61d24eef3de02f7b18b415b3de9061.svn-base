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
 * @file portdbg.c
 * @ingroup freertosmbport
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Misc. debugging functions used during the development of this port.
 * 
 * @note These functions are only compiled in if portDEBUG_CONTEXT_SWITCHES is
 * enabled.
 * 
 * @attention The functions in this file depend on custom modifications to
 * to the FreeRTOS source code that are not distributed with this port. These
 * custom functions include access to a task's name and configured stack depth.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#if ( portDEBUG_CONTEXT_SWITCHES == 1 )

static const portCHAR *const pcMSRBitNames[] = {
	"BE", "IE",  "C",   "BIP", "FSL", "ICE", "DZ",  "DCE",
	"EE", "EIP", "PVR", "UM",  "UMS", "VM",  "VMS", "",
	"",   "",    "",    "",    "",    "",    "",    "",
	"",   "",    "",    "",    "",    "",    "",    "C"
};

static const portCHAR *const pcFSRBitNames[] = {
	"DO", "UF", "OF", "DZ", "IO", "", "", "",
	"",   "",   "",   "",   "",   "", "", "",
	"",   "",   "",   "",   "",   "", "", "",
	"",   "",   "",   "",   "",   "", "", ""
};

/* External global variable declarations. */
extern void *_stack, *_stack_end;				/* Pointers to the mainline stack allocated at link time. */
typedef struct tskTaskControlBlock tskTCB;		/* We don't care what is in the TCB, just that it exists as a type. See tasks.c. */
extern tskTCB * volatile pxCurrentTCB;			/* Pointer to current TCB. See tasks.c. */

portBASE_TYPE xPortDebugIsStackValid(portSTACK_TYPE *pxBase, unsigned portLONG ulStackDepth, portSTACK_TYPE *pxTop)
{
	portSTACK_TYPE *pxCeiling;
	
	pxCeiling = pxBase + (ulStackDepth - 1);
	
	if (pxBase <= pxTop && pxTop < pxCeiling) {
		return pdTRUE;
	}
	else {
		return pdFALSE;
	}
}

portBASE_TYPE xPortDebugPrintStackState(portSTACK_TYPE *pxBase, unsigned portLONG ulStackDepth, portSTACK_TYPE *pxTop)
{
	portSTACK_TYPE *pxCeiling;
	
	pxCeiling = pxBase + (ulStackDepth - 1);
	
	printf("Stack:\t\t\t(%d words)\r\n", ulStackDepth);
	printf("Base:\t0x%08X\r\n", pxBase);
	printf("\t\t\t(%d words)\r\n", pxTop - pxBase + 1);
	printf("->\t0x%08X\r\n", pxTop);
	printf("\t\t\t(%d words)\r\n", pxCeiling - pxTop);
	printf("High:\t0x%08X\r\n", pxCeiling);
	
	return xPortDebugIsStackValid(pxBase, ulStackDepth, pxTop);
}

void vPortDebugPrintBitNames(const portCHAR *pcRegName, portSTACK_TYPE xRegValue, const portCHAR **pcBitNames)
{
	portBASE_TYPE i, f, m;
	
	printf("%s:\t\t\t", pcRegName);
	for (m = 1, i = 0, f = pdFALSE; i < 32; i++, m <<= 1) {
		if (pcBitNames[i] != NULL) {
			if (xRegValue & m) {
				if (f) {
					printf(" | ");
				}
				printf("%s", pcBitNames[i]);
				f = pdTRUE;
			}
		}
	}
	printf("\r\n");
}

portBASE_TYPE xPortDebugPrintContext(tskTCB *tcb, const portCHAR *order)
{
	portBASE_TYPE xValid = pdFALSE;
	task_context_t *context;
	const portCHAR *pcState;
	
	context = (task_context_t*)*((portSTACK_TYPE*)tcb);
	
	switch (context->state) {
		case portTASK_CONTEXT_STATE_INITIALIZED:
			pcState = "Initialized";
			break;
		case portTASK_CONTEXT_STATE_YIELDED:
			pcState = "Yielded";
			break;
		case portTASK_CONTEXT_STATE_INTERRUPTED:
			pcState = "Interrupted";
			break;
		default:
			pcState = "Unknown";
			break;
	}
	
	printf("%s Context: %s (%s)\r\n", order, pcTaskNameGet(tcb), pcState);
	
	xValid = xPortDebugPrintStackState(pxTaskStackGetBase(tcb), usTaskStackDepthGet(tcb), (portSTACK_TYPE*)*((portSTACK_TYPE*)tcb));
	
	printf("\r\n");
	vPortDebugPrintBitNames("MSR", context->MSR, pcMSRBitNames);
	vPortDebugPrintBitNames("FSR", context->FSR, pcFSRBitNames);
	printf("Critical Nesting:\t%d\r\n", context->uxCriticalNesting);
	printf("Enable Interrupts:\t%s\r\n", context->MSR & portMSR_MASK_IE ? "Yes" : "No");
	printf("Return Address:\t\t0x%08X\r\n", context->returnAddress);
	
	return xValid;
}

void vPortDebugContextSwitch(void)
{
	extern void vTaskSwitchContext();
	
	static portBASE_TYPE i = 0;
	portBASE_TYPE xValid;
	portSTACK_TYPE *pxBase = (portSTACK_TYPE*)&_stack_end;
	portSTACK_TYPE *pxHigh = (portSTACK_TYPE*)&_stack;
	unsigned portBASE_TYPE ulStackDepth = pxHigh - pxBase;
	
	printf("==================== Context Switch %d =========================\r\n", i++);
	printf("Main Context\r\n");
	xPortDebugPrintStackState(pxBase, ulStackDepth, (portSTACK_TYPE*)mfgpr(r1));
	
	printf("----------------------------------------------------------------\r\n");
	xValid = xPortDebugPrintContext(pxCurrentTCB, "Previous");
	
	if (!xValid) {
		printf("TASK CONTEXT ERROR\r\n");
	}
	
	vTaskSwitchContext();
	
	printf("----------------------------------------------------------------\r\n");
	xValid = xPortDebugPrintContext(pxCurrentTCB, "Current");
	
	if (!xValid) {
		printf("TASK CONTEXT ERROR\r\n");
	}
	
	printf("================================================================\r\n");
}

#endif
