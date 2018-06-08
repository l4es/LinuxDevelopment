/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.
	

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "StackMacros.h"
#include "multicore.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/*
 * Macro to define the amount of stack available to the idle task.
 */
#define tskIDLE_STACK_SIZE	configMINIMAL_STACK_SIZE

/*
 * Task control block.  A task control block (TCB) is allocated to each task,
 * and stores the context of the task.
 */
typedef struct tskTaskControlBlock
{
	volatile portSTACK_TYPE	*pxTopOfStack;		/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE STRUCT. */

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS xMPUSettings;				/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE STRUCT. */
	#endif	
	
	xListItem				xGenericListItem;	/*< List item used to place the TCB in ready and blocked queues. */
	xListItem				xEventListItem;		/*< List item used to place the TCB in event lists. */
	unsigned portBASE_TYPE	uxPriority;			/*< The priority of the task where 0 is the lowest priority. */
	portSTACK_TYPE			*pxStack;			/*< Points to the start of the stack. */
	signed char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */
	unsigned portBASE_TYPE	uxCPUAffinity;
	unsigned portBASE_TYPE	uxUID; // The UID of the task

	#if ( portSTACK_GROWTH > 0 )
		portSTACK_TYPE *pxEndOfStack;			/*< Used for stack overflow checking on architectures where the stack grows up from low memory. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		unsigned portBASE_TYPE uxCriticalNesting;
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		unsigned portBASE_TYPE	uxTCBNumber;	/*< This is used for tracing the scheduler and making debugging easier only. */
	#endif

	#if ( configUSE_MUTEXES == 1 )
		unsigned portBASE_TYPE uxBasePriority;	/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		pdTASK_HOOK_CODE pxTaskTag;
	#endif

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
		unsigned long ulRunTimeCounter;		/*< Used for calculating how much CPU time each task is utilising. */
	#endif

} tskTCB;

portBASE_TYPE uxTCBSize = sizeof(void *);

/*
 * Some kernel aware debuggers require data to be viewed to be global, rather
 * than file scope.
 */
#ifdef portREMOVE_STATIC_QUALIFIER
	#define static
#endif

/*lint -e956 */
PRIVILEGED_DATA tskTCB * volatile pxCurrentTCBs[portNUM_PROCESSORS];
PRIVILEGED_DATA unsigned portBASE_TYPE volatile tskNextUID = 1;

/* Used by the thread-safe mutex API in its implementation of a generalised Peterson's algorithm */

enum pstate {IDLE, WAITING, ACTIVE};
static volatile portBASE_TYPE flags[portMAX_TASKS];
static volatile portBASE_TYPE turn;

/*
 * Global mutex variables for kernel synchronisation
 */

