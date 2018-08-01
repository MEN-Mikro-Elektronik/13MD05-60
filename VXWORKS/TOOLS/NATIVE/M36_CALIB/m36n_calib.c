/*********************  P r o g r a m  -  M o d u l e ***********************/
/*
 *        \file  m36_calib.c
 *
 *      \author  ts
 *        $Date: 2008/01/11 11:19:19 $
 *    $Revision: 1.4 $
 *
 *  	\brief	MEN internal M36N calibration tool
 *           	The calibration process is split in raw data gathering and
 *              correction gain/offset calculation and programming into 
 *              Flash memory.
 *
 *
 *   Measurement principle:
 *   For each Gain/polarity there are 2 (unipol.) or 3 (bipol.) 
 *	 Voltages applied to the DUT.
 *
 * unipolar Calibration is done with A and B0. (0 V , FSR/2 V)
 *  bipolar Calibration is done with A and B1. (-FSR/2 V , FSR/2 V)
 *
 * So, for bipolar voltage the corrected Gain can be obtained twice as exact.
 * Offset at 0V is always measured with applied Voltage 0V
 *
 * [x] = 18bit sign hex value, raw
 * [y] = applied Voltage/FSR 
 *
 *                               [y] U_in/FSR
 * 	 							  ^\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 *								  |\Unipolar/pos. bipolar Range\\\\\\\\\
 *								  |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ 
 *	 							  |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 *								  |\\\\\\\\ x U_in2(+FSR/2 V) = A(x/y) 
 *								  |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 *								  |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 *								  |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 *								  |U_in1 (0V) = B0(x/y) for UNIPOL
 *          <-+----+----+----+----x----+----+----+----+-->[x]
 *			  |	   	   	|		  |         |         |
 *          -FSR      -FSR/2	  |		   FSR/2     FSR
 *	  ////////////////////////////|
 *	//////////////////////////////|
 *	////////////////    x U_in0(-FSR/2) = B1(x/y) for BIPOL
 *	//////////////////////////////|
 *	//////////////////////////////|
 *	////negative bipolar Range////|
 *	//////////////////////////////|
 *	//////////////////////////////|
 *
 *  Variants to calibrate:
 *
 *   04M036N-00 16 analog voltage inputs, 
 *   04M036N-01 8  analog voltage inputs, differential
 *   04M036N-02 16 analog current inputs, single-ended, 
 *   04M036N-03 8  analog current inputs, differential,  
 *
 * When mesurement is done, take reciproke of measured gain (1/measuredGain)
 * because its MULTIPLIED in the FPGA: 
 * realGain * (1/realGain) = 1 
 * in the code:
 *
 *     delta      =  ((double)(xa-xb)) / ((double)(ya-yb)) = true measured gain
 * so  delta      =  ((double)(ya-yb)) / ((double)(xa-xb)) = reciproke gain
 *
 *          recip
 * Also invert measured Offset because its Value is ADDED
 * in the FPGA design. The offset is directly the V_in = 0V. So:
 *		offs    =   -G_n_raw[p][g][1][c];
 *          Corr  
 *---------------------------[ History ] --------------------------------------
 *
 * $Log: m36_calib.c,v $
 * Revision 1.4  2008/01/11 11:19:19  ts
 * Cosmetics (80 chars/line, more in depth comments added)
 *
 * Revision 1.3  2008/01/10 14:35:05  ts
 * Fix: calculate the negative offset and reciproke Gain to write into Flash,
 * not the actual measured values.
 *
 * Revision 1.2  2007/11/12 18:28:06  ts
 * First working Revision
 * - removed unnecessary functions to program flash from a file
 * - added option -u to generate a unity gain table for quick ADC tests
 * - added a M56 as a current switcher module
 * cleaned function headers
 *
 * Revision 1.1  2007/11/09 16:45:32  ts
 * Initial Revision
 *
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/m36_drv.h>
#include <MEN/testutil.h>
/* M8 IEC Bus support */
#include <MEN/IB/ib_drv.h>
#include <MEN/IB/ib_api.h>


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

