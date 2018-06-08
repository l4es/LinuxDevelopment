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

#define BRIDGE_SWAP_ADDR			XPAR_DDR2_SDRAM_MPMC_BASEADDR

/**
 * @file portdefs.h
 * @ingroup freertosmbport
 * @author Tyrel Newton <newton@tethers.com>, Tethers Unlimited, Inc.
 * @brief Definitions that are used exclusively in the context of this port.
 * @attention This file is included in portasm.S which means it can only contain
 * pre-processor definitions.
 */

#ifndef PORTDEFS_H
#define PORTDEFS_H

/**
 * @def portDEBUG_STACK_FRAMES
 * @ingroup freertosmbport
 * @brief Disabled by default. Enables the use of global stack frame
 * pointers to aid debugging.
 * 
 * Disabled by default. Enabling this will cause the assembly language
 * functions in portasm.S to set global pointers to the various stack
 * frame structures. These global pointers are declared in port.c.
 * By enabling this definition, you can single-step through the assembly
 * language code and watch modifications to the current stack frame by
 * watching these global pointers.
 */
#define portDEBUG_STACK_FRAMES					0

/**
 * @def portDEBUG_CONTEXT_SWITCHES
 * @ingroup freertosmbport
 * @brief Disabled by default. Enables context switch debugging to stdout.
 *
 * Enabling this will wrap the vTaskSwitchContext call with another
 * function call that prints information about the current TCB before and
 * after the context switch to stdout.
 *
 * @warning Enabling this will enable the functions defined in portdbg.c.
 * These functions are dependent on custom modifications to the FreeRTOS
 * source code contained in tasks.c that are NOT distributed with these
 * port files. If you enable this definition without these custom
 * modifications, the code will not compile.
 */
#define portDEBUG_CONTEXT_SWITCHES				0

/**
 * @def portUSE_CRITICAL_SECTION_MACROS
 * @ingroup freertosmbport
 * @brief Enablse the use of macros instead of functions for entering and
 * exiting critical sections.
 * 
 * Using macros will increase code size, speed up code execution, and reduce
 * stack usage. Using functions will make it easier to debug entering and
 * exiting critical sections because there will be a single piece of code to
 * set a breakpoint on.
 */
#define portUSE_CRITICAL_SECTION_MACROS			1

/**
 * @def portSTACK_FILL_VALUE
 * @ingroup freertosmbport
 * @brief Value used to fill the mainline (ISR) stack when the first task
 * is started.
 * 
 * @note This is the same fill value that is used to fill a task's stack
 * when it is first initialized.
 * 
 * If stack high-water mark checking is enabled, the mainline and ISR stack
 * will be filled with this value just before the first task is started.
 * This allows the high-water mark for the mainline stack to be checked at
 * runtime.
 */
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )
	#define portSTACK_FILL_VALUE				0xA5A5A5A5
#endif

/**
 * @defgroup mbhwdef Hardware Definitions
 * @ingroup freertosmbport
 * @brief Hardware definitions required for interfacing to the MicroBlaze
 * processor and its peripherals.
 *
 * Ensures that the definitions defined in FreeRTOSConfig.h and xparameters.h
 * that are used by this port are both defined and valid. Xilinx changes the
 * behavior of their toolchains often so these definitions may be deprecated
 * by future versions of the development tools.
 *
 * @{
 */
#ifndef FREERTOS_CONFIG_H
    #error FreeRTOSConfig.h must be included before portdef.h.
#endif
/**
 * @def portXPS_INTC_DEVICE_ID
 * @see FreeRTOSConfig.h
 * 
 * Specifies the device ID of the xps_intc
 * instance that will be managed by the kernel via this port. The default
 * value of zero should be sufficient for the majority of systems where
 * only one interrupt controller is instantiated.
 */
#ifndef portXPS_INTC_DEVICE_ID
	#define portXPS_INTC_DEVICE_ID				0
#endif
/**
 * @def portXPS_TIMER_DEVICE_ID
 * @see FreeRTOSConfig.h
 *
 * Specifies the device ID of the xps_timer
 * instance that will be used to generate the periodic tick interrupt for
 * the kernel. The default value is zero. However, if there are multiple
 * timers defined in the system, the definition should be set to
 * the desired value in FreeRTOSConfig.h.
 */
#ifndef portXPS_TIMER_DEVICE_ID
	#define portXPS_TIMER_DEVICE_ID				0
#endif
/**
 * @def portXPS_TIMER_COUNTER_NUMBER
 * @see FreeRTOSConfig.h
 * 
 * Specifies the counter number within
 * a single xps_timer instance that will be used to generate the periodic
 * tick interrupt for the kernel. Each xps_timer instance includes two
 * timers, so the allowed values for this definition are 0 and 1. By default,
 * the first counter is used, i.e. counter 0.
 */
#ifndef portXPS_TIMER_COUNTER_NUMBER
	#define portXPS_TIMER_COUNTER_NUMBER		0
