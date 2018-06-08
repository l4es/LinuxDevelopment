#ifndef MULTICORE_H_
#define MULTICORE_H_

#include "portmacro.h"
#include "pvr.h"

#define MASTER_CPU_SYNC_MAGIC_CODE 	12034
#define SLAVE_CPU_SYNC_MAGIC_CODE 	56078

#define MAX_DEBUG_TEXT_LENGTH		40

typedef struct {

	volatile long interruptBridgeSwap[portNUM_PROCESSORS];
	volatile void * deviceHandlers[portNUM_PROCESSORS];
	volatile long initMagic;
	volatile void * interruptHandlerPtr;
	volatile void * startFirstTaskPtr;

} cpuComBlock;

unsigned portBASE_TYPE xPortGetCurrentCPU();

#endif /* MULTICORE_H_ */
