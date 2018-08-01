/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mipios_line_manager.c
 *
 *      \author  uf
 *        $Date: 2009/03/31 10:41:10 $
 *    $Revision: 1.2 $
 * 
 *       \brief  MIPIOS for VxWorks.
 *
 * This file contains the MIPIOS line manager for VxWorks.
 *
 *       \note  switches: DBG, INCLUDE_MIPIOS_VX
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_line_manager.c,v $
 * Revision 1.2  2009/03/31 10:41:10  ufranke
 * changed
 *  - switch from EXCLUDE_MIPIOS_VX to INCLUDE_MIPIOS_VX
 *
 * Revision 1.1  2009/01/30 09:21:05  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
#ifdef INCLUDE_MIPIOS_VX

/* ================================ if MIPIOS should be integrated into MDIS==================================== */
const char *MIPIOS_VX_LM_RCSid="$Id: mipios_line_manager.c,v 1.2 2009/03/31 10:41:10 ufranke Exp $\n"
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

#include <in.h>		
#include <string.h>
#include <sysLib.h>
#include <taskLib.h>
#include <tickLib.h>

#include <MEN/mdis_api.h>   /* MDIS api codes            */
#include <MEN/mdis_err.h>   /* MDIS error codes          */

#include <MEN/oss.h>
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



#define _COMPILE_MIPIOS_INTERNAL_
#include "mipios_line_manager.h"
#include <MEN/mipios_vx_api.h>
#include "mipios_communication.h"


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifdef __BIG_ENDIAN__
	#define _SWAP_16_CPUE( _x_ )	OSS_SWAP16(_x_)
	#define _SWAP_32_CPUE( _x_ )	OSS_SWAP32(_x_)
#else
	#define _SWAP_32_CPUE( _x_ )	(_x_)
	#define _SWAP_16_CPUE( _x_ )	(_x_)
#endif

/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/
static int LM_initialized_G = 0;
MIPIOS_LINE    MIPIOS_LineTbl[MIPIOS_MAX_LINES];


/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/

/**********************************************************************/
/** Initializes the MIPIOS library.
 *
 * MIPIOS_LM_Init() checks if the terminating 0 is inside the array[<max>] .  
 *
 * \return 0 or ERR_MIPIOS_ALREADY_INIT
 */
int32 MIPIOS_LM_Init( void )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error = 0;

    DBGWRT_1((DBH, "%s\n", functionName ));

	if( LM_initialized_G )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: already initialized %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	bzero( (char*)MIPIOS_LineTbl, sizeof(MIPIOS_LineTbl) );

	LM_initialized_G = 1;

CLEANUP:
	return( error );
}


/**********************************************************************/
/** Save string length check.
 *
 * MIPIOS_LM_StrMax() checks if the terminating 0 is inside the array[<max>] .  
 *
 * \param str	\IN pointer to array
 * \param max	\IN max of array
 *
 * \return 0 or ERR_MIPIOS_ILL_PARAM 
 */
int32 MIPIOS_LM_StrMax( char *str, int max )
{
	int 	i;
	int32	error = ERR_MIPIOS_ILL_PARAM;

	for( i=0; i < max; i++ )
	{
		if( *str++ == 0 )
		{
			error = 0;
			break;
		}
	}
	return( error );
}

/**********************************************************************/
/** Save string length.
 *
 * MIPIOS_LM_StrLenMax() counts the length of string until max.  
 *
 * \param str	\IN pointer to array
 * \param max	\IN max of array
 *
 * \return 0..max - if max terminating 0 has not been found
 */
int32 MIPIOS_LM_StrLenMax( char *str, int max )
{
	int 	i = 0;

	if( str == NULL )
	{
		goto CLEANUP;
	}

	for( i=0; i < max; i++ )
	{
		if( *str++ == 0 )
		{
			break;
		}
	}

CLEANUP:
	return( i );
}

/**********************************************************************/
/** Checks the parameter lineName.
 *
 * MIPIOS_LM_CheckLineName() the parameter lineName.
 *
 * \param lineName	\IN pointer to array
 *
 * \return 0 or ERR_MIPIOS_ILL_PARAM 
 */
