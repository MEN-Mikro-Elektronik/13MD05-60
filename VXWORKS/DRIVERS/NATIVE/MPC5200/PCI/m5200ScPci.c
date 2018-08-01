/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m5200ScPci.c
 *      Project: VxWorks for MPC5200
 *
 *       Author: uf/kp
 *        $Date: 2005/09/09 20:17:47 $
 *    $Revision: 1.3 $
 *
 *  Description: Support routines for PCI bus
 *				 This file should be included in sysLib.c
 *
 * All the following routines use SCPCI RX/TX interface to work around
 * Erratum #435.
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
 * $Log: m5200ScPci.c,v $
 * Revision 1.3  2005/09/09 20:17:47  CSchuster
 * updated to work together with m5200Pci.c
 *
 * Revision 1.2  2005/04/19 15:57:17  kp
 * First working rev.
 *
 * Revision 1.1  2005/04/12 10:17:45  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002..2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

LOCAL SEM_ID G_pciSpciLock;	/* semaphore to prevent concurr. access */

/******************************************************************************
*
* sysPciSpciInit2 - Init Spci PCI access = create access semaphore
*
* Called from sysHwInit2()
*
* RETURNS: OK or ERROR
*
* SEE ALSO: sysPciRead(), sysPciProgram(), sysPciWriteBlock(),
* sysPciReadBlock()
*/
STATUS sysPciSpciInit2( void )
{
	G_pciSpciLock = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
	if( G_pciSpciLock == NULL )
		return ERROR;

	return( OK );
}

/* function similiar to pciConfigBdfPack(bus, dev,fun) */
static u_int32 pciEncodeAddr(
	u_int32 bus,
	u_int32 dev,
	u_int32 func,
	u_int32 reg)
{
	u_int32 addr=0;
    u_int8 cfgTransactionType;

    /* set ADDR */
    if( bus != 0 /* local bus number */ )
    {
        /* IDSEL decoding will be done by a bridge */
        cfgTransactionType = 0x01;
        addr =  ((bus&0xFF) << 16)
                | ((dev & 0x1f)<< 11 )
                | ((func & 0x7) << 8 )
                | (reg & 0xfc )
                | cfgTransactionType;
    }
    else
    {
        u_int32 idsel;
        /* IDSEL decoding will be done here */
        cfgTransactionType = 0x00;

    	if( dev < 10 || 30 < dev )
    		goto CLEANUP;

		/* MPC5200 UM Table 10-8.
		   Type 0 Configuration Device Number to IDSEL Translation
		   to access own config space
		*/
		if( dev == 10 )
			dev = 0x1f;

        idsel = 1U<<dev;

        addr =  idsel
                | ((func & 0x7) << 8 )
                | (reg & 0xfc )
                | cfgTransactionType;
    }
 CLEANUP:
	return addr;
}


static void pciCfgWrite(
	u_int32 bus,
	u_int32 dev,
	u_int32 func,
	u_int32 reg,
	u_int32 val)
{
	MACCESS ma = (MACCESS)sysMbarAddrGet();
    u_int8 cfg_write_cmd = 0x0B;
    u_int32 addr, txStatus;

	if( semTake( G_pciSpciLock, WAIT_FOREVER ) == ERROR )
		return;

    MWRITE_D32( ma, MGT5200_SPCI_ENABLE_TX,
				MGT5200_SPCI_ENABLE_TX_RESET | MGT5200_SPCI_ENABLE_TX_MASTER);
    EIEIO_SYNC;

	/* clear status */
    MWRITE_D32(ma, MGT5200_SPCI_FIFO_STATUS_TX, 0x00ff0000 );
    MWRITE_D32(ma, MGT5200_SPCI_STATUS_TX,      0x01ff0000 );

	/* make sure fifo empty */
	MWRITE_D32(ma, MGT5200_SPCI_READ_POINTER_TX, 0 );
	MWRITE_D32(ma, MGT5200_SPCI_WRITE_POINTER_TX, 0 );

    EIEIO_SYNC;

	addr = pciEncodeAddr( bus, dev, func, reg );
	if( addr == 0 )
		goto CLEANUP;

    MWRITE_D32( ma, MGT5200_SPCI_START_ADDR_TX, addr );

    /* command PCI CFG WRITE */
    MWRITE_D32( ma, MGT5200_SPCI_CMD_TX,
                (cfg_write_cmd << MGT5200_SPCI_CMD_TX_CMD_SH)
                | ( 0xf0 << MGT5200_SPCI_CMD_TX_RETRY_SH)
                | ( 1 << MGT5200_SPCI_CMD_TX_MAXBEATS_SH)
              );


    MWRITE_D32( ma, MGT5200_SPCI_ENABLE_TX, MGT5200_SPCI_ENABLE_TX_MASTER );
    EIEIO_SYNC;


    /* fill FIFO (swap data, sync) */
	sysOutLong( (u_int32)ma + MGT5200_SPCI_FIFO_DATA_TX, val );

    /* WRITE size 4 */
    MWRITE_D32( ma, MGT5200_SPCI_PACKET_SIZE_TX,
                ( 4 << MGT5200_SPCI_PACKET_SIZE_TX_SH)
              );

	/* wait for transfer complete */
	{
		int tout=10000;

		while( (txStatus = (MREAD_D32(ma, MGT5200_SPCI_STATUS_TX ))
				& 0x01ff0000) == 0 ){
			if( --tout == 0 ){
				/*printf("***TOUT***\n");*/
				break;
			}
		}
	}


CLEANUP:
	semGive( G_pciSpciLock );
}