portBASE_TYPE mutexesInitialised = pdFALSE;
PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskUIDMutexFlags[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskUIDMutexTurns[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskUIDMutexNesting[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskUIDMutexEntryStage[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile void * TaskUIDMutex[4] = { &pxTaskUIDMutexFlags, &pxTaskUIDMutexTurns, &xTaskUIDMutexNesting, &xTaskUIDMutexEntryStage };

PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskQueueMutexFlags[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskQueueMutexTurns[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskQueueMutexNesting[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskQueueMutexEntryStage[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile void * TaskQueueMutex[4] = { &pxTaskQueueMutexFlags, &pxTaskQueueMutexTurns, &xTaskQueueMutexNesting, &xTaskQueueMutexEntryStage };

PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskPriorityMutexFlags[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE pxTaskPriorityMutexTurns[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskPriorityMutexNesting[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTaskPriorityMutexEntryStage[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile void * TaskPriorityMutex[4] = { &pxTaskPriorityMutexFlags, &pxTaskPriorityMutexTurns, &xTaskPriorityMutexNesting, &xTaskPriorityMutexEntryStage };

PRIVILEGED_DATA static volatile portBASE_TYPE pxTickCountMutexFlags[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE pxTickCountMutexTurns[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTickCountMutexNesting[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile portBASE_TYPE xTickCountMutexEntryStage[portNUM_PROCESSORS];
PRIVILEGED_DATA static volatile void * TickCountMutex[4] = { &pxTickCountMutexFlags, &pxTickCountMutexTurns, &xTickCountMutexNesting, &xTickCountMutexEntryStage };

PRIVILEGED_DATA volatile portBASE_TYPE pxMemMutexFlags[portNUM_PROCESSORS];
PRIVILEGED_DATA volatile portBASE_TYPE pxMemMutexTurns[portNUM_PROCESSORS];
PRIVILEGED_DATA volatile portBASE_TYPE xMemMutexNesting[portNUM_PROCESSORS];
PRIVILEGED_DATA volatile portBASE_TYPE xMemMutexEntryStage[portNUM_PROCESSORS];
PRIVILEGED_DATA volatile void * MemMutex[4] = { &pxMemMutexFlags, &pxMemMutexTurns, &xMemMutexNesting, &xMemMutexEntryStage };

/*
 * Structs for task synchronisation
 */

typedef struct taskNamedMutexTurn {

	signed portBASE_TYPE		mutexName; // The mutex name (for debugging)
	portBASE_TYPE				turnTaskUID; // The owning task's UID whose turn it is

} namedMutexTurn;

typedef struct taskNamedMutexFlag {

	volatile signed portBASE_TYPE		mutexName; // The mutex name (for debugging)
	volatile portBASE_TYPE				pendingEntry; // Does the task want to acquire the mutex?
	volatile portBASE_TYPE				entryStage; // The stage at which the mutex was entered

} namedMutexFlag;

/*
 * Global mutex variables for task synchronisation
 */

PRIVILEGED_DATA static volatile namedMutexTurn 	pxTaskMutexTurns[portMAX_TASKS + 1][portMAX_MUTEXES];
PRIVILEGED_DATA static volatile namedMutexFlag 	pxTaskMutexFlags[portMAX_TASKS + 1][portMAX_MUTEXES];

/* Lists for ready and blocked tasks. --------------------*/

PRIVILEGED_DATA static xList pxReadyTasksLists[ configMAX_PRIORITIES ];	/*< Prioritised ready tasks. */
PRIVILEGED_DATA static xList xDelayedTaskList1;							/*< Delayed tasks. */
PRIVILEGED_DATA static xList xDelayedTaskList2;							/*< Delayed tasks (two lists are used - one for delays that have overflowed the current tick count. */
PRIVILEGED_DATA static xList * volatile pxDelayedTaskList ;				/*< Points to the delayed task list currently being used. */
PRIVILEGED_DATA static xList * volatile pxOverflowDelayedTaskList;		/*< Points to the delayed task list currently being used to hold tasks that have overflowed the current tick count. */
PRIVILEGED_DATA static xList xPendingReadyList;							/*< Tasks that have been readied while the scheduler was suspended.  They will be moved to the ready queue when the scheduler is resumed. */

#if ( INCLUDE_vTaskDelete == 1 )

	PRIVILEGED_DATA static volatile xList xTasksWaitingTermination;		/*< Tasks that have been deleted - but the their memory not yet freed. */
	PRIVILEGED_DATA static volatile unsigned portBASE_TYPE uxTasksDeleted = ( unsigned portBASE_TYPE ) 0;

#endif

#if ( INCLUDE_vTaskSuspend == 1 )

	PRIVILEGED_DATA static xList xSuspendedTaskList;					/*< Tasks that are currently suspended. */

#endif

/* File private variables. --------------------------------*/
PRIVILEGED_DATA static volatile unsigned portBASE_TYPE uxCurrentNumberOfTasks 	= ( unsigned portBASE_TYPE ) 0;
PRIVILEGED_DATA static volatile portTickType xTickCount 						= ( portTickType ) 0;
PRIVILEGED_DATA static unsigned portBASE_TYPE uxTopUsedPriority	 				= tskIDLE_PRIORITY;
PRIVILEGED_DATA static volatile unsigned portBASE_TYPE uxTopReadyPriority 		= tskIDLE_PRIORITY;
PRIVILEGED_DATA static volatile signed portBASE_TYPE xSchedulerRunning 			= pdFALSE;
PRIVILEGED_DATA static volatile unsigned portBASE_TYPE uxSchedulerSuspended	 	= ( unsigned portBASE_TYPE ) pdFALSE;
PRIVILEGED_DATA static volatile unsigned portBASE_TYPE uxMissedTicks 			= ( unsigned portBASE_TYPE ) 0;
PRIVILEGED_DATA static volatile portBASE_TYPE xMissedYield 						= ( portBASE_TYPE ) pdFALSE;
PRIVILEGED_DATA static volatile portBASE_TYPE xNumOfOverflows 					= ( portBASE_TYPE ) 0;
PRIVILEGED_DATA static unsigned portBASE_TYPE uxTaskNumber 						= ( unsigned portBASE_TYPE ) 0;
PRIVILEGED_DATA static portTickType xNextTaskUnblockTime						= ( portTickType ) portMAX_DELAY;

#if ( configGENERATE_RUN_TIME_STATS == 1 )

	PRIVILEGED_DATA static char pcStatsString[ 50 ] ;
	PRIVILEGED_DATA static unsigned long ulTaskSwitchedInTime = 0UL;	/*< Holds the value of a timer/counter the last time a task was switched in. */
	static void prvGenerateRunTimeStatsForTasksInList( const signed char *pcWriteBuffer, xList *pxList, unsigned long ulTotalRunTime ) PRIVILEGED_FUNCTION;

#endif

/* Debugging and trace facilities private variables and macros. ------------*/

/*
 * The value used to fill the stack of a task when the task is created.  This
 * is used purely for checking the high water mark for tasks.
 */
#define tskSTACK_FILL_BYTE	( 0xa5U )

/*
 * Macros used by vListTask to indicate which state a task is in.
 */
#define tskBLOCKED_CHAR		( ( signed char ) 'B' )
#define tskREADY_CHAR		( ( signed char ) 'R' )
#define tskDELETED_CHAR		( ( signed char ) 'D' )
#define tskSUSPENDED_CHAR	( ( signed char ) 'S' )

/*
 * Macros and private variables used by the trace facility.
 */
#if ( configUSE_TRACE_FACILITY == 1 )

	#define tskSIZE_OF_EACH_TRACE_LINE			( ( unsigned long ) ( sizeof( unsigned long ) + sizeof( unsigned long ) ) )
	PRIVILEGED_DATA static volatile signed char * volatile pcTraceBuffer;
	PRIVILEGED_DATA static signed char *pcTraceBufferStart;
	PRIVILEGED_DATA static signed char *pcTraceBufferEnd;
	PRIVILEGED_DATA static signed portBASE_TYPE xTracing = pdFALSE;
	static unsigned portBASE_TYPE uxPreviousTask = 255U;
	PRIVILEGED_DATA static char pcStatusString[ 50 ];

#endif

/*-----------------------------------------------------------*/

/* NOT FULLY MULTICORE TESTED */
/*
 * Macro that writes a trace of scheduler activity to a buffer.  This trace
 * shows which task is running when and is very useful as a debugging tool.
 * As this macro is called each context switch it is a good idea to undefine
 * it if not using the facility.
 */
#if ( configUSE_TRACE_FACILITY == 1 )

	#define vWriteTraceToBuffer()																	\
	{																								\
		if( xTracing )																				\
		{																							\
			if( uxPreviousTask != pxCurrentTCB->uxTCBNumber )										\
			{																						\
				if( ( pcTraceBuffer + tskSIZE_OF_EACH_TRACE_LINE ) < pcTraceBufferEnd )				\
				{																					\
					uxPreviousTask = pxCurrentTCB->uxTCBNumber;										\
					*( unsigned long * ) pcTraceBuffer = ( unsigned long ) xTickCount;				\
					pcTraceBuffer += sizeof( unsigned long );										\
					*( unsigned long * ) pcTraceBuffer = ( unsigned long ) uxPreviousTask;			\
					pcTraceBuffer += sizeof( unsigned long );										\
				}																					\
				else																				\
				{																					\
					xTracing = pdFALSE;																\
				}																					\
			}																						\
		}																							\
	}

#else

	#define vWriteTraceToBuffer()

#endif
/*-----------------------------------------------------------*/

/*
 * Place the task represented by pxTCB into the appropriate ready queue for
 * the task.  It is inserted at the end of the list.  One quirk of this is
 * that if the task being inserted is at the same priority as the currently
 * executing task, then it will only be rescheduled after the currently
 * executing task has been rescheduled.
 */
#define prvAddTaskToReadyQueue( pxTCB )																					\
	if( ( pxTCB )->uxPriority > uxTopReadyPriority )																	\
	{																													\
		uxTopReadyPriority = ( pxTCB )->uxPriority;																		\
	}																													\
	vListInsertEnd( ( xList * ) &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xGenericListItem ) )
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
/*
 * Macro that looks at the list of tasks that are currently delayed to see if
 * any require waking.
 *
 * Tasks are stored in the queue in the order of their wake time - meaning
 * once one tasks has been found whose timer has not expired we need not look
 * any further down the list.
 */
#define prvCheckDelayedTasks()															\
{																						\
portTickType xItemValue;																\
																						\
	/* Is the tick count greater than or equal to the wake time of the first			\
	task referenced from the delayed tasks list? */										\
	if( xTickCount >= xNextTaskUnblockTime )											\
	{																					\
		for( ;; )																		\
		{																				\
			if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )						\
			{																			\
				/* The delayed list is empty.  Set xNextTaskUnblockTime to the			\
				maximum possible value so it is extremely unlikely that the				\
				if( xTickCount >= xNextTaskUnblockTime ) test will pass next			\
				time through. */														\
				xNextTaskUnblockTime = portMAX_DELAY;									\
				break;																	\
			}																			\
			else																		\
			{																			\
				/* The delayed list is not empty, get the value of the item at			\
				the head of the delayed list.  This is the time at which the			\
				task at the head of the delayed list should be removed from				\
				the Blocked state. */													\
				pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );	\
				xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xGenericListItem ) );	\
																						\
				if( xTickCount < xItemValue )											\
				{																		\
					/* It is not time to unblock this item yet, but the item			\
					value is the time at which the task at the head of the				\
					blocked list should be removed from the Blocked state -				\
					so record the item value in xNextTaskUnblockTime. */				\
					xNextTaskUnblockTime = xItemValue;									\
					break;																\
				}																		\
																						\
				/* It is time to remove the item from the Blocked state. */				\
				vListRemove( &( pxTCB->xGenericListItem ) );							\
																						\
				/* Is the task waiting on an event also? */								\
				if( pxTCB->xEventListItem.pvContainer )									\
				{																		\
					vListRemove( &( pxTCB->xEventListItem ) );							\
				}																		\
				prvAddTaskToReadyQueue( pxTCB );										\
			}																			\
		}																				\
	}																					\
}
/*-----------------------------------------------------------*/

/*
 * Several functions take an xTaskHandle parameter that can optionally be NULL,
 * where NULL is used to indicate that the handle of the currently executing
 * task should be used in place of the parameter.  This macro simply checks to
 * see if the parameter is NULL and returns a pointer to the appropriate TCB.
 */
#define prvGetTCBFromHandle( pxHandle ) ( ( ( pxHandle ) == NULL ) ? NULL : ( tskTCB * ) ( pxHandle ) )

/* Callback function prototypes. --------------------------*/
extern void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName );
extern void vApplicationTickHook( void );
		
/* File private functions. --------------------------------*/

/*
 * Utility to ready a TCB for a given task.  Mainly just copies the parameters
 * into the TCB structure.
 */
static void prvInitialiseTCBVariables( tskTCB *pxTCB, const signed char * const pcName, unsigned portBASE_TYPE uxPriority, const xMemoryRegion * const xRegions, unsigned short usStackDepth ) PRIVILEGED_FUNCTION;

/*
 * Utility to ready all the lists used by the scheduler.  This is called
 * automatically upon the creation of the first task.
 */
static void prvInitialiseTaskLists( void ) PRIVILEGED_FUNCTION;

/*
 * The idle task, which as all tasks is implemented as a never ending loop.
 * The idle task is automatically created and added to the ready lists upon
 * creation of the first user task.
 *
 * The portTASK_FUNCTION_PROTO() macro is used to allow port/compiler specific
 * language extensions.  The equivalent prototype for this function is:
 *
 * void prvIdleTask( void *pvParameters );
 *
 */
static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters );

/*
 * Utility to free all memory allocated by the scheduler to hold a TCB,
 * including the stack pointed to by the TCB.
 *
 * This does not free memory allocated by the task itself (i.e. memory
 * allocated by calls to pvPortMalloc from within the tasks application code).
 */
#if ( ( INCLUDE_vTaskDelete == 1 ) || ( INCLUDE_vTaskCleanUpResources == 1 ) )

	static void prvDeleteTCB( tskTCB *pxTCB ) PRIVILEGED_FUNCTION;

#endif

/*
 * Used only by the idle task.  This checks to see if anything has been placed
 * in the list of tasks waiting to be deleted.  If so the task is cleaned up
 * and its TCB deleted.
 */
static void prvCheckTasksWaitingTermination( void ) PRIVILEGED_FUNCTION;

/*
 * The currently executing task is entering the Blocked state.  Add the task to
 * either the current or the overflow delayed task list.
 */
static void prvAddCurrentTaskToDelayedList( unsigned portBASE_TYPE xProcessor, portTickType xTimeToWake ) PRIVILEGED_FUNCTION;

/*
 * Allocates memory from the heap for a TCB and associated stack.  Checks the
 * allocation was successful.
 */
static tskTCB *prvAllocateTCBAndStack( unsigned short usStackDepth, portSTACK_TYPE *puxStackBuffer ) PRIVILEGED_FUNCTION;

/*
 * Called from vTaskList.  vListTasks details all the tasks currently under
 * control of the scheduler.  The tasks may be in one of a number of lists.
 * prvListTaskWithinSingleList accepts a list and details the tasks from
 * within just that list.
 *
 * THIS FUNCTION IS INTENDED FOR DEBUGGING ONLY, AND SHOULD NOT BE CALLED FROM
 * NORMAL APPLICATION CODE.
 */
#if ( configUSE_TRACE_FACILITY == 1 )

	static void prvListTaskWithinSingleList( const signed char *pcWriteBuffer, xList *pxList, signed char cStatus ) PRIVILEGED_FUNCTION;

#endif

/* NOT FULLY MULTICORE TESTED */
/*
 * When a task is created, the stack of the task is filled with a known value.
 * This function determines the 'high water mark' of the task stack by
 * determining how much of the stack remains at the original preset value.
 */
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static unsigned short usTaskCheckFreeStackSpace( const unsigned char * pucStackByte ) PRIVILEGED_FUNCTION;

#endif


/*lint +e956 */



/*-----------------------------------------------------------
 * TASK CREATION API documented in task.h
 *----------------------------------------------------------*/

signed portBASE_TYPE xTaskGenericCreate( unsigned portBASE_TYPE xProcessor, pdTASK_CODE pxTaskCode, const signed char * const pcName, unsigned short usStackDepth, void *pvApplicationParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pxCreatedTask, portSTACK_TYPE *puxStackBuffer, const xMemoryRegion * const xRegions )
{

	signed portBASE_TYPE xReturn;
	tskTCB * pxNewTCB;
	unsigned portBASE_TYPE i = 0, k = 0;
	systemTaskParameters * pvParameters;
	unsigned portBASE_TYPE currentCPU = portGetCurrentCPU();

	/*
	 * "If" statement will always execute before the scheduler starts, as there must
	 * always be at least one task created before the scheduler is started.
	 */
	if (mutexesInitialised == pdFALSE) {

		for (i=0;i<portNUM_PROCESSORS;i++) {

			pxTaskQueueMutexFlags[i] = portNO_SPECIFIC_PROCESSOR;
			pxTaskQueueMutexTurns[i] = portNO_SPECIFIC_PROCESSOR;
			xTaskQueueMutexNesting[i] = 0;
			xTaskQueueMutexEntryStage[i] = 0;

			pxTaskUIDMutexFlags[i] = portNO_SPECIFIC_PROCESSOR;
			pxTaskUIDMutexTurns[i] = portNO_SPECIFIC_PROCESSOR;
			xTaskUIDMutexNesting[i] = 0;
			xTaskUIDMutexEntryStage[i] = 0;

			pxTaskPriorityMutexFlags[i] = portNO_SPECIFIC_PROCESSOR;
			pxTaskPriorityMutexTurns[i] = portNO_SPECIFIC_PROCESSOR;
			xTaskPriorityMutexNesting[i] = 0;
			xTaskPriorityMutexEntryStage[i] = 0;

			pxTickCountMutexFlags[i] = portNO_SPECIFIC_PROCESSOR;
			pxTickCountMutexTurns[i] = portNO_SPECIFIC_PROCESSOR;
			xTickCountMutexNesting[i] = 0;
			xTickCountMutexEntryStage[i] = 0;

			pxMemMutexFlags[i] = portNO_SPECIFIC_PROCESSOR;
			pxMemMutexTurns[i] = portNO_SPECIFIC_PROCESSOR;
			xMemMutexNesting[i] = 0;
			xMemMutexEntryStage[i] = 0;

		}

		for (i=0;i<portMAX_TASKS;i++) {
			for (k=0;k<portMAX_MUTEXES;k++) {
				pxTaskMutexFlags[i][k].mutexName = portNULL_MUTEX;
				pxTaskMutexFlags[i][k].pendingEntry = 0;

				pxTaskMutexTurns[i][k].mutexName = portNULL_MUTEX;
				pxTaskMutexTurns[i][k].turnTaskUID = 0;
			}
		}

		mutexesInitialised = pdTRUE;

	}

	pvParameters = pvPortMalloc( sizeof( systemTaskParameters ) );
	pvParameters->applicationParameters = pvApplicationParameters;

	configASSERT( pxTaskCode );
	configASSERT( ( uxPriority < configMAX_PRIORITIES ) );

	/* Allocate the memory required by the TCB and stack for the new task,
	checking that the allocation was successful. */
	pxNewTCB = prvAllocateTCBAndStack( usStackDepth, puxStackBuffer );

	pvParameters->taskHandle = (xTaskHandle) pxNewTCB;

	if (xProcessor == portNO_SPECIFIC_PROCESSOR) {

		pxNewTCB->uxCPUAffinity = portNO_SPECIFIC_PROCESSOR;
		xProcessor = (unsigned portBASE_TYPE)currentCPU;

	} else {

		pxNewTCB->uxCPUAffinity = xProcessor;

	}

	taskENTER_CRITICAL(currentCPU, TaskUIDMutex);

	pxNewTCB->uxUID = tskNextUID;
	if (tskNextUID + 1 > portMAX_TASKS) tskNextUID = 1;
	else tskNextUID++;

	taskEXIT_CRITICAL(currentCPU, TaskUIDMutex);

	if( pxNewTCB != NULL )
	{
		portSTACK_TYPE *pxTopOfStack;

		#if( portUSING_MPU_WRAPPERS == 1 )
			/* Should the task be created in privileged mode? */
			portBASE_TYPE xRunPrivileged;
			if( ( uxPriority & portPRIVILEGE_BIT ) != 0x00 )
			{
				xRunPrivileged = pdTRUE;
			}
			else
			{
				xRunPrivileged = pdFALSE;
			}
			uxPriority &= ~portPRIVILEGE_BIT;
		#endif /* portUSING_MPU_WRAPPERS == 1 */

		/* Calculate the top of stack address.  This depends on whether the
		stack grows from high memory to low (as per the 80x86) or visa versa.
		portSTACK_GROWTH is used to make the result positive or negative as
		required by the port. */
		#if( portSTACK_GROWTH < 0 )
		{
			pxTopOfStack = pxNewTCB->pxStack + ( usStackDepth - ( unsigned short ) 1 );
			pxTopOfStack = ( portSTACK_TYPE * ) ( ( ( unsigned long ) pxTopOfStack ) & ( ( unsigned long ) ~portBYTE_ALIGNMENT_MASK  ) );

			/* Check the alignment of the calculated top of stack is correct. */
			configASSERT( ( ( ( unsigned long ) pxTopOfStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
		}
		#else
		{
			pxTopOfStack = pxNewTCB->pxStack;
			
			/* Check the alignment of the stack buffer is correct. */
			configASSERT( ( ( ( unsigned long ) pxNewTCB->pxStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

			/* If we want to use stack checking on architectures that use
			a positive stack growth direction then we also need to store the
			other extreme of the stack space. */
			pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( usStackDepth - 1 );
		}
		#endif

		/* Setup the newly allocated TCB with the initial state of the task. */
		prvInitialiseTCBVariables( pxNewTCB, pcName, uxPriority, xRegions, usStackDepth );

		/* Initialize the TCB stack to look as if the task was already running,
		but had been interrupted by the scheduler.  The return address is set
		to the start of the task function. Once the stack has been initialised
		the	top of stack variable is updated. */
		#if( portUSING_MPU_WRAPPERS == 1 )
		{
			pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged );
		}
		#else
		{
			pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
		}
		#endif

		/* Check the alignment of the initialised stack. */
		configASSERT( ( ( ( unsigned long ) pxNewTCB->pxTopOfStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

		if( ( void * ) pxCreatedTask != NULL )
		{
			/* Pass the TCB out - in an anonymous way.  The calling function/
			task can use this as a handle to delete the task later if
			required.*/
			*pxCreatedTask = ( xTaskHandle ) pxNewTCB;
		}
		
		/* We are going to manipulate the task queues to add this task to a
		ready list, so must make sure no interrupts occur. */
		taskENTER_CRITICAL(currentCPU, TaskQueueMutex);
		{


			portBASE_TYPE otherTasks = pdFALSE;
			if ( pxCurrentTCBs[xProcessor] != NULL ) {

				otherTasks = pdTRUE;

			}

			uxCurrentNumberOfTasks++;
			if( otherTasks == pdFALSE )
			{
				/* There are no other tasks, or all the other tasks are in
				the suspended state - make this the current task on the selected processor. */
				pxCurrentTCBs[xProcessor] =  pxNewTCB;

				if( uxCurrentNumberOfTasks == ( unsigned portBASE_TYPE ) 1 )
				{
					/* This is the first task to be created so do the preliminary
					initialisation required.  We will not recover if this call
					fails, but we will report the failure. */
					prvInitialiseTaskLists();
				}
			}
			else
			{

				/* If the scheduler is not already running, make this task the
				current task if it is the highest priority task to be created
				so far. */
				if( xSchedulerRunning == pdFALSE )
				{
					/*
					 * Need to determine on which processor to schedule this task
					 */

					if (pxCurrentTCBs[xProcessor] == NULL) {

						pxCurrentTCBs[xProcessor] = pxNewTCB;

					} else
					for ( i=0; i<portNUM_PROCESSORS; i++ ) {

						if ((pxCurrentTCBs[i] == NULL)
								&& ((pxNewTCB->uxCPUAffinity == portNO_SPECIFIC_PROCESSOR)
										|| ((pxCurrentTCBs[i] == NULL) && (i == xProcessor)))) {

							pxCurrentTCBs[i] = pxNewTCB;
							break;

						} else if ((pxCurrentTCBs[i]->uxPriority <= uxPriority)
									&& ((pxNewTCB->uxCPUAffinity == portNO_SPECIFIC_PROCESSOR)
										|| (i == pxNewTCB->uxCPUAffinity))) {

							// Found a task on the current processor of lower or equal priority
							pxCurrentTCBs[i] = pxNewTCB;
							break;

						}

					}

				}

			}

			/* Remember the top priority to make context switching faster.  Use
			the priority in pxNewTCB as this has been capped to a valid value. */
			if( pxNewTCB->uxPriority > uxTopUsedPriority )
			{
				uxTopUsedPriority = pxNewTCB->uxPriority;
			}

			#if ( configUSE_TRACE_FACILITY == 1 )
			{
				/* Add a counter into the TCB for tracing only. */
				pxNewTCB->uxTCBNumber = uxTaskNumber;
			}
			#endif
			uxTaskNumber++;

			prvAddTaskToReadyQueue( pxNewTCB );

			xReturn = pdPASS;
			traceTASK_CREATE( pxNewTCB );

		}
		taskEXIT_CRITICAL(currentCPU, TaskQueueMutex);
	}
	else
	{
		xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
		traceTASK_CREATE_FAILED();
	}

	if( xReturn == pdPASS )
	{
		if( xSchedulerRunning != pdFALSE )
		{
			/*
			 * Only reaches here if the scheduler is running and thus the created task has not been
			 * scheduled, even by this function.
			 */

			/* If the created task is of a higher priority than an executing task
			then it should run now. */

			if (pxCurrentTCBs[xProcessor] == NULL) {

				portYIELD_WITHIN_API();

			}

		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

void * getTCBList() {

	return pxCurrentTCBs[0];

}
/*-----------------------------------------------------------*/

/* NOT FULLY MULTICORE TESTED */
#if ( INCLUDE_vTaskDelete == 1 )

	/* Note: If executing, tasks may now only delete themselves */
	void vTaskDelete( xTaskHandle pxTaskToDelete )
	{
		tskTCB *pxTCB;
		unsigned portBASE_TYPE processor = portGetCurrentCPU();
		unsigned portBASE_TYPE currentlyExecuting = pdFALSE;

		if (pxTaskToDelete != NULL) {

			taskENTER_CRITICAL(processor, TaskQueueMutex);
			{
				/* Ensure a yield is performed if a currently executing task is being
				deleted. */

				if( pxTaskToDelete == pxCurrentTCBs[processor])
				{
					currentlyExecuting = pdTRUE;
				}

				/* pxTaskToDelete should never be null */
				pxTCB = prvGetTCBFromHandle( pxTaskToDelete );

				/* Remove task from the ready list and place in the	termination list.
				This will stop the task from be scheduled.  The idle task will check
				the termination list and free up any memory allocated by the
				scheduler for the TCB and stack. */
				vListRemove( &( pxTCB->xGenericListItem ) );

				/* Is the task waiting on an event also? */
				if( pxTCB->xEventListItem.pvContainer )
				{
					vListRemove( &( pxTCB->xEventListItem ) );
				}

				vListInsertEnd( ( xList * ) &xTasksWaitingTermination, &( pxTCB->xGenericListItem ) );

				/* Increment the ucTasksDeleted variable so the idle task knows
				there is a task that has been deleted and that it should therefore
				check the xTasksWaitingTermination list. */
				++uxTasksDeleted;

				/* Increment the uxTaskNumberVariable also so kernel aware debuggers
				can detect that the task lists need re-generating. */
				uxTaskNumber++;

				traceTASK_DELETE( pxTCB );
			}
			taskEXIT_CRITICAL(processor, TaskQueueMutex);

			/* Force a reschedule if we have just deleted a currently executing task. */
			if( xSchedulerRunning != pdFALSE )
			{

				if( currentlyExecuting == pdTRUE )
				{
					portYIELD_WITHIN_API();
				}

			}

		}

	}

#endif

/* NOT FULLY MULTICORE TESTED */

/*-----------------------------------------------------------
 * TASK CONTROL API documented in task.h
 *----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelayUntil == 1 )

	void vTaskDelayUntil( xTaskHandle pxTaskToDelay, portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement )
	{

		tskTCB *pxTCB;
		portTickType xTimeToWake;
		portBASE_TYPE xAlreadyYielded, xShouldDelay = pdFALSE;
		unsigned portBASE_TYPE i = 0;
		unsigned portBASE_TYPE processor = portNUM_PROCESSORS + 1;

		if (pxTaskToDelay != NULL) {

			configASSERT( pxPreviousWakeTime );
			configASSERT( ( xTimeIncrement > 0 ) );


			pxTCB = prvGetTCBFromHandle( pxTaskToDelay );

			vTaskSuspendAll();
			{


				for ( i=0;i<portNUM_PROCESSORS;i++ ) {

					if( pxTaskToDelay == pxCurrentTCBs[i] )
					{
						processor = i;
						break;
					}

				}

				/* Generate the tick time at which the task wants to wake. */
				xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

				if( xTickCount < *pxPreviousWakeTime )
				{
					/* The tick count has overflowed since this function was
					lasted called.  In this case the only time we should ever
					actually delay is if the wake time has also	overflowed,
					and the wake time is greater than the tick time.  When this
					is the case it is as if neither time had overflowed. */
					if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xTickCount ) )
					{
						xShouldDelay = pdTRUE;
					}
				}
				else
				{
					/* The tick time has not overflowed.  In this case we will
					delay if either the wake time has overflowed, and/or the
					tick time is less than the wake time. */
					if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xTickCount ) )
					{
						xShouldDelay = pdTRUE;
					}
				}

				/* Update the wake time ready for the next call. */
				*pxPreviousWakeTime = xTimeToWake;

				if( xShouldDelay != pdFALSE )
				{
					traceTASK_DELAY_UNTIL();

					/* We must remove ourselves from the ready list before adding
					ourselves to the blocked list as the same list item is used for
					both lists. */
					vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );
					prvAddCurrentTaskToDelayedList( processor, xTimeToWake );
				}
			}
			xAlreadyYielded = xTaskResumeAll();

			/* Force a reschedule if xTaskResumeAll has not already done so, we may
			have put ourselves to sleep. */
			if (( !xAlreadyYielded ) && ( processor < portNUM_PROCESSORS ))
			{
				portYIELD_WITHIN_API();
			}

		}

	}

#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelay == 1 )

	void vTaskDelay( xTaskHandle pxTaskToDelay, portTickType xTicksToDelay )
	{
		portTickType xTimeToWake;
		signed portBASE_TYPE xAlreadyYielded = pdFALSE;
		unsigned portBASE_TYPE processor = portGetCurrentCPU();

		if (pxTaskToDelay != NULL) {

			tskTCB *pxTCB = prvGetTCBFromHandle( pxTaskToDelay );

			/* A delay time of zero just forces a reschedule. */
			if( xTicksToDelay > ( portTickType ) 0 )
			{
				vTaskSuspendAll();
				{
					traceTASK_DELAY();

					/* A task that is removed from the event list while the
					scheduler is suspended will not get placed in the ready
					list or removed from the blocked list until the scheduler
					is resumed.

					This task cannot be in an event list as it is the currently
					executing task. */

					/* Calculate the time to wake - this may overflow but this is
					not a problem. */
					xTimeToWake = xTickCount + xTicksToDelay;

					/* We must remove ourselves from the ready list before adding
					ourselves to the blocked list as the same list item is used for
					both lists. */
					vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );
					prvAddCurrentTaskToDelayedList( processor, xTimeToWake );
				}
				xAlreadyYielded = xTaskResumeAll();
			}

			/* Force a reschedule if xTaskResumeAll has not already done so, we may
			have put ourselves to sleep. */
			if ( !xAlreadyYielded )
			{
				portYIELD_WITHIN_API();
			}

		}

	}

#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

	unsigned portBASE_TYPE uxTaskPriorityGet( xTaskHandle pxTask )
	{
	tskTCB *pxTCB;
	unsigned portBASE_TYPE uxReturn;
	portBASE_TYPE processor = portGetCurrentCPU();

		if (pxTask != NULL) {

			taskENTER_CRITICAL(processor, TaskPriorityMutex);
			{
				/* pxTask should always be specified */
				pxTCB = prvGetTCBFromHandle( pxTask );
				uxReturn = pxTCB->uxPriority;
			}
			taskEXIT_CRITICAL(processor, TaskPriorityMutex);

			return uxReturn;

		} else return 0;

	}

#endif
/*-----------------------------------------------------------*/

/* NOT FULLY MULTICORE TESTED */

#if ( INCLUDE_vTaskPrioritySet == 1 )

	void vTaskPrioritySet( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority )
	{
		tskTCB *pxTCB;
		unsigned portBASE_TYPE processor = portGetCurrentCPU();
		unsigned portBASE_TYPE uxCurrentPriority;
		unsigned portBASE_TYPE i = 0;
		portBASE_TYPE xYieldRequired = pdFALSE;
		portBASE_TYPE xCurrentlyExecuting = -1;

		if (pxTask != NULL) {

			configASSERT( ( uxNewPriority < configMAX_PRIORITIES ) );

			/* Ensure the new priority is valid. */
			if( uxNewPriority >= configMAX_PRIORITIES )
			{
				uxNewPriority = configMAX_PRIORITIES - ( unsigned portBASE_TYPE ) 1U;
			}

			taskENTER_CRITICAL(processor, TaskQueueMutex);
			{

				/* pxTask should never be null */
				pxTCB = prvGetTCBFromHandle( pxTask );

				for ( i=0;i<portNUM_PROCESSORS;i++ ) {

					if( pxTask == pxCurrentTCBs[i] )
					{
						xCurrentlyExecuting = i;
						break;
					}

				}

				traceTASK_PRIORITY_SET( pxTask, uxNewPriority );

				#if ( configUSE_MUTEXES == 1 )
				{
					uxCurrentPriority = pxTCB->uxBasePriority;
				}
				#else
				{
					uxCurrentPriority = pxTCB->uxPriority;
				}
				#endif

				if( uxCurrentPriority != uxNewPriority )
				{
					/* The priority change may have readied a task of higher
					priority than the calling task. */
					if( uxNewPriority > uxCurrentPriority )
					{
						if( xCurrentlyExecuting == -1 )
						{
							/* The priority of another task is being raised.  If we
							were raising the priority of a currently running task
							there would be no need to switch as it must have already
							been the highest priority task. */
							xYieldRequired = pdTRUE;
						}
					}
					else if( xCurrentlyExecuting > -1 )
					{
						/* Setting a running task's priority down means there may now be another
						task of higher priority that is ready to execute. */
						xYieldRequired = pdTRUE;
					}



					#if ( configUSE_MUTEXES == 1 )
					{
						/* Only change the priority being used if the task is not
						currently using an inherited priority. */
						if( pxTCB->uxBasePriority == pxTCB->uxPriority )
						{
							pxTCB->uxPriority = uxNewPriority;
						}

						/* The base priority gets set whatever. */
						pxTCB->uxBasePriority = uxNewPriority;
					}
					#else
					{
						pxTCB->uxPriority = uxNewPriority;
					}
					#endif

					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( configMAX_PRIORITIES - ( portTickType ) uxNewPriority ) );

					/* If the task is in the blocked or suspended list we need do
					nothing more than change it's priority variable. However, if
					the task is in a ready list it needs to be removed and placed
					in the queue appropriate to its new priority. */
					if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ uxCurrentPriority ] ), &( pxTCB->xGenericListItem ) ) )
					{
						/* The task is currently in its ready list - remove before adding
						it to it's new ready list.  As we are in a critical section we
						can do this even if the scheduler is suspended. */
						vListRemove( &( pxTCB->xGenericListItem ) );
						prvAddTaskToReadyQueue( pxTCB );
					}

					if( xYieldRequired == pdTRUE )
					{
						portYIELD_WITHIN_API();
					}
				}
			}
			taskEXIT_CRITICAL(processor, TaskQueueMutex);

		}
	}

#endif
/*-----------------------------------------------------------*/

/* NOT FULLY MULTICORE TESTED */

#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskSuspend( xTaskHandle pxTaskToSuspend )
	{
	tskTCB *pxTCB;
	unsigned portBASE_TYPE processor = portGetCurrentCPU();
	unsigned portBASE_TYPE i = 0;
	portBASE_TYPE currentlyExecuting = -1;

	if (pxTaskToSuspend != NULL) {

			taskENTER_CRITICAL(processor, TaskQueueMutex);
			{
				/* Ensure a yield is performed if a current task is being
				suspended. */
				for ( i=0;i<portNUM_PROCESSORS;i++ ) {

					if( pxTaskToSuspend == pxCurrentTCBs[i] )
					{
						currentlyExecuting = i;
						break;
					}

				}

				/* Null should never be used here */
				pxTCB = prvGetTCBFromHandle( pxTaskToSuspend );

				traceTASK_SUSPEND( pxTCB );

				/* Remove task from the ready/delayed list and place in the	suspended list. */
				vListRemove( &( pxTCB->xGenericListItem ) );

				/* Is the task waiting on an event also? */
				if( pxTCB->xEventListItem.pvContainer )
				{
					vListRemove( &( pxTCB->xEventListItem ) );
				}

				vListInsertEnd( ( xList * ) &xSuspendedTaskList, &( pxTCB->xGenericListItem ) );


				if( currentlyExecuting != -1 )
				{
					if( xSchedulerRunning != pdFALSE )
					{
						/* We have just suspended a currently executing task. */
						portYIELD_WITHIN_API();
					}
					else
					{
						/* The scheduler is not running, but an executing task has just been suspended and pxCurrentTCBs
						must be adjusted to point to a different task. */
						if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList ) == uxCurrentNumberOfTasks )
						{
							/* No other tasks are ready, so set the pxCurrentTCBs element
							for the suspended task's processor back to NULL so when the next
							task is created pxCurrentTCB will be set to point to it no matter
							what its relative priority is. */
							pxCurrentTCBs[currentlyExecuting] = NULL;
						}
						else
						{
							vTaskSwitchContext();
						}
					}
				}

			}

			taskEXIT_CRITICAL(processor, TaskQueueMutex);

		}
	}

#endif
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

	signed portBASE_TYPE xTaskIsTaskSuspended( xTaskHandle xTask )
	{
	portBASE_TYPE xReturn = pdFALSE;
	const tskTCB * const pxTCB = ( tskTCB * ) xTask;

		/* It does not make sense to check if the calling task is suspended. */
		configASSERT( xTask );

		/* Is the task we are attempting to resume actually in the
		suspended list? */
		if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList, &( pxTCB->xGenericListItem ) ) != pdFALSE )
		{
			/* Has the task already been resumed from within an ISR? */
			if( listIS_CONTAINED_WITHIN( &xPendingReadyList, &( pxTCB->xEventListItem ) ) != pdTRUE )
			{
				/* Is it in the suspended list because it is in the
				Suspended state?  It is possible to be in the suspended
				list because it is blocked on a task with no timeout
				specified. */
				if( listIS_CONTAINED_WITHIN( NULL, &( pxTCB->xEventListItem ) ) == pdTRUE )
				{
					xReturn = pdTRUE;
				}
			}
		}

		return xReturn;
	}

#endif
/*-----------------------------------------------------------*/


/* NOT FULLY MULTICORE TESTED */

#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskResume( xTaskHandle pxTaskToResume )
	{
	tskTCB *pxTCB;
	unsigned portBASE_TYPE i = 0;
	portBASE_TYPE processor = portGetCurrentCPU();

		/* It does not make sense to resume an executing task. */
		configASSERT( pxTaskToResume );

		/* Remove the task from whichever list it is currently in, and place
		it in the ready list. */
		pxTCB = ( tskTCB * ) pxTaskToResume;

		taskENTER_CRITICAL(processor, TaskQueueMutex);
		{

			/* The parameter cannot be NULL */
			if ( pxTCB != NULL )
			{

				portBASE_TYPE taskRunning = pdFALSE;
				for ( i=0;i<portNUM_PROCESSORS;i++ ) {

					if (pxCurrentTCBs[i] == pxTCB) {

						taskRunning = pdTRUE;
						break;

					}

				}

				if (taskRunning == pdFALSE) {

					if( xTaskIsTaskSuspended( pxTCB ) == pdTRUE )
					{
						traceTASK_RESUME( pxTCB );

						/* As we are in a critical section we can access the ready
						lists even if the scheduler is suspended. */
						vListRemove(  &( pxTCB->xGenericListItem ) );
						prvAddTaskToReadyQueue( pxTCB );

						/* We may have just resumed a higher priority task. */
						if( (pxCurrentTCBs[processor]->uxPriority <= pxTCB->uxPriority)
								&& ((pxTCB->uxCPUAffinity == portNO_SPECIFIC_PROCESSOR) || (pxTCB->uxCPUAffinity == processor)) )
						{
							/* This yield may not cause the task just resumed to run, but
							will leave the lists in the correct state for the next yield. */
							portYIELD_WITHIN_API();
						}
					}


				}


			}

		}
		taskEXIT_CRITICAL(processor, TaskQueueMutex);
	}

#endif

/*-----------------------------------------------------------*/

/* NOT FULLY MULTICORE TESTED */

#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

	portBASE_TYPE xTaskResumeFromISR( xTaskHandle pxTaskToResume )
	{
	portBASE_TYPE xYieldRequired = pdFALSE;
	tskTCB *pxTCB;
	unsigned portBASE_TYPE i = 0;

		configASSERT( pxTaskToResume );

		pxTCB = ( tskTCB * ) pxTaskToResume;

		if (pxTCB != NULL) {

			if( xTaskIsTaskSuspended( pxTCB ) == pdTRUE )
			{
				traceTASK_RESUME_FROM_ISR( pxTCB );

				if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
				{

					xYieldRequired = pdFALSE;
					for ( i=0;i<portNUM_PROCESSORS;i++ ) {

						if (pxTCB->uxPriority >= pxCurrentTCBs[i]->uxPriority) {

							xYieldRequired = pdTRUE;
							break;

						}

					}

					vListRemove(  &( pxTCB->xGenericListItem ) );
					prvAddTaskToReadyQueue( pxTCB );
				}
				else
				{
					/* We cannot access the delayed or ready lists, so will hold this
					task pending until the scheduler is resumed, at which point a
					yield will be performed if necessary. */
					vListInsertEnd( ( xList * ) &( xPendingReadyList ), &( pxTCB->xEventListItem ) );
				}
			}

		}

		return xYieldRequired;
	}

#endif




/*-----------------------------------------------------------
 * PUBLIC SCHEDULER CONTROL documented in task.h
 *----------------------------------------------------------*/

void vTaskStartScheduler( void )
{
portBASE_TYPE xReturn;
unsigned portBASE_TYPE i, m;

	for (i=0;i<portMAX_TASKS;i++) {

		flags[i] = IDLE;

		for (m=0;m<portMAX_MUTEXES;m++) {

			pxTaskMutexTurns[i][m].mutexName = portNULL_MUTEX;
			pxTaskMutexTurns[i][m].turnTaskUID = 0;

			pxTaskMutexFlags[i][m].mutexName = portNULL_MUTEX;
			pxTaskMutexFlags[i][m].pendingEntry = 0;

		}

	}

	unsigned portBASE_TYPE idleProcessors[portNUM_PROCESSORS];
	for ( i=0;i<portNUM_PROCESSORS;i++) {

		idleProcessors[i] = i;
		/* Add the idle tasks at the lowest priority. */
		xReturn = xTaskCreate( i, prvIdleTask, ( signed char * ) "IDLE", tskIDLE_STACK_SIZE, &idleProcessors[i], ( tskIDLE_PRIORITY | portPRIVILEGE_BIT ), ( xTaskHandle * ) NULL );

	}

	#if ( configUSE_TIMERS == 1 )
	{
		if( xReturn == pdPASS )
		{
			xReturn = xTimerCreateTimerTask();
		}
	}
	#endif

	if( xReturn == pdPASS )
	{

		/* Interrupts are turned off here, to ensure a tick does not occur
		before or during the call to xPortStartScheduler().  The stacks of
		the created tasks contain a status word with interrupts switched on
		so interrupts will automatically get re-enabled when the first task
		starts to run.

		STEPPING THROUGH HERE USING A DEBUGGER CAN CAUSE BIG PROBLEMS IF THE
		DEBUGGER ALLOWS INTERRUPTS TO BE PROCESSED. */
		portDISABLE_INTERRUPTS();
		xSchedulerRunning = pdTRUE;
		xTickCount = ( portTickType ) 0;

		/* If configGENERATE_RUN_TIME_STATS is defined then the following
		macro must be defined to configure the timer/counter used to generate
		the run time counter time base. */
		portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();
		
		/* Setting up the timer tick is hardware specific and thus in the
		portable interface. */
		if( xPortStartScheduler() )
		{
			/* Should not reach here as if the scheduler is running the
			function will not return. */
		}
		else
		{
			/* Should only reach here if a task calls xTaskEndScheduler(). */
		}
	}

	/* This line will only be reached if the kernel could not be started. */
	configASSERT( xReturn );
}
/*-----------------------------------------------------------*/

void vTaskEndScheduler( void )
{
	/* Stop the scheduler interrupts and call the portable scheduler end
	routine so the original ISRs can be restored if necessary.  The port
	layer must ensure interrupts enable	bit is left in the correct state. */
	portDISABLE_INTERRUPTS();
	xSchedulerRunning = pdFALSE;
	vPortEndScheduler();
}
/*----------------------------------------------------------*/

void vTaskSuspendAll( void )
{

	/* A critical section is not required as the variable is of type
	portBASE_TYPE. */
	++uxSchedulerSuspended;
}
/*----------------------------------------------------------*/

signed portBASE_TYPE xTaskResumeAll( void )
{
register tskTCB *pxTCB;
signed portBASE_TYPE xAlreadyYielded = pdFALSE;

	/* If uxSchedulerSuspended is zero then this function does not match a
	previous call to vTaskSuspendAll(). */
	configASSERT( uxSchedulerSuspended );

	unsigned portBASE_TYPE processor = portGetCurrentCPU();

	/* It is possible that an ISR caused a task to be removed from an event
	list while the scheduler was suspended.  If this was the case then the
	removed task will have been added to the xPendingReadyList.  Once the
	scheduler has been resumed it is safe to move all the pending ready
	tasks from this list into their appropriate ready list. */
	taskENTER_CRITICAL(processor, TaskQueueMutex);
	{
		--uxSchedulerSuspended;

		if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
		{
			if( uxCurrentNumberOfTasks > ( unsigned portBASE_TYPE ) 0 )
			{
				portBASE_TYPE xYieldRequired = pdFALSE;

				/* Move any readied tasks from the pending list into the
				appropriate ready list. */
				while( listLIST_IS_EMPTY( ( xList * ) &xPendingReadyList ) == pdFALSE )
				{
					pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY(  ( ( xList * ) &xPendingReadyList ) );
					vListRemove( &( pxTCB->xEventListItem ) );
					vListRemove( &( pxTCB->xGenericListItem ) );
					prvAddTaskToReadyQueue( pxTCB );

					/* If we have moved a task that has a priority higher than
					the current task then we should yield. */

					if (pxTCB->uxPriority >= pxCurrentTCBs[processor]->uxPriority) {

						xYieldRequired = pdTRUE;
						break;

					}

				}

				/* If any ticks occurred while the scheduler was suspended then
				they should be processed now.  This ensures the tick count does not
				slip, and that any delayed tasks are resumed at the correct time. */
				if( uxMissedTicks > ( unsigned portBASE_TYPE ) 0 )
				{
					while( uxMissedTicks > ( unsigned portBASE_TYPE ) 0 )
					{
						vTaskIncrementTick();
						--uxMissedTicks;
					}

					/* As we have processed some ticks it is appropriate to yield
					to ensure the highest priority task that is ready to run is
					the task actually running. */
					#if configUSE_PREEMPTION == 1
					{
						xYieldRequired = pdTRUE;
					}
					#endif
				}

				if( ( xYieldRequired == pdTRUE ) || ( xMissedYield == pdTRUE ) )
				{
					xAlreadyYielded = pdTRUE;
					xMissedYield = pdFALSE;

					portYIELD_WITHIN_API();

				}
			}
		}
	}
	taskEXIT_CRITICAL(processor, TaskQueueMutex);

	return xAlreadyYielded;
}






/*-----------------------------------------------------------
 * PUBLIC TASK UTILITIES documented in task.h
 *----------------------------------------------------------*/



portTickType xTaskGetTickCount( void )
{
portTickType xTicks;
portBASE_TYPE currentCPU = portGetCurrentCPU();

	/* Critical section required if running on a 16 bit processor. */
	//taskENTER_CRITICAL(currentCPU, TickCountMutex);
	{
		xTicks = xTickCount;
	}
	//taskEXIT_CRITICAL(currentCPU, TickCountMutex);

	return xTicks;
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
portTickType xTaskGetTickCountFromISR( void )
{
portTickType xReturn;
unsigned portBASE_TYPE uxSavedInterruptStatus;

	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	xReturn = xTickCount;
	portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}
/*-----------------------------------------------------------*/

unsigned portBASE_TYPE uxTaskGetNumberOfTasks( void )
{
	/* A critical section is not required because the variables are of type
	portBASE_TYPE. */
	return uxCurrentNumberOfTasks;
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_TRACE_FACILITY == 1 )

	void vTaskList( signed char *pcWriteBuffer )
	{
	unsigned portBASE_TYPE uxQueue;

		/* This is a VERY costly function that should be used for debug only.
		It leaves interrupts disabled for a LONG time. */

		vTaskSuspendAll();
		{
			/* Run through all the lists that could potentially contain a TCB and
			report the task name, state and stack high water mark. */

			*pcWriteBuffer = ( signed char ) 0x00;
			strcat( ( char * ) pcWriteBuffer, ( const char * ) "\r\n" );

			uxQueue = uxTopUsedPriority + ( unsigned portBASE_TYPE ) 1U;

			do
			{
				uxQueue--;

				if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxQueue ] ) ) == pdFALSE )
				{
					prvListTaskWithinSingleList( pcWriteBuffer, ( xList * ) &( pxReadyTasksLists[ uxQueue ] ), tskREADY_CHAR );
				}
			}while( uxQueue > ( unsigned short ) tskIDLE_PRIORITY );

			if( listLIST_IS_EMPTY( pxDelayedTaskList ) == pdFALSE )
			{
				prvListTaskWithinSingleList( pcWriteBuffer, ( xList * ) pxDelayedTaskList, tskBLOCKED_CHAR );
			}

			if( listLIST_IS_EMPTY( pxOverflowDelayedTaskList ) == pdFALSE )
			{
				prvListTaskWithinSingleList( pcWriteBuffer, ( xList * ) pxOverflowDelayedTaskList, tskBLOCKED_CHAR );
			}

			#if( INCLUDE_vTaskDelete == 1 )
			{
				if( listLIST_IS_EMPTY( &xTasksWaitingTermination ) == pdFALSE )
				{
					prvListTaskWithinSingleList( pcWriteBuffer, ( xList * ) &xTasksWaitingTermination, tskDELETED_CHAR );
				}
			}
			#endif

			#if ( INCLUDE_vTaskSuspend == 1 )
			{
				if( listLIST_IS_EMPTY( &xSuspendedTaskList ) == pdFALSE )
				{
					prvListTaskWithinSingleList( pcWriteBuffer, ( xList * ) &xSuspendedTaskList, tskSUSPENDED_CHAR );
				}
			}
			#endif
		}
		xTaskResumeAll();
	}

