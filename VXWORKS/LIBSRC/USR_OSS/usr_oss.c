/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss.c
 *      Project: User OSS functions
 *
 *      $Author: ufranke $
 *        $Date: 2006/06/08 14:18:13 $
 *    $Revision: 1.5 $
 *
 *  Description: USER OSS - This module is the user oss core file.
 *
 *               In general the user operating system services module (prefix UOS)
 *				 is used for common MDIS examples, running on different OS like
 *               WinNT, OS9 and VxWorks.
 *
 *               You will get a better performance if you call the native VxWorks
 *               functions. Also you will have the restictions of VxWorks only.
 *               ( i.e. signals )
 *
 *				 Only for shared memory and callbacks it is strongly recomended to use
 *               the UOS functions. Because the UOS functions following the
 *               conventions of the OSS shared memory and callback functions called
 *               by the low level driver.
 *
 *			     This module provides functions for errno getting and delaying.
 *
 *     Required:
 *     Switches: --
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss.c,v $
 * Revision 1.5  2006/06/08 14:18:13  ufranke
 * added
 *  + UOS_VxShowRevision()
 *
 * Revision 1.4  2005/06/29 15:07:48  UFranke
 * cosmetics
 *
 * Revision 1.3  2004/05/13 14:36:54  UFranke
 * added
 *  + UOS_ErrnoSet()
 *
 * Revision 1.2  1999/08/31 10:53:34  Franke
 * upgrade to MDIS 4.3
 * splitted into modules for object level linking
 *
 * Revision 1.1  1998/03/10 12:09:47  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char UOS_RCSid[]="$Id: usr_oss.c,v 1.5 2006/06/08 14:18:13 ufranke Exp $\n";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"
#include <errnoLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <string.h>


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
DBG_HANDLE   *UOS_DbgHdl = NULL;
u_int32 UOS_DbgLev = OSS_DBG_DEFAULT;

DBGCMD( char *UOS_ErrorStartStr = "*** "; )
DBGCMD( char *UOS_ErrorLineStr  = " (line "; )
DBGCMD( char *UOS_ErrorEndStr   = ")"; )
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

/**********************************************************************/
/** This functions displays the revisions of the UOS lib.
 *  
 * \param  -
 *
 * \return -
 */
void UOS_VxShowRevision( void )
{
	printf(UOS_RCSid);
	printf("Build %s %s\n", __DATE__, __TIME__ );
}


/********************************* UOS_ErrnoGet *****************************
 *
 *  Description: Get global error code (errno)
 *			     The function returns the last occured system error code
 *               of the calling process.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return		error code
 *  Globals....: -
 ****************************************************************************/
u_int32 UOS_ErrnoGet( void )
{
    return( errnoGet() );
}/*UOS_ErrnoGet*/

/**********************************************************************/
/** Set global error code (task/process specific errno)
 * 
 * \copydoc usr_oss_specification.c::UOS_ErrnoSet()
 *
 * \sa UOS_ErrnoGet
 */
u_int32 UOS_ErrnoSet( u_int32 errCode )
{
	return( errnoSet( errCode ) );
}

/************************ UOS_MikroDelayInit *********************************
 *
 *  Description:  Calibrates the loop counter for UOS_MikroDelay.
 *
 *                Error ERR_UOS_NO_MIKRODELAY (Overflow) is returned
 *                if the function is not available.
 *
 *		   Note:  this routine do nothing under VxWorks
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 UOS_MikroDelayInit( void )
{
    DBGCMD( if( !UOS_DbgHdl ) DBG_Init( NULL, &UOS_DbgHdl ); );
    return( OSS_MikroDelayInit(OSS_VXWORKS_OS_HDL) );
}/*UOS_MikroDelayInit*/

/*************************** UOS_MikroDelay **********************************
 *
 *  Description:  Wait with microsecond resolution in a loop.
 *
 *                NOTE: May take longer if code/data are not in cache
 *                      or other tasks have a higher priority.
 *
 *---------------------------------------------------------------------------
 *  Input......:  usec    time to wait (0..UOS_MAX_USEC) [usec]
 *  Output.....:  return  success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 UOS_MikroDelay( u_int32 usec )
{
    return( OSS_MikroDelay( OSS_VXWORKS_OS_HDL, usec ) );
}/*UOS_MikroDelay*/

/*************************** UOS_Delay **************************************
 *
 *  Description:  Lets a task sleep for a specified time.
 *
 *				  Time will rounded up to the systems minimum ticker
 *				  resolution. This (rounded) time is returned.
 *
 *---------------------------------------------------------------------------
 *  Input......:  msec    time to sleep [msec]
 *  Output.....:  return  rounded time [msec]
 *  Globals....:  -
 ****************************************************************************/
int32 UOS_Delay( u_int32 msec )
{
    return( OSS_Delay( OSS_VXWORKS_OS_HDL, msec ) );
}/*UOS_Delay*/


/********************************* UOS_MsecTimerGet ***********************
 *
 *  Description: Read the current timer value (based on system ticks).
 *	             		   
 *				 Time will rounded to the minimum timer resolution. 
 *				 This (rounded) time is returned.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return	  elapsed time [msec]
 *  Globals....: -
 ****************************************************************************/
u_int32 UOS_MsecTimerGet(void)
{
   return( (1000 * tickGet())/ sysClkRateGet() );		/* in [msec] */
}

/*************************** UOS_MsecTimerResolution ************************
 *
 *  Description: Get the minimum timer resolution.
 *                         
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return   timer resolution [msec]
 *  Globals....: -
 ****************************************************************************/
u_int32 UOS_MsecTimerResolution(void)
{
    return( 1000/sysClkRateGet() );
}

