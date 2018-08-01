/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: systouchDrv.h
 *
 *       Author: rla
 *        $Date: 2006/09/09 17:29:52 $
 *    $Revision: 1.9 $
 *
 *  Description: Internal header for systouchDrv.c - Driver for touch panel
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systouchDrv.h,v $
 * Revision 1.9  2006/09/09 17:29:52  cs
 * removed definition of DBG_MYLEVEL, done in systouchdrv.c
 *
 * Revision 1.8  2006/03/22 15:09:04  cs
 * added:
 *     + IOCTL calls for reading temperature and AUX input (VBATT)
 * moved some defines (IOCTL calls, ...) to header file MEN/sysMenTouchDrv.h
 *
 * Revision 1.7  2006/01/17 13:46:11  cs
 * default DBG level set to 1 instead of 7 (less debugs)
 *
 * Revision 1.6  2005/12/01 19:10:24  cschuster
 * changed include file for 16Z044_DISP
 *
 * Revision 1.5  2005/07/19 15:06:06  CSchuster
 * added support for 16Z044_DISP graphic module
 * added support for WindML Touch Pointer device
 *
 * Revision 1.4  2005/04/12 16:56:17  kp
 * cosmetic
 *
 * Revision 1.3  2005/03/03 18:41:15  CSchuster
 * updated to use sysparam2
 *
 * Revision 1.2  2004/09/01 12:16:11  ufranke
 * cosmetics
 *
 * Revision 1.1  2003/04/10 17:21:33  Rlange
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _SYSTOUCHDRV_H
#define _SYSTOUCHDRV_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------------------------------------------+
 |	INCLUDES											              		|
 +--------------------------------------------------------------------------*/
#include "vxWorks.h"
#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/dbg.h>
#include <MEN/chameleon.h>
#include <MEN/maccess.h>
#include <MEN/sysparam2.h>
#include <MEN/em04touch.h>
#include <MEN/16z044_disp.h>
#include <MEN/sysMenTouchDrv.h>
#include "iv.h"
#include "errnoLib.h"
#include "ioLib.h"
#include "intLib.h"
#include "iosLib.h"
#include "config.h"
#include <stdio.h>
#include <cacheLib.h>
#include <string.h>
#include <taskLib.h>
#include <sysLib.h>
#include <selectLib.h>
#include <logLib.h>
#include <semLib.h>
#include "sysLib_men.h"

#ifdef SYSTOUCH_IRQ_FROM_PCI
	/* Standard VxWorks driver libraries */
#	include "drv/pci/pciConfigLib.h"   /* for PCI const */
#	include "drv/pci/pciIntLib.h"      /* for PCI interrupt */
#endif /* SYSTOUCH_IRQ_FROM_PCI */

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct 			/* TOUCH_DEV */
{
	DEV_HDR				devHdr;		/**< pointer to device header structure */
    BOOL				created;	/**< flag if device created */
    BOOL				dataAvailable;
    EM04TOUCH_INIT      tinit;		/**< touch init data structure */
    DBG_HANDLE			*dbh;		/**< debug handle */
    EM04TOUCH_HANDLE	*touch;		/**< pointer of TOUCH HANDLE */
    int					x;			/**< x coordinate */
    int					y;			/**< y coordinate */
    int					event;		/**< event (pressed or released) */
    SEL_WAKEUP_LIST		selWakeupList; 	/**< Select Wakeup List */
} TOUCH_DEV;

/**< shortcuts to globals */
#define G_ti	G_touchDev.tinit
#define G_th	G_touchDev.touch
#define G_dbh	G_touchDev.dbh

#define TOUCH_INTR_LEVEL		0

/*--------------------------------------------------------------------------+
 |	PROTOTYPES																|
 +--------------------------------------------------------------------------*/
int sysTouchDevCreate(char *name);
int sysTouchRemove(void);

#ifdef __cplusplus
	}
#endif

#endif	/* _SYSTOUCHDRV_H */


