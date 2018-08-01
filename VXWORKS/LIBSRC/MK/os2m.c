/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: os2m.c
 *      Project: MDIS for VxWorks
 *
 *       Author: uf
 *        $Date: 2010/11/26 13:55:54 $
 *    $Revision: 2.7 $
 *
 *  Description: OS2M - The MDIS API to MDIS kernel glue module.
 *               This module implements the following functions:
 *					-driver installation/deinstallation
 *					-device table management/locking
 *					-device locking
 *
 *     Required: -
 *     Switches: INCLUDE_MIPIOS_VX_TARGET for MIPIOS targets, INCLUDE_MIPIOS_VX
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 *
 * --- ts@men 16.11.2015: end of mcvs source maintenance ---
 *
 * $Log: os2m.c,v $
 * Revision 2.7  2010/11/26 13:55:54  UFranke
 * R: no debug print if OS2M_DrvInstall() call missing
 * M: DBG_INIT added to OS2M_DevCreate()
 *
 * Revision 2.6  2010/09/02 16:33:51  UFranke
 * R: OS2M_IdentString removed
 * M: OS2M_Ident() added
 *
 * Revision 2.5  2010/03/29 16:34:43  cs
 * R: generalized/enhanced exclude of RTP support
 * M: define EX-/INCLUDE_RTP in MDIS rules.mak already, parse it only here
 *
 * Revision 2.4  2009/03/30 15:03:08  ufranke
 * R: warnings under VxWorks 6.7
 * M: cosmetics
 *
 * Revision 2.3  2009/02/19 10:00:18  ufranke
 * prepared
 *  + for MIPIOS
 *
 * Revision 2.2  2008/09/05 13:49:42  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 2.1  2005/12/23 11:16:29  UFRANKE
 * added
 * + VxWorks 6.x RTP (user mode) support
 *   OS2M_DrvInstall calls MAPI_SC_InstallMdisSysCall
 *   to install MDIS API system calls
 *
 * Revision 2.0  2000/03/17 15:07:47  kp
 * === MEGA CHANGE ===
 * OS2M_Read, Write etc removed. MDIS_API calls directly MK functions
 *
 * Revision 1.7  1999/08/31 12:14:43  Franke
 * MDIS 4.3
 * added LOCKING MODE NO(LL-Driver locking) and Channel Locking
 *       OS2M_Show() function
 * removed use of the VxWorks I/O system
 *         bug stdLib.h renamed to stdlib.h
 * changed OS2M_DevRemove parameter p (OS2M_DevHandle) to device name
 * replaced include of configAll.h by config.h
 *
 * Revision 1.6  1998/07/30 10:06:07  Franke
 * cosmetics
 *
 * Revision 1.5  1998/06/24 10:48:10  Franke
 * added  lock mode check ( channel and ll-driver locking not supported )
 *        device descriptor type check
 *
 * Revision 1.4  1998/06/17 16:44:56  Franke
 * changed MDIS 4.1 debug handling / OSS_Exit() call added
 * 	OSS_MikroDelayInit() called now in OSS_Init()
 *
 * Revision 1.3  1998/04/14 15:47:32  Franke
 * cosmetics NULL replaced with OSS_VXWORKS_OS_HDL
 *
 * Revision 1.2  1998/03/16 09:59:05  franke
 * adapted to new interface of MK_DevHdlCreate()
 *
 * Revision 1.1  1998/03/10 12:22:01  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static char *OS2M_IdentString = "OS2M - MDIS API to MDIS glue: $Id: os2m.c,v 2.7 2010/11/26 13:55:54 UFranke Exp $";

#include <MEN/men_typs.h>

#include <vxWorks.h>
#include <stdlib.h>
#include <taskLib.h>
#include <iosLib.h>
#include <errnoLib.h>
#include <string.h>
#include <stdio.h>
#include <sysLib.h>
#include <logLib.h>
#include <errnoLib.h>

#include <MEN/mdis_err.h>
#include <MEN/oss.h>
#include <MEN/mbuf.h>
#include <MEN/maccess.h>
#include <MEN/mdis_api.h>
#include <MEN/ll_defs.h>
#include <MEN/bb_defs.h>
#include <MEN/dbg.h>        /* debug module */
#include <MEN/desc.h>
#include <MEN/os2m.h>
#include <MEN/ll_entry.h>
#include <MEN/mk.h>
#include <MEN/bb_entry.h>
#include <MEN/bk.h>

#include <MEN/mipios_vx_api.h>

#define DBG_MYLEVEL		    OS2M_DbgLev

#ifdef DBG
	#define  DBH OS2M_dbgHdl
	DBG_HANDLE   *OS2M_dbgHdl = NULL;
#endif /*DBG*/