static void usage(void);
static void PrintError(char *info);


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CHK(expression) \
 if((expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
     #expression,__FILE__,__LINE__);\
     goto ABORT;\
 }

#define KN_TIMEOUT			50
#define	KNICK_IBADDR  		0x08
#define	KNICK				0
#define WAVETEK_IBADDR		0x05
#define WAVETEK				1
#define HPIB_CTRL_NAME		"/m8/1"

/* helper definitions for M56 current switcher */
#define M56_1_NAME			"/m56/1"
#define M56_2_NAME			"/m56/2"

#define C_ON				1
#define C_OFF				0

/* M36N Calibration data location */
#define CALIB_BASE 				0xf0000
#define CALIB_OFF  				0x0f800
#define CALIB_SZ   				0x00800

/* Settling times in ms */
#define INPUT_VOLTAGE_SETTLE_TIME	300
#define INPUT_CURRENT_SETTLE_TIME	300
#define M36N_SETTLE_TIME			200


#define M36N_VAR_00	0	/* M036N-00 + AD01-01	16 Voltage single ended */
#define M36N_VAR_01	1	/* M036N-01	+ AD02-01	 8 Voltage diff 		*/
#define M36N_VAR_02	2	/* M036N-02	+ AD01-02	16 Current single ended */
#define M36N_VAR_03	3	/* M036N-03	+ AD02-02	 8 Current diff			*/


/*
 * how many 18bit Raw Values shall be read from each channel:
 * A word about median Calculation
 *
 * We must not exceed the range value of the storing Variable for adding
 * up NR_RAW_VALUES raw values to prevent wrapping errors. 
 *
 * how large may NR_RAW_VALUES be?
 *
 * [int32] stores maximum Values +/- 2147483647 -> 2147483647/131071 = 16384
 *
 * so NR_RAW_VALUES can be up to 16384
 */

#define NR_RAW_VALUES	10000 	/* max. 16384 ! */
#define ADC_FULLRANGE	131071	/* sign 18bit ADC has range +/- 1<<17  */

/* How many of each Elements are in the arrays  */
#define NR_GAINS		5	/* 5 Gain settings: 1, 2, 4, 8, 16 			*/
#define NR_POLAR		2	/* 2 polarities: unipolar, bipolar 			*/
#define NR_CHANS		1	/* max. 16 Channels on M36 (8 for diff.) 	*/
#define NR_VIN			3	/* apply max. 3 Voltages during calibration */
#define NR_IIN			3	/* apply max. 3 Voltages during calibration */

#define NR_VARS			4	/* Number of Variants we have				*/


#define LEN_CORR_TABLE	0x200	/* calib table len=0x200*int32 (0x800 byte) */


/* trust is good, control is better... V.I.Lenin */
#if NR_RAW_VALUES > 16384
#error "!! NR_RAW_VALUES must not exceed 16384 !!"
#endif

/*------------------+
|   EXTERN          |
+------------------*/
extern int cls();


/*------------------+
|   GLOBALS         |
+------------------*/

/* calib Voltage for each Gain/polarity as Array [gain][pol.] */
static const double G_voltFSR[NR_POLAR][NR_GAINS]={ {10, 5, 2.5, 1.25, 0.625 },
												  {10, 5, 2.5, 1.25, 0.625 }};
static const double	G_fakt[NR_VIN] = {-0.5, 0, 0.5};

/* calib Current, fixed Gain 8 is used but for readability the same array is 
kept for all 5 gains. */
static const double G_currFSR[NR_POLAR][NR_GAINS]= \
           {{0.04,0.02,0.01,0.005,0.0025},
			{0.04, 0.02, 0.01, 0.005, 0.0025 }};

/* 
 * Helper: lookup tables to determine gains/channels per variant 
 * array order is M36N-00 , -01, -02, -03 
 */

static const int G_gainStart[NR_VARS] = {0,0,3,3};
static const int G_gainEnd[NR_VARS]  =  {4,4,3,3};
static const int G_nrChans[NR_VARS] =  {16,8,16,8};

/* 
 * the central arrays for storing measured & calculated values
 * at each Polarity/Gain/V_in/Channel.
 */

/* G_n_raw stores all Raw Values for each parameter */
static int32   G_n_raw[NR_POLAR][NR_GAINS][NR_VIN][NR_CHANS];

/* G_corrTable is the linear table with all data as it is written to Flash */
static int32   G_corrTable[LEN_CORR_TABLE];


/* global MDIS device paths */
static int32   G_pathM56_1;
static int32   G_pathM56_2;

static int32 G_pathM36;
static char  G_text[80];
static char  G_iTxt[100];
static char G_vTxt[200];


/*------------------+
|   PROTOTYPES		|
+------------------*/
static int32 a3850_chan_16(int32 , int );
static int32 a3850_chan_step(int32 ,int, int );
static int32 a3850_chan_diff_step(int32 ,int, int );
static int32 a3850_diff(int32, int );
static int32 setCalibDevice(double );
static int32 setCalibDeviceOff(int);

/********************************* usage ***********************************/
/**  Print program usage
 */
static void usage(void)
{
	printf("Usage: m36_calib <device> [<opts>] [filename]\n");
	printf(" Tool to generate and program M36N calibration data.\n");
	printf("  -p  perform complete calibration and programming cycle.\n");
	printf("  -e  Erase Flash correction table\n");
	printf("  -d  dump calibration Flash data\n");
	printf("  -u  dont really calibrate, just create & program a\n");
	printf("      unity-gain table (Offset=0, Gain=1, for testing\n");
	printf("  -v  be verbose while calibrating\n\n");
	printf("  Calibration Flash Data location: 0xff800 len 0x800.\n");
	printf("\n(c) 2007 by MEN mikro elektronik GmbH\n\n");
}


/***************************************************************************/
/**  helper to free a pointer if previously allocated by malloc
 *
 *  \param mem      \IN malloced pointer to free or NULL
 *
 *  \return	        -
 */
static void do_free(void *mem )
{
	if (mem)
		free(mem);
}


/***************************************************************************/
/**  get_m36_variant - retrieve M36(N) Variant Info
 *
 *  \param path1      \IN   path from M_open for Switching device (M56...)
 *
 *
 *  \brief  official Variants to consider for calibration:
 *
 *   04M036N-00 16 analog voltage inputs, 
 *   04M036N-01 8  analog voltage inputs, differential
 *   04M036N-02 16 analog current inputs, single-ended, 
 *   04M036N-03 8  analog current inputs, differential,  
 *
 *  \return	 0 for M36N-00, 1 for M36N-01 etc...
 */
static int32 get_m36_variant( int32 path )
{

// int32 err = 0;
	int32 variant = 0;
	static u_int16 eeData[256];
	static M_SG_BLOCK blkdata;
	
	memset(eeData, 0, sizeof(eeData));
	blkdata.data = (void*)eeData;
	blkdata.size = 128;

	CHK( M_getstat( path, M_LL_BLK_ID_DATA, (int32*)&blkdata) !=0 );
//	UTL_Memdump("\nData:", (char*)eeData, 32, 2);

	variant = eeData[8]/256;
	return variant;
ABORT:
	return -1;	

}/* get_m36_variant */





/***************************************************************************/
/**  config_a3850_testadapter - switch extern Relais cards for proper routing
 *
 *  \param var      \IN   variant to calibrate -00 ... -03
 *  \param pol      \IN   polarity  0..1
 *  \param gn       \IN   gain      0..4
 *  \param v        \IN   voltage value to select 0..2 (-FS/2, 0, +FS/2)
 *  \param chan     \IN   channel   0..7 / 0..16
 *  \param onoff    \IN   1 when everything switched on, 0 after reading to 
 *						  switch off
 *
 *  \return	 -
 */
static void config_a3850_testadapter(int var, 
									 int pol, 
									 int gn, 
									 int v, 
									 int chan, 
									 int onoff)
{

	int timecal = 500;

	if (onoff == C_ON ) {
		/* ---- path when everything is switched on ---- */
		switch( var )	{
		case M36N_VAR_00:	
			a3850_chan_16(G_pathM56_1, C_ON );
			sprintf(G_text, 
					"Raw data collecting %s GAIN %02d voltage input %3.5f ",
					pol ? " BIPOLAR":"UNIPOLAR",
					1 << gn, G_voltFSR[pol][gn]*G_fakt[v]);

			tu_print(WDW_MSG, 0, 1, G_text);
			UOS_Delay(timecal);
			setCalibDevice(G_voltFSR[pol][gn] * G_fakt[v]);
			break;
		
		case M36N_VAR_01: 
			a3850_chan_16(G_pathM56_1, 	C_ON );
			a3850_diff(G_pathM56_2, 	C_ON );
		
			sprintf(G_text, "Raw data collecting %s GAIN %02d voltage input %3.5f ",    pol ? " BIPOLAR":"UNIPOLAR",1 << gn, G_voltFSR[pol][gn]*G_fakt[v]);

			tu_print(WDW_MSG, 0, 1, G_text);
			UOS_Delay(timecal);
			setCalibDevice(G_voltFSR[pol][gn] * G_fakt[v]);
			break;

		case M36N_VAR_02: 
			a3850_chan_step(G_pathM56_1, chan, C_ON );
			sprintf(G_text, "Raw data collecting %s GAIN %02d current input %3.5f ",	pol ? " BIPOLAR":"UNIPOLAR", 1 << gn, G_currFSR[pol][gn]*G_fakt[v]);

			tu_print(WDW_MSG, 0, 1, G_text);
			UOS_Delay(timecal);
			setCalibDevice(G_currFSR[pol][gn] * G_fakt[v]);
			break;
					
		case M36N_VAR_03: 
			a3850_chan_diff_step(G_pathM56_1, chan, C_ON );
			a3850_diff(G_pathM56_2, C_ON );
			sprintf(G_text, "Raw Data collecting %s GAIN %02d current input %3.5f ",	pol ? " BIPOLAR":"UNIPOLAR",
					1 << gn, G_currFSR[pol][gn]*G_fakt[v]);

			tu_print(WDW_MSG, 0, 1, G_text);							
			UOS_Delay(timecal);
			setCalibDevice(G_currFSR[pol][gn] * G_fakt[v]);
			break;

		}/* Config a3850 Test adapter */
	}
	else
	{
		/* ---- path when everything is switched off ---- */
		switch( var ){
		case M36N_VAR_00:
			tu_print(WDW_MSG, 0, 3 + chan, G_text);
			a3850_chan_16(G_pathM56_1, C_OFF);
			break;
							
		case M36N_VAR_01: 
			tu_print(WDW_MSG, 0, 3 + chan, 
					 "CH[%02d+%02d] : %05x",chan ,chan+8, G_n_raw[pol][gn][v][chan]);
			a3850_chan_16(G_pathM56_1, C_OFF);
			a3850_diff(G_pathM56_2, C_OFF);
			break;
							
		case M36N_VAR_02:
			tu_print(WDW_MSG, 0, 3 + chan, G_text); 
			setCalibDeviceOff(WAVETEK);
			a3850_chan_step(G_pathM56_1, chan, C_OFF);
			break;
					
		case M36N_VAR_03: 
			tu_print(WDW_MSG, 0, 3 + chan, "CH[%02d+%02d] : %05x",
					 chan , chan + 8, G_n_raw[pol][gn][v][chan]);

			a3850_chan_diff_step(G_pathM56_1, chan, C_OFF);
			a3850_diff(G_pathM56_2, C_OFF);
			break;

		}/* Config Calibrator + a3850 Test adapter */
	}
}


/***************************************************************************/
/** m36n_collectLoop - One complete loop to gather raw data from M36N for
 *                     calibration
 *
 *  \param variante    \IN  M36N Variant to calibrate (voltage or current)
 *  \param verb        \IN  verbosity level
 *
 *  \brief 	
 * 		This part of calibrating first just collects data, 
 *		the final calibration values are calculated in a 2nd step
 *
 *  \return	          	success (0) or error (1)
 */
static int m36n_collectLoop(int vari )
{
	int32 p, g, c=0, j, v, adc_val;

	int gain_start		= 	G_gainStart[vari];
	int gain_end		= 	G_gainEnd[vari];
	int nr_channels		= 	G_nrChans[vari];

	switch (vari ) {
	case M36N_VAR_00:	/*Voltage Variants: use all Gains*/
		sprintf(G_iTxt, "calibration M036N-00 + AD01-01 [16 channel voltage single ended]");
		break;
	case M36N_VAR_01: 
		sprintf(G_iTxt, "calibration M036N-01 + AD02-01 [ 8 channel voltage differential]");
		break;							
	case M36N_VAR_02:	/*current variants: fixed gain=8*/
		sprintf(G_iTxt,"calibration M036N-02 + AD01-02 [16 channel current single ended]");
		break;		
	case M36N_VAR_03: 
		sprintf(G_iTxt,"calibration M036N-03 + AD02-02 [ 8 channel current differential]");
		break;
	}

	/* ausgeklammert */
	tu_init_screen( G_iTxt, 0, 0, 0 );
	sprintf(G_text,"                ==== 1. collecting Raw Values: Channels"
			"   0 - 15  =====        ");
	tu_print_revers(WDW_MSG, 0, -1, G_text);
	

	for (p = 0; p < NR_POLAR; p++) { /* Polarity:  0:=uni 1:=bi */
		for (g = gain_start; g <= gain_end; g++) { /* Gain: 1,2,4,8,16 */
			for (v = p ? 0 : 1; v < NR_VIN; v++ ) { /* V_in:-FS/2,0,FS/2 */
				for (c = 0; c < nr_channels; c++  ) {
						
					config_a3850_testadapter( vari, p, g, v, c, C_ON);
					
					UOS_Delay(50);											
					CHK(M_setstat( G_pathM36, M_MK_CH_CURRENT, 	c ));
					CHK(M_setstat( G_pathM36, M36_CH_GAIN, 		g ));
					CHK(M_setstat( G_pathM36, M36_BIPOLAR, 		p )); 
					UOS_Delay(M36N_SETTLE_TIME); /* let all settle */

					/* === the core data gathering loop === */
					for (j = 0; j < NR_RAW_VALUES; j++) {
						CHK( M_getstat(G_pathM36, M36_GET_RAWDAT, &adc_val));
						G_n_raw[p][g][v][c] += adc_val;
					}/* j NR_RAW_VALUES */
					G_n_raw[p][g][v][c] /= NR_RAW_VALUES;
					
					sprintf(G_text,"\nCH(%d): %05x", c, G_n_raw[p][g][v][c]);
					tu_print(WDW_MSG, 50, 3 + c,G_text);
					/* config terminal for channel */
					sprintf(G_text, "CH[%02d]: %05x                                            ", c, G_n_raw[p][g][v][c]);

					/* Config Terminal / Calibrator + A3850 */
					config_a3850_testadapter( vari, p, g, v, c, C_OFF);
					UOS_Delay(100);				
				}/* end channel 0..8/16 */
				printf( "\n");
			}/* NR_VIN */
		}/* GAIN */
	}/* NR_POLAR */	
	return 0;
ABORT:
	return -1;	
	
}/* m36n_collectLoop */

#if 0
sprintf(G_text, "Raw data collecting %s GAIN %02d voltage input %3.5f ",	
		p ? " BIPOLAR":"UNIPOLAR",
		1 << g, G_voltFSR[p][g]*G_fakt[v]);
#endif


/***************************************************************************/
/* m36n_calculateLoop - complete loop to calculate calib data from M36N 
 *                     from previously gathered data
 *
 *  \param variante    \IN  M36N Variant to calibrate (voltage or current)
 *  \param testtable   \IN  flag to decide whether measured data shall be used
 *                          or a unity table for quick test purposes should 
 *                          be programmed. Unity table means that correction 
 *                          offset = 0, correction gain = 1.000
 *  \param verbose     \IN  verbosity level
 *
 *  \brief 	
 * 		This part of calibrating finally calculates the necessary correction
 *      data for offset (at 0 V) and correction Gain 
 *
 *  \return	          	success (0) or error (1)
 */
static int m36n_calculateLoop( int variante, int testtable, int verbose )
{
	int32 p=0, g=0, c=0, offs=0, xa=0, xb=0, ya=0, yb=0;
	u_int32 o_s=0, g_s=0, offs_uns=0;
	double delta=0.0, gn=0.0 ;
	int32 *pCorr = G_corrTable;

	/* use lookup table instead switch...case */
	int gain_start		= 	G_gainStart[variante]; 
	int gain_end		= 	G_gainEnd[variante];
	int nr_channels		= 	G_nrChans[variante];

	cls();
	sprintf(G_text, "                   ==== 2. calculate corr. Gains/Offsets: ====                ");
	tu_print_revers(WDW_MSG, 0, -1, G_text);

	for (p = 0; p < NR_POLAR; p++) { /* 0:= unipol 1:= bipol */
        for (g = gain_start; g <=gain_end; g++) { /* Gain: 1,2,4,8,16 */
			for (c=0; c < nr_channels; c++  ) {

				/* point A(x/y) is always at +FSR/2 */
				xa = G_n_raw[p][g][2][c];
				ya = ( 1 << 16 );

				/* point B(x/y) is either -FSR/2 V(Bipol) or 0V (unipolar) */
				xb = G_n_raw[p][g][1-p][c];
				yb = p ? -( 1 << 16 ) : 0;

				delta = ((double)(ya-yb)) / ((double)(xa-xb));
				/* fixpoint mapping: gain [0..1,99999] = [0x0..0x3ffff] */
				gn = delta*0x20000; 
				offs = -G_n_raw[p][g][1][c];
				
				if (  verbose ){
					sprintf(G_vTxt,	"%s G=%02d C=%02d| xa-xb:%d ya-yb:%d -> delta %f Of = %d Gc = %05x\n", p ? "BI":"UN", 1 << g, c, xa-xb,ya-yb, offs, gn);
					tu_print(WDW_MSG, 0, 2+c, G_vTxt);
				}
				
				if ( testtable ) {
					offs = 0x0; 	/* override for test purpose */
					gn 	 = 0x20000;
				}
				
				/* Attention! 16bit swapping of data occurs on D203 only? */
				offs_uns = (u_int32)offs;
				o_s = (offs_uns >> 16) | (offs_uns <<16);
				g_s = (((u_int32)gn) >> 16) | (((u_int32)gn) << 16);

				pCorr[ ((p<<8)+(g<<5)+(c<<1)) + 0 ] = (int32)o_s;
				pCorr[ ((p<<8)+(g<<5)+(c<<1)) + 1 ] = (int32)g_s;
				UOS_Delay(100);

			}/* for channel */
		}
	}
		
	return(0);

}/* m36n_calculateLoop */

/***************************************************************************/
/** m36n_calibrate - the main calibration function
 *
 *  \param version     \IN  M36N Variant to calibrate (voltage or current)
 *  \param verb        \IN  verbosity level
 *  \param unity       \IN  dont physically calibrate, just generate a
 *                          'unity-table' (offset = 0, gain = 0x20000)
 *
 *  \brief 	
 * 		The main calibration loop is split in two parts: 
 * 			1. data gathering
 * 			2. Gain/offs. calculation
 *
 *  \return	          	success (0) or error (1)
 */
static int m36n_calibrate( u_int32 version, 
						   u_int32 verb, 
						   u_int32 unity )
{
	int32 i;

	if (unity){ 
		sprintf(G_text,"                   ==== 1. generating Test (unity) table ====                 ");
		tu_print_revers(WDW_MSG, 0, -1, G_text);
	} else {
		CHK( m36n_collectLoop(version));
	}/* else if */

//	setCalibDeviceOff(WAVETEK);
	CHK ( m36n_calculateLoop( version, unity, verb ));

	/* Dump correction table if verbosity is on*/
	if (verb) {
		for (i=0; i < 0x200; i++) {
			if (!(i%16))
				printf("\n0x%04x|", i*4);
			printf("%06x ", G_corrTable[i] );
		}
		printf( "\n");
	}

	return(0);
ABORT:
	return (-1);
}


/***************************************************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main(int argc, char *argv[])
{
	char	 	*device=NULL, buf[40], *errstr = NULL, *str = NULL;
	u_int32  	adr = CALIB_BASE+CALIB_OFF, datasize = CALIB_SZ;
	u_int16  	*datbufP = NULL, *rcvbufP = NULL;
	int32	 	erase=0, verbose=0, dump=0, cali=0, unity=0, vari;
	M_SG_BLOCK 	blk;
	int32 		i;

	/* clean arrays */
	memset(G_n_raw, 	0x00, sizeof(G_n_raw));
	memset(G_corrTable,	0x00, sizeof(G_corrTable));

	printf("MEN M36N Calibrate tool built %s %s\n", __DATE__, __TIME__ );

	/*----------------------+
	  |  check arguments      |
	  +----------------------*/
	if( (errstr = UTL_ILLIOPT("epduva=", buf)) ){
		printf("*** %s\n", errstr);
		return(1);
	}

	if( argc < 3 ){
		usage();
		return(1);
	}

	for (device=NULL, i = 1; i < argc; i++)
		if (*argv[i] != '-') {
			device = argv[i];
			break;
		}
	if (!device) {
		usage();
		return(1);
	}

	erase   = (UTL_TSTOPT("e") ? 1 : 0);	/* just erase calib data */
	cali	= (UTL_TSTOPT("p") ? 1 : 0);	/* do a standard calibration */
	dump	= (UTL_TSTOPT("d") ? 1 : 0);	/* dump calibration Flash data */
	unity	= (UTL_TSTOPT("u") ? 1 : 0);	/* program a unity-gain table */
	verbose	= (UTL_TSTOPT("v") ? 1 : 0);	/* be verbose while calibrating */
	vari	= ((str = UTL_TSTOPT("a=")) ? atoi(str) : 0 );

	/*------------------------+
	  | open DUT, testequipment |
	  +------------------------*/
	if ((G_pathM36 = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}
	/* IP Bus controller */
    CHK( ib_open( HPIB_CTRL_NAME ) < 0 );
	CHK( ib_timeout( KN_TIMEOUT ));

	if ((G_pathM56_1 = M_open(M56_1_NAME)) < 0) {
		PrintError("open");
		return(1);
	}

	if ((G_pathM56_2 = M_open(M56_2_NAME)) < 0) {
		PrintError("open");
		return(1);
	}

	vari = get_m36_variant(G_pathM36);
	/* supporting helper functions: erase/program/read Flash */
	if (erase) {
		printf("Erase M36 Calibrate Data section\n");
		M_setstat( G_pathM36, M36_FLASH_ERASE, 0 );
	}

	if (dump) {
		CHK( (rcvbufP = malloc(datasize * sizeof(u_int16))) == NULL );
		memset(rcvbufP, 0x00, datasize * sizeof(u_int16) );
		blk.data = (void*)rcvbufP;
		blk.size = CALIB_SZ;
		printf("Read Flash: size=%d  Address 0x%08x\n", datasize, adr);
		CHK( M_getstat( G_pathM36, M36_BLK_FLASH, (int32*)&blk) !=0 );
		UTL_Memdump("\nData:", (char*)rcvbufP, datasize, 2);
	}


	m36n_calibrate(vari, verbose, unity );
	
	/* Terminal neu aufbauen */
	cls();
	switch( vari )	{
	case M36N_VAR_00:	sprintf(G_text,"M036N-00 + AD01-01   16 Voltage single ended");
		tu_init_screen( G_text, 0, 0, 0 );
//								tu_print(WDW_MSG, 0, 1,G_text);
		break;
		/* M036N-01	+ AD02-01		 8 Voltage diff 		*/
	case M36N_VAR_01:	sprintf(G_text,"M036N-01 + AD02-01    8 Voltage diff");
		tu_print(WDW_MSG, 0, 1,G_text);
		break;
		/* M036N-02	+ AD01-02		16 Current single ended */
	case M36N_VAR_02:	sprintf(G_text,"M036N-02 + AD01-02   16 Current single ended");
		tu_init_screen( G_text, 0, 0, 0 );
//								tu_print(WDW_MSG, 0, 1,G_text);
		break;
		/* M036N-03	+ AD02-02		 8 Current diff			*/
	case M36N_VAR_03: 	sprintf(G_text, "M036N-03 + AD02-02    8 Current diff");
		tu_print(WDW_MSG, 0, 1,G_text);
		break;
	}

	sprintf(G_text, "               ==== 3. programming correction Table into Flash ====           ");
	tu_print_revers(WDW_MSG, 0, -1,G_text);

	sprintf(G_text,"Erase Flash..");
	tu_print(WDW_MSG, 0, 2,G_text);

	CHK( M_setstat( G_pathM36, M36_FLASH_ERASE, 0 ) != 0);
	blk.data = (void*)G_corrTable;
	sprintf(G_text,"Done.Program Flash at 0x%x len 0x%x\n", adr, datasize);
	tu_print(WDW_MSG, 0, 3,G_text);

	CHK( M_setstat( G_pathM36, M36_BLK_FLASH, (int32)&blk));

 /*--------------------+
  |  cleanup           |
  +--------------------*/
	do_free(datbufP);
	do_free(rcvbufP);


	if (M_close(G_pathM36) < 0)
		PrintError("close");

	if (M_close(G_pathM56_1) < 0)
		PrintError("close");

	if (M_close(G_pathM56_2) < 0)
		PrintError("close");

/* exit screen */
	tu_exit();
	return(0);

ABORT:
	if (M_close(G_pathM36) < 0)
		PrintError("close");

	if (M_close(G_pathM56_1) < 0)
		PrintError("close");

	if (M_close(G_pathM56_2) < 0)
		PrintError("close");

	do_free(datbufP);
	do_free(rcvbufP);
/* exit screen */
	tu_exit();
	return(UOS_ErrnoGet());
}



/********************************* PrintError ******************************/
/** Print MDIS error message
 *
 *  \param info       \IN  info string
*/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}


