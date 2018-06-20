/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

/* RISCV includes */
#include "arch/syscalls.h"
#include "arch/clib.h"
#include "arch/irq.h"

/*
 * FreeRTOS hook for when malloc fails, enable in FreeRTOSConfig.
 */
void vApplicationMallocFailedHook( void );

/*
 * FreeRTOS hook for when freertos is idling, enable in FreeRTOSConfig.
 */
void vApplicationIdleHook( void );

/*
 * FreeRTOS hook for when a stackoverflow occurs, enable in FreeRTOSConfig.
 */
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

/*-----------------------------------------------------------*/


static volatile char * const TERMINAL_ADDR = (char * const)0x20000000;
static volatile char * const SENSOR_INPUT_ADDR = (char * const)0x50000000;
static volatile uint32_t * const SENSOR_SCALER_REG_ADDR = (uint32_t * const)0x50000080;
static volatile uint32_t * const SENSOR_FILTER_REG_ADDR = (uint32_t * const)0x50000084;


static const unsigned mainDELAY_LOOP_COUNT = 10000;

SemaphoreHandle_t sem;


BaseType_t sensor_irq_handler() {    
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    BaseType_t ans = xSemaphoreGiveFromISR(sem, &pxHigherPriorityTaskWoken);
    
    // Note: use *portRETURN_FROM_ISR* to return to the task that was interrupted, and use *portYIELD_FROM_ISR* to trigger a scheduler context switch (if necessary) in order to immediately return to the woken task
    
	//portRETURN_FROM_ISR;
	portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

void vTaskSomething(void *pvParameters) {
    volatile uint32_t ul;
    for (;;) {
        printf("Other Task Running @%d\n", xTaskGetTickCount());
    
        for (ul = 0; ul < mainDELAY_LOOP_COUNT; ul++)
	        ;
    }
}

void vTaskDumpSensorData(void *pvParameters) {
    const TickType_t xDelay100ms = pdMS_TO_TICKS( 100 );
    for (int k=0; k<5; ++k) {  //NOTE: terminating loop only for demonstration
        BaseType_t ans = xSemaphoreTake(sem, xDelay100ms);
        if (ans == pdFALSE) {
            printf("Dump Task sensor data not available @%d\n", xTaskGetTickCount());
        } else {
            printf("Dump Task Running @%d\n", xTaskGetTickCount());
        	for (int i=0; i<64; ++i) {
		        *TERMINAL_ADDR = *(SENSOR_INPUT_ADDR + i) % 92 + 32;
	        }
	        *TERMINAL_ADDR = '\n';
        }
    }
    exit(0);
}

int main() {
	register_interrupt_handler(2, sensor_irq_handler);
	
	*SENSOR_SCALER_REG_ADDR = 10;
	*SENSOR_FILTER_REG_ADDR = 2;
	
	sem = xSemaphoreCreateBinary();
	configASSERT( sem != NULL );
	
	xTaskCreate( vTaskSomething, "Task 1", 1000, NULL, 1, NULL );
	xTaskCreate( vTaskDumpSensorData, "Task 2", 1000, NULL, 2, NULL );

    vTaskStartScheduler();
		
	configASSERT( 0 );
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/
