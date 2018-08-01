/****************************************************************************/
/*!
 *         \file a4270_test1.c
 *       \author ts
 *        $Date: 2007/09/26 17:01:04 $
 *    $Revision: 1.1 $
 *
 *       \brief  Test program M78
 *
 *     Required: libraries: mdis_api
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
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

    printf("a4270_test1   ADC Operation and accuracy EP Test for M078\n");
	printf("usage: a4270_test1 <options>\n\n");
    printf(" Option       Meaning                              Examples\n");
    printf("---------------------------------------------------------------------\n");
	printf(" -t=<test>    0: DC accuracy 1: ADC sampling of waveform\n");
	printf("              2: ANA_IN connect to GND 3:IRQ test  -t=0 -t=1 -t=2\n");
	printf(" -v=<volt>    Knick Voltage(-1..1 V)               -v=0.1234 -v=-0.5\n");
	printf(" -f=<freq>    HM8131 Frequency(1..5000 kHz)        -f=50 -f=2000\n");
	printf(" -a=<ampl>    HM8131 Amplitude(0..1 V)             -a=0.2 -a=0.75 \n");
	printf(" -o=<volt>    HM8131 offset Voltage(-1..1V)        -o=0.2 -o=-0.5\n\n");
	printf(" -g=<samples> dump Graph of current chan.          -g=50 (# of samples to show)\n");
	printf(" -c=<chan>    Channel to select on M78(0..3)       -c=1/2/4/8 and combinations\n");
	printf(" -d           verbose Dumps (no argument)          -d \n");

	printf("    bitwise combinations are possible:\n");
	printf("   -c=1 chan0 -c=3 chan0/1 -c=7 chan0/1/2 -c=15 chan0/1/2/3\n");
}




/***************************************************************************/
/** Signal handler routine
*
*  \param sigCode       \IN  received signal
*/
static void SigHandler( u_int32 sigCode )
{

	printf("\n=== SigHandler: M-Modul IRQ received.\n");
	G_gotIRQ = 1;

}



#if 0
/*********************************** hm8131_read *****************************/
/** basic read function from HM8131
 *
 *  \param fd         \IN serial handle returned from open("/tyCo/1")
 *  \param buf        \IN pointer to copy read data to
 *
 *  \return	           0 
 */
static int32 hm8131_read( int32 fd, char *buf )
{

	u_int32 numRead = 0;	
	char rawbuf[BUFFERSIZE];
	char helpbuf[BUFFERSIZE];
	u_int32 i, j;

	/* clean intermediate buffers */
	memset( helpbuf, 0x0, sizeof(helpbuf));
	memset( rawbuf,  0x0, sizeof(rawbuf));
	numRead = read( fd, rawbuf, BUFFERSIZE);

	/* HM8131 sends some control chars to begin & end, extract Info from */	
	for ( i = 0, j = 0; i < BUFFERSIZE; i++ ) {
		if ( (rawbuf[i] >= 0x20) && (rawbuf[i] < 0x80)) {
			helpbuf[j] = rawbuf[i];
			DBGHM("%c", helpbuf[j] );
			j++;
		}
	}
	DBGHM("\n");

	/* and pass it back */
	strncpy(buf, helpbuf, j);
	return 0;
}
#endif


/*********************************** hm8131_send *****************************/
/** basic command send function to HM8131
 *
 *  \param fd         \IN serial handle returned from open("/tyCo/1")
 *  \param str        \IN pointer to ASCII command to send
 *
 *  \brief  The function sends each character with a delay of 100 ms, there
 *          is no clear Spec about how fast HM8131 receives data! Without
 *          the delay chars get lost.
 *
 *  \return	           0 
 *
 */
static int32 hm8131_send(int32 fd, char *str)
{

	int32 error		= 0;
	u_int32 i		= 0;
	DBGHM("%s:", __FUNCTION__ );
	while(str[i])  {
		DBGHM("%c", str[i] );
		write( fd, &str[i], 1);
		UOS_Delay(100);	
		i++;
	}
	write( fd, "\n", 1);
	DBGHM("\n");
	UOS_Delay(150);

	return error;
}


