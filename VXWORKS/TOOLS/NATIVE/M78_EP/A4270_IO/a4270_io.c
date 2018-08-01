/****************************************************************************/
/*!
 *         \file a4270_io.c
 *       \author ts
 *        $Date: 2007/09/26 17:01:01 $
 *    $Revision: 1.1 $
 *
 *       \brief  Test program for M78 binary input/outputs
 *
 *     Required: libraries: mdis_api
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: a4270_io.c,v $
 * Revision 1.1  2007/09/26 17:01:01  ts
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


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CHK(expression) \
 if((expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     goto ABORT;\
 }


#define M78_SAMPLES   10000 	 /* # of samples to get   */
#define MAXBUF M78_SAMPLES*4	 /* we sample 4 channels at a time 		 */
#define M78_AMPLITUDE_MAX 	1.000	/* max. Amplitude allowed to set */
#define	KNICK_IBADDR  		0x08
#define HPIB_CTRL_NAME		"/m8/1"
#define BUFFERSIZE 			64

 
#define HITKEY(x)  \
	printf("%s\n",x);\
	UOS_KeyWait();

#define VERBPR(x...)  {if (verbose) printf(x);}

#define MAXBUF          100     /* size of result buffer */
#define TIMEOUT         5000    /* timeout in msec (-1=wait forever) */
#define LOW				0
#define HIGH			1

/* 
 * Test adapter Signals to be set through M28 
 */


#define A4270_SOURCE	0x0001
#define A4270_SIG_1		0x0002
#define A4270_SIG_2		0x0004
#define A4270_SIG_3		0x0008
#define A4270_SIG_4		0x0010
#define A4270_GND_1		0x0020
#define A4270_GND_2		0x0040
#define A4270_GND_3		0x0080
#define A4270_GND_4		0x0100

/* set all 4 channel Relais */
#define A4270_SIG_ALL	0x001e
#define A4270_GND_ALL	0x01e0


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

/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

/* static void SigHandler( u_int32 sigCode ); */




static void usage( void )
{

    printf("a4270_io   GPI[0..7] and binary out EP Test for M078\n");
	printf("usage: a4270_io <options>\n\n");
    printf(" Option     Meaning                            Examples\n");
    printf("-------------------------------------------------------------\n");
	printf(" -t=<num>   Test type: 0=BOUT 1=GPI             -t=0 -t=1\n");
	printf(" -d         verbose Dumps (no argument)         -d \n\n");
	printf(" if runlight test (-t=0) specified no other argument is used.\n");

}



/********************************  a4270_setGalOut ***************************/
/** switch Signal input to corresponding M78 LEMO connector or ground it
 *
 *  \param hdl        \IN handle returned from M_open("/m28/1")
 *  \param value      \IN Selector Value to put low on GAL
 *
 *  \return	           0 on success or negative error number
 *
 *  \brief            The GAL output is selected based on the scheme below.
 *                    It works as a 1-out-of-8 selector, the selected Line
 *                    is driven low.
 *
 *   15  14  13  12  11   10  9   8   7   6   5   4   3   2   1   0
 *  | ...GAL ......|GAL |..NC  | ....GND.......|.In Signal.....|Input  |   
 *  |              |EN  |      |ch3 ch2 ch1 ch0|ch3 ch2 ch1 ch0|0 HM   |  
 *  |              |    |      |  0..ANA       |    0..50Ohm   |1 Knick|
 *  |              |    |      |  1..ANA+DIG   |    1..Sig     |       |
 *  +--------------+----+------+---------------+---------------+-------+
 *
 */
static int32 a4270_setGalOut( int32 hdl, u_int8 value )
{
	
	/* enable GAL */
	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 11 ) < 0 );
	CHK( M_write( hdl, 1 ) < 0);

	/* write value for 1-of-8 selector in GAL */

	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 12 ) < 0 );
	CHK( M_write( hdl, value & 0x1 ) < 0);

	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 13 ) < 0 );
	CHK( M_write( hdl, (value >> 1) & 0x1 ) < 0);

	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 14 ) < 0 );
	CHK( M_write( hdl, (value >> 2) & 0x1 ) < 0);

	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 15 ) < 0 );
	CHK( M_write( hdl, (value >> 3) & 0x1 ) < 0);

	return 0;

