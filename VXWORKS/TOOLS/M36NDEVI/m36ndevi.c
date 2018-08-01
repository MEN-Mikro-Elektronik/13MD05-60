/****************************************************************************/
/*!
 *       Author: ts
 *        $Date: 2007/09/07 15:05:20 $
 *    $Revision: 1.7 $
 *
 *  Description: Utility to calculate standard deviation from M36N ADC Raw data
 *
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m36ndevi.c,v $
 * Revision 1.7  2007/09/07 15:05:20  ts
 * allow reading not only 18bit raw data, but also 16bit converted Section.
 * Criteria is given address: e.g. 0x97fffe00 reads 16bits, 0x97fffe80 the 18bit
 *
 * Revision 1.6  2007/08/28 15:11:51  ts
 * fixed: clean median/deviation arrays, in vxWorks global data remains after
 * function call
 * swapping raw data bytes
 *
 * Revision 1.5  2007/08/23 18:58:12  ts
 * fixed y reading of Channel while hiword/loword not both updated
 * added statistics output in CSV form to e.g. import to Excel
 *
 * Revision 1.4  2007/08/22 11:26:39  ts
 * Delay added to allow data refreshing
 *
 * Revision 1.3  2007/08/22 09:43:36  ts
 * Tested with Simulation Data
 *
 * Revision 1.2  2007/08/21 16:53:09  ts
 * Compiles and loads, calculates median
 *
 * Revision 1.1  2007/08/21 13:09:37  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef SIMULATION_MODE
# include <vxWorks.h>
# include <string.h>
# include <math.h>
# include <memLib.h>
# include <MEN/maccess.h>
# include <MEN/men_typs.h>
# include <MEN/usr_oss.h>
# include <MEN/usr_utl.h>
# include <MEN/testutil.h>
#else
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
#endif


#ifdef SIMULATION_MODE
#define DAT_COUNT 	131072/* 261120 */	/* 10000 */
#else
#define DAT_COUNT 	10000
#endif

#define CHANS		16

/*------------------+
| Global variables  |
+------------------*/

static unsigned int G_baseAdr = 0x81fffe80;

static double G_med[CHANS];				/* median 		*/
static double G_stddev[CHANS];				/* deviation 	*/
static int adc_val[CHANS][DAT_COUNT];	/* the raw 18bit samples */


/*****************************************************************************/
/** program usage
 *
 * \param row		\IN count nr
 *
 * \return 			-
 */
static void dev_usage(void) 
{
	printf("usage: devi <HWadr> [s] [?]\n");
	printf("<HWadr>: complete HW address,e.g. 81fffe80 (MMbase+regoff)\n");
	printf("s: more statistic, if passed dump min/max and distribution\n");
	printf("?: dump this help\n");
}

#ifndef SIMULATION_MODE
/*****************************************************************************/
/** Read one sample of 16 values
 *
 * \param row			\IN count nr of Sample, 0...10000
 * \param conv16Bit		\IN if 1, interpret and read the 16bit corrected data
 *
 * \brief	Convert the 18Bit signed value to a 32bit signed (= int)
 * 			fill leading 14 bits with 1 if sign bit is 1
 * 			so the Value becomes a 32bit signed int
 *
 * \return 			-
 */
static void readADC(unsigned int row, unsigned int conv16bit)
{
	unsigned int i,j;

	volatile unsigned int dummy = 0;
	unsigned int loword=0, hiword=0;
	unsigned int loword1=0, hiword1=0;
	unsigned int loword2=0, hiword2=0, longval=0;
	
	short int val16bit;

	if (row >= DAT_COUNT)
		return;

	if (conv16bit) 
	{/* read from the 16bit converted data section */

		for (i = 0, j = 0; i < (CHANS*2); i+=2,j++ ) {

			loword1 = MREAD_D16( G_baseAdr, i  );
			loword = ((loword1 << 8) & 0xff00 ) | ((loword1 >> 8) & 0xff );
			val16bit =  (int)loword;

			adc_val[j][row]=val16bit; 
		}
	} 
	else 
	{ /* read from the 18bit raw data section */
		for (i = 0, j = 0; i < (CHANS*4); i+=4,j++ ) {

			do {/* prevent read in middle of CPLD updating data! */
				hiword1 = MREAD_D16( G_baseAdr, i + 2 );
				loword1 = MREAD_D16( G_baseAdr, i + 0 );
				hiword2 = MREAD_D16( G_baseAdr, i + 2 );
				loword2 = MREAD_D16( G_baseAdr, i + 0 );
			} while((loword1 != loword2) || (hiword1 != hiword2));

			hiword = ((hiword1 << 8) & 0xff00 ) | ((hiword1 >> 8) & 0xff );
			loword = ((loword1 << 8) & 0xff00 ) | ((loword1 >> 8) & 0xff );

			if (hiword & 0x2) /* fill signed 18bit value to be 32bit */
				hiword |=0xfffc;
			else
				hiword &=0x0001;

			longval = (hiword << 16) | loword;	/* unsigned int  */
			adc_val[j][row]=(int)longval;		/* now becomes signed value */
		}

	}	


	for ( j = 0; j < 180; j++ ) { /* wait about 200 us */
		dummy = MREAD_D16( G_baseAdr, 96 );
		if (dummy == 0x4711)
			printf("fubar!\n");
	}
}
#else

