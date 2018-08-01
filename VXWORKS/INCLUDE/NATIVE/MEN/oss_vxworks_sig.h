/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: oss_vxworks_sig.h
 *
 *       Author: rla/uf
 *        $Date: 2006/06/02 09:52:20 $
 *    $Revision: 2.3 $
 *
 *  Description: UOS/OSS for VxWorks signal handling interface
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_vxworks_sig.h,v $
 * Revision 2.3  2006/06/02 09:52:20  ufranke
 * changed
 *  - completely reworked
 *
 * Revision 2.2  2005/06/29 15:23:17  UFranke
 * cosmetics
 *
 * Revision 2.1  2003/04/04 12:12:04  Rlange
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003..2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _OSS_VXWORKS_SIG_H
#define _OSS_VXWORKS_SIG_H

#ifdef __cplusplus
   extern "C" {
#endif


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern int32 OSS_VxSigMsgQInstall( int taskId, int *kernelTidP, int maxMsgs, MSG_Q_ID *kernelMsgQIdP );
extern int32 OSS_VxSigMsgQRemove(  int taskId );

#ifdef __cplusplus
   }
#endif
#endif /*_OSS_VXWORKS_SIG_H*/







