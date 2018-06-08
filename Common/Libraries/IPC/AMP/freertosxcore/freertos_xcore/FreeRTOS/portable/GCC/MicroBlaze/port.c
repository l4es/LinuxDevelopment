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
 * @file port.c
 * @ingroup freertosmbport
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Implementation of the port-specific functions defined in portable.h
 * and portmacro.h.
 */

/* Port/kernel includes. */
#include "FreeRTOS.h"

/* Standard includes. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "task.h"
#include "multicore.h"

typedef void (DeviceHandlerFunc)(void *);

/**
 * @var uxCriticalNesting
 * @ingroup freertosmbport
 * @brief Counts the nesting depth of calls to portENTER_CRITICAL().
 *
 * Each task maintains it's own count, which is saved as part of task's context on the
 * stack.
 */
volatile portSTACK_TYPE uxCriticalNesting = portINITIAL_CRITICAL_NESTING;

/**
 * @var pulMainStack
 * @ingroup freertosmbport
 *
 * To limit the amount of stack required by each task, this port uses a separate stack
 * for interrupts. This stack is the mainline stack allocated at link time. This variable
 * simply stores the top of stack value when the first task is started.
 */
volatile portSTACK_TYPE *pulMainStack[portNUM_PROCESSORS];

#define portINTERRUPT_STACK_SIZE		2048

/**
 * @var ulYieldFlag
 * @ingroup freertosmbport
 * @brief Indicates that a context switch should be performed at the end of an ISR.
 * @see vPortSwitchContext
 *
 * This flag indicates that at the end of the master interrupt handler,
 * a context switch should be performed. This prevents the possibily of switching
 * context multiple times when multiple interrupts are serviced during the same
 * interrupt handler session. As an example, if an ISR calls xSemaphoreGiveFromISR
 * and the higher priority task woken flag was set, then the ISR should call
 * vPortSwitchContext to set this flag. This flag is only declared and used if the
 * kernel is configured for preemption.
 */
#if ( configUSE_PREEMPTION == 1 )
    volatile portBASE_TYPE ulYieldFlag[portNUM_PROCESSORS];
#endif

#if ( portDEBUG_STACK_FRAMES == 1 )
    /**
     * @var pxTaskContext
     * @ingroup freertosmbport
     * @brief Global pointer to context frame of the current task. Set in portasm.S.
     */
	volatile task_context_t *pxTaskContext = NULL;
    /**
     * @var pxCallerFrame
     * @ingroup freertosmbport
     * @brief Global pointer to current caller's stack frame. Set in portasm.S.
     */
	volatile stack_frame_t *pxCallerFrame = NULL;
#endif

/**
 * @var ulTickTimerCounterValue
 * @ingroup freertosmbport
 * @brief Counter value used to generate the periodic tick for the kernel.
 */
static const unsigned portLONG ulTickTimerCounterValue = configCPU_CLOCK_HZ / configTICK_RATE_HZ;

cpuComBlock * cpuComBlockPtr;
void * deviceHandlersPtr;
portBASE_TYPE deviceHandlersSize = sizeof(void *);

/**
 * @ingroup freertosmbport
 *
 * Initialise the stack of a task to look exactly as if the task's context had been saved.
 *
 * @param pxTopOfStack Initial stack pointer for task.
 * @param pxCode Function pointer to the actual code for the task.
 * @param pvParameters Parameters that will be passed to the task when it is started.
 * @return The new stack pointer for the task which has been adjusted to accomodate the
 * initial context for the task.
 */
