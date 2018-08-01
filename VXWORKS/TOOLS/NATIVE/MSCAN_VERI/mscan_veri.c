/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  mscan_veri.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2006/08/11 12:17:00 $
 *    $Revision: 1.6 $
 *
 *  	 \brief  Verification program for MSCAN-like CAN controllers
 *
 *     Switches: mode select:
 *				  MSCAN_VERI_MGT5100EVB - to run on MGT5100 IceCube/MSCAN
 *				  MSCAN_VERI_EM04		- to run on EM04/Boromir
 *                MSCAN_VERI_PP01       - to run on PP01
 *                MSCAN_VERI_KD12       - to run on KD12
 *                MSCAN_VERI_KM01       - to run on KM01
 *                MSCAN_VERI_SC15B      - to run on SC15B
 *                MSCAN_VERI_SC15C      - to run on SC15C
 *				 VXWORKS/OS9000
 *               USE_ONLY_ONE_BUFFER    - just tx buffer 0 is used
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mscan_veri.c,v $
 * Revision 1.6  2006/08/11 12:17:00  rtrübenbach
 * added:
 * + MSCAN_VERI_SC15B
 * + MSCAN_VERI_SC15C
 * + USE_ONLY_ONE_BUFFER (needs mscan_veri.cfg 1.5)
 * + RX_BUF_NO
 *
 * Revision 1.5  2005/02/15 15:39:39  ufranke
 * added KM01
 *
 * Revision 1.4  2004/05/12 09:13:11  AJendrusch
 * adapted for EM05 (AJendrusch)
 *
 * Revision 1.3  2004/02/12 11:12:50  UFranke
 * adapted for KD12
 *
 * adapted for KD12
 * Revision 1.2  2004/01/21 11:22:46  Rlange
 * adapted for KX03 and PP01
 *
 * Revision 1.4  2003/08/27 08:41:46  kp
 * adapted to mscan_veri.can 1.3
 *
 * Revision 1.3  2003/03/18 12:44:27  kp
 * Sent IDs begin with ID 10
 *
 * Revision 1.2  2003/02/07 14:52:10  kp
 * don't access TARQ/TAACK
 *
 * Revision 1.1  2003/02/07 12:06:48  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifdef MSCAN_VERI_MGT5100EVB
# define MSCAN_IS_ODIN
# define CONFIG_MBAR 0xf0000000
# define CANCLOCK 33000000
#endif

#ifdef MSCAN_VERI_PP01
# define MSCAN_IS_ODIN
# define CONFIG_MBAR 0xf0000000
# define CANCLOCK 64000000
#endif

#ifdef MSCAN_VERI_KM01
# define MSCAN_IS_ODIN
# define CONFIG_MBAR 0xf0000000
# define CANCLOCK 64000000
#endif

#ifdef MSCAN_VERI_KX03
# define MSCAN_IS_BOROMIR
# define CANCLOCK 32000000
#endif

#ifdef MSCAN_VERI_KD12
# define MSCAN_IS_BOROMIR
# define CANCLOCK 32000000
#endif

#ifdef MSCAN_VERI_EM04
# define MSCAN_IS_BOROMIR
# define CANCLOCK 32000000
#endif

#ifdef MSCAN_VERI_EM05
# define MSCAN_IS_BOROMIR
# define CANCLOCK 32000000
#endif

#ifdef MSCAN_VERI_SC15B
# define MSCAN_IS_BOROMIR
# define CANCLOCK 16000000
#define RX_BUF_NO 5
#endif

#ifdef MSCAN_VERI_SC15C
# define MSCAN_IS_BOROMIR
# define CANCLOCK 16000000
#define RX_BUF_NO 5
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/maccess.h>
#include <MEN/mscan.h>

#ifdef MSCAN_IS_ODIN
# include <MEN/mgt5200.h>
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifndef RX_BUF_NO
 #define RX_BUF_NO 4 /* default: 4 rx buffers */
#endif

#define CHKABORT if(++G_errCnt>=G_maxErrCnt) goto abort

