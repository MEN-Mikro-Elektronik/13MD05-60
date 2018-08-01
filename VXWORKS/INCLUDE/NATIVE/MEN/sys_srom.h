/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sys_srom.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2005/06/23 08:33:29 $
 *    $Revision: 2.1 $
 *
 *  	 \brief  SROM/EEPROM exported routines of MEN VxWorks BSPs
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_srom.h,v $
 * Revision 2.1  2005/06/23 08:33:29  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _SYS_SROM_H
#define _SYS_SROM_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

extern int32 sysSromRead( int busNo, u_int8 smbAddr,	
						  u_int16 devAddr, u_int8 *valP );

extern int32 sysSromProgram( int busNo,  u_int8 smbAddr, u_int16 devAddr,	
							 u_int8 val );

extern int32 sysSromWriteBlock(
	void *osh, 
	int smbBus,
	int smbAdr,
	int offset,
	u_int8 *buf,
	int bufLen);

extern int32 sysSromReadBlock( 
	void *osh, 
	int smbBus,
	int smbAdr,
	int offset,
	u_int8 *buf,
	int bufLen);

#ifdef __cplusplus
	}
#endif

#endif /* SYS_SROM_H */

