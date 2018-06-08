#include "FreeRTOS.h"
#include "task.h"

#include "irq.h"


static volatile uint32_t * const PLIC_CLAIM_AND_RESPONSE_REGISTER = (uint32_t * const)0x40000004; 

static BaseType_t irq_empty_handler() { return pdFALSE; }

#define IRQ_TABLE_NUM_ENTRIES 64

static irq_handler_t irq_handler_table[IRQ_TABLE_NUM_ENTRIES] = { [ 0 ... IRQ_TABLE_NUM_ENTRIES-1 ] = irq_empty_handler };


BaseType_t xExternalInterruptHandler() {
	uint32_t irq_id = *PLIC_CLAIM_AND_RESPONSE_REGISTER;

	BaseType_t ans = irq_handler_table[irq_id]();

	*PLIC_CLAIM_AND_RESPONSE_REGISTER = 1;
	
	return ans;
}

void register_interrupt_handler(uint32_t irq_id, irq_handler_t fn) {
	configASSERT (irq_id < IRQ_TABLE_NUM_ENTRIES);
	irq_handler_table[irq_id] = fn;
}