#define CHK(expression) \
 if( !(expression)) {\
	 printf("\n*** mscan_veri: Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     CHKABORT;\
 }

#define CHKEX(expression, args ) \
 if( !(expression)) {\
	 printf("\n*** mscan_veri: Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     printf args;\
     printf("\n");\
     CHKABORT;\
 }

/** CAN message and filter flags */
typedef enum {
	MSCAN_EXTENDED=0x1,			/**< interpret ID as extended ID */
	MSCAN_RTR=0x2,				/**< remote transmit request bit */
} MSCAN_FLAGS;

/** CAN frame */
typedef struct{
	u_int32 id;					/**< CAN ID of frame */
	/*MSCAN_FLAGS*/u_int32 flags;/**< ORed flags: MSCAN_EXTENDED/MSCAN_RTR */
	u_int8  dataLen;			/**< data length (0..8) */
	u_int8 	data[8];			/**< data */
	u_int8	_pad;				/**< filler  */
} MSCAN_FRAME;

/** MSCAN filter definition */
typedef struct{

    /** acceptance code
	 * Each non-masked bit must match the received CAN frame's ID.
	 *
	 * - For standard IDs: Bits 10..0
	 * - For extended IDs: Bits 28..0
	 */
	u_int32 code;

    /** acceptance mask.
	 * 	Any bit set to zero is compared, any bit set to 1 is ignored
	 *  For bit numbers, see \em code field.
	 */
	u_int32 mask;

    /** Code flags
	 * ORed flags value of MSCAN_EXTENDED or MSCAN_RTR
	 *
	 * - #MSCAN_EXTENDED: setup filter for extended identifiers. Block
	 *   standard identifiers.
	 *
	 * - #MSCAN_RTR: If mflags.MSCAN_RTR is 1, then the received frame's
	 *   RTR bit must be of the same value as this bit.
	 */
	/*MSCAN_FLAGS*/ u_int32 cflags;

	/** Mask flags
	 *
	 * - #MSCAN_RTR: If set to 1, then cflags.MSCAN_RTR bit must
	 *   match received frame's RTR.
	 */
	/*MSCAN_FLAGS*/ u_int32 mflags;
} MSCAN_FILTER;


/** CANalyzer communication commands */
typedef enum {
	AcStop=0,
	AcTx=1,
	AcRx=2,
	AcRxOver=3,
	AcRxOver2=4,
	AcRxTx=5
} AC_CMD;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static MACCESS G_ma=(MACCESS)NULL;
static MSCAN_FILTER G_flt[2];		/**< currently applied filter config */

static int G_txPrio;
static int G_errCnt;
static int G_maxErrCnt;

#ifdef MSCAN_VERI_EM05
int32 mscan_basicreg(int can);
int32 mscan_loopb( int bitrate,  int testNo, int can );
int32 mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_EM04
int32 mscan_basicreg(int can);
int32 mscan_loopb( int bitrate,  int testNo, int can );
int32 mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_PP01
int32 pp01_mscan_basicreg(int can);
int32 pp01_mscan_loopb( int bitrate,  int testNo, int can );
int32 pp01_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_KM01
int32 km01_mscan_basicreg(int can);
int32 km01_mscan_loopb( int bitrate,  int testNo, int can );
int32 km01_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_KX03
int32 kx03_mscan_basicreg();
int32 kx03_mscan_loopb( int bitrate,  int testNo, int can );
int32 kx03_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_KD12
int32 kd12_mscan_basicreg();
int32 kd12_mscan_loopb( int bitrate,  int testNo, int can );
int32 kd12_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_SC15B
int32 sc15b_mscan_basicreg( int can );
int32 sc15b_mscan_loopb( int bitrate,  int testNo, int can );
int32 sc15b_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef MSCAN_VERI_SC15C
int32 sc15c_mscan_basicreg( int can );
int32 sc15c_mscan_loopb( int bitrate,  int testNo, int can );
int32 sc15c_mscan_alyzer( int bitrate, int testNo, int can );
#endif

#ifdef VXWORKS
#include <vxworks.h>
#include <taskLib.h>
#include <sysLib.h>
#include <intLib.h>
static void Delay(int ms)
{
	u_int32 ticksToWait = (ms * sysClkRateGet()) / 1000;

	if( ((ticksToWait * 1000) / sysClkRateGet()) != ms )
		ticksToWait++;

	taskDelay( ticksToWait );
}

#endif

#ifdef OS9000
#include <cglob.h>
#include <modes.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>

char *usage_str = "\
Syntax:   mscan_veri <opts>\n\
Function: Verification suite for CAN Boromir on EM04\n\
Options:\n\
  -b=<addr>    Base address of Boromir regs [90000600]\n\
  -r           run basic register test (0=all)\n\
  -l=<num>     run loopback tests (0=all)\n\
  -a=<num>     run canalyzer test (0=all)\n\
  -s=<bps>     run tests at bitrate <speed> [250000]\n\
  -x=<cnt>     abort after <cnt> errors     [1]\n\
";


int usage(int no)
{
    fprintf(stderr, "%s", usage_str );

    fprintf(stderr, "(c) 2003 by men GmbH\n");

    exit(no);
	return 0;
}


int main( int argc, char *argv[] )
{
	u_int32 base = 0x90000600;
	u_int32 speed = 250000;
	int rv=0;

	char *optp;

	G_maxErrCnt = 1;

	if( UTL_TSTOPT("?")) usage(0);

	if( (optp = UTL_TSTOPT("b=")))
		base = strtoul( optp, NULL, 16 );

	if( (optp = UTL_TSTOPT("s=")))
		speed = strtoul( optp, NULL, 10 );

	if( (optp = UTL_TSTOPT("x=")))
		G_maxErrCnt = strtoul( optp, NULL, 10 );

	G_ma = (MACCESS)base;
	_os_permit( (void *)base, 0x100, S_IREAD|S_IWRITE, _procid );

	printf("Boromir test CTL1=0x%08x\n", base+MSCAN_CTL1 );

	if( UTL_TSTOPT("r")){
		rv = mscan_basicreg();
	}
	if( (rv == 0) && (optp = UTL_TSTOPT("l="))){
		int tests = atoi(optp);
		rv = mscan_loopb(speed, tests);
	}
	if( (rv == 0) && (optp = UTL_TSTOPT("a="))){
		int tests = atoi(optp);
		rv = mscan_alyzer(speed, tests);
	}
	printf("Exit status: %d\n", rv );
	return rv ? 1:0;
}

static void Delay(int ms)
{
	UOS_Delay(ms);
}



#endif /* OS9000 */



static void Prepare(int can)
{
	G_txPrio=0;
	G_errCnt=0;
	G_ma = NULL;
	printf("\n CAN Prepare entered \n");

#ifdef MSCAN_VERI_MGT5100EVB
	/* assign CAN pins on MGT5100*/
	{
		u_int32 tmp;

		tmp = MREAD_D32( (MACCESS)CONFIG_MBAR, MGT5100_GPIOPCR );
		tmp &= ~0x3f000000;
		tmp |=  0x10000000;		/* ALT CAN position: CAN1 on I2C1 */
		MWRITE_D32( (MACCESS)CONFIG_MBAR, MGT5100_GPIOPCR, tmp );
	}
	G_ma = (MACCESS)(CONFIG_MBAR+MGT5100_CAN1);
#endif

#if defined(MSCAN_VERI_PP01) || defined(MSCAN_VERI_KM01)
	{
		u_int32 tmp;

		tmp = MREAD_D32( (MACCESS)CONFIG_MBAR, MGT5200_GPIOPCR );
		tmp &= ~MGT5200_GPIOPCR__PCS2_MASK;
		tmp |=  MGT5200_GPIOPCR__PCS2_CAN;		/* PSC2: CAN1 CAN2 */
		MWRITE_D32( (MACCESS)CONFIG_MBAR, MGT5200_GPIOPCR, tmp );
	}
	if( !can ){
		G_ma = (MACCESS)(CONFIG_MBAR+MGT5200_CAN1);
	}
	else{
		G_ma = (MACCESS)(CONFIG_MBAR+MGT5200_CAN2);
	}
#endif

#ifdef MSCAN_VERI_KX03
	switch( can ){
		case 0:
			G_ma = (MACCESS)0x80000100;
			break;
		case 1:
			G_ma = (MACCESS)0x80000400;
			break;
	}
#endif /* MSCAN_VERI_KX03 */

#ifdef MSCAN_VERI_KD12
	switch( can )
	{
		case 0:
			G_ma = (MACCESS)0x80040100;
			break;
	}
#endif /* MSCAN_VERI_KD12 */

#ifdef MSCAN_VERI_EM05
	switch( can )
	{
		case 0:
			G_ma = (MACCESS)0xe5140100;
			break;
		case 1:
			G_ma = (MACCESS)0xe5140200;
			break;
	}
	printf("\n CAN Prepare for EM05: CAN - %x \n", G_ma);
#endif /* MSCAN_VERI_EM05 */

#ifdef MSCAN_VERI_SC15B
	switch( can )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			G_ma = (MACCESS)(0x90000200 + can*0x100);
			break;
	}
#endif /* MSCAN_VERI_SC15B */

#ifdef MSCAN_VERI_SC15C
	switch( can )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			G_ma = (MACCESS)(0x90010300 + can*0x100);
			break;
	}
#endif /* MSCAN_VERI_SC15C */

	if( G_ma == NULL )
		printf("*** Prepare line %d\n", __LINE__ );

}


static void OutTest( const char *fmt, ... )
{
	va_list argptr;
	va_start(argptr, fmt);
	vprintf( fmt, argptr );
	printf("...\n");
	va_end(argptr);
}

static void OutSub( const char *fmt, ... )
{
	va_list argptr;
	va_start(argptr, fmt);
	printf("  Testing ");
	vprintf( fmt, argptr );
	printf("...\n");
	va_end(argptr);
}

/********************************** mk_rand_pat *****************************
 *
 *  Description:  Creates a random pattern using a fast random algorithm.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  address - only used to dermine when to create a new random
 *							pattern. If address is long aligned, a new
 *							random pattern is created, otherwise the old
 *							random value is shifted according to size.
 *				  oldpat  - previous pattern returned by mk_rand_pat or
 *							starting value for pattern
 *				  size    - required size for random pattern
 *					        1=byte 2=word 4=long
 *
 *  Output.....:  return  - 8/16/32 bit random pattern
 *
 *  Globals....:  ---
 ****************************************************************************/
static u_int32 MkRandPat( u_int32 address, u_int32 oldpat, u_int8 size )
{
	u_int32 pattern;

	/*return oldpat+1;*/			/* ??? */

	if( size!=4 ){
		pattern = oldpat << (size<<3);	/* emulates rol.l #y,xx */
		pattern |= (oldpat>>(32-(size<<3))) & ((1<<(size<<3))-1);
	} else
		pattern = oldpat;

	if((address & 0x3)==0){
		register u_int32 a = pattern;

		a <<= 11;
		a += pattern;
		a <<= 2;
		pattern += a;
		pattern += 13849;
	}

	return pattern;
}

/**********************************************************************/
/** Test w/r access to register
 *
 * Checks if every bit specified in \a mask can be written and read back
 *
 * Restores org. value after test (if successful)
 *
 * \return 0=ok, -1=error
 */
static int RegTest( int regOff, u_int8 mask )
{
	u_int8 val, reg;
	MACCESS ma = G_ma;
	u_int8 orgVal = MREAD_D8( ma, regOff );

	for( val=0x01; val; val<<=1 ){
		if( val & mask ){
			MWRITE_D8( ma, regOff, val );

			CHKEX( ((reg = MREAD_D8( ma, regOff )) & mask ) == val,
				   ("off %02x regval is %02x", regOff, reg ));
		}
	}

	MWRITE_D8( ma, regOff, mask );
	CHKEX( ((reg = MREAD_D8( ma, regOff )) & mask ) == mask,
				   ("off %02x regval is %02x", regOff, reg) );


	MWRITE_D8( ma, regOff, orgVal );

	return 0;
 abort:
	return -1;
}

#ifndef MSCAN_IS_BOROMIR
/**********************************************************************/
/** Test r/w access to IDAR/IDMR regs
 *
 * Controller must be in INIT mode
 *
 * Destroys register contents
 *
 * \return 0=ok, -1=error
 */
static int IdRegTest(void)
{
	u_int32 pattern, regidx, pass;
	MACCESS ma = G_ma;
	static const int reg[] = {
		MSCAN_IDAR0, MSCAN_IDAR1, MSCAN_IDAR2, MSCAN_IDAR3,
		MSCAN_IDAR4, MSCAN_IDAR5, MSCAN_IDAR6, MSCAN_IDAR7,
		MSCAN_IDMR0, MSCAN_IDMR1, MSCAN_IDMR2, MSCAN_IDMR3,
		MSCAN_IDMR4, MSCAN_IDMR5, MSCAN_IDMR6, MSCAN_IDMR7
	};

	for( pass=1; pass<=4; pass++ ){

		/* fill regs */
		pattern = pass;

		for( regidx=0; regidx<16; regidx++ ){
			pattern = MkRandPat( regidx, pattern, 1 );
			printf("writing %02x -> %02x\n", reg[regidx], (int)(pattern & 0xff) );
			MWRITE_D8( ma, reg[regidx], pattern & 0xff );
		}

		/* verify regs */
		pattern = pass;

		for( regidx=0; regidx<16; regidx++ ){
			u_int8 regval;

			pattern = MkRandPat( regidx, pattern, 1 );
			regval = MREAD_D8( ma, reg[regidx]);

			CHKEX( regval == (pattern & 0xff),
				   ("pass %ld, reg offs 0x%x is 0x%02x sb 0x%02lx",
				   pass, reg[regidx], regval, pattern & 0xff ));
		}
	}
	return 0;
 abort:
	return -1;
}
#endif /* MSCAN_IS_BOROMIR */

/**********************************************************************/
/** Test r/w access to Tx buffer regs
 *
 * Controller must be in NORMAL mode
 *
 * Destroys register contents
 *
 * \return 0=ok, -1=error
 */
static int TxRegTest(void)
{
	u_int32 pattern, regidx, pass, txbuf;
	MACCESS ma = G_ma;
	static const int reg[] = {
		MSCAN_TXIDR0, MSCAN_TXIDR1, MSCAN_TXIDR2, MSCAN_TXIDR3,
		MSCAN_TXDSR0, MSCAN_TXDSR1, MSCAN_TXDSR2, MSCAN_TXDSR3,
		MSCAN_TXDSR4, MSCAN_TXDSR5, MSCAN_TXDSR6, MSCAN_TXDSR7,
		MSCAN_TXBPR, MSCAN_TXDLR
	};

	for( pass=1; pass<=4; pass++ ){

		/* fill regs */
		for( txbuf=0; txbuf<3; txbuf++ ){

			MWRITE_D8( ma, MSCAN_BSEL, 1<<txbuf );

			pattern = pass + (0x100<<txbuf);

			for( regidx=0; regidx<14; regidx++ ){
				pattern = MkRandPat( regidx, pattern, 1 );
				/*printf("pattern %02x\n", pattern & 0xff );*/
				MWRITE_D8( ma, reg[regidx], pattern & 0xff );
			}
		}

		/* verify regs */
		for( txbuf=0; txbuf<3; txbuf++ ){
			u_int8 regval;

			MWRITE_D8( ma, MSCAN_BSEL, 1<<txbuf );

			pattern = pass + (0x100<<txbuf);

			for( regidx=0; regidx<13; regidx++ ){

				pattern = MkRandPat( regidx, pattern, 1 );
				regval = MREAD_D8( ma, reg[regidx]);

				CHKEX( regval == (pattern & 0xff),
					   ("pass %ld, reg offs 0x%x is 0x%02x sb 0x%02lx",
					   pass, reg[regidx], regval, pattern & 0xff ));
			}

			/* DLR has only 4 bits */
			pattern = MkRandPat( regidx, pattern, 1 );
			regval = MREAD_D8( ma, reg[regidx]) & 0x0f;
			CHKEX( regval == (pattern & 0x0f),
				   ("pass %ld, reg offs 0x%x is 0x%02x sb 0x%02lx",
				   pass, reg[regidx], regval, pattern & 0x0f ));

		}
	}
	return 0;
 abort:
	return -1;
}


/**********************************************************************/
/** Basic register access test
 *
 * \return 0=ok, -1=error
 */
#ifdef MSCAN_VERI_PP01
int32 pp01_mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_KM01
int32 km01_mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_KX03
int32 kx03_mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_KD12
int32 kd12_mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_EM04
int32 mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_EM05
int32 mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_SC15B
int32 sc15b_mscan_basicreg(int can)
#endif
#ifdef MSCAN_VERI_SC15C
int32 sc15c_mscan_basicreg(int can)
#endif
{
	MACCESS ma;
	u_int8 reg;


	Prepare(can);
	ma = G_ma;

	OutTest( "Basic register test" );
	OutTest( "  Testing CAN_%d at 0x%x",can+1, G_ma );
	OutSub("INITAK handshake");

	/* try to set CANE */
	MWRITE_D8( ma, MSCAN_CTL1, MSCAN_CTL1_CANE);
	CHK( MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_CANE );

	/* INITAK handshake */
	{
		int timeout=20000;

		MSETMASK_D8( ma, MSCAN_CTL0, MSCAN_CTL0_INITRQ );

		while( (MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_INITAK) == 0  )
			CHK( timeout-- );
	}

	OutSub("register reset values");
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_CTL0 )) & 0xff ) == 0x01,
		   ("regval is %02x", reg ));
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_RFLG )) & 0xff ) == 0x00,
		   ("regval is %02x", reg ));
