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
#define HPIB_CTRL_NAME		"/m8/1"

/* helper definitions for M56 current switcher */
// #define M56_NAME			"/m56/1"
#define M56_1_NAME			"/m56/1"
#define M56_2_NAME			"/m56/2"


/* M36N Calibration data location */
#define CALIB_BASE 				0xf0000
#define CALIB_OFF  				0x0f800
#define CALIB_SZ   				0x00800

/* Settling times in ms */
#define INPUT_VOLTAGE_SETTLE_TIME	300
#define M36N_SETTLE_TIME			200

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
#define NR_CHANS		16	/* max. 16 Channels on M36 (8 for diff.) 	*/
#define NR_VIN			3	/* apply max. 3 Voltages during calibration */

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
static const double voltFSR[NR_POLAR][NR_GAINS]={ {10, 5, 2.5, 1.25, 0.625 },
												  {10, 5, 2.5, 1.25, 0.625 }};
static const double	factor[NR_VIN] = {-0.5, 0, 0.5};

/* 
 * the central arrays for storing measured & calculated values
 * at each Polarity/Gain/V_in/Channel.
 */

/* G_n_raw stores all Raw Values for each parameter */
static int32   G_n_raw[NR_POLAR][NR_GAINS][NR_VIN][NR_CHANS];
/* G_corrTable is the linear table with all data as it is written to Flash */
static int32   G_corrTable[LEN_CORR_TABLE];
/* G_nrChans keeps wether 8 or 16 channels are to be calibrated  */
static int32   G_nrChans = NR_CHANS;

int32	 	m56_1_path;
int32	 	m56_2_path;


/**************************** M056 Routines **********************************/
int32 a3850_chan_16(int32 path,int onOff)
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
}/* a3850_chan_16 */
		 
	
int32 a3850_diff(int32 path, int onOff)
{
	
	if( onOff==1){
		M_setstat(path,0x1002,1);
		M_write(path,1);
		
		M_setstat(path,0x1002,2);
		M_write(path,1);
	}
	
	if( onOff==0){
		M_setstat(path,0x1002,1);
		M_write(path,0);
		
		M_setstat(path,0x1002,2);
		M_write(path,0);
	}
	return(0);
}/* a3850 diff mode */


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

}

#if 0
/***************************************************************************/
/**  switch_input_channel - select current input path
 *
 *  \param chan       \IN   M36N Channel to select (0..15)
 *  \param path1      \IN   path from M_open for Switching device (M56...)
 *
 *  \return	          0 on success or error code
 */
static int32 switch_input_channel( int32 path1, int32 chan )
{

	int32 err = 0;
	err=M_setstat( path1, M_MK_CH_CURRENT, 0 );
	err=M_write( path1, chan );
	return(err);
}
#endif

/***************************************************************************/
/**  set_input_voltage - set input on Knick DC calibrator
 *
 *  \param chan       \IN   M36N Channel to select (0..15)
 *  \param path       \IN   path from M_open/ip_open for Generator or Knick
 *
 *  \return	          0 on success or error code
 */
// static int32 set_input_voltage( int32 path, double flVolt )
static int32 set_input_voltage( double flVolt )
{

	int32 	err = 0;
	char 	strKnick[32];
	memset( strKnick, 0x00, sizeof(strKnick));

	sprintf(strKnick, "x out %f", flVolt );
	err=ib_output( KNICK_IBADDR, strKnick);

	/* let it settle */
	UOS_Delay(INPUT_VOLTAGE_SETTLE_TIME);


	return(err);
}

int32 set_knick_off()
{
	int32 	err = 0;
	char 	strKnick[32];
	memset( strKnick, 0x00, sizeof(strKnick));

	sprintf(strKnick, "x NULL"  );
	err= ib_output( KNICK_IBADDR, strKnick);

	return(err);
}


