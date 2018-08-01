/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sys_srom_24c0x.c
 *      Project: VxWorks BSPs
 *
 *       Author: kp
 *        $Date: 2006/09/14 11:45:00 $
 *    $Revision: 1.3 $
 *
 *  Description: Support routines for 24C0x I2C EEPROMs
 *				 This file should be included in sysLib.c
 *
 *				 Requires sysSmbXXX API to access SM bus
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_srom_24c0x.c,v $
 * Revision 1.3  2006/09/14 11:45:00  cs
 * fixed for apigen (vxW6.x):
 *    - function headers and parameter definitions/comments
 *
 * Revision 1.2  2006/02/22 11:29:51  ts
 * Function header edited for html generation
 * (vxW6.2 make man)
 *
 * Revision 1.1  2005/06/23 08:33:54  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/


/*
 * semaphore to prevent concurr. access.
 * Note that a single semaphore is used for ALL EEPROMs in system
 */
LOCAL SEM_ID G_sromLock;

/***************************************************************************
*
* sysSromInit - Init EEPROM access, create access semaphore.
*
* Called from sysHwInit2()
*
* RETURNS: 0 or ERROR
*
* SEE ALSO: sysSromRead(), sysSromProgram(), sysSromWriteBlock(),
* sysSromReadBlock()
*/
STATUS sysSromInit( void )
{
	G_sromLock = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
	if( G_sromLock == NULL )
		return ERROR;

	return( OK );
}


/**********************************************************************
* sysSromReadBlock - Read block from EPROM
*
* Suitable to be used as callback for SYSPARAM
* When offset is higher then 256, the next smbAdr is used
*
* RETURNS: 0 on success, or SMB_ERR_xx error code
*/
int32 sysSromReadBlock(
	void *osh, 		/* OSS handle (NULL on VXWORKS) */
	int smbBus,		/* bus number (0..x) */
	int smbAdr,		/* SMB address where first EPROM block is located */
	int offset,		/* byte offset within EEPROM */
	u_int8 *buf,	/* receives data read */
	int bufLen		/* number of bytes to read */
)
{
	u_int32 i;
	int32 error=0;
	u_int8 smbRealAddr=0, devAddr=0;
	int lastSmbRealAddr=1;

 	for( i=0; i<(u_int32)bufLen; i++ )		/* preset entire block with 0xff */
		buf[i] = 0xff;

	if( semTake( G_sromLock, WAIT_FOREVER ) == ERROR )
		return -1;

	/* read from EEPROM */
	for( i=0; i<(u_int32)bufLen; i++, offset++ ){
		smbRealAddr = smbAdr | ((offset>>7) & 0xe);
		devAddr = offset & 0xff;

		/*
		 * only apply new device address initially or if the SMB address
		 * crossed device boundary
		 */
		if( lastSmbRealAddr != smbRealAddr ){
			if( (error = sysSmbWriteByte( smbBus, smbRealAddr, devAddr )))
				break;
			lastSmbRealAddr = smbRealAddr;
		}

		if( (error = sysSmbReadByte( smbBus, smbRealAddr, buf++ )))
			break;
	}

	semGive( G_sromLock );
	return error;
}

/**********************************************************************/
/* sysSromWriteBlock - Write block to EEPROM
*
* When offset is higher then 256, the next smbAdr is used
*
* RETURNS: 0 on success or SMB_ERR_xx error code or -1
*/
int32 sysSromWriteBlock(
	void *osh,			/* OSS handle (NULL on VXWORKS) */
	int smbBus,			/* bus number (0..x) */
	int smbAdr,			/* SMB address where first EPROM block is located */
	int offset,			/* byte offset within EEPROM */
	u_int8 *buf,		/* data to be written */
	int bufLen			/* number of bytes to write */
)
{
	u_int32 i;
	int32 error=0, timeout;
	u_int8 smbRealAddr=0, devAddr=0;

	for( i=0; i<(u_int32)bufLen; i++, offset++, buf++ ){
		smbRealAddr = smbAdr | ((offset>>7) & 0xe);
		devAddr = offset & 0xff;

		if( semTake( G_sromLock, WAIT_FOREVER ) == ERROR )
			return -1;

		if( (error = sysSmbWriteTwoByte( smbBus, smbRealAddr, devAddr, *buf )))
			break;

		timeout = 1000;
		/* wait for programming complete */
		while ((error = sysSmbWriteByte( smbBus, smbRealAddr, devAddr )))
		{
			OSS_Delay( osh, 1 );
			if( --timeout == 0){
				break;
			}
		}

		semGive( G_sromLock );

		/* verify */
		if( error == 0 ){
			u_int8 rv;
			sysSromReadBlock( osh, smbBus, smbAdr, offset, &rv, 1 );
			if( *buf != rv ){
				error = -1;
				break;
			}

		}
	}

	return error;
}




/******************************************************************************
*
* sysSromProgram - Program single byte in EEPROM (24c04).
*
* Waits until programming ready and verifies programmed byte
*
* RETURNS: 0 or SMB lib error code
*
* SEE ALSO: sysSromRead(), sysSromWriteBlock()
*/
int32 sysSromProgram(
	int    busNo,				/* SMB bus number */
	u_int8 smbAddr,				/* Address on SMB */
	u_int16 devAddr,			/* Address within device */
	u_int8 val					/* byte to be programmed */
)
{
	return sysSromWriteBlock( NULL, busNo, smbAddr, devAddr, &val, 1 );
}

/******************************************************************************
*
* sysSromRead - Read single byte from EEPROM (24c04).
*
* RETURNS: 0 or SMB lib error code
*
* SEE ALSO: sysSromProgram(), sysSromReadBlock()
*/
int32 sysSromRead(
	int    busNo,				/* SMB bus number */
	u_int8 smbAddr,				/* Address on SMB */
	u_int16 devAddr,			/* Address within device */
	u_int8 *valP				/* place to store read value */
)
{
	return sysSromReadBlock( NULL, busNo, smbAddr, devAddr, valP, 1 );
}

