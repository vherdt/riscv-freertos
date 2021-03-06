/*
 * FreeRTOS+UDP V1.0.4
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* Standard includes. */
#include <stdint.h>
#include <limits.h>

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

/* Demo includes. */
#include "NetworkInterface.h"

/* If a packet cannot be sent immediately then the task performing the send
operation will be held in the Blocked state (so other tasks can execute) for
niTX_BUFFER_FREE_WAIT ticks.  It will do this a maximum of niMAX_TX_ATTEMPTS
before giving up. */
#define niTX_BUFFER_FREE_WAIT	( ( TickType_t ) 2UL / portTICK_RATE_MS )
#define niMAX_TX_ATTEMPTS		( 5 )

/*-----------------------------------------------------------*/

/*
 * A deferred interrupt handler task that processes received frames.
 */
static void prvGMACDeferredInterruptHandlerTask( void *pvParameters );

/*
 * Called by the ASF GMAC driver when a packet is received.
 */
static void prvGMACRxCallback( uint32_t ulStatus );

/*-----------------------------------------------------------*/

/* The queue used to communicate Ethernet events to the IP task. */
extern xQueueHandle xNetworkEventQueue;

/* The GMAC driver instance. */
static gmac_device_t xGMACStruct;

/* Handle of the task used to process MAC events. */
static TaskHandle_t xMACEventHandlingTask = NULL;

