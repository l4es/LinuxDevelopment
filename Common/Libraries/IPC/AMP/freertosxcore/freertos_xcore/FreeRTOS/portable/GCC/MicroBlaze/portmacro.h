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
 * @defgroup freertosmbport FreeRTOS GCC/MicroBlaze Port
 *
 * @note This port has been tested extensively using version 11.4 of the Xilinx
 * SDK in conjunction with FreeRTOS V6.0.2. The intended use for this port is
 * is a FreeRTOS-based system that uses preemption.
 *
 * @note Documentation for this port was generated for use with Doxygen 1.6.1,
 * though it should be compatible with most versions of Doxygen.
 *
 * @attention This port has been tested extensively and optimized for use with
 * the preemption option in FreeRTOSConfig.h. While this port has been tested
 * with the preemption option disabled, it is definitely not optimized for this
 * mode of operation. For example, context saves and restores in the interrupt
 * handler could be removed if the kernel is not using preemption. This would
 * greatly decrease the overhead and latency of servicing interrupts, at the
 * expense of increased stack usage of course.
 *
 * @par Stack Frame Layout:
 * Derived from the Application Binary Interface (ABI) defined in the
 * MicroBlaze reference guide (mb_ref_guide.pdf) and captured in the
 * structure task_context_t.
 * 
 * @par
 * The stack frame will look as if the task called a non-leaf function. That is, a
 * function that calls other functions with parameters, which means it needs room
 * on its stack frame for local function parameters as well as all of the registers.
 * The stack pointer grows downward in memory and always points to the last item
 * pushed onto the stack.
 * 
 * @par
 * We capture the frame layout in the structure task_context_t so that a pointer to
 * this structure can be watched in the debugger. This significantly eases debugging
 * assembly language code. The order of the elements in the stack frame structure
 * are such that to save the context of the task, we can use the standard function
 * assembly language prologue code used by GCC for the MicroBlaze. This also makes
 * it easier for the Eclipse debugging environment to follow the assembly language
 * code when single-stepping.
 * 
 * @par Stack Size Considerations:
 * The minimum stack size for this port is somewhere between 512
 * and 1024 words, that is between 2kB and 4kB. Because of the 32
 * 32-bit registers, the stack frame where a task's context is saved
 * is greater than 32 words (128 bytes) by itself. Additional room is
 * needed within this context frame to keep it in sync with the stack
 * frames created and destroyed by GCC. This brings the total size of
 * a saved task's context to 160 bytes (40 words). Additional room is
 * needed below this context to create a minimal "caller's frame" that
 * the task will push its function parameters onto. When the stack
 * frame for a task is initialized, it already consumes 188 bytes
 * (47 words). 160 bytes are immediately removed when the task is
 * started.
 * 
 * @par
 * Generally speaking, the GCC port for the MicroBlaze is very
 * liberal with its stack usage. Non-leaf functions that don't use
 * local variables but call other functions that take parameters use
 * at a minimum 36 bytes (7 words) of additional stack space. Local
 * variables will typically consume at least an additional 4 bytes
 * (1 word) of stack space.
 * 
 * @par
 * To minimize the stack size requirements for tasks, the mainline
 * stack is used for ISRs and context switches following a yield.
 * The minimum size for this stack is similar to the minimum size
 * for a task's stack, that is 1024 words.
 *
 * @attention The inclusion chain for the files in this port should be:
 * @code
 *      #include "FreeRTOS.h"
 *          #include "FreeRTOSConfig.h"
 *              #include <xparameters.h>
 *          #include "portable.h"
 *              #include "portmacro.h"
 *                  #include <mb_interface.h>
 *                  #include "portdefs.h"
 *                  #include "porthw.h"
 * @endcode
 * This means that application code and port-specific code only needs
 * to include the main FreeRTOS.h header.
 *
 * @par Example main.c Initialization:
 * @code
        int main(void)
        {
            xPortSetupHardware();
            // perform remaining initialization in a dedicated thread
            xTaskCreate(vMainThread, "MAIN", configMINIMAL_STACK_SIZE, NULL, 0, NULL);
            vTaskStartScheduler();
            return 0;
        }
 * @endcode
 *
 * @{
 */

/**
 * @file portmacro.h
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Primary file included by the FreeRTOS portable layer for the
 * GCC/MicroBlaze environment.
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <mb_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in FreeRTOSConfig.h */
#if ( configMINIMAL_STACK_SIZE < 1024 )
	#error Invalid definition: configMINIMAL_STACK_SIZE should be >= 1024 for the GCC MicroBlaze port. See portmacro.h for help.
#endif

/*
 * Include port-specific definitions. These are kept in a separate file
 * so they can be included from assembly language files. This is kept
 * inside the extern "C" bracket because portdefs.h does not include
 * C++ protection.
 */
#include "portdefs.h"

/**
 * @defgroup ktypes Kernel Types
 * @brief CPU/compiler-specific type definitions exported to FreeRTOS.
 * @note These cannot be typedefs. Otherwise, the FreeRTOS source code will not compile.
 * @{
 */
#define portCHAR								char
#define portSHORT								short int
#define portLONG								long int
#define portFLOAT								float
#define portDOUBLE								double
#define portBASE_TYPE							long int
#define portSTACK_TYPE							unsigned long int
#define portBOOL								portBASE_TYPE
typedef long int								bool;
/**
 * @def portTickType
 * @brief Kernel tick type.
 */
/**
 * @def portMAX_DELAY
 * @brief Maximum kernel delay in units of ticks.
 */
#if ( configUSE_16_BIT_TICKS == 1 )
	#define portTickType						unsigned short int
	#define portMAX_DELAY						(portTickType)0xFFFF
#else
	#define portTickType						unsigned long int
	#define portMAX_DELAY						(portTickType)0xFFFFFFFF
#endif
/** @} */

/**
 * @defgroup kdefs Kernel Definitions
 * @brief CPU/compiler-specific configuration options exported to FreeRTOS.
 * @{
 */
/**
 * @def TRUE
 * @brief Positive assertion. Only defined if its not already defined.
 */
#ifndef TRUE
#define TRUE									1
#endif
/**
 * @def FALSE
 * @brief Negative assertion. Only defined if its not already defined.
 */
#ifndef FALSE
#define FALSE									0
#endif
/**
 * @def portCRITICAL_NESTING_IN_TCB
 * @brief Controls storage of the critical nesting value in the TCB.
 *
 * Disabled for this port because we also use a critical nesting value for interrupt context.
 * Since interrupt context does not have a corresponding TCB, there is no place to store the
 * the critical nesting value. Hence, it must be global for all tasks AND ISRs.
 */
#define portCRITICAL_NESTING_IN_TCB				0
/**
 * @def portBYTE_ALIGNMENT
 * @brief CPU byte alignment.
 */
#define portBYTE_ALIGNMENT						4
/**
 * @def portSTACK_GROWTH
 * @brief Controls whether the stack grows up or down in memory.
 */
#define portSTACK_GROWTH						-1
/**
 * @def portTICK_RATE_MS
 * @brief The period of the kernel's periodic tick, in milliseconds.
 */
#define portTICK_RATE_MS						( ( portTickType ) 1000 / configTICK_RATE_HZ )
/**
 * @def portNOP()
 * @brief Executes a single NOP instruction.
 */
#define portNOP()								asm volatile ( "nop" )
/**
 * @def portTASK_FUNCTION_PROTO
 * @brief See the description on the FreeRTOS.org website.
 */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )		void vFunction( void *pvParameters )
