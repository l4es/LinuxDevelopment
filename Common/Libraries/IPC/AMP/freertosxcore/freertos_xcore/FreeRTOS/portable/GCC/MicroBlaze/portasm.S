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
 * @file portasm.S
 * @ingroup freertosmbport
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Assembly language functions.
 * @attention The header files included herein can only contain pre-processor
 * definitions, i.e. #defines.
 */

#include <xparameters.h>
#include "FreeRTOSConfig.h"
#include "portdefs.h"

/* External function declarations. */
.extern XIntc_DeviceInterruptHandler
.extern vTaskSwitchContext
.extern vPortDebugContextSwitch
.extern xPortDebugVerifyCurrentContext
.extern xPortDeviceHandler
/* External variable declarations. */
.extern pxCurrentTCBs
.extern pulMainStack
.extern uxCriticalNesting
.extern _stack_end
.extern _stack
.extern _STACK_SIZE
#if ( configUSE_PREEMPTION == 1 )
.extern ulYieldFlag
#endif
#if ( portDEBUG_STACK_FRAMES == 1 )
.extern pxTaskContext
.extern pxCallerFrame
#endif

/**
 * @def prologue
 * @ingroup freertosmbport
 *
 * Creates a local stack frame of the given \size. Pushes
 * the return address saved in register \ret onto the
 * newly created stack. Uses R19 as the frame pointer.
 */
.macro prologue ret, size
#if ( portDEBUG_STACK_FRAMES == 1 )
	swi		r1,		r0,		pxCallerFrame
#endif
	addik	r1,		r1,		-\size
#if ( portDEBUG_STACK_FRAMES == 1 )
	swi		r1,		r0,		pxTaskContext
#endif
	swi		\ret,	r1, 	0
	swi		r19,	r1,		\size - 4
	addk	r19,	r1,		r0
.endm

/**
 * @def pushargs
 * @ingroup freertosmbport
 *
 * Pushes all local function parameters from the caller onto
 * the caller's stack frame using the local frame pointer in
 * R19. Its up to the caller to ensure that there is enough
 * room on the stack for these parameters.
 */
.macro pushargs
	swi		r5,		r19,	12
	swi		r6,		r19,	16
	swi		r7,		r19,	20
	swi		r8,		r19,	24
	swi		r9,		r19,	28
	swi		r10,	r19,	32
.endm

/**
 * @def epilogue
 * @ingroup freertosmbport
 *
 * Destroys a local stack frame of the given \size. Pops the
 * return address from the stack into register \ret. Uses R19
 * as the frame pointer.
 */
.macro epilogue ret, size
	lwi		\ret,	r1,		0
	addk	r1,		r19,	r0
	lwi		r19,	r1,		\size - 4
	addik	r1,		r1,		\size
#if ( portDEBUG_STACK_FRAMES == 1 )
	swi		r1,		r0,		pxCallerFrame
#endif
.endm

/**
 * @def memalign
 * @ingroup freertosmbport
 *
 * Aligns the value in the given register to the lower word boundary
 * so that if the register is used to access a memory location,
 * the access will be word-aligned.
 */
.macro memalign reg
	andni	\reg	\reg	3
.endm

.global vPortRestoreContext
.section .text
.align 4
.ent vPortRestoreContext
.type vPortRestoreContext, @function
vPortRestoreContext:

	/* recreate the local stack frame that was created when the task was
	 * either interrupted or yielded */
	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the absolute address of the current CPU's TCB */

	lwi			r3,		r18,	pxCurrentTCBs	/* load pointer to current TCB into R3 */
	lwi			r1,		r3,		0				/* load saved stack pointer in TCB into R1 */
#if ( portDEBUG_STACK_FRAMES == 1 )
	swi			r1,		r0,		pxTaskContext	/* set global context pointer */
#endif
	addk		r19,	r1,		r0				/* set frame pointer (R19) to local stack pointer (R1) */

	/* restore the entire context for the task */
												/* pop critical nesting into R3 from R19 slot */
	lwi			r3,		r1,		portTASK_CONTEXT_OFFSET(19)
												/* restore critical nesting from R3 */
	swi			r3,		r0,		uxCriticalNesting
#if ( XPAR_MICROBLAZE_USE_FPU == 1 )
												/* pop FSR into R3 from R1 slot */
	lwi			r3,		r1,		portTASK_CONTEXT_OFFSET(1)
	mts			rfsr,	r3						/* restore FSR from R3 */
#else
	mts			rfsr,	r0						/* clear FSR if there is no FPU */
