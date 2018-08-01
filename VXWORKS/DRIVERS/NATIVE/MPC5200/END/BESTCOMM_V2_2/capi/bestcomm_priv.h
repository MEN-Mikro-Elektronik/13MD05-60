#ifndef __BESTCOMM_PRIV_H
#define __BESTCOMM_PRIV_H 1

/******************************************************************************
*
*       COPYRIGHT (c) 2001-2004 MOTOROLA INC.
*       ALL RIGHTS RESERVED
*
*       The code is the property of Motorola Semiconductor Products Sector.
*
*       The copyright notice above does not evidence any
*       actual or intended publication of such source code.
*
* Filename:     $Source: /Customizations/VxWorks-6.0/mpc5200/target/config/lite5200.pkr/bestComm/capi/bestcomm_priv.h,v $
* Author:       $Author: pkleiner $
* Locker:       $Locker:  $
* State:        $State: Exp $
* Revision:     $Revision: 1.1 $
*
******************************************************************************/

/*
 * Global variables necessitated by the TaskSetup_*() location in
 * separate files from the main code and the inline functions. These are
 * private data structures that should not be manipulated by API users.
 */

extern uint32           SramOffsetGlobal;
extern TaskBDIdxTable_t TaskBDIdxTable[MAX_TASKS];
extern int              TaskRunning[MAX_TASKS];
extern sint64			MBarPhysOffsetGlobal;

#endif	/* __BESTCOMM_PRIV_H */
