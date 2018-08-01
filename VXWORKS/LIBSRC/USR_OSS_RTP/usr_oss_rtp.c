/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_rtp.c
 *      Project: User OSS functions for VxWorks 6.x RTPs
 *
 *       Author: uf
 *        $Date: 2009/04/01 13:59:24 $
 *    $Revision: 1.4 $
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
 * $Log: usr_oss_rtp.c,v $
 * Revision 1.4  2009/04/01 13:59:24  ufranke
 * cosmetics
 *
 * Revision 1.3  2008/09/10 09:13:09  ufranke
 * fixed
 * - UOS_Delay() now tickGet() rollover save
 *   happens if UOS_Delay() will be running
 *   during a tick rollover and will be interrupted
 *   by a signal before taskDelay() finished
 *
 * Revision 1.2  2006/06/08 14:16:59  ufranke
 * added
 *  + UOS_VxShowRevision()
 *
 * Revision 1.1  2006/06/06 13:59:20  ufranke
 * Initial Revision
 *
 * renamed to usr_oss_rtp.c
 * Revision 1.1  2005/12/23 15:23:30  UFRANKE
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2008 by MEN Mikro Elektronik GmbH, Nueremberg, Germany 
 ****************************************************************************/
static const char UOS_RCSid[]="$Id: usr_oss_rtp.c,v 1.4 2009/04/01 13:59:24 ufranke Exp $";

#include <MEN/men_typs.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg_vx_rtp.h>
#include <MEN/dbg.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#include <errnoLib.h>
#include <tickLib.h>
#include <taskLib.h>
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
u_int32 UOS_DbgLev = 0xC0008000;

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
/** This functions displays the revision of the UOS lib.
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
    ULONG    TicksToWait;
    int		 RestTicksToWait;
    unsigned long long int StopTime;
    ULONG    CurrentTime;
    int      RealMsec;
    STATUS   RetCode;
    long long int RestTicksToWait64;
    ULONG	      StartTime;


    TicksToWait = 0;

    if( msec )
    {
		/*--- round time, and correct for timer inaccuracy ---*/
        TicksToWait = (msec * sysClkRateGet()) / 1000;

		if( ((TicksToWait * 1000) / sysClkRateGet()) != (u_int32)msec )
			TicksToWait++;

		TicksToWait++;

        RestTicksToWait = TicksToWait;
        if( RestTicksToWait < 0 )
        {
        	for(;;)
        	{	/* block forever */
	        	printf("*** tid 0x%08x %s msec %lu - RestTicksToWait %d out of range\n", taskIdSelf(), __FUNCTION__ , msec, (int)RestTicksToWait );
        		taskDelay( sysClkRateGet() * 5 );
        	}
    	}

		StartTime = tickGet();
        StopTime  = StartTime + (unsigned long long int)TicksToWait;

        do
        {
        	RetCode = taskDelay( RestTicksToWait );
        	if( RetCode != OK )
          	{
        		/* signal received or called form ISR */
        		CurrentTime = tickGet();

				RestTicksToWait64 = StopTime - (unsigned long long int)CurrentTime;
		        if( CurrentTime < StartTime )
		        	RestTicksToWait64 -= 0xffffffff; /* CurrentTime rollover adjust */
		        RestTicksToWait = (int)RestTicksToWait64;

            	if( RestTicksToWait < 1 )
            	{
                	RestTicksToWait = 0;
            	}/*if*/
           }/*if*/
        }while( RetCode != OK && RestTicksToWait );
    }/*if*/

    RealMsec = (TicksToWait * 1000) / sysClkRateGet();

    return( RealMsec );
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