/******************************* a4270_selectSource **************************/
/** select Signal input to routde to M78 connectors
 *
 *  \param hdl        \IN handle returned from M_open("/m28/1")
 *  \param source     \IN 0: Signal source = HM8131 1: Signal source = Knick 
 *
 *  \return	           0 on success or negative error number
 *
 *  \brief            The signal source is selected based on the scheme below
 * 
 *   15  14  13  12  11   10  9   8   7   6   5   4   3   2   1   0
 *  | ...GAL ......|GAL |..NC  | ....GND.......|.In Signal.....|Input  |   
 *  |              |EN  |      |ch3 ch2 ch1 ch0|ch3 ch2 ch1 ch0|0 HM   |  
 *  |              |    |      |  0..ANA       |    0..50Ohm   |1 Knick|
 *  |              |    |      |  1..ANA+DIG   |    1..Sig     |       |
 *  +--------------+----+------+---------------+---------------+-------+
 *
 */
static int32 a4270_selectSource( int32 hdl, u_int32 source )
{
	int32 error = 0;

	CHK( M_setstat( hdl, M_MK_CH_CURRENT, 0 ) < 0 );
	CHK( M_write( hdl, source ) < 0);

	return 0;
ABORT:
	return error;

}


/******************************** a4270_selectSignal ************************/
/** switch Signal input to corresponding M78 LEMO connector or ground it
 *
 *  \param hdl        \IN handle returned from M_open("/m28/1")
 *  \param chan       \IN channel 0..3 on M78 to feed signal to
 *  \param sig        \IN 0: signal input grounded 1: signal source to input
 *
 *  \return	           0 on success or negative error number
 *
 *  \brief            The signal source is selected based on the scheme below
 * 
 *   15  14  13  12  11   10  9   8   7   6   5   4   3   2   1   0
 *  | ...GAL ......|GAL |..NC  | ....GND.......|.In Signal.....|Input  |   
 *  |              |EN  |      |ch3 ch2 ch1 ch0|ch3 ch2 ch1 ch0|0 HM   |  
 *  |              |    |      |  0..ANA       |    0..50Ohm   |1 Knick|
 *  |              |    |      |  1..ANA+DIG   |    1..Sig     |       |
 *  +--------------+----+------+---------------+---------------+-------+
 *
 */
static int32 a4270_selectSignal( int32 hdl, u_int32 chan, u_int32 sig )
{

	int32 error = 0;
	CHK( M_setstat( hdl, M_MK_CH_CURRENT, chan + 1 ) < 0 );
	CHK( M_write( hdl, sig ) < 0);

	return 0;
ABORT:
	return error;

}



/********************************* hm8131_init *******************************/
/** Initialize HM8131
 *
 *  \param fd         \IN serial handle returned from open("/tyCo/1")
 *
 *  \return	        0 on success or negative error number
 */
static int32 hm8131_init( int32 fd )
{
	int32 	arg 	= 0;
	int32	error = 0;		

	/* set baudrate */
	CHK( (error = ioctl( fd, FIOBAUDRATE, HM8131_BAUDRATE )) < 0);

	/* set 2 stop bits, 8 data bits, parity none(default) */
	CHK( (error = ioctl( fd, SIO_HW_OPTS_GET, arg)) < 0);
	arg |=(STOPB | CS8 |  CREAD | CLOCAL);
	CHK( (error = ioctl( fd, SIO_HW_OPTS_SET, arg)) < 0);

	/* also Reset Device to default state */
	CHK(( hm8131_send( fd, "RST" )) < 0 );
	UOS_Delay( 500 );
	
ABORT:
	return error;
}



/****************************** hm8131_AmplitudeSet **************************/
/** Set the Amplitude on the HM8131
 *
 *  \param fd         \IN serial handle returned from open("/tyCo/1")
 *  \param amplitude  \IN float value of Amplitude to set
 *
 *  \return	        amplitude as floating point value or 0 on error
 */