/***************************************************************************/
/** m36n_calibrate - the main calibration function
 *
 *  \param m36path     \IN  path from M_open /m36/1
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
static int m36n_calibrate( int32 m36path, 
						   u_int32 version, 
						   u_int32 verb, 
						   u_int32 unity )
{
	
	int32 i, p, g, c=0, j, v, adc_val, offs, xa, xb, ya, yb;
	u_int32 o_s=0, g_s=0, offs_uns=0;
	double delta=0.0, gn=0.0 ;
	int32 *pCorr = G_corrTable;

	if (unity) 
		printf("==== 1. generating Test (unity) table ====\n");
	else{
		printf("==== 1. collecting Raw Values: =====\n");
		printf("====    Channels   0 - 15      ====\n");
//		a3850_chan_16(m56_1_path,1);
		for (p = 0; p < NR_POLAR; p++) { /* Polarity:  0:=uni 1:=bi */
			for (g = 0; g < NR_GAINS; g++) { /* Gain: 1,2,4,8,16 */
				for (v = p ? 0 : 1; v < NR_VIN; v++ ) { /* V_in:-FS/2,0,FS/2 */

//					set_input_voltage(m36path, voltFSR[p][g] * factor[v]);
					set_input_voltage(voltFSR[p][g] * factor[v]);
					printf( "%sP GAIN %02d Vin %3.4f ",	p ? " BI":"UNI",
							1 << g, voltFSR[p][g]*factor[v]);
					UOS_Delay(1000);

					for (c = 0; c < G_nrChans; c++  ) {
//						M_setstat(m56_1_path,0x1002,c);
//						M_write(m56_1_path,1);
						UOS_Delay(50);											
//						printf( "%d ", c);
						CHK(M_setstat( m36path, M_MK_CH_CURRENT, c ));
						CHK(M_setstat( m36path, M36_CH_GAIN, g ));
						CHK(M_setstat( m36path, M36_BIPOLAR, p )); 
						UOS_Delay(M36N_SETTLE_TIME); /* let all settle */

						for (j = 0; j < NR_RAW_VALUES; j++) {
							CHK( M_getstat(m36path, M36_GET_RAWDAT, &adc_val));
							G_n_raw[p][g][v][c] += adc_val;
						}/* j NR_RAW_VALUES */
						G_n_raw[p][g][v][c] /= NR_RAW_VALUES;
						printf( "\nCH(%d): %05x", c, G_n_raw[p][g][v][c]);
//						printf( "%d %05x", c, G_n_raw[p][g][v][c]);
					} /* end channel 0..8/16 */
					printf( "\n");
				}/* NR_VIN */
			}/* GAIN */
		}/* NR_POLAR */
	}/* else if */

	set_knick_off();
	a3850_chan_16(m56_1_path,0);


	printf( "==== 2. calculate corr. Gains/Offsets: ====\n");
	for (p = 0; p < NR_POLAR; p++) { /* 0:= unipol 1:= bipol */
		for (g=0; g < NR_GAINS; g++) {
			for (c=0; c < G_nrChans; c++  ) {

				/* point A(x/y) is always at +FSR/2 */
				xa = G_n_raw[p][g][2][c];
				ya = ( 1 << 16 );

				/* point B(x/y) is either -FSR/2 V(Bipol) or 0V (unipolar) */
				xb = G_n_raw[p][g][1-p][c];
				yb = p ? -( 1 << 16 ) : 0;
#if 0
				printf(	"%s G=%02d C=%02d| xa=0x%08x xb=0x%08x\n", 
					p ? "BI":"UN", 1 << g, c, xa, xb);
#endif

				/* 
				 * delta = ((double)(xa-xb)) / ((double)(ya-yb)); WRONG!
				 * 10.01.08 ts: 
				 * Take reciproke of measured gain (1/measuredGain)
			 	 * because its MULTIPLIED in the FPGA: 
				 * realGain * (1/realGain) = 1 
			 	 */

				delta = ((double)(ya-yb)) / ((double)(xa-xb));
				/* fixed point mapping: gain [0..1,99999] = [0x0..0x3ffff] */
				gn = delta*0x20000; 

				/* 
				 * 10.01.08 ts: 
				 * Attention! invert measured Offset because its Value is ADDED
			 	 * in the FPGA design. The offset is directly the V_in = 0V  */

				offs = -G_n_raw[p][g][1][c];
				
				if (verb)
					printf("%s G=%02d C=%02d|"
						   " xa-xb:%d ya-yb:%d ->delta %f Of = %d Gc = %05x\n",
							p ? "BI":"UN", 1 << g, c, 
						   xa-xb,ya-yb, offs, gn);

				if (unity) {
					offs = 0x0; 	/* override for test purpose */
					gn 	 = 0x20000;
				}
				/* Attention! 16bit swapping of data occurs on D203 only? */
				offs_uns=(u_int32)offs;
				o_s = (offs_uns >> 16) | (offs_uns <<16);
				g_s = (((u_int32)gn) >> 16) | (((u_int32)gn) << 16);
#if 0				
				printf(	"%s G=%02d C=%02d| "
						"o_s = 0x%08x g_s = 0x%08x (o=0x%08x g=0x%08x)\n", 
						p ? "BI":"UN", 1 << g, c, 
						o_s, g_s, offs, gn );
#endif		
				pCorr[ ((p<<8)+(g<<5)+(c<<1)) + 0 ] = (int32)o_s;
				pCorr[ ((p<<8)+(g<<5)+(c<<1)) + 1 ] = (int32)g_s;
			}
		}
	}

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
	int32	 	erase=0, verbose=0, dump=0, cali=0, unity=0, m36path, vari;