static void pciCfgRead(
	u_int32 bus,
	u_int32 dev,
	u_int32 func,
	u_int32 reg,
	u_int32 *valP)
{
	MACCESS ma = (MACCESS)sysMbarAddrGet();
    u_int8 cfg_read_cmd = 0x0a;
    u_int32 val = 0xFFFFFFFF;
    u_int32 addr;
    u_int32 rxStatus;

	if( semTake( G_pciSpciLock, WAIT_FOREVER ) == ERROR )
		return;

    MWRITE_D32( ma, MGT5200_SPCI_ENABLE_RX,
				MGT5200_SPCI_ENABLE_RX_RESET | MGT5200_SPCI_ENABLE_RX_MASTER);
    EIEIO_SYNC;

	/* clear STATUS */
    MWRITE_D32(ma, MGT5200_SPCI_FIFO_STATUS_RX, 0x00ff0000 );
    MWRITE_D32(ma, MGT5200_SPCI_STATUS_RX,      0x01ff0000 );

	/* make sure fifo empty */
	MWRITE_D32(ma, MGT5200_SPCI_READ_POINTER_RX, 0 );
	MWRITE_D32(ma, MGT5200_SPCI_WRITE_POINTER_RX, 0 );

    EIEIO_SYNC;

	addr = pciEncodeAddr( bus, dev, func, reg );
	if( addr == 0 )
		goto CLEANUP;


    MWRITE_D32( ma, MGT5200_SPCI_START_ADDR_RX, addr );

    /* PCI CFG READ - set command */
    MWRITE_D32( ma, MGT5200_SPCI_CMD_RX,
                (cfg_read_cmd << MGT5200_SPCI_CMD_RX_CMD_SH)
                | ( 0xf0 << MGT5200_SPCI_CMD_RX_RETRY_SH)
                | ( 1 << MGT5200_SPCI_CMD_RX_MAXBEATS_SH)
              );


    MWRITE_D32( ma, MGT5200_SPCI_ENABLE_RX, MGT5200_SPCI_ENABLE_RX_MASTER );
    EIEIO_SYNC;

    /* READ size 4 -> fire command */
    MWRITE_D32( ma, MGT5200_SPCI_PACKET_SIZE_RX,
                ( 4 << MGT5200_SPCI_PACKET_SIZE_RX_SH)
              );


	/* wait for transfer complete */
	{
		int tout=10000;

		while( ( (rxStatus = MREAD_D32(ma, MGT5200_SPCI_STATUS_RX ))
				 & 0x01ff0000) == 0 ){
			if( --tout == 0 ){
				/*printf("***TOUT***\n");*/
				break;
			}
		}
	}

    /* get value from FIFO (swap data, sync) */
	val = sysInLong( (u_int32)ma + MGT5200_SPCI_FIFO_DATA_RX );

	/* check PCI TRANSFER status */
	rxStatus = MREAD_D32(ma, MGT5200_SPCI_STATUS_RX );
	if( rxStatus != MGT5200_SPCI_STATUS_RX_NORMAL_TERM )
		val = 0xFFFFFFFF;

CLEANUP:
	*valP = val;
	/*printf("pciCfgRead: %x/%x/%x/%x = 0x%08x %08x (addr %x)\n",
	  bus,dev,func,reg,*valP,rxStatus,addr);*/
	semGive( G_pciSpciLock );
}

/******************************************************************************
*
* sysPciWrite -
*
* CAUTION: May cause side effects on read-modify-clear registers when
* called with byte/word size
*
* RETURNS: N/A
*/
LOCAL STATUS sysPciSpciWrite(
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,	  /* function number */
    int	offset,	  /* offset into the configuration space */
    int size,     /* size 1,2,4 byte */
	int value
	)
{
	u_int32 temp;
	int mask;
	int shift = 0;

	pciCfgRead( busNo, deviceNo, funcNo, offset & ~0x03, &temp );

	switch( size )
	{
		case 1:
			mask = 0xFF;
			shift = (offset & 0x03) * 8;
			break;
		case 2:
			mask = 0xFFFF;
			if( offset & 0x2 )
				shift = 16;
			break;
		case 4:
			mask = 0xFFFFFFFF;
			break;

		default:
			goto CLEANUP;
			break;
	}

	temp &= ~(mask << shift);
	temp |= value << shift;

	pciCfgWrite( busNo, deviceNo, funcNo, offset & ~0x03, temp );

CLEANUP:
	return OK;
}/*sysPciSpciWrite*/


/******************************************************************************
*
* sysPciRead -
*
* .
*
* RETURNS: N/A
*/
LOCAL STATUS sysPciSpciRead(
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,	  /* function number */
    int	offset,	  /* offset into the configuration space */
    int size,     /* size 1,2,4 byte */
	int *valueP
)
{
	u_int32 temp;
	int error = OK;
	int i;

	pciCfgRead( busNo, deviceNo, funcNo, offset & ~0x03, &temp );

	switch( size )
	{
		case 1:
			i = offset & 0x03;
			temp >>= (8*i);
			temp &= 0x0000FF;
			*(u_int8*)valueP = temp;
			break;
		case 2:
			if( offset & 0x02 )
				temp >>= 16;
			temp &= 0x0000FFFF;
			*(u_int16*)valueP = temp;
			break;
		case 4:
			*valueP = temp;
			break;

		default:
			error = ERROR;
			break;
	}

	return( error );
}/*sysPciSpciRead*/