#ifndef MSCAN_IS_BOROMIR
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_RIER )) & 0xff ) == 0x00,
		   ("regval is %02x", reg ));
#endif
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_TFLG )) & 0x07 ) == 0x07,
		   ("regval is %02x", reg ));
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_TIER )) & 0x07 ) == 0x00,
		   ("regval is %02x", reg ));
#ifndef MSCAN_IS_BOROMIR
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_TARQ )) & 0x07 ) == 0x00,
		   ("regval is %02x", reg ));
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_TAAK )) & 0x07 ) == 0x00,
		   ("regval is %02x", reg ));
#endif
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_BSEL )) & 0x07 ) == 0x00,
		   ("regval is %02x", reg ));


	OutSub("registers in init mode");

#ifndef MSCAN_IS_BOROMIR
	CHK( RegTest( MSCAN_IDAC, 0x30 ) == 0 );
#endif
	CHK( RegTest( MSCAN_CTL1, 0x20 ) == 0 );
#ifndef MSCAN_IS_BOROMIR
	/* ID regs are r/o for boromir */
	CHK( IdRegTest() == 0 );
#endif
	OutSub("registers in normal mode");

	/* leave init mode */
	{
		int timeout=20000;

		MCLRMASK_D8( ma, MSCAN_CTL0, MSCAN_CTL0_INITRQ );

		while( (MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_INITAK) != 0  )
			CHK( timeout-- );
	}

	/* CTL1 must be write protected in normal mode */
	MWRITE_D8( ma, MSCAN_CTL1, 0x20 );
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_CTL1 )) & 0x20 ) == 0x00,
		   ("regval is %02x", reg) );


#ifndef MSCAN_IS_BOROMIR
	CHK( RegTest( MSCAN_RIER, 0x7f ) == 0 );
#endif

#ifdef MSCAN_VERI_KX03
	intDisable(0x20);
#endif
#ifdef MSCAN_VERI_KD12
	intDisable(0x20);
#endif
	CHK( RegTest( MSCAN_TIER, 0x07 ) == 0 );

#ifdef MSCAN_VERI_KX03
	intEnable(0x20);
#endif
#ifdef MSCAN_VERI_KD12
	intEnable(0x20);
#endif
	/* special w/r handling for BSEL */
	MWRITE_D8( ma, MSCAN_BSEL, 0x7 );
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_BSEL )) & 0x07 ) == 0x01,
		   ("regval is %02x", reg ));

	MWRITE_D8( ma, MSCAN_BSEL, 0x6 );
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_BSEL )) & 0x07 ) == 0x02,
		   ("regval is %02x", reg ));

	MWRITE_D8( ma, MSCAN_BSEL, 0x4 );
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_BSEL )) & 0x07 ) == 0x04,
		   ("regval is %02x", reg ));

	MWRITE_D8( ma, MSCAN_BSEL, 0x1 );
	CHKEX( ((reg = MREAD_D8( ma, MSCAN_BSEL )) & 0x07 ) == 0x01,
		   ("regval is %02x", reg ));

	CHK( TxRegTest() == 0 );

	return 0;

 abort:
	return -1;
}