#endif
/*----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configGENERATE_RUN_TIME_STATS == 1 )

	void vTaskGetRunTimeStats( signed char *pcWriteBuffer )
	{
	unsigned portBASE_TYPE uxQueue;
	unsigned long ulTotalRunTime;

		/* This is a VERY costly function that should be used for debug only.
		It leaves interrupts disabled for a LONG time. */

		vTaskSuspendAll();
		{
			#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
				portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime );
			#else
				ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
			#endif

			/* Divide ulTotalRunTime by 100 to make the percentage caluclations
			simpler in the prvGenerateRunTimeStatsForTasksInList() function. */
			ulTotalRunTime /= 100UL;
			
			/* Run through all the lists that could potentially contain a TCB,
			generating a table of run timer percentages in the provided
			buffer. */

			*pcWriteBuffer = ( signed char ) 0x00;
			strcat( ( char * ) pcWriteBuffer, ( const char * ) "\r\n" );

			uxQueue = uxTopUsedPriority + ( unsigned portBASE_TYPE ) 1U;

			do
			{
				uxQueue--;

				if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxQueue ] ) ) == pdFALSE )
				{
					prvGenerateRunTimeStatsForTasksInList( pcWriteBuffer, ( xList * ) &( pxReadyTasksLists[ uxQueue ] ), ulTotalRunTime );
				}
			}while( uxQueue > ( unsigned short ) tskIDLE_PRIORITY );

			if( listLIST_IS_EMPTY( pxDelayedTaskList ) == pdFALSE )
			{
				prvGenerateRunTimeStatsForTasksInList( pcWriteBuffer, ( xList * ) pxDelayedTaskList, ulTotalRunTime );
			}

			if( listLIST_IS_EMPTY( pxOverflowDelayedTaskList ) == pdFALSE )
			{
				prvGenerateRunTimeStatsForTasksInList( pcWriteBuffer, ( xList * ) pxOverflowDelayedTaskList, ulTotalRunTime );
			}

			#if ( INCLUDE_vTaskDelete == 1 )
			{
				if( listLIST_IS_EMPTY( &xTasksWaitingTermination ) == pdFALSE )
				{
					prvGenerateRunTimeStatsForTasksInList( pcWriteBuffer, ( xList * ) &xTasksWaitingTermination, ulTotalRunTime );
				}
			}
			#endif

			#if ( INCLUDE_vTaskSuspend == 1 )
			{
				if( listLIST_IS_EMPTY( &xSuspendedTaskList ) == pdFALSE )
				{
					prvGenerateRunTimeStatsForTasksInList( pcWriteBuffer, ( xList * ) &xSuspendedTaskList, ulTotalRunTime );
				}
			}
			#endif
		}
		xTaskResumeAll();
	}