/*****************************************************************************/
/** dummy function to inject test data
 *
 * \return 			-
 */
static void simulateADC(void)
{
	unsigned int i, chn, value, samp;

	/* Fill Array with values given in Data Sheet */
	samp = 0;
	for (i = samp; i < (samp + 5); i++ ) {	
		adc_val[0][i]= 0x10; 
		adc_val[1][i]= -4; 

	}
	samp = i;
	printf("now samp = %d\n", samp);

	for (i = samp; i < (samp + 349); i++ ) {	
		adc_val[0][i]= 0x11; 
		adc_val[1][i]= -3; 
	}
	samp = i;
	printf("now samp = %d\n", samp);
	for (i = samp; i < (samp + 11838); i++ ) {	
		adc_val[0][i]= 0x12; 
		adc_val[1][i]= -2; 
	}
	samp = i;
	printf("now samp = %d\n", samp);
	for (i = samp; i < (samp + 54874); i++ ) {	
		adc_val[0][i]= 0x13; 
		adc_val[1][i]= -1; 
	}
	samp = i;
	printf("now samp = %d\n", samp);

	for (i = samp; i < (samp + 56811); i++ ) {	
		adc_val[0][i]= 0x14; 
		adc_val[1][i]= 0; 
	}
	samp = i;
	printf("now samp = %d\n", samp);
	for (i = samp; i < (samp + 6901); i++ ) {	
		adc_val[0][i]= 0x15; 
		adc_val[1][i]= 1; 
	}
	samp = i;
	printf("now samp = %d\n", samp);

	for (i = samp; i < (samp + 294); i++ ) {	
		adc_val[0][i]= 0x16; 
		adc_val[1][i]= 2; 
	}
	samp = i;
	printf("now samp = %d\n", samp);

}
#endif


/*******************************************************************/
/** get min/max values
 *  
 * \param channel		\IN Number of ADC Channel, 0-15
 * \param max			\IN flag wether to return smallest or largest
 *						    value of the array 
 *
 * \return				minimum or maximum value in that channels data row
 */
static int get_extrema(unsigned int channel, unsigned int max)
{
	int mval = 0;
	unsigned int i = 0;

	if (max) { /* return maximum */
		mval = -2000000000; /* start with a maximum negative */
		for (i=0; i < DAT_COUNT; i++) {
			if (adc_val[channel][i] > mval)
				mval = adc_val[channel][i];
		}

	} else {	/* return minimum */

		mval = 2000000000; /* start with a maximum positive */
		for (i=0; i < DAT_COUNT; i++) {
			if (adc_val[channel][i] < mval)
				mval = adc_val[channel][i];
		}
	}
	return mval;
}


/*******************************************************************/
/** Display for ONE channel how often each value [min/max] occurs
 *  care to treat all values signed!
 *
 * \param channel		\IN Number of ADC Channel, 0-15
 * \param min			\IN start value to count occurance of
 * \param max			\IN end value to count occurance of
 *						    
 * \brief    the goal is to count how often the discrete values
 *           between min and max occur so a gaussian distribution can
 *           be displayed
 *
 * \return				
 *
 */
static void dump_distribution(unsigned int channel, int min, int max)
{
	int value;
	unsigned int i, valCount=0;

	/* use a rather dumb 2way-loop ... */
	for (value = min; value<=max; value ++) { 		
		valCount = 0;
		for (i=0; i < DAT_COUNT; i++) {			
			if (adc_val[channel][i] == value)
				valCount++;
		}
		printf("%d;%d\n", value, valCount);
	}
}