/**********************************************************************/
/** Set up one of the two 32 bit filters
 *
 * Controller must be in INIT mode
 *
 * \param fltNum 	filter number to setup (0 or 1)
 * \param fspec		filter specification (see struct doc)
 *
 */
static void SetFilter( int fltNum, const MSCAN_FILTER *fspec )
{
	MACCESS ma = G_ma;
	u_int32 id;
	static const int mreg[] = {
		MSCAN_IDMR0, MSCAN_IDMR1, MSCAN_IDMR2, MSCAN_IDMR3,
		MSCAN_IDMR4, MSCAN_IDMR5, MSCAN_IDMR6, MSCAN_IDMR7
	};
	static const int areg[] = {
		MSCAN_IDAR0, MSCAN_IDAR1, MSCAN_IDAR2, MSCAN_IDAR3,
		MSCAN_IDAR4, MSCAN_IDAR5, MSCAN_IDAR6, MSCAN_IDAR7
	};
	const int *ar, *mr;


	if( fltNum == 0 ){
		ar = &areg[0];
		mr = &mreg[0];
	}
	else {
		ar = &areg[4];
		mr = &mreg[4];
	}


	/*
	 * Filter config:
	 * IDAM=0	- 2*32 bit acceptance filters
	 */
	MWRITE_D8( ma, MSCAN_IDAC, 0x00 );

	if( fspec->cflags & MSCAN_EXTENDED ){

		/* extended filter */
		id = fspec->mask;

		MWRITE_D8( ma, mr[0], id>>21 );
		MWRITE_D8( ma, mr[1], ((id>>13)&0xe0) | 0x00 | ((id>>15)&0x07));
		MWRITE_D8( ma, mr[2], id>>7 );
		MWRITE_D8( ma, mr[3], (id<<1) |
				   ((fspec->mflags & MSCAN_RTR) ? 0x0:0x1));

		id = fspec->code;

		MWRITE_D8( ma, ar[0], id>>21 );
		MWRITE_D8( ma, ar[1], ((id>>13)&0xe0) | 0x18 | ((id>>15)&0x07));
		MWRITE_D8( ma, ar[2], id>>7 );
		MWRITE_D8( ma, ar[3], (id<<1) |
				   ((fspec->cflags & MSCAN_RTR) ? 0x1:0x0));
	}
	else {

		/* standard filter */
		id = fspec->mask;

		MWRITE_D8( ma, mr[0], id>>3 );
		MWRITE_D8( ma, mr[1], (id<<5) | 0x07 |
				   ((fspec->mflags & MSCAN_RTR) ? 0x0:0x10));
		MWRITE_D8( ma, mr[2], 0xff ); /* really necessary? */
		MWRITE_D8( ma, mr[3], 0xff );

		id = fspec->code;

		MWRITE_D8( ma, ar[0], id>>3 );
		MWRITE_D8( ma, ar[1], (id<<5) |
				   ((fspec->cflags & MSCAN_RTR) ? 0x10:0x0));

	}
	G_flt[fltNum] = *fspec;
}

/**********************************************************************/
/** Software filtering. Check if \a frm matches filter \fspec
 *
 * \param frm		frame to compare
 * \param fspec		filter specification (see struct doc)
 * \returns 0=no hit, 1 hit
 */
static int SwFilter( const MSCAN_FRAME *frm, const MSCAN_FILTER *fspec )
{
	if( (frm->flags & MSCAN_EXTENDED) != (fspec->cflags & MSCAN_EXTENDED))
		return 0;

	if( fspec->mflags & MSCAN_RTR ){
		if( (fspec->cflags & MSCAN_RTR) != (frm->flags & MSCAN_RTR))
			return 0;
	}

	if( (fspec->code & ~fspec->mask) != (frm->id & ~fspec->mask) )
		return 0;

	return 1;
}

/**********************************************************************/
/** Calculate BRP and TSEG values (time quantas) for given bitrate
 *
 *
 *               Tries to find out the best combination of BRP and TSEG:
 *
 *                   bitrate = canclock / (BRP * (TSEG+1))
 *
 *			     (BRP=1..64)
 *			     (TSEG=SYNC+TSEG1+TSEG2=6..25)
 *
 * \param canclock		controller clock [Hz]
 * \param bitrate		bus bitrate [bit/s]
 *
 * \return resulting bitrate [bit/s]
 *		   *calcBrpP	best matching BRP
 * 		   *calcTsegP	best matching TSEG1+TSEG2 sum [time quantas]
 */
static int32 CalcBustime( u_int32 canclock,
						  u_int32 bitrate,
						  u_int32 *calcBrpP,
						  u_int32 *calcTsegP )
{
	u_int32 brp,best_brp=0,tseg,best_tseg=0;
	u_int32 rate,best_rate=0,diff,best_diff;

	best_diff = canclock / 16;			/* max. diff */

	for (brp=1; brp<=64; brp++) {
		for (tseg=6; tseg<=17; tseg++) {
			rate = canclock / (brp * (tseg));
			diff = (bitrate > rate ? bitrate-rate : rate-bitrate);

			if (diff <= best_diff) {		/* better match ? */
				best_brp = brp;			/* store params */
				best_tseg = tseg;
				best_rate = rate;
				best_diff = diff;
			}

			if (diff == 0) 				/* exact match ? */
				goto alldone;
		}
	}

alldone:
	*calcBrpP  = best_brp;			/* return best matching params */
	*calcTsegP = best_tseg;
	return(best_rate);
}

static int EnterInitMode( void )
{
	MACCESS ma = G_ma;
	MWRITE_D8( ma, MSCAN_CTL1, MSCAN_CTL1_CANE);
	CHK( MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_CANE );

	/* INITAK handshake */
	{
		int timeout=20000;

		MSETMASK_D8( ma, MSCAN_CTL0, MSCAN_CTL0_INITRQ );

		while( (MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_INITAK) == 0  )
			CHK( timeout-- );
	}
	return 0;

 abort:
	return -1;
}

static int LeaveInitMode( void )
{
	MACCESS ma = G_ma;
	/* leave init mode */
	{
		int timeout=20000;

		MCLRMASK_D8( ma, MSCAN_CTL0, MSCAN_CTL0_INITRQ );

		while( (MREAD_D8( ma, MSCAN_CTL1 ) & MSCAN_CTL1_INITAK) != 0  )
			CHK( timeout-- );
	}
	return 0;

 abort:
	return -1;
}

static int SetSpeed( int bitrate )
{
	MACCESS ma = G_ma;
	u_int32 brp, tseg, tseg1, tseg2, rate;
	u_int8 btr0=0, btr1=0;

	struct {
		int bitrate;
		u_int8 btr0;
		u_int8 btr1;
	} btrs32MHz[] = {
		{ 1000000, 0x03, 0x14 },
		{  800000, 0x03, 0x16 },
/*		{  500000, 0x03, 0x1c },*/
		{  500000, 0x03, 0x49 },
		{  250000, 0x07, 0x1c },
		{  125000, 0x0f, 0x1c },
		{   50000, 0x27, 0x1c },
		/*{   20000, 0x63, 0x1c }, CHECK???*/
		{   -1 }
	};

	if( CANCLOCK == 32000000 ){
		int i;

		for( i=0; btrs32MHz[i].bitrate >= 0; i++ ){
			if( bitrate == btrs32MHz[i].bitrate ){
				btr0 = btrs32MHz[i].btr0;
				btr1 = btrs32MHz[i].btr1;
				break;
			}
		}
		if( btrs32MHz[i].bitrate < 0 ){
			printf("*** can't set bitrate %d\n", bitrate);
			return -1;
		}
		printf("SetSpeed: bitrate=%d btr0=%02x btr1=%02x\n",
			   bitrate, btr0, btr1 );
	}
	else {
		rate = CalcBustime( CANCLOCK, bitrate, &brp, &tseg );

		if( bitrate == 1000000 )
			tseg1 = tseg * 75 / 100;
		else if( bitrate == 800000 )
			tseg1 = tseg * 80 / 100;
		else
			tseg1 = tseg * 875 / 1000;

		tseg2 = tseg - tseg1;
		tseg1 -= 1;					/* subtract SYNC */

		btr0 = 0x40 | (brp-1);		/* SJW=1 := 2 Tq */
		btr1 = ((tseg2-1)<<4) + (tseg1-1);

		printf("SetSpeed: bitrate=%d rate=%ld brp=%ld tseg=%ld "
			   "tseg1=%ld tseg2=%ld btr0=%02x btr1=%02x\n",
			   bitrate, rate, brp, tseg, tseg1+1, tseg2, btr0, btr1 );
	}


	MWRITE_D8( ma, MSCAN_BTR0, btr0 );
	MWRITE_D8( ma, MSCAN_BTR1, btr1 );
	return 0;

}


