// based on the porting guide available from: https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_UDP/Embedded_Ethernet_Porting.shtml#simple_porting_embedded_ethernet

/* Standard includes. */
#include <stdint.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+UDP includes. */
#include "FreeRTOS_UDP_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_Sockets.h"
#include "NetworkBufferManagement.h"

/* Arch specific */
#include "irq.h"

/* Demo includes. */
#include "NetworkInterface.h"

#include <FreeRTOSIPConfig.h>

/* If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 1, then the Ethernet
driver will filter incoming packets and only pass the stack those packets it
considers need processing.  In this case ipCONSIDER_FRAME_FOR_PROCESSING() can
be #defined away.  If ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES is set to 0
then the Ethernet driver will pass all received packets to the stack, and the
stack must do the filtering itself.  In this case ipCONSIDER_FRAME_FOR_PROCESSING
needs to call eConsiderFrameForProcessing. */
#if ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES != 1
	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eProcessBuffer
#else
	#define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer ) eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
#endif

/*-----------------------------------------------------------*/
// VP/HW interface

size_t ReceiveSize( void ) {
    //printf("ReceiveSize()\n");
    return *ETHERNET_RECEIVE_SIZE_REG_ADDR;
}


void ReceiveData( uint8_t *buf ) {
    //printf("RecvData dst=%u\n", (uint32_t)buf);
    // assume that buf is large enough to hold ReceiveSize bytes
    *ETHERNET_RECEIVE_DST_REG_ADDR = (uint32_t)buf;
    *ETHERNET_STATUS_REG_ADDR = ETHERNET_OPERATION_RECV;
}


void SendData( uint8_t *buf, size_t num_bytes ) {
    //printf("SendData src=%u, size=%u\n", (uint32_t)buf, (uint32_t)num_bytes);
    *ETHERNET_SEND_SRC_REG_ADDR = (uint32_t)buf;
    *ETHERNET_SEND_SIZE_REG_ADDR = (uint32_t)num_bytes;
    *ETHERNET_STATUS_REG_ADDR = ETHERNET_OPERATION_SEND;
}

BaseType_t IsDataAvailable()
{
	return ReceiveSize() != 0;
}


/*-----------------------------------------------------------*/
// FreeRTOS interface

SemaphoreHandle_t xEMACRxEventSemaphore;


BaseType_t ethernet_mac_irq_handler( void ) {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xEMACRxEventSemaphore, &pxHigherPriorityTaskWoken);

	//portRETURN_FROM_ISR;
	portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}


