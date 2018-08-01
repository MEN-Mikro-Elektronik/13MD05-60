/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: usr_oss_intern.h
 *
 *       Author: uf
 *        $Date: 2006/06/08 14:18:46 $
 *    $Revision: 1.3 $
 * 
 *  Description: user oss internal defines...
 *     Switches: DBG
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_intern.h,v $
 * Revision 1.3  2006/06/08 14:18:46  ufranke
 * cosmetics
 *
 * Revision 1.2  2005/06/29 15:08:42  UFranke
 * cosmetics
 *
 * Revision 1.1  1999/08/31 10:53:32  Franke
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


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define  DBH UOS_DbgHdl

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern u_int32 UOS_DbgLev;
extern   DBG_HANDLE   *UOS_DbgHdl;
DBGCMD( extern char *UOS_ErrorStartStr; )
DBGCMD( extern char *UOS_ErrorLineStr; )
DBGCMD( extern char *UOS_ErrorEndStr; )

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

#  ifdef __cplusplus
      }
#  endif

#endif/*_OSS_INTERN_H*/