static int WaitTx( int tout )
{
	MACCESS ma = G_ma;
	
#ifdef USE_ONLY_ONE_BUFFER
	while( (MREAD_D8( ma, MSCAN_TFLG ) & 0x1) == 0 ){
#else
	while( (MREAD_D8( ma, MSCAN_TFLG ) & 0x7) == 0 ){
#endif /* USE_ONLY_ONE_BUFFER */
		if( tout-- == 0 ){
			goto abort;
		}
	}
	return 0;

 abort:
	return -1;
}

/** wait until all buffers free */
static int WaitTxIdle( int tout )
{
	MACCESS ma = G_ma;

	while( (MREAD_D8( ma, MSCAN_TFLG ) & 0x7) != 0x7 ){
		if( tout-- == 0 ){
			goto abort;
		}
	}
	return 0;

 abort:
	return -1;
}

/**********************************************************************/
/** Write Tx frame in first available tx buffer
 *
 * if \a startNow is TRUE, start frame tx
 *
 * \param frm 	frame to send
 * \param txbuf -1=auto select, start immediately
 *				0..2 = tx buffer number, don't start
 * \param prio  tx buffer priority 0x00=highest, 0xff=lowest, -1=auto
 *				If \a prio is -1, then use a chronological ordering of send
 *				requests
 * \return 0=ok, -1=error
 */
static int TxFrame(const MSCAN_FRAME *frm, int txbuf, int prio)
{
	MACCESS ma = G_ma;
	const int maxTxPrio=254;

	u_int8 sel;
	const u_int8 *dataP = frm->data;
	u_int32 id = frm->id;

	if( prio == -1 ){
		/*
		 * This avoids the priority wrap problem.
		 * When we reach the max. priority limit ,
		 * we wait until all tx buffers get empty and start again
		 * with priority 0 for next frame
		 */
		if( G_txPrio+1 >= maxTxPrio ){
			/*printf("prio wrap\n");*/
			CHK( WaitTxIdle( 50*1000*1000 ) == 0 );
			G_txPrio=0;
		}
		else
			G_txPrio++;
		prio = G_txPrio;
	}

	if( txbuf == -1 ){
		/* follows MSCAN Block Guide, 3.3.1.11 */
		sel = MREAD_D8( ma, MSCAN_TFLG );

		/*printf("selecting tx buffer mask (1) TFLG=0x%x\n", sel );*/

		CHK( sel != 0x00 );			/* check if any buffer free */

		MWRITE_D8( ma, MSCAN_BSEL, sel );	/* select Tx buffer */
	}
	else {
		MWRITE_D8( ma, MSCAN_BSEL, 1<<txbuf );
	}

	MWRITE_D8( ma, MSCAN_TXDSR0, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR1, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR2, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR3, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR4, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR5, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR6, *dataP++ );
	MWRITE_D8( ma, MSCAN_TXDSR7, *dataP++ );

	MWRITE_D8( ma, MSCAN_TXDLR, frm->dataLen );

	if( frm->flags & MSCAN_EXTENDED )
	{
		/*printf("TxFrame MSCAN_EXTENDED id %08x length %d flag %x\n",
		        id, frm->dataLen, frm->flags ); */
		/* extended message */
		MWRITE_D8( ma, MSCAN_TXIDR0, id>>21 );
		MWRITE_D8( ma, MSCAN_TXIDR1, ((id>>13)&0xe0) | 0x18 | ((id>>15)&0x07));
		MWRITE_D8( ma, MSCAN_TXIDR2, id>>7 );
		MWRITE_D8( ma, MSCAN_TXIDR3, (id<<1) |
				   ((frm->flags & MSCAN_RTR) ? 0x1:0x0));
	}
	else 
	{
		/*printf("TxFrame SHORT id %08x length %d flag %x\n",
		        id, frm->dataLen, frm->flags );*/
		MWRITE_D8( ma, MSCAN_TXIDR0, id>>3 );
		MWRITE_D8( ma, MSCAN_TXIDR1, (id<<5) |
				   ((frm->flags & MSCAN_RTR) ? 0x10:0x0));
	}

	MWRITE_D8( ma, MSCAN_TXBPR, prio );

	if( txbuf==-1 ){
		/* write of 1 clears flag:start Tx */

		sel = MREAD_D8( ma, MSCAN_BSEL );

		/*printf("selecting tx buffer mask (2) TFLG=0x%x\n", sel );*/

		MWRITE_D8( ma, MSCAN_TFLG, sel );
	}
	return 0;

 abort:
	return -1;
}


static int LoopBSetup( int bitrate )
{
	MACCESS ma = G_ma;
	const MSCAN_FILTER stdFilter = {
		0,
		0xffffffff,
		0,
		0
	};
	const MSCAN_FILTER extFilter = {
		0,
		0xffffffff,
		MSCAN_EXTENDED,
		0
	};


	CHK( EnterInitMode() == 0 );
	CHK( SetSpeed( bitrate ) == 0 );

	MSETMASK_D8( ma, MSCAN_CTL1, MSCAN_CTL1_LOOPB );

	/*
	 * Filter config:
	 * Set filter to ignore, let all messages pass through
	 */
	SetFilter( 0, &stdFilter );
	SetFilter( 1, &extFilter );

	CHK( LeaveInitMode() == 0 );

	return 0;

 abort:
	return -1;
}

static int WaitRx( int tout )
{
	MACCESS ma = G_ma;

	while( (MREAD_D8( ma, MSCAN_RFLG ) & MSCAN_RFLG_RXF) == 0 ){
		if( tout-- == 0 ){
			goto abort;
		}
	}
	return 0;

 abort:
	return -1;
}



/**********************************************************************/
/** Read Rx frame from Rx FIFO
 *
 *
 *
 * \return 0=ok, -1=error
 */
static int RxFrame( MSCAN_FRAME *frm )
{
	MACCESS ma = G_ma;
	u_int32 id, idr1, idr3;
	u_int8 *dataP = frm->data;


	if( (MREAD_D8( ma, MSCAN_RFLG ) & MSCAN_RFLG_RXF) == 0 ){
		CHKEX( 0, ("RxFrame: no message received") );
	}

	memset( frm, 0, sizeof(*frm));
	frm->flags = 0;

	if( (idr1 = MREAD_D8( ma, MSCAN_RXIDR1 )) & 0x8 ){
		/* extended id */
		id = (u_int32)MREAD_D8( ma, MSCAN_RXIDR0 ) << 21;
		id |= (idr1 & 0x7) << 15;
		id |= (idr1 & 0xe0) << 13;
		id |= (u_int32)MREAD_D8( ma, MSCAN_RXIDR2 ) << 7;
		idr3 = MREAD_D8( ma, MSCAN_RXIDR3 );
		id |= idr3 >> 1;

		if (idr3 & 0x1)
			frm->flags |= MSCAN_RTR;

		frm->flags |= MSCAN_EXTENDED;
	}
	else {
		/* standard ID */
		id = (u_int32)MREAD_D8( ma, MSCAN_RXIDR0 ) << 3;
		id |= idr1 >> 5;
		if (idr1 & 0x10)
			frm->flags |= MSCAN_RTR;
	}
	frm->id = id;

	switch( frm->dataLen = (MREAD_D8( ma, MSCAN_RXDLR ) & 0xf) ){
	case 8:	dataP[7] = MREAD_D8( ma, MSCAN_RXDSR7 );
	case 7:	dataP[6] = MREAD_D8( ma, MSCAN_RXDSR6 );
	case 6:	dataP[5] = MREAD_D8( ma, MSCAN_RXDSR5 );
	case 5:	dataP[4] = MREAD_D8( ma, MSCAN_RXDSR4 );
	case 4:	dataP[3] = MREAD_D8( ma, MSCAN_RXDSR3 );
	case 3:	dataP[2] = MREAD_D8( ma, MSCAN_RXDSR2 );
	case 2:	dataP[1] = MREAD_D8( ma, MSCAN_RXDSR1 );
	case 1:	dataP[0] = MREAD_D8( ma, MSCAN_RXDSR0 );
	case 0:
	default:
		break;
	}

	/* release Rx buffer */
	MWRITE_D8( ma, MSCAN_RFLG, MSCAN_RFLG_RXF );

	if( MREAD_D8( ma, MSCAN_RFLG ) & MSCAN_RFLG_OVRIF ){
		printf("Overrun!\n");
		MWRITE_D8( ma, MSCAN_RFLG, MSCAN_RFLG_OVRIF );
	}
	return 0;

 abort:
	return -1;
}

static void DumpFrame( char *msg, const MSCAN_FRAME *frm )
{
	int i;
	printf("%s: ID=0x%08lx%s%s data=",
		   msg,
		   frm->id,
		   (frm->flags & MSCAN_EXTENDED) ? "x":"",
		   (frm->flags & MSCAN_RTR) ? "RTR":"");

	for(i=0; i<frm->dataLen; i++ ){
		printf("%02x ", frm->data[i] );
	}
	printf("\n");
#if 0
	{
		u_int8 *p=(u_int8 *)frm;
		for(i=0; i<sizeof(*frm);i++)
			printf("%02x ", *p++ );
		printf("\n");
	}
#endif
}

static int LoopbBasic(void)
{
	int i;
	/* frames to send */
	MSCAN_FRAME txFrm[] = {
		/* ID,  flags,          dlen, data */
		{ 0x08, 0,				2,   { 0x0f, 0x0f } },
		{ 0x45, 0,				8,   { 0x01, 0x02, 0x03, 0x04, 0x05,
									   0x06, 0x07, 0x08 } },
		{ 0x13218765,  MSCAN_EXTENDED, 2, { 0x99, 0xcc } },
		{ 0x55, 0,				4,   { 0xff, 0x00, 0x7f, 0x1e } },
		{ 0x124, 0,				0,   { 0 } }
	};
	MSCAN_FRAME rxFrm;

	for( i=0; i<sizeof(txFrm)/sizeof(MSCAN_FRAME); i++ ){

		/* send one frame */
		CHK( TxFrame( &txFrm[i], -1, 0 ) == 0 );

		/* wait until received */
		CHK( WaitRx(10*1000*1000) == 0 );

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );

		/* check if received correctly */
		if( memcmp( &rxFrm, &txFrm[i], sizeof(rxFrm )) != 0 ){
			DumpFrame( "Sent", &txFrm[i] );
			DumpFrame( "Recv", &rxFrm );
			CHKEX(0,("Incorrect Frame received"));
		}
	}

	return 0;

 abort:
	return -1;

}

