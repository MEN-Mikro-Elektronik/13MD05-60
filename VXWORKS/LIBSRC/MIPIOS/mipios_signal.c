/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mipios_signal.c
 *
 *      \author  uf
 *        $Date: 2009/03/31 10:41:28 $
 *    $Revision: 1.2 $
 * 
 *       \brief  MIPIOS host signal handling for VxWorks.
 *
 * This file contains the MIPIOS signal handling for VxWorks.
 *
 *       \note  switches: DBG, INCLUDE_MIPIOS_VX
 *                        if not define MIPIOS_SIGNALS_MAX default is 0x100
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_signal.c,v $
 * Revision 1.2  2009/03/31 10:41:28  ufranke
 * changed
 *  - switch from EXCLUDE_MIPIOS_VX to INCLUDE_MIPIOS_VX
 *
 * Revision 1.1  2009/01/30 09:21:12  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
#ifdef INCLUDE_MIPIOS_VX

/* ================================ if MIPIOS should be integrated into MDIS==================================== */
const char *MIPIOS_SIG_RCSid="$Id: mipios_signal.c,v 1.2 2009/03/31 10:41:28 ufranke Exp $\n"
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

#include <vxWorks.h>

#include <in.h>		
#include <logLib.h>
#include <string.h>
#include <sysLib.h>
#include <taskLib.h>

#include <MEN/men_typs.h>   /* MEN type definitions      */
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

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct
{
	int32			full;
	int32			linkNbr;
	u_int32 		oldDevFrameNbr;
	u_int8 			oldRetryCnt;
	int32			tgtSigHdl;
	int32			sigNo;
	OSS_SIG_HANDLE	*ossSigHdl;
}MIPIOS_SIG_VX;


/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/
#ifndef MIPIOS_SIGNALS_MAX	
	#define MIPIOS_SIGNALS_MAX	0x100
#endif

#define _NEVER_SIGNALED 0xFF
/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/
static int 				notInitialized_G = ERR_MIPIOS_NOT_INITIALIZED;
static MIPIOS_SIG_VX	sigTbl_G[MIPIOS_SIGNALS_MAX];  /* target signal to host signal mapping - OPT: 32 tables with hash */

/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/

/**********************************************************************/
/** Checks if MIPIOS_SIG is initialized.
 *
 * CheckNotInitialized() checks if this module is initialized.  
 *
 * \return 0 or ERR_MIPIOS_NOT_INITIALIZED 
 */
static int32 CheckNotInitialized( void )
{
	return( notInitialized_G );
}


/**********************************************************************/
/** Initializes MIPIOS_SIG module if necessary.
 *
 * TryInitialize() initializes this module if necessary.  
 *
 * \return N/A
 */
static void TryInitialize( void )
{
	if( CheckNotInitialized() )
	{
		bzero( (char*)sigTbl_G, sizeof(sigTbl_G) );
		notInitialized_G = ERR_SUCCESS;
	}
}

/**********************************************************************/
/** Finds a MIPIOS signal.
 *
 * FindMipiosSignal() finds a MIPIOS signal.
 *
 * \param str	\IN linkNbr   link number of the target connection
 * \param str	\IN tgtSigHdl target signal handle
 *
 * \return 0 or error code
 */
static int32 FindMipiosSignal( u_int32 linkNbr, u_int32 tgtSigHdl, MIPIOS_SIG_VX **mipiosSigHdlP )
{
  	static const char *functionName = __FUNCTION__;
  	int32	error = ERR_MIPIOS_ILL_PARAM;
  	int		idx;

	for( idx = 0; idx < MIPIOS_SIGNALS_MAX; idx++ )
	{
		/* check if full and link number and target signal handle matches */
		if( sigTbl_G[idx].full 
			&& sigTbl_G[idx].linkNbr   == linkNbr
			&& sigTbl_G[idx].tgtSigHdl == tgtSigHdl
		  )
		{
			*mipiosSigHdlP = &sigTbl_G[idx];
			error = 0;
			break;
		}/*if*/
	}/*for*/

	if( error )
	{
        DBGWRT_ERR(( DBH, ">>> %s%s: l %08x tgtSigHdl %08x %s%d%s", errStart, functionName,
        				linkNbr, tgtSigHdl, errLine, __LINE__, errEnd ));

        logMsg( ">>> *** %s: l %08x tgtSigHdl %08x\n", (int)functionName, (int)linkNbr, (int)tgtSigHdl, 4,5,6 );

        goto CLEANUP;
	}	

CLEANUP:
	return( error );
}