static int32 hm8131_AmplitudeSet(int32 fd, float amplitude)
{
	
	char buf[BUFFERSIZE];
	memset(buf, 0x0, BUFFERSIZE);
	sprintf(buf, "AMP:%f", amplitude );

	CHK( (hm8131_send( fd, buf )) < 0 );
 	UOS_Delay(50);
	return 0;

 ABORT:
	return -1;

}


#if 0
/****************************** hm8131_AmplitudeGet **************************/
/** Read back the Amplitude on the HM8131
 *
 *  \param fd       \IN serial handle returned from open("/tyCo/1")
 *
 *  \return	        amplitude as floating point value or 0 on error
 */
static double hm8131_AmplitudeGet(int32 fd)
{

	char buf[32];
	double amp = 0.0;
	memset(buf, 0x0, sizeof(buf));
	CHK( (hm8131_send( fd, "AMP?" )) < 0 );
	CHK( (hm8131_read( fd, buf)) < 0 );

	amp = atof(buf);
 	DBGPR("%s: Amplitude=%f V (buf='%s')\n", __FUNCTION__, amp, buf );
	UOS_Delay(100);
	return amp;
 ABORT:
	return 0;
}
#endif



/****************************** hm8131_FrequencySet **************************/
/** Select the Frequency on HM8131
 *
 *  \param fd       \IN serial handle returned from open("/tyCo/1")
 *  \param frequenz \IN Frequency in kHz
 *
 *  \return	        0 or error code
 */
static int32 hm8131_FrequencySet(int32 fd, u_int32 frequenz )
{
	
	char buf[BUFFERSIZE];
	memset(buf, 0x0, BUFFERSIZE);

	sprintf(buf, "FRQ:%d000", frequenz ); /* in kHz! */
	CHK( (hm8131_send( fd, buf )) < 0 );
 	UOS_Delay(100);
	return 0;
 ABORT:
	return -1;

}

/****************************** hm8131_OffsetSet *****************************/
/** Select the Offset on HM8131
 *
 *  \param fd       \IN serial handle returned from open("/tyCo/1")
 *  \param offs     \IN Offset Voltage to set
 *
 *  \return	        0 or -1 on error
 */
static int32 hm8131_OffsetSet(int32 fd, float offs)
{
	
	char buf[BUFFERSIZE];
	memset(buf, 0x0, BUFFERSIZE);
	sprintf(buf, "OFS:%f", offs );
	CHK( (hm8131_send( fd, buf )) < 0 );
	UOS_Delay(100);
	return 0;

 ABORT:
	return -1;

}


/****************************** hm8131_hm8131_WaveformSet ********************/
/** Select the Waveform to use on HM8131
 *
 *  \param fd       \IN serial handle returned from open("/tyCo/1")
 *  \param wform    \IN Value which selects the Waveform 
 *
 *  \brief  		 0: Sinus 1: square wave 2: Triangle 3: Ramp
 *
 *  \return	        frequency in kHz or 0 in case of error
 */
static int32 hm8131_WaveformSet(int32 fd, u_int32 wform )
{

	DBGPR("%s: Setting Waveform '%s'\n",__FUNCTION__, bf );
	CHK(wform>3);
	switch(wform) {
	case 0:
		CHK( (hm8131_send( fd, "SIN" )) < 0 );
		break;
	case 1:
		CHK( (hm8131_send( fd, "SQR" )) < 0 );
		break;
	case 2:
		CHK( (hm8131_send( fd, "TRI" )) < 0 );
		break;
	case 3:
		CHK( (hm8131_send( fd, "RMP" )) < 0 );
		break;
	default:
		printf("*** Waveform invalid!\n");
	}

	UOS_Delay(100);
	return 0;

 ABORT:
	return -1;
}



#if 0
/****************************** hm8131_FrequencyGet **************************/
/** Read back Frequency from HM8131
 *
 *  \param fd       \IN  serial handle returned from open("/tyCo/1")
 *
 *  \brief  
 *
 *  \return	        frequency in kHz or 0 in case of error
 */