#endif
/*----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_TRACE_FACILITY == 1 )

	void vTaskStartTrace( signed char * pcBuffer, unsigned long ulBufferSize )
	{
		configASSERT( pcBuffer );
		configASSERT( ulBufferSize );

		taskENTER_CRITICAL();
		{
			pcTraceBuffer = ( signed char * )pcBuffer;
			pcTraceBufferStart = pcBuffer;
			pcTraceBufferEnd = pcBuffer + ( ulBufferSize - tskSIZE_OF_EACH_TRACE_LINE );
			xTracing = pdTRUE;
		}
		taskEXIT_CRITICAL();
	}

#endif
/*----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_TRACE_FACILITY == 1 )

	unsigned long ulTaskEndTrace( void )
	{
	unsigned long ulBufferLength;

		taskENTER_CRITICAL();
			xTracing = pdFALSE;
		taskEXIT_CRITICAL();

		ulBufferLength = ( unsigned long ) ( pcTraceBuffer - pcTraceBufferStart );

		return ulBufferLength;
	}

#endif



/*-----------------------------------------------------------
 * SCHEDULER INTERNALS AVAILABLE FOR PORTING PURPOSES
 * documented in task.h
 *----------------------------------------------------------*/

	/* NOT FULLY MULTICORE TESTED */