#include <MEN/mdis_rtp.h>

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
static const int OS2M_MaxDevName = 40;
#define  MAX_NAME_LEN   30
#define  NO_DRV          1
#define  ILLEGAL_NAME    2

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
u_int32 OS2M_DbgLev = OSS_DBG_DEFAULT;

OS2M_DEV_HDR   *OS2M_DevTbl[OS2M_MAX_DEVS];
OSS_SEM_HANDLE *OS2M_DevTblSem  = NULL;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static int DrvNum_g = 0;  /* driver number */

DBGCMD( static char errorStartStr[] = "*** ");
DBGCMD( static char errorLineStr[]   = " (line ");
DBGCMD( static char errorEndStr[]   = ")\n");

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int32    OS2M_CreateDevData( char 		 *device,
									DESC_SPEC    *llDescSpec,
                                    DESC_SPEC    *bbDescSpec,
                                    OS2M_DEV_HDR **devHdlP
                                  );

static int32 OS2M_DevTblInit(   void );
static int32 OS2M_DevTblExit(   void );
static int32 OS2M_DevTblAdd(    OS2M_DEV_HDR *devHdl );
static int32 OS2M_DevTblRemove( char *device, OS2M_DEV_HDR **devHdlP );
static int32 OS2M_DevTblFindDevPerName( char *device, OS2M_DEV_HDR **devHdlP );

char* OS2M_Ident( void )
{
	return( (char*) OS2M_IdentString );
}


/***************************** OS2M_ErrnoSet ********************************
 *
 *  Description:  sets the errno of the current task.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  errorNbr  errno to set
 *
 *  Output.....:  ---
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
void OS2M_ErrnoSet( int errorNbr ) /*nodoc*/
{
    errnoSet( errorNbr );
}/*OS2M_ErrnoSet*/


/****************************** OS2M_DrvInstall ******************************
 *
 *  Description: This routine initializes the MDIS driver.
 *
 *               OSS_Init is called for OSS_MikroDelayInit to adjust the
 *				 internal delay limits.
 *               ( disables/reenables interrupts and task scheduling )
 *
 *               It should be called, before any reads, writes, ...
 *               or OS2M_DevCreate() calls are performed.
 *               Normally, it should be called from usrRoot() in usrConfig.c.
 *
 *
 *               SEE ALSO: OSS_Init(), OSS_MikroDelayInit(), OS2M_DevCreate()
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *
 *  Output.....: return  OK | ERROR
 *
 *  Globals....: DrvNum_g, IOSemaphoresCreated_g, PathTableInitialized_g,
 *               errno, OSS_IrqNum0
 *
 ****************************************************************************/
STATUS OS2M_DrvInstall( void )
{
    DBGCMD( static const char *functionName = "OS2M_DrvInstall"; )
    int32 ret1 = ERROR;


    /* if driver already installed, just return */
    if( DrvNum_g > 0 )
    {
        ret1 = OK;
        goto CLEANUP;
    }/*if*/

#ifdef DBG
	if(!DBH)
	    DBGINIT((OSS_VXWORKS_OS_HDL,&DBH));
#endif /*DBG*/

#ifdef INCLUDE_MIPIOS_VX
	if(	MK_NUM_FILES >= OS2M_MIPIOS_FILE_HANDLE_START )
	{
		ret1 = ERR_MK_ILL_PARAM;
        DBGWRT_ERR( ( DBH, "%s%s: MK_NUM_FILES %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
	}
#endif

    DBGWRT_1((DBH,"%s\n", functionName));

#ifdef INCLUDE_RTP
	ret1 = MAPI_SC_InstallMdisSysCall();
    if( ret1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_InstallMdisSysCall %s%d%s",
					  errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/
#endif

    ret1 = OSS_Init();
    if( ret1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OSS_Init %s%d%s",
					  errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ret1 );
        return( ERROR );
    }/*if*/

	ret1 = OS2M_DevTblInit();
    if( ret1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblInit %s%d%s",
					  errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ret1 );
        return( ERROR );
    }/*if*/

	MK_Init();					/* initialize MDIS kernel */

    /* add driver to driver table */
    DrvNum_g = 1;

CLEANUP:
    return( ret1 );
}/*OS2M_DrvInstall*/

/****************************** OS2M_DrvRemove *******************************
 *
 *  Description: This routine deinitializes the MDIS driver.
 *
 *               For deinitialization the hardware and freeing the allocated
 *               buffers, close (M_close) all open files on the device and call
 *               OSS_DevRemove() for all devices before.
 *
 *               SEE ALSO: OS2M_DrvInstall(), OS2M_DevInstall/Remove()
 *---------------------------------------------------------------------------
 *  Input......: void
 *
 *  Output.....: -
 *
 *  Return.....: STATUS: OK | ERROR
 *
 *  Globals....: DrvNum_g, errno
 *
 ****************************************************************************/
STATUS  OS2M_DrvRemove( void )
{
    int32 ret1;

    DBGCMD( static const char *functionName = "OS2M_DrvRemove"; )

    DBGWRT_1((DBH,"%s\n", functionName));

    /* if driver not installed, just return */
    if( DrvNum_g < 1 )
    {
        DBGWRT_ERR( ( DBH, "*** Warning - %s: driver not installed %s%d%s",
                           functionName, errorLineStr, __LINE__,
					  errorEndStr ));
        return( OK );
    }/*if*/

	ret1 = OS2M_DevTblExit();
    if( ret1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblExit %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ret1 );
        return( ERROR );
    }/*if*/

    DrvNum_g = 0;   /* enabling new installation */
    #ifdef DBG
	    /* close dbg handle */
	   	if(DBH)
            DBGEXIT((&DBH));
    #endif /*DBG*/

	/* free OSS */
	OSS_Exit();

    return( OK );
}/*OS2M_DrvRemove*/