#else
	#if ( portXPS_TIMER_COUNTER_NUMBER < 0 || portXPS_TIMER_COUNTER_NUMBER > 1 )
		#error Invalid definition: portXPS_TIMER_COUNTER_NUMBER must be either 0 or 1.
	#endif
#endif
/**
 * @def portXPS_TIMER_INTERRUPT_ID
 * @see FreeRTOSConfig.h
 * 
 * Specifies the interrupt number for the
 * xps_timer instance that is used to generate periodic tick interrupt for
 * the kernel. There is no reasonable default value for this definition, so
 * it must be defined in FreeRTOSConfig.h. The interrupt number represents
 * the priority of the interrupt as handled by the xps_intc interrupt
 * controller with 0 being the highest priority interrupt. Note that the
 * interrupt number is specified at MicroBlaze synthesis time.
 */
#ifndef portXPS_TIMER_INTERRUPT_ID
	#error Missing definition: portXPS_TIMER_INTERRUPT_ID must be defined as the interrupt number of the timer used to generate the periodic operating system tick interrupt.
#endif
/**
 * @def portGENERATE_ISR_SIGNAL
 * @see FreeRTOSConfig.h
 * @see portISR_SIGNAL_ADDRESS
 * @see portISR_SIGNAL_MASK
 * 
 * Tells the primary interrupt handler to generate a toggle signal whose
 * width indicates the time spent within the interrupt handler. This signal
 * is generated by setting a bit in memory at the beginning of the handler
 * and then clearing that bit at the end of the handler. The bit in the specified
 * memory address should be mapped to an output pin so that the signal can be
 * monitored on an oscilliscope or similar device. To keep the generation of this
 * signal as lightweight and non-intrusive as possible, no provisions are made for
 * guaranteeing that the memory mapped I/O bit is actually an output. This signal
 * is generated in assembly language in portmacro.S. Setting and clearing the bit
 * takes only three instructions each.
 * 
 * Pseudo-code for the generation of this signal looks like:
 * @code
 * 	interrupt_handler:
 * 		// save context
 * 		MEM[address] |= mask;
 * 		// service interrupts
 * 		MEM[address] &= ~mask;
 * 		// restore context
 * @endcode
 */
#ifndef portGENERATE_ISR_SIGNAL
	#define portGENERATE_ISR_SIGNAL				0
#endif
/**
 * @def portISR_SIGNAL_ADDRESS
 * @see FreeRTOSConfig.h
 * 
 * Address containing the bit that will represent the signal generated in the
 * primary interrupt handler. The width of this signal indicates the time spent
 * servicing interrupts.
 */
#if ( portGENERATE_ISR_SIGNAL == 1 )
	#ifndef portISR_SIGNAL_ADDRESS
		#warning Missing definition: portISR_SIGNAL_ADDRESS must be defined if portGENERATE_ISR_SIGNAL is being used.
		#undef portGENERATE_ISR_SIGNAL
		#define portGENERATE_ISR_SIGNAL			0
	#elif ( portISR_SIGNAL_ADDRESS == 0 )
		#error Invalid definition: portISR_SIGNAL_ADDRESS cannot be zero.
	#endif
#endif
/**
 * @def portISR_SIGNAL_MASK
 * @see FreeRTOSConfig.h
 * 
 * Bit mask containg the bit that will represent the signal generated in the
 * primary interrupt handler. The width of this signal indicates the time spent
 * servicing interrupts. Normally, only one bit should be set in this mask.
 */
#if ( portGENERATE_ISR_SIGNAL == 1 )
	#ifndef portISR_SIGNAL_MASK
		#warning Missing definition: portISR_SIGNAL_MASK must be defined if portGENERATE_ISR_SIGNAL is being used.
		#undef portGENERATE_ISR_SIGNAL
		#define portGENERATE_ISR_SIGNAL			0
	#elif ( portISR_SIGNAL_MASK == 0 )
		#error Invalid definition: portISR_SIGNAL_MASK cannot be zero.
	#endif
#endif
/**
 * @def XPAR_MICROBLAZE_D_OPB
 * @see xparameters.h
 */
/*
#ifndef XPAR_MICROBLAZE_D_OPB
	#define XPAR_MICROBLAZE_D_OPB				0
	#warning Not defined: XPAR_MICROBLAZE_D_OPB. Is this port out of date?
#endif
*/

/**
 * @def XPAR_MICROBLAZE_I_OPB
 * @see xparameters.h
 */

/*
#ifndef XPAR_MICROBLAZE_I_OPB
	#define XPAR_MICROBLAZE_I_OPB				0
	#warning Not defined: XPAR_MICROBLAZE_I_OPB. Is this port out of date?
#endif
*/

/**
 * @def XPAR_MICROBLAZE_USE_ICACHE
 * @see xparameters.h
 */