void vTaskIncrementTick( void )
{
tskTCB * pxTCB;

	/* Called by the portable layer each time a tick interrupt occurs. ** Only ever executed by the master core. **
	Increments the tick then checks to see if the new tick value will cause any
	tasks to be unblocked. */
	if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
	{
		++xTickCount;
		if( xTickCount == ( portTickType ) 0 )
		{
			xList *pxTemp;

			/* Tick count has overflowed so we need to swap the delay lists.
			If there are any items in pxDelayedTaskList here then there is
			an error! */
			configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList ) ) );
			
			pxTemp = pxDelayedTaskList;
			pxDelayedTaskList = pxOverflowDelayedTaskList;
			pxOverflowDelayedTaskList = pxTemp;
			xNumOfOverflows++;
	
			if( listLIST_IS_EMPTY( pxDelayedTaskList ) != pdFALSE )
			{
				/* The new current delayed list is empty.  Set 
				xNextTaskUnblockTime to the maximum possible value so it is 
				extremely unlikely that the	
				if( xTickCount >= xNextTaskUnblockTime ) test will pass until 
				there is an item in the delayed list. */
				xNextTaskUnblockTime = portMAX_DELAY;
			}
			else
			{
				/* The new current delayed list is not empty, get the value of 
				the item at the head of the delayed list.  This is the time at 
				which the task at the head of the delayed list should be removed 
				from the Blocked state. */
				pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList );
				xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE( &( pxTCB->xGenericListItem ) );
			}
		}

		/* See if this tick has made a timeout expire. */
		prvCheckDelayedTasks();
	}
	else
	{
		++uxMissedTicks;

		/* The tick hook gets called at regular intervals, even if the
		scheduler is locked. */
		#if ( configUSE_TICK_HOOK == 1 )
		{
			vApplicationTickHook();
		}
		#endif
	}

	#if ( configUSE_TICK_HOOK == 1 )
	{
		/* Guard against the tick hook being called when the missed tick
		count is being unwound (when the scheduler is being unlocked. */
		if( uxMissedTicks == ( unsigned portBASE_TYPE ) 0U )
		{
			vApplicationTickHook();
		}
	}
	#endif

	traceTASK_INCREMENT_TICK( xTickCount );
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( ( INCLUDE_vTaskCleanUpResources == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

	void vTaskCleanUpResources( void )
	{
	unsigned short usQueue;
	volatile tskTCB *pxTCB;

		usQueue = ( unsigned short ) uxTopUsedPriority + ( unsigned short ) 1;

		/* Remove any TCB's from the ready queues. */
		do
		{
			usQueue--;

			while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ usQueue ] ) ) == pdFALSE )
			{
				listGET_OWNER_OF_NEXT_ENTRY( pxTCB, &( pxReadyTasksLists[ usQueue ] ) );
				vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );

				prvDeleteTCB( ( tskTCB * ) pxTCB );
			}
		}while( usQueue > ( unsigned short ) tskIDLE_PRIORITY );

		/* Remove any TCB's from the delayed queue. */
		while( listLIST_IS_EMPTY( &xDelayedTaskList1 ) == pdFALSE )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxTCB, &xDelayedTaskList1 );
			vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );

			prvDeleteTCB( ( tskTCB * ) pxTCB );
		}

		/* Remove any TCB's from the overflow delayed queue. */
		while( listLIST_IS_EMPTY( &xDelayedTaskList2 ) == pdFALSE )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxTCB, &xDelayedTaskList2 );
			vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );

			prvDeleteTCB( ( tskTCB * ) pxTCB );
		}

		while( listLIST_IS_EMPTY( &xSuspendedTaskList ) == pdFALSE )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxTCB, &xSuspendedTaskList );
			vListRemove( ( xListItem * ) &( pxTCB->xGenericListItem ) );

			prvDeleteTCB( ( tskTCB * ) pxTCB );
		}
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	void vTaskSetApplicationTaskTag( xTaskHandle xTask, pdTASK_HOOK_CODE pxHookFunction )
	{
	tskTCB *xTCB;

		/* xTask cannot be NULL */
		if( xTask != NULL )
		{
			xTCB = ( tskTCB * ) xTask;

			/* Save the hook function in the TCB.  A critical section is required as
			the value can be accessed from an interrupt. */
			taskENTER_CRITICAL();
				xTCB->pxTaskTag = pxHookFunction;
			taskEXIT_CRITICAL();

		}

	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	pdTASK_HOOK_CODE xTaskGetApplicationTaskTag( xTaskHandle xTask )
	{
	tskTCB *xTCB;
	pdTASK_HOOK_CODE xReturn = pdFAIL;

		/* xTask cannot be NULL */
		if( xTask != NULL )
		{
			xTCB = ( tskTCB * ) xTask;

			/* Save the hook function in the TCB.  A critical section is required as
			the value can be accessed from an interrupt. */
			taskENTER_CRITICAL();
				xReturn = xTCB->pxTaskTag;
			taskEXIT_CRITICAL();

		}

		return xReturn;
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	portBASE_TYPE xTaskCallApplicationTaskHook( xTaskHandle xTask, void *pvParameter )
	{
	tskTCB *xTCB;
	portBASE_TYPE xReturn = pdFAIL;

		/* If xTask is NULL then we are calling our own task hook. */
		if( xTask != NULL )
		{
			xTCB = ( tskTCB * ) xTask;

			if( xTCB->pxTaskTag != NULL )
			{
				xReturn = xTCB->pxTaskTag( pvParameter );
			}
			else
			{
				xReturn = pdFAIL;
			}

		}

		return xReturn;
	}

#endif
/*-----------------------------------------------------------*/

void vTaskSwitchContext()
{

	unsigned portBASE_TYPE processor = portGetCurrentCPU();
	tskTCB * pxTCB;

	if (processor < portNUM_PROCESSORS) {

		if( uxSchedulerSuspended != ( unsigned portBASE_TYPE ) pdFALSE )
		{
			/* The scheduler is currently suspended - do not allow a context
			switch. */
			xMissedYield = pdTRUE;
		}
		else
		{
			traceTASK_SWITCHED_OUT();

			#if ( configGENERATE_RUN_TIME_STATS == 1 )
			{
				unsigned long ulTempCounter;

					#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
						portALT_GET_RUN_TIME_COUNTER_VALUE( ulTempCounter );
					#else
						ulTempCounter = portGET_RUN_TIME_COUNTER_VALUE();
					#endif

					/* Add the amount of time the task has been running to the accumulated
					time so far.  The time the task started running was stored in
					ulTaskSwitchedInTime.  Note that there is no overflow protection here
					so count values are only valid until the timer overflows.  Generally
					this will be about 1 hour assuming a 1uS timer increment. */
					pxCurrentTCBs[processor]->ulRunTimeCounter += ( ulTempCounter - ulTaskSwitchedInTime );
					ulTaskSwitchedInTime = ulTempCounter;
			}
			#endif

			taskFIRST_CHECK_FOR_STACK_OVERFLOW(processor);
			taskSECOND_CHECK_FOR_STACK_OVERFLOW();

			taskENTER_CRITICAL(processor, TaskQueueMutex);

			unsigned portBASE_TYPE foundNonExecutingWaiter, ableToSchedule, resetListHead;

			foundNonExecutingWaiter = pdFALSE;
			ableToSchedule = pdFALSE;

			unsigned portBASE_TYPE uxDynamicTopReady = uxTopReadyPriority;

			/* Find the highest priority queue that contains a ready task not currently being executed */
			while( (ableToSchedule == pdFALSE) && (uxDynamicTopReady >= 0) )
			{
				configASSERT( uxTopReadyPriority );

				resetListHead = pdFALSE;

				// Nothing to do for empty lists
				if (!listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxDynamicTopReady ] ) )) {

					ableToSchedule = pdFALSE;
					tskTCB * pxRefTCB;

					/* Remember the current list item so that we
					can detect if all items have been inspected.
					Once this happens, we move on to a lower
					priority list (assuming nothing is suitable
					for scheduling). */
					pxRefTCB = pxReadyTasksLists[ uxDynamicTopReady ].pxIndex->pvOwner;

					do {

						listGET_OWNER_OF_NEXT_ENTRY( pxTCB, &( pxReadyTasksLists[ uxDynamicTopReady ] ) );

						/* Find out if the next task in the list is
						already being executed by another core */
						foundNonExecutingWaiter = pdTRUE;
						portBASE_TYPE i = 0;
						for (i=0;i<portNUM_PROCESSORS;i++) {

							if (i == processor) continue;
							else if (pxCurrentTCBs[i] == pxTCB) {

								foundNonExecutingWaiter = pdFALSE;
								break;

							}

						}

						if (foundNonExecutingWaiter == pdTRUE) {
							/* If the task is not being executed
							by another core and its affinity is
							compatible with the current one,
							prepare it to be swapped in */
							if (pxTCB->uxCPUAffinity == portNO_SPECIFIC_PROCESSOR) {
								pxCurrentTCBs[processor] = pxTCB;
								ableToSchedule = pdTRUE;
							} else if (pxTCB->uxCPUAffinity == processor) {
								pxCurrentTCBs[processor] = pxTCB;
								ableToSchedule = pdTRUE;
							} else ableToSchedule = pdFALSE;
						} else ableToSchedule = pdFALSE;

						if (ableToSchedule == pdFALSE) {
							resetListHead = pdTRUE;
						} else if ((ableToSchedule == pdTRUE) && (resetListHead == pdTRUE)) {
							tskTCB * pxResetTCB;
							do {
								listGET_OWNER_OF_NEXT_ENTRY( pxResetTCB, &( pxReadyTasksLists[ uxDynamicTopReady ] ) );
							} while(pxResetTCB != pxRefTCB);
						}

					} while ((ableToSchedule == pdFALSE) && (pxTCB != pxRefTCB));

				} else --uxTopReadyPriority;

				--uxDynamicTopReady;

			}

			taskEXIT_CRITICAL(processor, TaskQueueMutex);

			traceTASK_SWITCHED_IN();
			vWriteTraceToBuffer();
		}

	}
}

