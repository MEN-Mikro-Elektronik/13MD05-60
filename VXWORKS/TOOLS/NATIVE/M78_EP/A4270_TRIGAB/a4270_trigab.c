/****************************************************************************/
/*!
 *         \file a4270_trigab.c
 *       \author ts
 *        $Date: 2007/09/26 17:01:07 $
 *    $Revision: 1.1 $
 *
 *       \brief  Test program for M78 rear Triggerline Variants.
 *
 * 		Test if DSUB Pin OUT4 (pin30) is routed to MM IF line TRIG_A  (C9)
 *
 *      Attention! The test requires OUT3 (Pin44) to be connected to
 *                 AIN2+(Pin32) on the DSUB front connector.
 *
 *                 Take care to have a Block Mode enabled FPGA Revision on M78
 *                 under test.
 *
 *     Required: libraries: mdis_api
 *     \switches (none)
 */
/*-------------------------------[ History ]--------------------------------
 *
 * $Log: a4270_trigab.c,v $
 * Revision 1.1  2007/09/26 17:01:07  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sioLibCommon.h>
#include <ioLib.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m78_drv.h>
#include <MEN/m72_drv.h>
#include <MEN/m78ep_defs.h>


#include <MEN/hm8131.h>
#include <MEN/IB/ib_drv.h>
#include <MEN/IB/ib_api.h>


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CHK(expression) \
 if( (expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     goto ABORT;\
 }

#define	CHKARG( expr, errstr) \
  if (expr) {  \
	  printf("*** error:" errstr); \
		error = -1; \
		goto ABORT; \
 } 


/* To use test equipment without M78 (to check if trigger signals are routed)*/
/* #undef WITH_M78 */
#define WITH_M78

/* HM8131 debugs (sending etc)*/
#if 0
#define DBGHM(x...) printf(x)
#else
#define DBGHM(x...) 	
#endif

/* Program debugs */
#if 0
#define DBGPR(x...) printf(x)
#else
#define DBGPR(x...) 
#endif

 
 /* m28 bits 
  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1   0
 | ...GAL ......|GAL |..NC  | ....GND.......|.In Signal.....|Input  |   
 |              |EN  |      |ch3 ch2 ch1 ch0|ch3 ch2 ch1 ch0|0 HM   |  
 |              |    |      |  0..ANA       |    0..50Ohm   |1 Knick|
 |              |    |      |  1..ANA+DIG   |    1..Sig     |       |
 +--------------+----+------+---------------+---------------+-------+ */

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */


/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

static u_int32 G_gotIRQ = 0;
/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/


/***************************************************************************/
/** The usage display function
*
*/
static void usage( void )
{

	printf("a4270_trigab   ADC Operation and accuracy EP Test for M078\n");
	printf("usage: a4270_trigab <options>\n\n");
	printf(" Option       Meaning                              Examples\n");
	printf("-------------------------------------------------------------\n");
	printf(" -t=<test>  0: toggle OUT3/4 manual for test       -t=0 -t=1 \n");
	printf("            1: TRIG_A test\n");
	printf(" -d         verbose Dumps (no argument)     -d \n");

}


#if 0 /* samples are not looked at here */
/***************************** m78_getSamples ********************************/
/** Collect a shot of Samples
 *
 *  \param hdl        \IN  MDIS handle returned from M_open("/m78/1")
 *  \param dat        \IN  pointer to store sampled data
 *  \param nomVal     \IN  Voltage (16bit signed) in Knick for DC accuracy test
 *  \param nrSamples  \IN  Number of Samples to read with M_getblock
 *
 *  \return	          0 on success or MDIS error code
 */
static int32 m78_getSamples(int32 hdl, u_int16 *dat, u_int32 nrSamples)
{
	u_int32 nSampled	= 0;
	/* for reading data from M78 */
    M78_RESULT_PARMS 	result;
    M_SETGETSTAT_BLOCK 	sg;
	M78_SEEK_PARMS 		skp;
	M_SETGETSTAT_BLOCK 	seekg;

	seekg.size = sizeof(skp);
	seekg.data = (void *)&skp;
	skp.origin = M78_SP_START;

    sg.data = (void *)&result;
    sg.size = sizeof(result);

	/* kick a short read cycle, always use 4 channels internal */
	CHK( M_setstat( hdl, M78_SAMPLE_STATE, M78_ST_RUN) < 0 );
	UOS_Delay(2000);
	CHK( M_setstat( hdl, M78_SAMPLE_STATE, M78_ST_HALT) < 0 );
	M_getstat( hdl, M78_BLK_RESULT, (int32 *)&sg );
 
	nSampled = result.totSamples;
	printf("\nsampled %d Samples\n", nSampled);

	/* set MEM_POINTER to read out, we use always SDRAM start */
	skp.offset = 0;
	CHK( M_setstat( hdl, M78_BLK_SEEK,(int32)&seekg ) < 0 );

	/* read out the sample data */
	CHK( M_getblock( hdl,(u_int8*)dat, nrSamples)< 0 );

			
	return 0;
ABORT:
	return -1;

}
#endif