/**************************** M056 Routines ***********************************
 *
 * Testadapter A_3850
 *
 *****************************************************************************/
int32 a3850_chan_16( int32 path, int onOff)
{
	int channel;
	
	if( onOff == 1){
		for(channel = 0;channel < 16;channel++){
			M_setstat(path,0x1002,channel);
			M_write(path,1);
		}
	}
	
	if(onOff == 0){
		for(channel = 0;channel < 16;channel++){
			M_setstat(path,0x1002,channel);
			M_write(path,0);
		}
	}
	return(0);
}/* a_3850_chan_16 */


		 
int32 a3850_chan_step(int32 path,int channel,int onOff)
{
	if( onOff == 1){
		M_setstat(path,0x1002,channel);
		M_write(path,1);
	}
	
	if(onOff == 0){
		M_setstat(path,0x1002,channel);
		M_write(path,0);
	}
	return(0);
}/* a3850_chan_step */


int32 a3850_chan_diff_step(int32 path,int channel,int onOff)
{
	if( onOff == 1){
		M_setstat(path,0x1002,channel);
		M_write(path,1);
		M_setstat(path,0x1002,channel + 8);
		M_write(path,1);
	}
	
	if(onOff == 0){
		M_setstat(path,0x1002,channel);
		M_write(path,0);
		M_setstat(path,0x1002,channel + 8);
		M_write(path,0);
	}
	return(0);

}/* a3850_chan_diff_step */


