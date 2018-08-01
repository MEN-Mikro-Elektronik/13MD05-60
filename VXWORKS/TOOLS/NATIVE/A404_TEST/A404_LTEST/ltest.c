/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *         \file ltest.c
 *       \author dieter.pfeuffer@men.de
 *        $Date: 2007/10/26 14:24:21 $
 *    $Revision: 1.6 $
 *
 *        \brief A404 ltest
 *
 *     Required: modules: testutil.o, mdis_PPC_VME_6U.o, mtest.o, a404_ltest.o
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: ltest.c,v $
 * Revision 1.6  2007/10/26 14:24:21  DPfeuffer
 * RXSTAT/TXSTAT state not printed for option -p (performance test)
 *
 * Revision 1.5  2007/01/30 07:54:21  DPfeuffer
 * - option -e added
 * - clearing G_xxxErrCount fixed
 *
 * Revision 1.4  2006/03/16 15:46:42  DPfeuffer
 * - reads now TXSTAT/RXSTAT if mtest fails
 *
 * Revision 1.3  2006/02/17 15:53:20  DPfeuffer
 * - enable A32 space fixed
 * - option -e removed
 *
 * Revision 1.2  2006/01/02 09:40:08  dpfeuffer
 * added: clear RX1/RX2 space for mtest
 *
 * Revision 1.1  2005/12/07 10:58:11  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/maccess.h>
#include  <MEN/a404_reg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define SWAP16(word)	(((word)>>8) | ((word)<<8))

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int32 G_rxCrcErrCount[2];
static u_int32 G_rxDataLostErrCount[2];
static u_int32 G_txCrcErrCount[2];
static u_int32 G_txDataLostErrCount[2];
static u_int32 G_trxStatErr;

/***************************************************************************/
/**  Print program usage
 */
static void usage(void)
{
	printf(
	"Usage: a404_ltest <A24-brd-addr> [<opts>]\n"
	"Function: Program for A404 loopback-test\n"
	"Options:\n"
	"  -a=<A32-VME-addr>,<A32-brd-offset>\n"
	"               use A32 space (requires A404 in enhanced mode)\n"
	"               A32-brd-addr = <A32-VME-addr> + <A32-brd-offset>\n"
	"  -n=<num>     number of bytes to transfer (max size if not specified)\n"
	"  -l           endless loop (press any key to abort)\n"	
	"  -p           output according to a404_ptest (performance test)\n"	
	"  -e           enable LVDS-protocol and error mechanisms\n"	
    "  === memory test (mtest) ===\n"    
    "  Write pattern to TX12 space, then read from RX1/RX2 space and verify.\n"  
    "  -t=<lst>     list of r/w tests to execute (e.g. -t=wl):\n"    
    "                 b: Byte access, random pattern\n"
    "                 w: Word access, random pattern\n"
    "                 l: Long access, random pattern\n"
    "  === transmit test ===\n"    
    "  Write <val> to TX12 space.\n"
	"  -v=<val>     hex-value to transmit:\n"
	"                 val=      0x00 ..      0xff : byte access\n"
	"                 val=    0x0000 ..    0xffff : word access\n"
	"                 val=0x00000000 ..0xffffffff : long access\n"	      
	"\n"
	"(c) 2005-2007 by MEN mikro elektronik GmbH\n\n");
}
 
