/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *         \file a404_init.c
 *       \author dieter.pfeuffer@men.de
 *        $Date: 2005/12/07 10:58:01 $
 *    $Revision: 1.1 $
 *
 *        \brief A404 FPGA design test function library - IRQ stuff
 *
 *     Required: libraries: tbd.
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: a404_irq.c,v $
 * Revision 1.1  2005/12/07 10:58:01  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#include "a404_int.h"

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
typedef struct{
	u_int8	registerred;	
	u_int8	vec;
	u_int8	installed;
	u_int8	lev;
	u_int32	count;
} IRQV;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
IRQV G_irqv[256];

/***************************************************************************/
/** IRQ enable/disable 
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param lev		\IN  IRQ level 1..7
 *  \param enable	\IN  1: enable, 0: disable IRQ
 *
 *  \return		success (0) or error (1)
 */
int32 a404RxIrqEnable( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 lev, u_int32 enable )
{
	MACCESS	ma = (MACCESS)brdBase;
		
	if( enable )
		MCLRMASK_D8( ma, A404_RXMASQ(em,ch), 1<<lev );
	else
		MSETMASK_D8( ma, A404_RXMASQ(em,ch), 1<<lev );
	
	return 0;
}

/***************************************************************************/
/** IRQ set vector 
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param vec		\IN  IRQ vector 0..255
 *
 *  \return		success (0) or error (1)
 */
int32 a404RxSetIrqVector( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 vec )
{
	MACCESS	ma = (MACCESS)brdBase;
		
	MWRITE_D8( ma, A404_RXVECT(em,ch), vec );
	
	return 0;
}

/***************************************************************************/
/** IRQ get pending state
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param lev		\IN  IRQ level 1..7
 *
 *  \return		0: not pending, 1: pending
 */
int32 a404RxGetIrqPending( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 lev )
{
	u_int8 val;
	
	MACCESS	ma = (MACCESS)brdBase;
		
	val = MREAD_D8( ma, A404_RXIRQ(em,ch) );
	
	return (val>>lev) & 1;
}

/***************************************************************************/
/** IRQ send 
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  0: common, 1/2: channel specific register
 *  \param lev		\IN  IRQ level 1..7
 *
 *  \return		success (0) or error (1)
 */
int32 a404TxIrqSend( u_int32 brdBase, u_int32 em, u_int32 ch, u_int32 lev )
{
	MACCESS	ma = (MACCESS)brdBase;
		
	MWRITE_D8( ma, A404_TXCOM(em,ch), A404_TXCOM_IRQ(lev) );
	
	return 0;
}

/***************************************************************************/
/**  ISR - don't use printf !!!
 */
void a404Isr( int ctxtP )
{
	IRQV 	*irqvP = (IRQV*)ctxtP;

  	DBGPRINT(("a404Isr: ctxtP=0x%x\n", ctxtP));

	// not installed?
	if( irqvP->installed == 0 ){
		return;
	}

  	irqvP->count++;  	   	  	
}

/***************************************************************************/
/** Install ISR 
 *
 *  \param vec		\IN  IRQ vector 0..255
 *  \param lev		\IN  IRQ level 1..7
 *
 *  \return		success (0) or error (1)
 */
int a404InstallIsr( u_int8 vec, u_int8 lev )
{
	int 	ctxt;
	
	// not yet registerred?
	if( G_irqv[vec].registerred == 0 ){		
		DBGPRINT(("G_irqv[vec].registerred == 0\n"));
	
		ctxt = (int)(&G_irqv[vec]);
	  	DBGPRINT(("ctxt=0x%x\n", ctxt));
	  		
		if( intConnect( INUM_TO_IVEC((int)vec),
	   		a404Isr, ctxt) != 0 ){
			printf("*** intConnect (lev=%d, vec=%d) failed\n",
	    		lev, vec);
	    	return 1;
	  	}
	  	
	  	G_irqv[vec].registerred = 1; 	
		G_irqv[vec].vec = vec;
	}

	DBGPRINT(("G_irqv[vec].registerred = 1\n"));	

	// set context
	G_irqv[vec].installed = 1;
	G_irqv[vec].lev = lev;
	G_irqv[vec].count = 0;

	DBGPRINT(("G_irqv[vec].installed = 1\n"));  	

  	sysIntEnable( lev );
	  
  	return 0;
}

/***************************************************************************/
/** Uninstall ISR 
 *
 *  \param vec		\IN  IRQ vector 0..255
 *
 *  \return		success (0) or error (1)
 */
int a404UnInstallIsr( u_int8 vec )
{
	
  	sysIntDisable( G_irqv[vec].lev );
  	G_irqv[vec].installed = 0;
  	
	DBGPRINT(("G_irqv[vec].installed = 0\n"));  	
  
  	return 0;
}

/***************************************************************************/
/**  Print ISR info 
 *
 *  \param vec		\IN  IRQ vector 0..255
 *
 *  \return		success (0) or error (1)
 */
