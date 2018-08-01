/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sys_smb.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/11/26 09:23:33 $
 *    $Revision: 2.3 $
 *
 *  	 \brief  SMB export routines of MEN VxWorks BSPs
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_smb.h,v $
 * Revision 2.3  2007/11/26 09:23:33  SVogel
 * added prototype of sysSmbWriteWordData
 *
 * Revision 2.2  2006/11/20 15:49:19  svogel
 * added sysSmbQuickCommand
 *
 * Revision 2.1  2005/06/23 08:33:28  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _SYS_SMB_H
#define _SYS_SMB_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

extern STATUS sysSmbInit( void );
extern int32 sysSmbWriteByte( u_int8 smbNum, u_int8 smbAddr, u_int8 data );
extern int32 sysSmbWriteTwoByte( u_int8 smbNum, u_int8 smbAddr, u_int8 data1,
						  		u_int8 data2);
extern int32 sysSmbReadByte( u_int8 smbNum, u_int8 smbAddr, u_int8 *dataP );
extern int32 sysSmbWriteReadTwoByte( u_int8 smbNum, u_int8 smbAddr, 
									 u_int8 inData,
									 u_int8 *outData1P, u_int8 *outData2P);

extern int32 sysSmbWriteWordData(   u_int8 smbNum, u_int8 smbAddr, u_int8 cmd, 
                                    u_int16 data);

extern int32 sysSmbQuickCommand(u_int8 smbNum, u_int8 smbAddr);
#ifdef __cplusplus
	}
#endif

#endif /* SYS_SMB_H */