/***************************** OS2M_DevCreate *****************************
 *
 *  Description:  Installs the device to the OS2M device table.
 *                Gets the low level driver entrys.
 *                Creates the device data structure.
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName     pointer to the device name
 *                llDescSpec  low level driver descriptor specifier
 *                bbDescSpec  base board descriptor specifier
 *
 *  Output.....:  return OK | ERROR
 *
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
STATUS OS2M_DevCreate
(
    char      *devName,
    DESC_SPEC *llDescSpec,
    DESC_SPEC *bbDescSpec
)
{
    DBGCMD( static const char *functionName = "OS2M_DevCreate"; )
    OS2M_DEV_HDR *devHdl;                       /* pointer to MXX device */
    STATUS       retCode;

#ifdef DBG
	if(!DBH)
	    DBGINIT((OSS_VXWORKS_OS_HDL,&DBH));
#endif /*DBG*/

    DBGWRT_1((DBH,"%s( %s )\n", functionName, devName ));

    /*-----------------------------+
    | check if driver is installed |
    +-----------------------------*/
    if( DrvNum_g < 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: no driver - call OS2M_DrvInstall() missing %s%d%s",
					  errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ENOENT );
        return( ERROR );
    }/*if*/

    /*-----------------+
	| check parameters |
    +------------------*/
    if( devName == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: no device name %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ERR_MK_ILL_PARAM );
        return( ERROR );
    }/*if*/
    if( llDescSpec == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: no low level driver descriptor %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ERR_MK_NO_LLDESC );
        return( ERROR );
    }/*if*/
    if( bbDescSpec == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: no base board descriptor %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( ERR_MK_NO_BBISDESC );
        return( ERROR );
    }/*if*/

    if( strlen(devName) > OS2M_MaxDevName )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to long %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        return( ERROR );
    }/*if*/

    if( strlen(devName) < 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to short %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        return( ERROR );
    }/*if*/

    /*------------------------+
    | create device structure |
    +------------------------*/
    retCode = OS2M_CreateDevData( devName, llDescSpec, bbDescSpec, &devHdl );
    if( devHdl == NULL || retCode )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_CreateDevData %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( retCode );
        return ( ERROR );
    }/*if*/

	devHdl->mipiosFlag = 0; /* no mipios - this function is for local MDIS modules only */

    /***** add devName/structure to OS2M device table *****/
	if( (retCode = OS2M_DevTblAdd( devHdl )) )
	{
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblAdd %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        OS2M_ErrnoSet( retCode );
        if( devHdl )
            MK_DevHdlRemove( OS2M_DbgLev, (void*) &devHdl );
        return( ERROR );
    }/*if*/

    return( OK );
}/*OS2M_DevCreate*/