#ifndef XPAR_MICROBLAZE_USE_ICACHE
	#define XPAR_MICROBLAZE_USE_ICACHE			0
	#warning Not defined: XPAR_MICROBLAZE_USE_ICACHE. Is this port out of date?
#endif
/**
 * @def XPAR_MICROBLAZE_USE_DCACHE
 * @see xparameters.h
 */
#ifndef XPAR_MICROBLAZE_USE_DCACHE
	#define XPAR_MICROBLAZE_USE_DCACHE			0
	#warning Not defined: XPAR_MICROBLAZE_USE_DCACHE. Is this port out of date?
#endif
/**
 * @def XPAR_MICROBLAZE_USE_MSR_INSTR
 * @see xparameters.h
 */
#ifndef XPAR_MICROBLAZE_USE_MSR_INSTR
	#define XPAR_MICROBLAZE_USE_MSR_INSTR		0
	#warning Not defined: XPAR_MICROBLAZE_USE_MSR_INSTR. Is this port out of date?
#endif
/** @} */

/**
 * @defgroup mbmsrmasks Machine Status Register Masks
 * @ingroup freertosmbport
 * @{
 */
#define portMSR_MASK_CC							0x80000000
#define portMSR_MASK_VMS						0x00004000
#define portMSR_MASK_VM							0x00002000
#define portMSR_MASK_UMS						0x00001000
#define portMSR_MASK_UM							0x00000800
#define portMSR_MASK_PVR						0x00000400
#define portMSR_MASK_EIP						0x00000200
#define portMSR_MASK_EE							0x00000100
#define portMSR_MASK_DCE						0x00000080
#define portMSR_MASK_DZ							0x00000040
#define portMSR_MASK_ICE						0x00000020
#define portMSR_MASK_FSL						0x00000010
#define portMSR_MASK_BIP						0x00000008
#define portMSR_MASK_C							0x00000004
#define portMSR_MASK_IE							0x00000002
#define portMSR_MASK_BE							0x00000001
/** @} */

/**
 * @defgroup mbfsrmasks Floating Point Status Register Masks
 * @ingroup freertosmbport
 * @{
 */
#define portFSR_MASK_IO							0x00000010
#define portFSR_MASK_DZ							0x00000008
#define portFSR_MASK_OF							0x00000004
#define portFSR_MASK_UF							0x00000002
#define portFSR_MASK_DO							0x00000001
/** @} */

/**
 * @def portINITIAL_CRITICAL_NESTING
 * @ingroup freertosmbport
 * @brief Initial critical section nesting value.
 *
 * Although tasks are started with a critical section nesting of 0, prior to the
 * scheduler being started, we don't want the critical nesting level to reach
 * zero, so it is initialized to a high value. During an interrupt, we also
 * initialize the critical nesting to this value so that critical section macros
 * can be used within ISRs.
 */
#define portINITIAL_CRITICAL_NESTING			0x7FFFFFFF

/**
 * @def portTASK_CONTEXT_SIZE
 * @ingroup freertosmbport
 * @brief Size of the stack frame used to hold the currenttly executing task's context.
 * @see task_context_t
 * 
 * 4 because the size of a word is 4 bytes (32-bits) and 40 because the size of the
 * task_context_t structure is 40 words (160 bytes).
 */
#define portTASK_CONTEXT_SIZE					( 4 * 40 )
/**
 * @def portTASK_CONTEXT_OFFSET
 * @ingroup freertosmbport
 * @brief Returns byte offset for slot of numbered register within context frame.
 * @see task_context_t
 * @param reg Register number, i.e. 0 to 31.
 *
 * If re-written as C-style macro, this would be become:
 * @code
 *      #define portTASK_CONTEXT_OFFSET(reg)            (sizeof(portSTACK_TYPE) * (reg + offsetof(task_context_t, MSR)/sizeof(portSTACK_TYPE)))
 * @endcode
 * where MSR is saved in the R0 slot and is located 7 words from the bottom of the
 * context frame.
 */
#define portTASK_CONTEXT_OFFSET(reg)			( 4 * ( reg + 7 ) )
/**
 * @def portTASK_CONTEXT_STATE_YIELDED
 * @ingroup freertosmbport
 * @see task_context_t
 * @brief Indicates that a task yielded.
 */
#define portTASK_CONTEXT_STATE_YIELDED			0
/**
 * @def portTASK_CONTEXT_STATE_INTERRUPTED
 * @ingroup freertosmbport
 * @see task_context_t
 * @brief Indicates that a task was interrupted.
 */
#define portTASK_CONTEXT_STATE_INTERRUPTED		1
/**
 * @def portTASK_CONTEXT_STATE_INITIALIZED
 * @ingroup freertosmbport
 * @see task_context_t
 * @brief Indicates that a task has been initialized but not started.
 */
#define portTASK_CONTEXT_STATE_INITIALIZED		2

#endif /* PORTDEFS_H */