#endif
												/* pop MSR into R3 from R0 slot */
	lwi			r3,		r1,		portTASK_CONTEXT_OFFSET(0)
												/* mask only IE in MSR into R4 */
	andi		r4,		r3,		portMSR_MASK_IE
												/* mask all but IE in MSR into R3 */
	andi		r3,		r3,		~portMSR_MASK_IE
	mts			rmsr,	r3						/* restore MSR from R3 */

												/* pop all necessary registers */
	lwi			r31,	r1,		portTASK_CONTEXT_OFFSET(31)
	lwi			r30,	r1,		portTASK_CONTEXT_OFFSET(30)
	lwi			r29,	r1,		portTASK_CONTEXT_OFFSET(29)
	lwi			r28,	r1,		portTASK_CONTEXT_OFFSET(28)
	lwi			r27,	r1,		portTASK_CONTEXT_OFFSET(27)
	lwi			r26,	r1,		portTASK_CONTEXT_OFFSET(26)
	lwi			r25,	r1,		portTASK_CONTEXT_OFFSET(25)
	lwi			r24,	r1,		portTASK_CONTEXT_OFFSET(24)
	lwi			r23,	r1,		portTASK_CONTEXT_OFFSET(23)
	lwi			r22,	r1,		portTASK_CONTEXT_OFFSET(22)
	lwi			r21,	r1,		portTASK_CONTEXT_OFFSET(21)
	lwi			r20,	r1,		portTASK_CONTEXT_OFFSET(20)
	lwi			r18,	r1,		portTASK_CONTEXT_OFFSET(18)
	lwi			r17,	r1,		portTASK_CONTEXT_OFFSET(17)
	lwi			r16,	r1,		portTASK_CONTEXT_OFFSET(16)
	lwi			r15,	r1,		portTASK_CONTEXT_OFFSET(15)
	lwi			r12,	r1,		portTASK_CONTEXT_OFFSET(12)
	lwi			r11,	r1,		portTASK_CONTEXT_OFFSET(11)
	lwi			r10,	r1,		portTASK_CONTEXT_OFFSET(10)
	lwi			r9,		r1,		portTASK_CONTEXT_OFFSET(9)
	lwi			r8,		r1,		portTASK_CONTEXT_OFFSET(8)
	lwi			r7,		r1,		portTASK_CONTEXT_OFFSET(7)
	lwi			r6,		r1,		portTASK_CONTEXT_OFFSET(6)
	lwi			r5,		r1,		portTASK_CONTEXT_OFFSET(5)
	lwi			r3,		r1,		portTASK_CONTEXT_OFFSET(3)

	bneid		r4,		__return_with_rtid		/* was the MSR IE bit in R4 clear? */
												/* pop R4 in the delay slot (always executed) */
	lwi			r4,		r1,		portTASK_CONTEXT_OFFSET(4)

	/* destroy the local stack frame and return to the task with either
	 * a normal return from sub-routine instruction or a special return
	 * from interrupt instruction */
	epilogue	r14,	portTASK_CONTEXT_SIZE
	rtsd		r14		8
	nop

__return_with_rtid:
	epilogue	r14,	portTASK_CONTEXT_SIZE
	rtid		r14,	0
	nop
.end vPortRestoreContext

.global vPortSaveContext
.section .text
.align 4
.ent vPortSaveContext
.type vPortSaveContext, @function
vPortSaveContext:
	prologue	r15,	8
	pushargs

#if ( portDEBUG_STACK_FRAMES == 1 )
	swi			r5,		r0,		pxTaskContext
#endif

	swi			r6,		r5,		0				/* save return address at bottom of context frame */
												/* save all non-volatile registers, R18 and R20 to R31 */
	swi			r11,	r5,		portTASK_CONTEXT_OFFSET(11)
	swi			r12,	r5,		portTASK_CONTEXT_OFFSET(12)
	swi			r18,	r5,		portTASK_CONTEXT_OFFSET(18)
	swi			r20,	r5,		portTASK_CONTEXT_OFFSET(20)
	swi			r21,	r5,		portTASK_CONTEXT_OFFSET(21)
	swi			r22,	r5,		portTASK_CONTEXT_OFFSET(22)
	swi			r23,	r5,		portTASK_CONTEXT_OFFSET(23)
	swi			r24,	r5,		portTASK_CONTEXT_OFFSET(24)
	swi			r25,	r5,		portTASK_CONTEXT_OFFSET(25)
	swi			r26,	r5,		portTASK_CONTEXT_OFFSET(26)
	swi			r27,	r5,		portTASK_CONTEXT_OFFSET(27)
	swi			r28,	r5,		portTASK_CONTEXT_OFFSET(28)
	swi			r29,	r5,		portTASK_CONTEXT_OFFSET(29)
	swi			r30,	r5,		portTASK_CONTEXT_OFFSET(30)
	swi			r31,	r5,		portTASK_CONTEXT_OFFSET(31)
	
	mfs			r3,		rmsr					/* load MSR into R3 */
	beqid		r7,		12						/* should interrupts be enabled in the MSR? */
	nop
												/* set the interrupt enable bit in the MSR */
	ori			r3,		r3,		portMSR_MASK_IE
												/* push MSR into slot R0 from R3 */
	swi			r3,		r5,		portTASK_CONTEXT_OFFSET(0)