int32 a3850_diff(int32 path2, int onOff)
{
	
	if( onOff==1){
		M_setstat(path2,0x1002,1);
		M_write(path2,1);
		
		M_setstat(path2,0x1002,2);
		M_write(path2,1);
	}
	
	if( onOff==0){
		M_setstat(path2,0x1002,1);
		M_write(path2,0);
		
		M_setstat(path2,0x1002,2);
		M_write(path2,0);
	}
	return(0);
}/* a_3850 diff mode */


#if 0
int32 a3850_trig(int32 path2, int onOff)
{
	
	if( onOff==1){
		M_setstat(path2,0x1002,4);
		M_write(path2,1);
		M_setstat(path2,0x1002,15);
		M_write(path2,1);
	}
	
	if( onOff==0){
		M_setstat(path2,0x1002,4);
		M_write(path2,0);
		M_setstat(path2,0x1002,15);
		M_write(path2,0);
		
		
	}
	return(0);
}/* a_3850_trig */




int32 a3850Bi(int32 path2, int onOff)
{
	
	if( onOff==1){
		M_setstat(path2,	0x1002,14);
		M_write(path2,	1);
		M_setstat(path2,	0x1002,3);
		M_write(path2,	0);
	}
	
	if( onOff==0){
		M_setstat(path2,	0x1002,14);
		M_write(path2, 0);
		M_setstat(path2,	0x1002,3);
		M_write(path2, 1);
	}
	
	return(0);
}/* a_3850Bi */


