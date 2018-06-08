#include <stdlib.h>
#include <stdio.h>
#include "xuartlite_l.h"

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "xparameters.h"

#include "quad_sync_test.h"

/* Task function prototypes */
static portTASK_FUNCTION_PROTO( vSyncTestMonitor, pvParameters );
static portTASK_FUNCTION_PROTO( vSyncTest0, pvParameters );
static portTASK_FUNCTION_PROTO( vSyncTest1, pvParameters );
static portTASK_FUNCTION_PROTO( vSyncTest2, pvParameters );
static portTASK_FUNCTION_PROTO( vSyncTest3, pvParameters );

/* Mutex names used by the tasks */
static const signed portBASE_TYPE SYNC_TEST_MUTEX = 1;

/* Debug variables */
static volatile portBASE_TYPE interTaskMessage = 0;
static volatile portBASE_TYPE isSecondTaskAlive = 0;
static volatile portBASE_TYPE firstSpinCount = -50;
static volatile portBASE_TYPE secondSpinCount = -50;

/* Shared variables for the sync test tasks */
static volatile portBASE_TYPE usSynchroniserCheckin[4];
static volatile portBASE_TYPE usRunCount[4];
static volatile portBASE_TYPE sharedVar = 0;
static volatile portBASE_TYPE badSync = 0;

/* Constants for the sync test tasks */
static const portBASE_TYPE SYNC_RUNS = 25000;
static const portBASE_TYPE SYNCHRONISER_UNKNOWN = 0;
static const portBASE_TYPE SYNCHRONISER_ALIVE = 1;
static const portBASE_TYPE SYNCHRONISER_FINISHED = 2;
static const portBASE_TYPE CHECK_TASK_DELAY_PERIOD = 100;
static const portBASE_TYPE NO_CHECKIN_THRESHOLD = 5000;

void vStartQuadSyncTestTasks( unsigned portBASE_TYPE uxPriority ) {

	xTaskCreate( 0, vSyncTestMonitor, ( signed char * ) "T1", configMINIMAL_STACK_SIZE, NULL, uxPriority, NULL );
	xTaskCreate( 0, vSyncTest0, ( signed char * ) "T2", configMINIMAL_STACK_SIZE, (void *)&( usSynchroniserCheckin[ 0 ] ), uxPriority, NULL );
	xTaskCreate( 1, vSyncTest1, ( signed char * ) "T3", configMINIMAL_STACK_SIZE, (void *)&( usSynchroniserCheckin[ 1 ] ), uxPriority, NULL );
	xTaskCreate( 0, vSyncTest2, ( signed char * ) "T4", configMINIMAL_STACK_SIZE, (void *)&( usSynchroniserCheckin[ 2 ] ), uxPriority, NULL );
	xTaskCreate( 1, vSyncTest3, ( signed char * ) "T5", configMINIMAL_STACK_SIZE, (void *)&( usSynchroniserCheckin[ 3 ] ), uxPriority, NULL );

}

static portTASK_FUNCTION( vSyncTestMonitor, pvParameters )
{

	portBASE_TYPE noCheckinSince = 0;
	portBASE_TYPE currentRunCount[4] = {0, 0, 0, 0};
	portBASE_TYPE lastRunCount[4] = {0, 0, 0, 0};

	for (;;) {

		currentRunCount[0] = usRunCount[0];
		currentRunCount[1] = usRunCount[1];
		currentRunCount[2] = usRunCount[2];
		currentRunCount[3] = usRunCount[3];

		portBASE_TYPE i = 0;

		for (i=0;i<4;i++) {

			if (currentRunCount[i] > lastRunCount[i]) {

				if ((currentRunCount[i] - lastRunCount[i]) >= 100) {

					if (i == 0) printf("Task 1: Still alive (%li runs complete).\n", usRunCount[i]);
					else if (i == 1) printf("Task 2: Still alive (%li runs complete).\n", usRunCount[i]);
					else if (i == 2) printf("Task 3: Still alive (%li runs complete).\n", usRunCount[i]);
					else if (i == 3) printf("Task 4: Still alive (%li runs complete).\n", usRunCount[i]);

					lastRunCount[i] = currentRunCount[i];

				}

			}

		}

		if ((usSynchroniserCheckin[0] == SYNCHRONISER_UNKNOWN) || (usSynchroniserCheckin[1] == SYNCHRONISER_UNKNOWN)
				|| (usSynchroniserCheckin[2] == SYNCHRONISER_UNKNOWN) || (usSynchroniserCheckin[3] == SYNCHRONISER_UNKNOWN)) {

			noCheckinSince++;

		} else if ((usSynchroniserCheckin[0] == SYNCHRONISER_FINISHED) && (usSynchroniserCheckin[1] == SYNCHRONISER_FINISHED)
				&& (usSynchroniserCheckin[2] == SYNCHRONISER_FINISHED) && (usSynchroniserCheckin[3] == SYNCHRONISER_FINISHED)) {

			noCheckinSince = 0;
			if (badSync == 0) print("Tasks finished without deadlock or synchronisation failure.\n");
			else if (badSync == 1) print("!!! Tasks finished without deadlock, but there was a synchronisation failure !!!");
			for (;;) {}

		} else if ((((usSynchroniserCheckin[0] == SYNCHRONISER_ALIVE) || (usSynchroniserCheckin[0] == SYNCHRONISER_FINISHED))
				&& ((usSynchroniserCheckin[1] == SYNCHRONISER_ALIVE) || (usSynchroniserCheckin[0] == SYNCHRONISER_FINISHED))
				&& ((usSynchroniserCheckin[2] == SYNCHRONISER_ALIVE) || (usSynchroniserCheckin[0] == SYNCHRONISER_FINISHED))
				&& ((usSynchroniserCheckin[3] == SYNCHRONISER_ALIVE) || (usSynchroniserCheckin[0] == SYNCHRONISER_FINISHED)))) {

			noCheckinSince = 0;
			usSynchroniserCheckin[0] = SYNCHRONISER_UNKNOWN;
			usSynchroniserCheckin[1] = SYNCHRONISER_UNKNOWN;

		}

		if (noCheckinSince >= NO_CHECKIN_THRESHOLD) {

			print("!!! Possible deadlock detected !!!\n");

		}

		if (badSync != 0) {

			print("!!! Synchronisation failure detected !!!\n");

		}

		vTaskDelay(((systemTaskParameters *)(pvParameters))->taskHandle, CHECK_TASK_DELAY_PERIOD);

	}

}