ABORT:
	return -1;

}



static int32 m78_init( int32 path )
{

    /* general */
    CHK( M_setstat(path, M78_DATA_SOURCE, 		M78_DS_NORMAL) < 0 );
    CHK( M_setstat(path, M78_CLOCK_SOURCE, 		M78_CS_INTERNAL) < 0 );

    /* 500 kHz */
    CHK( M_setstat(path, M78_ADCLK_DIV,    		48) < 0 );
    CHK( M_setstat(path, M78_NUM_CHANNELS, 		4) < 0 );

	/* current channel: 0 */
    CHK( M_setstat(path, M_MK_CH_CURRENT,  		0) < 0 );

    /* channel 102 -> read all 4 physical channels at once */
    CHK( M_setstat( path, M_MK_CH_CURRENT, 102 ) < 0 );

    /* trigger settings */
    CHK( M_setstat( path, M78_TRIG_EDGE,    	1   ) < 0 );
    CHK( M_setstat( path, M78_TRIG_LEVEL,   	500 ) < 0 );
    CHK( M_setstat( path, M78_TRIG_DELAY,   	0   ) < 0 );

    /* kick Triggering in Software */
    CHK( M_setstat( path, M78_TRIG_SOURCE,      M78_TS_SW ) < 0 );
    CHK( M_setstat( path, M78_TRIG_HYSTERESIS,  0 ) < 0 );

	/* we want to collect <M78_SAMPLES> samples */
    CHK( M_setstat( path, M78_POST_TRIG_SAMPLES, M78_SAMPLES ) < 0 );

    /* set timeout for M_getblock() */
    CHK( M_setstat( path, M78_GET_BLOCK_TIMEOUT, 0 ) < 0 );
	
    /* enable interrupts */
    CHK( M_setstat( path, M_MK_IRQ_ENABLE, TRUE ) < 0 );

	return 0;
ABORT:
	return -1;

}







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
	/* helpers */
	char  *errstr, *str, b[128];
	int32	i			= 0;
	int32 error 		= 0;
	u_int32 nSampled	= 0;
	int32 hdlM28 		= 0;	
	int32 hdlM78 		= 0;	/* DUT */
	u_int8  gpi[8]		= {0, 0, 0, 0, 0, 0, 0, 0};
	u_int16   sampBuf[MAXBUF];

	/* for reading data from M78 */
    M78_RESULT_PARMS 	result;
    M_SETGETSTAT_BLOCK 	sg;
	M78_SEEK_PARMS 		skp;
	M_SETGETSTAT_BLOCK 	seekg;

	/* variables for passed option values */
	u_int32 test		= 0;
	u_int32 verbose		= 0;

	/* introduce ourselves */
	printf("Test a4270_io built %s %s\n", __DATE__, __TIME__ );


	/*--------------------+
	|  init Vars          |
    +--------------------*/

	memset(b, 0, sizeof(b));
	seekg.size = sizeof(skp);
	seekg.data = (void *)&skp;
	skp.origin = M78_SP_START;

    sg.data = (void *)&result;
    sg.size = sizeof(result);


	/*--------------------+
    |  check arguments    |
    +--------------------*/
    if( argc < 2 )  {
    	usage();
    	return (1);
    }
    
	if ((errstr = UTL_ILLIOPT("b:d:t=?", b))) {
		printf("*** %s\n", errstr);
		HITKEY("press any key to continue\n");
		return(1);
	}
	if (UTL_TSTOPT("?")) {	/* help requested ? */
		usage();
		HITKEY("press any key to continue\n");
		return(1);
	}

	test	= ((str = UTL_TSTOPT("t=")) ? atoi(str) : 0);
	
	verbose 	= (UTL_TSTOPT("d") ? 1 : 0);

	/*-----------------------------+
    |  Open & Init Test equipment  |
    +-----------------------------*/

	/* M28 for A_4240 */
	CHK( (hdlM28 = M_open("/m28/1")) < 0 );

	/* the DUT */
	CHK( (hdlM78 = M_open("/m78/1")) < 0 );

	/* Select source and channels */