/**
 * @def portTASK_FUNCTION
 * @brief See the description on the FreeRTOS.org website.
 */
#define portTASK_FUNCTION( vFunction, pvParameters )			void vFunction( void *pvParameters )
/** @} */

/**
 * @def portMSR_INHERIT_MASK
 * @brief Determines which MSR bits are inherited when a task is created.
 *
 * We only want to inherit certain bits in the MSR when a task's context is
 * first initialized. Note that this isn't really necessary because if the
 * MicroBlaze is not configured to handle execeptions (EE) or use instruction
 * and data caches (ICE and DCE), these bits should be read-only zero in the
 * MSR. The biggest problem with the original MicroBlaze port for FreeRTOS is
 * that the current state of the MSR was not inherited when a task's context was
 * initialized. For a MicroBlaze that uses instruction and data caches, this
 * caused the caches to be disabled when the task started because the task
 * did not inherit the '1'-state of the ICE and DCE bits in the MSR when it
 * was initialized.
 */
#if ( XPAR_MICROBLAZE_D_OPB == 1 || XPAR_MICROBLAZE_I_OPB == 1 )
	#define portMSR_INHERIT_MASK				( portMSR_MASK_PVR | portMSR_MASK_EE | portMSR_MASK_DCE | portMSR_MASK_ICE | portMSR_MASK_BE )
#else
	#define portMSR_INHERIT_MASK				( portMSR_MASK_PVR | portMSR_MASK_EE | portMSR_MASK_DCE | portMSR_MASK_ICE )
