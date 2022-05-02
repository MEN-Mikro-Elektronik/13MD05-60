/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sys_rtc.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2006/11/10 16:40:17 $
 *    $Revision: 2.2 $
 *
 *  	 \brief  RTC export routines of MEN VxWorks BSPs
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_rtc.h,v $
 * Revision 2.2  2006/11/10 16:40:17  svogel
 * added functions
 * ===============
 * 	+ sysWeekdayCalc
 * 	+ sysWeekdayCheck
 *
 * Revision 2.1  2005/06/23 08:33:25  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _SYS_RTC_H
#define _SYS_RTC_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define SYS_RTC_ERR_VLF		10	/* voltage low detect */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern int32 sysRtcGetDate( u_int8 *yyP, u_int8 *mmP,u_int8 *ddP,u_int8 *hourP,
							u_int8 *minP, u_int8 *secP);
extern void sysRtcShow( void );
#if _WRS_VXWORKS_MAJOR >= 7
extern int32 sysRtcSet_tm( u_int8 yy,	u_int8 mm, u_int8 dd, u_int8 hour, 
						u_int8 min,
				 		u_int8 sec);
extern int32 sysRtcGet_tm( void );
#else
extern int32 sysRtcSet( u_int8 yy,	u_int8 mm, u_int8 dd, u_int8 hour, 
						u_int8 min,
				 		u_int8 sec);
extern int32 sysRtcGet( void );
#endif
extern void sysWeekdayCheck(u_int8 day, u_int8 month, u_int32 year);
extern u_int8 sysWeekdayCalc(u_int8 day, u_int8 month, u_int32 year);

#ifdef __cplusplus
	}
#endif

#endif /* SYS_RTC_H */