static int LoopbRxFifo(void)
{
	int i;
	/* frames to send */
	MSCAN_FRAME txFrm[] = {
		/* ID,  flags,          dlen, data */
		{ 0x12, 0,				1,   { 0xa5 } },
		{ 0x45, 0,				8,   { 0x01, 0x02, 0x03, 0x04, 0x05,
									   0x06, 0x07, 0x08 } },
		{ 0x13218765,  MSCAN_EXTENDED, 2, { 0x99, 0xcc } },
	};
	MSCAN_FRAME rxFrm;

	/* send frames */
	for( i=0; i<sizeof(txFrm)/sizeof(MSCAN_FRAME); i++ ){

		CHK( TxFrame( &txFrm[i], -1, 0 ) == 0 );
		/*Delay(100); */				/* ??? */
	}


	for( i=0; i<sizeof(txFrm)/sizeof(MSCAN_FRAME); i++ ){
		/* wait until received */
		CHK( WaitRx(10*1000*1000) == 0 );

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );

		/* check if received correctly */
		if( memcmp( &rxFrm, &txFrm[i], sizeof(rxFrm )) != 0 ){
			DumpFrame( "Sent", &txFrm[i] );
			DumpFrame( "Recv", &rxFrm );
			CHKEX(0,("Incorrect Frame received"));
		}
	}

	return 0;

 abort:
	return -1;

}

static int LoopbTxPrio(void)
{
	int i;
	/* frames to send */
	MSCAN_FRAME txFrm[] = {
		/* ID,  flags,          dlen, data */
		{ 0x12, 0,				1,   { 0xa5 } },
		{ 0x45, 0,				8,   { 0x01, 0x02, 0x03, 0x04, 0x05,
									   0x06, 0x07, 0x08 } },
		{ 0x48, 0,				8,   { 0x01, 0x02, 0x03, 0x04, 0x05,
									   0x06, 0x07, 0x08 } }
	};
	MSCAN_FRAME rxFrm;

	/* prepare to send frames, don't start tx yet */
	for( i=0; i<3; i++ ){

		CHK( TxFrame( &txFrm[i], i, 3-i ) == 0 );

	}
	/* start all frames now, last frame has hightest prio */
	MWRITE_D8( G_ma, MSCAN_TFLG, 0x7 );


	for( i=0; i<3; i++ ){
		printf("%d\n",i);
		/* wait until received */
		CHK( WaitRx(10*1000*1000) == 0 );

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );

		/* check if received correctly */
		if( memcmp( &rxFrm, &txFrm[2-i], sizeof(rxFrm )) != 0 ){
			DumpFrame( "Sent", &txFrm[2-i] );
			DumpFrame( "Recv", &rxFrm );
			CHKEX(0,("Incorrect Frame received"));
		}

	}

	return 0;

 abort:
	return -1;

}

static int FltCheck( const MSCAN_FRAME *txFrm )
{
	int idacSb=-1, idacIs;
	MACCESS ma = G_ma;
	MSCAN_FRAME rxFrm;

	if( SwFilter( txFrm, &G_flt[0] ))
		idacSb = 0;
	else if( SwFilter( txFrm, &G_flt[1] ))
		idacSb = 1;

	/* send the frame */
	CHK( TxFrame( txFrm, -1, 0 ) == 0 );

	/* wait for receiption */
	if( WaitRx( 10*1000 ) == 0 ){
		printf("    RX! id=%lx\n", txFrm->id);
		idacIs = MREAD_D8( ma, MSCAN_IDAC ) & 0x7;

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );
	}
	else
		idacIs = -1;

	CHKEX( idacSb == idacIs,
		   ("IDAC is %d sb %d", idacIs, idacSb ));

	return 0;
 abort:
	DumpFrame( "Sent:", txFrm );
	return -1;
}

static int LoopbFilter(void)
{
	/* filter setups */
	const MSCAN_FILTER stdFilter = {
		0xe0,
		0x53,
		0,
		MSCAN_RTR
	};
	const MSCAN_FILTER extFilter = {
		0x000007ff,
		0x10000000,
		MSCAN_EXTENDED,
		0
	};
	const MSCAN_FILTER stdFilter2 = {
		0xe0,
		0x01,
		MSCAN_RTR,
		MSCAN_RTR
	};


	/* frames to send */
	MSCAN_FRAME txFrm;


	CHK( EnterInitMode() == 0 );

	/*
	 * Filter config:
	 * Set filter to ignore, let all messages pass through
	 */
	SetFilter( 0, &stdFilter );
	SetFilter( 1, &extFilter );

	CHK( LeaveInitMode() == 0 );

	/*
	 * should receive a0, a1, a2, a3, b0, b1, b2, b3,
	 * e0, e1, e2, e3, f0, f1, f2, f3 here
	 */
	txFrm.dataLen = 0;
	txFrm.flags = 0;
	for(txFrm.id=0; txFrm.id<0x800; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );

	/* should receive nothing, as RTR is set in frame */
	txFrm.flags = MSCAN_RTR;
	for(txFrm.id=0; txFrm.id<0x800; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );

	/* should receive 0x7ff */
	txFrm.flags = MSCAN_RTR | MSCAN_EXTENDED;
	for(txFrm.id=0; txFrm.id<0x800; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );

	/* should receive 0x100007ff */
	for(txFrm.id=0x10000000; txFrm.id<0x10002000; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );


	/* Reconfigure STD filter to receive RTR frames only */
	CHK( EnterInitMode() == 0 );
	SetFilter( 0, &stdFilter2 );
	CHK( LeaveInitMode() == 0 );

	txFrm.flags = 0;
	for(txFrm.id=0; txFrm.id<0x800; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );

	txFrm.flags = MSCAN_RTR;
	for(txFrm.id=0; txFrm.id<0x800; txFrm.id++)
		CHK( FltCheck( &txFrm ) == 0 );


	return 0;

 abort:
	DumpFrame( "Sent:", &txFrm );
	return -1;

}

