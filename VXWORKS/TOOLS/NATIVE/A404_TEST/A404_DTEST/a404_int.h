/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  a404_int.h
 *
 *      \author  dieter.pfeuffer@men.de
 *        $Date: 2006/02/17 15:53:14 $
 *    $Revision: 1.2 $
 *
 *       \brief  A404 FPGA design test function library - internal header file
 *
 *    \switches  -
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: a404_int.h,v $
 * Revision 1.2  2006/02/17 15:53:14  DPfeuffer
 * added writeRegWord( ), readRegWord() prototypes
 *
 * Revision 1.1  2005/12/07 10:58:03  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _A404_INT_H
#define _A404_INT_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysLib.h>
#include <intLib.h>
#include <iv.h>
#include <taskLib.h>

#include <MEN/men_typs.h>
#include <MEN/maccess.h>
#include <MEN/sysMenPci2Vme.h>

#include  <MEN/a404_reg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifdef DBG
	#define DBGPRINT( _x_ ) printf _x_
#else
	#define DBGPRINT( _x_ )
#endif

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
int32 writeRegByte( u_int32 base, u_int32 offs, u_int8 val );
int32 readRegByte( u_int32 base, u_int32 offs, u_int8 *val );
int32 writeRegWord( u_int32 base, u_int32 offs, u_int16 val );
int32 readRegWord( u_int32 base, u_int32 offs, u_int16 *val );

#ifdef __cplusplus
      }
#endif

#endif /* _A404_INT_H */