/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main(int argc, char *argv[])
{

	/* helpers  */
	char  	*errstr, *str, b[128];
	int32 	error 			= 0;
	u_int32 bM78open 	= 0; 
	u_int32	i 			= 0;
	u_int16 *sampBuf 	= NULL;
	int32 	hdlM72 		= 0;
	int32 	hdlM78 		= 0;	/* DUT */

	u_int32 nSamp[4];

	/* for reading data from M78 */
    M78_RESULT_PARMS 	result;
    M_SETGETSTAT_BLOCK 	sg;
	M78_SEEK_PARMS 		skp;
	M_SETGETSTAT_BLOCK 	seekg;

	/* variables for passed option values */
	u_int32 test		= 0;
	u_int32 graphs		= 0;	/* Waveform 	*/
	u_int32 channel		= 0;
	u_int32 verbose		= 0;

	/* introduce ourselves */
	printf("Test a4270_trigab built %s %s\n", __DATE__, __TIME__ );
	printf("==============================\n");

	/*--------------------+
	|  init Vars          |
    +--------------------*/

	G_gotIRQ 	 = 0;
	memset(b, 	 0,	sizeof(b)		);
	CHK((sampBuf = malloc(M78_SAMPLES)) == NULL);
	seekg.size 	 = sizeof(skp);
	seekg.data 	 = (void *)&skp;
	skp.origin 	 = M78_SP_START;
    sg.data 	 = (void *)&result;
    sg.size 	 = sizeof(result);
	

	/*--------------------+
    |  check arguments    |
    +--------------------*/
    if( argc < 2 )  {
    	usage();
    	return (1);
    }
    
	if ((errstr = UTL_ILLIOPT("d:g=w=s=o=c=a=f=v=t=r=?", b))) {
		printf("*** %s\n", errstr);
		HITKEY("press any key to continue\n");
		return(1);
	}

	channel	= ((str = UTL_TSTOPT("c=")) ? atoi(str) : 15); /* 15 = 0xf = all */
	CHKARG( channel>15, "channel invalid! -c=[0..15] (bit combination)\n");

	graphs = ((str = UTL_TSTOPT("g=")) ? atoi(str) : 0); 
	CHKARG( ((graphs > 999)), "# of samples to dump invalid. -g=[1..999] \n");

	test	= ((str = UTL_TSTOPT("t=")) ? atoi(str) : 0);
	CHKARG( test>3, "test type invalid! -t=[0/1/2]\n");

	verbose = (UTL_TSTOPT("d") ? 1 : 0);	   
	
	/*-----------------------------+
    |  Open & Init Test equipment  |
    +-----------------------------*/

	CHK( (hdlM72 = M_open("/m72/1")) < 0 );

	/* the DUT */
#ifdef WITH_M78
	CHK( (hdlM78 = M_open("/m78/1")) < 0 );
	bM78open = 1;

#endif

	/*-----------------------------+
    |  Start Test procedures       |
    +-----------------------------*/
	switch (test) {

	case 0:
		printf("== TRIG_A/B manual toggle Test ==\n" );
		for (i = 0; i < 16; i++ ) {
			printf("set OUT4 (= TRIG_A)\n" );
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0x8) < 0 );
			UOS_Delay(100);
			printf("clear OUT4 (= TRIG_A)\n" );
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0) < 0 );
			UOS_Delay(100);

		}
		for (i = 0; i < 16; i++ ) {
			printf("set OUT3 (=AIN2+)\n" );
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0x4) < 0 );
			UOS_Delay(100);
			printf("clear OUT3 (=AIN2+)\n" );
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0) < 0 );
			UOS_Delay(100);

		}
		break;

	case 1:
		printf("== TRIG_A/B Test 1 ==\n" );

		CHK( M_setstat(hdlM78, M78_DATA_SOURCE, 	M78_DS_NORMAL) < 0 );
		CHK( M_setstat(hdlM78, M78_CLOCK_SOURCE, 	M78_CS_INTERNAL) < 0 );
		/* 500 kHz */
		CHK( M_setstat(hdlM78, M78_ADCLK_DIV,    	48) < 0 );
		CHK( M_setstat(hdlM78, M78_NUM_CHANNELS, 	4) < 0 );
		CHK( M_setstat(hdlM78, M_MK_CH_CURRENT,  	0) < 0 );
		/* trigger settings */
		CHK( M_setstat( hdlM78, M78_TRIG_EDGE,    	1   ) < 0 );
		CHK( M_setstat( hdlM78, M78_TRIG_LEVEL,   	500 ) < 0 );
		CHK( M_setstat( hdlM78, M78_TRIG_DELAY,   	0   ) < 0 );

		CHK( M_setstat( hdlM78, M78_TRIG_SOURCE,          M78_TS_SW )< 0 );
		CHK( M_setstat( hdlM78, M78_TRIG_HYSTERESIS,      0 ) < 0 );
		CHK( M_setstat( hdlM78, M78_POST_TRIG_SAMPLES,    1000 ) < 0 );
		/* get BLOCKSIZE_SAMPLES samples in Blockmode */
		CHK( M_setstat( hdlM78, M78_BLOCK_SIZE, 10000  ) < 0 );
		/* set timeout for M_getblock() */
		CHK( M_setstat( hdlM78, M78_GET_BLOCK_TIMEOUT, 0 ) < 0 );
		/* channel 102 -> read all 4 physical channels at once */
		CHK( M_setstat( hdlM78, M_MK_CH_CURRENT, 102 ) < 0 );
		/* enable interrupts */
		CHK( M_setstat( hdlM78, M_MK_IRQ_ENABLE, TRUE ) < 0 );
		/* And go*/
		CHK( M_setstat( hdlM78, M78_SAMPLE_STATE, M78_ST_RUN) < 0 );

		/* first part of Test: No TRIG_A */
		printf("== No TRIG_A: SAMPLED_VALUES must remain constant ==\n" );
		M_getstat( hdlM78, M78_BLK_RESULT, (int32 *)&sg );
		nSamp[0] = result.totSamples; /* get initial Value */
		
		for (i = 1; i < NR_M78_TRIGS; i++) {
			M_getstat( hdlM78, M78_BLK_RESULT, (int32 *)&sg );
			nSamp[i] = result.totSamples;
			printf("== sample count %d = %d\n", i, nSamp[i]);
			CHK(nSamp[0] != nSamp[i]); /* check for equality */
			UOS_Delay(500);
		}
		printf("== TRIG_A: Test 1 ok\n" );	

		/* second part of Test: With TRIG_A from M72 */
		printf("== With TRIG_A: SAMPLED_VALUES must increase ==\n" );

		M_getstat( hdlM78, M78_BLK_RESULT, (int32 *)&sg );
		nSamp[0] = result.totSamples; /* get initial Value */
		printf("== sample count 0 = %d\n", nSamp[0]);

		for (i = 1; i < NR_M78_TRIGS; i++) {
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0x8) < 0 );
			UOS_Delay(1);
			CHK( M_setstat(hdlM72, M72_OUT_SET, 0) < 0 );
			UOS_Delay(500);
			M_getstat( hdlM78, M78_BLK_RESULT, (int32 *)&sg );
			nSamp[i] = result.totSamples;
			printf("== sample count %d = %d\n", i, nSamp[i]);
	/* CHK( nSamp[i-1] < nSamp[i]); /\* check for increment in samples *\/ */
		}
		printf("== TRIG_A: Test 2 ok\n" );	
		break;
	case 2:
		printf("== TRIG_A/B Test 2 ==\n" );
		break;

	}


	UOS_Delay(200);
	HITKEY("press any key to continue\n");
	/* close devices */	

#ifdef WITH_M78
	M_close(hdlM78);
#endif
	M_close(hdlM72);

	ib_close();	
	free(sampBuf);

	return error;

 ABORT:
	
	if (sampBuf)
		free(sampBuf);
#ifdef WITH_M78
	if (bM78open)
		M_close(hdlM78);
#endif

	return error;

}