/***************************** OS2M_MipiosDevCreate *****************************
 *
 *  Description:  Installs a MIPIOS device to the OS2M device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName     pointer to the device name
 *
 *  Output.....:  return OK | ERROR
 *
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
STATUS OS2M_MipiosDevCreate
(
    char      *devName,
    OS2M_DEV_HDR *devHdl
)
{
    DBGCMD( static const char *functionName = __FUNCTION__; )
    STATUS       retCode = ERR_MK_ILL_PARAM;

    DBGWRT_1((DBH,"%s: %s handle %08x\n", functionName, devName, devHdl ));

    /*-----------------------------+
    | check if driver is installed |
    +-----------------------------*/
    if( DrvNum_g < 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: call OS2M_DrvInstall() before %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        retCode = ENOENT;
        goto CLEANUP;
    }/*if*/

    /*-----------------+
	| check parameters |
    +------------------*/
    if( devName == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: no device name %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

    if( strlen(devName) > OS2M_MaxDevName )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to long %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

    if( strlen(devName) < 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to short %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

	if( devHdl == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: devHdl %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

	if( devHdl->mipiosFlag != 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: mipiosFlag %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

    /***** add devName/structure to OS2M device table *****/
	if( (retCode = OS2M_DevTblAdd( devHdl )) )
	{
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblAdd %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

CLEANUP:
	if( retCode )
        OS2M_ErrnoSet( retCode );

    return( retCode );
}/*OS2M_DevCreate*/


/******************************** OS2M_DevRemove *****************************
 *
 *  Description: This routine deletes a device which was created
 *               by OS2M_DevCreate().
 *               It does not deinitialize the hardware.(Call M_close() before.)
 *
 *               SEE ALSO: OS2M_DrvRemove()
 *---------------------------------------------------------------------------
 *  Input......: device   name of the device e.g. "/m34/0"
 *
 *  Output.....: return OK | ERROR
 *
 *  Globals....: errno
 *
 ****************************************************************************/
STATUS  OS2M_DevRemove
(
	char *device
)
{
    DBGCMD( static const char *functionName = "OS2M_Remove"; )
    OS2M_DEV_HDR *devHdl;
    int32 error;

    DBGWRT_1((DBH,"%s\n", functionName));

    /* deletes the device from the OS2M device table */
    error = OS2M_DevTblRemove( device, &devHdl );
    if( error )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblRemove %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/

    error = MK_DevHdlRemove( OS2M_DbgLev, (void*) &devHdl );
    if( error )
    {
        DBGWRT_ERR( ( DBH, "%s%s: MK_DevHdlRemove %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        goto CLEANUP;
    }/*if*/


    return( OK );

CLEANUP:
    OS2M_ErrnoSet( error );
    if( devHdl != NULL )
    {
    	if( devHdl )
        	MK_DevHdlRemove( OS2M_DbgLev, (void*) &devHdl );
    }/*if*/
    return( ERROR );
}/*OS2M_DevRemove*/

#ifdef INCLUDE_MIPIOS_VX_TARGET
	/* this function will be used for MIPIOS target test only */
	int32 OS2M_SetLlDescForMipios  /*nodoc*/
	(
	    char		*devName,
	    DESC_SPEC	llDescSpec
	)
	{
	    DBGCMD( const char *functionName = __FUNCTION__; )
	    OS2M_DEV_HDR *devHdl;
	    int32 error = ERR_MK_ILL_PARAM;

		/* lock device table */
	    if( ( error = OS2M_DevTblLock() ) != 0 )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblLock - with error 0x%08x %s%d%s", errorStartStr, functionName, error, errorLineStr, __LINE__, errorEndStr ));

		    OS2M_ErrnoSet( error );
	    	return( ERROR);
	    }/*if*/

		/* look for device */
	    if( ( error = OS2M_DevTblFindDevPerName( devName, &devHdl) ) != 0 )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblFindDevPerName(\"%s\") - with error 0x%08x %s%d%s", errorStartStr, functionName, devName, error, errorLineStr, __LINE__, errorEndStr ));
			goto CLEANUP;
	    }/*if*/

		/* setup ll desc */
		error = MK_SetLlDescForMipios( devHdl, llDescSpec );
		if( error )
		{
	        DBGWRT_ERR( ( DBH, "%s%s: MK_SetLlDescForMipios() %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			goto CLEANUP;
		}

	CLEANUP:
	    if( OS2M_DevTblUnLock() != 0 )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock() %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		}/*if*/
		return( error );
	}
#endif /*INCLUDE_MIPIOS_VX_TARGET*/


/*************************** OS2M_Open ***************************************
 *
 *  Description:  Opens a MDIS device path (calls MK_Open).
 *
 *---------------------------------------------------------------------------
 *  Input......:  device   name of the device e.g. "/m34/0"
 *
 *  Output.....:  return  path | ERROR
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
int32 OS2M_Open  /*nodoc*/
(
	char *device
)
{
    DBGCMD( static const char *functionName = "OS2M_Open"; )
    OS2M_DEV_HDR *devHdl;
    int32 error;
    int32 error2;
    int32 path;

    DBGWRT_1((DBH,"%s - device %s\n", functionName, device));


	/* lock device table */
    if( ( error = OS2M_DevTblLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblLock - with error 0x%08x %s%d%s",
                          errorStartStr, functionName, error, errorLineStr,
					  __LINE__, errorEndStr ));

	    OS2M_ErrnoSet( error );
    	return( ERROR);
    }/*if*/

	/* look for device */
    if( ( error = OS2M_DevTblFindDevPerName( device, &devHdl) ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblFindDevPerName(\"%s\") - "
					  "with error 0x%08x %s%d%s",
					  errorStartStr, functionName,
					  device, error,
					  errorLineStr, __LINE__, errorEndStr ));
		goto ERROR_RETURN;
    }/*if*/

	if( devHdl->mipiosFlag )
	{
	    /*------------------------------------------+
	    | open MIPIOS                               |
	    +------------------------------------------*/
	    error = MIPIOS_Open( devHdl, &path );
	    if( error )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: MIPIOS_Open - with error 0x%08x %s%d%s",
						  errorStartStr, functionName, error, errorLineStr, __LINE__, errorEndStr ));
			goto ERROR_RETURN;
		}
	}
	else
	{
	    /*------------------------------------------+
	    | open path and init path table entry       |
	    +------------------------------------------*/
	    if( ( error = MK_Open( devHdl, &path) ) != 0 )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: MK_Open - with error 0x%08x %s%d%s",
						  errorStartStr, functionName, error, errorLineStr,
						  __LINE__, errorEndStr ));

			goto ERROR_RETURN;
	    }/*if*/
    }/*if*/

	/* unlock device table */
    if( ( error = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - "
					  "with error 0x%08x %s%d%s",errorStartStr, functionName,
					  error, errorLineStr, __LINE__, errorEndStr ));

	    OS2M_ErrnoSet( error );
    	return( ERROR);
    }/*if*/

    DBGWRT_2((DBH, "%s path=%ld\n", functionName, path ) );
    return( path );

