/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: men_kbd.h
 *
 *       Author: uf
 *        $Date: 1998/05/29 15:13:09 $
 *    $Revision: 1.1 $
 *
 *  Description: export interface of men_keyboard module
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: men_kbd.h,v $
 * Revision 1.1  1998/05/29 15:13:09  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _MKBD_H
#define _MKBD_H

#ifdef __cplusplus
   extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define MKBD_MAXDEVNAME  40

#define MKBD_RAW         0
#define MKBD_LANGUAGE_US 1

/* special IO controls */
#define MKBD_CONV_MODE   0x1001

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern void   MKBD_Ident(     void );
extern STATUS MKBD_DrvInstall(char *devName, int  conversionMode );
extern STATUS MKBD_DrvRemove( void );
extern void   MKBD_Convert(   u_char scanCode, RING_ID rxRing );

#ifdef __cplusplus
   }
#endif

#endif/*_MKBD_H*/


