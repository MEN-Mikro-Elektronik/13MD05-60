/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_task.c
 *      Project: Low Level Driver and MDIS 4.x
 *
 *      $Author: ufranke $
 *        $Date: 2009/03/31 09:36:37 $
 *    $Revision: 1.3 $
 *
 *  Description: task routines
 *               system independend functions using os-functions
 *               for LowLevel Driver, MDIS and other
 *
 *     Required:
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_task.c,v $
 * Revision 1.3  2009/03/31 09:36:37  ufranke
 * cosmetics
 *
 * Revision 1.2  1999/08/30 11:05:21  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:15  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
/*-----------------------------------------+
|  INCLUDES                                |
+------------------------------------------*/
#include <MEN/men_typs.h>

#include <vxWorks.h>
#include <taskLib.h>

#include <MEN/mdis_err.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/



/*************************** OSS_GetPid *************************************
 *
 *  Description:  Get current task id.
 *
 *         Note:  Task ID is invalid if called at interrupt level.
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *  Output.....:  return  task id
 *  Globals....:  -
 ****************************************************************************/
u_int32 OSS_GetPid(OSS_HANDLE *oss)
{
	int tid;
    DBGCMD( static const char functionName[] = "OSS_GetPid"; )

    DBGWRT_1((DBH,"%s()\n", functionName));
	tid = taskIdSelf();
    DBGWRT_2((DBH," tid=0x%08x\n", tid));

	return(tid);
}/*OSS_GetPid*/

