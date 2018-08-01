/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m5200Pci.c
 *      Project: VxWorks for MPC5200
 *
 *       Author: cs
 *        $Date: 2006/03/15 16:07:58 $
 *    $Revision: 1.2 $
 *
 *  Description: Support routines for PCI bus
 *				 This file should be included in sysLib.c, after m5200ScPci.c
 *
 * Before anything other function sysPciInit() has to be called.
 * The other API functions are sysPciRead() and sysPciWrite(). They must
 * not be called from interrupt context!
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m5200Pci.c,v $
 * Revision 1.2  2006/03/15 16:07:58  UFRANKE
 * fixed
 *  - PCI for MPC5200 B1
 *
 * Revision 1.1  2005/09/09 20:17:46  CSchuster
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#include "vxWorks.h"
#include "config.h"
#include "WRS/m5200.h"
#include "drv/pci/pciConfigLib.h"

LOCAL STATUS sysPciSpecialCycle (int busNo, UINT32 message);
LOCAL STATUS sysPciCfgRead (int busNo, int deviceNo, int funcNo, int offset, int width, void * valueP);
LOCAL STATUS sysPciCfgWrite (int busNo, int deviceNo, int funcNo, int offset, int width, ULONG data);

LOCAL SEM_ID G_pciLock;	/* semaphore to prevent concurr. access */

/******************************************************************************
*
* sysPciInit - Init PCI access = create access semaphore
*
* Called from sysHwInit()
*
* RETURNS: OK or ERROR
*
* SEE ALSO: sysPciRead(), sysPciProgram(), sysPciWriteBlock(),
* sysPciReadBlock()
*/
STATUS sysPciInit( void )
{
	STATUS retVal;
	MACCESS ma = (MACCESS)sysMbarAddrGet();
	int i;

	/* clear any pending int */
	MWRITE_D32( ma, MGT5200_PCI_I_STAT, MGT5200_PCI_I_STAT__RE |
										MGT5200_PCI_I_STAT__IA |
										MGT5200_PCI_I_STAT__TA );

	/* disable CFG - enable IO access */
	MCLRMASK_D32( ma, MGT5200_PCI_CONFIG_ADDR, 0 );

	MWRITE_D32( ma, MGT5200_PCI_ARB, MGT5200_PCI_ARB_RESET );
    for (i=0; i<100000;i++) ;
	MWRITE_D32( ma, MGT5200_PCI_ARB, 0 );

	if( sysGetSvr() < 0xb1 ) 			/* MGT5200 special PCI config access */
		retVal = pciConfigLibInit( PCI_MECHANISM_0,
								  (ULONG)sysPciSpciRead,
								  (ULONG)sysPciSpciWrite,
								  (ULONG)NULL /* special cycle */ );
	else 							/* PCI should is bugfixed, normal access */
		retVal = pciConfigLibInit (PCI_MECHANISM_0,
								  (ULONG) sysPciCfgRead,
								  (ULONG) sysPciCfgWrite,
								  (ULONG) sysPciSpecialCycle);

	return retVal;
}

/******************************************************************************
*
* sysPciInit2 - Init PCI access = create access semaphore
*
* Called from sysHwInit2()
*
* RETURNS: OK or ERROR
*
* SEE ALSO: sysPciRead(), sysPciProgram(), sysPciWriteBlock(),
* sysPciReadBlock()
*/
STATUS sysPciInit2( void )
{
	STATUS status = ERROR;

	if( sysGetSvr() < 0xb1 )
		status = sysPciSpciInit2();
	else {
		G_pciLock = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		if( G_pciLock == NULL )
			status = ERROR;
	}

	return( status );
}


/***********************************************************************
*
* sysPciConfigRead - read from the PCI configuration space
*
* This routine reads either a byte, word or a long word specified by
* the argument <width>, from the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* SEE ALSO: sysPciConfigWrite()
*/

LOCAL STATUS sysPciCfgRead(
	int busNo,    /* bus number */
	int deviceNo, /* device number */
	int funcNo,   /* function number */
	int offset,   /* offset into the configuration space */
	int width,    /* width to be read */
	void *valueP )/* data read from the offset */
{
	u_int32 pciAddrLines;
	MACCESS ma = (MACCESS)sysMbarAddrGet();
	STATUS retStat = ERROR;
	int status;

	*(UINT32*)valueP = 0xFFFFFFFF;

	/* printf( "pciCfgRead: %2x/%2x/%2x/%2x (size=%d) = ",
			busNo, deviceNo, funcNo, offset, width); */

	if( deviceNo == 0x1f )
		goto donothing;

	pciAddrLines = pciConfigBdfPack (busNo, deviceNo, funcNo) | (offset & 0xfc);

	if( semTake( G_pciLock, WAIT_FOREVER ) == ERROR )
		return ERROR;

	/* prepare for config access */
	MWRITE_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN
				| pciAddrLines );
	EIEIO_SYNC;

	/* perform config access (swap data) */
	switch (width) {
	case 1: /* byte */
		*(u_int8*)valueP = MREAD_D8( (MACCESS)EM01CFG_PCI_IO_CFG_START, (offset & 0x3));
		break;
	case 2: /* word */
		*(u_int16*)valueP = MREAD_D16( (MACCESS)EM01CFG_PCI_IO_CFG_START, (offset & 0x2));
		*(u_int16*)valueP = OSS_SWAP16( *(u_int16*)valueP );
		break;
	case 4: /* long */
		*(u_int32*)valueP = MREAD_D32( (MACCESS)EM01CFG_PCI_IO_CFG_START, 0);
		*(u_int32*)valueP = OSS_SWAP32( *(u_int32*)valueP );
		break;
	default:
		*(u_int32*)valueP = 0xffffffff;
		retStat = ERROR;
		goto ret_sem_exit;
	}

	status = MREAD_D32( ma, MGT5200_PCI_I_STAT);
	MWRITE_D32( ma, MGT5200_PCI_I_STAT, MGT5200_PCI_I_STAT__IA); /* clear */
	retStat = ((status & MGT5200_PCI_I_STAT__IA) ? ERROR : OK);

	/* disable CFG - enable IO access */
	MCLRMASK_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN );
	EIEIO_SYNC;