/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main( int argc, char *argv[])
{
	char		*errstr, *optp, buf[200];
	char		*testlist=NULL, *testP, test;
	u_int32 	brdBase, a32VmeAddr=0, a32brdOff, size;
	char		mtest_t[20], mtest_arg[100];
	u_int32 	em, loop, perf, protEnbl, byteNbr, txVal, txByteAcc=0;
	MACCESS		ma;
	u_int8 		ch, regVal;
	int			ret=0;
	int32 		start, n;
	u_int8		abort=0;
	
	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if( argc <= 2) {
		usage();
		return(1);
	}    
    
	if( (errstr = UTL_ILLIOPT("a=et=n=lpv=?", buf)) ){	/* check args */
		printf("*** %s\n", errstr);
		return(1);
	}

	if( UTL_TSTOPT("?") ){						/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	sscanf(argv[1], "%lx", &brdBase);
	ma = (MACCESS)brdBase;

	loop    = (UTL_TSTOPT("l") ? 1 : 0);
	perf    = (UTL_TSTOPT("p") ? 1 : 0);
	protEnbl= (UTL_TSTOPT("e") ? 1 : 0);

	/* tx */
	if( (optp = UTL_TSTOPT("v=")) ){
		txByteAcc = (strlen(optp) - 2) / 2;
		sscanf(optp, "%lx", &txVal);
	}
	
	/* mtest */
	testlist = UTL_TSTOPT("t=");

	byteNbr = ((optp = UTL_TSTOPT("n=")) ? atoi(optp) : 0);
    
    /* use A32 space? */
	if( (optp = UTL_TSTOPT("a=")) ){
		char *p, a32VmeAddrStr[20];
	
		p = strchr( optp, ',');
		if( !p ){
			usage();
			return(1);
		}
		strncpy(a32VmeAddrStr, optp, p-optp);
		sscanf(a32VmeAddrStr, "%lx", &a32VmeAddr);
		sscanf(p+1, "%lx", &a32brdOff);
	}

	/*--------------------+
    |  get mode           |
    +--------------------*/
	/* assume compatibility mode */
	regVal = MREAD_D8( ma, A404_VME_CONF(0) );

	if( (regVal & A404_VME_CONF_IDMASK) != A404_VME_CONF_ID ){
		/* assume enhanced mode */
		regVal = MREAD_D8( ma, A404_VME_CONF(1) );
	}

	if( (regVal & A404_VME_CONF_IDMASK) != A404_VME_CONF_ID ){
		printf("*** no A404 detected\n");
		return -1;
	}		
		
	/* A404 in enhanced mode? */
	if( regVal & A404_VME_CONF_ENH ){
		printf("detected: A404 in enhanced mode (A24)\n");
		em = 1;
	}
	/* A404 in compatibility mode */
	else{
		printf("detected: A404 in compatibility mode\n");
		em = 0;
	}

	/*--------------------+
    |  init               |
    +--------------------*/
	/* config TX */
	MWRITE_D8( ma, A404_TXCTRL(em,1), protEnbl ? A404_TXCTRL_PROTENBL : 0x00 );
	MWRITE_D8( ma, A404_TXCTRL(em,2), protEnbl ? A404_TXCTRL_PROTENBL : 0x00 );    
    
	/* config RX */
	MWRITE_D8( ma, A404_RXCTRL(em,1), A404_RXCTRL_FFRST_NO);
	MWRITE_D8( ma, A404_RXCTRL(em,2), A404_RXCTRL_FFRST_NO);

	taskDelay(1); /* required for A404 boards to clear the RXSTAT */

	/* clear RX status */
	MWRITE_D8( ma, A404_RXSTAT(em,1), 0x00 );
	MWRITE_D8( ma, A404_RXSTAT(em,2), 0x00 );

	/* clear TX status */
	regVal = A404_TXSTAT_DATALOST | A404_TXSTAT_CRCERR;
	MWRITE_D8( ma, A404_TXSTAT(em,1), regVal );
	MWRITE_D8( ma, A404_TXSTAT(em,2), regVal );	

	/* clear TEST0 status */
	regVal = A404_TEST0_DL_FSM | A404_TEST0_DL_INP | 	
			 A404_TEST0_DL_FULL | A404_TEST0_DL_RETRY |
			 A404_TEST0_CLR_ALL; 
	MWRITE_D8( ma, A404_TEST0(em,1), regVal );
	MWRITE_D8( ma, A404_TEST0(em,2), regVal );  

   	for( ch=1; ch<=2; ch++ ){
		G_rxCrcErrCount[ch-1] = 0;
		G_rxDataLostErrCount[ch-1] = 0;
		G_txCrcErrCount[ch-1] = 0;
		G_txDataLostErrCount[ch-1] = 0;
	}
	G_trxStatErr = 0;

	if( a32VmeAddr ){
		/* set and enable A32 space */
		MWRITE_D16( ma, A404_VME_A32BASE_EM,
			SWAP16(A404_VME_A32BASE_ADDR(a32brdOff) | A404_VME_A32BASE_ENBL ));
			
		ma = (MACCESS)(a32VmeAddr + a32brdOff);
		printf("using A32 space, brd-addr=0x%lx\n", (u_int32)ma);
	}
	else{
		printf("using A24 space, brd-addr=0x%lx\n", (u_int32)ma);
	}
	
	size = byteNbr ? byteNbr : A404_TRX_SIZE(em);

	if( loop ){
		if( perf ){
			printf("endless-loop-for-a404_ptest\n");
		}
		else{		
			printf(">>> ------------------------------------ <<<\n");		
			printf(">>> endless loop, press any key to abort <<<\n");
			printf(">>> ------------------------------------ <<<\n");		
		}
	}
	
	do {
		/*--------------------+
	    |  mtest               |
	    +--------------------*/	
		if( testlist ){									
			testP = testlist;
			for( test=*testP; test; testP++, test=*testP ){
														
				/* clear RX1/RX2 space */    
		       	for( ch=1; ch<=2; ch++ ){ 
	    			start = ((u_int32)ma) + A404_RX(em,ch);
	      			memset( (void*)start, 0x00, size ); 
	    		}
								
				sprintf( mtest_t," -t=%c", test );
							
			 	/* write to TX12 space */
				printf("=== %c === - write TX12 space\n", test);
			 	start = ((u_int32)ma) + A404_TX12(em);
				sprintf( mtest_arg, "0x%lx 0x%lx%s -w -o=1 -q=10",
					start, start + size, mtest_t );
						
			 	if( (ret = mtest(mtest_arg)) ){
					printf("*** TX12: write failed\n"); 		
			 		abort = 1;
			 	}
			 		
			 	/* check/clear TXSTAT1/TXSTAT2 */
				printf("=== %c === - check TXSTAT1/TXSTAT2\n", test);
			    for( ch=1; ch<=2; ch++ ){ 
					regVal = MREAD_D8( ma, A404_TXSTAT(em,ch) );
					if( regVal & A404_TXSTAT_CRCERR ){
						printf("*** TXSTAT%d: TX_CRC_ERR\n", ch);
						G_txCrcErrCount[ch-1]++;
						G_trxStatErr=1;
					}
					if( regVal & A404_TXSTAT_DATALOST ){
						printf("*** TXSTAT%d: TX_DATA_LOST\n", ch);
						G_txDataLostErrCount[ch-1]++;
						G_trxStatErr=1;
					}
					/* clear TX status */
					MWRITE_D8( ma, A404_TXSTAT(em,ch), regVal );					
				}
				if( abort )
			 		goto CLEANUP;	 		
				
			 	/* read/verify from RX1/RX2 space */
				printf("=== %c === - read/verify RX1/RX2 space\n", test);
			    for( ch=1; ch<=2; ch++ ){ 
			 		start = ((u_int32)ma) + A404_RX(em,ch);
					sprintf( mtest_arg, "0x%lx 0x%lx%s -v -o=1 -q=10",
						start, start + size, mtest_t );
						
				 	if( (ret = mtest(mtest_arg)) ){
						printf("*** RX%d: verify failed\n", ch);
				 		abort = 1;
				 	}
				}
				
			 	/* check/clear RXSTAT1/RXSTAT2 */
				printf("=== %c === - check RXSTAT1/RXSTAT2\n", test);
			    for( ch=1; ch<=2; ch++ ){ 
					regVal = MREAD_D8( ma, A404_RXSTAT(em,ch) );
					if( regVal & A404_RXSTAT_CRCERR ){
						printf("*** RXSTAT%d: RCV_CRC_ERR\n", ch);
						G_rxCrcErrCount[ch-1]++;
						G_trxStatErr=1;
					}
					if( regVal & A404_RXSTAT_DATALOST ){
						printf("*** RXSTAT%d: RX_DATA_LOST\n", ch);
						G_rxDataLostErrCount[ch-1]++;
						G_trxStatErr=1;
					}
					/* clear RX status */
					MWRITE_D8( ma, A404_RXSTAT(em,ch), 0x00 );
					
					regVal = MREAD_D8( ma, A404_TEST0(em,ch) );
					if( regVal ){
						printf("*** LVDSTEST%d0=0x%02x\n", (int)ch, regVal);
						regVal & A404_TEST0_DL_FSM 	 ? printf(" - DATA_LOST_FSM\n") : 0;
						regVal & A404_TEST0_DL_INP 	 ? printf(" - DATA_LOST_INP\n") : 0;
						regVal & A404_TEST0_DL_FULL  ? printf(" - DATA_LOST_FULL\n") : 0;
						regVal & A404_TEST0_DL_RETRY ? printf(" - DATA_LOST_RETRY\n") : 0;
						regVal & A404_TEST0_CLR_ALL  ? printf(" - CLR_ALL_SENT\n") : 0;
						
						/* clear */
						MWRITE_D8( ma, A404_TEST0(em,ch), regVal );
					}					
				}
				if( abort )
			 		goto CLEANUP;	 		
				
			}
		} /* testlist */
				
		/*--------------------+
	    |  tx                 |
	    +--------------------*/	
		if( txByteAcc ){
		
			int 	dataNbr = size / txByteAcc;
			char 	*accStr;
			start = A404_TX12(em);

			switch( txByteAcc ){
			case 1: accStr = "byte"; break;
			case 2: accStr = "word"; break;
			case 4: accStr = "long"; break;
			default:
				usage();
				return(1);
			}
			
			if( !perf )
				printf("write 0x%lx to 0x%lx..0x%lx (%s access)\n",
					txVal, ((u_int32)ma) + start,
					((u_int32)ma) + start + size - 1, accStr);
			
			for( n=0; n<dataNbr; n++ ){
				switch( txByteAcc ){
				case 1: MWRITE_D8 ( ma, start, txVal ); break;
				case 2: MWRITE_D16( ma, start, txVal ); break;
				case 4: MWRITE_D32( ma, start, txVal ); break;
				}
				start += txByteAcc;
			}				
		}
					
	} while(loop && UOS_KeyPressed() == -1);

	if( loop ){
		printf("- endless loop finished\n");
	}

CLEANUP:
	if( !perf ){
	   	for( ch=1; ch<=2; ch++ ){
			printf("RXSTAT%d: RCV_CRC_ERR=%lu times, RX_DATA_LOST=%lu times\n",
				ch, G_rxCrcErrCount[ch-1], G_rxDataLostErrCount[ch-1]);
			printf("TXSTAT%d: TX_CRC_ERR=%lu times, TX_DATA_LOST=%lu times\n",
				ch, G_txCrcErrCount[ch-1], G_txDataLostErrCount[ch-1]);
		}
	}
	
	if( G_trxStatErr )
		ret = -1;
  	
	return ret;
}