#endif

/**
 * @enum task_context_state_t
 * @brief Represents the current state of a task.
 * @see portdefs.h
 */
typedef enum task_context_state {
	yielded = portTASK_CONTEXT_STATE_YIELDED,
	interrupted = portTASK_CONTEXT_STATE_INTERRUPTED,
	initialized = portTASK_CONTEXT_STATE_INITIALIZED
} task_context_state_t;

/**
 * @struct stack_frame_t
 * @brief Generic stack frame of a non-leaf function, i.e. a function that passes parameters to other
 * functions.
 *
 * Only represents the stack frame up to any local parameters declared by the function. The generic
 * layout of a complete stack frame is defined in the Application Binary Interface (ABI) of the
 * MicroBlaze Reference Guide (mb_ref_guide.pdf).
 */
typedef struct stack_frame {
/* LOW MEMORY */
/*   0 */	void					*returnAddress;
/*   4 */	portSTACK_TYPE			param1;             /**< 1st function parameter, passed in R5. */
/*   8 */	portSTACK_TYPE			param2;             /**< 2nd function parameter, passed in R6. */
/*  12 */	portSTACK_TYPE			param3;             /**< 3rd function parameter, passed in R7. */
/*  16 */	portSTACK_TYPE			param4;             /**< 4th function parameter, passed in R8. */
/*  20 */	portSTACK_TYPE			param5;             /**< 5th function parameter, passed in R9. */
/*  24 */	portSTACK_TYPE			param6;             /**< 6th function parameter, passed in R10. */
/* HIGH MEMORY */
} stack_frame_t;

/**
 * @struct task_context_t
 * @brief Represents the stack frame used to save the context of a task when it yields or is interrupted.
 */
typedef struct task_context {
/* LOW MEMORY */
/*   0 */	void					*returnAddress;     /**< Return address to task. Used to save R14 during an interrupted context. */
/*   4 */	portSTACK_TYPE			calleeParam1;       /**< 1st callee parameter, passed in R5. */
/*   8 */	portSTACK_TYPE			calleeParam2;       /**< 2nd callee parameter, passed in R6. */
/*  12 */	portSTACK_TYPE			calleeParam3;       /**< 3rd callee parameter, passed in R7. */
/*  16 */	portSTACK_TYPE			calleeParam4;       /**< 4th callee parameter, passed in R8. */
/*  20 */	portSTACK_TYPE			calleeParam5;       /**< 5th callee parameter, passed in R9. */
/*  24 */	portSTACK_TYPE			calleeParam6;       /**< 6th callee parameter, passed in R10. */
/*  28 */	portSTACK_TYPE			MSR;                /**< Machine Status Register (MSR), saved in R0 slot. */
/*  32 */	portSTACK_TYPE			FSR;                /**< Floating Point Status Register (FSR), saved in R1 slot. */
/*  36 */	portSTACK_TYPE			R2;                 /**< Read-only small data area anchor (SDA2). Not saved because it is constant through program execution. */
/*  40 */	portSTACK_TYPE			R3;
/*  44 */	portSTACK_TYPE			R4;
/*  48 */	portSTACK_TYPE			R5;
/*  52 */	portSTACK_TYPE			R6;
/*  56 */	portSTACK_TYPE			R7;
/*  60 */	portSTACK_TYPE			R8;
/*  64 */	portSTACK_TYPE			R9;
/*  68 */	portSTACK_TYPE			R10;
/*  72 */	portSTACK_TYPE			R11;
/*  76 */	portSTACK_TYPE			R12;
/*  80 */	portSTACK_TYPE			R13;                /**< Read/write small data area anchor (SDA). Not saved because it is constant through program execution. */
/*  84 */	task_context_state_t	state;  		    /**< Task context state, saved in R14 slot. */
/*  88 */	portSTACK_TYPE			R15;                /**< Sub-routine return address (R15). Must be saved across context switches. */
/*  92 */	portSTACK_TYPE			R16;
/*  96 */	portSTACK_TYPE			R17;
/* 100 */	portSTACK_TYPE			R18;
/* 104 */	portSTACK_TYPE			uxCriticalNesting;	/**< Critical section nesting value, saved in R19 slot. */
/* 108 */	portSTACK_TYPE			R20;
/* 112 */	portSTACK_TYPE			R21;
/* 116 */	portSTACK_TYPE			R22;
/* 120 */	portSTACK_TYPE			R23;
/* 124 */	portSTACK_TYPE			R24;
/* 128 */	portSTACK_TYPE			R25;
/* 132 */	portSTACK_TYPE			R26;
/* 136 */	portSTACK_TYPE			R27;
/* 140 */	portSTACK_TYPE			R28;
/* 144 */	portSTACK_TYPE			R29;
/* 148 */	portSTACK_TYPE			R30;
/* 152 */	portSTACK_TYPE			R31;
/* 156 */	stack_frame_t			*framePointer;		/**< Stack frame pointer (R19). */
/* HIGH MEMORY */
} task_context_t;