static portTASK_FUNCTION( vSyncTest0, pvParameters )
{

	usRunCount[0] = 0; // Initialise the run count
	portBASE_TYPE * checkIn = (portBASE_TYPE *)(((systemTaskParameters *)(pvParameters))->applicationParameters);
	*checkIn = 0; // Initialise the check-in flag

	firstSpinCount = 0; // For debugging
	for (;;) {

		firstSpinCount++; // Helps with debugging

		if (usRunCount[0] == SYNC_RUNS) {

			for (;;) {
				*checkIn = SYNCHRONISER_FINISHED;
			}

		} else {

			*checkIn = SYNCHRONISER_ALIVE;

		}

		vTaskAcquireNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		// If the shared variable is not a multiple of 1000, synchronisation has failed somewhere
		if (sharedVar % 1000 != 0) badSync = 1;

		portBASE_TYPE i = 0;

		*checkIn = SYNCHRONISER_ALIVE;

		for (i=0;i<1000;i++) {
			sharedVar++;
		}

		vTaskReleaseNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		usRunCount[0]++;

	}

}

static portTASK_FUNCTION( vSyncTest1, pvParameters )
{

	usRunCount[1] = 0; // Initialise the run count
	portBASE_TYPE * checkIn = (portBASE_TYPE *)(((systemTaskParameters *)(pvParameters))->applicationParameters);
	*checkIn = 0; // Initialise the check-in flag

	secondSpinCount = 0; // For debugging
	for (;;) {

		secondSpinCount++; // Helps with debugging

		if (usRunCount[1] == SYNC_RUNS) {

			for (;;) {
				*checkIn = SYNCHRONISER_FINISHED;
			}

		} else {

			*checkIn = SYNCHRONISER_ALIVE;

		}

		vTaskAcquireNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		// If the shared variable is not a multiple of 1000, synchronisation has failed somewhere
		if (sharedVar % 1000 != 0) badSync = 1;

		portBASE_TYPE i = 0;
		for (i=0;i<1000;i++) {
			sharedVar++;
		}

		vTaskReleaseNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		usRunCount[1]++;

	}

}

static portTASK_FUNCTION( vSyncTest2, pvParameters )
{

	usRunCount[2] = 0; // Initialise the run count
	portBASE_TYPE * checkIn = (portBASE_TYPE *)(((systemTaskParameters *)(pvParameters))->applicationParameters);
	*checkIn = 0; // Initialise the check-in flag

	firstSpinCount = 0; // For debugging
	for (;;) {

		firstSpinCount++; // Helps with debugging

		if (usRunCount[2] == SYNC_RUNS) {

			for (;;) {
				*checkIn = SYNCHRONISER_FINISHED;
			}

		} else {

			*checkIn = SYNCHRONISER_ALIVE;

		}

		vTaskAcquireNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		// If the shared variable is not a multiple of 1000, synchronisation has failed somewhere
		if (sharedVar % 1000 != 0) badSync = 1;

		portBASE_TYPE i = 0;

		*checkIn = SYNCHRONISER_ALIVE;

		for (i=0;i<1000;i++) {
			sharedVar++;
		}

		vTaskReleaseNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		usRunCount[2]++;

	}

}

static portTASK_FUNCTION( vSyncTest3, pvParameters )
{

	usRunCount[3] = 0; // Initialise the run count
	portBASE_TYPE * checkIn = (portBASE_TYPE *)(((systemTaskParameters *)(pvParameters))->applicationParameters);
	*checkIn = 0; // Initialise the check-in flag

	firstSpinCount = 0; // For debugging
	for (;;) {

		firstSpinCount++; // Helps with debugging

		if (usRunCount[3] == SYNC_RUNS) {

			for (;;) {
				*checkIn = SYNCHRONISER_FINISHED;
			}

		} else {

			*checkIn = SYNCHRONISER_ALIVE;

		}

		vTaskAcquireNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		// If the shared variable is not a multiple of 1000, synchronisation has failed somewhere
		if (sharedVar % 1000 != 0) badSync = 1;

		portBASE_TYPE i = 0;

		*checkIn = SYNCHRONISER_ALIVE;

		for (i=0;i<1000;i++) {
			sharedVar++;
		}

		vTaskReleaseNamedMutex(((systemTaskParameters *)(pvParameters))->taskHandle, SYNC_TEST_MUTEX);

		usRunCount[3]++;

	}

}
