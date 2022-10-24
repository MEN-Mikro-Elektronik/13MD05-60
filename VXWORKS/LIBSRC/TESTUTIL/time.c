/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: time.c
 *      Project: test utility library
 *
 *      $Author: kp $
 *        $Date: 2000/01/12 10:57:16 $
 *    $Revision: 1.1 $
 *
 *  Description: time functions
 *                      
 *                      
 *     Required: 
 *     Switches:  
 *
 *---------------------------[ Public Functions ]----------------------------
 *  
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: time.c,v $
 * Revision 1.1  2000/01/12 10:57:16  kp
 * Initial Revision
 *
 * Revision 1.2  1995/09/11 16:08:34  kp
 * tu_bytes_per_sec: algorithm changed to avoid overflow when bytes>40MB.
 *
 * Revision 1.1  95/09/05  14:30:40  kp
 * Initial revision
 * 
 *---------------------------------------------------------------------------
 * (c) Copyright 1995 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static char *RCSid="$Header: /dd2/CVSR/VXWORKS/LIBSRC/TESTUTIL/time.c,v 1.1 2000/01/12 10:57:16 kp Exp $";

#include <vxWorks.h>
#include <tickLib.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <MEN/men_typs.h>
#include <MEN/testutil.h>


/********************************** tu_get_tick ******************************
 *
 *  Description:  tu_get_tick returns a relative tick expressed in 1/100s.
 *   Mainly used for duration measurement.                       
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *    return	   relative tick in 1/100s units.
 *  Globals....:  ---
 ****************************************************************************/
u_int32 tu_get_tick(void)
{
	u_int32 time;

	time = tickGet() * 100 / sysClkRateGet();

	return time;
}


/********************************** tu_tick2string ****************************
 *
 *  Description:  creates a string from a tick difference.
 *                fmt can be:
 *				  0 	hh:mm:ss
 *				  1		hh:mm:ss.tt		(tt=ticks)         
 *				  2     mm:ss.tt		
 *				  3		ss.tt
 *---------------------------------------------------------------------------
 *  Input......:  
 *   fmt			0-3 string format
 *	 tick_diff		difference of two tu_get_tick calls
 *  Output.....:  (static) formatted string
 *  Globals....:  ---
 ****************************************************************************/
char *tu_tick2string( int fmt, u_int32 tick_diff )
{
	u_int32 hh,mm,ss,tt;
	static char str[30];
	char *p;

	tt = tick_diff % 100;
	tick_diff /= 100;
	ss = tick_diff % 60;
	tick_diff /= 60;
	mm = tick_diff % 60;
	tick_diff /= 60;
	hh = tick_diff % 24;
	tick_diff /= 24;

	p = str;
	if( fmt<= 1) p+=sprintf(p,"%02d:", hh );
	if( fmt<= 2) p+=sprintf(p,"%02d:", mm );
	p+=sprintf(p,"%02d", ss );
	if( fmt>0 )  p+=sprintf(p,".%02d", tt );
	return str;
}

/********************************** tu_bytes_per_sec **************************
 *
 *  Description:  returns a string with the transfer speed
 *                The string returned is:
 *                ___x.yy  B/s	  if( speed <  1.00 kB/s)
 *                ___x.yy KB/s     if( speed >= 1.00 kB/s)
 *                ___x.yy MB/s     if( speed >= 1.00 MB/s)
 *---------------------------------------------------------------------------
 *  Input......:  
 *   bytes          number of bytes transfered
 *	 tick_diff		difference of two tu_get_tick calls
 *  Output.....:  (static) formatted string
 *  Globals....:  ---
 ****************************************************************************/
char *tu_bytes_per_sec( u_int64 bytes, u_int64 tick_diff )
{
	static char str[30];
	u_int64 bytes_per_tick;
	u_int64 bps, bps_rem;
	char *unit=" ";

	if( tick_diff==0 ) return "****.** MB/s"; 
	bytes_per_tick = bytes/tick_diff;

	if( bytes/tick_diff >= (1024*1024)/100 ){
		bytes_per_tick = (bytes/tick_diff)/105;
		unit="M";
	}
	else if( (bytes_per_tick = bytes*100/tick_diff) >= 1024 ){
		bytes_per_tick *= 100;
		bytes_per_tick /= 1024;
		unit = "K";
	}
	else 
		bytes_per_tick *= 100 * 100;

	bps 	= bytes_per_tick/100;
	bps_rem = bytes_per_tick%100;

	sprintf( str, "%4llu.%02llu %sB/s", bps, bps_rem, unit );
	return str;
}