ret_sem_exit:
	semGive( G_pciLock );

donothing:
	/* printf(" %08x\n", *(u_int32*)valueP ); */
	return retStat;
}

/***********************************************************************
*
* sysPciConfigWrite - write to the PCI configuration space
*
* This routine writes either a byte, word or a long word specified by
* the argument <width>, to the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* SEE ALSO: sysPciConfigRead()
*/

LOCAL STATUS sysPciCfgWrite(
	int   busNo,    /* bus number */
	int   deviceNo, /* device number */
	int   funcNo,   /* function number */
	int   offset,   /* offset into the configuration space */
	int   width,    /* width to write */
	ULONG data )     /* data to write */
{
	u_int32 pciAddrLines;
	MACCESS ma = (MACCESS)sysMbarAddrGet();
	STATUS retStat = ERROR;
	int status;

	printf( "pciCfgWrite: %2x/%2x/%2x/%2x (size=%d) = 0x%08x\n",
			busNo, deviceNo, funcNo, offset, width, (UINT)data);

	if( deviceNo == 0x1f )
		goto donothing;

	pciAddrLines = pciConfigBdfPack (busNo, deviceNo, funcNo) | (offset & 0xfc);

	if( semTake( G_pciLock, WAIT_FOREVER ) == ERROR )
		return ERROR;

	/* prepare for config access */
	MWRITE_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN
				| pciAddrLines );
	EIEIO_SYNC;

	/* perform config access (swap data) */
	switch (width) {
	case 1: /* byte */
		MWRITE_D8(  (MACCESS)EM01CFG_PCI_IO_CFG_START,
					(offset & 0x3),
					(u_int8)data);
		break;
	case 2: /* word */
		MWRITE_D16( (MACCESS)EM01CFG_PCI_IO_CFG_START,
					(offset & 0x2),
					OSS_SWAP16((u_int16)data));
		break;
	case 4: /* long */
		MWRITE_D32( (MACCESS)EM01CFG_PCI_IO_CFG_START, 0, OSS_SWAP32(data));
		break;
	default:
		retStat = ERROR;
		goto ret_sem_exit;
	}

	status = MREAD_D32( ma, MGT5200_PCI_I_STAT);
	MWRITE_D32( ma, MGT5200_PCI_I_STAT, MGT5200_PCI_I_STAT__IA); /* clear */
	retStat = ((status & MGT5200_PCI_I_STAT__IA) ? ERROR : OK);

	/* disable CFG - enable IO access */
	MCLRMASK_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN );
	EIEIO_SYNC;

ret_sem_exit:
	semGive( G_pciLock );

donothing:
	return retStat;
}

/***********************************************************************
*
* sysPciSpecialCycle - generate a special cycle with a message
*
* This routine generates a special cycle with a message.
*
* NOMANUAL
*
* RETURNS: OK
*/

LOCAL STATUS sysPciSpecialCycle(
	int     busNo,
	UINT32  message)
{
	int deviceNo    = 0x0000001f;
	int funcNo      = 0x00000007;
	MACCESS ma = (MACCESS)sysMbarAddrGet();
	u_int32 pciAddrLines;


	if (busNo != 0)
	return ERROR;

	pciAddrLines = pciConfigBdfPack (busNo, deviceNo, funcNo);
	if( semTake( G_pciLock, WAIT_FOREVER ) == ERROR )
		return ERROR;


	/* prepare for config access */
	MWRITE_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN
				| pciAddrLines );
	EIEIO_SYNC;

	/* perform config access (swap data) */
	MWRITE_D32( (MACCESS)EM01CFG_PCI_IO_CFG_START, 0, OSS_SWAP32(message) );
	EIEIO_SYNC;

	/* disable CFG - enable IO access */
	MCLRMASK_D32( ma, MGT5200_PCI_CONFIG_ADDR, MGT5200_PCI_CONFIG_ADDR__EN );
	EIEIO_SYNC;

	semGive( G_pciLock );

	return (OK);
}