/**********************************************************************/
/** Internal loopback tests
 *
 * \param bitrate in Hz
 *
 * \return 0=ok, -1=error
 */
#ifdef MSCAN_VERI_PP01
int32 pp01_mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KM01
int32 km01_mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KX03
int32 kx03_mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KD12
int32 kd12_mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_EM04
int32 mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_EM05
int32 mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_SC15B
int32 sc15b_mscan_loopb( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_SC15C
int32 sc15c_mscan_loopb( int bitrate, int testNo, int can )
#endif
{
	if( bitrate == 0 )
		bitrate = 250000;



	Prepare(can);

	OutTest( "Internal Loopback Test Bitrate=%d", bitrate );
	OutTest( "  Testing CAN_%d at 0x%x",can+1, G_ma );
	CHK( LoopBSetup( bitrate ) == 0 );

	if( testNo==0 || testNo==1 ){
		OutSub("Basic Tx/Rx (single frame, open filter)");
		CHK( LoopbBasic() == 0 );
	}

	if( testNo==0 || testNo==2 ){
		OutSub("Rx fifo handling (send 3 frames in advance)");
		CHK( LoopbRxFifo() == 0 );
	}

	if( testNo==0 || testNo==3 ){
		OutSub("Tx buffer priority handling (send 3 frames simultanously)");
		CHK( LoopbTxPrio() == 0 );
	}

	if( testNo==0 || testNo==4 ){
		OutSub("Rx filtering (2x32 bit filters)");
		CHK( LoopbFilter() == 0 );
	}

	return 0;

 abort:
	return -1;
}

static int AlyzerSetup( int bitrate )
{
	MACCESS ma = G_ma;
	const MSCAN_FILTER stdFilter = {
		0,
		0xffffffff,
		0,
		0
	};
	const MSCAN_FILTER extFilter = {
		0,
		0xffffffff,
		MSCAN_EXTENDED,
		0
	};


	CHK( EnterInitMode() == 0 );
	CHK( SetSpeed( bitrate ) == 0 );
	MCLRMASK_D8( ma, MSCAN_CTL1, MSCAN_CTL1_LOOPB );

	/*
	 * Filter config:
	 * Set filter to ignore, let all messages pass through
	 */
	SetFilter( 0, &stdFilter );
	SetFilter( 1, &extFilter );

	CHK( LeaveInitMode() == 0 );

	return 0;

 abort:
	return -1;
}

static int CmpFrames( const MSCAN_FRAME *frm1, const MSCAN_FRAME *frm2 )
{
	int i;

	if( frm1->id != frm2->id )
		return -1;

	if( frm1->flags != frm2->flags )
		return -1;

	if( !(frm1->flags & MSCAN_RTR)){
		/* Note: CANalyzer sends remote frames always with DLC=0 */
		if( frm1->dataLen != frm2->dataLen )
			return -1;

		for( i=0; i<frm1->dataLen; i++ )
			if( frm1->data[i] != frm2->data[i] )
				return -1;
	}

	return 0;
}

/**********************************************************************/
/** Build next ID/state for frames send by MSCAN
 * state:
 * 0: std, no rtr	ID 0x090..0x3ff
 * 1: std, rtr		ID 0x090..0x3ff
 * 2: ext, no rtr	ID 0x10000..0x100ff
 * 3: ext, rtr		ID 0x1000000..0x100000ff
 */
static void NxtTxId( int *stateP, u_int32 *idP )
{
	u_int32 id = *idP;
	int state = *stateP;

	id++;
	switch( state ){
	case -1:
		state = 0;
		id = 0x300;
		break;
	case 0:
		if( id==0x400 ){
			state++;
			id=0x300;
		}
		break;
	case 1:
		if( id==0x400 ){
			state++;
			id=0x10000;
		}
		break;
	case 2:
		if( id==0x10100 ){
			state++;
			id=0x10000000;
		}
		break;
	case 3:
		if( id==0x10000100 ){
			state++;
		}
		break;
	}
	*stateP = state;
	*idP = id;
}

/**********************************************************************/
/** Build next ID/state for frames sent by CANalyzer
 * state:
 * 0: std, no rtr	ID 0x400..0x7ff
 * 1: std, rtr		ID 0x400..0x7ff
 * 2: ext, no rtr	ID 0x10100..0x101ff
 * 3: ext, rtr		ID 0x1000100..0x100001ff
 */
static void NxtRxId( int *stateP, u_int32 *idP )
{
	u_int32 id = *idP;
	int state = *stateP;

	id++;
	switch( state ){
	case -1:
		state = 0;
		id = 0x400;
		break;
	case 0:
		if( id==0x7ff ){
			state++;
			id=0x400;
		}
		break;
	case 1:
		if( id==0x7ff ){
			state++;
			id=0x10100;
		}
		break;
	case 2:
		if( id==0x10200 ){
			state++;
			id=0x10000100;
		}
		break;
	case 3:
		if( id==0x10000200 ){
			state++;
		}
		break;
	}
	*stateP = state;
	*idP = id;
}

/**********************************************************************/
/** Query teststatus from CANalyzer CAPL script
 *
 * \return 0=ok, 0xff=test still running, >0 error code
 */
static int AlyzerStatus( void )
{
	MSCAN_FRAME frm;

	frm.id = 0x7ff;
	frm.flags = MSCAN_EXTENDED|MSCAN_RTR;
	frm.dataLen = 3;

	CHK( WaitTxIdle( 50*1000*1000 ) == 0 );
	CHK( TxFrame(&frm, -1, 0) == 0 );

	CHK( WaitRx(10*1000*1000) == 0 );

	/* get frame from fifo */
	CHK( RxFrame( &frm ) == 0 );


	CHK( frm.id == 0x7fe );
	CHK( frm.dataLen == 3 );

	return frm.data[1];

 abort:
	return -1;
}

static int AlyzerCmd( u_int8 cmd, u_int8 p1, u_int8 p2 )
{
	MSCAN_FRAME txFrm;

	txFrm.id = 0x7ff;
	txFrm.flags = MSCAN_EXTENDED;
	txFrm.dataLen = 3;

	txFrm.data[0] = cmd;
	txFrm.data[1] = p1;
	txFrm.data[2] = p2;

	CHK( WaitTxIdle( 50*1000*1000 ) == 0 );
	CHK( TxFrame(&txFrm, -1, 0) == 0 );
	CHK( WaitTxIdle( 50*1000*1000 ) == 0 );

	return 0;

 abort:
	return -1;
}

static void AlyzerMkFrame( MSCAN_FRAME *frm, int state, int id )
{
	int i;
	u_int8 d;

	/*
	 * states:
	 * 0: std, no rtr	ID 0..0x7ff
	 * 1: std, rtr		ID 0..0x7ff
	 * 2: ext, no rtr	ID 0x10000..0x100ff
	 * 3: ext, rtr		ID 0x1000000..0x100000ff
	 *
	 * DLC: 	id % 9 (0 for RTR frames)
	 * data[0]:	id&0xff
	 * data[1]:	(id&0xff)+1
	 * ...
	 */
	frm->id = id;
	frm->flags = ((state & 0x1) ? MSCAN_RTR : 0) |
		((state & 0x2) ? MSCAN_EXTENDED : 0 );
	frm->dataLen = id % 9;

#if 0 /*???*/
	if( frm->flags & MSCAN_RTR )
		frm->dataLen = 0;
#endif
	d = id & 0xff;

	for( i=0; i<frm->dataLen; i++ )
		frm->data[i] = d++;
	for( ; i<8; i++ )
		frm->data[i] = 0;

}



static int AlyzerTx(void)
{
	u_int32 id;
	int state=-1;
	MSCAN_FRAME txFrm;

	CHK( AlyzerCmd(AcTx, 0, 0) == 0 );

	NxtTxId( &state, &id );

	/* state 0 */
	while( state < 4 ){

		AlyzerMkFrame( &txFrm, state, id );

		/*DumpFrame( "Tx  ", &txFrm );*/
		CHK( WaitTx( 50*1000*1000 ) == 0);
#ifdef USE_ONLY_ONE_BUFFER
		CHK( TxFrame( &txFrm, -1, 0 ) == 0 );
#else
		CHK( TxFrame( &txFrm, -1, -1 ) == 0 );
#endif /* USE_ONLY_ONE_BUFFER */
	
		NxtTxId( &state, &id );
	}


	CHK( AlyzerStatus() == 0 );
	CHK( AlyzerCmd(AcStop, 0, 0) == 0 );

	return 0;

 abort:
	AlyzerCmd(AcStop, 0, 0);
	return -1;

}

static int AlyzerTxSlow(void)
{
	int id, i;
	MSCAN_FRAME txFrm;

	txFrm.id = 0x555;
	txFrm.flags = 0;
	txFrm.dataLen = 0;

	for( i=0; i<8; i++ )
		txFrm.data[i] = 0x5e + i;

	for( id=0; id<=0x0; id++ ){
#if 0
		AlyzerMkFrame( &txFrm, 0, id );
		txFrm.dataLen = 8;
#endif
		CHK( WaitTx( 50*1000*1000 ) == 0);
		
#ifdef USE_ONLY_ONE_BUFFER
		CHK( TxFrame( &txFrm, -1, 0 ) == 0 );
#else
		CHK( TxFrame( &txFrm, -1, -1 ) == 0 );
#endif /* USE_ONLY_ONE_BUFFER */
		/*Delay(100);*/
	}
	return 0;

 abort:
	AlyzerCmd(AcStop, 0, 0);
	return -1;
}

static int AlyzerRx(void)
{
	u_int32 id;
	int state=-1;
	MSCAN_FRAME rxFrm, sbFrm;

	CHK( AlyzerCmd(AcRx, 0, 0) == 0 );

	NxtRxId( &state, &id );

	while(state<4){
		int newState = state;

		AlyzerMkFrame( &sbFrm, state, id );

		/* wait until received */
		CHK( WaitRx(10*1000*1000) == 0 );

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );


		/* check if received correctly */
		if( CmpFrames( &rxFrm, &sbFrm ) != 0 ){
			DumpFrame( "Sb  ", &sbFrm );
			DumpFrame( "Recv", &rxFrm );
			CHKEX(0,("Incorrect Frame received"));
		}

		NxtRxId( &newState, &id );

		if( newState != state ){
			state = newState;
			printf("   Now in state %d\n", state );
		}
	}

	CHK( AlyzerCmd(AcStop, 0, 0) == 0 );

	return 0;

 abort:
	AlyzerCmd(AcStop, 0, 0);
	return -1;
}