portSTACK_TYPE *pxPortInitialiseStack(portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters)
{
	int i = 0;
	portSTACK_TYPE *p = NULL;
	task_stack_frame_t *stack = NULL;
	
	pxTopOfStack -= sizeof(task_stack_frame_t) / sizeof(portSTACK_TYPE);
	stack = (task_stack_frame_t*)pxTopOfStack;
	
	/*
	 * the return address to the task is always pulled from the top of the
	 * stack per a normal function epilogue sequence
	 */
	stack->context.returnAddress = pxCode;

	/*
	 * these slots constitute space for local function parameters that may
	 * be passed to functions after the task is yielded or interrupted, in
	 * other words, it allows the yield and interrupt handlers to pass
	 * parameters to sub-routines
	 */
	p = pxTopOfStack;
	p += offsetof(task_stack_frame_t, context) / sizeof(portSTACK_TYPE);
	p += offsetof(task_context_t, calleeParam1) / sizeof(portSTACK_TYPE);
	for (i = 1; i <= 6; i++) {
		*p++ = i;
	}
	
	/* initialize all registers to a zero, some will be overwritten later */
	p = pxTopOfStack;
	p += offsetof(task_stack_frame_t, context) / sizeof(portSTACK_TYPE);
	p += offsetof(task_context_t, MSR) / sizeof(portSTACK_TYPE);
	for (i = 0; i <= 31; i++) {
		*p++ = 0;
	}

	/*
	 * inherit the current MSR settings based on the defined mask and make
	 * sure that interrupts are enabled since tasks are always started within
	 * interrupts enabled
	 */
	stack->context.MSR = (mfmsr() & portMSR_INHERIT_MASK) | portMSR_MASK_IE;
	/*
	 * even if the floating point unit is configured for the MicroBlaze, the
	 * floating point status register should always be initialized to zero
	 */
	stack->context.FSR = 0;
	/*
	 * task parameters are passed in R5 to R10, when the task starts, it will
	 * copy the parameters to the caller's stack frame, which is why we include
	 * the stack frame above the task's context when its initialized
	 */
	stack->context.R5 = (portSTACK_TYPE)pvParameters;
	/*
	 * set the state flag to indicate that this stack frame has been initialized
	 * (vs. interrupted or yielded)
	 */
	stack->context.state = portTASK_CONTEXT_STATE_INITIALIZED;
	/*
	 * tasks are always started with interrupts enabled, which means the critical
	 * nesting value must also be set to zero
	 */
	stack->context.uxCriticalNesting = 0;
	stack->context.framePointer = (void*)(pxTopOfStack + sizeof(task_context_t) / sizeof(portSTACK_TYPE));
	
	/*
	 * this is the return address that would be used if a task were to return
	 * normally, i.e. a task without an infinite loop that doesn't call vTaskDelete,
	 * a possible enhancement to this port would be to set this address to some
	 * function that handles a task that returns--this hypothetical function could
	 * call vTaskDelete so that the application does not crash due to a poorly
	 * written task, note that if this enhancement were used, the initial stack frame
	 * for the task would need to be increased in size to accomodate the entire
	 * stack frame of the function that the task "returns" too
	 */
	stack->callerFrame.returnAddress = NULL;
	/*
	 * these slots represent the hypothetical stack frame of the function that
	 * "calls" the task, i.e. the function parameters to the task, when the
	 * task starts, the normal function prologue code will copy the function
	 * parameters for the task onto this stack frame (i.e. the caller's stack
	 * frame)
	 */
	p = pxTopOfStack;
	p += offsetof(task_stack_frame_t, callerFrame);
	p += offsetof(stack_frame_t, param1);
	for (i = 1; i <= 6; i++) {
		*p++ = i;
	}
	
	/*
	 * return a pointer to the top of the stack we have generated so this can be
	 * stored in the task control block for the task
	 */
	return pxTopOfStack;
}

/**
 * @ingroup freertosmbport
 *
 * Periodic tick interrupt handler for the kernel. Calls vTaskIncrementTick, which
 * updates the current tick count and potentially awakens tasks that have been
 * blocked or delayed. Then it calls vPortSwitchContext which determines if the
 * the master interrupt handler should perform a context switch before returning
 * to mainline task code. Connected to the interrupt controller by xPortSetupTickTimer.
 *
 * @param CallBackRef
 * @param nTmrCtrNumber
 */
void vTickISR(void *CallBackRef, unsigned portCHAR nTmrCtrNumber)
{
	unsigned portBASE_TYPE processor = xPortGetCurrentCPU();
	if (processor == 0) {

		/* Increment the RTOS tick - this might cause a task to unblock. */
		vTaskIncrementTick();

		portBASE_TYPE i;
		for (i=0;i<portNUM_PROCESSORS;i++) {
			/* Notify the interrupt handler that a context switch should be
			 * performed. Note that if the kernel is not configured for
			 * preemption, then this is an empty definition. See portmacro.h. */
			vPortSwitchContext(i);
		}

	}

}

