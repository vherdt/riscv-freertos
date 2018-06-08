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


static const unsigned mainDELAY_LOOP_COUNT = 10000;


void vPrintString(const char *s) {
    printf(s);
}


void vTask1(void *pvParameters) {
    volatile uint32_t ul;
    for (;;) {	    
	    printf("task 1 start ticks %d\n", xTaskGetTickCount());

	    for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++) {
	        // delay the printing
	    }
	    
	    printf("task 1 end ticks %d\n", xTaskGetTickCount());
    }
    
    // a task should never return, so either use a non-terminating loop or call the *vTaskDelete* function
    vTaskDelete( NULL );
}


void vTask2(void *pvParameters) {
    const TickType_t xDelay10ms = pdMS_TO_TICKS( 10 );
    printf("xDelay10ms %d\n", xDelay10ms);
    
    volatile uint32_t ul;
    for (;;) {
	    printf("task 2 start ticks %d\n", xTaskGetTickCount());
	    
	    for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++)
	        ;
	        
	    printf("task 2 end ticks %d\n", xTaskGetTickCount());

	    vTaskDelay( xDelay10ms );
    }
    
    vTaskDelete( NULL );
}


void vTask3( void *pvParameters ) {
    const TickType_t xDelay15ms = pdMS_TO_TICKS( 15 );
    TickType_t xLastWakeTime = xTaskGetTickCount();

    volatile uint32_t ul;
    for( ;; ) {
	    printf("task 3 start ticks %d\n", xTaskGetTickCount());
	    
	    for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++)
	        ;
	        
        printf("task 3 end ticks %d\n", xTaskGetTickCount());

        vTaskDelayUntil( &xLastWakeTime, xDelay15ms );
    }
}


int main( void ) {
    xTaskCreate( vTask1, "Task 1", 1000, NULL, 1, NULL );
    
    xTaskCreate( vTask2, "Task 2", 1000, NULL, 2, NULL );
    
    xTaskCreate( vTask3, "Task 3", 1000, NULL, 3, NULL );

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
