/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *         \file a404_init.c
 *       \author dieter.pfeuffer@men.de
 *        $Date: 2006/02/17 15:53:12 $
 *    $Revision: 1.2 $
 *
 *        \brief A404 FPGA design test function library - misc
 *
 *     Required: libraries: tbd.
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: a404_misc.c,v $
 * Revision 1.2  2006/02/17 15:53:12  DPfeuffer
 * - writeRegWord(), readRegWord() added
 * - a404TxstatGetClr(): A404_TXSTAT_CRCERR added
 * - a404RxstatGetClr(): A404_TEST0 debug register check implemented
 *
 * Revision 1.1  2005/12/07 10:58:02  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include "a404_int.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define SWAP16(word)	(((word)>>8) | ((word)<<8))

/***************************************************************************/
/**  helper function for write byte
 */
int32 writeRegByte( u_int32 base, u_int32 offs, u_int8 val )
{
	u_int8		*addr;
	int32		ret;
		
	addr = (u_int8*)base + offs;
	ret = sysVmeProbe( addr, VX_WRITE, 1, &val );
	if( ret != 0 )
		printf("*** write access to 0x%x failed\n", (int)addr);
	else
		DBGPRINT(("writeRegByte: *0x%x = 0x%02x\n", (int)addr, val));

	return ret;
}

/***************************************************************************/
/**  helper function for write word
 */
int32 writeRegWord( u_int32 base, u_int32 offs, u_int16 val )
{
	u_int8		*addr;
	int32		ret;
		
	addr = (u_int8*)base + offs;
	val = SWAP16(val);
	ret = sysVmeProbe( addr, VX_WRITE, 2, (u_int8*)&val );
	if( ret != 0 )
		printf("*** write access to 0x%x failed\n", (int)addr);
	else
		DBGPRINT(("writeRegWord: *0x%x = 0x%04x\n", (int)addr, val));

	return ret;
}

/***************************************************************************/
/**  helper function for read byte
 */
int32 readRegByte( u_int32 base, u_int32 offs, u_int8 *val )
{
	u_int8		*addr;
	int32		ret;
		
	addr = (u_int8*)base + offs;
	ret = sysVmeProbe( addr, VX_READ, 1, val );
	if( ret != 0 )
		printf("*** read access from 0x%x failed\n", (int)addr);
	else
		DBGPRINT(("readRegByte : *0x%x = 0x%02x\n", (int)addr, *val));

	return ret;
}

/***************************************************************************/
/**  helper function for read word
 */
int32 readRegWord( u_int32 base, u_int32 offs, u_int16 *val )
{
	u_int8		*addr;
	int32		ret;
		
	addr = (u_int8*)base + offs;
	ret = sysVmeProbe( addr, VX_READ, 2, (u_int8*)val );
	if( ret != 0 )
		printf("*** read access from 0x%x failed\n", (int)addr);
	else
		DBGPRINT(("readRegWord : *0x%x = 0x%04x\n", (int)addr, *val));
	
	*val = SWAP16(*val);

	return ret;
}

/***************************************************************************/
/** Read byte from CME board register
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param reg		\IN  register offset (without channel offset)
 *
 *  \return		read value (0x0000..0xffff) or error (-1)
 */
int32 a404ByteRegRead( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 reg )
{
	int32	ret;
	u_int8	regVal; 	 

	ret = readRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, &regVal );
	if( ret == 0 ){
		return regVal;
	} else{
		printf("*** read access failed\n");
		return -1;
	}
}

/***************************************************************************/
/** Write byte to CME board register
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param reg		\IN  register offset (without channel offset)
 *  \param val		\IN  value to write
 *
 *  \return		success (0) or error (-1)
 */
int32 a404ByteRegWrite( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 reg, u_int32 val )
{
	int32	ret;
	
	ret = writeRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, val );
	if( ret != 0 )
		return -1;
		
	return 0;
}

