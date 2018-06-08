#include "defs.h"

.extern masterInterruptHandler

.global _interrupt_handler
.section .text
.align 4
.ent _interrupt_handler
.type _interrupt_handler, @function
_interrupt_handler:

	// First save r18 (the register to use for temporary data)
	swi		r18,	r0,		BRIDGE_SWAP_ADDR

	// Load the address of the master interrupt handler
	lwi		r18,	r0,		masterInterruptHandler

	// Branch to the master interrupt handler
	bra		r18

.end _interrupt_handler
