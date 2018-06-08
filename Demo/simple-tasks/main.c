/* Based on Task example from "Mastering the FreeRTOST Real Time Kernel: A Hands-On Tutorial Guide" */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Common demo includes. */
#include "blocktim.h"
#include "countsem.h"
#include "recmutex.h"

/* RISCV includes */
#include "arch/syscalls.h"
#include "arch/clib.h"


static const mainDELAY_LOOP_COUNT = 10000;


void vPrintString(const char *s) {
    printf(s);
}


void vTask1(void *pvParameters) {
    const char *pcTaskName = "Task 1 is running\r\n";
    volatile uint32_t ul;
    for (;;) {
	    vPrintString(pcTaskName);

	    for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++) {
	        // delay the printing
	    }
    }
    
    // a task should never return, so either use a non-terminating loop or call the *vTaskDelete* function
    vTaskDelete( NULL );
}

void vTask2(void *pvParameters) {
    const char *pcTaskName = "Task 2 is running\r\n";
    volatile uint32_t ul;
    for (;;) {
	    vPrintString(pcTaskName);

	    for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++) {
	        // delay the printing
	    }
    }
    
    vTaskDelete( NULL );
}


int main( void ) {
    xTaskCreate( vTask1, "Task 1", 1000, NULL, 1, NULL );
    
    xTaskCreate( vTask2, "Task 2", 1000, NULL, 1, NULL );

	vTaskStartScheduler();
	
	return 0;
}


void vApplicationMallocFailedHook( void )
{
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	// vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set to 1 in FreeRTOSConfig.h
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	taskDISABLE_INTERRUPTS();
	for( ;; );
}
