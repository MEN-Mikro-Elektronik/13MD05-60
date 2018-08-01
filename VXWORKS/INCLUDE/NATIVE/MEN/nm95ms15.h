/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: nm95ms15.h
 *
 *       Author: uf
 *        $Date: 1998/04/30 16:52:30 $
 *    $Revision: 1.1 $
 *
 *  Description: EEPROM functions for the ISA plug and play controller
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: nm95ms15.h,v $
 * Revision 1.1  1998/04/30 16:52:30  Franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _NM95MS15_H
#  define NM95MS15_H

#  ifdef __cplusplus
      extern "C" {
#  endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define NM95MS15_WR_RANGE_PNP_DATA		  1
#define NM95MS15_WR_RANGE_USER_BIOS		  2
#define NM95MS15_WR_RANGE_USER_VXWORKS	  3

#define NM95MS15_ERR_WR_RANGE	  0x20
#define NM95MS15_ERR_WR_DONE 	  0x21

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern u_int32 NM95MS15_ReadWord(      u_int32 addr, u_short *dataP );
extern u_int32 NM95MS15_WriteWord(     u_int32 addr, u_short data );
extern u_int32 NM95MS15_SetWriteRange( u_int32 range );
extern u_int32 NM95MS15_GetWriteRange( void );

#  ifdef __cplusplus
      }
#  endif

#endif/*_NM95MS15_H*/

