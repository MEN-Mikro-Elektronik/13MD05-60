/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mipios_vx_api.c
 *
 *      \author  uf
 *        $Date: 2009/03/31 10:41:36 $
 *    $Revision: 1.2 $
 * 
 *       \brief  MIPIOS for VxWorks.
 *
 * This file contains the MIPIOS API for VxWorks.
 *
 *       \note  switches: INCLUDE_MIPIOS_VX if MIPIOS will be needed
 *						  MIPIOS_MAX_LINES			default 5
 *						  MIPIOS_MAX_DEVS_PER_LINE	default 10
 *						  MIPIOS_MAX_DEVS			default 50
 *						  MIPIOS_NUM_FILES			default 100
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_vx_api.c,v $
 * Revision 1.2  2009/03/31 10:41:36  ufranke
 * changed
 *  - switch from EXCLUDE_MIPIOS_VX to INCLUDE_MIPIOS_VX
 *
 * Revision 1.1  2009/01/30 09:21:13  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
const char *MIPIOS_VX_RCSid="$Id: mipios_vx_api.c,v 1.2 2009/03/31 10:41:36 ufranke Exp $\n"
"build " __DATE__ " " __TIME__ "\n";

/*! \page mdisapicommonspec Common MDIS_API specificiation "MDIS4/2003-0"

  blabla

  blubla, see the 
  \ref usrosscommonspec "USR_OSS library specification".
  
  \menimages
*/

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/

#include <MEN/men_typs.h>   /* MEN type definitions      */

#include <vxWorks.h>
#include <string.h>
#include <tickLib.h>
#include <sysLib.h>

#include <MEN/mdis_err.h>   /* MDIS error codes          */

#include <MEN/oss.h>
#include <MEN/dbg.h>
#include <MEN/desc.h>
#include <MEN/os2m.h>
#include <MEN/mipios.h>

#ifdef DBG
	const static char errStart[]	= "*** ";
	const static char warnStart[]	= "*** warning ";
	const static char errLine[]		= " (line ";    
	const static char errEnd[]		= ")\n";      
#endif

/* use the debug stuff from the OS2M module for mipios_vx_api.c */
#define DBG_MYLEVEL		    OS2M_DbgLev
#include <MEN/dbg.h>        /* debug module */
#ifdef DBG
	#define  DBH OS2M_dbgHdl
	extern DBG_HANDLE   *OS2M_dbgHdl;
#endif /*DBG*/

extern u_int32 OS2M_DbgLev;


#ifdef INCLUDE_MIPIOS_VX
/* ================================ if MIPIOS should be integrated into MDIS==================================== */
#warning "INFO: MIPIOS included"

#define _COMPILE_MIPIOS_INTERNAL_
#include "mipios_line_manager.h"
#include <MEN/mipios_vx_api.h>

/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct
{
	int32			targetPath;
	MIPIOS_DEVICE	*devHdl;
}MIPIOS_PATH;


/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/
static int systemInitialized_G = 0;
int32		   MIPIOS_RandomVal;			/* can be set externaly before MIPIOS_SystemInit() call */
MIPIOS_SYSTEM  MIPIOS_System;
MIPIOS_DEVICE  MIPIOS_DevTbl[MIPIOS_MAX_DEVS];
MIPIOS_PATH    MIPIOS_PathTbl[MIPIOS_NUM_FILES];


/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/


/**********************************************************************/
/** Displays the MIPIOS configuration and state.
 *
 * MIPIOS_Show() displays the MIPIOS configuration and state.  
 *
 * \param str	\IN verbose
 *
 * \return - 
 */