int a404PrintIsrInfo( u_int8 vec )
{
	printf(" vec=%u, installed=%u, lev=%u, count=%lu\n",
		G_irqv[vec].vec, G_irqv[vec].installed,
		G_irqv[vec].lev, G_irqv[vec].count);  	
  
  	return 0;
}

/***************************************************************************/
/**   Get and clear ISR count for vector 
 *
 *  \param vec		\IN  IRQ vector 0..255
 *
 *  \return		ISR count
 */
int a404GetClrIsrCount( u_int8 vec )
{
	int count;
	
	count = G_irqv[vec].count;
	G_irqv[vec].count = 0;
	
	return count;
}

/***************************************************************************/
/** IRQ test 
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *
 *  \return		success (0) or error (1)
 */
int32 a404IrqTest( u_int32 brdBase, u_int32 em )
{
#define T_NBR		4		// number of combined TX1->RX2/TX2->RX1 tests
#define TRX_NBR		2*T_NBR	// number of TX->RX test parameters
	
	int i, n, ret, t, x1, x2;
	
	enum tblParam {
		testNbr,
		txCh,
		rxCh,
		lev,
		vec,
		delay,
		fire
	};
	 
	u_int16 tbl[][TRX_NBR] =
	 { {  1,   1,     2,   2,     3,   3,     4,   4}, 	// testNbr
	   {  1,   2,     2,   1,     1,   2,     2,   1}, 	// txCh
       {  2,   1,     1,   2,     2,   1,     1,   2}, 	// rxCh
       {  1,   2,     3,   4,     5,   6,     7,   1}, 	// lev
       {201, 101,   102, 202,   203, 103,   104, 204},  // vec
       { 10,  10,     8,   6,     4,   2,     1,   0},  // delay
       {  5,   5,    10,   10,   15,  15,    20,  20} };// fire

	for( t=1; t<=T_NBR; t++ ){

		x1 = (t-1)*2;
		x2 = x1+1;	
		printf("IRQ test #%d : TX%d->RX%d (lev=%d, vec=%d), TX%d->RX%d (lev=%d, vec=%d)\n",																																																												
			t,
			tbl[txCh][x1], tbl[rxCh][x1], tbl[lev][x1], tbl[vec][x1],																																																											
			tbl[txCh][x2], tbl[rxCh][x2], tbl[lev][x2], tbl[vec][x2]);
		 
		// configure																																																												
		printf("  - set vector, install isr, enable level\n");																																																												
		for( i=0; i<TRX_NBR; i++ ){																																																												
			if( t != tbl[testNbr][i] )
				continue;
																																																														
			ret = a404RxSetIrqVector( brdBase, em, tbl[rxCh][i], tbl[vec][i] );																																																												
			if( ret )																																																												
				return ret;																																																												
																																																																
			ret = a404InstallIsr( tbl[vec][i], tbl[lev][i] );																																																												
			if( ret )																																																												
				return ret;																																																												
																																																																
			ret = a404RxIrqEnable( brdBase, em, tbl[rxCh][i], tbl[lev][i], 1 );																																																												
			if( ret )																																																												
				return ret;																																																												
		}																																																													
																																																													
		// fire																																																												
		printf("  - fire IRQs on ch: ");																																																												
		for( n=0; n<tbl[fire][x1]; n++ ){																																																												
			for( i=0; i<TRX_NBR; i++ ){																																																												
				if( t != tbl[testNbr][i] )
					continue;
					
				taskDelay( tbl[delay][i] );
																																																																
				printf("%d", tbl[txCh][i]);																																																												
				ret = a404TxIrqSend( brdBase, em, tbl[txCh][i], tbl[lev][i] );																																																												
				if( ret )																																																												
					return ret;																																																												
			}																																																												
		}																																																												
		printf("\n");																																																												
																																																													
		// cleanup																																																												
		printf("  - uninstall isr, disable level:\n");																																																												
		for( i=0; i<TRX_NBR; i++ ){																																																												
			if( t != tbl[testNbr][i] )
				continue;
																																																																	
			ret = a404UnInstallIsr( tbl[vec][i] );																																																												
			if( ret )																																																												
				return ret;																																																												
																																																																
			ret = a404RxIrqEnable( brdBase, em, tbl[rxCh][i], tbl[lev][i], 0 );																																																												
			if( ret )																																																												
				return ret;																																																												
		}	
	}																																																													

	// verify
	printf("Verify IRQ tests 1..%d\n", T_NBR);
	for( i=0; i<TRX_NBR; i++ ){
		ret = a404GetClrIsrCount( tbl[vec][i] );
		printf("  RX%d (irq vec=%d) : %d IRQs received\n",
			tbl[rxCh][i], tbl[vec][i], ret);
		if( ret != tbl[fire][i] )
			return -1;
	}        	
	
	return 0;
}