//	int32	 	m56path, m56_1_path, m56_2_path;
//	int32	 	m56_1_path, m56_2_path;
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
	if ((m36path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}
	/* IP Bus controller */
    CHK( ib_open( HPIB_CTRL_NAME ) < 0 );
	CHK( ib_timeout( KN_TIMEOUT ));

	if ((m56_1_path = M_open(M56_1_NAME)) < 0) {
		PrintError("open");
		return(1);
	}

	if ((m56_2_path = M_open(M56_2_NAME)) < 0) {
		PrintError("open");
		return(1);
	}

	vari = get_m36_variant(m36path);
	switch(vari){
		case 0: printf("\nCalibration M36N-00\n\n");break;
		case 1: printf("\nCalibration M36N-01\n\n");break;
		case 2: printf("\nCalibration M36N-02\n\n");break;
		case 3: printf("\nCalibration M36N-03\n\n");break;
	}
	/* supporting helper functions: erase/program/read Flash */
	if (erase) {
		printf("Erase M36 Calibrate Data section\n");
		M_setstat( m36path, M36_FLASH_ERASE, 0 );
	}

	if (dump) {
		CHK( (rcvbufP = malloc(datasize * sizeof(u_int16))) == NULL );
		memset(rcvbufP, 0x00, datasize * sizeof(u_int16) );
		blk.data = (void*)rcvbufP;
		blk.size = CALIB_SZ;
		printf("Read Flash: size=%d  Address 0x%08x\n", datasize, adr);
		CHK( M_getstat( m36path, M36_BLK_FLASH, (int32*)&blk) !=0 );
		UTL_Memdump("\nData:", (char*)rcvbufP, datasize, 2);
	}

	if (cali || unity) {
		switch (vari) {	/* calibrate the # of channels according to Variant */
			case 0:
			case 2:
				G_nrChans = NR_CHANS;	
					break;
			case 1:
			case 3:
				G_nrChans = NR_CHANS/2;	
					break;
			default:
				printf("*** internal error: invalid Variant!\n");
				goto ABORT;
		}/* switch vari */
		
//		m36n_calibrate(m36path, m56path, vari, verbose, unity );
		m36n_calibrate(m36path, vari, verbose, unity );
		printf( "==== 3. programming correction Table: =====\n");
		printf("Erase Flash..");
		CHK( M_setstat( m36path, M36_FLASH_ERASE, 0 ) != 0);
 		blk.data = (void*)G_corrTable;
		printf("Done.\nProgram Flash at 0x%x len 0x%x\n", adr, datasize);
        CHK( M_setstat( m36path, M36_BLK_FLASH, (int32)&blk));
	}

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	do_free(datbufP);
	do_free(rcvbufP);
#if 0
	if (M_close(m36path) < 0)
		PrintError("close");

	if (M_close(m56_1_path) < 0)
		PrintError("close");

	if (M_close(m56_2_path) < 0)
		PrintError("close");

	return(0);
#endif
ABORT:
	if (M_close(m36path) < 0)
		PrintError("close");

	if (M_close(m56_1_path) < 0)
		PrintError("close");

	if (M_close(m56_2_path) < 0)
		PrintError("close");

	do_free(datbufP);
	do_free(rcvbufP);
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


/********************************* M056 Routines ***********************************
 *
 * Testadapter A_3850
 *
 **********************************************************************************/
int32 a_3850_chan_16(int32 m56_1_path,int onOff)
{
	int channel;
	
	if( onOff == 1){
		for(channel = 0;channel < 16;channel++){
			M_setstat(m56_1_path,0x1002,channel);
			M_write(m56_1_path,1);
		}
	}
	
	if(onOff == 0){
		for(channel = 0;channel < 16;channel++){
			M_setstat(m56_1_path,0x1002,channel);
			M_write(m56_1_path,0);
		}
	}


	return(0);
}/* a_3850_chan_16 */
		 
int32 a3850_chan_step(int32 m56_1_path,int channel,int onOff)
{
	if( onOff == 1){
		M_setstat(m56_1_path,0x1002,channel);
		M_write(m56_1_path,1);
	}
	
	if(onOff == 0){
		M_setstat(m56_1_path,0x1002,channel);
		M_write(m56_1_path,0);
	}
	return(0);
}/* a3850_chan_step */

static int32 a3850_chan_diff_step(int32 m56_1_path,int channel,int onOff)
{
	if( onOff == 1){
		M_setstat(m56_1_path,0x1002,channel);
		M_write(m56_1_path,1);
		M_setstat(m56_1_path,0x1002,channel + 8);
		M_write(m56_1_path,1);
	}
	
	if(onOff == 0){
		M_setstat(m56_1_path,0x1002,channel);
		M_write(m56_1_path,0);
		M_setstat(m56_1_path,0x1002,channel + 8);
		M_write(m56_1_path,0);
	}
	return(0);
}/* a3850_chan_diff_step */

int32 a_3850_diff(int32 m56_2_path, int onOff)
{
	
	if( onOff==1){
		M_setstat(m56_2_path,0x1002,1);
		M_write(m56_2_path,1);
		
		M_setstat(m56_2_path,0x1002,2);
		M_write(m56_2_path,1);
	}
	
	if( onOff==0){
		M_setstat(m56_2_path,0x1002,1);
		M_write(m56_2_path,0);
		
		M_setstat(m56_2_path,0x1002,2);
		M_write(m56_2_path,0);
	}
	return(0);
}/* a_3850 diff mode */

int32 a_3850_trig(int32 m56_2_path, int onOff)
{
	
	if( onOff==1){
		M_setstat(m56_2_path,0x1002,4);
		M_write(m56_2_path,1);
		M_setstat(m56_2_path,0x1002,15);
		M_write(m56_2_path,1);
	}
	
	if( onOff==0){
		M_setstat(m56_2_path,0x1002,4);
		M_write(m56_2_path,0);
		M_setstat(m56_2_path,0x1002,15);
		M_write(m56_2_path,0);
		
		
	}
	return(0);
}/* a_3850_trig */

int32 a3850Bi(int32 m56_2_path, int onOff)
{
	
	if( onOff==1){
		M_setstat(m56_2_path,0x1002,14);
		M_write(m56_2_path,1);
		M_setstat(m56_2_path,0x1002,3);
		M_write(m56_2_path,0);
	}
	
	if( onOff==0){
		M_setstat(m56_2_path,0x1002,14);
		M_write(m56_2_path,0);
		M_setstat(m56_2_path,0x1002,3);
		M_write(m56_2_path,1);
	}
	
	return(0);
}/* a_3850_trig */


static int32 m56_1_open()
{
	if ((m56_1_path = M_open(M56_1_NAME)) < 0) {
		PrintError("open");
		return(1);
	}
	return 0;
}

static int32 m56_2_open()
{
	if ((m56_2_path = M_open(M56_2_NAME)) < 0) {
		PrintError("open");
		return(1);
	}
	return 0;
}