/***************************************************************************/
/** Set mask in CME board register
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param reg		\IN  register offset (without channel offset)
 *  \param mask		\IN  mask to set
 *
 *  \return		success (0) or error (-1)
 */
int32 a404ByteRegSetmask( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 reg, u_int32 mask )
{
	int32	ret;
	u_int8 	val;	

	ret = readRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, &val );
	if( ret != 0 ){
		return -1;
	}

	val |= mask;
	
	ret = writeRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, val );

	return ret;
}

/***************************************************************************/
/** Clear mask in CME board register
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param reg		\IN  register offset (without channel offset)
 *  \param mask		\IN  mask to set
 *
 *  \return		success (0) or error (-1)
 */
int32 a404ByteRegClrmask( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 reg, u_int32 mask )
{
	int32	ret;
	u_int8 	val;	

	ret = readRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, &val );
	if( ret != 0 ){
		return -1;
	}

	val &= ~mask;
	
	ret = writeRegByte( brdBase, A404_REG(em) + A404_CH(ch) + reg, val );

	return ret;
}

/***************************************************************************/
/** Get and clear RXSTAT1/2
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  channel 1 or 2
 *
 *  \return		gotten value of RXSTAT1/2
 */
int32 a404RxstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch )
{
	u_int8 regVal, fifoMask, fifoFlags, test0;
	MACCESS	ma = (MACCESS)brdBase;

	/* get */
	regVal = MREAD_D8( ma, A404_RXSTAT(em,ch) );
	
	printf(" RXSTAT%d=0x%02x\n", (int)ch, regVal);
	regVal & A404_RXSTAT_CONNECT 	? printf(" - CONNECTED\n") : 0;
	regVal & A404_RXSTAT_CRCERR 	? printf(" - RCV_CRC_ERR\n") : 0;
	regVal & A404_RXSTAT_DATALOST 	? printf(" - RX_DATA_LOST\n") : 0;

	/* evaluate FIFO state */
	fifoMask = A404_RXSTAT_FFEMPTY_NO | A404_RXSTAT_FFFULL_NO;
	fifoFlags = regVal & fifoMask;

	if( fifoFlags == 
		(A404_RXSTAT_FFEMPTY_NO | A404_RXSTAT_FFFULL_NO) ){
		printf(" - FIFO: contains data (RCV_FF_EMPTY_n | RCV_FF_FULL_n)\n");
	}
	else if( fifoFlags ==
		A404_RXSTAT_FFEMPTY_NO ){
		printf(" - FIFO: full (RCV_FF_EMPTY_n)\n");
	}
	else if( fifoFlags ==
		A404_RXSTAT_FFFULL_NO ){
		printf(" - FIFO: empty (A404_RXSTAT_FFFULL_NO)\n");
	}
	else{ 
		printf(" - FIFO: *** illegal state (full AND empty)\n");
	}
	
	/* clear */
	MWRITE_D8( ma, A404_RXSTAT(em,ch), 0x00 );
	
	/* get */
	test0 = MREAD_D8( ma, A404_TEST0(em,ch) );
	
	if( test0 ){
		printf(" LVDSTEST%d0=0x%02x\n", (int)ch, test0);
		test0 & A404_TEST0_DL_FSM 	? printf(" - DATA_LOST_FSM\n") : 0;
		test0 & A404_TEST0_DL_INP 	? printf(" - DATA_LOST_INP\n") : 0;
		test0 & A404_TEST0_DL_FULL 	? printf(" - DATA_LOST_FULL\n") : 0;
		test0 & A404_TEST0_DL_RETRY ? printf(" - DATA_LOST_RETRY\n") : 0;
		test0 & A404_TEST0_CLR_ALL 	? printf(" - CLR_ALL_SENT\n") : 0;
		
		/* clear */
		MWRITE_D8( ma, A404_TEST0(em,ch), test0 );
	}
	
	return regVal;
}

/***************************************************************************/
/** Get and clear TXSTAT1/2
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  channel 1 or 2
 *
 *  \return		gotten value of TXSTAT1/2
 */