/*******************************************************************/
/** public function for displaying deviation
 *
 */
#ifndef SIMULATION_MODE
int devi( unsigned int adr, unsigned int dostat )
#else
int main( int argc, char *argv[] )
#endif
{

#ifdef SIMULATION_MODE
	int retval=0;
#endif

	unsigned int cnt=0, chan=0, dumpStats=0;
	float s=0.0;
	int minval=0, maxval=0, conv16=0;

	printf("        ___\n");
	printf(" 2  1  \\   ]       _  2\n");
	printf("s = -   |    ( x - x )       ADC Data deviation build %s %s\n",
		   __DATE__, __TIME__ );
	printf("    n  /___]    i\n\n");


	/*  parse cmd line args  */
#ifdef SIMULATION_MODE
	printf("argc = %d\n", argc );
	for (cnt=0; cnt<argc; cnt++)
		printf("argv[%d]=%s ", cnt, argv[cnt] );
	printf("\n");
	if ( (argc==2) && (argv[1][0] == '?')) {
		dev_usage();
		return(0);
	}
	/* dump statistics? */
	if ( ((argc==3) && (argv[2][0] == 's')) ||
		 ((argc==2) && (argv[1][0] == 's'))  ) 
		dumpStats = 1;	

	if (argc == 1) 
		G_baseAdr = 0x81fffe80;

	else if ((argc == 2) || (argc == 3)){ 
		/* assume address was passed */
		retval = sscanf(argv[1],"%x", &G_baseAdr) ;
	}

#else
	printf("passed Adr. 0x%08x. Statistics: %s\n", adr, dostat ? "on":"off" );
	G_baseAdr = adr; 
	dumpStats = dostat;

	/*
	 * check if converted 16bit values(offs. 0x00) are read or 18bit raw data
	 */
	conv16 = !(G_baseAdr & 0x80);

	if (conv16)
		printf("(reading converted 16bit values).\n");

	if (!G_baseAdr)
		G_baseAdr = 0x81fffe80; 
#endif
	printf("using MMod Adr. 0x%08x\n", G_baseAdr );

	/* Cleanout Arrays */
	for (chan = 0; chan < CHANS; chan++) {
		G_med[chan] = 0;
		G_stddev[chan] = 0;
	}

	/* 
	 * 1. gather data 
	 */
#ifndef SIMULATION_MODE
	for (cnt = 0; cnt < DAT_COUNT; cnt++ )	/* read live data... */
		readADC(cnt, conv16);
#else
		simulateADC();
#endif


	/*
	 * 2.1 calculate median 
	 */
	for (chan=0; chan < CHANS; chan ++) {
		for (cnt = 0; cnt < DAT_COUNT; cnt++ ) {
			G_med[chan] += adc_val[chan][cnt];
		}
	}
	for (chan=0; chan < CHANS; chan ++) {
		G_med[chan] /= DAT_COUNT;
	}


	/* 
	 * 2.2 calculate deviation Sigma
	 */
	/* sum up all deviations for each channel */
	for (chan=0; chan < CHANS; chan ++) { 
		for (cnt = 0; cnt < DAT_COUNT; cnt++ ) {
			s = ( adc_val[chan][cnt] - G_med[chan] );
			G_stddev[chan] +=(s*s);
		}
	}
	/* then divide each chan by # of runs */
	for (chan=0; chan < CHANS; chan ++) {
		G_stddev[chan] /= DAT_COUNT;
	}


	/* 3. print results */
	printf("Result with %d Samples:\n", DAT_COUNT );
	for (chan = 0; chan < CHANS; chan ++) {
		printf("Ch. %d: median = %f  Deviation %f\n",
			   chan, G_med[chan], sqrt( G_stddev[chan])  );
	}

	printf("\n");
	/* and additional statistics */
	if (dumpStats) 
	{
		for (chan = 0; chan < CHANS; chan ++) {
			printf("=== Channel %d ===\n", chan);
			printf("Sample;count\n" );
			minval=get_extrema(chan, 0);
			maxval=get_extrema(chan, 1);
			/* printf("min: %d, max: %d\n", minval, maxval ); */
			dump_distribution( chan, minval, maxval );
		}
	}

	return 0;
}
