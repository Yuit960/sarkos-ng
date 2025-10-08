/* GPLv2 (c) Airbus */
#include <debug.h>
#include "intr.h"

void bp_handler() {
	//QUESTION 2
   debug("Handling interruption\n");
   asm volatile("leave\niret");
}

void bp_trigger() {
	//QUESTION 4
	asm volatile("int3"::);
}

void tp() {
	// QUESTION 1
	idt_reg_t idtr_reg;
	get_idtr(idtr_reg);
	debug("IDTR address: %lx\n", idtr_reg.addr);

	//QUESTION 3
	idtr_reg.desc[3].offset_1 = ((uint32_t)&bp_handler) & 0xffff;
	idtr_reg.desc[3].offset_2 = ((uint32_t)&bp_handler >> 16) & 0xffff;

	bp_trigger();
}