int32 MIPIOS_LM_CheckLineName( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = 0;

	/* NULL check */
	if( lineName == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: lineName missing %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	/* length check */
	error = MIPIOS_LM_StrMax( lineName, MIPIOS_MAX_LINE_NAME );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_StrMax() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Get the MIPIOS line by name
 *
 * MIPIOS_LM_GetLine() get the line.  
 *
 * \param lineName	\IN line name
 * \param lineHdlP	\IN pointer to variable where lineHdl will be stored
 *
 * \return 0 | error code
 */
int32 MIPIOS_LM_GetLine( char *lineName, MIPIOS_LINE **lineHdlP )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS_NOT_INITIALIZED;
	int			i;
	int			found;

	if( lineName == NULL || lineHdlP == NULL )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	*lineHdlP = NULL;

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	for( i=0; i<MIPIOS_MAX_LINES ;i++ )
	{
		found = !strncmp( MIPIOS_LineTbl[i].lineName, lineName, MIPIOS_MAX_LINE_NAME );
		if( found )
		{
			*lineHdlP = &MIPIOS_LineTbl[i];
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
/** Allocate space for a MIPIOS line
 *
 * AllocLine() get space for a MIPIOS line.  
 *
 * \param lineName	\IN line name
 * \param lineHdlP	\IN pointer to variable where lineHdl will be stored
 *
 * \return 0 | error code
 */
static int32 AllocLine( char *lineName, DESC_SPEC *lineDesc, MIPIOS_LINE **lineHdlP )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS_NO_SPACE;
	int			i;
	MIPIOS_LINE *lineHdl = NULL;

	*lineHdlP = NULL;

	/* check if line already initialized */
	error = MIPIOS_LM_GetLine( lineName, &lineHdl );
	if( lineHdl != NULL )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: line %s already initialized %s%d%s", errStart, functionName, lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* look for free space */
	for( i=0; i<MIPIOS_MAX_LINES ;i++ )
	{
		if( MIPIOS_LineTbl[i].lineName[0] == 0 )
		{
			strcpy( MIPIOS_LineTbl[i].lineName, lineName ); /* mark it allocated */
			MIPIOS_LineTbl[i].lineDesc = lineDesc;
			*lineHdlP = &MIPIOS_LineTbl[i];
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
/** Free space for a MIPIOS line
 *
 * FreeLine() free space of a MIPIOS line.  
 *
 * \param lineName	\IN line name
 *
 * \return 0 | error code
 */
static int32 FreeLine( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS;
	int			i;
	int32		len;

	len = MIPIOS_LM_StrLenMax( lineName, MIPIOS_MAX_LINE_NAME );
	if( len == 0 || len == MIPIOS_MAX_LINE_NAME )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_StrLenMax %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/*--- lock critical section ---*/
	if( taskLock() )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	error = ERR_MIPIOS_ILL_PARAM; /* not found */
	/* look for free space */
	for( i=0; i<MIPIOS_MAX_LINES ;i++ )
	{
		if( strncmp( lineName, MIPIOS_LineTbl[i].lineName, MIPIOS_MAX_LINE_NAME) == 0  )
		{
			bzero( (char*)&MIPIOS_LineTbl[i], sizeof(MIPIOS_LINE) ); /* mark it as free */
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
/** Sorts a rx frame to the standard or extra buffer of a line.
 *
 * MIPIOS_LM_SortToRxBuffer() sorts a rx frame to the standard
 * or extra buffer of a MIPIOS line.
 *
 * \param lineHdl		\IN  line handle
 * \param cpueRxFrameP	\IN  rx frame with header in CPU endian format
 * \param ip			\IN  target IP address as u_int32
 * \param targetIp		\IN  target IP address as string
 * \param frameHdlP		\IN  NULL for internal or &rxOutputFrameHdl
 *
 * \return 0 or error code 
 */
int MIPIOS_LM_SortToRxBuffer
(
	MIPIOS_LINE *lineHdl,
	MIPIOS_FRAME *cpueRxFrameP,
	u_int32 ip,
	char *targetIp,
	MIPIOS_FRAME_HDL	*frameHdlP
)
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;


	if( lineHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: lineHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( cpueRxFrameP == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: cpueRxFrameP %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( targetIp == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: targetIp %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

    DBGWRT_1(( DBH, "%s: IP %s l/f/r %08x/%d/%d r/errno/s %d/%08x/%d t/c %02x/%02x\n", functionName, targetIp, 
    			cpueRxFrameP->hdr.linkNumber, cpueRxFrameP->hdr.devFrameNbr, cpueRxFrameP->hdr.retryCnt,
    			cpueRxFrameP->hdr.returnCode, cpueRxFrameP->hdr.errorNo, cpueRxFrameP->hdr.dataSize,
    			cpueRxFrameP->hdr.type, cpueRxFrameP->hdr.command ));

	if( frameHdlP == NULL ) /* use an internal frame handle */
	{
		/* check if autoconnect frame */
		if( (cpueRxFrameP->hdr.type == MIPIOS_TYPE_DEV_NAME)
			&& ((cpueRxFrameP->hdr.command & ~MIPIOS_TYPE_RESPONSE_REQ_MASK) == MIPIOS_CMD_ANNOUNCE_DEV_NAME) )
		{
    	    DBGWRT_3(( DBH, "%s: target autoconnect from IP %s\n", functionName, targetIp ));
        	frameHdlP = &lineHdl->rxAutoConnectFrameHdl;
		}
		else
		{
    	    frameHdlP = &lineHdl->rxFrameHdl;
		}
	}
	
	if( frameHdlP == NULL )
	{
		/* NULL pointer */
		error = ERR_MIPIOS_NO_SPACE;
		DBGWRT_ERR(( DBH, "%s%s: IP %s frameHdlP %s%d%s", errStart, functionName, targetIp, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}	

	/*-------------------------+
	| critical section         |
	+-------------------------*/
	{
		error = OSS_SemWait( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffAccess, OSS_SEM_WAITFOREVER );
		if( error )
		{
			DBGWRT_ERR(( DBH, "%s%s: IP %s OSS_SemWait() %s%d%s", errStart, functionName, targetIp, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
	
		/* check buffer empty */
		if( !frameHdlP->empty )
		{
			u_int32 linkNbr = frameHdlP->frame.hdr.linkNumber;
			u_int32 frameNbr = frameHdlP->frame.hdr.devFrameNbr;
			u_int8	rtry 	 = frameHdlP->frame.hdr.retryCnt;
			
			lineHdl->errCnt.rxBuff++;
			DBGWRT_ERR(( DBH, "%s%s: IP %s buffer overrun # l/f/r %08x/%d/%d %s%d%s", errStart, functionName, 
						targetIp, linkNbr, frameNbr, rtry, errLine, __LINE__, errEnd ));
		}

		/* copy frame data */
		frameHdlP->empty	= 0;
		frameHdlP->ip		= ip;
		memcpy( (char*)&frameHdlP->frame, cpueRxFrameP, sizeof(MIPIOS_FRAME) );  /* can size optimized */
	
		error = OSS_SemSignal( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffAccess );
		if( error )
		{
			DBGWRT_ERR(( DBH, "%s%s: IP %s OSS_SemSignal() %s%d%s", errStart, functionName, targetIp, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
	}

	/*----------------------------------------------+
	| signal that there is something in the buffer  |
	+----------------------------------------------*/
	error = OSS_SemSignal( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffFull );
	if( error )
	{
		DBGWRT_ERR(( DBH, "%s%s: IP %s OSS_SemSignal() %s%d%s", errStart, functionName, targetIp, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}



	
CLEANUP:
	return( error );
}

/**********************************************************************/
/** Gets a rx frame from the standard or extra buffer.
 *
 * GetFrameFromBuffer() gets a rx frame from the standard
 * or extra buffer with timeout. 
 *
 * \param lineHdl		\IN  line handle
 * \param frame			\IN  where buffer will be copied to
 * \param ipP  			\OUT IP address
 * \param ticksToWait	\IN  how long we wait for semBuffFull
 *
 * \return 0 or error code 
 */
static u_int32 GetFrameFromBuffer
(
	MIPIOS_FRAME_HDL *frameHdlP,
	MIPIOS_FRAME *frameP, 
	u_int32 *ipP,
	int ticksToWait
)
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error;
	int32 				errRet = 0;
	
	
	*ipP = 0;
	/*-------------------------+
	| check buffer full        |
	+-------------------------*/
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffFull, ticksToWait );
	if( error )
	{
		/* empty */
		error = ERR_MIPIOS_NO_BUF;
        goto CLEANUP;
	}
	
	/*-------------------------+
	| critical section         |
	+-------------------------*/
	{
		error = OSS_SemWait( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffAccess, OSS_SEM_WAITFOREVER );
		if( error )
		{
			DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
	
		/* check buffer empty */
		if( frameHdlP->empty )
		{
			DBGWRT_ERR(( DBH, "%s%s: buffer underrun %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			errRet = ERR_MIPIOS;
		}

		/* copy frame data / ip */
		frameHdlP->empty	= 1;
		*ipP 				= frameHdlP->ip;
		memcpy( frameP, (char*)&frameHdlP->frame, sizeof(MIPIOS_FRAME) );  /* can size optimized */
	
		error = OSS_SemSignal( OSS_VXWORKS_OS_HDL, frameHdlP->semBuffAccess );
		if( error )
		{
			DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
	}

CLEANUP:
	return( error );
}


/**********************************************************************/
/** Check the header of a response frame.
 *
 * CheckResponseFrameHeader() checks the following fields of a received
 * header:
 * 				linkNumber
 * 				devFrameNbr
 * 				retryCnt
 *				type
 *				command
 *				size ( if possible )
 *
 *
 * \param rxHdrP	\IN  received frame header
 * \param txHdrP	\IN  tranmitted frame header
 *
 * \return 0 or error code 
 */
int32 CheckResponseFrameHeader( MIPIOS_FRAME_HEADER *rxHdrP, MIPIOS_FRAME_HEADER *txHdrP )
{
  	static const char *functionName = __FUNCTION__;
  	u_int32			  error = ERR_MIPIOS_ILL_PARAM;
  	u_int8			  cmd;
  	int 			  expectedRxDataSize;
	
	if( rxHdrP->linkNumber != txHdrP->linkNumber )
	{
		DBGWRT_ERR(( DBH, "%s%s: link %x but is %x %s%d%s", errStart, functionName, txHdrP->linkNumber, rxHdrP->linkNumber,  errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( rxHdrP->devFrameNbr != txHdrP->devFrameNbr )
	{
		DBGWRT_ERR(( DBH, "%s%s: frame# %d but is %d %s%d%s", errStart, functionName, txHdrP->devFrameNbr, rxHdrP->devFrameNbr,  errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( rxHdrP->retryCnt != txHdrP->retryCnt )
	{
		DBGWRT_ERR(( DBH, "%s%s: retry %d but is %d %s%d%s", errStart, functionName, txHdrP->retryCnt, rxHdrP->retryCnt,  errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( rxHdrP->type != txHdrP->type )
	{
		DBGWRT_ERR(( DBH, "%s%s: typ %x but is %x %s%d%s", errStart, functionName, txHdrP->type, rxHdrP->type,  errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	cmd = (txHdrP->command & ~MIPIOS_TYPE_RESPONSE_REQ_MASK);
	if( rxHdrP->command != cmd )
	{
		if( (rxHdrP->command == MIPIOS_CMD_M_SETSTAT_SIG_INSTALLED || rxHdrP->command == MIPIOS_CMD_M_SETSTAT_SIG_REMOVED)
			&& cmd == MIPIOS_CMD_M_SETSTAT 
		  )
		{
			/* all right */
		}
		else
		{
			DBGWRT_ERR(( DBH, "%s%s: cmd %x but is %x %s%d%s", errStart, functionName, cmd, rxHdrP->command,  errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
    	}
	}
	
	switch( cmd )
	{
		/* response data must be > 0 */
		case MIPIOS_CMD_ANNOUNCE_DEV_NAME:
			expectedRxDataSize = MIPIOS_DATA_SIZE_NOT_FIX_GTZERO;
			break;

		/* response data can be 0..MAX */
		case MIPIOS_CMD_M_GET_BLK:
		case MIPIOS_CMD_M_GETSTAT_BLK:
		case MIPIOS_CMD_SIGNAL_POLL:
			expectedRxDataSize = MIPIOS_DATA_SIZE_NOT_FIX;
			break;

		/* response data must be == 0 */
		case MIPIOS_CMD_ESTABLISH_LINK:
		case MIPIOS_CMD_OPERATIONAL:
		case MIPIOS_CMD_GUARDING:
		case MIPIOS_CMD_SET_LL_DESC:
		case MIPIOS_CMD_M_OPEN:
		case MIPIOS_CMD_M_CLOSE:
		case MIPIOS_CMD_M_WRITE:
		case MIPIOS_CMD_M_SET_BLK:
		case MIPIOS_CMD_M_SETSTAT_BLK:
			expectedRxDataSize = MIPIOS_DATA_SIZE_0;
			break;

		case MIPIOS_CMD_M_SETSTAT:
			expectedRxDataSize = MIPIOS_DATA_SIZE_0;  /* may be != 0 for block set/getstats */
			if( rxHdrP->command == MIPIOS_CMD_M_SETSTAT_SIG_INSTALLED || rxHdrP->command == MIPIOS_CMD_M_SETSTAT_SIG_REMOVED )
			{
				expectedRxDataSize = MIPIOS_DATA_SIZE_8;
			}
			break;

		case MIPIOS_CMD_M_GETSTAT:
			expectedRxDataSize = MIPIOS_DATA_SIZE_4;  /* may be != 0 for block set/getstats */
			break;

		/* response data must be == 4 */
		case MIPIOS_CMD_M_READ:
			expectedRxDataSize = MIPIOS_DATA_SIZE_4;
			break;

		default:	
			{
				DBGWRT_ERR(( DBH, "%s%s: unknown cmd %02x %s%d%s", errStart, functionName, cmd, errLine, __LINE__, errEnd ));
        		goto CLEANUP;
			}
			break;
	}	
	
	if(	expectedRxDataSize != MIPIOS_DATA_SIZE_NOT_FIX )
	{
		if( expectedRxDataSize == MIPIOS_DATA_SIZE_NOT_FIX_GTZERO && rxHdrP->dataSize == 0 )
		{
			DBGWRT_ERR(( DBH, "%s%s: expected/size %d/%d %s%d%s", errStart, functionName, 0, rxHdrP->dataSize, errLine, __LINE__, errEnd ));
			goto CLEANUP;
		}
		else if ( rxHdrP->dataSize != expectedRxDataSize )
		{
			DBGWRT_ERR(( DBH, "%s%s: expected/size %d/%d %s%d%s", errStart, functionName, expectedRxDataSize, rxHdrP->dataSize, errLine, __LINE__, errEnd ));
			goto CLEANUP;
		}
	}	

	error = 0;


CLEANUP:
	return( error );	
}

/**********************************************************************/
/** Store the last send time for a device.
 *
 * MIPIOS_SaveLastSendTime() stores the last send time to a device.
 * 
 * \param devHdl		\IN  device handle
 *
 * \return N/A 
 */
void MIPIOS_SaveLastSendTime( MIPIOS_DEVICE	*devHdl )
{
	devHdl->lastSendTick = tickGet();
}

/**********************************************************************/
/** Decides if a device has to be polled.
 *
 * MIPIOS_CheckPollTimer() checks if time to poll signals of a device.
 * The current tick will be compared with the last tick at calling
 * MIPIOS_LM_SendReceive() for this device. 
 * 
 * \param devHdl		\IN  device handle
 *
 * \return 0 | 1 for have to poll device
 */
int MIPIOS_CheckSignalPollTimer( MIPIOS_DEVICE	*devHdl )
{
  	static const char *functionName = __FUNCTION__;
	int		haveToPollDevice = 0;
	u_int32 currentTick		 = tickGet();
	u_int32	diffTicks;
	u_int32	diffMs;
	
	if( currentTick > devHdl->lastSendTick )
		diffTicks = currentTick - devHdl->lastSendTick;
	else
		diffTicks = (0xFFFFFFFF - devHdl->lastSendTick) + currentTick;  /* rollover */
	
	diffMs = (diffTicks * 1000) / sysClkRateGet();
	if( diffMs == 0 )
		diffMs++;

	if(	devHdl->timeoutMs_signalPoll < diffMs )
	{
	   	DBGWRT_ERR(( DBH, "%s: %s diffMs %d haveToPoll\n", functionName, devHdl->name, diffMs ));
		haveToPollDevice = 1;
	}
	
	return( haveToPollDevice );
}

/**********************************************************************/
/** Decides if a device has to be guarded.
 *
 * MIPIOS_CheckGuardingTimer() checks if time to guard a device.
 * The current tick will be compared with the last tick at calling
 * MIPIOS_LM_SendReceive() for this device. 
 * 
 * \param devHdl		\IN  device handle
 *
 * \return 0 | 1 for have to guard device
 */
int MIPIOS_CheckGuardingTimer( MIPIOS_DEVICE	*devHdl )
{
  	static const char *functionName = __FUNCTION__;
	int		haveToGuardDevice = 0;
	u_int32 currentTick		 = tickGet();
	u_int32	diffTicks;
	u_int32	diffMs;
	
	if( currentTick > devHdl->lastSendTick )
		diffTicks = currentTick - devHdl->lastSendTick;
	else
		diffTicks = (0xFFFFFFFF - devHdl->lastSendTick) + currentTick;  /* rollover */
	
	diffMs = (diffTicks * 1000) / sysClkRateGet();
	if( diffMs == 0 )
		diffMs++;

	if(	devHdl->timeoutMs_guarding < diffMs )
	{
	   	DBGWRT_ERR(( DBH, "%s: %s diffMs %d haveToGuard\n", functionName, devHdl->name, diffMs ));
		haveToGuardDevice = 1;
	}
	
	return( haveToGuardDevice );
}

/**********************************************************************/
/** Send/Receive a frame with retry.
 *
 * MIPIOS_LM_SendReceive() send/receive with receive. 
 * The CRC32 of the TX frame will be set here. Calls MIPIOS_COM_Send()
 * to transmit a frame. GetFrameFromBuffer() gets frames from
 * lineHdl->rxFrameHdl with timeout. Received frames will be checked
 * by CheckResponseFrameHeader().
 * 
 * \param lineHdl		\IN  line handle
 *
 * \return 0 or error code 
 */
static u_int32 MIPIOS_LM_SendReceive
(
	MIPIOS_LINE		*lineHdl,
	MIPIOS_DEVICE	*devHdl,
	u_int32			ip,
	u_int16			port,
	MIPIOS_FRAME	*txFrameP,
	u_int32			*rxIpP,
	MIPIOS_FRAME	*rxFrameP
)
{
  	static const char *functionName = __FUNCTION__;
  	u_int32			error;
	u_int32			crc32;

   	DBGWRT_1(( DBH, "%s: %s %s frame# %d\n", functionName, lineHdl->lineName, devHdl->name, txFrameP->hdr.devFrameNbr ));

	devHdl->frameNumber++; /* count up the device frame number */

	/* store the last send time to control scheduler for guarding/signal polling */
	MIPIOS_SaveLastSendTime( devHdl );

	/* send/receive with retry */
	do
	{
		/* send */
		lineHdl->errCnt.txTotal++;

		/* CRC32 */
		crc32 			  	= MIPIOS_Crc32Get( ((u_int8*)txFrameP)+4, (sizeof(MIPIOS_FRAME_HEADER) - 4) + txFrameP->hdr.dataSize );
		txFrameP->hdr.crc32 = crc32;

		error = MIPIOS_COM_Send( lineHdl->fhTxSocket, ip, port, txFrameP );
		if( error )
		{
			lineHdl->errCnt.socket++;
	    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_COM_Send( %s ) retry %d %s%d%s", errStart, functionName, 
   							lineHdl->lineName, devHdl->name, txFrameP->hdr.retryCnt, errLine, __LINE__, errEnd ));
   			continue; /* retry */
		}

		/* receive  without timeout */
		error = GetFrameFromBuffer( &lineHdl->rxFrameHdl, rxFrameP, rxIpP, 0 /* no wait */ );
		if( error )
		{
			if( error != ERR_MIPIOS_NO_BUF )
			{
				 /* internal error */
	   	    	DBGWRT_ERR(( DBH, "%s%s: %s GetFrameFromBuffer() internal error %s%d%s", errStart, functionName, 
   							lineHdl->lineName, errLine, __LINE__, errEnd ));
				continue; 
			}
			else
			{
				/* nothing in the buffer - receive with timeout now */
				error = GetFrameFromBuffer( &lineHdl->rxFrameHdl, rxFrameP, rxIpP, lineHdl->timeoutTicks ); 
				if( error )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s GetFrameFromBuffer() %s%d%s", errStart, functionName, 
   								lineHdl->lineName, errLine, __LINE__, errEnd ));
					continue; /* timeout -> retry */
				}
				else
					goto CHECK_DELAYED_FRAME2; /* we have something in the buffer -> check the RX frame */
			}
		}
		else
		{
			/* there was a frame very quick - check response frame - may be there are old data inside */
			error = CheckResponseFrameHeader( &rxFrameP->hdr, &txFrameP->hdr );
			if( error )
			{
				/* old data */
	   	    	DBGWRT_ERR(( DBH, "%s%s: %s GetFrameFromBuffer() - ignoring old data %s%d%s", warnStart, functionName, 
   					lineHdl->lineName, errLine, __LINE__, errEnd ));

				/* buffer contains old data - retry receive with timeout */
				error = GetFrameFromBuffer( &lineHdl->rxFrameHdl, rxFrameP, rxIpP, lineHdl->timeoutTicks ); 
				if( error )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s GetFrameFromBuffer() %s%d%s", errStart, functionName, 
   								lineHdl->lineName, errLine, __LINE__, errEnd ));
					continue; /* timeout -> retry */
				}
				else
					goto CHECK_DELAYED_FRAME2; /* we have something in the buffer -> check the RX frame */
			}
			else
			{
				/* no error - response frame matches - leave while */
				continue;
			}
		}
		
	  CHECK_DELAYED_FRAME2:
		/* check response frame */
		error = CheckResponseFrameHeader( &rxFrameP->hdr, &txFrameP->hdr );
		if( error )
		{
	   	    DBGWRT_ERR(( DBH, "%s%s: GetFrameFromBuffer() %s %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
		}
		
	}while( error 
	        && txFrameP->hdr.retryCnt++ < (lineHdl->maxRetry) );

	if( !error )
	{
		/* check for transmitted signal */
		if( rxFrameP->hdr.signalCnt )
		{
			MIPIOS_SIG_Signal( rxFrameP->hdr.linkNumber, 
							   rxFrameP->hdr.devFrameNbr,
							   rxFrameP->hdr.retryCnt,
							   rxFrameP->hdr.singleSignal );

			if( rxFrameP->hdr.signalCnt > 1 )
			{
				devHdl->haveToPollSignals = 1; /* remainder for more signals waiting in device */
			}
		}
	}
	
	return( error );	
}



/**********************************************************************/
/** MIPIOS line task send/receive on a line.
 *
 * LineTask() MIPIOS line task handles communication on a line like
 * MDIS calls, signal polling, guarding and target autoconnection.  
 * Guarding and signal polling is only active if line and device
 * is in state MIPIOS_STATE_OPERATIONAL.
 *
 * \note   only this task counts up the frame number of a device
 *
 * \param  lineHdl
 *
 * \return 0 | error code
 */
static int LineTask( MIPIOS_LINE *lineHdl )
{
  	static const char *functionName = __FUNCTION__;
  	u_int32			error;
  	u_int32			ip;
  	u_int32			rxIp;
  	u_int8			*ipArr = (u_int8*)&ip;
	u_int16			port;	
	u_int8			typ;	
	u_int8			cmd;	
	MIPIOS_FRAME	txFrame;
	MIPIOS_FRAME	rxFrame;
	MIPIOS_DEVICE   *devHdl;
	int				nothingDoneLastLoop = 1;
	int 			normalTxWait;
	int				i;
	u_int32			dataSize;
	  	
  	if( lineHdl == NULL )
  	{
		DBGWRT_ERR(( DBH, "%s%s: lineHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
  		return( -1 );
  	}
  	
	DBGWRT_1((DBH, "%s %s started\n", functionName, lineHdl->lineName ));
   
	for(;;)
	{
		/* are we nice to the other tasks ? */
		if( nothingDoneLastLoop )
			normalTxWait = 1; /* last time it was nothing to do, so we want give process time to the other processes */
		else
			normalTxWait = 0; /* hurry hurry - we are not nice to other processes */
			
		nothingDoneLastLoop = 1;			

/*-----------------------------------------------------+ 
| normal communication - wait for TX sem               |
+-----------------------------------------------------*/
		error = OSS_SemWait( OSS_VXWORKS_OS_HDL, lineHdl->txSem, normalTxWait ); /* zero or 1 tick wait */
		if( error )
		{
			goto GUARDING_POLLING;
		}
		else
		{
			nothingDoneLastLoop = 0;  /* there was an interesting thing for us - zero wait next loop */

			/*----------------+
			|  TX section     |
			+----------------*/
			{
				/* lock critical section */
				if( taskLock() )
				{
					error = ERR_MIPIOS;
			        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		    	    goto CLEANUP;
				}
	
				devHdl		= (MIPIOS_DEVICE*)lineHdl->currDevHdl;
				memcpy( &txFrame, &lineHdl->txFrame, sizeof(txFrame) ); /* can size optimized */
	
	
				/* unlock critical section */
				if( taskUnlock() )
				{
					error = ERR_MIPIOS;
			        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		    	    goto CLEANUP;
				}
			}			   

			txFrame.hdr.linkNumber		= devHdl->linkNumber;		/* FRAME: link number */
			txFrame.hdr.devFrameNbr		= devHdl->frameNumber;		/* FRAME: setup frame number */
			txFrame.hdr.magic			= MIPIOS_MAGIC;
			txFrame.hdr.version			= MIPIOS_VERSION_1;

			ip   = devHdl->targetIp;
			port = devHdl->targetReceivePort;

			/* send/receive with retry */
			error = MIPIOS_LM_SendReceive( lineHdl, devHdl, ip, port, &txFrame, &rxIp, &rxFrame );
			if( error )
			{
   	    		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SendReceive() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			}
			else
			{
	   	    	DBGWRT_ERR(( DBH, "%s: %s MIPIOS_LM_SendReceive() frame# %d - OK\n", functionName, devHdl->name, devHdl->frameNumber ));

	   	    	/* copy rx buffer to output */
        		error = MIPIOS_LM_SortToRxBuffer( lineHdl, &rxFrame, 0, "unkown", &lineHdl->rxOutputFrameHdl );
        		if( error )
        		{
			        DBGWRT_ERR(( DBH, "%s%s: line %s MIPIOS_LM_SortToRxBuffer() rxOutputFrameHdl error %d %s%d%s", errStart, functionName, 
	        			 lineHdl->lineName, error, errLine, __LINE__, errEnd ));

	        		/*/* error handling ... - switching dev, line or system into STOP mode */
        		}

			}
		}/*if TX sem*/

/*-------------------------------------------------------------------------------+ 
|  AUTO driven communication schedule guarding, signal polling, link setup       |
+-------------------------------------------------------------------------------*/
GUARDING_POLLING:
		/* check each device on this line - if there is something to do for */
		{
			int devIdx;
			for( devIdx=0; devIdx<MIPIOS_MAX_DEVS_PER_LINE; devIdx++ )
			{
				devHdl = lineHdl->devTbl[devIdx];

				if( devHdl == NULL )
					continue; /* check next dev */

				if( devHdl->mipiosLineP == NULL )
				{
					continue; /* check next dev */
				}

				if( !devHdl->discovered )
				{
					continue; /* check next dev */
				}

				if( devHdl->mipiosLineP != lineHdl )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s dev[%d] line %08x/%08x mismatch %s%d%s", errStart, functionName, 
	   	    				lineHdl->lineName, devIdx, devHdl->mipiosLineP, lineHdl,
	   	    				errLine, __LINE__, errEnd ));
					continue; /* check next dev */
				}

				switch( lineHdl->state )
				{
					default:
						/* fall through */
					case MIPIOS_STATE_STOP: /* this is the save state */
						break;
						
					case MIPIOS_STATE_OPERATIONAL:
						bzero( (char*)&txFrame, sizeof(txFrame) );		/* can size optimized */
						txFrame.hdr.linkNumber		= devHdl->linkNumber;
						txFrame.hdr.devFrameNbr		= devHdl->frameNumber;
						txFrame.hdr.retryCnt		= 0;
						txFrame.hdr.magic			= MIPIOS_MAGIC;
						txFrame.hdr.version			= MIPIOS_VERSION_1;
						
						ip   = devHdl->targetIp;
						port = devHdl->targetReceivePort;

						if( devHdl->state == MIPIOS_STATE_CONFIGURATION )
						{
							/* set target to operational state - host endian information */
							txFrame.hdr.type	= MIPIOS_TYPE_CONFIGURATION;
							txFrame.hdr.command	= MIPIOS_CMD_OPERATIONAL | MIPIOS_TYPE_RESPONSE_REQ_MASK;

					 		txFrame.hdr.dataSize = MIPIOS_DATA_SIZE_1;

					 	  #ifdef __BIG_ENDIAN__
					 		txFrame.data[0] = MIPIOS_HOST_IS_BIG_ENDIAN;
					 	  #else
					 		txFrame.data[0] = MIPIOS_HOST_IS_LITTLE_ENDIAN;
					 	  #endif

							/* send/receive with retry */
							error = MIPIOS_LM_SendReceive( lineHdl, devHdl, ip, port, &txFrame, &rxIp, &rxFrame );
							if( error )
							{
				   	    		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SendReceive() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
								devHdl->state = MIPIOS_STATE_STOP;
								lineHdl->state = MIPIOS_STATE_STOP;  /* line fallback to stop */
							}
							else
							{
								devHdl->state = MIPIOS_STATE_OPERATIONAL;
					   	    	DBGWRT_ERR(( DBH, "%s: %s is operational\n", functionName, devHdl->name ));
							}
						}
						else if( devHdl->state == MIPIOS_STATE_OPERATIONAL )
						{
							/*-------------------------+
							|   guarding               |
							+-------------------------*/
							if( MIPIOS_CheckGuardingTimer( devHdl ) )
							{
					   	    	DBGWRT_ERR(( DBH, "%s: guarding %s\n", functionName, devHdl->name ));

								txFrame.hdr.devFrameNbr		= devHdl->frameNumber;
								txFrame.hdr.retryCnt		= 0;

								txFrame.hdr.type	= MIPIOS_TYPE_GUARDING;
								txFrame.hdr.command	= MIPIOS_CMD_GUARDING | MIPIOS_TYPE_RESPONSE_REQ_MASK;
						 		txFrame.hdr.dataSize = MIPIOS_DATA_SIZE_0;

								/* send/receive with retry */
								error = MIPIOS_LM_SendReceive( lineHdl, devHdl, ip, port, &txFrame, &rxIp, &rxFrame );
								if( error )
								{
					   	    		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SendReceive() while guarding %s %s%d%s", errStart, functionName,
					   	    							devHdl->name, errLine, __LINE__, errEnd ));
									devHdl->state = MIPIOS_STATE_STOP;
									lineHdl->state = MIPIOS_STATE_STOP;  /* line fallback to stop */
									break; /* leave the for loop */
								}

							}
							
							/*---------------------------------------------------------------------------------+
							| signal polling - max 250 signals + 1 singleSignal in one MIPIOS_LM_SendReceive   |
							|                  * 3 loop = 753 signals                                          |
							+---------------------------------------------------------------------------------*/
							if( !devHdl->haveToPollSignals       /* no communication or no more signal in frame */
								&& devHdl->installedSignalCount  /* there are signals installed on device */
							  )
							{
								devHdl->haveToPollSignals = MIPIOS_CheckSignalPollTimer( devHdl );
							}
							
                            for( i=0; devHdl->haveToPollSignals && (i < 3); i++  )
                            {
                            	u_int8	signalCnt;
                            	int		idx;
                            	u_int32	*signalP;

								txFrame.hdr.devFrameNbr		= devHdl->frameNumber;
								txFrame.hdr.retryCnt		= 0;

					   	    	DBGWRT_ERR(( DBH, "%s: poll %s\n", functionName, devHdl->name ));
								/* printf(">>> haveToPollSignals i=%d\n", i);
								fflush( stdout );
								taskDelay( sysClkRateGet() /5 ); */

                            	devHdl->haveToPollSignals = 0; /* reset device poll request */

								txFrame.hdr.type	= MIPIOS_TYPE_DATA;
								txFrame.hdr.command	= MIPIOS_CMD_SIGNAL_POLL | MIPIOS_TYPE_RESPONSE_REQ_MASK;
						 		txFrame.hdr.dataSize = MIPIOS_DATA_SIZE_0;

								/* send/receive with retry */
								error = MIPIOS_LM_SendReceive( lineHdl, devHdl, ip, port, &txFrame, &rxIp, &rxFrame );
								if( error )
								{
					   	    		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SendReceive() while polling signals %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
									devHdl->state = MIPIOS_STATE_STOP;
									lineHdl->state = MIPIOS_STATE_STOP;  /* line fallback to stop */
									break; /* leave the for loop */
								}

								signalCnt = rxFrame.hdr.signalCnt;
								dataSize  = rxFrame.hdr.dataSize;
								if( signalCnt == 0 )
								{
									/* nothing to do */
									break;
								}
								else if( signalCnt > sizeof(rxFrame.data)/sizeof(u_int32) )
								{	
					   	    		DBGWRT_ERR(( DBH, "%s%s: signalCnt %d %s%d%s", errStart, functionName, signalCnt, errLine, __LINE__, errEnd ));
									devHdl->state = MIPIOS_STATE_STOP;
									lineHdl->state = MIPIOS_STATE_STOP;  /* line fallback to stop */
									break;
								}
								else if( ((signalCnt-1) * sizeof(u_int32)) != rxFrame.hdr.dataSize  )
								{	
					   	    		DBGWRT_ERR(( DBH, "%s%s: signalCnt %d dataSize %d %s%d%s", errStart, functionName, signalCnt, errLine, __LINE__, errEnd ));
									devHdl->state = MIPIOS_STATE_STOP;
									lineHdl->state = MIPIOS_STATE_STOP;  /* line fallback to stop */
									break;
								}

								signalP	= (u_int32*) rxFrame.data;  /* will force problems on MIPS/ARM due to missalignment */
								for( idx = 0; idx < signalCnt-1; idx++ )
								{
									MIPIOS_SIG_Signal( rxFrame.hdr.linkNumber, 
													   rxFrame.hdr.devFrameNbr,
													   rxFrame.hdr.retryCnt,
													   *signalP );
									taskDelay( 1 ) ;/*/* should be removed */
									signalP++;
								}/*while*/
							}/*for*/
						}/*if*/
							
						break;
						
					case MIPIOS_STATE_CONFIGURATION:
						if( devHdl->linkState == MIPIOS_LINK_CONNECT ) /* we know the dev name and setup link and frame number now */
						{
							MIPIOS_CMD_ANNOUNCE_DEV_NAME_STRUCT *announceDevDataP;

							bzero( (char*)&txFrame, sizeof(txFrame) );		/* can size optimized */
							txFrame.hdr.linkNumber		= devHdl->linkNumber;
							txFrame.hdr.devFrameNbr		= devHdl->frameNumber;
							txFrame.hdr.retryCnt		= 0;
							txFrame.hdr.magic			= MIPIOS_MAGIC;
							txFrame.hdr.version			= MIPIOS_VERSION_1;
							
							ip   = devHdl->targetIp;
							port = devHdl->targetReceivePort;

							/* establish link */
							txFrame.hdr.type			= MIPIOS_TYPE_CONFIGURATION;
							txFrame.hdr.command			= MIPIOS_CMD_ESTABLISH_LINK | MIPIOS_TYPE_RESPONSE_REQ_MASK;
							
							announceDevDataP = (MIPIOS_CMD_ANNOUNCE_DEV_NAME_STRUCT *)txFrame.data;
							announceDevDataP->ipPort.ipType = MIPIOS_IP_TYPE_V4;
							/* target RX IP && port */
						  #ifdef __BIG_ENDIAN__
						    *(u_int32*)&announceDevDataP->ipPort.ipAddr[0] = OSS_SWAP32( ip );  /* be careful on NIOS may be not aligned */
							announceDevDataP->ipPort.port[0]   =  ( port & 0x00FF );
							announceDevDataP->ipPort.port[1]   = (( port & 0xFF00 ) >> 8 );
				  		  #else
				    		*(u_int32*)&announceDevDataP->ipPort.ipAddr[0] = ip;  /* be careful on NIOS may be not aligned */
							announceDevDataP->ipPort.port[0]   = (( port & 0xFF00 ) >> 8 );
							announceDevDataP->ipPort.port[1]   =  ( port & 0x00FF );
				  		  #endif

							/* target devName / data size / crc */
							sprintf( announceDevDataP->devName, devHdl->name );

					 		txFrame.hdr.dataSize = sizeof(MIPIOS_IP_PORT_DATA) + strlen( devHdl->name );

							/* send/receive with retry */
							error = MIPIOS_LM_SendReceive( lineHdl, devHdl, ip, port, &txFrame, &rxIp, &rxFrame );
							if( error )
							{
				   	    		DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_SendReceive() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
								devHdl->linkState = MIPIOS_LINK_QUIET;
							}
							else
							{
								devHdl->linkState = MIPIOS_LINK_ESTABLISHED;
								devHdl->state	  = MIPIOS_STATE_CONFIGURATION;
					   	    	DBGWRT_ERR(( DBH, "%s: %s has link %08x\n", functionName, devHdl->name, devHdl->linkNumber ));
							}
						}
						break;		
				}/*switch*/
			}/*for*/
		}
		goto AUTOCONNECT_CHECK;
		
/*-------------------------------------------------------------------------+ 
|  check if there are rx frames i.e. from target autoconfig                |
+-------------------------------------------------------------------------*/
AUTOCONNECT_CHECK:
		/* check the extra buffer */
		error = GetFrameFromBuffer( &lineHdl->rxAutoConnectFrameHdl, &rxFrame, &ip, 0/* no wait */ ); /* empty the buffer always */
		if( error )
		{
			if( error != ERR_MIPIOS_NO_BUF ) /* internal error */
			{
	   	    	DBGWRT_ERR(( DBH, "%s%s: GetFrameFromBuffer() %s %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, errLine, __LINE__, errEnd ));
			}
			continue; 
		}
		else
		{
			char *devName;
			typ = rxFrame.hdr.type;
			cmd = rxFrame.hdr.command;

			/* check we got an autoconnect packet */
			if( ( typ != MIPIOS_TYPE_DEV_NAME)
				|| (( cmd & ~MIPIOS_TYPE_RESPONSE_REQ_MASK) != MIPIOS_CMD_ANNOUNCE_DEV_NAME) )
			{
	   	    	DBGWRT_ERR(( DBH, "%s%s: %s ip %d.%d.%d.%d typ/cmd %02x/%02x %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, _IP_ARR(ipArr), typ, cmd,
   	    				errLine, __LINE__, errEnd ));

				lineHdl->errCnt.unknown++;
				continue; 
			}
			else
			{
				MIPIOS_CMD_ANNOUNCE_DEV_NAME_STRUCT	*announceDevDataP;

				announceDevDataP = (MIPIOS_CMD_ANNOUNCE_DEV_NAME_STRUCT *)&rxFrame.data;
				port = _SWAP_16_CPUE( ((announceDevDataP->ipPort.port[0] << 8 )| announceDevDataP->ipPort.port[1]) );

				announceDevDataP->devName[MIPIOS_MAX_DEV_NAME] = 0; /* for safety reasons */
				devName = announceDevDataP->devName;

				/* sanity checks IPV4 and IP */
				if(	announceDevDataP->ipPort.ipType != MIPIOS_IP_TYPE_V4
					|| *((u_int32*)announceDevDataP->ipPort.ipAddr) != _SWAP_32_CPUE(ip)
					|| strlen( devName ) == 0
				  )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s ip %d.%d.%d.%d typ/cmd %02x/%02x %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, _IP_ARR(ipArr), typ, cmd,
   	    				errLine, __LINE__, errEnd ));
					lineHdl->errCnt.unknown++;
					continue; 
				}

				if( lineHdl->state != MIPIOS_STATE_CONFIGURATION )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s ip %d.%d.%d.%d typ/cmd %02x/%02x wrong state %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, _IP_ARR(ipArr), typ, cmd,
   	    				errLine, __LINE__, errEnd ));
					continue; /* ignore if not in configuration state */
				}

       			DBGWRT_1(( DBH, "%s: autoconnect target(%d.%d.%d.%d/%d): %s frame# %d\n", functionName, _IP_ARR(ipArr), port, devName, rxFrame.hdr.devFrameNbr ));

				/* put it into MIPIOS/MDIS */
				devHdl = NULL;
				error = MIPIOS_DevCreateByDiscover( devName, lineHdl->lineName, &devHdl );
				if( error == ERR_MIPIOS_ALREADY_INIT )
				{
					/* new link number, if it already exists */
					error =	MIPIOS_DevIncreaseLinkNumber( devHdl );
					if( error )
					{
			   	    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_DevIncreaseLinkNumber() %s %s%d%s", errStart, functionName, 
   		    				lineHdl->lineName, devName,
   	    					errLine, __LINE__, errEnd ));
   	    				goto CLEANUP;
					}
				}
				else if( error )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_DevCreateByDiscover() %s %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, devName,
   	    				errLine, __LINE__, errEnd ));
					continue; 
				}


				if( devHdl == NULL )
				{
		   	    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_DevCreateByDiscover() %s no devHdl %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, devName,
   	    				errLine, __LINE__, errEnd ));
					continue; 
				}

				devHdl->linkState 	= MIPIOS_LINK_CONNECT; /* force a ESTABLISH_LINK to the target */
				devHdl->frameNumber	= rxFrame.hdr.devFrameNbr + 100; /* for ESTABLISH_LINK frame number */

				/* store device ip/port */
				devHdl->targetIp 			= ip;
				devHdl->targetReceivePort	= port;

				nothingDoneLastLoop = 0; /* there was an interesting thing for us */
			}
		}
	}/*for*/

CLEANUP:
	printf("*** %s %s internal error\n", functionName, lineHdl->lineName );
   	DBGWRT_ERR(( DBH, "%s%s: %s internal error line %s%d%s", errStart, functionName, 
   	    				lineHdl->lineName, errLine, __LINE__, errEnd ));
   	taskDelay( sysClkRateGet()/3 );
   	return( ERROR );
}




/**********************************************************************/
/** MIPIOS line initialization routine
 *
 * LineInit() finishs the initialization of a MIPIOS line.  
 *
 * \param lineHdl	\IN line handle
 *
 * \return the device handle (>= 0)  on success or \c -1 on error. 
 */
static int32 LineInit( MIPIOS_LINE *lineHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error = 0;
	char	*lineName;
	int 	taskId;

	lineName = lineHdl->lineName;
    DBGWRT_1((DBH, "%s %s\n", functionName, lineName ));

	/* line semaphore */
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1, &lineHdl->lineSem );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* tx semaphore */
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 0, &lineHdl->txSem );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* rx frame - empty - semaphore is free */
	lineHdl->rxFrameHdl.empty = 1;
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1, &lineHdl->rxFrameHdl.semBuffAccess );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 0, &lineHdl->rxFrameHdl.semBuffFull );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* rx autoconnect frame  - empty - semaphore is free */
	lineHdl->rxAutoConnectFrameHdl.empty = 1;
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1, &lineHdl->rxAutoConnectFrameHdl.semBuffAccess );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 0, &lineHdl->rxAutoConnectFrameHdl.semBuffFull );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* rx output frame  - empty - semaphore is free */
	lineHdl->rxOutputFrameHdl.empty = 1;
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1, &lineHdl->rxOutputFrameHdl.semBuffAccess );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	error = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 0, &lineHdl->rxOutputFrameHdl.semBuffFull );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemCreate %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	/* line setting defaults */
	lineHdl->connectMode	= MIPIOS_TARGET_AUTO_CONNECT; /*/* MIPIOS_HOST_AUTO_CONNECT */
	lineHdl->state		 	= MIPIOS_STATE_CONFIGURATION;
	lineHdl->maxRetry		= 2;
	lineHdl->timeoutMs		= 5000;
	lineHdl->timeoutTicks	= sysClkRateGet() * lineHdl->timeoutMs/1000;
	if( lineHdl->timeoutTicks == 0 )
		lineHdl->timeoutTicks = 1; /* at least one tick */
	
	lineHdl->hostRxPort = MIPIOS_HOST_RX_PORT_DEFAULT;
	lineHdl->fhRxSocket = -1;
	lineHdl->fhTxSocket = -1;

	if( lineHdl->lineDesc == NULL )
	{
        DBGWRT_ERR(( DBH, "INFO: %s no desc - using defaults for %s\n", functionName, lineName ));
	}
	else
	{
		/*/**/
        DBGWRT_ERR(( DBH, "%s%s: DESC handling not implemented for %s %s%d%s", warnStart, functionName, lineName, errLine, __LINE__, errEnd ));
		/* line settings from descriptor */
	}		

	taskId = taskNameToId( lineHdl->lineName );
	if( taskId != ERROR )
	{
		error = ERR_MIPIOS_ALREADY_INIT;
        DBGWRT_ERR(( DBH, "%s%s: task %s already running %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	error = MIPIOS_COM_RxInit( lineHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_COM_RxInit %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	error = MIPIOS_COM_TxInit( lineHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_COM_TxInit %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}


	/* spawn line task */
	lineHdl->lineTaskId = taskSpawn( lineHdl->lineName, 		/* task name equal to line name 	*/
									MIPIOS_LM_TASK_PRIORITY, 	/* task priority 	*/
									0,        					/* options 			*/
									0x8000,   					/* 32kB stack 		*/
									LineTask, 					/* function 		*/
									(int)lineHdl,				/* arg1 			*/
									0,0,0,0,0,0,0,0,0 );
	if( lineHdl->lineTaskId == ERROR )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: taskSpawn %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* check if task is running */
	taskDelay( 10 ); /* give time to start the task */
	taskId = taskNameToId( lineHdl->lineName );
	if( taskId != lineHdl->lineTaskId )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: %s not running %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	return( error );
}


/**********************************************************************/
/** MIPIOS line initialization routine
 *
 * MIPIOS_LM_LineInit() initialize a MIPIOS line.  
 *
 * \param lineName	\IN name of the line e.g. "line_A"
 *
 * \return the device handle (>= 0)  on success or \c -1 on error. 
 */
int32 MIPIOS_LM_LineInit( char *lineName, DESC_SPEC *lineDesc )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = 0;
	MIPIOS_LINE *lineHdl = NULL;

    DBGWRT_1((DBH, "%s %s %08x\n", functionName, lineName, lineDesc ));

	/* check if system is initialized */
	if( !LM_initialized_G )
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
	
	/* alloc line */
	error = AllocLine( lineName, lineDesc, &lineHdl );
	if( lineHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: AllocLine %s %s%d%s", errStart, functionName, lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		
	
	error = LineInit( lineHdl );
	if( error )
	{
		FreeLine( lineName );
        DBGWRT_ERR(( DBH, "%s%s: LineInit %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	
CLEANUP:
	return( error );
}

/**********************************************************************/
/** MIPIOS line/system start configuration routine
 *
 * MIPIOS_LM_StartConfiguration() starts a line or the complete system to configuration
 * state. If lineName is 0 all lines will be switched to operational.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_StartConfiguration( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS;
	MIPIOS_LINE 	*lineHdl = &MIPIOS_LineTbl[0];

    DBGWRT_1(( DBH, "%s( %s )\n", functionName, lineName ? lineName : "0 - whole system" ));

	if( lineHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no lineHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( MIPIOS_LineTbl[0].state != MIPIOS_STATE_STOP )
	{
        DBGWRT_ERR(( DBH, "%s%s: line state %d %s%d%s", errStart, functionName, MIPIOS_LineTbl[0].state, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	lineHdl->state = MIPIOS_STATE_CONFIGURATION; /*/**/

	error = 0;

CLEANUP:
	return( error );
}



/**********************************************************************/
/** MIPIOS line/system start operational routine
 *
 * MIPIOS_LM_StartOperational() starts a line or the complete system to operational
 * state. If lineName is 0 all lines will be switched to operational.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_StartOperational( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS;
	int devIdx;
	MIPIOS_LINE 	*lineHdl = &MIPIOS_LineTbl[0];
	MIPIOS_DEVICE   *devHdl;

    DBGWRT_1(( DBH, "%s( %s )\n", functionName, lineName ? lineName : "0 - whole system" ));

	if( lineHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no lineHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( lineHdl->state != MIPIOS_STATE_CONFIGURATION )
	{
        DBGWRT_ERR(( DBH, "%s%s: wrong state %d %s%d%s", errStart, functionName, lineHdl->state, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}	

	/* switch to operational */
	lineHdl->state = MIPIOS_STATE_OPERATIONAL;	

	/* delay a bit */
	taskDelay( (lineHdl->timeoutTicks + 1 ) );
	
	if( MIPIOS_LineTbl[0].state != MIPIOS_STATE_OPERATIONAL )
	{
        DBGWRT_ERR(( DBH, "%s%s: line state %d %s%d%s", errStart, functionName, MIPIOS_LineTbl[0].state, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	/* check if each device on the line is in operational state */
	for( devIdx=0; devIdx<MIPIOS_MAX_DEVS_PER_LINE; devIdx++ )
	{
		devHdl = lineHdl->devTbl[devIdx];

		if( devHdl == NULL )
			continue; /* check next dev */

		if( devHdl->mipiosLineP != lineHdl )
		{
   	    	DBGWRT_ERR(( DBH, "%s%s: %s dev[%d] line %08x/%08x mismatch %s%d%s", errStart, functionName, 
    				lineHdl->lineName, devIdx, devHdl->mipiosLineP, lineHdl,
    				errLine, __LINE__, errEnd ));
			goto CLEANUP;
		}

		if( devHdl->linkState != MIPIOS_LINK_ESTABLISHED )
			continue;  /*/* for test only - jump over /mipios/mt/0 */

		if( devHdl->state != MIPIOS_STATE_OPERATIONAL )
		{
   	    	DBGWRT_ERR(( DBH, "%s%s: %s dev[%d] wrong state %d %s%d%s", errStart, functionName, 
    				lineHdl->lineName, devIdx, devHdl->state,
    				errLine, __LINE__, errEnd ));
			goto CLEANUP;
		}
	}/*for*/

	error = 0;

CLEANUP:	
	return( error );
}

/**********************************************************************/
/** MIPIOS line stop routine
 *
 * MIPIOS_LM_Stop() starts a line or the complete system to stop
 * state. If lineName is 0 all lines will be switched to stop.
 *
 * \param lineName	\IN 0 or line name
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_Stop( char *lineName )
{
  	static const char *functionName = __FUNCTION__;
	int32 error = ERR_MIPIOS;

    DBGWRT_1(( DBH, "%s( %s )\n", functionName, lineName ? lineName : "0 - whole system" ));

	
	
	return( error );
}


/**********************************************************************/
/** MIPIOS line line manager transceive.
 *
 * MIPIOS_LM_Transceive() .
 *
 * \param devHdl	\IN device handle
 *
 * \return 0 or error code 
 */
static int32 MIPIOS_LM_Transceive
(
	MIPIOS_DEVICE 		*devHdl,
	MIPIOS_FRAME		*txFrameP, 
	char  				*txDataP, 
	int32 				txSize, 
	char  				*txData2P, 
	int32 				txSize2, 
	char  				*rxDataP, 
	int32 				*rxSizeP,
	int32 				*returnCodeP,
	int32 				*errorNoP
)
{
  	static const char *functionName = __FUNCTION__;
	int32 			error;
	int32 			error2 = 0;
	int32 			rxIp;
	MIPIOS_FRAME	rxFrame;
	int				lineLocked = 0;
	MIPIOS_LINE		*lineHdl	   = devHdl->mipiosLineP;
	
	*returnCodeP = -1;

	
	/* parameter checking */
	if( txSize+txSize2 > MIPIOS_MAX_DATA_SIZE )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: txSize/txSize2 %d/%d %s%d%s", errStart, functionName, txSize, txSize2, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( (rxDataP == 0 && *rxSizeP != 0)
		|| (rxDataP != 0 && *rxSizeP == 0)
	  )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: rxDataP/*rxSizeP %08x/%d %s%d%s", errStart, functionName, rxDataP, *rxSizeP, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	
	/* prepare common frame data */
	txFrameP->hdr.dataSize	 = txSize + txSize2;		/* FRAME: setup size */
	memcpy( txFrameP->data, txDataP, txSize );			/* FRAME: setup data */ /* can size optimized */

	if( txData2P && txSize2 )
	{
		memcpy( txFrameP->data+txSize, txData2P, txSize2 );			/* FRAME: setup data */ /* can size optimized */
	}
	
	/* lock line */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, lineHdl->lineSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait line/dev %s/%s %s%d%s", errStart, functionName, 
        			lineHdl->lineName, devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	lineLocked = 1;
	
	/*-------------------------------------------------------+
	| update txFrame                                         |
	+-------------------------------------------------------*/
	{

		/* lock critical section */
		if( taskLock() )
		{
			error = ERR_MIPIOS;
	        DBGWRT_ERR(( DBH, "%s%s: taskLock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}

		memcpy( &lineHdl->txFrame, txFrameP, sizeof(MIPIOS_FRAME) ); /* can size optimized */
		lineHdl->currDevHdl = devHdl;

		/* unlock critical section */
		if( taskUnlock() )
		{
			error = ERR_MIPIOS;
	        DBGWRT_ERR(( DBH, "%s%s: taskUnlock %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
    	    goto CLEANUP;
		}
	}

	/*-----------------------------------------+
	| switch to line task by unlocking TX sem  |
	+-----------------------------------------*/
	error = OSS_SemSignal( OSS_VXWORKS_OS_HDL, lineHdl->txSem );
	if( error )
	{
	    DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait line/dev %s/%s %s%d%s", errStart, functionName, 
    				lineHdl->lineName, devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}


	/*--------------------------------------------+
	|  wait for input buffer                      |
	+--------------------------------------------*/
	error = GetFrameFromBuffer( &lineHdl->rxOutputFrameHdl, &rxFrame, &rxIp, (lineHdl->timeoutTicks * 3) + 1 ); 
	if( error )
	{
    	DBGWRT_ERR(( DBH, "%s%s: %s GetFrameFromBuffer() %s%d%s", errStart, functionName, 
					lineHdl->lineName, errLine, __LINE__, errEnd ));
	}
	else
	{	
		if( rxFrame.hdr.dataSize > *rxSizeP )
		{
			/* we have more data than we have space in buffer */
			error = ERR_MIPIOS_ILL_PARAM;
	    	DBGWRT_ERR(( DBH, "%s%s: %s dataSize %d - buffer space %d %s%d%s", errStart, functionName, 
					lineHdl->lineName, rxFrame.hdr.dataSize, *rxSizeP, errLine, __LINE__, errEnd ));
			goto CLEANUP;
		}
		else if( rxFrame.hdr.dataSize )
		{
			/* there is something in the buffer - endian swapping will be done in the caller if possible */
			memcpy( rxDataP, rxFrame.data, rxFrame.hdr.dataSize );
		}/*if*/

		*rxSizeP     = rxFrame.hdr.dataSize;
		*returnCodeP = rxFrame.hdr.returnCode;
		*errorNoP    = rxFrame.hdr.errorNo;

		/* check if we have special signal command return */
		if( ( rxFrame.hdr.command == MIPIOS_CMD_M_SETSTAT_SIG_INSTALLED || rxFrame.hdr.command == MIPIOS_CMD_M_SETSTAT_SIG_REMOVED )
				&& rxFrame.hdr.type == MIPIOS_TYPE_DATA
				&&     *returnCodeP == ERR_SUCCESS 
		  )
		{
			u_int32 linkNbr		= rxFrame.hdr.linkNumber;
			u_int32 devFrameNbr	= rxFrame.hdr.devFrameNbr;
			u_int32 retryCnt	= rxFrame.hdr.retryCnt;
			u_int32 tgtSigHdl;
			u_int32 sigNo;

			tgtSigHdl	= *((u_int32*)rxFrame.data);
			sigNo		= *((u_int32*)(rxFrame.data+4));
			
			if( rxFrame.hdr.command == MIPIOS_CMD_M_SETSTAT_SIG_INSTALLED )
			{
				error = MIPIOS_SIG_Install( linkNbr, devFrameNbr, retryCnt, tgtSigHdl, sigNo );
				if( error )
				{
			    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_SIG_Install() %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
					goto CLEANUP;
				}
				else
				{
					devHdl->installedSignalCount++;
				}/*if*/
			}
			else
			{
				error = MIPIOS_SIG_Remove( linkNbr, devFrameNbr, retryCnt, tgtSigHdl, sigNo );
				if( error )
				{
			    	DBGWRT_ERR(( DBH, "%s%s: %s MIPIOS_SIG_Remove() %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
					goto CLEANUP;
				}
				else
				{
					devHdl->installedSignalCount--;
				}/*if*/
			}/*if*/
		}/*if*/
	}/*if*/
	
CLEANUP:
	/* unlock line */
	if( lineLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, lineHdl->lineSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait line/dev %s/%s %s%d%s", errStart, functionName, 
        			lineHdl->lineName, devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Opens a MIPIOS device.
 *
 * MIPIOS_LM_Open() open device. 
 * Will be called by M_open for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param fileHandleP	\IN  -1 or opened file handle
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_Open( MIPIOS_DEVICE *devHdl, int32 *fileHandleP )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	char				*devName;
	int32				devNameSize;
	int32				rxSize = 0;
	int32 				error2 = 0;
	int					devLocked  = 0;

	*fileHandleP = -1;

	/* validate */
	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	devName 	= devHdl->name;
	devNameSize = MIPIOS_LM_StrLenMax( devName, MIPIOS_MAX_LINE_NAME );

	/*-------------------------------------------------------------+
	|  at first open: pump the low level descriptior if available  |
	+-------------------------------------------------------------*/
	if( devHdl->openCount == 0 )
	{
		/* check if low level descriptor exists */
		if( devHdl->llDesc )
		{
			DESC_HANDLE *descHdl = NULL;
			int32 descSize;

			/* compute descriptor size */
			error = DESC_Init( devHdl->llDesc, OSS_VXWORKS_OS_HDL, &descHdl );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: DESC_Init() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}

			error = DESC_GetDescSize( descHdl, &descSize );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: DESC_GetDescSize() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}

			DESC_Exit( &descHdl	);

			/* descriptor in binary big ??? little endian format */
			bzero( (char*)&txFrame, sizeof(txFrame) );


			txFrame.hdr.command		= MIPIOS_CMD_SET_LL_DESC | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

			/* check if one frame is enough */		
			if( (devNameSize + 1 + descSize) <= MIPIOS_MAX_DATA_SIZE )
			{
				txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */

				/* send / receive frame */
				rxSize = 0;
				error = MIPIOS_LM_Transceive( devHdl, &txFrame, 
											  (u_int8*)devName/*data*/,        devNameSize + 1/*size including EOS*/,
											  (u_int8*)devHdl->llDesc/*data*/, descSize/*size*/,
											  NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
				if( error )
				{
			        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}
			}
			else
			{
				/* compute the numbers of byte of the desc transmitted by the first transfer */
				int alreadyTransmitted = MIPIOS_MAX_DATA_SIZE - (devNameSize + 1);

				/*-------------------------+
				| MULTI frame === START TX |
				+-------------------------*/
				txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_BEGIN;		/* FRAME: setup typ */

				/* send / receive frame */
				rxSize = 0;
				error = MIPIOS_LM_Transceive( devHdl, &txFrame, 
											  (u_int8*)devName/*data*/,        devNameSize + 1/*size including EOS*/,
											  (u_int8*)devHdl->llDesc/*data*/, alreadyTransmitted/*size*/,
											  NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
				if( error )
				{
			        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}

				/*------------------------+
				| MULTI frame === MID TX  |
				+------------------------*/
				while( (descSize - alreadyTransmitted) > MIPIOS_MAX_DATA_SIZE )
				{
					txFrame.hdr.type = MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
					rxSize = 0;
					error = MIPIOS_LM_Transceive( devHdl, &txFrame, 
												  ((u_int8*)devHdl->llDesc) + alreadyTransmitted/*data*/, MIPIOS_MAX_DATA_SIZE,
												  NULL/*data*/, 0/*size*/,
												  NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
					if( error )
					{
				        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
				        goto CLEANUP;
					}
					alreadyTransmitted += MIPIOS_MAX_DATA_SIZE;
					
				}/*while*/
				
				/*--------------------------+
				| MULTI frame === END TX/RX |
				+--------------------------*/
				txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_END;	/* FRAME: setup typ */
		
				error = MIPIOS_LM_Transceive( devHdl, &txFrame, 
											  ((u_int8*)devHdl->llDesc) + alreadyTransmitted/*data*/, (descSize - alreadyTransmitted),
											  NULL/*data*/, 0/*size*/,
											  NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
				if( error )
				{
			        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}
				alreadyTransmitted += (descSize - alreadyTransmitted);
				
			}
		}
		else
		{
	        DBGWRT_ERR(( DBH, "INFO: %s no desc - using defaults for %s\n", functionName, devName ));
		}
	}


	/*-------------------------+
	| M_open                   |
	+-------------------------*/
	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_OPEN | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	rxSize = 0;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, devName/*data*/, devNameSize/*size*/, NULL, 0, NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	*fileHandleP = returnCode;
	if( returnCode < 0 )
	{
		error = errorNo; /* map target errno */
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}


/**********************************************************************/
/** Closes a MIPIOS device path.
 *
 * MIPIOS_LM_Close() closes a device path. 
 * Will be called by M_close for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_Close( MIPIOS_DEVICE *devHdl, int32 targetPath )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;


	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_CLOSE | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	rxSize = MIPIOS_DATA_SIZE_0;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)&targetPath/*data*/, MIPIOS_DATA_SIZE_4/*size*/, NULL, 0, NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	error = returnCode;
	if( error )
	{
		error = errorNo; /* map target errno */
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Writes to MIPIOS device path.
 *
 * MIPIOS_LM_Write() writes a  value to device path. 
 * Will be called by M_write for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 * \param value			\IN  value to write
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_Write( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 value )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32				data[2];
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_WRITE | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	data[0] = targetPath;
	data[1] = value;
	rxSize = 0;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)data, MIPIOS_DATA_SIZE_8, NULL, 0, NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	error = returnCode;
	if( error )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Reads from MIPIOS device path.
 *
 * MIPIOS_LM_Read() reads a  value from device path. 
 * Will be called by M_read for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 * \param valueP		\IN  address where read value will be stored
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_Read( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 *valueP )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_READ | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	rxSize = MIPIOS_DATA_SIZE_4;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)&targetPath, MIPIOS_DATA_SIZE_4, NULL, 0, (u_int8*)valueP, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	error = returnCode;
	if( error )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Setstat to MIPIOS device path.
 *
 * MIPIOS_LM_SetStat() setstat code/value to device path. 
 * Will be called by M_setstat for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 * \param code			\IN  setstat code 
 * \param value			\IN  setstat value
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_SetStat( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 code, int32 value )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32				txSize;
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */


	/* check if block set/getstat */
	if( 	(M_MK_BLK_OF 	<= code  &&  code <= M_MK_BLK_NOTEXIST)
		||	(M_LL_BLK_OF 	<= code  &&  code <= M_LL_BLK_NOTEXIST)
		||	(M_DEV_BLK_OF 	<= code  &&  code <= M_DEV_BLK_NOTEXIST)
		||	(M_BB_BLK_OF  	<= code  &&  code <= M_BB_BLK_NOTEXIST)
		||	(M_BRD_BLK_OF 	<= code  &&  code <= M_BRD_BLK_NOTEXIST)
	  )
	{
		int32				txData[MIPIOS_MAX_DATA_SIZE/4]; /*/**/
		M_SETGETSTAT_BLOCK  *sgBlkP = (M_SETGETSTAT_BLOCK *)value;
		u_int8				*toTransmitP;
		int32				toTransmitSize;

		txFrame.hdr.command		= MIPIOS_CMD_M_SETSTAT_BLK | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

		/* setup path, code, block size , tx/rx pointer */
		txData[0] = targetPath;
		txData[1] = code;
		txData[2] = sgBlkP->size;
		txData[3] = (int32)NULL;
		toTransmitSize	= sgBlkP->size;
		toTransmitP		= sgBlkP->data;

		txFrame.hdr.command		= MIPIOS_CMD_M_SETSTAT_BLK | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

		if( (MIPIOS_DATA_SIZE_16 + sgBlkP->size) > MIPIOS_MAX_DATA_SIZE   )
		{
			error = ERR_MIPIOS_NO_SPACE;
	        DBGWRT_ERR(( DBH, "%s%s: no mem %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
		else
		{
			/*------------------------------+
			| not fragmented block setstat  |
			+------------------------------*/
			memcpy( &txData[4], toTransmitP, sgBlkP->size );	/* getstat block tx data */
			
			txSize = MIPIOS_DATA_SIZE_16 + sgBlkP->size;
			rxSize = 0;
			error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)0, &rxSize, &returnCode, &errorNo );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
		}/*if*/
	}
	else
	{		
		/*----------------+
		| normal setstat  |
		+----------------*/
		int32				txData[3];
		int32				rxData[2]; /* special space for signal install/remove setstat */

		txFrame.hdr.command		= MIPIOS_CMD_M_SETSTAT | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

		/* send / receive frame */
		txData[0] = targetPath;
		txData[1] = code;
		txData[2] = value;
		rxSize = sizeof(rxData);
		error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, MIPIOS_DATA_SIZE_12, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
		if( error )
		{
	        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}		
	}/*if*/

	error = returnCode;
	if( error )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Getstat to MIPIOS device path.
 *
 * MIPIOS_LM_GetStat() Getstat code/value to device path. 
 * Will be called by M_getstat for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 * \param code			\IN  setstat code 
 * \param dataP			\IN  pointer to getstat value
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_GetStat( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 code, int32 *dataP )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32				rxData;
	int32				data[2];
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */


	/*----------------------------+
	| check if block set/getstat  |
	+-----------------------------*/
	if( 	(M_MK_BLK_OF 	<= code  &&  code <= M_MK_BLK_NOTEXIST)
		||	(M_LL_BLK_OF 	<= code  &&  code <= M_LL_BLK_NOTEXIST)
		||	(M_DEV_BLK_OF 	<= code  &&  code <= M_DEV_BLK_NOTEXIST)
		||	(M_BB_BLK_OF  	<= code  &&  code <= M_BB_BLK_NOTEXIST)
		||	(M_BRD_BLK_OF 	<= code  &&  code <= M_BRD_BLK_NOTEXIST)
	  )
	{
		int32				txSize;
		int32				txData[MIPIOS_MAX_DATA_SIZE/4]; /*/**/
		int32				rxData[MIPIOS_MAX_DATA_SIZE/4];
		M_SETGETSTAT_BLOCK  *sgBlkP = (M_SETGETSTAT_BLOCK *) dataP;
		u_int8				*toTransmitP;
		u_int8				*toReceiveP;
		int32				toTransmitSize;
		int32				toReceiveSize;

		txFrame.hdr.command		= MIPIOS_CMD_M_GETSTAT_BLK | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

		/* setup path, code, block size , tx/rx pointer */
		txData[0] = targetPath;
		txData[1] = code;
		txData[2] = sgBlkP->size;
		txData[3] = (int32)NULL;
		toTransmitSize	= sgBlkP->size;
		toReceiveSize	= sgBlkP->size;
		toTransmitP		= sgBlkP->data;
		toReceiveP  	= sgBlkP->data;
		
		if( (MIPIOS_DATA_SIZE_16 + sgBlkP->size) > MIPIOS_MAX_DATA_SIZE   )
		{
			/*-------------------------+
			| MULTI frame === START TX |
			+-------------------------*/
			txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_BEGIN;		/* FRAME: setup typ */
			memcpy( &txData[4], toTransmitP, MIPIOS_MAX_DATA_SIZE-MIPIOS_DATA_SIZE_16 );	/* set/getstat block tx data */
			txSize = MIPIOS_MAX_DATA_SIZE;
			rxSize = sizeof(rxData);
			error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, MIPIOS_MAX_DATA_SIZE, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
			if( rxSize != 0 )
			{
				error = ERR_MIPIOS;
		        DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
			/* compute data/size still to transmit */
			toTransmitP    += MIPIOS_MAX_DATA_SIZE - MIPIOS_DATA_SIZE_12;
			toTransmitSize -= MIPIOS_MAX_DATA_SIZE - MIPIOS_DATA_SIZE_12;

			/*------------------------+
			| MULTI frame === MID TX  |
			+------------------------*/
			while( toTransmitSize > MIPIOS_MAX_DATA_SIZE )
			{
				txFrame.hdr.type = MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
				memcpy( &txData[0], toTransmitP, MIPIOS_MAX_DATA_SIZE );	/* set/getstat block tx data */
				txSize = MIPIOS_MAX_DATA_SIZE;
				rxSize = MIPIOS_MAX_DATA_SIZE;
				error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
				if( error )
				{
			        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}/*if*/
				if( rxSize != 0 )
				{
					error = ERR_MIPIOS;
			        DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}/*if*/
				/* compute data/size still to transmit */
				toTransmitP    += MIPIOS_MAX_DATA_SIZE;
				toTransmitSize -= MIPIOS_MAX_DATA_SIZE;
			}

			/*--------------------------+
			| MULTI frame === END TX/RX |
			+--------------------------*/
			txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_END;		/* FRAME: setup typ */
			memcpy( &txData[0], toTransmitP, toTransmitSize );	/* set/getstat block tx data */
			txSize = toTransmitSize;
			rxSize = sizeof(rxData);
			error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
			if( rxSize )
			{
				memcpy( toReceiveP, rxData, rxSize );	/* set/getstat block rx data */
				toReceiveP    += rxSize;
				toReceiveSize -= rxSize;
			}
			else
			{
				error = ERR_MIPIOS;
		        DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
			toTransmitP += toTransmitSize;
			
			/* sanity check */
			if( toTransmitP != (u_int8*)sgBlkP->data + sgBlkP->size )
			{
				error = ERR_MIPIOS;
		        DBGWRT_ERR(( DBH, "%s%s: toTransmitP %08x %s%d%s", errStart, functionName, toTransmitP, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}
			
			/*-----------------------+
			| MULTI frame === MID RX |
			+-----------------------*/
			while( toReceiveSize > MIPIOS_MAX_DATA_SIZE )
			{
				txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_RX;		/* FRAME: setup typ */

				txSize = 0;
				rxSize = sizeof(rxData);
				error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
				if( error )
				{
			        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}/*if*/
				if( rxSize )
				{
					memcpy( toReceiveP, rxData, rxSize );	/* set/getstat block rx data */
					toReceiveP    += rxSize;
					toReceiveSize -= rxSize;
				}
				else
				{
					error = ERR_MIPIOS;
			        DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
			        goto CLEANUP;
				}/*if*/
			}/*while*/
			
			/*-----------------------+
			| MULTI frame === END RX |
			+-----------------------*/
			txFrame.hdr.type = MIPIOS_TYPE_DATA_FRAG_RX_END;		/* FRAME: setup typ */
			txSize = 0;
			rxSize = sizeof(rxData);
			error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
			if( error )
			{
				DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
			    goto CLEANUP;
			}/*if*/
			if( rxSize == toReceiveSize )
			{
				memcpy( toReceiveP, rxData, rxSize );	/* set/getstat block rx data */
				toReceiveP    += rxSize;
				toReceiveSize -= rxSize;
			}
			else
			{
				error = ERR_MIPIOS;
			    DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
			    goto CLEANUP;
			}/*if*/
			
			/* sanity check */
			if( toReceiveP != (u_int8*)sgBlkP->data + sgBlkP->size )
			{
				error = ERR_MIPIOS;
		        DBGWRT_ERR(( DBH, "%s%s: toReceiveP %08x %s%d%s", errStart, functionName, toReceiveP, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}
		}
		else
		{
			/*------------------------------+
			| not fragmented block getstat  |
			+------------------------------*/
			memcpy( &txData[4], toTransmitP, sgBlkP->size );	/* getstat block tx data */
			
			txSize = MIPIOS_DATA_SIZE_16 + sgBlkP->size;
			rxSize = sgBlkP->size;
			error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)txData, txSize, NULL, 0, (char*)rxData, &rxSize, &returnCode, &errorNo );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		        goto CLEANUP;
			}/*if*/
	
			if( rxSize == 0 )
			{
				error = ERR_MIPIOS;
			    DBGWRT_ERR(( DBH, "%s%s: rxSize %d %s%d%s", errStart, functionName, rxSize, errLine, __LINE__, errEnd ));
			    goto CLEANUP;
			}/*if*/
			memcpy( toReceiveP, rxData, rxSize );	/* set/getstat block rx data */
		}/*if*/
	}
	else
	{	
		/*----------------+
		| normal getstat  |
		+----------------*/
		txFrame.hdr.command		= MIPIOS_CMD_M_GETSTAT | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */
	
		/* send / receive frame */
		data[0] = targetPath;
		data[1] = code;
		rxSize = 4;
		error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)data, MIPIOS_DATA_SIZE_8, NULL, 0, (u_int8*)&rxData, &rxSize, &returnCode, &errorNo );
		if( error )
		{
	        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}		
	
		if( rxSize == MIPIOS_DATA_SIZE_4 )
		{
			*dataP = rxData;
		}
		else
		{
			error = ERR_MIPIOS_ILL_PARAM;
	        DBGWRT_ERR(( DBH, "%s%s: block getstat not implemented %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
	        goto CLEANUP;
		}
	}/*if*/

	error  = returnCode;
	if( error )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

/**********************************************************************/
/** Writes to MIPIOS device path.
 *
 * MIPIOS_LM_Write() writes a  value to device path. 
 * Will be called by M_write for MIPIOS devices.
 *
 * \param devHdl		\IN  device handle
 * \param targetPath	\IN  target path
 * \param buffer			\IN  pointer to value buffer
 * \param length			\IN  lentgh of value buffer in byte
 * \param nbrWrittenBytesP	\IN  pointer where nbrWrittenBytes will be stored
 *
 * \return 0 or error code 
 */
int32 MIPIOS_LM_SetBlock( MIPIOS_DEVICE *devHdl, int32 targetPath, u_int8 *buffer, int32 length, int32 *nbrWrittenBytesP )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32				data[MIPIOS_MAX_DATA_SIZE/4 + 4/*safety area*/];
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	if( length < 0 || length >= (MIPIOS_MAX_DATA_SIZE - MIPIOS_DATA_SIZE_8 /*path/length*/) )
	{
        DBGWRT_ERR(( DBH, "%s%s: length %d %s%d%s", errStart, functionName, length, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_SET_BLK | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	data[0] = targetPath;
	data[1] = length;
	memcpy( &data[2], buffer, length );
	rxSize = 0;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)data, length + MIPIOS_DATA_SIZE_8, NULL, 0, NULL/*rxData*/, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	error = returnCode;
	if( error < 0 )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	else
	{
		error = 0; /*no error*/
		*nbrWrittenBytesP = returnCode;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

int32 MIPIOS_LM_GetBlock( MIPIOS_DEVICE *devHdl, int32 targetPath, u_int8 *buffer, int32 length, int32 *nbrReadBytesP )
{
  	static const char 	*functionName = __FUNCTION__;
	int32 				error = ERR_MIPIOS_ILL_PARAM;
	int32 				returnCode;
	int32 				errorNo;
	MIPIOS_FRAME		txFrame;
	int32				rxSize;
	int32				data[2];
	int32 				error2 = 0;
	int					devLocked  = 0;

	if( devHdl == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: no devHdl %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	if( length < 0 || length >= MIPIOS_MAX_DATA_SIZE )
	{
        DBGWRT_ERR(( DBH, "%s%s: length %d %s%d%s", errStart, functionName, length, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* lock device */
	error = OSS_SemWait( OSS_VXWORKS_OS_HDL, devHdl->devSem, OSS_SEM_WAITFOREVER );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() dev %s %s%d%s", errStart, functionName, 
        			 devHdl->name, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	devLocked = 1;

	bzero( (char*)&txFrame, sizeof(txFrame) );
	txFrame.hdr.type		= MIPIOS_TYPE_DATA;		/* FRAME: setup typ */
	txFrame.hdr.command		= MIPIOS_CMD_M_GET_BLK | MIPIOS_TYPE_RESPONSE_REQ_MASK;	/* FRAME: setup command */

	/* send / receive frame */
	data[0] = targetPath;
	data[1] = length;
	rxSize = length;
	error = MIPIOS_LM_Transceive( devHdl, &txFrame, (char*)data, MIPIOS_DATA_SIZE_8, NULL, 0, buffer, &rxSize, &returnCode, &errorNo );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: MIPIOS_LM_Transceive %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}		

	error = returnCode;
	if( error < 0 )
	{
		error = errorNo;
        DBGWRT_ERR(( DBH, "%s%s: returnCode %d %s%d%s", errStart, functionName, returnCode, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	else
	{
		error = 0; /*no error*/
		*nbrReadBytesP = returnCode;
	}

CLEANUP:
	/* unlock device */
	if( devLocked )
	{
		error2 = OSS_SemSignal( OSS_VXWORKS_OS_HDL, devHdl->devSem );
		if( error2 )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() dev %s %s%d%s", errStart, functionName, 
        			     devHdl->name, errLine, __LINE__, errEnd ));
		}
	}

	if( error )
		return( error );
	else
		return( error2 );
}

#endif /*INCLUDE_MIPIOS_VX*/

