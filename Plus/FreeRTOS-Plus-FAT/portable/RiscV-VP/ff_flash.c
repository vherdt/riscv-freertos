/*
 * FreeRTOS+FAT Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include James Walmsley, Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+FAT IS STILL IN THE LAB:                                     ***
 ***                                                                         ***
 ***   This product is functional and is already being used in commercial    ***
 ***   products.  Be aware however that we are still refining its design,    ***
 ***   the source code does not yet fully conform to the strict coding and   ***
 ***   style standards mandated by Real Time Engineers ltd., and the         ***
 ***   documentation and testing is not necessarily complete.                ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+FAT can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+FAT is
 * executed, as follows:
 *
 * If FreeRTOS+FAT is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+FAT license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+FAT is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+FAT License Information Page: http://www.FreeRTOS.org/fat_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+FAT is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+FAT unless you agree that you use the software 'as is'.
 * FreeRTOS+FAT is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_flash.h"
#include "ff_sys.h"

#define flashHIDDEN_SECTOR_COUNT	8
#define flashPRIMARY_PARTITIONS		1
#define flashHUNDRED_64_BIT			100ULL
#define flashSECTOR_SIZE			512UL
#define flashPARTITION_NUMBER		0 /* Only a single partition is used. */
#define flashBYTES_PER_MB			( 1024ull * 1024ull )
#define flashSECTORS_PER_MB			( flashBYTES_PER_MB / 512ull )

/* Used as a magic number to indicate that an FF_Disk_t structure is a cool disk. */
#define flashSIGNATURE				0xBEEF0815

//From flash-controller
static const unsigned int BLOCKSIZE = 512;
static const unsigned int FLASH_ADDR_REG = 0;
static const unsigned int FLASH_SIZE_REG = sizeof(uint64_t);
static const unsigned int DATA_ADDR = FLASH_SIZE_REG + sizeof(uint64_t);
//static constexpr unsigned int ADDR_SPACE = DATA_ADDR + BLOCKSIZE;
static uint8_t* volatile const FLASH_CONTROLLER = (uint8_t* volatile const)(0x71000000);

/*-----------------------------------------------------------*/

static int32_t prvWriteFlash( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );

static int32_t prvReadFlash( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );

static void prvSetTargetBlock(uint64_t block);

static uint64_t prvGetSectorCount( void );

static void prvFlushFlash( void );

/*-----------------------------------------------------------*/

FF_Disk_t* FF_FlashDiskInit( void )
{
	FF_Error_t xError;
	FF_Disk_t *pxDisk = NULL;
	FF_CreationParameters_t xParameters;

	/* Attempt to allocated the FF_Disk_t structure. */
	pxDisk = ( FF_Disk_t * ) pvPortMalloc( sizeof( FF_Disk_t ) );

	if( pxDisk == NULL )
	{
		FF_PRINTF( "FF_MMAPDiskInit: Malloc failed\n" );
		return NULL;
	}

	/* Start with every member of the structure set to zero. */
	memset( pxDisk, '\0', sizeof( FF_Disk_t ) );

	/* The pvTag member of the FF_Disk_t structure allows the structure to be
	extended to also include media specific parameters. */
	pxDisk->pvTag = ( void * ) FLASH_CONTROLLER;

	/* The signature is used by the disk read and disk write functions to
	ensure the disk being accessed is a RAM disk. */
	pxDisk->ulSignature = flashSIGNATURE;

	/* The number of sectors is recorded for bounds checking in the read and
	write functions. */
	pxDisk->ulNumberOfSectors = prvGetSectorCount();

	/* Create the IO manager that will be used to control the RAM disk. */
	memset( &xParameters, '\0', sizeof( xParameters ) );
	xParameters.pucCacheMemory = NULL;
	xParameters.ulMemorySize = 2 * flashSECTOR_SIZE;
	xParameters.ulSectorSize = flashSECTOR_SIZE;
	xParameters.fnWriteBlocks = prvWriteFlash;
	xParameters.fnReadBlocks = prvReadFlash;
	xParameters.pxDisk = pxDisk;

	/* Driver is reentrant so xBlockDeviceIsReentrant can be set to pdTRUE.
	In this case the semaphore is only used to protect FAT data
	structures. */
	xParameters.pvSemaphore = ( void * ) xSemaphoreCreateRecursiveMutex();
	xParameters.xBlockDeviceIsReentrant = pdFALSE;

	pxDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xError );

	if( ( pxDisk->pxIOManager == NULL ) || ( FF_isERR( xError ) == pdTRUE ) )
	{
		FF_PRINTF( "FF_FLASHDiskInit: FF_CreateIOManger: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

		/* The disk structure was allocated, but the disk's IO manager could
		not be allocated, so free the disk again. */
		FF_FlashRelease( pxDisk );
		return NULL;
	}
	/* Record that the MM disk has been initialised. */
	pxDisk->xStatus.bIsInitialised = pdTRUE;
	return pxDisk;
}