/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
void vTaskPlaceOnEventList( unsigned portBASE_TYPE processor, const xList * const pxEventList, portTickType xTicksToWait )
{
portTickType xTimeToWake;

	configASSERT( pxEventList );

	/* THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED OR THE
	SCHEDULER SUSPENDED. */

	/* Place the event list item of the TCB in the appropriate event list.
	This is placed in the list in priority order so the highest priority task
	is the first to be woken by the event. */
	vListInsert( ( xList * ) pxEventList, ( xListItem * ) &( pxCurrentTCBs[processor]->xEventListItem ) );

	/* We must remove ourselves from the ready list before adding ourselves
	to the blocked list as the same list item is used for both lists.  We have
	exclusive access to the ready lists as the scheduler is locked. */
	vListRemove( ( xListItem * ) &( pxCurrentTCBs[processor]->xGenericListItem ) );


	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		if( xTicksToWait == portMAX_DELAY )
		{
			/* Add ourselves to the suspended task list instead of a delayed task
			list to ensure we are not woken by a timing event.  We will block
			indefinitely. */
			vListInsertEnd( ( xList * ) &xSuspendedTaskList, ( xListItem * ) &( pxCurrentTCBs[processor]->xGenericListItem ) );
		}
		else
		{
			/* Calculate the time at which the task should be woken if the event does
			not occur.  This may overflow but this doesn't matter. */
			xTimeToWake = xTickCount + xTicksToWait;
			prvAddCurrentTaskToDelayedList( processor, xTimeToWake );
		}
	}
	#else
	{
			/* Calculate the time at which the task should be woken if the event does
			not occur.  This may overflow but this doesn't matter. */
			xTimeToWake = xTickCount + xTicksToWait;
			prvAddCurrentTaskToDelayedList( xTimeToWake );
	}
	#endif
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if configUSE_TIMERS == 1

	void vTaskPlaceOnEventListRestricted( unsigned portBASE_TYPE processor, const xList * const pxEventList, portTickType xTicksToWait )
	{
	portTickType xTimeToWake;

		configASSERT( pxEventList );

		/* This function should not be called by application code hence the
		'Restricted' in its name.  It is not part of the public API.  It is
		designed for use by kernel code, and has special calling requirements -
		it should be called from a critical section. */

	
		/* Place the event list item of the TCB in the appropriate event list.
		In this case it is assume that this is the only task that is going to
		be waiting on this event list, so the faster vListInsertEnd() function
		can be used in place of vListInsert. */
		vListInsertEnd( ( xList * ) pxEventList, ( xListItem * ) &( pxCurrentTCBs[processor]->xEventListItem ) );

		/* We must remove this task from the ready list before adding it to the
		blocked list as the same list item is used for both lists.  This
		function is called form a critical section. */
		vListRemove( ( xListItem * ) &( pxCurrentTCBs[processor]->xGenericListItem ) );

		/* Calculate the time at which the task should be woken if the event does
		not occur.  This may overflow but this doesn't matter. */
		xTimeToWake = xTickCount + xTicksToWait;
		prvAddCurrentTaskToDelayedList( xTimeToWake );
	}
	