/**********************************************************************/
/** Installs a OSS signal.
 *
 * MIPIOS_SIG_Install() calls OSS_SigCreate() to install a VxWorks OSS
 * singal. <linkNbr> and <tgtSigHdl> will be used to determine the
 * corresponding <ossSigHdl> in MIPIOS_SIG_Remove() and MIPIOS_SIG_Signal().
 * This function will be called by MIPIOS_LM_Transceive() in the context
 * of the task calling signal installing M_setstat() to know the task ID
 * where the signal should be sent to.
 *
 * \param str	\IN linkNbr   link number of the target connection
 * \param str	\IN tgtSigHdl target signal handle
 * \param str	\IN sigNo	  signal number
 *
 * \return 0 or error code
 */
int32 MIPIOS_SIG_Install( u_int32 linkNbr,  u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl, u_int32 sigNo )
{
  	static const char *functionName = __FUNCTION__;
  	int32	error;
  	int		idx;

	/*/* t.b.d. lock critical section */

	TryInitialize();

	error = ERR_MIPIOS_NO_BUF; /* if we not found empty space */

    DBGWRT_ERR(( DBH, "%s l/f/r %08x/%d/%d tgtSigHdl %08x sigNo %d\n", functionName, linkNbr, devFrameNbr, retryCnt, tgtSigHdl, sigNo ));

	for( idx = 0; idx < MIPIOS_SIGNALS_MAX; idx++ )
	{
		if( !sigTbl_G[idx].full )
		{
			error = OSS_SigCreate( OSS_VXWORKS_OS_HDL, sigNo, &sigTbl_G[idx].ossSigHdl );
			if( error )
			{
		        DBGWRT_ERR(( DBH, "%s%s: OSS_SigCreate() l/f/r %08x/%d/%d tgtSigHdl %08x  %s%d%s", errStart, functionName,
        					linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        		goto CLEANUP;
			}
			sigTbl_G[idx].linkNbr 	= linkNbr;
			sigTbl_G[idx].tgtSigHdl = tgtSigHdl;
			sigTbl_G[idx].sigNo 	= sigNo;
			sigTbl_G[idx].oldDevFrameNbr = devFrameNbr - 1; /* now we have 2^32 - 1 frames time until devFrameNbr matches */
			sigTbl_G[idx].oldRetryCnt 	 = _NEVER_SIGNALED;	
			sigTbl_G[idx].full		= 1;
			break;
		}
	}

CLEANUP:
	if( error )
		DBGWRT_ERR(( DBH, "%s%s: %s error %x l/tgtSigHdl %08x/%08x sigNo %d\n", errStart, functionName,
					 error, linkNbr, tgtSigHdl, sigNo,
					 errLine, __LINE__, errEnd ));
	return( error );
}


/**********************************************************************/
/** Removes a OSS signal.
 *
 * MIPIOS_SIG_Remove() removes a OSS signal.  
 *
 * \param str	\IN linkNbr   link number of the target connection
 * \param str	\IN tgtSigHdl target signal handle
 * \param str	\IN sigNo	  signal number
 *
 * \return 0 or error code
 */
int32 MIPIOS_SIG_Remove( u_int32 linkNbr, u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl, u_int32 sigNo )
{
  	static const char *functionName = __FUNCTION__;
  	MIPIOS_SIG_VX *mipiosSigHdl = NULL;
  	int32	error;

    DBGWRT_ERR(( DBH, "%s l/f/r %08x/%d/%d tgtSigHdl %08x sigNo %d\n", functionName, linkNbr, devFrameNbr, retryCnt, tgtSigHdl, sigNo ));

	error = CheckNotInitialized();
	if( error )
	{
        DBGWRT_ERR(( DBH, ">>> %s%s: CheckNotInitialized() l/f/r %08x/%d/%d tgtSigHdl %08x %s%d%s", errStart, functionName,
        				linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/*/* t.b.d. lock critical section */

	error = FindMipiosSignal( linkNbr, tgtSigHdl, &mipiosSigHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: FindMipiosSignal() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}
	
	error = OSS_SigRemove( OSS_VXWORKS_OS_HDL, &mipiosSigHdl->ossSigHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SigRemove() l/f/r %08x/%d/%d tgtSigHdl %08x  %s%d%s", errStart, functionName,
        				linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	mipiosSigHdl->linkNbr 	= 0;
	mipiosSigHdl->tgtSigHdl = 0;
	mipiosSigHdl->sigNo 	= 0;
	mipiosSigHdl->full		= 0;
	
	
	
CLEANUP:
	return( error );
}


/**********************************************************************/
/** Fires a OSS signal.
 *
 * MIPIOS_SIG_Signal() fires a OSS signal.  
 *
 * \param str	\IN linkNbr   	link number of the target connection
 * \param str	\IN devFrameNbr frame number
 * \param str	\IN retryCnt   	retry count
 * \param str	\IN tgtSigHdl target signal handle
 *
 * \return 0 or error code
 */
int32 MIPIOS_SIG_Signal( u_int32 linkNbr, u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl )
{
  	static const char *functionName = __FUNCTION__;
  	MIPIOS_SIG_VX *mipiosSigHdl = NULL;
  	int32	error;

    DBGWRT_ERR(( DBH, ">>> %s l/f/r %08x/%d/%d tgtSigHdl %08x\n", functionName, linkNbr, devFrameNbr, retryCnt, tgtSigHdl ));

	error = CheckNotInitialized();
	if( error )
	{
        DBGWRT_ERR(( DBH, ">>> %s%s: CheckNotInitialized() l/f/r %08x/%d/%d tgtSigHdl %08x %s%d%s", errStart, functionName,
        				linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/*logMsg(">>> %s  l %08x tgtSigHdl %08x\n", (int)functionName, (int)linkNbr, (int)tgtSigHdl, 4,5,6 );*/
	/*/* t.b.d. lock critical section */
	error = FindMipiosSignal( linkNbr, tgtSigHdl, &mipiosSigHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: FindMipiosSignal() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}


	/* avoid sending the same signal twice - due to late receive of a second target answer */
	if( mipiosSigHdl->oldDevFrameNbr == devFrameNbr 
		&& mipiosSigHdl->oldRetryCnt != retryCnt
	  )
	{
		error = 0; /* ignore retry */
        DBGWRT_ERR(( DBH, "%s%s: already signaled l/f/r %08x/%d/%d tgtSigHdl %08x  %s%d%s", warnStart, functionName,
        				linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/*logMsg(">>> %s ossSigHdl %08x\n", (int)functionName, (int)mipiosSigHdl->ossSigHdl, (int)3, 4,5,6 );*/
	error = OSS_SigSend( OSS_VXWORKS_OS_HDL, mipiosSigHdl->ossSigHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_SigSend() l/f/r %08x/%d/%d tgtSigHdl %08x  %s%d%s", errStart, functionName,
        				linkNbr, devFrameNbr, retryCnt, tgtSigHdl, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	mipiosSigHdl->oldDevFrameNbr = devFrameNbr; /* save last signaled frame number */
	mipiosSigHdl->oldRetryCnt    = retryCnt;

CLEANUP:
	return( error );
}


/**********************************************************************/
/** Initializes MIPIOS_SIG module if necessary.
 *
 * TryInitialize() initializes this module if necessary.  
 *
 * \return N/A
 */
void MIPIOS_SIG_Show( void )
{
  	static const char *functionName = __FUNCTION__;
  	int i;
	MIPIOS_SIG_VX *sP;

	if( CheckNotInitialized() )
	{
		printf("%s: not initialized\n", functionName );
		goto CLEANUP;
	}

	printf("%s:\n", functionName );
	printf("================\n");

	for( i=0; i<MIPIOS_SIGNALS_MAX; i++ )
	{
		sP = &sigTbl_G[i];
		if( sP->full )
		{
			printf("  l/tgtSigHdl %08x/%08x hostSigHdl/sigNo %08x/%d\n", (int)sP->linkNbr, (int)sP->tgtSigHdl, (int)sP->ossSigHdl, (int)sP->sigNo );
			
		}/*if*/
	}/*for*/
	
	
CLEANUP:
	return;
}


#endif /*INCLUDE_MIPIOS_VX*/