#if ( XPAR_MICROBLAZE_USE_FPU == 1 )
	mfs			r3,		rfsr					/* load FSR into R3 */
												/* push FSR into slot R1 from R3 */
	swi			r3,		r5,		portTASK_CONTEXT_OFFSET(1)
#else											/* push blank FSR into slot R1 */
	swi			r0,		r5,		portTASK_CONTEXT_OFFSET(1)
#endif
	
												/* store context state (R8) in R14 slot */
	swi			r8,		r5,		portTASK_CONTEXT_OFFSET(14)
												/* load critical nesting into R3 */
	lwi			r3,		r0,		uxCriticalNesting
												/* push critical nesting into slot R19 from R3 */
	swi			r3,		r5,		portTASK_CONTEXT_OFFSET(19)
												/* load pointer to current TCB into R3 */
	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the absolute address of the current CPU's TCB */

	lwi			r3,		r18,	pxCurrentTCBs
	swi			r5,		r3,		0				/* save pointer to context frame into TCB */
	
	epilogue	r15,	8
	rtsd		r15,	8
	nop
.end vPortSaveContext

.global _interrupt_handler
.section .text
.align 4
.ent _interrupt_handler
.type _interrupt_handler, @function
_interrupt_handler:

	prologue	r14,	portTASK_CONTEXT_SIZE
	
												/* save volatile registers */
												/* save r3 before restoring the bridge
												 * swap value as we need a spare register */
	swi			r3,		r1,		portTASK_CONTEXT_OFFSET(3)

												/* Now that there's a register available, get the bridge value back */
	mfs			r3,		rpvr0					/* Get the contents of the PVR register */
	andi		r3,		r3,		0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r3 */

	beqi		r3,		_no_bridge_swap			/* Ensure CPU 0 (master CPU) never uses the bridge swap */
	muli		r3,		r3,		4
	lwi			r18,	r3,		BRIDGE_SWAP_ADDR

_no_bridge_swap:

	swi			r4,		r1,		portTASK_CONTEXT_OFFSET(4)
	swi			r5,		r1,		portTASK_CONTEXT_OFFSET(5)
	swi			r6,		r1,		portTASK_CONTEXT_OFFSET(6)
	swi			r7,		r1,		portTASK_CONTEXT_OFFSET(7)
	swi			r8,		r1,		portTASK_CONTEXT_OFFSET(8)
	swi			r9,		r1,		portTASK_CONTEXT_OFFSET(9)
	swi			r10,	r1,		portTASK_CONTEXT_OFFSET(10)
												/* save sub-routine return address */
	swi			r15,	r1,		portTASK_CONTEXT_OFFSET(15)
	
	addk		r5,		r1,		r0				/* R5 = context pointer */
	addk		r6,		r14,	r0				/* R6 = return address (R14) */
	addik		r7,		r0,		1				/* R7 = TRUE */
												/* R8 (state) = interrupted */
	addik		r8,		r0,		portTASK_CONTEXT_STATE_INTERRUPTED
	brlid		r15,	vPortSaveContext		/* call context saver */
	nop

#if ( portGENERATE_ISR_SIGNAL == 1 )
												/* MEM[address] |= mask */
	lwi			r3,		r0,		(portISR_SIGNAL_ADDRESS)
	ori			r3,		r3,		(portISR_SIGNAL_MASK)
	swi			r3,		r0,		(portISR_SIGNAL_ADDRESS)
#endif

	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the relative address of a word for the current CPU */
	lwi			r1,		r18,	pulMainStack	/* Load the current CPU's interrupt stack pointer */

	addk		r15,	r0,		r0				/* clear return address */
	prologue	r15,	36						/* create local stack frame */
												/* load the intial critical nesting value
												 * into R3 and store it in the global
												 * critical nesting variable */
	addik		r3,		r0,		portINITIAL_CRITICAL_NESTING
	swi			r3,		r0,		uxCriticalNesting

	addik		r5,		r0,		portXPS_INTC_DEVICE_ID
	/*
	 * The low-level device interrupt handler (XIntc_DeviceInterruptHandler) called must
	 * be the one specific to the current core. Slave cores will have their low-level
	 * handlers compiled to a location in their private BRAM, while the master core will
	 * have its handler in shared memory. The addresses of the handlers are
	 * placed into the ignition communication block on startup. Here we delegate to
	 * xPortDeviceHandler to execute the appropriate device interrupt handler.
	 */
    bralid 		r15,	xPortDeviceHandler
    nop

