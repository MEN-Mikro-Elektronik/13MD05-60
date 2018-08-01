/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *         \file a404_init.c
 *       \author dieter.pfeuffer@men.de
 *        $Date: 2006/03/16 15:46:38 $
 *    $Revision: 1.3 $
 *
 *        \brief A404 FPGA design test function library - initialization
 *
 *     Required: libraries: tbd.
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: a404_init.c,v $
 * Revision 1.3  2006/03/16 15:46:38  DPfeuffer
 * - a404DisableA32() re-fixed :-(
 *
 * Revision 1.2  2006/02/17 15:53:09  DPfeuffer
 * - a404EnableA32(), a404DisableA32(): bug fixed
 * - a404Init(): hwLoop parameter removed
 *
 * Revision 1.1  2005/12/07 10:58:00  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include "a404_int.h"

/***************************************************************************/
/** Detecting CME board in A24 space
 *
 *  Requirements:
 *    - A404 in compatibility mode
 *    OR 
 *    - A404 in enhanced mode, A24
 *    OR
 *    - old CME board
 *
 *  \param a24VmeBase	\IN  A24 base for VMEbus
 *  \param scarte		\IN  scarte3..0 (0x0..0xf)
 *
 *  \return		success (0..2) or error (-1)
 */
int32 a404DetectA24( u_int32 a24VmeBase, u_int32 scarte )
{
	int32 		ret;
	u_int32 	a24comp, a24enha;
	u_int8		regVal;

	a24comp = a24VmeBase + ((scarte & 0xf) << 20);
	a24enha = a24VmeBase + ((scarte & 0xc) << 20);
	DBGPRINT(("a24comp=0x%x, a24enha=0x%x\n", (int)a24comp, (int)a24enha));

	/*
	 * A404?
	 * Note: a24comp + A404_VME_CONF(0) = a24enha + A404_VME_CONF(1)
	 */
	/* assume compatibility mode */
	ret = readRegByte( a24comp, A404_VME_CONF(0), &regVal );

	if( (ret != 0) ||
		((regVal & A404_VME_CONF_IDMASK) != A404_VME_CONF_ID) ){
		/* assume enhanced mode */
		ret = readRegByte( a24enha, A404_VME_CONF(1), &regVal );
	}

	if( (ret == 0) &&
		((regVal & A404_VME_CONF_IDMASK) == A404_VME_CONF_ID) ){
		
		/* A404 in enhanced mode? */
		if( regVal & A404_VME_CONF_ENH ){
			printf("detected: A404 in enhanced mode (A24)\n");
			return 2;
		}
		/* A404 in compatibility mode */
		else{
			printf("detected: A404 in compatibility mode\n");
			return 1;
		}
	}

	/* CME board? */
	ret = readRegByte( a24comp, A404_TXCTRL(0,1), &regVal );
	if( ret == 0 ){
		printf("detected: old CME board\n");
		return 0;
	}
	
	printf("*** ERROR: no board detected\n");
	return -1;
}

/***************************************************************************/
/** Initialize CME board for transmission
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param prot		\IN  0: protocol disabled
 *                       1: protocol enabled
 *
 *  \return		success (0) or error (-1)
 */
int32 a404Init( u_int32 brdBase, u_int32 em, u_int32 prot )
{
	MACCESS	ma = (MACCESS)brdBase;
	u_int8	val;

	/* TX authorization (for old CME boards) */
	val = (prot ? A404_TXCTRL_PROTENBL : 0) | 0x19;
	MWRITE_D8( ma, A404_TXCTRL(em,1), val );
	MWRITE_D8( ma, A404_TXCTRL(em,2), val );

	taskDelay(1); /* required for old CME boards */

	/* RX authorization (for old CME boards) */
	MWRITE_D8( ma, A404_RXCTRL(em,1), 0x05 );
	MWRITE_D8( ma, A404_RXCTRL(em,2), 0x05 );

	/* config RX */
	MWRITE_D8( ma, A404_RXCTRL(em,1), A404_RXCTRL_FFRST_NO);
	MWRITE_D8( ma, A404_RXCTRL(em,2), A404_RXCTRL_FFRST_NO);

	taskDelay(1); /* required for A404 boards to clear the RXSTAT */

	/* clear RX status */
	MWRITE_D8( ma, A404_RXSTAT(em,1), 0x00 );
	MWRITE_D8( ma, A404_RXSTAT(em,2), 0x00 );
	
	/* clear TX status */
	val = A404_TXSTAT_DATALOST | A404_TXSTAT_CRCERR;
	MWRITE_D8( ma, A404_TXSTAT(em,1), val );
	MWRITE_D8( ma, A404_TXSTAT(em,2), val );	
	
	/* clear TEST0 status */
	val = A404_TEST0_DL_FSM | A404_TEST0_DL_INP | 	
			 A404_TEST0_DL_FULL | A404_TEST0_DL_RETRY |
			 A404_TEST0_CLR_ALL; 
	MWRITE_D8( ma, A404_TEST0(em,1), val );
	MWRITE_D8( ma, A404_TEST0(em,2), val );
	
	return 0;
}

/***************************************************************************/
/** Check lane connection
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *
 *  \return		em  | return |detected connection
 *              ----+--------+-------------------
 * 				0,1 |  0     | any
 *				0	|  1     | TX1->RX1, TX2->RX2
 *				0	|  2     | TX1->RX2, TX2->RX1
 *				0,1	| -1:    | none
 */
int32 a404CheckLanes( u_int32 brdBase, u_int32 em )
{
#define TEST_WORD_TX12	0xdead
#define TEST_WORD_TX1	0xbeef
#define TEST_WORD_TX2	0xfeed

	u_int16 testVal;
	MACCESS	ma = (MACCESS)brdBase;
	int32 rcv=0, txArxA=0, txArxB=0;

    /* zero rx ch 1/2 */
	MWRITE_D16( ma, A404_RX(em,1), 0x00 );
	MWRITE_D16( ma, A404_RX(em,2), 0x00 );

	/*--------------------+
    |  TX12               |
    +--------------------*/    
	MWRITE_D16( ma, A404_TX12(em), TEST_WORD_TX12 );
	
	testVal = MREAD_D16( ma, A404_RX(em,1) );
	if( testVal == TEST_WORD_TX12 ){
		printf("- RX1 connected to TX12\n"); 
		rcv++;
	} 		
	
	testVal = MREAD_D16( ma, A404_RX(em,2) );
	if( testVal == TEST_WORD_TX12 ){
		printf("- RX2 connected to TX12\n");
		rcv++;
	} 		

	if( !em ){

		/*--------------------+
	    |  TX1                |
	    +--------------------*/    
		MWRITE_D16( ma, A404_TX_CM(1), TEST_WORD_TX1 );
		
		testVal = MREAD_D16( ma, A404_RX(em,1) );
		if( testVal == TEST_WORD_TX1 ){
			printf("- RX1 connected to TX1\n");
			rcv++;
			txArxA++;
		}
		 		
		testVal = MREAD_D16( ma, A404_RX(em,2) );
		if( testVal == TEST_WORD_TX1 ){
			printf("- RX2 connected to TX1\n"); 		
			rcv++;
			txArxB++;
		}
					
		/*--------------------+
	    |  TX2                |
	    +--------------------*/    
		MWRITE_D16( ma, A404_TX_CM(2), TEST_WORD_TX2 );
		
		/* rx ch1 */
		testVal = MREAD_D16( ma, A404_RX(em,1) );
		if( testVal == TEST_WORD_TX2 ){
			printf("- RX1 connected to TX2\n"); 
			rcv++;
			txArxB++;
		}
				
		/* rx ch2 */
		testVal = MREAD_D16( ma, A404_RX(em,2) );
		if( testVal == TEST_WORD_TX2 ){
			printf("- RX2 connected to TX2\n"); 				
			rcv++;
			txArxA++;
		}
	
		if( txArxA == 2 )
			return 1;
			
		if( txArxB == 2 )
			return 2;		
	}
	
	/* no connection? */
	if( rcv == 0 ){
		printf("*** no connection detected\n"); 					
		return -1;
	}

	return 0;
}

/***************************************************************************/
/** Enable A32 space
 *
 *  Requirements:
 *    - A404 (no old CME board) 
 *    - enhanced mode
 *    - A32 space disabled
 *
 *  \param a24Base		\IN  A24 base
 *  \param a32Base		\IN  A32 base to set
 *                           0x0000 0000 .. [0x0040 0000] .. 0xffc0 0000
 *
 *  \return		success (0) or error (1)
 */
int32 a404EnableA32( u_int32 a24Base, u_int32 a32Base )
{
	return writeRegWord( a24Base, A404_VME_A32BASE_EM,
		A404_VME_A32BASE_ADDR(a32Base) | A404_VME_A32BASE_ENBL );
}

/***************************************************************************/
/** Disable A32 space
 *
 *  Requirements:
 *    - A404 (no old CME board)
 *    - enhanced mode
 *    - A32 space enabled
 *
 *  \param a32Base		\IN  A32 base
 *
 *  \return		success (0) or error (1)
 */
int32 a404DisableA32( u_int32 a32Base )
{
	/* Must be ONE byte access because a word access would be
	   splitted into TWO byte accesses for odd addresses and
	   the first access would disable the A32 space! */
	return writeRegByte( a32Base, A404_VME_A32BASE_EM,
		~A404_VME_A32BASE_ENBL );
}


