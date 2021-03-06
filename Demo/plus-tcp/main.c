// based on https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/FreeRTOS_UDP_IP_Embedded_Ethernet_Tutorial.shtml
// and on https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/API/vApplicationIPNetworkEventHook.shtml

/* Kernel includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

/* TCP+Plus includes */
#include "FreeRTOSIPConfig.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_IP.h"

/* RISCV includes */
#include "arch/syscalls.h"
#include "arch/clib.h"
#include "arch/irq.h"


//TODO: use a random number here, e.g. obtain from VP through MMIO
UBaseType_t uxRand() {
	return 46274;
}


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

/* Called by the IP stack when the network either connects or disconnects. */
//void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent );


/*-----------------------------------------------------------*/
/*
 * Note: loopback interface seems not to be implemented in the FreeRTOS stack. You can either:
 *
 * 1) Filter packets during sending (in NetworkInterface.c) and manually inject them into the network queue (in case the IP address does match 127.0.0.1, and probably also modify the source and destination MAC addresses).
 *
 * 2) In the EthernetAdapter in the VP detect localhost packets and inject them back to the FreeRTOS application.
 */

/*-----------------------------------------------------------*/


static void vSendUsingStandardInterface( void *pvParameters )
{
Socket_t xSocket;
struct freertos_sockaddr xDestinationAddress;
uint8_t cString[ 50 ];
uint32_t ulCount = 0UL;
const TickType_t x1000ms = 1000UL / portTICK_PERIOD_MS;
long lBytes;

    /* Send strings to port 10000 on IP address 192.168.0.188. */
    xDestinationAddress.sin_addr = FreeRTOS_inet_addr( "192.168.0.90" );
    xDestinationAddress.sin_port = FreeRTOS_htons( 10000 );

    /* Create the socket. */
    xSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                               FREERTOS_SOCK_DGRAM,
                               FREERTOS_IPPROTO_UDP );

    /* Check the socket was created. */
    configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

    /* NOTE: FreeRTOS_bind() is not called.  This will only work if
    ipconfigALLOW_SOCKET_SEND_WITHOUT_BIND is set to 1 in FreeRTOSIPConfig.h. */

    for( ;; )
    {
        /* Create the string that is sent. */
        sprintf( cString,
                 "Standard send message number %lu\r\n",
                 ulCount );
                 
        printf("[freertos] send:\n");
        printf(cString);

        /* Send the string to the socket.  ulFlags is set to 0, so the standard
        semantics are used.  That means the data from cString[] is copied
        into a network buffer inside FreeRTOS_sendto(), and cString[] can be
        reused as soon as FreeRTOS_sendto() has returned. */
        lBytes = FreeRTOS_sendto( xSocket,
                                  cString,
                                  strlen( cString ),
                                  0,
                                  &xDestinationAddress,
                                  sizeof( xDestinationAddress ) );
                                  
        if( lBytes > 0 )
        {
            /* Data was received and can be process here. */
            printf("[freertos] successful send (bytes > 0)\n");
        } else {
            printf("[freertos] send returned zero bytes\n");
        }

        ulCount++;

        /* Wait until it is time to send again. */
        vTaskDelay( x1000ms );
    }
}


static void vReceivingUsingStandardInterface( void *pvParameters )
{
long lBytes;
uint8_t cReceivedString[ 60 ];
struct freertos_sockaddr xClient, xBindAddress;
uint32_t xClientLength = sizeof( xClient );
Socket_t xListeningSocket;

    /* Attempt to open the socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                        FREERTOS_SOCK_DGRAM,
                                        FREERTOS_IPPROTO_UDP );

    /* Check the socket was created. */
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* Bind to port 10000. */
    xBindAddress.sin_port = FreeRTOS_htons( 10001 );
    FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

    for( ;; )
    {
        printf("[freertos] begin-recv:\n");
        
        memset(cReceivedString, 0, 60);
    
        /* Receive data from the socket.  ulFlags is zero, so the standard
        interface is used.  By default the block time is portMAX_DELAY, but it
        can be changed using FreeRTOS_setsockopt(). */
        lBytes = FreeRTOS_recvfrom( xListeningSocket,
                                    cReceivedString,
                                    sizeof( cReceivedString ),
                                    0,
                                    &xClient,
                                    &xClientLength );
                                    
        printf("[freertos] end-recv:\n");

        if( lBytes > 0 )
        {
            /* Data was received and can be process here. */
            printf("[freertos] recv:\n");
            //printf(cReceivedString);
        }
    }
}


/* Define the network addressing.  These parameters will be used if either
ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
failed. */
static const uint8_t ucIPAddress[ 4 ] = { 192, 168, 0, 200 };
static const uint8_t ucNetMask[ 4 ] = { 255, 255, 255, 255 };
static const uint8_t ucGatewayAddress[ 4 ] = { 192, 168, 0, 1 };