_no_device_handler:

#if ( configUSE_PREEMPTION == 1 )

	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the relative address of a word for the current CPU */
	lwi			r3,		r18,	ulYieldFlag		/* load the yield flag into R3 */
												/* is the yield flag zero? if so,
												 * branch to the restore context */

	beqi		r3,		__interrupt_handler_restore_context
	nop

#if ( portDEBUG_CONTEXT_SWITCHES == 1 )
	bralid		r15,	vPortDebugContextSwitch	/* handle debug context switch */
	nop
#else
	bralid		r15,	vTaskSwitchContext		/* handle context switch */
	nop
#endif
#endif

__interrupt_handler_restore_context:

#if ( configUSE_PREEMPTION == 1 )
	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the relative address of a word for the current CPU */
	swi			r0,		r18,	ulYieldFlag		/* clear the yield flag */
#endif

#if ( portGENERATE_ISR_SIGNAL == 1 )
												/* MEM[address] &= ~mask */
	lwi			r3,		r0,		(portISR_SIGNAL_ADDRESS)
	andi		r3,		r3,		~(portISR_SIGNAL_MASK)
	swi			r3,		r0,		(portISR_SIGNAL_ADDRESS)

#endif

    brlid		r15,	vPortRestoreContext		/* function never returns */
    nop
.end _interrupt_handler

.global vPortYieldASM
.section .text
.align 4
.ent vPortYieldASM
.type vPortYieldASM, @function
vPortYieldASM:
	prologue	r15,	portTASK_CONTEXT_SIZE	/* create stack frame for task context */
	
	addk		r5,		r1,		r0				/* R5 = context pointer */
	addk		r6,		r15,	r0				/* R6 = return address */
	addk		r7,		r0,		r0				/* R7 = FALSE */
												/* R8 (state) = yielded */
	addik		r8,		r0,		portTASK_CONTEXT_STATE_YIELDED
	
	brlid		r15,	vPortSaveContext		/* call context save function */
	nop
	
	mfs			r18,	rpvr0					/* Get the contents of the PVR register */
	andi		r18,	r18,	0xFF			/* Put the value of the high byte (the USER1/CPU ID information) in r18 */
	muli		r18,	r18,	4				/* Find the relative address of a word for the current CPU */
	lwi			r1,		r18,	pulMainStack	/* load the current CPU's interrupt stack pointer */

	addk		r15,	r0,		r0				/* clear return address */
	prologue	r15,	36						/* create local stack frame */
												/* load the intial critical nesting value
												 * into R3 and store it in the global
												 * critical nesting variable */
	addik		r3,		r0,		portINITIAL_CRITICAL_NESTING
	swi			r3,		r0,		uxCriticalNesting

#if ( portDEBUG_CONTEXT_SWITCHES == 1 )
	bralid		r15,	vPortDebugContextSwitch	/* handle debug context switch */
	nop
#else
	bralid		r15,	vTaskSwitchContext		/* handle context switch */
	nop
#endif

    brlid		r15,	vPortRestoreContext		/* function never returns */
    nop
.end vPortYieldASM

.global vPortStartFirstTask
.section .text
.align 4
.ent vPortStartFirstTask
.type vPortStartFirstTask, @function
vPortStartFirstTask:
	prologue	r15,	8						/* local stack frame */

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
												/* R18 = stack fill value */
	addik		r18,	r0,		portSTACK_FILL_VALUE
	addik		r3,		r0,		_stack_end		/* R3 = bottom of stack */
	rsub		r4,		r3,		r1				/* R4 = R1 - R3 (R4 = offset from stack pointer) */
	memalign	r4								/* MEMALIGN(R4) */

_again:
	addik		r4,		r4,		-4				/* R4 -= 4 */
	bltid		r4,		_done					/* done if R4 < 0 */
	nop
	sw			r18,	r3,		r4				/* MEM(R3 + R4) = R18 */
	brid		_again
	nop
_done:
	nop
	nop
#endif

    brlid		r15,	vPortRestoreContext		/* function never returns */
    nop
	lwi			r1,		r0,		pulMainStack	/* load the main stack pointer */
												/* load the intial critical nesting value
												 * into R3 and store it in the global
												 * critical nesting variable */
	addik		r3,		r0,		portINITIAL_CRITICAL_NESTING
	swi			r3,		r0,		uxCriticalNesting
    epilogue	r15,	8
    rtsd		r15,	8
    nop
.end vPortStartFirstTask
