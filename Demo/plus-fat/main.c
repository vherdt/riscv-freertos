// based on https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/FreeRTOS_UDP_IP_Embedded_Ethernet_Tutorial.shtml
// and on https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/API/vApplicationIPNetworkEventHook.shtml

#include <stdio.h>
#include <time.h>

/* Kernel includes */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <timers.h>

/* FAT+Plus includes */
#include <ff_headers.h>
#include <ff_stdio.h>
#include <ff_ramdisk.h>

/* RISCV includes */
#include "arch/syscalls.h"
#include "arch/clib.h"
#include "arch/irq.h"


/* The number and size of sectors that will make up the MRAM disk. */
#define mainRAM_DISK_SECTOR_SIZE	512UL
#define mainRAM_DISK_SECTORS		( ( 0x0FFFFFFF ) / mainRAM_DISK_SECTOR_SIZE ) /* 5M bytes. */
#define mainIO_MANAGER_CACHE_SIZE	( 5UL * mainRAM_DISK_SECTOR_SIZE )

/* Where the RAM disk is mounted. */
#define mainRAM_DISK_NAME			"/mram"

static uint8_t* const MRAM_START_ADDR = (uint8_t* const)(0x60000000);

#define mainTASK_PRIORITY					( tskIDLE_PRIORITY + 2 )
#define	mainSTACK_SIZE						1400

void vLoggingPrintf( const char *pcFormat, ... );

static TaskHandle_t prvCreateDiskAndExampleFilesHandle = NULL;
static void prvCreateDiskAndExampleFiles( void *pvParameters );

time_t FreeRTOS_time( time_t* pxTime )
{
	(void) pxTime;
	return 4711;
}

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

int main( void )
{

	xTaskCreate( prvCreateDiskAndExampleFiles, "TEST", mainSTACK_SIZE, NULL, mainTASK_PRIORITY, &prvCreateDiskAndExampleFilesHandle );

    /* Start the RTOS scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
    line will never be reached.  If the following line does execute, then
    there was insufficient FreeRTOS heap memory available for the idle and/or
    timer tasks to be created. */
    exit(0);
}

void vLoggingPrintf( const char *pcFormat, ... )
{
	va_list args;
	va_start( args, pcFormat );
	printf(pcFormat, args);
}

static void prvCreateDiskAndExampleFiles( void *pvParameters )
{
	(void) pvParameters;
	FF_Disk_t *pxDisk = NULL;
	FF_RAMDiskInit( mainRAM_DISK_NAME, MRAM_START_ADDR, mainRAM_DISK_SECTORS, mainIO_MANAGER_CACHE_SIZE );
	/* Create the RAM disk. */
	configASSERT( pxDisk );

	/* Print out information on the disk. */
	FF_RAMDiskShowPartition( pxDisk );
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