/**
 * Executes the correct device interrupt handler for the current processor core.
 *
 * Called by the master interrupt handler.
 */
void xPortDeviceHandler(void * DeviceId) {

	portBASE_TYPE processor = xPortGetCurrentCPU();

	((DeviceHandlerFunc*)(cpuComBlockPtr->deviceHandlers[processor]))(DeviceId);

}

/**
 * @ingroup freertosmbport
 *
 * Starts the scheduler by first initializing the periodic tick timer and then
 * starting the first task by calling vPortStartFirstTask.
 *
 * @return pdFAIL because this function is not supposed to return to the caller.
 */
portBASE_TYPE xPortStartScheduler(void)
{

	unsigned portBASE_TYPE i = 0;

	/* Initialise the yield flags for all processors and allocate space on the mainline
	 * stack for use by the interrupt handler.
	 *
	 * Note that slave cores will be spinning until this function assigns a value to
	 * the initMagic member of the ignition communication block.
	 *
	 */
	for (i=0;i<portNUM_PROCESSORS;i++) {

		ulYieldFlag[i] = 0;
		pulMainStack[i] = ( portSTACK_TYPE * ) pvPortMallocAligned( ( ( ( size_t )portINTERRUPT_STACK_SIZE ) * sizeof( portSTACK_TYPE ) ), NULL );

	}

	// The ignition communication block is at the bottom of shared memory
	cpuComBlockPtr = (cpuComBlock *)(XPAR_DDR2_SDRAM_MPMC_BASEADDR);

	/* Tell the slave cores where to find the master interrupt handler and
	 * "start first task" code.
	 */
	cpuComBlockPtr->deviceHandlers[0] = &XIntc_DeviceInterruptHandler;
	cpuComBlockPtr->interruptHandlerPtr = &_interrupt_handler;
	cpuComBlockPtr->startFirstTaskPtr = &vPortStartFirstTask;
	cpuComBlockPtr->initMagic = MASTER_CPU_SYNC_MAGIC_CODE;

	deviceHandlersPtr = &cpuComBlockPtr->deviceHandlers[0];

#if (configSTDOUT_STARTUP_MSG == 1)
	print("FreeRTOS started OK.\n----------\n");
#endif

	extern void vStartFirstTask(void);

	if (!xPortSetupTickTimer(vTickISR, ulTickTimerCounterValue)) {
		return pdFAIL;
	}

    vPortStartFirstTask();

	return pdFAIL;
}

/**
 * @ingroup freertosmbport
 *
 * Stops the scheduler.
 *
 * @attention Not implemented because this function has no practical use
 * with this port as there is nothing to return to.
 */
void vPortEndScheduler(void)
{
}

void vPortYield(void)
{
	extern void vPortYieldASM(void);
	portDISABLE_INTERRUPTS();
	vPortYieldASM();
	portENABLE_INTERRUPTS();
}

portBASE_TYPE xPortAreInterruptsEnabled() {

	portBASE_TYPE msrMem, xReturn;

	msrMem = mfmsr() & 0x02;

	if (msrMem == 1) xReturn = pdTRUE;
	else xReturn = pdFALSE;

	return xReturn;

}

void vPortMemoryBarrier( void ) {

	// Issue the mbar instruction for data accesses
	asm __volatile__ ("nop\nmbar 2\nnop");

}

#if ( portUSE_CRITICAL_SECTION_MACROS != 1 )

void vPortEnterCritical(void)
{
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;
}

void vPortExitCritical(void)
{
	if (uxCriticalNesting > 0) {
		if (--uxCriticalNesting == 0 ) {
			portENABLE_INTERRUPTS();
		}
	}
}

#endif

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

unsigned portSHORT usMainCheckFreeStackSpace(void)
{
	extern void *_stack_end;
	portSTACK_TYPE *pulStack = (portSTACK_TYPE*)&_stack_end;

	vPortEnterCritical();
	while (*pulStack++ == portSTACK_FILL_VALUE);
	vPortExitCritical();
	
	return (unsigned portSHORT)(pulStack - (portSTACK_TYPE*)&_stack_end + 1);
}

#endif
