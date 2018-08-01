/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: dbg_vx_rtp.h
 *
 *       Author: uf
 *        $Date: 2006/06/02 09:56:02 $
 *    $Revision: 2.1 $
 *
 *  Description: VxWorks User Mode macros to display driver debug messages
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: dbg_vx_rtp.h,v $
 * Revision 2.1  2006/06/02 09:56:02  ufranke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _DBG_VX_RTP_H
#define _DBG_VX_RTP_H

#ifdef __cplusplus
   extern "C" {
#endif

#include <stdio.h>

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

#define DBG_WRITE_DEFINED_BY_DBG_OS_H
#define DBG_Write(dbh,fmt,args...) printf( fmt, ## args )

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/* none */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern void DBG_DisableOverwrite( void );


#ifdef __cplusplus
   }
#endif
#endif /*_DBG_VX_RTP_H*/