#endif /* configUSE_TIMERS */
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
signed portBASE_TYPE xTaskRemoveFromEventList( unsigned portBASE_TYPE processor, const xList * const pxEventList )
{
tskTCB *pxUnblockedTCB;
portBASE_TYPE xReturn;

	/* THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED OR THE
	SCHEDULER SUSPENDED.  It can also be called from within an ISR. */

	/* The event list is sorted in priority order, so we can remove the
	first in the list, remove the TCB from the delayed list, and add
	it to the ready list.

	If an event is for a queue that is locked then this function will never
	get called - the lock count on the queue will get modified instead.  This
	means we can always expect exclusive access to the event list here.
	
	This function assumes that a check has already been made to ensure that
	pxEventList is not empty. */
	pxUnblockedTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
	configASSERT( pxUnblockedTCB );
	vListRemove( &( pxUnblockedTCB->xEventListItem ) );

	if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
	{
		vListRemove( &( pxUnblockedTCB->xGenericListItem ) );
		prvAddTaskToReadyQueue( pxUnblockedTCB );
	}
	else
	{
		/* We cannot access the delayed or ready lists, so will hold this
		task pending until the scheduler is resumed. */
		vListInsertEnd( ( xList * ) &( xPendingReadyList ), &( pxUnblockedTCB->xEventListItem ) );
	}

	if( pxUnblockedTCB->uxPriority >= pxCurrentTCBs[processor]->uxPriority )
	{
		/* Return true if the task removed from the event list has
		a higher priority than the calling task.  This allows
		the calling task to know if it should force a context
		switch now. */
		xReturn = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

void vTaskSetTimeOutState( xTimeOutType * const pxTimeOut )
{
	configASSERT( pxTimeOut );
	pxTimeOut->xOverflowCount = xNumOfOverflows;
	pxTimeOut->xTimeOnEntering = xTickCount;
}
/*-----------------------------------------------------------*/

portBASE_TYPE xTaskCheckForTimeOut( xTimeOutType * const pxTimeOut, portTickType * const pxTicksToWait )
{
portBASE_TYPE xReturn;

	configASSERT( pxTimeOut );
	configASSERT( pxTicksToWait );

	portBASE_TYPE interruptsRequireEnabling = xPortAreInterruptsEnabled();

	if (interruptsRequireEnabling) {
		portDISABLE_INTERRUPTS();
	}

	{
		#if ( INCLUDE_vTaskSuspend == 1 )
			/* If INCLUDE_vTaskSuspend is set to 1 and the block time specified is
			the maximum block time then the task should block indefinitely, and
			therefore never time out. */
			if( *pxTicksToWait == portMAX_DELAY )
			{
				xReturn = pdFALSE;
			}
			else /* We are not blocking indefinitely, perform the checks below. */
		#endif

		if( ( xNumOfOverflows != pxTimeOut->xOverflowCount ) && ( ( portTickType ) xTickCount >= ( portTickType ) pxTimeOut->xTimeOnEntering ) )
		{
			/* The tick count is greater than the time at which vTaskSetTimeout()
			was called, but has also overflowed since vTaskSetTimeOut() was called.
			It must have wrapped all the way around and gone past us again. This
			passed since vTaskSetTimeout() was called. */
			xReturn = pdTRUE;
		}
		else if( ( ( portTickType ) ( ( portTickType ) xTickCount - ( portTickType ) pxTimeOut->xTimeOnEntering ) ) < ( portTickType ) *pxTicksToWait )
		{
			/* Not a genuine timeout. Adjust parameters for time remaining. */
			*pxTicksToWait -= ( ( portTickType ) xTickCount - ( portTickType ) pxTimeOut->xTimeOnEntering );
			vTaskSetTimeOutState( pxTimeOut );
			xReturn = pdFALSE;
		}
		else
		{
			xReturn = pdTRUE;
		}
	}

	if (interruptsRequireEnabling) {
		portENABLE_INTERRUPTS();
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

unsigned portBASE_TYPE xGetTaskProcessor( void * task )
{
	unsigned portBASE_TYPE xReturn;

	if (task != NULL) {

		unsigned portBASE_TYPE i = 0;

		for (i=0;i<portNUM_PROCESSORS;i++) {

			if (pxCurrentTCBs[i] == task) {

				xReturn = i;
				break;

			}

		}

	}

	return xReturn;
}

/*-----------------------------------------------------------*/

void vCPUAcquireMutex( unsigned portBASE_TYPE processor, void * mutex[4] )
{

	vPortMemoryBarrier();

	portBASE_TYPE *pxCPUMutexFlags = mutex[0];
	portBASE_TYPE *pxCPUMutexTurns = mutex[1];
	portBASE_TYPE *pxCPUMutexNesting = mutex[2];
	portBASE_TYPE *pxCPUMutexEntryStage = mutex[3];

	if (*(pxCPUMutexNesting + processor) == 0) {

		unsigned portBASE_TYPE i, m, k;

		i = 0;
		do {

			i++;

			*(pxCPUMutexFlags + processor) = i;

			vPortMemoryBarrier();

			*(pxCPUMutexTurns + i) = processor;

			vPortMemoryBarrier();

			for (m=0;m<portNUM_PROCESSORS;m++) {

				if (m == processor) continue;
				else {

					portBASE_TYPE specific;

					do {

						specific = 0;
						for (k=0;k<portNUM_PROCESSORS;k++) {
							vPortMemoryBarrier();
							if (*(pxCPUMutexFlags + k) != portNO_SPECIFIC_PROCESSOR) {
								specific++;
							}
						}

						vPortMemoryBarrier();

					} while(!((*(pxCPUMutexTurns + i) != processor)
							|| (((*(pxCPUMutexFlags + m) < i) || (*(pxCPUMutexFlags + m) == portNO_SPECIFIC_PROCESSOR))
								&& (specific <= i))));

				}

			}

			vPortMemoryBarrier();

		} while (*(pxCPUMutexTurns + i) != processor);

		*(pxCPUMutexEntryStage + processor) = i;

		vPortMemoryBarrier();

	}

	*(pxCPUMutexNesting + processor) = *(pxCPUMutexNesting + processor) + 1;

	vPortMemoryBarrier();

}

/*-----------------------------------------------------------*/

void vCPUReleaseMutex( unsigned portBASE_TYPE processor, void * mutex[4] )
{

	portBASE_TYPE *pxCPUMutexFlags = mutex[0];
	portBASE_TYPE *pxCPUMutexTurns = mutex[1];
	portBASE_TYPE *pxCPUMutexNesting = mutex[2];
	portBASE_TYPE *pxCPUMutexEntryStage = mutex[3];
	portBASE_TYPE entryStage = *(pxCPUMutexEntryStage + processor);

	if (*(pxCPUMutexNesting + processor) > 0) {

		*(pxCPUMutexNesting + processor) = *(pxCPUMutexNesting + processor) - 1;

		if (*(pxCPUMutexNesting + processor) == 0) {

			vPortMemoryBarrier();

			portBASE_TYPE i = 0;

			for (i=1;i<entryStage;i++) {

				*(pxCPUMutexTurns + entryStage) = processor;
				vPortMemoryBarrier();

			}

			for (i=0;i<portNUM_PROCESSORS;i++) {

				vPortMemoryBarrier();

				if (i == processor) continue;
				else {

					while(!((*(pxCPUMutexFlags + i) == portNO_SPECIFIC_PROCESSOR) || (*(pxCPUMutexTurns + ((*(pxCPUMutexFlags + i)) == portNO_SPECIFIC_PROCESSOR ? 0 : (*(pxCPUMutexFlags + i))) ) == i))) { }

				}

			}

			*(pxCPUMutexFlags + processor) = portNO_SPECIFIC_PROCESSOR;

			vPortMemoryBarrier();

		}

	}

}

/*-----------------------------------------------------------*/

void vTaskAcquireNamedMutex( xTaskHandle currentTask, unsigned portBASE_TYPE mutexName )
{

	tskTCB * task = (tskTCB *) currentTask;

	// ToDo: Add error handling for when mutex name is invalid

	unsigned portBASE_TYPE i, m, k;

	vPortMemoryBarrier();

	if (pxTaskMutexFlags[task->uxUID][mutexName].mutexName == portNULL_MUTEX) {

		pxTaskMutexFlags[task->uxUID][mutexName].mutexName = mutexName;
		pxTaskMutexFlags[task->uxUID][mutexName].pendingEntry = 0;
		vPortMemoryBarrier();

	}

	i = 0;
	do {

		i++;
		pxTaskMutexFlags[task->uxUID][mutexName].pendingEntry = i;

		vPortMemoryBarrier();

		pxTaskMutexTurns[i][mutexName].turnTaskUID = task->uxUID;

		vPortMemoryBarrier();

		for (m=1;m<portMAX_TASKS;m++) {
			if (m == task->uxUID) continue;
			else {

				portBASE_TYPE nonZero;

				do {

					nonZero = 0;
					for (k=0;k<portMAX_TASKS;k++) {
						vPortMemoryBarrier();
						if (pxTaskMutexFlags[k][mutexName].pendingEntry != 0) {
							nonZero++;
						}
					}

					vPortMemoryBarrier();

				} while (!((pxTaskMutexTurns[i][mutexName].turnTaskUID != task->uxUID)
						|| ((pxTaskMutexFlags[m][mutexName].pendingEntry < i)
							&& (nonZero <= i))));

			}

		}

		vPortMemoryBarrier();

	} while(pxTaskMutexTurns[i][mutexName].turnTaskUID != task->uxUID);

	pxTaskMutexFlags[task->uxUID][mutexName].entryStage = i;

	vPortMemoryBarrier();

}

/*-----------------------------------------------------------*/

void vTaskReleaseNamedMutex( xTaskHandle currentTask, unsigned portBASE_TYPE mutexName )
{

	tskTCB * task = (tskTCB *) currentTask;

	vPortMemoryBarrier();

	portBASE_TYPE i = 1;

	for(i=1;i<pxTaskMutexFlags[task->uxUID][mutexName].entryStage;i++) {

		pxTaskMutexTurns[pxTaskMutexFlags[task->uxUID][mutexName].entryStage][mutexName].turnTaskUID = task->uxUID;
		vPortMemoryBarrier();

	}

	for (i=0;i<portMAX_TASKS;i++) {

		vPortMemoryBarrier();

		if (task->uxUID == i) continue;
		else {

			while (!((pxTaskMutexFlags[i][mutexName].pendingEntry == 0) || (pxTaskMutexTurns[pxTaskMutexFlags[i][mutexName].pendingEntry][mutexName].turnTaskUID == i))) { }

		}

	}

	pxTaskMutexFlags[task->uxUID][mutexName].pendingEntry = 0;

	vPortMemoryBarrier();

}
/*-----------------------------------------------------------*/

void vTaskMissedYield( void )
{
	xMissedYield = pdTRUE;
}

/*
 * -----------------------------------------------------------
 * The Idle task.
 * ----------------------------------------------------------
 *
 * The portTASK_FUNCTION() macro is used to allow port/compiler specific
 * language extensions.  The equivalent prototype for this function is:
 *
 * void prvIdleTask( void *pvParameters );
 *
 */
static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
	for( ;; )
	{

		/* See if any tasks have been deleted. */
		prvCheckTasksWaitingTermination();

		#if ( configUSE_PREEMPTION == 0 )
		{
			/* If we are not using preemption we keep forcing a task switch to
			see if any other task has become available.  If we are using
			preemption we don't need to do this as any task becoming available
			will automatically get the processor anyway. */
			taskYIELD();
		}
		#endif

		#if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
		{
			/* When using preemption tasks of equal priority will be
			timesliced.  If a task that is sharing the idle priority is ready
			to run then the idle task should yield before the end of the
			timeslice.

			A critical region is not required here as we are just reading from
			the list, and an occasional incorrect value will not matter.  If
			the ready list at the idle priority contains more than one task
			then a task other than the idle task is ready to execute. */

			if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ tskIDLE_PRIORITY ] ) ) > ( unsigned portBASE_TYPE ) 1 )
			{
				taskYIELD();
			}
		}
		#endif

		#if ( configUSE_IDLE_HOOK == 1 )
		{
			extern void vApplicationIdleHook( void );

			/* Call the user defined function from within the idle task.  This
			allows the application designer to add background functionality
			without the overhead of a separate task.
			NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
			CALL A FUNCTION THAT MIGHT BLOCK. */
			vApplicationIdleHook();
		}
		#endif
	}
} /*lint !e715 pvParameters is not accessed but all task functions require the same prototype. */







/*-----------------------------------------------------------
 * File private functions documented at the top of the file.
 *----------------------------------------------------------*/



static void prvInitialiseTCBVariables( tskTCB *pxTCB, const signed char * const pcName, unsigned portBASE_TYPE uxPriority, const xMemoryRegion * const xRegions, unsigned short usStackDepth )
{
	/* Store the function name in the TCB. */
	#if configMAX_TASK_NAME_LEN > 1
	{
		/* Don't bring strncpy into the build unnecessarily. */
		strncpy( ( char * ) pxTCB->pcTaskName, ( const char * ) pcName, ( unsigned short ) configMAX_TASK_NAME_LEN );
	}
	#endif
	pxTCB->pcTaskName[ ( unsigned short ) configMAX_TASK_NAME_LEN - ( unsigned short ) 1 ] = ( signed char ) '\0';

	/* This is used as an array index so must ensure it's not too large.  First
	remove the privilege bit if one is present. */
	if( uxPriority >= configMAX_PRIORITIES )
	{
		uxPriority = configMAX_PRIORITIES - ( unsigned portBASE_TYPE ) 1U;
	}

	pxTCB->uxPriority = uxPriority;
	#if ( configUSE_MUTEXES == 1 )
	{
		pxTCB->uxBasePriority = uxPriority;
	}
	#endif

	vListInitialiseItem( &( pxTCB->xGenericListItem ) );
	vListInitialiseItem( &( pxTCB->xEventListItem ) );

	/* Set the pxTCB as a link back from the xListItem.  This is so we can get
	back to	the containing TCB from a generic item in a list. */
	listSET_LIST_ITEM_OWNER( &( pxTCB->xGenericListItem ), pxTCB );

	/* Event lists are always in priority order. */
	listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) uxPriority );
	listSET_LIST_ITEM_OWNER( &( pxTCB->xEventListItem ), pxTCB );

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
	{
		pxTCB->uxCriticalNesting = ( unsigned portBASE_TYPE ) 0;
	}
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	{
		pxTCB->pxTaskTag = NULL;
	}
	#endif

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
	{
		pxTCB->ulRunTimeCounter = 0UL;
	}
	#endif

	#if ( portUSING_MPU_WRAPPERS == 1 )
	{
		vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, pxTCB->pxStack, usStackDepth );
	}
	#else
	{
		( void ) xRegions;
		( void ) usStackDepth;
	}
	#endif
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( portUSING_MPU_WRAPPERS == 1 )

	void vTaskAllocateMPURegions( xTaskHandle xTaskToModify, const xMemoryRegion * const xRegions )
	{
	tskTCB *pxTCB;
	
		if (xTaskToModify != NULL) {

			// Cannot be NULL here
			pxTCB = prvGetTCBFromHandle( xTaskToModify );

			vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, NULL, 0 );

		}

	}
	/*-----------------------------------------------------------*/
#endif

static void prvInitialiseTaskLists( void )
{
unsigned portBASE_TYPE uxPriority;

	for( uxPriority = ( unsigned portBASE_TYPE ) 0U; uxPriority < configMAX_PRIORITIES; uxPriority++ )
	{
		vListInitialise( ( xList * ) &( pxReadyTasksLists[ uxPriority ] ) );
	}

	vListInitialise( ( xList * ) &xDelayedTaskList1 );
	vListInitialise( ( xList * ) &xDelayedTaskList2 );
	vListInitialise( ( xList * ) &xPendingReadyList );

	#if ( INCLUDE_vTaskDelete == 1 )
	{
		vListInitialise( ( xList * ) &xTasksWaitingTermination );
	}
	#endif

	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		vListInitialise( ( xList * ) &xSuspendedTaskList );
	}
	#endif

	/* Start with pxDelayedTaskList using list1 and the pxOverflowDelayedTaskList
	using list2. */
	pxDelayedTaskList = &xDelayedTaskList1;
	pxOverflowDelayedTaskList = &xDelayedTaskList2;
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
static void prvCheckTasksWaitingTermination( void )
{
	#if ( INCLUDE_vTaskDelete == 1 )
	{
		portBASE_TYPE xListIsEmpty;
		portBASE_TYPE currentCPU = portGetCurrentCPU();

		/* ucTasksDeleted is used to prevent vTaskSuspendAll() being called
		too often in the idle task. */
		if( uxTasksDeleted > ( unsigned portBASE_TYPE ) 0 )
		{
			vTaskSuspendAll();
				xListIsEmpty = listLIST_IS_EMPTY( &xTasksWaitingTermination );
			xTaskResumeAll();

			if( xListIsEmpty == pdFALSE )
			{
				tskTCB *pxTCB;

				taskENTER_CRITICAL(currentCPU, TaskQueueMutex);
				{
					pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY( ( ( xList * ) &xTasksWaitingTermination ) );
					vListRemove( &( pxTCB->xGenericListItem ) );
					--uxCurrentNumberOfTasks;
					--uxTasksDeleted;
				}
				taskEXIT_CRITICAL(currentCPU, TaskQueueMutex);

				prvDeleteTCB( pxTCB );
			}
		}
	}
	#endif
}
/*-----------------------------------------------------------*/