/* 	CHK( a4270_selectSource( hdlM28, 0 ) < 0 ); */
/* 	CHK( a4270_selectChannel( hdlM28, channel ) < 0 ); */
	

	/*-----------------------------+
    |  Start Test procedure        |
    +-----------------------------*/

	switch(test) {
	case 0:

		printf("BOUT test selected. Watch runlight effect on all 4 LEDs.\n");
		for( i = 0; i < 4; i++) {
			CHK( M_setstat( hdlM78, M78_BOUT, 1 << i ) < 0 );
			UOS_Delay(200);
		} 
		for( i = 2; i > 0; i-- ) {
			CHK( M_setstat( hdlM78, M78_BOUT, 1 << i) < 0 );
			UOS_Delay(200);
		} 
		for( i = 0; i < 4; i++ ) {
			CHK( M_setstat( hdlM78, M78_BOUT, 1 << i ) < 0 );
			UOS_Delay(200);
		} 
		for( i = 2; i > 0; i-- ) {
			CHK( M_setstat( hdlM78, M78_BOUT, 1 << i) < 0 );
			UOS_Delay(200);
		} 
		CHK( M_setstat( hdlM78, M78_BOUT, 0xf ) < 0 );
		UOS_Delay(200);
		CHK( M_setstat( hdlM78, M78_BOUT, 0x0 ) < 0 );
		UOS_Delay(200);
		
		break;

	case 1:
		/* GPI[0..7] test */
		for( i = 1; i < 9; i++ ) 
		{
			CHK( (m78_init(hdlM78) < 0 ));
			/* the GAL works as 1 from 8 selector */
			a4270_setGalOut( hdlM28, i );
			UOS_Delay(10);

			/* kick a short read cycle, always use 4 channels internal */
			CHK( M_setstat( hdlM78, M78_SAMPLE_STATE, M78_ST_RUN) < 0 );
			UOS_Delay(50);
			CHK( M_setstat( hdlM78, M78_SAMPLE_STATE, M78_ST_HALT) < 0 );
			M_getstat( hdlM78, M78_BLK_RESULT, (int32 *)&sg );
			nSampled = result.totSamples;
			VERBPR("\nsampled %d Samples\n", nSampled);

			/* set MEM_POINTER to read out, we use always SDRAM start */
			skp.offset = 0;
			CHK( M_setstat( hdlM78, M78_BLK_SEEK,(int32)&seekg ) < 0 );
			/* read out the amount of M78_SAMPLES data */
			CHK( M_getblock( hdlM78,(u_int8*)sampBuf, sizeof(sampBuf)) < 0 );
			/* the 8 GPI bits are part of Sample buffers bits 12 and 13 */
			gpi[6] = (sampBuf[0] & 0x1000) >> 12;
			gpi[7] = (sampBuf[0] & 0x2000) >> 13;
			gpi[4] = (sampBuf[3] & 0x1000) >> 12;
			gpi[5] = (sampBuf[3] & 0x2000) >> 13;
			gpi[2] = (sampBuf[2] & 0x1000) >> 12;
			gpi[3] = (sampBuf[2] & 0x2000) >> 13;
			gpi[0] = (sampBuf[1] & 0x1000) >> 12;
			gpi[1] = (sampBuf[1] & 0x2000) >> 13;
			
			VERBPR("GALout %d: GPI[7:0] = %d %d %d %d %d %d %d %d\n", i,
				   gpi[7],gpi[6],gpi[5],gpi[4],gpi[3],gpi[2],gpi[1],gpi[0] );
			UOS_Delay(300);
			/* Test is positive, if GPI[i] is 0 when GAL output set to i */
			printf("== Test of GPI%d: %s\n", i,	gpi[i-1] ? "FAILED" : "OK" );
		} 
		break;
	}
	
	
	/* Stop Acquisition */
	CHK( M_setstat( hdlM78, M78_SAMPLE_STATE, M78_ST_HALT) < 0 );

	/* close devices */	
	M_close(hdlM78);
	M_close(hdlM28);

ABORT:
	return error;

}


#if 0
/***************************************************************************/
/** Signal handler routine
*
*  \param sigCode       \IN  received signal
*/
static void SigHandler( u_int32 sigCode )
{
    /* received our signal ? */
    if( sigCode == UOS_SIG_USR1 )  {
        ++G_trigCnt;
    }
    else  {
    	++G_errSig;
    }
}
#endif