ERROR_RETURN:
    if( ( error2 = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - "
					  "with error 0x%08x %s%d%s",
					  errorStartStr, functionName, error2,
					  errorLineStr, __LINE__, errorEndStr ));
	}/*if*/
    OS2M_ErrnoSet( error );
    return( ERROR);

}/*OS2M_Open*/

/************************** OS2M_Close ***************************************
 *
 *  Description:  Closes a MDIS device path (calls MK_Close).
 *
 *---------------------------------------------------------------------------
 *  Input......:  s       pointer to MDIS device data
 *
 *  Output.....:  return  OK or ERROR
 *
 *  Globals....:  errno
 *
 ****************************************************************************/
int OS2M_Close( int32 path )  /*nodoc*/
{
    DBGCMD( static const char *functionName = "OS2M_Close"; )
    OS2M_DEV_HDR *devHdl;
    int error;
    int32 error2;

    DBGWRT_1((DBH,"%s path = %ld\n", functionName, path ));

    if( path < 0 )
    {
    	error = ERR_BAD_PATH;
        DBGWRT_ERR( ( DBH, "%s%s: path %d %s%d%s", errorStartStr, functionName, path, errorLineStr, __LINE__, errorEndStr ));
        OS2M_ErrnoSet( error );
        return( ERROR );
	}

    /*--------------------------+
    | get the device handle     |
    +--------------------------*/
    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
		error = MK_GetDevHdl( path, (void*) &devHdl );
	    if( error )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: MK_GetDevHdl %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	        OS2M_ErrnoSet( error );
	        return( ERROR );
	    }/*if*/
	}/*if*/

	/* lock device table */
    if( ( error = OS2M_DevTblLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblLock - with error 0x%08x %s%d%s", errorStartStr, functionName, error, errorLineStr, __LINE__, errorEndStr ));
	    OS2M_ErrnoSet( error );
    	return( ERROR);
    }/*if*/

    /*------------------------------------------+
    | close path and clear path table entry     |
    +------------------------------------------*/
    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
	    error = MK_Close( path );              /* error closing path ? */
	    if( error != OK )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: MK_Close %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			goto ERROR_RETURN;
	    }/*if*/
	}
	else
    {
	    error = MIPIOS_Close( path );              /* error closing path ? */
	    if( error != OK )
	    {
	        DBGWRT_ERR( ( DBH, "%s%s: MK_Close %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			goto ERROR_RETURN;
	    }/*if*/
	}

	/* unlock device table */
    if( ( error = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - with error 0x%08x %s%d%s", errorStartStr, functionName, error, errorLineStr, __LINE__, errorEndStr ));
	    OS2M_ErrnoSet( error );
    	return( ERROR);
    }/*if*/

    DBGWRT_2((DBH, "%s path=%ld\n", functionName, path ) );
    return( OK );


ERROR_RETURN:
    if( ( error2 = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - with error 0x%08x %s%d%s", errorStartStr, functionName, error2, errorLineStr, __LINE__, errorEndStr ));
	}/*if*/
    OS2M_ErrnoSet( error );
    return( ERROR);

}/*OS2M_Close*/

/**************************** OS2M_DbgLevelSet ******************************
 *
 *  Description:  sets the debug level of this module.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgLevel - the new dbg level
 *
 *  Output.....:  ---
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
void OS2M_DbgLevelSet( u_int32 dbgLevel )
{
    DBGCMD( static const char *functionName = "OS2M_DbgLevelSet"; )
    DBGWRT_1((DBH,"%s\n", functionName));
    OS2M_DbgLev = dbgLevel;
}/*OS2M_DbgLevelSet*/

/**************************** OS2M_DbgLevelGet ******************************
 *
 *  Description:  gets the debug level of this module.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *
 *  Output.....:  dbgLevelP - the current dbg level
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
void OS2M_DbgLevelGet( u_int32 *dbgLevelP )
{
    DBGCMD( static const char *functionName = "OS2M_DbgLevelGet"; )
    DBGWRT_1((DBH,"%s\n", functionName));
     *dbgLevelP = OS2M_DbgLev;
}/*OS2M_DbgLevelGet*/

/************************** OS2M_CreateDevData ******************************
 *
 *  Description:  Creates the MDIS device data structure.
 *                Calls OS2M_CreateDevLockHdl(), OS2M_GetDrvEntrys
 *                and MK_DevHdlCreate().
 *
 *---------------------------------------------------------------------------
 *  Input......:  device	  name of the device
 *                llDescSpec  ll-driver descriptor specifier
 *                bbDescSpec  bbis descriptor specifier
 *                os2mHdlP    pointer to variable where device data stored
 *
 *  Output.....:  *os2mHdlP   device data handle
 *                return      0 or error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int32 OS2M_CreateDevData  /*nodoc*/
(
	char		 *device,
    DESC_SPEC    *llDescSpec,
    DESC_SPEC    *bbDescSpec,
    OS2M_DEV_HDR **devHdlP
)
{
    DBGCMD( static const char *functionName = "OS2M_CreateDevData"; )
    int32        retCode;
    DESC_HANDLE  *llDescHdl = NULL;
    LL_ENTRY     llDrvEntries;
	char         llDrvName[MAX_NAME_LEN];
    u_int32      strLen;
	u_int32      descType;

    DBGWRT_1((DBH,"%s\n", functionName));

	*devHdlP = NULL;

    /*----------------------------------+
    | get LL-Drv entries                |
    +----------------------------------*/
    if( ( retCode = DESC_Init( llDescSpec,
                               OSS_VXWORKS_OS_HDL,
                               &llDescHdl )
         )
      )
    {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_Init %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
    }
	else
	{

        retCode = DESC_GetUInt32( llDescHdl,
                                  0,
                                  &descType,
                                  "DESC_TYPE" );
	    if( retCode != 0 )
		{
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 DESC_TYPE %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
		}/*if*/

	    if( descType != 1 )
		{
            DBGWRT_ERR( ( DBH, "%s%s: DESC_TYPE not a device descriptor "
						  "%s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
		}/*if*/

        strLen = MAX_NAME_LEN;
        if( ( retCode = DESC_GetString( llDescHdl,
                                        "",
                                        llDrvName,
                                        &strLen,
                                        "HW_TYPE" )
            )
          )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetString HW_TYPE %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/


        /* here llDrvName should contain e.g. "M099" */
        if( (retCode = LL_FindEntry( llDrvName, &llDrvEntries ) ) )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL_FindEntry - "
						  "no LowLevel Driver for %s %s%d%s",
                          errorStartStr, functionName, llDrvName,
						  errorLineStr, __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/
        if( (retCode = DESC_Exit( &llDescHdl )) )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_Exit %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/
    }/*if*/

    /*----------------------------------+
    | create device handle              |
    +----------------------------------*/
    retCode = MK_DevHdlCreate(  OS2M_DbgLev,
                                llDescSpec,
                                bbDescSpec,
					    	    llDrvName,
                                &llDrvEntries,
                                (OS2M_MK_HANDLE*)devHdlP,
                                device
                             );

    if( retCode )
    {
        DBGWRT_ERR( ( DBH, "%s%s: MK_DevHdlCreate %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        return( retCode );
    }/*if*/

    return( retCode );

CLEANUP:
	if( llDescHdl )
        DESC_Exit( &llDescHdl );
    if( *devHdlP )
        MK_DevHdlRemove( OS2M_DbgLev, (OS2M_MK_HANDLE*) devHdlP );

    return( retCode );
}/*OS2M_CreateDevData*/




/*
 *
 *   OS2M Device Table Functions
 *
 */


/**************************** OS2M_DevTblInit *******************************
 *
 *  Description:  Creates the device table semaphore and
 *                initializes the device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return      0 | error code
 *  Globals....:  OS2M_DevTblSem, OS2M_DevTbl
 ****************************************************************************/
static int32 OS2M_DevTblInit(   void )  /*nodoc*/
{
int32 retCode;
int	  i;

	for( i=0; i<OS2M_MAX_DEVS; i++ )
		OS2M_DevTbl[i] = NULL;

    /* create device table locking semaphores */
    retCode = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 1,
							 &OS2M_DevTblSem  );
    if( retCode )
    {
        goto CLEANUP;
    }/*if*/

CLEANUP:
	return( retCode );
}/*OS2M_DevTblInit*/

/**************************** OS2M_DevTblExit *******************************
 *
 *  Description:  Removes the device table semaphore.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return      0 | error code
 *  Globals....:  OS2M_DevTblSem
 ****************************************************************************/
static int32 OS2M_DevTblExit( void )   /*nodoc*/
{
int32 retCode;

    /* remove device table locking semaphores */
    retCode = OSS_SemRemove( OSS_VXWORKS_OS_HDL, &OS2M_DevTblSem );
    if( retCode )
    {
        goto CLEANUP;
    }/*if*/


CLEANUP:
	return( retCode );
}/*OS2M_DevTblExit*/

/**************************** OS2M_DevTblLock *******************************
 *
 *  Description:  Locks the device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return      0 | error code
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
int32 OS2M_DevTblLock( void )   /*nodoc*/
{
    DBGCMD( static const char *functionName = "OS2M_DevTblLock"; )
	int32		 error;

    error = OSS_SemWait(OSS_VXWORKS_OS_HDL, OS2M_DevTblSem,
						OSS_SEM_WAITFOREVER );
    if ( error )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OSS_SemWait %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
    }/*if*/

	return( error );
}/*OS2M_DevTblLock*/

/**************************** OS2M_DevTblUnLock *****************************
 *
 *  Description:  Unlocks the device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return      0 | error code
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
int32 OS2M_DevTblUnLock( void )   /*nodoc*/
{
    DBGCMD( static const char *functionName = "OS2M_DevTblUnLock"; )
	int32		 error;

    error = OSS_SemSignal(OSS_VXWORKS_OS_HDL, OS2M_DevTblSem );
    if ( error )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OSS_SemSignal %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
    }/*if*/

	return( error );
}/*OS2M_DevTblUnLock*/

/**************************** OS2M_DevTblFindDevPerName *********************
 *
 *  Description:  Looks for a device in the OS2M device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  device	name of the device e.g. "/m34/0"
 *                devData	pointer variable where pointer to MDIS device
 *                			data will be stored
 *
 *  Output.....:  return    0 | error code
 *                *devData	0 | pointer to MDIS device
 *
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
static int32 OS2M_DevTblFindDevPerName( char *device, OS2M_DEV_HDR **devHdlP )   /*nodoc*/
{
int32 error = ENODEV;
int	  i;

	*devHdlP = NULL;

	for( i=0; i<OS2M_MAX_DEVS; i++ )
	{
		if( OS2M_DevTbl[i] != NULL )
		{
			if( !strcmp(device,OS2M_DevTbl[i]->devName) )
			{
				*devHdlP = OS2M_DevTbl[i];
				error = 0;
				break;
			}/*if*/
		}/*if*/
	}/*for*/

	return( error );
}/*OS2M_DevTblFindDevPerName*/

/**************************** OS2M_DevTblAdd *******************************
 *
 *  Description:  Add a device to the device table.
 *
 *---------------------------------------------------------------------------
 *	Input......:  devData     OS2M device handle
 *  Output.....:  return      0 | error code
 *  Globals....:  OS2M_DevTbl, errno (EEXIST)
 ****************************************************************************/
static int32 OS2M_DevTblAdd(    OS2M_DEV_HDR *devHdl )   /*nodoc*/
{
    DBGCMD( static const char *functionName = "OS2M_DevTblAdd"; )
	OS2M_DEV_HDR *devDataDummy;
	int32		 error;
	int32		 err2;
	int			 i;
	int			 found;

	/* lock device table */
    if( ( error = OS2M_DevTblLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblLock - with error 0x%08x %s%d%s",
                          errorStartStr, functionName, error, errorLineStr,
					  __LINE__, errorEndStr ));
		return( error );
    }/*if*/

	if( OS2M_DevTblFindDevPerName( devHdl->devName, &devDataDummy ) == OK )
	{
		error = EEXIST;
        DBGWRT_ERR( ( DBH, "%s%s: device (%s) exist %s%d%s",
                          errorStartStr, functionName,
						  devHdl->devName,
						  errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
	}/*if*/

	found = 0;
	for( i=0; i<OS2M_MAX_DEVS; i++ )
	{
		if( OS2M_DevTbl[i] == NULL )
		{
			found = 1;
			break;
		}/*if*/
	}/*for*/

	if( !found )
	{
		error = ERR_PATH_FULL;
        DBGWRT_ERR( ( DBH, "%s%s: OS2M device table full %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        goto CLEANUP;
	}/*if*/

	OS2M_DevTbl[i] = devHdl;

CLEANUP:
	/* unlock device table */
    if( ( err2 = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - "
					  "with error 0x%08x %s%d%s",
					  errorStartStr, functionName, error, errorLineStr,
					  __LINE__, errorEndStr ));
		if( !error )
			error = err2;
    }/*if*/
	return( error );
}/*OS2M_DevTblAdd*/

/**************************** OS2M_DevTblRemove *******************************
 *
 *  Description:  Removes a device from the device table.
 *
 *---------------------------------------------------------------------------
 *  Input......:  device    device name e.g. "/m55/0"
 *				  devDataP  pointer to variable where OS2M device handle
 *                          will be stored
 *  Output.....:  return    0 | error code
 *				  *devDataP OS2M device handle
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
static int32 OS2M_DevTblRemove( char *device, OS2M_DEV_HDR **devHdlP )   /*nodoc*/
{
    DBGCMD( static const char *functionName = "OS2M_DevTblRemove"; )
	int32		 error;
	int32		 err2;
	int			 i;

	*devHdlP = NULL;

	/* lock device table */
    if( ( error = OS2M_DevTblLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblLock - with error 0x%08x %s%d%s",
					  errorStartStr, functionName, error, errorLineStr,
					  __LINE__, errorEndStr ));
		return( error );
    }/*if*/

	if( (error = OS2M_DevTblFindDevPerName( device, devHdlP )) )
	{
        DBGWRT_ERR( ( DBH, "%s%s: device (%s) not exist %s%d%s",
					  errorStartStr, functionName, device,
					  errorLineStr, __LINE__, errorEndStr ));
        goto CLEANUP;
	}/*if*/

	for( i=0; i<OS2M_MAX_DEVS; i++ )
	{
		if( OS2M_DevTbl[i] != NULL )
		{
			if( !strcmp(device,OS2M_DevTbl[i]->devName) )
			{
				OS2M_DevTbl[i] = NULL;
				break;
			}/*if*/
		}/*if*/
	}/*for*/

CLEANUP:
	/* unlock device table */
    if( ( err2 = OS2M_DevTblUnLock() ) != 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OS2M_DevTblUnLock - "
					  "with error 0x%08x %s%d%s",
                          errorStartStr, functionName, error,
					  errorLineStr, __LINE__, errorEndStr ));
		if( !error )
			error = err2;
    }/*if*/
	return( error );
}/*OS2M_DevTblRemove*/

/*
 *
 *   OS2M Show Functions
 *
 */

/******************************** OS2M_GetDescSpec **************************
 *
 *  Description:  returns a DESC_SPEC pointer referring to device devName
 *                needed to avoid increasing problem with SDA21 relocation 
 *                error:
 *                
 *  symptom: error during final link of mdis_MEN_XXXX.o object to the rest 
 *           of VIP image:
 *           "ldppc: sysLib.o: the target (m99_3) of a R_PPC_EMB_SDA21 
 *           relocation is in the wrong output section (.data)"
 *---------------------------------------------------------------------------
 *  Input......:  device name as specified in MDIS project, e.g. "m99_3"
 *  Output.....:  pointer to DESC_SPEC struct or NULL if device not found
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
DESC_SPEC *OS2M_GetDescSpec(char *devName)
{
	DESC_SPEC *pSpec=NULL;
	
    DBGCMD( static const char *functionName = "OS2M_GetDescSpec"; )

    if( strlen(devName) > OS2M_MaxDevName )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to long %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        return( NULL );
    }/*if*/

    if( strlen(devName) < 1 )
    {
        DBGWRT_ERR( ( DBH, "%s%s: dev name to short %s%d%s",
                          errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
        return( NULL);
    }/*if*/

    pSpec = (DESC_SPEC *)MdisDescRefGetByName(devName);
    
    return pSpec;
}

/******************************** OS2M_Show *********************************
 *
 *  Description:  Displays the device table and calls MK_Show and BK_Show.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  OS2M_DevTbl
 ****************************************************************************/
void OS2M_Show( void )
{
int i;

	printf("\n\n=== OS2M Device Table ===\n");
	printf("nbr\t\tdevName\t\tconnected\n");
	printf("---\t\t-------\t\t---------\n");

	for( i=0; i<OS2M_MAX_DEVS; i++ )
	{
		if( OS2M_DevTbl[i] != NULL )
		{
			printf("%3d\t\t%s\t%s\n", i, OS2M_DevTbl[i]->devName, OS2M_DevTbl[i]->mipiosFlag ? "mipios" : "\tlocal" );
		}/*if*/
	}/*for*/

	MIPIOS_ShowPath();

	MK_Show( -1 );

	BK_Show();

}/*OS2M_DevShow*/