#endif
/*********************************************************/
/**  calibDeviceOff - set output of calibrator device off
 *
 *  \param dev       \IN   close Device 0=Knick   1=Wavetek
 *
 *  \return	          0 on success or error code
 */
static int32 setCalibDeviceOff(int calDev)
{
	int32 	err = 0;
	char 	strKnick[32];
	memset( strKnick, 0x00, sizeof(strKnick));

	char 	strWaveTek[128];
	memset( strWaveTek, 0x00, sizeof(strWaveTek));

#if 0
	int m36Type,calDev=0;
	
	m36Type=get_m36_variant(G_pathM36);

	switch(m36Type){
		/* config for KNICK */
		case 0:
		case 1: calDev=0;break;
		/* config for Wavetek */
		case 2:
		case 3: calDev=1;break;
	}
#endif 	
	if(calDev==0){
		sprintf(strKnick, "x NULL"  );
		err = ib_output( KNICK_IBADDR, strKnick);
		return(err);
	}

	if(calDev==1){
		sprintf(strWaveTek, "OUTP:STATe OFF");
		err = ib_output(WAVETEK_IBADDR,strWaveTek);
		return(err);
	}
	return(0);
}

/*****************************************************************************/
/**  setCalibDevice - set input on Knick DC calibrator or Wavetek DC calibrator
 *
 *  \param calDev     \IN   set calibration Device 0=Knick   1=Wavetek
 *  \param Xvalue     \IN   set output value for device
 *
 *  \return	          0 on success or error code
 */
