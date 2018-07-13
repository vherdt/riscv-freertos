#ifndef __RISCV_IRQ_H__
#define __RISCV_IRQ_H__

#include "stdint.h"

typedef BaseType_t (*irq_handler_t)(void);

void register_interrupt_handler(uint32_t irq_id, irq_handler_t fn);

#define portYIELD_FROM_ISR( xHigherPriorityTaskWoken )      \
    if ( (xHigherPriorityTaskWoken) == pdTRUE ) {           \
        return pdTRUE;                                      \
    } else {                                                \
        return pdFALSE;                                     \
    }

#define portRETURN_FROM_ISR                                 \
    return pdFALSE;

#endif