/* The deferred interrupt handler is a standard RTOS task. */
static void prvEMACDeferredInterruptHandlerTask( void *pvParameters )
{
    xNetworkBufferDescriptor_t *pxNetworkBuffer;
    size_t xBytesReceived;
    extern QueueHandle_t xNetworkEventQueue;
    xIPStackEvent_t xRxEvent;
    (void) pvParameters;
    for( ;; )
    {
        //printf("NetworkInterface Wait\n");
        
        /* Wait for the Ethernet MAC interrupt to indicate that another packet
        has been received.  It is assumed xEMACRxEventSemaphore is a counting
        semaphore (to count the Rx events) and that the semaphore has already
        been created. */
        xSemaphoreTake( xEMACRxEventSemaphore, portMAX_DELAY );
        
        //printf("NetworkInterface Ready to Receive\n");

        /* See how much data was received.  Here it is assumed ReceiveSize() is
        a peripheral driver function that returns the number of bytes in the
        received Ethernet frame. */
        xBytesReceived = ReceiveSize();

        if( xBytesReceived > 0 )
        {
            /* Allocate a network buffer descriptor that references an Ethernet
            buffer large enough to hold the received data. */
            pxNetworkBuffer = pxNetworkBufferGet( xBytesReceived, 0 );

            if( pxNetworkBuffer != NULL )
            {
                /* pxNetworkBuffer->pucEthernetBuffer now points to an Ethernet
                buffer large enough to hold the received data.  Copy the
                received data into pcNetworkBuffer->pucEthernetBuffer.  Here it
                is assumed ReceiveData() is a peripheral driver function that
                copies the received data into a buffer passed in as the function's
                parameter. */
                ReceiveData( pxNetworkBuffer->pucEthernetBuffer );
                
                //printf("Receive Finished\n");

                /* See if the data contained in the received Ethernet frame needs
                to be processed. */
                if( eConsiderFrameForProcessing( pxNetworkBuffer->pucEthernetBuffer )
                                                                      == eProcessBuffer )
                {
                    //printf("Consider Frame For Processing\n");
                    
                    /* The event about to be sent to the IP stack is an Rx event. */
                    xRxEvent.eEventType = eEthernetRxEvent;

                    /* pvData is used to point to the network buffer descriptor that
                    references the received data. */
                    xRxEvent.pvData = ( void * ) pxNetworkBuffer;

                    /* Send the data to the IP stack. */
                    if( xQueueSendToBack( xNetworkEventQueue, &xRxEvent, 0 ) == pdFALSE )
                    {
                        //printf("Message Rejected\n");
                        
                        /* The buffer could not be sent to the IP task so the buffer
                        must be released. */
                        vNetworkBufferRelease( pxNetworkBuffer );

                        /* Make a call to the standard trace macro to log the
                        occurrence. */
                        iptraceETHERNET_RX_EVENT_LOST();
                    }
                    else
                    {
                        //printf("Message Accepted\n");
                        
                        /* The message was successfully sent to the IP stack.  Call
                        the standard trace macro to log the occurrence. */
                        iptraceNETWORK_INTERFACE_RECEIVE();
                    }
                }
                else
                {
                    //printf("Drop Frame\n");
                    
                    /* The Ethernet frame can be dropped, but the Ethernet buffer
                    must be released. */
                    vNetworkBufferRelease( pxNetworkBuffer );
                }
            }
            else
            {
                /* The event was lost because a network buffer was not available.
                Call the standard trace macro to log the occurrence. */
                iptraceETHERNET_RX_EVENT_LOST();
            }
        }
    }
}


BaseType_t xNetworkInterfaceOutput( xNetworkBufferDescriptor_t * const pxNetworkBuffer )
{
    /* Simple network interfaces (as opposed to more efficient zero copy network
    interfaces) just use Ethernet peripheral driver library functions to copy
    data from the FreeRTOS+UDP buffer into the peripheral driver's own buffer.

    This example assumes SendData() is a peripheral driver library function that
    takes a pointer to the start of the data to be sent and the length of the
    data to be sent as two separate parameters.  The start of the data is located
    by pxNetworkBuffer->pucEthernetBuffer.  The length of the data is located
    by pxNetworkBuffer->xDataLength. */
    SendData( pxNetworkBuffer->pucEthernetBuffer, pxNetworkBuffer->xDataLength );

    /* Call the standard trace macro to log the send event. */
    iptraceNETWORK_INTERFACE_TRANSMIT();

    /* It is assumed SendData() copies the data out of the FreeRTOS+UDP Ethernet
    buffer.  The Ethernet buffer is therefore no longer needed, and must
    be returned to the IP stack. */
    vNetworkBufferRelease( pxNetworkBuffer );

    return pdTRUE;
}


BaseType_t xNetworkInterfaceInitialise( void ) {
    xEMACRxEventSemaphore = xSemaphoreCreateCounting(16, 0);

    register_interrupt_handler(7, ethernet_mac_irq_handler);
    xTaskCreate( prvEMACDeferredInterruptHandlerTask, "prvEMACDeferredInterruptHandlerTask", 1000, NULL, 3, NULL );
    
    if(IsDataAvailable() && uxSemaphoreGetCount(xEMACRxEventSemaphore) == 0)
    {	//this is safe because we have actually just one element in Semaphore
    	xSemaphoreGive(xEMACRxEventSemaphore);
    }

    return pdPASS;
}