/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceInitialise( void )
{
gmac_options_t xGMACOptions;
extern uint8_t ucMACAddress[ 6 ];
const TickType_t xPHYDelay_400ms = 400UL;
BaseType_t xReturn = pdFALSE;

	/* Ensure PHY is ready. */
	vTaskDelay( xPHYDelay_400ms / portTICK_RATE_MS );

	/* Enable GMAC clock. */
	pmc_enable_periph_clk( ID_GMAC );

	/* Fill in GMAC options */
	xGMACOptions.uc_copy_all_frame = 0;
	xGMACOptions.uc_no_boardcast = 0;
	memcpy( xGMACOptions.uc_mac_addr, ucMACAddress, sizeof( ucMACAddress ) );

	xGMACStruct.p_hw = GMAC;

	/* Init GMAC driver structure. */
	gmac_dev_init( GMAC, &xGMACStruct, &xGMACOptions );

	/* Init MAC PHY driver. */
	if( ethernet_phy_init( GMAC, BOARD_GMAC_PHY_ADDR, sysclk_get_cpu_hz() ) == GMAC_OK )
	{
		/* Auto Negotiate, work in RMII mode. */
		if( ethernet_phy_auto_negotiate( GMAC, BOARD_GMAC_PHY_ADDR ) == GMAC_OK )
		{
			/* Establish Ethernet link. */
			vTaskDelay( xPHYDelay_400ms * 2UL );
			if( ethernet_phy_set_link( GMAC, BOARD_GMAC_PHY_ADDR, 1 ) == GMAC_OK )
			{
				/* Register the callbacks. */
				gmac_dev_set_rx_callback( &xGMACStruct, prvGMACRxCallback );

				/* The Rx deferred interrupt handler task is created at the
				highest	possible priority to ensure the interrupt handler can
				return directly to it no matter which task was running when the
				interrupt occurred. */
				xTaskCreate( 	prvGMACDeferredInterruptHandlerTask,/* The function that implements the task. */
								"MACTsk",
								configMINIMAL_STACK_SIZE,	/* Stack allocated to the task (defined in words, not bytes). */
								NULL, 						/* The task parameter is not used. */
								configMAX_PRIORITIES - 1, 	/* The priority assigned to the task. */
								&xMACEventHandlingTask );	/* The handle is stored so the ISR knows which task to notify. */

				/* Enable the interrupt and set its priority as configured.
				THIS DRIVER REQUIRES configMAC_INTERRUPT_PRIORITY TO BE DEFINED,
				PREFERABLY IN FreeRTOSConfig.h. */
				NVIC_SetPriority( GMAC_IRQn, configMAC_INTERRUPT_PRIORITY );
				NVIC_EnableIRQ( GMAC_IRQn );
				xReturn = pdPASS;
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static void prvGMACRxCallback( uint32_t ulStatus )
{
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	configASSERT( xMACEventHandlingTask );

	/* Unblock the deferred interrupt handler task if the event was an Rx. */
	if( ( ulStatus & GMAC_RSR_REC ) != 0 )
	{
		vTaskNotifyGiveFromISR( xMACEventHandlingTask, &xHigherPriorityTaskWoken );
	}

	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

BaseType_t xNetworkInterfaceOutput( xNetworkBufferDescriptor_t * const pxNetworkBuffer )
{
BaseType_t xReturn = pdFAIL;
int32_t x;

	/* Attempt to obtain access to a Tx descriptor. */
	for( x = 0; x < niMAX_TX_ATTEMPTS; x++ )
	{
		if( gmac_dev_write( &xGMACStruct, pxNetworkBuffer->pucEthernetBuffer, ( uint32_t ) pxNetworkBuffer->xDataLength, NULL ) == GMAC_OK )
		{
			/* The Tx has been initiated. */
			xReturn = pdPASS;
			break;
		}
		else
		{
			iptraceWAITING_FOR_TX_DMA_DESCRIPTOR();
			vTaskDelay( niTX_BUFFER_FREE_WAIT );
		}
	}

	/* Finished with the network buffer. */
	vNetworkBufferRelease( pxNetworkBuffer );

	return xReturn;
}
/*-----------------------------------------------------------*/

void GMAC_Handler( void )
{
	gmac_handler( &xGMACStruct );
}
/*-----------------------------------------------------------*/

static void prvGMACDeferredInterruptHandlerTask( void *pvParameters )
{
xNetworkBufferDescriptor_t *pxNetworkBuffer = NULL;
xIPStackEvent_t xRxEvent = { eEthernetRxEvent, NULL };
static const TickType_t xBufferWaitDelay = 1500UL / portTICK_RATE_MS;
uint32_t ulReturned;

	/* This is a very simply but also inefficient implementation. */

	( void ) pvParameters;

	for( ;; )
	{
		/* Wait for the GMAC interrupt to indicate that another packet has been
		received.  A while loop is used to process all received frames each time
		this task is notified, so it is ok to clear the notification count on the
		take (hence the first parameter is pdTRUE ). */
		ulTaskNotifyTake( pdTRUE, xBufferWaitDelay );

		ulReturned = GMAC_OK;
		while( ulReturned == GMAC_OK )
		{
			/* Allocate a buffer to hold the data if one is not already held. */
			if( pxNetworkBuffer == NULL )
			{
				pxNetworkBuffer = pxNetworkBufferGet( ipTOTAL_ETHERNET_FRAME_SIZE, xBufferWaitDelay );
			}

			if( pxNetworkBuffer != NULL )
			{
				/* Attempt to read data. */
				ulReturned = gmac_dev_read( &xGMACStruct, pxNetworkBuffer->pucEthernetBuffer, ipTOTAL_ETHERNET_FRAME_SIZE, ( uint32_t * ) &( pxNetworkBuffer->xDataLength ) );

				if( ulReturned == GMAC_OK )
				{
					#if ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 1
					{
						if( pxNetworkBuffer->xDataLength > 0 )
						{
							/* If the frame would not be processed by the IP
							stack then don't even bother sending it to the IP
							stack. */
							if( eConsiderFrameForProcessing( pxNetworkBuffer->pucEthernetBuffer ) != eProcessBuffer )
							{
								pxNetworkBuffer->xDataLength = 0;
							}
						}
					}
					#endif

					if( pxNetworkBuffer->xDataLength > 0 )
					{
						/* Store a pointer to the network buffer structure in
						the	padding	space that was left in front of the Ethernet
						frame.  The pointer is needed to ensure the network
						buffer structure can be located when it is time for it
						to be freed if the Ethernet frame gets used as a zero
						copy buffer. */
						*( ( xNetworkBufferDescriptor_t ** ) ( ( pxNetworkBuffer->pucEthernetBuffer - ipBUFFER_PADDING ) ) ) = pxNetworkBuffer;

						/* Data was received and stored.  Send it to the IP task
						for processing. */
						xRxEvent.pvData = ( void * ) pxNetworkBuffer;
						if( xQueueSendToBack( xNetworkEventQueue, &xRxEvent, ( TickType_t ) 0 ) == pdFALSE )
						{
							/* The buffer could not be sent to the IP task. The
							frame will be dropped and the buffer reused. */
							iptraceETHERNET_RX_EVENT_LOST();
						}
						else
						{
							iptraceNETWORK_INTERFACE_RECEIVE();

							/* The buffer is not owned by the IP task - a new
							buffer is needed the next time around. */
							pxNetworkBuffer = NULL;
						}
					}
					else
					{
						/* The buffer does not contain any data so there is no
						point sending it to the IP task.  Re-use the buffer on
						the next loop. */
						iptraceETHERNET_RX_EVENT_LOST();
					}
				}
				else
				{
					/* No data was received, keep the buffer for re-use.  The
					loop will exit as ulReturn is not GMAC_OK. */
				}
			}
			else
			{
				/* Left a frame in the driver as a buffer was not available.
				Break out of loop. */
				ulReturned = GMAC_INVALID;
			}
		}
	}
}
/*-----------------------------------------------------------*/