static void prvAddCurrentTaskToDelayedList( unsigned portBASE_TYPE xProcessor, portTickType xTimeToWake )
{

	/* The list item will be inserted in wake time order. */
	listSET_LIST_ITEM_VALUE( &( pxCurrentTCBs[xProcessor]->xGenericListItem ), xTimeToWake );

	if( xTimeToWake < xTickCount )
	{
		/* Wake time has overflowed.  Place this item in the overflow list. */
		vListInsert( ( xList * ) pxOverflowDelayedTaskList, ( xListItem * ) &( pxCurrentTCBs[xProcessor]->xGenericListItem ) );
	}
	else
	{
		/* The wake time has not overflowed, so we can use the current block list. */
		vListInsert( ( xList * ) pxDelayedTaskList, ( xListItem * ) &( pxCurrentTCBs[xProcessor]->xGenericListItem ) );

		/* If the task entering the blocked state was placed at the head of the
		list of blocked tasks then xNextTaskUnblockTime needs to be updated
		too. */
		if( xTimeToWake < xNextTaskUnblockTime )
		{
			xNextTaskUnblockTime = xTimeToWake;
		}
	}
}
/*-----------------------------------------------------------*/

static tskTCB *prvAllocateTCBAndStack( unsigned short usStackDepth, portSTACK_TYPE *puxStackBuffer )
{
tskTCB *pxNewTCB;


	/* Allocate space for the TCB.  Where the memory comes from depends on
	the implementation of the port malloc function. */
	pxNewTCB = ( tskTCB * ) pvPortMalloc( sizeof( tskTCB ) );

	if( pxNewTCB != NULL )
	{
		/* Allocate space for the stack used by the task being created.
		The base of the stack memory stored in the TCB so the task can
		be deleted later if required. */
		pxNewTCB->pxStack = ( portSTACK_TYPE * ) pvPortMallocAligned( ( ( ( size_t )usStackDepth ) * sizeof( portSTACK_TYPE ) ), puxStackBuffer );

		if( pxNewTCB->pxStack == NULL )
		{
			/* Could not allocate the stack.  Delete the allocated TCB. */
			vPortFree( pxNewTCB );
			pxNewTCB = NULL;
		}
		else
		{
			/* Just to help debugging. */
			memset( pxNewTCB->pxStack, tskSTACK_FILL_BYTE, usStackDepth * sizeof( portSTACK_TYPE ) );
		}
	}

	return pxNewTCB;
}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_TRACE_FACILITY == 1 )

	static void prvListTaskWithinSingleList( const signed char *pcWriteBuffer, xList *pxList, signed char cStatus )
	{
	volatile tskTCB *pxNextTCB, *pxFirstTCB;
	unsigned short usStackRemaining;

		/* Write the details of all the TCB's in pxList into the buffer. */
		listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
		do
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
			#if ( portSTACK_GROWTH > 0 )
			{
				usStackRemaining = usTaskCheckFreeStackSpace( ( unsigned char * ) pxNextTCB->pxEndOfStack );
			}
			#else
			{
				usStackRemaining = usTaskCheckFreeStackSpace( ( unsigned char * ) pxNextTCB->pxStack );
			}
			#endif			
			
			sprintf( pcStatusString, ( char * ) "%s\t\t%c\t%u\t%u\t%u\r\n", pxNextTCB->pcTaskName, cStatus, ( unsigned int ) pxNextTCB->uxPriority, usStackRemaining, ( unsigned int ) pxNextTCB->uxTCBNumber );
			strcat( ( char * ) pcWriteBuffer, ( char * ) pcStatusString );

		} while( pxNextTCB != pxFirstTCB );
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configGENERATE_RUN_TIME_STATS == 1 )

	static void prvGenerateRunTimeStatsForTasksInList( const signed char *pcWriteBuffer, xList *pxList, unsigned long ulTotalRunTime )
	{
	volatile tskTCB *pxNextTCB, *pxFirstTCB;
	unsigned long ulStatsAsPercentage;

		/* Write the run time stats of all the TCB's in pxList into the buffer. */
		listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
		do
		{
			/* Get next TCB in from the list. */
			listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );

			/* Divide by zero check. */
			if( ulTotalRunTime > 0UL )
			{
				/* Has the task run at all? */
				if( pxNextTCB->ulRunTimeCounter == 0 )
				{
					/* The task has used no CPU time at all. */
					sprintf( pcStatsString, ( char * ) "%s\t\t0\t\t0%%\r\n", pxNextTCB->pcTaskName );
				}
				else
				{
					/* What percentage of the total run time has the task used?
					This will always be rounded down to the nearest integer.
					ulTotalRunTime has already been divided by 100. */
					ulStatsAsPercentage = pxNextTCB->ulRunTimeCounter / ulTotalRunTime;

					if( ulStatsAsPercentage > 0UL )
					{
						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcStatsString, ( char * ) "%s\t\t%lu\t\t%lu%%\r\n", pxNextTCB->pcTaskName, pxNextTCB->ulRunTimeCounter, ulStatsAsPercentage );							
						}
						#else
						{
							/* sizeof( int ) == sizeof( long ) so a smaller
							printf() library can be used. */
							sprintf( pcStatsString, ( char * ) "%s\t\t%u\t\t%u%%\r\n", pxNextTCB->pcTaskName, ( unsigned int ) pxNextTCB->ulRunTimeCounter, ( unsigned int ) ulStatsAsPercentage );
						}
						#endif
					}
					else
					{
						/* If the percentage is zero here then the task has
						consumed less than 1% of the total run time. */
						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcStatsString, ( char * ) "%s\t\t%lu\t\t<1%%\r\n", pxNextTCB->pcTaskName, pxNextTCB->ulRunTimeCounter );							
						}
						#else
						{
							/* sizeof( int ) == sizeof( long ) so a smaller
							printf() library can be used. */
							sprintf( pcStatsString, ( char * ) "%s\t\t%u\t\t<1%%\r\n", pxNextTCB->pcTaskName, ( unsigned int ) pxNextTCB->ulRunTimeCounter );
						}
						#endif
					}
				}

				strcat( ( char * ) pcWriteBuffer, ( char * ) pcStatsString );
			}

		} while( pxNextTCB != pxFirstTCB );
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static unsigned short usTaskCheckFreeStackSpace( const unsigned char * pucStackByte )
	{
	register unsigned short usCount = 0;

		while( *pucStackByte == tskSTACK_FILL_BYTE )
		{
			pucStackByte -= portSTACK_GROWTH;
			usCount++;
		}

		usCount /= sizeof( portSTACK_TYPE );

		return usCount;
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

	unsigned portBASE_TYPE uxTaskGetStackHighWaterMark( xTaskHandle xTask )
	{
	tskTCB *pxTCB;
	unsigned char *pcEndOfStack;
	unsigned portBASE_TYPE uxReturn;

		pxTCB = prvGetTCBFromHandle( xTask );

		#if portSTACK_GROWTH < 0
		{
			pcEndOfStack = ( unsigned char * ) pxTCB->pxStack;
		}
		#else
		{
			pcEndOfStack = ( unsigned char * ) pxTCB->pxEndOfStack;
		}
		#endif

		uxReturn = ( unsigned portBASE_TYPE ) usTaskCheckFreeStackSpace( pcEndOfStack );

		return uxReturn;
	}

#endif
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_vTaskDelete == 1 ) || ( INCLUDE_vTaskCleanUpResources == 1 ) )

	static void prvDeleteTCB( tskTCB *pxTCB )
	{
		/* Free up the memory allocated by the scheduler for the task.  It is up to
		the task to free any memory allocated at the application level. */
		vPortFreeAligned( pxTCB->pxStack );
		vPortFree( pxTCB );
	}

#endif


/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )

	xTaskHandle xTaskGetCurrentTaskHandle( unsigned portBASE_TYPE processor )
	{
	xTaskHandle xReturn;

		/* A critical section is not required as this is not called from
		an interrupt and the current TCB will always be the same for any
		individual execution thread. */
		xReturn = pxCurrentTCBs[processor];

		return xReturn;
	}

#endif

/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )

	portBASE_TYPE xTaskGetSchedulerState( void )
	{
	portBASE_TYPE xReturn;

		if( xSchedulerRunning == pdFALSE )
		{
			xReturn = taskSCHEDULER_NOT_STARTED;
		}
		else
		{
			if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
			{
				xReturn = taskSCHEDULER_RUNNING;
			}
			else
			{
				xReturn = taskSCHEDULER_SUSPENDED;
			}
		}

		return xReturn;
	}

#endif
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_MUTEXES == 1 )

	/*
	 * WARNING: This has not been tested on multicore FreeRTOS.
	 */

	void vTaskPriorityInherit( xTaskHandle * const pxMutexHolder )
	{
		tskTCB * const pxTCB = ( tskTCB * ) pxMutexHolder;
		unsigned portBASE_TYPE i = 0;

		configASSERT( pxMutexHolder );

		unsigned portBASE_TYPE inheritTarget = portNUM_PROCESSORS;
		unsigned portBASE_TYPE highestPriority = 0;
		for (i=0;i<portNUM_PROCESSORS;i++) {

			if( pxTCB->uxPriority < pxCurrentTCBs[i]->uxPriority ) {

				if (pxCurrentTCBs[i]->uxPriority > highestPriority) {

					highestPriority = pxCurrentTCBs[i]->uxPriority;
					inheritTarget = i;

				}

			}

		}

		if (inheritTarget < portNUM_PROCESSORS) {

			/* Adjust the mutex holder state to account for its new priority. */
			listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) pxCurrentTCBs[inheritTarget]->uxPriority );

			/* If the task being modified is in the ready state it will need to
			be moved in to a new list. */
			if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxTCB->uxPriority ] ), &( pxTCB->xGenericListItem ) ) )
			{
				vListRemove( &( pxTCB->xGenericListItem ) );

				/* Inherit the priority before being moved into the new list. */
				pxTCB->uxPriority = pxCurrentTCBs[inheritTarget]->uxPriority;
				prvAddTaskToReadyQueue( pxTCB );
			}
			else
			{
				/* Just inherit the priority. */
				pxTCB->uxPriority = pxCurrentTCBs[inheritTarget]->uxPriority;
			}

		}

	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( configUSE_MUTEXES == 1 )

	/*
	 * WARNING: This has not been tested on multicore FreeRTOS.
	 */

	void vTaskPriorityDisinherit( xTaskHandle * const pxMutexHolder )
	{
	tskTCB * const pxTCB = ( tskTCB * ) pxMutexHolder;

		if( pxMutexHolder != NULL )
		{
			if( pxTCB->uxPriority != pxTCB->uxBasePriority )
			{
				/* We must be the running task to be able to give the mutex back.
				Remove ourselves from the ready list we currently appear in. */
				vListRemove( &( pxTCB->xGenericListItem ) );

				/* Disinherit the priority before adding ourselves into the new
				ready list. */
				pxTCB->uxPriority = pxTCB->uxBasePriority;
				listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), configMAX_PRIORITIES - ( portTickType ) pxTCB->uxPriority );
				prvAddTaskToReadyQueue( pxTCB );
			}
		}
	}

#endif
/*-----------------------------------------------------------*/
void * pvGetMemoryMutexObject() {

	/* Return a pointer to the mutex global variables for memory allocation/
	deallocation locking */
	return (void *)MemMutex;

}
/*-----------------------------------------------------------*/
/* NOT FULLY MULTICORE TESTED */
#if ( portCRITICAL_NESTING_IN_TCB == 1 )

	void vTaskEnterCritical( unsigned portBASE_TYPE processor )
	{
		portDISABLE_INTERRUPTS();

		if( xSchedulerRunning != pdFALSE )
		{
			( pxCurrentTCBs[processor]->uxCriticalNesting )++;
		}
	}

#endif
/*-----------------------------------------------------------*/
	/* NOT FULLY MULTICORE TESTED */
#if ( portCRITICAL_NESTING_IN_TCB == 1 )

void vTaskExitCritical( unsigned portBASE_TYPE processor )
{
	if( xSchedulerRunning != pdFALSE )
	{
		if( pxCurrentTCBs[processor]->uxCriticalNesting > 0 )
		{
			( pxCurrentTCBs[processor]->uxCriticalNesting )--;

			if( pxCurrentTCBs[processor]->uxCriticalNesting == 0 )
			{
				portENABLE_INTERRUPTS();
			}
		}
	}
}

#endif
