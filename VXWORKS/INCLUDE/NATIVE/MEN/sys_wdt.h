/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sys_wdt.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2005/07/05 12:28:36 $
 *    $Revision: 2.1 $
 *
 *  	 \brief  Watchdog timer export routines of MEN VxWorks BSPs
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_wdt.h,v $
 * Revision 2.1  2005/07/05 12:28:36  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _SYS_WDT_H
#define _SYS_WDT_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
void sysWdtPing( void );
STATUS sysWdtSetTimeout( u_int32 timeoutMs );

#ifdef __cplusplus
	}
#endif

#endif /* SYS_WDT_H */