static int32 hm8131_FrequencyGet(int32 fd)
{
	
	char bf[32];
	int32 fr = 0;
	memset(bf, 0x0, sizeof(bf));

	CHK( (hm8131_send( fd, "FRQ?" )) < 0 );
	CHK( (hm8131_read( fd, bf)) < 0 );
	fr = atof(bf);
 	DBGPR("%s: Frequenz = %d kHz (buf='%s'\n", __FUNCTION__, fr/1000, bf );
	UOS_Delay(200);
	return fr;

 ABORT:
	return 0;
}

static int32 hm8131_deviceIdGet(int32 fd, char *buf )
{

	char b[32];
	memset(b, 0x0, sizeof(b));
	CHK( (hm8131_send( fd, "ID?" )) < 0 );
	CHK( (hm8131_read( fd,  b)) < 0 );
	DBGPR("Device ID = '%s'\n", b);
	strncpy(buf, b, 32);
	UOS_Delay(100);
	return 0;
 ABORT:
	return -1;

}
#endif


/******************************* hm8131_OutputEnable *************************/
/** Switch Signal output on HM8131 on or off
 *
 *  \param fd       \IN  serial handle returned from open("/tyCo/1")
 *  \param enable   \IN  1: switch signal On 0: turn signal off 
 *
 *  \brief  
 *
 *  \return	          0 on success or serial driver error
 */
static int32 hm8131_OutputEnable(int32 fd, u_int32 enable)
{
	/* and activate */
	if (enable) {
		CHK( (hm8131_send( fd, "OT1" ) < 0 ));
	} else {
		CHK( (hm8131_send( fd, "OT0" ) < 0 ));
	}

	return 0;
 ABORT:
	return -1;
}


/********************************* m78_init **********************************/
/** Initialize DUT
 *
 *  \param hdl        \IN  MDIS handle returned from M_open("/m78/1")
 *  \param samplerate \IN  Data Akquisition rate, pass to SetStat M78_ADCLK_DIV
 *
 *  \brief  The samplerate is passed like the SetStats M78_ADCLK_DIV values:
 *          48 = 500 kSPS, 24 = 1 MSPS, 12 = 2 MSPS. The tests default is 1MSPS
 *
 *  \return	          0 on success or MDIS error code
 */
static int32 m78_init( int32 hdl, u_int32 samplerate )
{

    /* general */
#if 1
	CHK( M_setstat(hdl, M78_DATA_SOURCE, 		M78_DS_NORMAL) < 0 );
#else
	CHK( M_setstat(hdl, M78_DATA_SOURCE, 		M78_DS_TP_COUNTER) < 0 );
 	CHK( M_setstat(hdl, M_MK_CH_CURRENT, 		0) < 0 );
	CHK( M_setstat(hdl, M78_TESTPATTERN, 		0x555) < 0 );
#endif

    CHK( M_setstat(hdl, M78_CLOCK_SOURCE, 		M78_CS_INTERNAL) < 0 );

    /* 2 MSPS */
    CHK( M_setstat(hdl, M78_ADCLK_DIV,    		samplerate) < 0 );
    CHK( M_setstat(hdl, M78_NUM_CHANNELS, 		4) < 0 );

	/* current channel: 0 */
    CHK( M_setstat(hdl, M_MK_CH_CURRENT,  		0) < 0 );

    /* channel 102 -> read all 4 physical channels at once */
    CHK( M_setstat( hdl, M_MK_CH_CURRENT, 102 ) < 0 );

    /* trigger settings */
    CHK( M_setstat( hdl, M78_TRIG_EDGE,    	1   ) < 0 );
    CHK( M_setstat( hdl, M78_TRIG_LEVEL,   	500 ) < 0 );
    CHK( M_setstat( hdl, M78_TRIG_DELAY,   	0   ) < 0 );

    /* kick Triggering in Software */
    CHK( M_setstat( hdl, M78_TRIG_SOURCE,      M78_TS_SW ) < 0 );
    CHK( M_setstat( hdl, M78_TRIG_HYSTERESIS,  0 ) < 0 );

	/* we want to collect <M78_SAMPLES> samples */
    CHK( M_setstat( hdl, M78_POST_TRIG_SAMPLES, M78_SAMPLES ) < 0 );

    /* set timeout for M_getblock() */
    CHK( M_setstat( hdl, M78_GET_BLOCK_TIMEOUT, 0 ) < 0 );
	
    /* enable interrupts */
    CHK( M_setstat( hdl, M_MK_IRQ_ENABLE, TRUE ) < 0 );

	return 0;
ABORT:
	return -1;

}


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





