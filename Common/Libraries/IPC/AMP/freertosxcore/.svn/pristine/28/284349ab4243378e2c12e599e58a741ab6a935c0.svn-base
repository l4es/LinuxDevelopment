#ifndef MULTICORE_H_
#define MULTICORE_H_

#include "pvr.h"

#define MASTER_CPU_SYNC_MAGIC_CODE 	12034
#define SLAVE_CPU_SYNC_MAGIC_CODE 	56078

#define MAX_DEBUG_TEXT_LENGTH		40

#define NUM_PROCESSORS				2

typedef struct {

	volatile long interruptBridgeSwap[NUM_PROCESSORS];
	volatile void * deviceHandlers[NUM_PROCESSORS];
	volatile long initMagic;
	volatile void * interruptHandlerPtr;
	volatile void * startFirstTaskPtr;

} cpuComBlock;

#endif /* MULTICORE_H_ */