/**
 * @struct task_stack_frame_t
 * @brief Represents the intitial stack frame of a task when it is first created.
 *
 * The size of this structure is what determines the initial stack pointer for the
 * stack that is initialized in pxPortInitialiseStack.
 */
typedef struct task_stack_frame {
/* LOW MEMORY */
			task_context_t			context;            /**< Initialized task context. */
			stack_frame_t			callerFrame;        /**< Stack frame of function that "calls" task. */
/* HIGH MEMORY */
} task_stack_frame_t;

/**
 * @def portDISABLE_INTERRUPTS
 * @brief Disables interrupts.
 * 
 * If the special MSR bit set and bit clear instructions are available, this
 * maps to a single line of inline assembly code. Otherwise, it is mapped to
 * the function provided by the @em standalone system libraries.
 */
/**
 * @def portENABLE_INTERRUPTS
 * @brief Enables interrupts.
 * @copydetails portDISABLE_INTERRUPTS
 */
#if ( XPAR_MICROBLAZE_USE_MSR_INSTR == 1 )
	#define portDISABLE_INTERRUPTS()			asm volatile ( "msrclr r0 0x2" )
	#define portENABLE_INTERRUPTS()				asm volatile ( "msrset r0 0x2" )
#else
	#define portDISABLE_INTERRUPTS()			microblaze_disable_interrupts()
	#define portENABLE_INTERRUPTS()				microblaze_enable_interrupts()
#endif

/**
 * @brief Enters a critical section. Maps to portENTER_CRITICAL().
 *
 * Enters a critical section by disabling interrupts and incrementing the
 * task-specific nesting value.
 */
#if ( portUSE_CRITICAL_SECTION_MACROS == 1 )
	#define vPortEnterCritical(xProcessor, pxMutex)						\
		do {															\
			vCPUAcquireMutex(xProcessor, (void *)pxMutex);				\
		} while (0)
#else
	void vPortEnterCritical(void);
#endif
/**
 * @def portENTER_CRITICAL
 * @copydetails vPortEnterCritical
 */
#define portENTER_CRITICAL(pxMutex, xProcessor)				vPortEnterCritical(pxMutex, xProcessor)

/**
 * @brief Exits a critical section. Maps to portEXIT_CRITICAL().
 *
 * Leaves a critical section by first decrementing the task-specific nesting
 * value and then enabling interrupts if the nesting value has completely
 * unwound and reached zero. By definition, interrupts are disabled when
 * entering this function. This is defined as a separate function to both
 * save code space and make it easier to trap critical section exits in the
 * debugger.
 */
#if ( portUSE_CRITICAL_SECTION_MACROS == 1 )
	#define vPortExitCritical(xProcessor, pxMutex)								\
		do {																	\
			vCPUReleaseMutex(xProcessor, (void *)pxMutex);						\
																				\
		} while (0)
#else
	void vPortExitCritical(void);
#endif
/**
 * @def portEXIT_CRITICAL
 * @copydetails vPortExitCritical
 */
#define portEXIT_CRITICAL(pxMutex, xProcessor)					vPortExitCritical(pxMutex, xProcessor)

/**
 * @brief Manual context switch (yield) from mainline code. Maps to
 * portYIELD() and taskYIELD().
 *
 * Performs a manual context switch within a critical section. The context switch
 * is done inside of a critical section to ensure that it is not interrupted.
 * This is OK because each task maintains its own interrupt and critical
 * nesting status.
 */
void vPortYield(void);
/**
 * @def portYIELD
 * @copydetails vPortYield
 */
#define portYIELD()							vPortYield()

#define portNUM_PROCESSORS			( (unsigned portBASE_TYPE ) 2 ) /* MULTI-CORE MOD */
#define portDEFAULT_PROCESSOR		( (unsigned portBASE_TYPE) 0 ) /* MULTI-CORE MOD */
#define portNO_SPECIFIC_PROCESSOR	( (unsigned portBASE_TYPE) portNUM_PROCESSORS + 1) /* MULTI-CORE MOD */