/*************************** m78_checkSamples*********************************/
/** Central Sample plausibility check function
 *
 *  \param chan       \IN  M78 channel, 0..3
 *  \param dat        \IN  pointer to sample data buffer of M78 
 *  \param nomVal     \IN  Voltage (16bit signed) in Knick for DC accuracy test
 *  \param nrSamples  \IN  Number of Samples from Shot to Check
 *  \param testtypes  \IN  Type of data test, see defines
 *
 *  \brief The function displays the signed channel values with 0 being
 *         the middle axis. An 'x' 
 *
 *  \return	          0 on positive test or -1 if data exceeds allowed 
 *                    tolerance
 */
static int32 m78_checkSamples(u_int32 	chan, 
							  u_int16 	*dat, 
							  int16 	nomVal,	 /* Knick DC value */
							  u_int32 	nrSamples, 
							  u_int32 	testtype )
{

	u_int32 i = 0, j = 0, nrErrors = 0 ;
	u_int16 *pDat 	= dat;
	u_int16 samp[4] = {0,0,0,0}; 	/* raw samples per Chunk */
	int16 val[4] 	= {0,0,0,0};	/* samples converted to 16bit signed */
	int16 delta 	= 0;

	int16 valWav[nrSamples];	/* samples converted to 16bit signed */
	for (i = 0; i < nrSamples; i++)
		valWav[i] = 0;

	switch (testtype) {

	case M78_DC_TEST:  /* Check deviation around the nominal DC Voltage */
		for (i = 0; i < nrSamples; i++) {
			for ( j = 0; j < 4; j++) {
				samp[j] = (*(pDat + j)) & 0xfff;
				/* expand the signed 12bit ADC Value to a 16bit signed */
				if (samp[j] & 0x800) 
					val[j] = 0xf000 | samp[j];
				else
					val[j] = samp[j]; 
			}
			if ( abs( nomVal - val[chan]) > M78_DC_LIMIT )
				nrErrors ++;
	
			pDat +=4;
		}
		break;

	case M78_SIN_TEST:
		/* 1. readout all values of channel <chan> */
		for (i = 0; i < nrSamples; i++) {
			for ( j = 0; j < 4; j++) { /* convert each 4 samples per chunk */
				samp[j] = (*(pDat + j)) & 0xfff;
				if (samp[j] & 0x800) 
					val[j] = 0xf000 | samp[j];
				else
					val[j] = samp[j];
			}
			valWav[i] = val[chan];
			/* printf("%04d %04d %04d %04d\n",val[0],val[1],val[2],val[3]); */
			pDat +=4;
		}

		/* 2. compare delta of sample n and n-1 of this channel*/
		for (i = 1; i < nrSamples; i++) {
			delta = valWav[i] - valWav[i-1];
			/* printf("delta: %d\n", delta ); */
			if ( abs(delta) > M78_SIN_LIMIT )
				nrErrors ++;
		}
		break;
	}

	if (nrErrors > M78_ALLOWED_ERRORS ) {
		printf("*** errors occured: %d\n", nrErrors );
		return -1;
	}
	else
		return 0;

}





/*************************** dumpAsciiGraph **********************************/
/** Simple ASCII Graph display.. poor mans Labview...
 *
 *  \param chan       \IN  M78 channel, 0..3
 *  \param dat        \IN  pointer to sample data buffer of M78 
 *  \param nrSamples  \IN  number of sample lines to display
 *
 *  \brief The function displays the signed channel values with 0 being
 *         the middle axis. An 'x' 
 *
 *  \return	          -
 */
