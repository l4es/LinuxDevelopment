#include "xparameters.h"
#include <stdio.h>
#include "xintc.h"
#include "xtmrctr.h"
#include "mb_interface.h"
#include "xuartlite_l.h"
#include "pvr.h"
#include "multicore.h"

void * masterInterruptHandler;
void _interrupt_handler(void);

static XIntc xIntc;
static XTmrCtr xTickTimer;

void safePrint(char * text);

#define TCSR0 (*((volatile unsigned long *)(XPAR_XPS_TIMER_1_BASEADDR + 0x00)))

void (* taskLauncher) (void);
cpuComBlock * cpuComBlockPtr = (cpuComBlock *)(XPAR_DDR2_SDRAM_MPMC_BASEADDR);

int setupInterruptController() {

	XStatus status;
	int xReturn = 0;

	/* initialize the interrupt controller */
	status = XIntc_Initialize(&xIntc, XPAR_XPS_INTC_2_DEVICE_ID);

	if (status == XST_SUCCESS) {
	    /* configure the ISR to service all interrupts when a single interrupt occurs */
	    XIntc_SetIntrSvcOption(xIntc.BaseAddress, XIN_SVC_ALL_ISRS_OPTION);
	    /* start the interrupt controller, note that this doesn't actually enable interrupts */
	    status = XIntc_Start(&xIntc, XIN_REAL_MODE);
	    if (status == XST_SUCCESS) {
			xReturn = 1;
	    }
	}

	return xReturn;

}

void enableTimerInterrupt() {

	XIntc_Enable(&xIntc, XPAR_XPS_INTC_2_XPS_TIMER_1_INTERRUPT_INTR);
	microblaze_enable_interrupts();

}

int connectInterruptHandler() {

	XStatus status;
	int xReturn = 0;

	/* pass the handler information to the interrupt controller */
	status = XIntc_Connect(&xIntc, XPAR_XPS_INTC_2_XPS_TIMER_1_INTERRUPT_INTR, XTmrCtr_InterruptHandler, &xTickTimer);

	if (status == XST_SUCCESS) {
		/* enable the connected interrupt within the interrupt controller */
		enableTimerInterrupt();
		xReturn = 1;
	}

	return xReturn;

}

void _TimerHandler(void *CallBackRef, u8 TmrCtrNumber) {

	// Acknowledge the interrupt
	TCSR0 = TCSR0;

}

int setupTickTimer() {

	XStatus status;
	int xReturn = 0;
	/* initialize the tick timer to periodically generate an interrupt */
	status = XTmrCtr_Initialize(&xTickTimer, XPAR_XPS_TIMER_1_DEVICE_ID);
	if (status == XST_SUCCESS) {
		XTmrCtr_SetOptions(&xTickTimer, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);
		XTmrCtr_SetResetValue(&xTickTimer, 0, ( ( unsigned long ) 125000000 ) / ( ( unsigned long ) 100));

		/* register and enable the timer/counter handler with the interrupt controller,
		 * then register our tick handler with the specific timer/counter */
		XTmrCtr_SetHandler(&xTickTimer, &_TimerHandler, &xTickTimer);
		//xReturn = connectInterruptHandler(XPAR_XPS_INTC_2_XPS_TIMER_1_INTERRUPT_INTR, XTmrCtr_InterruptHandler, &xTickTimer);
		xReturn = connectInterruptHandler();
		if (xReturn == 1) {
			/* start the timer counting */
			XTmrCtr_Start(&xTickTimer, 0);
			xReturn = 1;
		}
	}

	return xReturn;

}

unsigned long GetCurrentCPU() {

	/* Query the USER1 bits in the PVR register to determine the current core's
	 * ID.
	 * */
	pvr_t pvr_data;
	microblaze_get_pvr(&pvr_data);
	unsigned long xReturn = (unsigned long)((unsigned char)((pvr_data.pvr[0] >> 0) & 0xFF));

	return xReturn;

}

int main() {

	unsigned long processor = GetCurrentCPU();

	cpuComBlockPtr->initMagic = SLAVE_CPU_SYNC_MAGIC_CODE;
	cpuComBlockPtr->deviceHandlers[processor] = &XIntc_DeviceInterruptHandler;

	// Spin until the master CPU has finished writing the CPU COM block
	while (cpuComBlockPtr->initMagic != MASTER_CPU_SYNC_MAGIC_CODE) { }

	masterInterruptHandler = (void *)cpuComBlockPtr->interruptHandlerPtr;

	taskLauncher = cpuComBlockPtr->startFirstTaskPtr;

	setupInterruptController();
	setupTickTimer();
	taskLauncher(); /* Never returns */

	return 0;

}