/*-----------------------------------------------------------*/

FF_Error_t FF_FlashPartitionAndFormatDisk( FF_Disk_t *pxDisk, const char* label)
{
FF_PartitionParameters_t xPartition;
FF_Error_t xError;

	/* Create a single partition that fills all available space on the disk. */
	memset( &xPartition, '\0', sizeof( xPartition ) );
	xPartition.ulSectorCount = pxDisk->ulNumberOfSectors;
	xPartition.ulHiddenSectors = flashHIDDEN_SECTOR_COUNT;
	xPartition.xPrimaryCount = flashPRIMARY_PARTITIONS;
	xPartition.eSizeType = eSizeIsQuota;

	/* Partition the disk */
	xError = FF_Partition( pxDisk, &xPartition );
	FF_PRINTF( "FF_Partition: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

	if( FF_isERR( xError ) == pdFALSE )
	{
		/* Format the partition. */
		xError = FF_Format( pxDisk, flashPARTITION_NUMBER, pdTRUE, pdTRUE, label);
		FF_PRINTF( "FF_MMAPDiskInit: FF_Format: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
	}

	return xError;
}
/*-----------------------------------------------------------*/

BaseType_t FF_FlashRelease( FF_Disk_t *pxDisk )
{
	prvFlushFlash();

	if( pxDisk != NULL )
	{
		pxDisk->ulSignature = 0;
		pxDisk->xStatus.bIsInitialised = 0;
		if( pxDisk->pxIOManager != NULL )
		{
			FF_DeleteIOManager( pxDisk->pxIOManager );
		}

		vPortFree( pxDisk );
	}

	return pdPASS;
}
/*-----------------------------------------------------------*/

static int32_t prvReadFlash( uint8_t *pucDestination, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
int32_t lReturn;
uint8_t *pucSource;

	if( pxDisk != NULL )
	{
		if( pxDisk->ulSignature != flashSIGNATURE )
		{
			/* The disk structure is not valid because it doesn't contain a
			magic number written to the disk when it was created. */
			lReturn = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
		}
		else if( pxDisk->xStatus.bIsInitialised == pdFALSE )
		{
			/* The disk has not been initialised. */
			lReturn = FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG;
		}
		else if( ulSectorNumber >= pxDisk->ulNumberOfSectors )
		{
			/* The start sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else if( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) < ulSectorCount )
		{
			/* The end sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else
		{
			//Obtain Flash controller base address
			pucSource = ( uint8_t * ) pxDisk->pvTag;
			for(uint16_t block = ulSectorNumber; block < ulSectorNumber + ulSectorCount; block++)
			{
				prvSetTargetBlock(block);
				memcpy(pucDestination, pucSource + DATA_ADDR, BLOCKSIZE);
			}

			lReturn = FF_ERR_NONE;
		}
	}
	else
	{
		lReturn = FF_ERR_NULL_POINTER | FF_ERRFLAG;
	}

	return lReturn;
}
/*-----------------------------------------------------------*/

static int32_t prvWriteFlash( uint8_t *pucSource, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
int32_t lReturn = FF_ERR_NONE;
uint8_t *pucDestination;

	if( pxDisk != NULL )
	{
		if( pxDisk->ulSignature != flashSIGNATURE )
		{
			/* The disk structure is not valid because it doesn't contain a
			magic number written to the disk when it was created. */
			lReturn = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
		}
		else if( pxDisk->xStatus.bIsInitialised == pdFALSE )
		{
			/* The disk has not been initialised. */
			lReturn = FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG;
		}
		else if( ulSectorNumber >= pxDisk->ulNumberOfSectors )
		{
			/* The start sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else if( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) < ulSectorCount )
		{
			/* The end sector is not within the bounds of the disk. */
			lReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
		else
		{
			//Obtain Flash controller base address
			pucDestination = ( uint8_t * ) pxDisk->pvTag;
			for(uint16_t block = ulSectorNumber; block < ulSectorNumber + ulSectorCount; block++)
			{
				prvSetTargetBlock(block);
				memcpy(pucDestination + DATA_ADDR, pucSource, BLOCKSIZE);
			}

			lReturn = FF_ERR_NONE;
		}
	}
	else
	{
		lReturn = FF_ERR_NULL_POINTER | FF_ERRFLAG;
	}

	return lReturn;
}
/*-----------------------------------------------------------*/

static void prvSetTargetBlock(uint64_t block)
{
	memcpy(FLASH_CONTROLLER + FLASH_ADDR_REG, &block, sizeof(uint64_t));
}

static uint64_t prvGetSectorCount( void )
{
	uint64_t flashNumOfBlocks = 0;
	memcpy(&flashNumOfBlocks, FLASH_CONTROLLER + FLASH_SIZE_REG, sizeof(uint64_t));
	return flashNumOfBlocks;
}

static void prvFlushFlash( void )
{
	uint8_t dummy;
	prvSetTargetBlock(0);
	dummy = *(uint8_t*)(FLASH_CONTROLLER + DATA_ADDR);
	prvSetTargetBlock(1);
	dummy = *(uint8_t*)(FLASH_CONTROLLER + DATA_ADDR);
	(void) dummy;
}

BaseType_t FF_FlashDiskShowPartition( FF_Disk_t *pxDisk )
{
FF_Error_t xError;
uint64_t ullFreeSectors;
uint32_t ulTotalSizeMB, ulFreeSizeMB;
int iPercentageFree;
FF_IOManager_t *pxIOManager;
const char *pcTypeName = "unknown type";
BaseType_t xReturn = pdPASS;

	if( pxDisk == NULL )
	{
		xReturn = pdFAIL;
	}
	else
	{
		pxIOManager = pxDisk->pxIOManager;

		FF_PRINTF( "Reading FAT and calculating Free Space\n" );

		switch( pxIOManager->xPartition.ucType )
		{
			case FF_T_FAT12:
				pcTypeName = "FAT12";
				break;

			case FF_T_FAT16:
				pcTypeName = "FAT16";
				break;

			case FF_T_FAT32:
				pcTypeName = "FAT32";
				break;

			default:
				pcTypeName = "UNKOWN";
				break;
		}

		FF_GetFreeSize( pxIOManager, &xError );

		ullFreeSectors = pxIOManager->xPartition.ulFreeClusterCount * pxIOManager->xPartition.ulSectorsPerCluster;
		iPercentageFree = ( int ) ( ( flashHUNDRED_64_BIT * ullFreeSectors + pxIOManager->xPartition.ulDataSectors / 2 ) /
			( ( uint64_t )pxIOManager->xPartition.ulDataSectors ) );

		ulTotalSizeMB = pxIOManager->xPartition.ulDataSectors / flashSECTORS_PER_MB;
		ulFreeSizeMB = ( uint32_t ) ( ullFreeSectors / flashSECTORS_PER_MB );

		/* It is better not to use the 64-bit format such as %Lu because it
		might not be implemented. */
		printf( "Partition Nr   %8u\n", pxDisk->xStatus.bPartitionNumber );
		printf( "Type           %8u (%s)\n", pxIOManager->xPartition.ucType, pcTypeName );
		printf( "VolLabel       '%.8s' \n", pxIOManager->xPartition.pcVolumeLabel );
		printf( "TotalSectors   %8lu\n", pxIOManager->xPartition.ulTotalSectors );
		printf( "SecsPerCluster %8lu\n", pxIOManager->xPartition.ulSectorsPerCluster );
		printf( "Size           %8lu MB\n", ulTotalSizeMB );
		printf( "FreeSize       %8lu MB ( %d perc free )\n", ulFreeSizeMB, iPercentageFree );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/