void MIPIOS_Show( int verbose )
{
	int i;
	int j;
	
	printf("MIPIOS: %s\n", MIPIOS_VX_RCSid );
	if( !systemInitialized_G )
	{
		printf("not initialized\n");
		goto CLEANUP;
	}

	/* system */
	printf( "SYSTEM:\n" );
	printf( "=======\n" );
	printf("  guardingMode:\t\t%d\n",			MIPIOS_System.guardingMode );
	printf("  newLinkNumber:\t%08x\n", 			(int)MIPIOS_System.newLinkNumber );
	printf("  MIPIOS_MAX_LINES:\t\t%d\n",		MIPIOS_MAX_LINES );
	printf("  MIPIOS_MAX_DEVS_PER_LINE:\t%d\n", MIPIOS_MAX_DEVS_PER_LINE );
	printf("  MIPIOS_MAX_DEVS:\t\t%d\n", 		MIPIOS_MAX_DEVS );
	printf("  MIPIOS_NUM_FILES:\t\t%d\n", 		MIPIOS_NUM_FILES );

	for( i=0; i<MIPIOS_MAX_LINES; i++ )
	{
		if( MIPIOS_LineTbl[i].lineName[0] )
		{
			printf( "\nLINE: %s", MIPIOS_LineTbl[i].lineName );
			if( verbose )
				printf( " lineHdl %08x\n", (int)&MIPIOS_LineTbl[i] );
			else
				printf("\n");
			printf(   "=====\n" );
			printf("\trx port       %d\n", (int)MIPIOS_DevTbl[i].mipiosLineP->hostRxPort );
			printf("\trx socket fd  %d\n", (int)MIPIOS_DevTbl[i].mipiosLineP->fhRxSocket );
			printf("\ttx socket fd  %d\n", (int)MIPIOS_DevTbl[i].mipiosLineP->fhTxSocket );
			printf("\ttimeout ticks %d\n", (int)MIPIOS_DevTbl[i].mipiosLineP->timeoutTicks );

			printf("\t   err socket  %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.socket );
			printf("\trx total       %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.rxTotal );
			printf("\t   err ignored %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.ignored );
			printf("\t   err unknown %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.unknown );
			printf("\t   err size    %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.size );
			printf("\t   err crc     %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.crc );
			printf("\t   err rx buff %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.rxBuff );
			printf("\t   err tmo     %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.tmo );
			printf("\ttx total       %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.txTotal );
			printf("\t   err tx buff %u\n", (int)MIPIOS_DevTbl[i].mipiosLineP->errCnt.txBuff );

			for( j=0; j<MIPIOS_MAX_DEVS; j++ )
			{
				if( MIPIOS_DevTbl[j].mipiosLineP == &MIPIOS_LineTbl[i] )
				{
					u_int8 *ipArr = (u_int8*)&MIPIOS_DevTbl[j].targetIp;
					
					printf("\t=> dev: %s (%d.%d.%d.%d/%d)\n", MIPIOS_DevTbl[j].os2mDevHdl.devName, _IP_ARR(ipArr), MIPIOS_DevTbl[j].targetReceivePort );
					printf("\t\tlinkNumber: %08x   frame# %8d\n", (int)MIPIOS_DevTbl[j].linkNumber, (int)MIPIOS_DevTbl[j].frameNumber );
					printf("\t\tllDesc:     %08x\n",	(int)MIPIOS_DevTbl[j].llDesc    );
					printf("\t\tdiscovered: %8d\n",		MIPIOS_DevTbl[j].discovered     );
					printf("\t\tlinkState : %8d\n",		(int)MIPIOS_DevTbl[j].linkState );
					printf("\t\tstate     : %8d\n",		MIPIOS_DevTbl[j].state      	);
					printf("\t\tnbrInstSig: %8d\n",		MIPIOS_DevTbl[j].installedSignalCount );
				}
			}
		}
	}

CLEANUP:
	return;
}

/**********************************************************************/
/** Display the MIPIOS path table.
 *
 * MIPIOS_ShowPath() displays the MIPIOS path table.
 *
 * \return -
 */
void MIPIOS_ShowPath( void )
{
	MIPIOS_DEVICE	*devHdl;
	int 			i;

	printf("\n=== MIPIOS Path Table ===\n" );
	if( !systemInitialized_G )
	{
		printf("not initialized\n");
		goto CLEANUP;
	}

	printf("  path    \ttargetPath\t       devName         \t   line/link\n");
	printf("----------\t----------\t-----------------------\t----------\n");

	/* look for dev in path table */
	for( i=0; i<MIPIOS_NUM_FILES ;i++ )
	{
		if( MIPIOS_PathTbl[i].devHdl != NULL )
		{
			devHdl = MIPIOS_PathTbl[i].devHdl;
			
			printf("%7d  \t%7d  \t%20s\t%8s/%08x\n", i+OS2M_MIPIOS_FILE_HANDLE_START, (int)MIPIOS_PathTbl[i].targetPath,
										 devHdl->name, devHdl->mipiosLineP->lineName, (int)devHdl->linkNumber );
		}
	}


CLEANUP:
	return;
}

/**********************************************************************/
/** Checks the parameter devName.
 *
 * CheckDevName() the parameter devName.
 *
 * \param devName	\IN pointer to array
 *
 * \return 0 or ERR_MIPIOS_ILL_PARAM 
 */
static int32 CheckDevName( char *devName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = 0;

	/* NULL check */
	if( devName == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: lineName missing %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	/* length check */
	error = MIPIOS_LM_StrMax( devName, MIPIOS_MAX_DEV_NAME );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_StrMax() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );
}



/**********************************************************************/
/** MIPIOS system initialization routine
 *
 * MIPIOS_SystemInit() initialize the MIPIOS system. This function must
 * be called first at MIPIOS startup.
 *
 * \param systemDesc	\IN 0 or pointer to MIPIOS system descriptor
 *
 * \return the device handle (>= 0)  on success or \c -1 on error. 
 */
int32 MIPIOS_SystemInit( DESC_SPEC *systemDesc )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error = 0;
	int		idx;
	int		i;

    DBGWRT_1((DBH, "%s %08x\n", functionName, systemDesc ));

	if( systemInitialized_G )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: already initialized %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	
	error = MIPIOS_LM_Init();
	if( error )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Init %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	/* cleanup all tables... */
	bzero( (char*)&MIPIOS_System, sizeof(MIPIOS_System) );
	bzero( (char*)MIPIOS_DevTbl,  sizeof(MIPIOS_DevTbl)  );

	bzero( (char*)MIPIOS_PathTbl, sizeof(MIPIOS_PathTbl)  );
	for( idx=0; idx<MIPIOS_NUM_FILES; idx++ )
	{
		MIPIOS_PathTbl[idx].targetPath	= -1;
	}
	
	/* setup with defaults */
	MIPIOS_System.guardingMode 	= MIPIOS_GUARDING_MODE_SYSTEM;

	i = ( MIPIOS_RandomVal + rand() + tickGet() ) & 0x2f;						/* randomization of newLinkNumber */
	for(; i < 0x31; i++ )
	{
		MIPIOS_RandomVal  = rand() 
							+ ((MIPIOS_RandomVal & 0xFF000000) >> 24) + ((MIPIOS_RandomVal & 0x00FF0000) >> 8)
							+ ((MIPIOS_RandomVal & 0x000000FF) << 24) + ((MIPIOS_RandomVal & 0x0000FF00) << 8);
	}
	MIPIOS_System.newLinkNumber	= tickGet()
									+ rand() 
									+ (int32)&MIPIOS_System 
									+ (int32)&idx 
									+ sysClkRateGet()
									+ MIPIOS_RandomVal;
	
	if( systemDesc == NULL )
	{
        DBGWRT_ERR(( DBH, "INFO: %s no desc - using defaults\n", functionName ));
        /* fix new link number */
	}
	else
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: DESC handling not implemented %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	
	systemInitialized_G = 1;

CLEANUP:
	return( error );
}

/**********************************************************************/
/** MIPIOS system start configuration routine
 *
 * MIPIOS_SystemStartConfiguration() starts a line or the complete system to configuration
 * state. If lineName is 0 all lines will be switched.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_SystemStartConfiguration( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error;
	
	error = MIPIOS_LM_StartConfiguration( lineName );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_StartConfiguration( %s ) %s%d%s", errStart, functionName, errLine, 
        			lineName ? lineName : "0 - whole system", __LINE__, errEnd ));
	}
	
	return( error );
}

/**********************************************************************/
/** MIPIOS system start operational routine
 *
 * MIPIOS_SystemStartOperational() starts a line or the complete system to operational
 * state. If lineName is 0 all lines will be switched.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_SystemStartOperational( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error;
	
	error = MIPIOS_LM_StartOperational( lineName );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_StartOperational( %s ) %s%d%s", errStart, functionName, 
        			 lineName ? lineName : "0 - whole system",
        			 errLine, __LINE__, errEnd ));
	}
	
	return( error );
}

/**********************************************************************/
/** MIPIOS system stop routine
 *
 * MIPIOS_SystemStop() stops a line or the complete system to stop
 * state. If lineName is 0 all lines will be switched to stop.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_SystemStop( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error;
	
	error = MIPIOS_LM_Stop( lineName );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Stop( %s ) %s%d%s", errStart, functionName, errLine, 
        			lineName ? lineName : "0 - whole system", __LINE__, errEnd ));
	}
	
	return( error );
}

/**********************************************************************/
/** Get the MIPIOS device by name
 *
 * GetDev() gets the device.  
 *
 * \param devName	\IN device name
 * \param devHdlP	\IN pointer to variable where devHdl will be stored
 *
 * \return 0 | error code
 */
static int GetDev( char *devName, MIPIOS_DEVICE **devHdlP )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS_NOT_INITIALIZED;
	int			i;
	int			found;

	if( devName == NULL || devHdlP == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	*devHdlP = NULL;

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* look for dev */
	for( i=0; i<MIPIOS_MAX_DEVS ;i++ )
	{
		if( MIPIOS_DevTbl[i].os2mDevHdl.devName != NULL )
		{
			found = !strncmp( MIPIOS_DevTbl[i].os2mDevHdl.devName, devName, MIPIOS_MAX_DEV_NAME );
			if( found )
			{
				*devHdlP = &MIPIOS_DevTbl[i];
				error = 0;
				break;
			}
		}
	}

	/*--- unlock critical section ---*/
	if( taskUnlock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}


CLEANUP:
	return( error );
}

static int32 PutDevIntoLine( MIPIOS_LINE *lineHdl, MIPIOS_DEVICE *devHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS_NO_SPACE;
	int devIdx;

	for( devIdx=0; devIdx<MIPIOS_MAX_DEVS_PER_LINE; devIdx++ )
	{
		if( lineHdl->devTbl[devIdx] == NULL )
		{
			lineHdl->devTbl[devIdx] = devHdl;
			error = 0;
			break;
		}
	}


/*CLEANUP:*/
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: no space %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}
	return( error );
}

	
/**********************************************************************/
/** Allocate space for a MIPIOS device
 *
 * AllocDev() get space for a MIPIOS device. MIPIOS_System.newLinkNumber
 * will be increased. 
 *
 * \param devName	\IN device name
 * \param devHdlP	\IN pointer to variable where devHdl will be stored
 *
 * \return 0 | error code
 */
static int32 AllocDev( char *devName, MIPIOS_DEVICE **devHdlP, MIPIOS_LINE 	*lineHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS_NO_SPACE;
	int			i;
	MIPIOS_DEVICE *devHdl = NULL;

	if( devName == NULL || devHdlP == NULL || lineHdl == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	*devHdlP = NULL;

	/* check if device already initialized */
	error = GetDev( devName, &devHdl );
	if( devHdl != NULL )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: dev %s already initialized %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* look for a free space */
	for( i=0; i<MIPIOS_MAX_DEVS ;i++ )
	{
		if( MIPIOS_DevTbl[i].name[0] == 0 )
		{
			/* clean the turkey */
			bzero( (char*)&MIPIOS_DevTbl[i], sizeof(MIPIOS_DevTbl[i]) );

			/* fill the turkey */
			strcpy( MIPIOS_DevTbl[i].name, devName );                 /* mark it allocated */
			MIPIOS_DevTbl[i].mipiosLineP = lineHdl;					  /* attach corresponding line */
			MIPIOS_DevTbl[i].timeoutMs_guarding 	= MIPIOS_GUARDING_TIMEOUT_DEFAULT_MS;
			MIPIOS_DevTbl[i].timeoutMs_signalPoll 	= MIPIOS_SIGNAL_POLL_TIMEOUT_DEFAULT_MS;

			/* OS2M part of the turkey */
			MIPIOS_DevTbl[i].os2mDevHdl.devName = MIPIOS_DevTbl[i].name;  /* OS2M devName points to name */
			MIPIOS_DevTbl[i].os2mDevHdl.mipiosFlag = 1;					  /* say to OS2M it's a MIPIOS device */

			error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1, &MIPIOS_DevTbl[i].devSem );
			if( error )
			{
        		DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        		goto CLEANUP;
			}		
			
			MIPIOS_DevTbl[i].linkNumber = MIPIOS_System.newLinkNumber++;
			DBGWRT_1((DBH, "%s %s new link number is %08x\n", functionName, MIPIOS_DevTbl[i].name, MIPIOS_DevTbl[i].linkNumber ));

			error = PutDevIntoLine( lineHdl, &MIPIOS_DevTbl[i] ); 
			if( error )
			{
        		DBGWRT_ERR(( DBH, "%s%s: PutDevIntoLine() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        		goto CLEANUP;
			}		

			*devHdlP = &MIPIOS_DevTbl[i];
			error = 0;
			break;
		}
	}

CLEANUP:
	/*--- unlock critical section ---*/
	if( taskUnlock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

	return( error );
}

/**********************************************************************/
/** Increase  link number.
 *
 * MIPIOS_DevIncreaseLinkNumber() .  
 *
 * \param devName	\IN device name
 *
 * \return 0 | error code
 */
int32 MIPIOS_DevIncreaseLinkNumber( MIPIOS_DEVICE *devHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = 0;

	/* parameter check */
	if( devHdl == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* increase link number */
	devHdl->linkNumber = MIPIOS_System.newLinkNumber++;

	DBGWRT_1((DBH, "%s %s new link number is %08x\n", functionName, devHdl->name, devHdl->linkNumber ));


	/*--- unlock critical section ---*/
	if( taskUnlock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}


CLEANUP:
	return( error );
	
}


/**********************************************************************/
/** Creates a MIPIOS device or bind a low level descriptor on it
 *
 * MIPIOS_DevCreate() creates a MIPIOS device and/or attach it to MDIS
 * if <discovered>.
 * Binds a low level descriptor on a MIPIOS device if !<discovered>.  
 *
 * \param devName		\IN name of the device e.g. "/m66/0" 
 * \param lineName		\IN name of the MIPIOS line e.g. "line_A" 
 * \param lowlevelDesc	\IN low level descriptor
 * \param discovered	\IN 0 | 1
 *
 * \return 0 | error code
 */
static int32 MIPIOS_DevCreateCore( char *devName, char *lineName, DESC_SPEC *lowlevelDesc, int discovered )
{
  	static const char *functionName = __FUNCTION__;
	int32 			error = ERR_MIPIOS;
	MIPIOS_LINE 	*lineHdl = NULL;
	MIPIOS_DEVICE	*devHdl  = NULL;

    DBGWRT_1((DBH, "%s dev/line %s/%s desc at %08x just discovered %d\n", functionName, devName, lineName, lowlevelDesc, discovered ));

	/* check if system is initialized */
	if( !systemInitialized_G )
	{
		error = ERR_MIPIOS_NOT_INITIALIZED;
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_SystemInit() not called %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	
	/* check line name */
	error = MIPIOS_LM_CheckLineName( lineName );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_CheckLineName %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* get line */
	error = MIPIOS_LM_GetLine( lineName, &lineHdl );
	if( error || lineHdl == NULL )
	{
		error = ERR_MIPIOS_NOT_INITIALIZED;
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_GetLine() line %s not initialized %s%d%s", errStart, functionName, lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* check dev name */
	error = CheckDevName( devName );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckDevName %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	
	/* try to get a device handle */
	error = GetDev( devName, &devHdl );
	if( devHdl == NULL )
	{
		/* must be allocated */
		error = AllocDev( devName, &devHdl, lineHdl );
		if( lineHdl == NULL )
		{
			error = ERR_MIPIOS_ALREADY_INIT;
	        DBGWRT_ERR(( DBH, "%s%s: AllocDev %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}		
	}
	else
	{
		/* already allocated  - bind the descriptor or attach a discovered device */
		if( discovered && devHdl->discovered )
		{
			error = ERR_MIPIOS_ALREADY_INIT;
	        DBGWRT_ERR(( DBH, "%s%s: already discovered %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}

		if( lowlevelDesc && devHdl->llDesc )
		{
			error = ERR_MIPIOS_ALREADY_INIT;
	        DBGWRT_ERR(( DBH, "%s%s: already low level desc %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}

		if( lineHdl != devHdl->mipiosLineP )
		{
			error = ERR_MIPIOS_ILL_PARAM;
	        DBGWRT_ERR(( DBH, "%s%s: %s line %s %s%d%s", errStart, functionName, devName, lineName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}
		
	}

	if( discovered )
	{
		devHdl->discovered = discovered;
		/* add to MDIS/OS2M only if device was discovered */
		error = OS2M_MipiosDevCreate( devName, &devHdl->os2mDevHdl );
		if( error )
		{
        	DBGWRT_ERR(( DBH, "%s%s: OS2M_MipiosDevCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        	goto CLEANUP;
		}
	}
	else
	{
		/* bind a low level descriptor to a MIPIOS device */
		devHdl->llDesc = lowlevelDesc;
		
		/*if( lowlevelDesc == NULL ) */ /* check descriptor */
	}

CLEANUP:
	return( error );	
}


/**********************************************************************/
/** Bind a low level descriptor on a MIPIOS device
 *
 * MIPIOS_DevCreate() bind a low level descriptor on a MIPIOS device.  
 * The given name <devName> must match the name stored in the corresponding
 * MIPIOS device.
 *
 * \param devName	\IN name of the device e.g. "/m66/0" 
 *
 * \return the device handle (>= 0)  on success or \c -1 on error. 
 *         On failure, global \em errno is set to the error code
 */
int32 MIPIOS_DevCreate( char *devName, DESC_SPEC *lowlevelDesc, char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error;
	
    DBGWRT_1((DBH, "%s %s %08x %s\n", functionName, devName, lowlevelDesc, lineName ));

	if( lowlevelDesc == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: desc missing %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	error = MIPIOS_DevCreateCore( devName, lineName, lowlevelDesc, 0 /* not discovered */ );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_DevCreateCore %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );	
}

/**********************************************************************/
/** Create a discovered MIPIOS device
 *
 * MIPIOS_DevCreateByDiscover() creates a discovered MIPIOS device.  
 *
 * \param devName	\IN name of the device e.g. "/m66/0" 
 * \param lineName	\IN name of the line   e.g. "line_A" 
 * \param devHdlP	\OUT pointer to device handle if new or ERR_MIPIOS_ALREADY_INIT
 *
 * \return 0 or error code 
 */
int32 MIPIOS_DevCreateByDiscover( char *devName, char *lineName, MIPIOS_DEVICE **devHdlP )
{
  	static const char *functionName = __FUNCTION__;
	int32 error;
	MIPIOS_DEVICE *devHdl = NULL;
	
    DBGWRT_1((DBH, "%s %s %s\n", functionName, devName, lineName ));
	
	error = MIPIOS_DevCreateCore( devName, lineName, NULL /*no llDesc*/, 1 /*discovered*/ );
	if( error == ERR_MIPIOS_ALREADY_INIT )
	{
		GetDev( devName, &devHdl );
		goto CLEANUP; /* error code stays ERR_MIPIOS_ALREADY_INIT  but the existing devHdl will deleivered */
	}
	else if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_DevCreateCore %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* get device handle */
	error = GetDev( devName, &devHdl );
	if( error || devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: dev %s no handle %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	*devHdlP = devHdl;
	return( error );	
}

/**********************************************************************/
/** Open path to device
 *
 */
int32 AllocPath( MIPIOS_DEVICE	*devHdl, int32 targetPath, int32 *pathTblIdxP )
{
    DBGCMD( static const char *functionName = __FUNCTION__);
	int32 		error = ERR_MIPIOS_NO_SPACE;
	int32		idx;
	
	*pathTblIdxP = -1;

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	for( idx=0; idx<MIPIOS_NUM_FILES; idx++ )
	{
		if( MIPIOS_PathTbl[idx].devHdl == NULL )
		{
			MIPIOS_PathTbl[idx].devHdl		= devHdl; 		/* mark it as allocated */
			MIPIOS_PathTbl[idx].targetPath	= targetPath;
			*pathTblIdxP = idx;
			error = 0;
			break;
		}
	}

	/*--- unlock critical section ---*/
	if( taskUnlock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Close path to device
 *
 */
int32 FreePath( int32 hostPath )
{
    DBGCMD( static const char *functionName = __FUNCTION__);
	int32 		error = ERR_MIPIOS_ILL_PARAM;
	
	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( MIPIOS_PathTbl[hostPath].devHdl != NULL )
	{
		MIPIOS_PathTbl[hostPath].devHdl		= NULL;
		MIPIOS_PathTbl[hostPath].targetPath	= -1;
		error = 0;
	}
	else
	{
        DBGWRT_ERR(( DBH, "%s%s: path not open %d %s%d%s", errStart, functionName, hostPath, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/*--- unlock critical section ---*/
	if( taskUnlock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Open path to device
 *
 */
int32 MIPIOS_Open( OS2M_DEV_HDR *os2mDevHdl, int32 *pathP )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__);
	int32 			error;
	int32 			error2;
	int32 			targetPath = -1;
	int32	  		pathTblIdx = -1;
	MIPIOS_DEVICE	*devHdl  = (MIPIOS_DEVICE*)os2mDevHdl;
	char			*devName = "unknown";

	/* parameter checking */
	if( devHdl == NULL || pathP == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: NULL pointer %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	if( devHdl->name[0] == 0 )
	{
		error = ERR_MIPIOS_NO_DEV;
        DBGWRT_ERR(( DBH, "%s%s: no device %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}
	devName = devHdl->name;

    DBGWRT_1((DBH, "%s %s\n", functionName, devName ));

	/* open target */
	error = MIPIOS_LM_Open( devHdl, &targetPath );
	if( error || targetPath < 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: _MIPIOS_Open %s - error %04x %s%d%s", errStart, functionName, devName, error, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}
	else
	{
		devHdl->openCount++;
		error = targetPath;
	}

	/* alloc path mapping */
	error = AllocPath( devHdl, targetPath, &pathTblIdx );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: AllocPath %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

    DBGWRT_1((DBH, "%s hostPath/targetPath %ld/%ld %s\n", functionName, pathTblIdx+ OS2M_MIPIOS_FILE_HANDLE_START, targetPath, devName ));

	
CLEANUP:
	if( error )
	{
		if( targetPath >= 0 )
		{
			error2 = MIPIOS_LM_Close( devHdl, targetPath );
			if( error2 )
			{
        		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Close %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
			}
		}

		if( pathTblIdx >= 0 )
		{
			error2 = FreePath( pathTblIdx );
			if( error2 )
		        DBGWRT_ERR(( DBH, "%s%s: FreePath %s %s%d%s", errStart, functionName, devName, errLine, __LINE__, errEnd ));
		}
		*pathP = -1;
	}
	else
	{
		/* no error */
		*pathP = pathTblIdx + OS2M_MIPIOS_FILE_HANDLE_START;
	}
	return( error );	
}

/**********************************************************************/
/** Checks the host path
 *  
 * CheckHostPath() checks range and if path is open.
 *
 * \param  hostPath    \IN hostPath
 * \return 0 | error code
 */
int32 CheckHostPath( int32 hostPath )
{
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 error = 0;
	
	if( hostPath < 0 || MIPIOS_NUM_FILES <= hostPath )
	{
		error = ERR_MIPIOS_NO_DEV;
        DBGWRT_ERR(( DBH, "%s%s: out of range %d %s%d%s", errStart, functionName, hostPath, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	if( MIPIOS_PathTbl[hostPath].devHdl == NULL )
	{
		error = ERR_MIPIOS_NO_DEV;
        DBGWRT_ERR(( DBH, "%s%s: not open %d %s%d%s", errStart, functionName, hostPath, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}
	
CLEANUP:
	return( error );
}

/**********************************************************************/
/** Close path to device
 *  
 * MIPIOS_Close() .
 *
 * \param  path    \IN OS2M path
 * \return 0 | error code
 */
int32 MIPIOS_Close( int32 path )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			errorClose = -1;
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;
	
	errorClose = MIPIOS_LM_Close( devHdl, targetPath );
	if( errorClose )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Close %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
        /* only debugs - proceed close handling as nothing happens - may be we have a watchdog error
           and the target has closed all pathes itself */
	}
	else
		devHdl->openCount--;

	if( devHdl->openCount < 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: already closed %d %s%d%s", warnStart, functionName, path, errLine, __LINE__, errEnd ));
	}

	error = FreePath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: FreePath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

CLEANUP:
	if( error )
		return( error ); /* give back range check error...*/
	else
		return( errorClose ); /* before giving back close errors */
}


/**********************************************************************/
/** Read 32-bit integer value from MIPIOS device
 *  
 * MIPIOS_Read() reads a 32-bit integer value from the current channel of
 * the specified path into the variable where value points to, which
 * is passed as a pointer. Depending on the selected I/O mode, the
 * current channel may be incremented on target.
 *
 * \param path          \IN device file handle, returned by M_open()
 * \param valueP        \OUT pointer to variable where read value will  
 *                           be stored.
 *
 * \return 0 | eror code
 */
int32 MIPIOS_Read( int32 path, int32 *valueP )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_Read( devHdl, targetPath, valueP );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Read %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Write 32-bit integer value to device
 *  
 * M_write() outputs a 32-bit integer value to the current channel of
 * the specified path.  Depending on the selected I/O mode, the
 * current channel may be incremented on target.
 *
 * \param path      \IN device file handle, returned by M_open()
 * \param value     \IN value to write
 *
 * \return 0 | eror code
 */
int32 MIPIOS_Write( int32 path, int32 value )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_Write( devHdl, targetPath, value );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Write error %d %s%d%s", errStart, functionName, error, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Read data block from device
 *  
 * MIPIOS_GetBlock() reads a data block of (maximum) \a length bytes from the
 * specified path into buffer \a buf provided by the application and
 * returns the number of bytes read.  Depending on the device driver
 * implementation, you can read data from the hardware or from an
 * input buffer.  
 *
 * If an input buffer is supported, the buffer handling method depends
 * on the block I/O mode used. The minimum size of the application
 * buffer, one buffer entry, can be queried using the \c M_BUF_RD_WIDTH
 * GetStat call. If the requested block size is smaller than one
 * buffer entry, the function returns an error.
 *
 * \param path      \IN device file handle, returned by M_open()
 * \param buffer    \OUT will contain read data
 * \param length    \IN max. number of bytes to read
 *
 * \return number of bytes read on success or \c -1 on error. 
 *
 */
int32 MIPIOS_GetBlock( int32 path, u_int8 *buffer, int32 length, int32 *nbrReadBytesP  )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_GetBlock( devHdl, targetPath, buffer, length, nbrReadBytesP );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_GetBlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}
   

/**********************************************************************/
/** Write data block to device
 *  
 * M_setblock() writes a data block buf of (maximum) \a length bytes to the
 * specified path and returns the number of bytes written.  Depending
 * on the device driver implementation, you can write data to the
 * hardware or to an output buffer.  
 *
 * If an output buffer is supported, the buffer handling method
 * depends on the block I/O mode used. The minimum size of the
 * application buffer, one buffer entry, can be queried using the
 * \c M_BUF_WR_WIDTH GetStat call. If the requested block size is smaller
 * than one buffer entry, the function returns an error.
 *
 * \param path      \IN device file handle, returned by M_open()
 * \param buffer    \IN data block to write
 * \param length    \IN max. number of bytes to write
 *
 * \return number of bytes written on success or \c -1 on error. 
 */
int32 MIPIOS_SetBlock( int32 path, const u_int8 *buffer, int32 length, int32 *nbrWrittenBytesP )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_SetBlock( devHdl, targetPath, (u_int8*)buffer, length, nbrWrittenBytesP );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SetBlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Get status from device
 *  
 * MIPIOS_GetStat() is used to obtain the parameters for path.  The
 * parameter \a code specifies which parameter is to be obtained. The
 * value obtained is written to the variable \a data depending on the
 * status code type:
 *
 * - For normal status codes, \a dataP points to a 32-bit
 *   integer value. 
 * - For block status codes, \a dataP it is interpreted as a
 *   pointer to structure #M_SG_BLOCK, which contains the size and
 *   location of the data buffer. Note that block getstats can
 * 	 be used to exchange data with the driver in both directions
 *   (to and from the driver).
 *
 * All standard status codes are defined in \em MEN/mdis_api.h and in
 * the MDIS4 user guide. In addition, most device drivers also support
 * device-specific status codes. These are defined in the driver’s
 * header file and described in the corresponding device driver user
 * manual.
 *
 * 
 * \param path      \IN device file handle, returned by M_open()
 * \param code      \IN status code (defined in \em mdis_api.h or \em xxx_drv.h
 * \param dataP     \INOUT depends on type of code:
 *     - standard codes: *dataP will contain the obtained paramter
 *     - block codes: dataP must point to #M_SG_BLOCK structure
 *
 * \return \c 0 on success or \c -1 on error. 
 */
int32 MIPIOS_GetStat( int32 path, int32 code, int32 *dataP )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_GetStat( devHdl, targetPath, code, dataP );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_GetStat %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Set status of device
 *  
 * MIPIOS_SetStat() is used to modify the parameters for path. The status
 * code \a code specifies which parameter is to be set to the new value
 * data.  
 * - For normal status codes, \a data contains a 32-bit integer
 *   value. 
 * - For block status codes, \a data it is interpreted as a pointer to
 *   structure #M_SG_BLOCK, which contains the size and location of the
 *   data buffer. Note that block setstats can
 * 	 be used only to send data from the application to the driver (not the
 *   opposite direction).
 *
 * All standard status codes are defined in \em MEN/mdis_api.h and in
 * the MDIS4 user guide. In addition, most device drivers also support
 * device-specific status codes. These are defined in the driver’s
 * header file and described in the corresponding device driver user
 * manual.
 *
 * 
 * \param path      \IN device file handle, returned by M_open()
 * \param code      \IN status code (defined in \em mdis_api.h or \em xxx_drv.h
 * \param data      \IN depends on type of code:
 *     - standard codes: value to write
 *     - block codes: data must point to #M_SG_BLOCK structure
 *
 * \return \c 0 on success or \c -1 on error. 
 */
int32 MIPIOS_SetStat( int32 path, int32 code, int32 data )
{ 
    DBGCMD( static const char *functionName = __FUNCTION__; 	)
	int32 			error;
	int32 			hostPath;
	int32 			targetPath;
	MIPIOS_DEVICE	*devHdl = NULL;

    DBGWRT_1((DBH, "%s path %d\n", functionName, path ));

	/* compute/check host path */
	hostPath =  path - OS2M_MIPIOS_FILE_HANDLE_START;
	error = CheckHostPath( hostPath );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: CheckHostPath %d %s%d%s", errStart, functionName, path, errLine, __LINE__, errEnd ));
   	    goto CLEANUP;
	}

	devHdl 		= MIPIOS_PathTbl[hostPath].devHdl;
	targetPath	= MIPIOS_PathTbl[hostPath].targetPath;

	error = MIPIOS_LM_SetStat( devHdl, targetPath, code, data );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SetStat %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	}

CLEANUP:
	return( error );
}

#else
	/* ================================ if MIPIOS should not be integrated into MDIS==================================== */
	/* #warning "INFO: MIPIOS not included" */

	void MIPIOS_ShowPath( void )
	{
		printf("MIPIOS: excluded\n");
		return;
	}

	int32 MIPIOS_Open(const char *device )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_Close(int32 path )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_GetStat(int32 path, int32 code, int32 *data )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_SetStat(int32 path, int32 code, int32 data )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_Read(int32 path, int32  *value )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_Write(int32 path, int32 value )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_GetBlock(int32 path, u_int8 *buffer, int32 length )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}

	int32 MIPIOS_SetBlock(int32 path, const u_int8 *buffer, int32 length )
	{ 
	    DBGCMD( static const char *functionName = __FUNCTION__; 																	\
		if( OS2M_dbgHdl )																									\
		{																													\
	        DBGWRT_ERR(( DBH, "%s%s: not supported %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));			\
		}																													\
		)/*DBGCMD*/
		return( ERR_MIPIOS_NOT_SUPPORTED );
	}
#endif /*INCLUDE_MIPIOS_VX*/