static int32 setCalibDevice(double Xvalue )
{
	int32 	err = 0;
	char 	strKnick[32];
	memset( strKnick, 0x00, sizeof(strKnick));
	char 	strWaveTek[128];
	memset( strWaveTek, 0x00, sizeof(strWaveTek));
	int 	m36Type, calDev=0; 
	
	m36Type=get_m36_variant(G_pathM36);

	switch(m36Type){
		/* config for KNICK */
		case 0:
		case 1: calDev=0;break;
		/* config for Wavetek */
		case 2:
		case 3: calDev=1;break;
	}
//	printf("calDev %d\n",calDev);


	if(calDev==0){

		sprintf(strKnick, "x out %f", Xvalue );
		err = ib_output( KNICK_IBADDR, strKnick);
	
		/* let it settle */
		UOS_Delay(INPUT_VOLTAGE_SETTLE_TIME);
//		return(err);
	}

	if(calDev==1){
		sprintf(strWaveTek, "FUNC DC;:CURR %f", Xvalue );
		err = ib_output( WAVETEK_IBADDR, strWaveTek);
		UOS_Delay(100);
		sprintf(strWaveTek, "OUTP:STATe ON");
		err = ib_output(WAVETEK_IBADDR,strWaveTek);
	
		/* let it settle */
		UOS_Delay(INPUT_CURRENT_SETTLE_TIME);
//		return(err);
	}
	return(0);
}



#if 0
/* config terminal for -01 -03 */
switch(variante){
case M36N_VAR_00:
case M36N_VAR_02: tu_print(WDW_MSG, 0, 3 + c, G_text);break;
case M36N_VAR_01:
case M36N_VAR_03:
	if(c <=1){
		tu_print(WDW_MSG, 0, 3 + c, "CH[%d+ %d] : %05x",c,c+8, G_n_raw[p][g][v][c]);
	}
	else{
		tu_print(WDW_MSG, 0, 3 + c, "CH[%d+%d] : %05x",c,c+8, G_n_raw[p][g][v][c]);
	}
	break;
}/* config terminal for -xx */
#endif