#define portMAX_TASKS				( (unsigned portBASE_TYPE ) 20 ) /* MULTI-CORE MOD */

#define portMAX_MUTEXES				( (unsigned portBASE_TYPE ) 10 ) /* MULTI-CORE MOD */
#define portNULL_MUTEX				( (signed portBASE_TYPE ) -1 ) /* MULTI-CORE MOD */

/**
 * @def vPortSwitchContext
 * @brief Notifies the master interrupt handler to perform a context switch.
 *
 * If a context switch needs to be performed from within an ISR, the ISR
 * should call vPortSwitchContext(). This function simply sets the yield
 * flag which the master interrupt handler then checks once all ISRs have
 * been completed. Note that its OK to directly access this variable because
 * its only accessed from within an ISR, where interrupts are disabled by
 * definition.
 */
#if ( configUSE_PREEMPTION == 1 )
	#define vPortSwitchContext(processor)									\
		do {																\
			extern volatile portBASE_TYPE ulYieldFlag[portNUM_PROCESSORS];	\
			ulYieldFlag[processor]++;										\
		} while (0)
#else
	#define vPortSwitchContext()
#endif

/**
 * @brief Restores the context of the current task and returns control to that
 * task. Defined in portasm.S.
 * @note Does not return to caller.
 *
 * Restores the context of a task and returns control to that task. Interrupts
 * are optionally enabled when returning to the task based on the state of the
 * IE bit in the saved MSR.
 */
void vPortRestoreContext(void);

/**
 * @brief Saves the context of the current task. Defined in portasm.S.
 *
 * Saves registers R11, R12, R18 and R20 through R31. Saves all required task
 * context. Caller is responsible for saving volatile registers R3 to R10 and R15.
 *
 * @param context (R5)
 * @param returnAddress (R6)
 * @param enableInterrupts (R7)
 * @param state (R8)
 */
void vPortSaveContext(task_context_t *context, void *returnAddress, portBASE_TYPE enableInterrupts, portBASE_TYPE state);

/**
 * @brief Global interrupt handler installed at the interrupt vector by the
 * linker. Defined in portasm.S.
 *
 * Performs three major operations:
 * 1) Saves the context of the currently executing task.
 * 2) Calls the handler provided by the xps_intc interrupt controller.
 * 3) Restores the context of currently executing task. Note that the call
 * to the interrupt controller may have triggered a context switch.
 * 
 * Upon entering this function:
 * 1) Interrupts are disabled, but were enabled just before we vectored here.
 * 2) The return address is saved in R14 (not R15).
 * 3) Volatiles are not dirty because this was a clean function call with interrupts
 * disabled.
 * 4) All volatile registers are considered dirty, including R15.
 */
void _interrupt_handler(void);

/**
 * @brief Performs a context switch from a yielded task. Defined in portasm.S.
 * @see vPortYield
 * @note Does not return to caller directly.
 *
 * This function will effectively return when a task's context is restored.
 * This is because the return address from calling function is saved in R15, which
 * is saved as the return address in the task's context.
 * 
 * Upon entering this function:
 * 1) Interrupts are disabled because the yield is done from within a critical section.
 * 2) The return address to the task is saved in R15.
 * 3) Volatiles are not dirty because this was a clean function call with interrupts disabled.
 * 4) We may be returning to a task that originally had interrupts enabled.
 */
void vPortYieldASM(void);

/**
 * @brief Starts the first task by calling vPortRestoreContext. Defined in
 * portasm.S.
 * @note Does not return to caller.
 *
 * Starts the first task by calling vPortRestoreContext. Before the first
 * task is started, the current stack pointer for mainline code is saved into
 * a global variable so that it can be used later while servicing interrupts
 * and performing context switches after a task yields. This reduces the stack
 * size requirements for tasks.
 */
void vPortStartFirstTask(void);

/**
 * @brief Determines the high-water mark for the mainline stack Defined in port.c.
 * 
 * @note Analogous to usTaskCheckFreeStackSpace defined in tasks.c. Only defined
 * if INCLUDE_uxTaskGetStackHighWaterMark is enabled.
 * 
 * @return Minimum free stack space available on the mainline stack.
 */
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
	unsigned portSHORT usMainCheckFreeStackSpace(void);
#endif

void vTickISR(void *CallBackRef, unsigned portCHAR nTmrCtrNumber);

/*
 * Include additional port-specific headers. Including this last ensures that all
 * of the definitions defined above are valid in the following includes. This is
 * kept inside the extern "C" bracket because porthw.h does include C++ protection.
 */
#include "porthw.h"

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */

/** @} */