/* The following is the address of an OpenDNS server. */
static const uint8_t ucDNSServerAddress[ 4 ] = { 208, 67, 222, 222 };

/* The MAC address array is not declared const as the MAC address will
normally be read from an EEPROM and not hard coded (in real deployed
applications).*/
//static uint8_t ucMACAddress[ 6 ] = { 0x54, 0xe1, 0xad, 0x74, 0x33, 0xfd };
static uint8_t ucMACAddress[ 6 ] = { 0x72, 0x5c, 0xb4, 0x2c, 0xac, 0x4a };


int main( void )
{	
    /* Initialise the embedded Ethernet interface.  The tasks that use the
    network are created in the vApplicationIPNetworkEventHook() hook function
    below.  The hook function is called when the network connects. */
    FreeRTOS_IPInit( ucIPAddress,
                     ucNetMask,
                     ucGatewayAddress,
                     ucDNSServerAddress,
                     ucMACAddress );

    /*
     * Other RTOS tasks can be created here.
     */

    /* Start the RTOS scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
    line will never be reached.  If the following line does execute, then
    there was insufficient FreeRTOS heap memory available for the idle and/or
    timer tasks to be created. */
    exit(0);
}


void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
    static BaseType_t xTasksAlreadyCreated = pdFALSE;
    int8_t cBuffer[ 16 ];

    /* Check this was a network up event, as opposed to a network down event. */
    if( eNetworkEvent == eNetworkUp )
    {
        /* The network is up and configured.  Print out the configuration,
        which may have been obtained from a DHCP server. */
        FreeRTOS_GetAddressConfiguration( &ulIPAddress,
                                          &ulNetMask,
                                          &ulGatewayAddress,
                                          &ulDNSServerAddress );
                                          
        printf("[freertos] network up\n");

        /* Convert the IP address to a string then print it out. */
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
        printf( "IP Address: %s\r\n", cBuffer );

        /* Convert the net mask to a string then print it out. */
        FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
        printf( "Subnet Mask: %s\r\n", cBuffer );

        /* Convert the IP address of the gateway to a string then print it out. */
        FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
        printf( "Gateway IP Address: %s\r\n", cBuffer );

        /* Convert the IP address of the DNS server to a string then print it out. */
        FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
        printf( "DNS server IP Address: %s\r\n", cBuffer );
        
        /* Create the tasks that use the IP stack if they have not already been
        created. */
        if( xTasksAlreadyCreated == pdFALSE )
        {
            /*
             * Create the tasks here.
             */
             //xTaskCreate( vReceivingUsingStandardInterface, "Task Receive", 1000, NULL, 1, NULL );
             //xTaskCreate( vSendUsingStandardInterface, "Task Send", 1000, NULL, 1, NULL );

            xTasksAlreadyCreated = pdTRUE;
        }
    } else if( eNetworkEvent == eNetworkDown ) {
        printf("[freertos] network down\n");
    }
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


/* Define a name that will be used for LLMNR and NBNS searches. */
#define mainHOST_NAME				"RTOSDemo"
#define mainDEVICE_NICK_NAME		"windows_demo"


#if( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) || ( ipconfigDHCP_REGISTER_HOSTNAME == 1 )

	const char *pcApplicationHostnameHook( void )
	{
		/* Assign the name "FreeRTOS" to this network node.  This function will
		be called during the DHCP: the machine will be registered with an IP
		address plus this name. */
		return mainHOST_NAME;
	}

#endif
/*-----------------------------------------------------------*/

#if( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

	BaseType_t xApplicationDNSQueryHook( const char *pcName )
	{
	BaseType_t xReturn;

		/* Determine if a name lookup is for this node.  Two names are given
		to this node: that returned by pcApplicationHostnameHook() and that set
		by mainDEVICE_NICK_NAME. */
		if( strcmp( pcName, pcApplicationHostnameHook() ) == 0 )
		{
			xReturn = pdPASS;
		}
		else if( strcmp( pcName, mainDEVICE_NICK_NAME ) == 0 )
		{
			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
		}

		return xReturn;
	}

#endif


#if( ipconfigUSE_TCP )
/*
 * Callback that provides the inputs necessary to generate a randomized TCP
 * Initial Sequence Number per RFC 6528.  THIS IS ONLY A DUMMY IMPLEMENTATION
 * THAT RETURNS A PSEUDO RANDOM NUMBER SO IS NOT INTENDED FOR USE IN PRODUCTION
 * SYSTEMS.
 */
extern uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
													uint16_t usSourcePort,
													uint32_t ulDestinationAddress,
													uint16_t usDestinationPort )
{
	( void ) ulSourceAddress;
	( void ) usSourcePort;
	( void ) ulDestinationAddress;
	( void ) usDestinationPort;

	return uxRand();
}
#endif