static int AlyzerRxOver(void)
{
	int id;
	MSCAN_FRAME rxFrm;
	MACCESS ma = G_ma;

	/* CANalyzer sends 5 frames, first 4 must be received */
	CHK( AlyzerCmd(AcRxOver, RX_BUF_NO, 0) == 0 );

	Delay(2000);

	/* check if overrun occurred */
	CHK( (MREAD_D8( ma, MSCAN_RFLG) & MSCAN_RFLG_OVRIF) != 0 );
	/* clear overrun flag */
	MWRITE_D8( ma, MSCAN_RFLG, MSCAN_RFLG_OVRIF );
	CHK( (MREAD_D8( ma, MSCAN_RFLG) & MSCAN_RFLG_OVRIF) == 0 );

	for( id=0; id<RX_BUF_NO; id++ ){
		CHK( WaitRx(1000*1000) == 0 ); /* takes a little to get buffer... */
		CHK( RxFrame( &rxFrm ) == 0 );
		CHK( rxFrm.id == id );
	}


	/* CANalyzer sends another 4 frames */
	CHK( AlyzerCmd(AcRxOver2, RX_BUF_NO, 0) == 0 );

	for( id=0x10; id<(0x10+RX_BUF_NO); id++ ){
		/* wait until received */
		CHK( WaitRx(10*1000*1000) == 0 );

		/* get frame from fifo */
		CHK( RxFrame( &rxFrm ) == 0 );
		CHK( rxFrm.id == id );

	}
	CHK( AlyzerCmd(AcStop, 0, 0) == 0 );

	return 0;

 abort:
	AlyzerCmd(AcStop, 0, 0);
	return -1;
}

static int AlyzerRxTx(void)
{
	int rv = -1, i;
	u_int32 txId, rxId;
	int txState=-1, rxState=-1;
	MSCAN_FRAME rxFrm, sbFrm, txFrm;

	CHK( AlyzerCmd( AcRxTx, 0, 0) == 0 );

	NxtRxId( &rxState, &rxId );
	NxtTxId( &txState, &txId );

	while( (txState < 4) || (rxState < 4) ){

		while( (rxState < 4) && (WaitRx(4*1000) == 0) ){
			int newRxState = rxState;

			CHK( RxFrame( &rxFrm ) == 0 );
			/*printf("<"); fflush(stdout);*/
			/* received something */

			AlyzerMkFrame( &sbFrm, rxState, rxId );

			if( CmpFrames( &rxFrm, &sbFrm ) != 0 ){
				DumpFrame( "Sb  ", &sbFrm );
				DumpFrame( "Recv", &rxFrm );
				CHK(0);
			}

			NxtRxId( &newRxState, &rxId );

			if( newRxState != rxState ){
				rxState = newRxState;
				printf("   Rx now in state %d\n", rxState );
			}

		}


		/* send burst */
		for( i=0; i<1 && txState<4; i++ ){
			int newTxState = txState;

			AlyzerMkFrame( &txFrm, txState, txId );
			if( WaitTx( 1 ) == 0){
				/*printf(">"); fflush(stdout);*/

#ifdef USE_ONLY_ONE_BUFFER
		        CHK( TxFrame( &txFrm, -1, 0 ) == 0 );
#else
		        CHK( TxFrame( &txFrm, -1, -1 ) == 0 );
#endif /* USE_ONLY_ONE_BUFFER */

				NxtTxId( &newTxState, &txId );

				if( newTxState != txState ){
					txState = newTxState;
					printf("   Tx now in state %d\n", txState );
				}
			}
		}

	}

	CHK( AlyzerStatus() == 0 );

    rv = 0;

 abort:
	AlyzerCmd( AcStop, 0, 0);

	return rv;
}

/**********************************************************************/
/** Canalyzer tests
 *
 * \param bitrate in Hz
 * \param testNo  0=all 1=Tx 2=Rx 3=RxOver
 * \return 0=ok, -1=error
 */
#ifdef MSCAN_VERI_PP01
int32 pp01_mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KM01
int32 km01_mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KX03
int32 kx03_mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_KD12
int32 kd12_mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_EM04
int32 mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_EM05
int32 mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_SC15B
int32 sc15b_mscan_alyzer( int bitrate, int testNo, int can )
#endif
#ifdef MSCAN_VERI_SC15C
int32 sc15c_mscan_alyzer( int bitrate, int testNo, int can )
#endif
{
	if( bitrate == 0 )
		bitrate = 250000;

	Prepare(can);

	OutTest( "CANalyzer Test Bitrate=%d", bitrate );
	OutTest( "  Testing CAN_%d at 0x%x",can+1,G_ma );
	CHK( AlyzerSetup( bitrate ) == 0 );

	if( testNo==0 || testNo==1 ){
		OutSub("Tx");
		CHK( AlyzerTx() == 0 );
	}

	if( testNo==0 || testNo==2 ){
		OutSub("Rx");
		CHK( AlyzerRx() == 0 );
	}

	if( testNo==0 || testNo==3 ){
		OutSub("Rx Overrun handling");
		CHK( AlyzerRxOver() == 0 );
	}

	if( testNo==0 || testNo==4 ){
		OutSub("Rx/Tx");
		CHK( AlyzerRxTx() == 0 );
	}

	if( testNo==5 ){
		OutSub("Slow Simple Tx");
		CHK( AlyzerTxSlow() == 0 );
	}

	return 0;

 abort:
	return -1;
}


