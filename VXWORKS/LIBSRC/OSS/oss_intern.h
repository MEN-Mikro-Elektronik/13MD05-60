/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: oss_intern.h
 *
 *       Author: uf
 *        $Date: 2008/09/26 11:49:08 $
 *    $Revision: 1.5 $
 *
 *  Description: oss internal defines...
 *     Switches: DBG
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_intern.h,v $
 * Revision 1.5  2008/09/26 11:49:08  ufranke
 * R: SMP support
 * M: _OSS_DataUn/Lock() added
 *
 * Revision 1.4  2008/09/04 15:38:03  ufranke
 * R: diab compiler support
 * M: cosmetics
 *
 * Revision 1.3  2005/06/29 15:14:44  UFranke
 * added
 *  - missing function prototypes
 *
 * Revision 1.2  2003/04/04 12:09:29  Rlange
 * added prototypes for init/exit vxWorks signal calls
 *
 * Revision 1.1  1999/05/05 11:11:08  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _OSS_INTERN_H
#  define _OSS_INTERN_H

#  ifdef __cplusplus
      extern "C" {
#  endif

#include <vxBusLib.h>
/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

#ifdef DBG
	#define  DBH OSS_DbgHdl
#endif /*DBG*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern u_int32 OSS_DbgLev;

#ifdef DBG
	extern   DBG_HANDLE   *OSS_DbgHdl;
#endif /*DBG*/
DBGCMD( extern char *OSS_ErrorStartStr; )
DBGCMD( extern char *OSS_ErrorLineStr;  )
DBGCMD( extern char *OSS_ErrorEndStr;   )

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern void _OSS_DataLock(   void );
extern void _OSS_DataUnlock( void );
#ifdef OSS_VXBUS_SUPPORT
extern VXB_DEVICE_ID sysGetPciCtrlID(int instance);
#endif




#  ifdef __cplusplus
      }
#  endif

#endif/*_OSS_INTERN_H*/