int32 a404TxstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch )
{
	u_int8 regVal;
	MACCESS	ma = (MACCESS)brdBase;

	/* get */
	regVal = MREAD_D8( ma, A404_TXSTAT(em,ch) );
	
	printf(" TXSTAT%d=0x%02x\n", (int)ch, regVal);
	regVal & A404_TXSTAT_DATALOST 	? printf(" - TX_DATA_LOST\n") : 0;
	regVal & A404_TXSTAT_CRCERR 	? printf(" - TX_CRCERR\n") : 0;
	
	/* clear DATALOST and CRCERR */
	MWRITE_D8( ma, A404_TXSTAT(em,ch), A404_TXSTAT_DATALOST |
	                                   A404_TXSTAT_CRCERR );
	
	return regVal;
}

/***************************************************************************/
/** Toggle LEDs
 *
 *  Requirements:
 *    - A404 (no old CME board)
 *
 *  \param base			\IN  A24/A32 base
 *  \param em			\IN  0=compatibility, 1=enhanced mode
 *
 *  \return		success (0) or error (1)
 */
int32 a404ToggleLeds( u_int32 base, u_int32 em )
{
	int32 		ret=0;
	u_int8		val, old;
	u_int8		c, l;
	
	/* for each channel */
	for( c=1; c<=2; c++ ){
	
		/* save current state */		
		if( (ret = readRegByte( base, A404_TEST(em,c), &old )) )
			return ret;

		/* switch all LEDs off */
		val = old & ~0x1f;
		if( (ret = writeRegByte( base, A404_TEST(em,c), val )) )
			return ret;
				
		/* switch LEDs on (one by one) */
		val = 0x00;
		for( l=0; l<5; l++ ){
			val |= (1 << l);
			taskDelay(30);
			if( (ret = writeRegByte( base, A404_TEST(em,c), val )) )
				return ret;						
		}
		
		/* restore old state */
		taskDelay(90);
		if( (ret = writeRegByte( base, A404_TEST(em,c), old )) )
			return ret;
	}	

	return ret;
}

/***************************************************************************/
/** RX FIFO test
 *
 *  Requirements:
 *    - A404 (no old CME board)
 *
 *  \param base			\IN  A24/A32 base
 *  \param em			\IN  0=compatibility, 1=enhanced mode
 *  \param rx			\IN  rx channel
 *
 *  \return		success (0) or error (1)
 */
int32 a404RxFifoTest( u_int32 base, u_int32 em, u_int32 rx )
{
	int32 		ret=0, count=0;
	u_int8 		rxStat;
	MACCESS		ma = (MACCESS)base;

	/* stop SDRAM accesses from LVDS-receiver */
	MSETMASK_D8( ma, A404_TEST(em,rx), A404_TEST_RAMDIS );
	
	/* while RCV_FF_FULL_n=1 */
	while( A404_RXSTAT_FFFULL_NO &
		(rxStat = MREAD_D8( ma, A404_RXSTAT(em,rx))) ){
		
		/* DATA_LOST must be '0' */
		if( rxStat & A404_RXSTAT_DATALOST ){
			printf("*** RXSTAT%lu=0x%x (DATALOST)\n",
				rx, rxStat);
			ret = -1;
			goto CLEANUP;
		}
		
		/* write */
		MWRITE_D8( ma, A404_TX12(em), 0xab );
		count++;
		
		/* limit */
		if( count > 128 ){
			printf("*** %ld bytes written but FIFO not full\n",
				count);
			ret = -1;
			goto CLEANUP;
		}			

	}

	/* check count */
	if( count <= 8 ){
		printf("*** only %ld bytes written but FIFO already full\n",
			count);
		ret = -1;
		goto CLEANUP;
	}			

	printf("FIFO full after %ld bytes written\n", count);

CLEANUP:
	/* re-enable SDRAM accesses from LVDS-receiver */
	MCLRMASK_D8( ma, A404_TEST(em,rx), A404_TEST_RAMDIS );
	
	return ret;
}