static void	dumpAsciiGraph(u_int32 chan, u_int16 *dat, u_int32 nrSamples)
{

	
	u_int32 i = 0, j = 0, nrSpaces = 0 ;
	u_int16 *pDat = dat;
	char c;
	u_int16 samp[4] = {0,0,0,0}; 	/* raw samples per Chunk */
	int16 val[4] 	= {0,0,0,0};	/* samples converted to 16bit signed */

	for (i = 0; i < nrSamples; i++) { /* convert data for all 4 channels */
		for ( j = 0; j < 4; j++) {
			samp[j] = (*(pDat + j)) & 0xfff;
			/* expand the signed 12bit ADC Value to a 16bit signed */
			if (samp[j] & 0x800) 
				val[j] = 0xf000 | samp[j];
			else
				val[j] = samp[j]; 
		}
		/* The whole Graphs width is MID_AXIS + X_SPAN  */	
		nrSpaces = MID_AXIS + ((X_SPAN * val[chan]) / 2047);
		printf("|");
		for (j=0; j < (MID_AXIS + X_SPAN); j++) {
			c = ' ';
			/* border / middle axis */
			if ((j == MID_AXIS) || (j == MID_AXIS + X_SPAN -1 )) 
				c = '|';
			if (j == nrSpaces )
				c = 'x';
			printf("%c", c);
		}
 
		pDat +=4;
		printf("\n");
	}
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

	/* helpers  */
	char  	*errstr, *str, b[128];
	char 	strKnick[128];
	int32 	error 			= 0;
	u_int32 bM78open = 0, bM28open = 0, bHMopen=0, bIBopen=0 ; 
	u_int32	i = 0;
	u_int16 *sampBuf = NULL;
	int16 	valueExpected 	= 0;
	int32 	hdlHM  		= 0;	/* Handle for Hameg on tyCo1 */
	u_int32 timeout     = 0;

	int32 	hdlM28 		= 0;	
/* 	int32 	hdlM72 		= 0;	 */
	int32 	hdlM78 		= 0;	/* DUT */
	char 	tyCo1Buf[128];


	/* variables for passed option values */
	double voltage 		= 0.0;
	double ampl 		= 0.0;
	double offs 		= 0.0;
	u_int32 test		= 0;
	u_int32 wave		= 0;	/* Waveform 	*/
	u_int32 graphs		= 0;	/* Waveform 	*/
	u_int32 rate		= 0;	/* M78 Samplerate, Driver value */
	u_int32 freq 		= 0;	/* HM8131 Frequency, [kHz] */
	u_int32 channel		= 0;
	u_int32 verbose		= 0;

	/* introduce ourselves */
	printf("Test a4270_test1 built %s %s\n", __DATE__, __TIME__ );
	printf("==============================\n");

	/*--------------------+
	|  init Vars          |
    +--------------------*/

	G_gotIRQ = 0;
	memset(b, 	 	 0,	sizeof(b)		);
	memset(strKnick, 	 0,	sizeof(strKnick)	);
	memset(tyCo1Buf, 0,	sizeof(tyCo1Buf));
	CHK( (sampBuf=malloc(M78_SAMPLES)) == NULL);
	

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

	freq = ((str = UTL_TSTOPT("f=")) ? atoi(str) : 20); /* 20 kHz default */
	CHKARG( ((freq<1)||(freq>5000)), "frequency invalid! -f=[1..5000] kHz\n");

	voltage	= ((str = UTL_TSTOPT("v=")) ? atof(str) : 1);
	CHKARG( abs(voltage)>10, "Knick Voltage invalid! max. Voltage=1V\n");

	ampl = ((str = UTL_TSTOPT("a=")) ? atof(str) : 1); 
	CHKARG((ampl<0)||(ampl>4), "HM8131 Voltage invalid! max. Voltage=1V\n");

	rate = ((str = UTL_TSTOPT("r=")) ? atof(str) : 24); /* 1MSPS default */
	CHKARG((rate == 0), "Sample rate invalid! 24 = 1MSPS, 48 = 500kSPS\n");

	offs = ((str = UTL_TSTOPT("o=")) ? atof(str) : 0); 
	CHKARG( abs(offs)>1, "HM8131 offset Voltage invalid! max. Offs=1V\n");

	channel	= ((str = UTL_TSTOPT("c=")) ? atoi(str) : 15); /* 15 = 0xf = all */
	CHKARG( channel>15, "channel invalid! -c=[0..15] (bit combination)\n");

	wave = ((str = UTL_TSTOPT("w=")) ? atoi(str) : 0);  /* default: Sine wave*/
	CHKARG( channel>15, "waveform invalid! -w=[0..3] \n");

	graphs = ((str = UTL_TSTOPT("g=")) ? atoi(str) : 0); 
	CHKARG( ((graphs > 999)), "# of samples to dump invalid. -g=[1..999] \n");

	test	= ((str = UTL_TSTOPT("t=")) ? atoi(str) : 0);
	CHKARG( test>3, "test type invalid! -t=[0/1/2/3]\n");

	verbose = (UTL_TSTOPT("d") ? 1 : 0);	   
	
	/*-----------------------------+
    |  Open & Init Test equipment  |
    +-----------------------------*/

	/* IP Bus controller */
    CHK( ib_open(	HPIB_CTRL_NAME	) < 0 );
	bIBopen = 1;
	CHK( ib_timeout( TIMEOUT ));

	/*  HM8131 */
	CHK( (hdlHM = open ("/tyCo/1", 0x202, 0 )) < 0);
	bHMopen = 1;
	CHK( ( hm8131_init( hdlHM )) < 0 );

	/* M28 for A_4240 */
	CHK( (hdlM28 = M_open("/m28/1")) < 0 );
	bM28open = 1;

	/* CHK( (hdlM72 = M_open("/m72/1")) < 0 ); */
	/* the DUT */
	CHK( (hdlM78 = M_open("/m78/1")) < 0 );
	bM78open = 1;

    /* install signal for irq check */
    CHK( UOS_SigInit( SigHandler ) < 0 );
    CHK( UOS_SigInstall( UOS_SIG_USR1 ) < 0 );
    CHK( M_setstat( hdlM78, M78_SIG_END_SET, UOS_SIG_USR1 ) < 0 );

	/*-----------------------------+
    |  Start Test procedures       |
    +-----------------------------*/

	CHK( (m78_init(hdlM78, rate) < 0 ));
	UOS_Delay(10);
	
	switch (test) {
	case 0:	/* Check DC accuracy with Knick */
		printf("== DC accuracy Test == \n" );
		
		sprintf(strKnick, "x out %f", voltage );
		CHK( ib_output( KNICK_IBADDR, strKnick) < 0 );

		/* Select Source = Knick */
		CHK ( a4270_selectSource( hdlM28, 1 ) < 0 );
		UOS_Delay(100);
		/* route Signals to LEMO connectors (all 4 or selected ones) */
		for ( i = 0; i < 4; i++ ) {
			printf("channel %d: %s\n", i, (channel & (1<<i)) ? "ON":"OFF");
			a4270_selectSignal( hdlM28, i, (channel & (1<<i)) ? 1 : 0);
		}
		UOS_Delay(100);

		/* make a shot.. */
		CHK( m78_getSamples( hdlM78, sampBuf, M78_GET_SAMPLES) < 0);

		/* check data deviation. We test without AD61 so Voltage = -1..1 V .*/
		valueExpected = (int16)(voltage * 0x7ff);
		VERBPR("DC Voltage %fV => expect 0x%04x \n", voltage, valueExpected );

		/* scan all 4 channels result for extensive deviation */
		for (i = 0; i < 4; i++ ) {
			
			if (channel & (1<<i)) {
				if (m78_checkSamples(i, sampBuf, valueExpected, 
									 100, M78_DC_TEST) < 0) {
					printf("*** Channel %d exceeds DC Deviation!\n", i );
					error =-1;
					goto ABORT;
				} else
					printf("Channel %d DC Deviation OK\n", i );
			}
		}	
	
		break;


	case 1:  /* ANA_IN connected to HM8131 */
		printf("== Waveform sampling Test == \n" );
		CHK( hm8131_WaveformSet(  hdlHM, 	wave) < 0 );
		CHK( hm8131_FrequencySet( hdlHM, 	freq));
		CHK( hm8131_AmplitudeSet( hdlHM, 	ampl));
		CHK( hm8131_OffsetSet(    hdlHM, 	offs));

		/* Select Source = HM8131 */
		CHK ( a4270_selectSource( hdlM28, 0 ) < 0 );

		/* route Signals to LEMO connectors (all 4 or selected ones) */
		for ( i = 0; i < 4; i++ ) {
			printf("channel %d: %s\n", i, (channel & (1<<i)) ? "ON":"OFF");
			a4270_selectSignal( hdlM28, i, (channel & (1<<i)) ? 1 : 0);
		}

		/* enable output */
		CHK((hm8131_OutputEnable( hdlHM, 1 )) < 0 );
		UOS_Delay(100);

		/* make a shot.. */
		CHK( m78_getSamples( hdlM78, sampBuf, M78_GET_SAMPLES ) < 0);

		/* dump graph and analyze */
		for ( i = 0; i < 4; i++ ) {
			if (graphs) {
				printf("=== Channel %d Data graph\n", i);
				dumpAsciiGraph(i, sampBuf, graphs );
			}
		}
		for ( i = 0; i < 4; i++ ) {
			CHK(( m78_checkSamples( i, sampBuf, 0, 100, M78_SIN_TEST ) < 0));
			printf("== Channel %d waveform sampling OK\n",i );
		}
	
		break;

	case 2: /* ANA_IN shorten to GND. check that every channel show value 0 */
		printf("== Signals on GND Test == \n" );

		/* Select all 4 ANA_INx connect to GND */
		for ( i = 0; i < 4; i++ ) 
			CHK( a4270_selectSignal( hdlM28, i, 0 ) < 0);

		/* make a shot.. */
		CHK( m78_getSamples(hdlM78, sampBuf, M78_GET_SAMPLES) < 0);
		
		/* If a simple Dump is requested, do so */
		for ( i = 0; i < 4; i++ ) {
			if (graphs) {
				printf("=== Channel %d Data graph\n", i);
				dumpAsciiGraph(i, sampBuf, graphs );
			}
		}

		/* check data for being around +/- 0 when Signal = GND*/
		for (i = 0; i < 4; i++ ) {
			if (channel & ( 1<<i )) {
				CHK((m78_checkSamples(i, sampBuf, 0, 100, M78_DC_TEST) < 0 ));
				printf("== Channel %d GND Test OK \n",i );
			}
		}	
	
		break;

	case 3: /* Irq reception message only */
		printf("== IRQ Receive test == \n" );

		/* make a shot.. */
		CHK( m78_getSamples(hdlM78, sampBuf, 100) < 0);

		/* wait for IRQ, 1 second */
		timeout=10000;
		
		while (!G_gotIRQ /* && timeout */) {
			CHK(timeout==0);
			timeout--; 
		}

		
		printf("== Test OK == \n" );
		/* Nothing more.. SigHandler displays message*/
		break;

	}


	UOS_SigRemove(UOS_SIG_USR1);	
	UOS_Delay(200);
	HITKEY("press any key to continue\n");
	/* close devices */	
	CHK( (hm8131_OutputEnable(hdlHM, 0)) < 0 );
	M_close(hdlM78);
	M_close(hdlM28);
	/* M_close(hdlM72); */
	close( hdlHM );
	ib_close();	
	free(sampBuf);

	return error;

 ABORT:
	
	if (sampBuf)
		free(sampBuf);

	if (bM78open)
		M_close(hdlM78);

	if (bM28open)
		M_close(hdlM28);

	if (bHMopen)
		close( hdlHM );

	if (bIBopen)
		ib_close();	

	return error;

}



/*      printf("0x%04x (%04d) 0x%04x (%04d) 0x%04x (%04d) 0x%04x (%04d)\n", */
/* 		   samp[0], val[0], samp[1], val[1], samp[2], val[2], */
/* 		   samp[3], val[3]); */
