/****************************************************************************
 *
 *      $Author: ts $
 *        $Date: 2014/05/23 18:18:57 $
 *    $Revision: 1.1 $
 *
 *  Description: Memory test for M-Modules with pointer-indexed accesses
 *               (68331 based like M57, M65)
 *               Derived from m57_ramtest
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mmod_ramtest.c,v $
 * Revision 1.1  2014/05/23 18:18:57  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1992-2014 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static char _RCSid[]="$Header: /dd2/CVSR/COM/TOOLS/MMOD_RAMTEST/COM/mmod_ramtest.c,v 1.1 2014/05/23 18:18:57 ts Exp $";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pciConfigLib.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>

#define MODE_A08
#define INTEL

u_int32 mmod_base = 0xd3500000;
u_int32 G_usDelay=0;

int opt_errpause=0;

#ifdef INTEL
# define CNTR_REG	0xff
# define ID_REG		0xfe
#else
# define CNTR_REG	0xfe
# define ID_REG		0xff
#endif

/* mode flags */
#define MD_NORMAL			0x00	/* normal ram test */
#define MD_FAST				0x01	/* fast ramtest (destructive, fixed pattern */
									/* long accesses */
/* additional flags for normal mode */
#define MD_WORD				0x04	/* word accesses */
#define MD_ONEZERO			0x08	/* long accesses */

#define MD_DESTRUCTIVE		0x08	/* destructive test */
#define MD_RANDPAT			0x10	/* use random pattern */

#define MD_AUTOINCREMENT	0x40

#define PAT1	0x5A5A5A5A			/* fast test pattern #1 */
#define PAT2	0xA5A5A5A5			/* fast test pattern #2 */

#define BLKSIZ		128L			/* fast test block size */
#define INFOSTEP	4096L			/* number of bytes to test until printing*/

#define M57_RESET( base_a08, on ) (*(volatile u_int8 *)(base_a08+ID_REG) = on ? 0x4 : 0x0 )
#define M65_RESET

/* M57 Shared RAM access address */
#define M57_SHRAM_ACCESS_ADDR_MSB   0xA0
#define M57_SHRAM_ACCESS_ADDR_LSB   0xA2
#define M57_SHRAM_DATA_ACCESS	    0x80

/* M65 Chan. 1 shared RAM access address */
#define M65_CH1_SHRAM_ACCESS_ADDR_MSB   0x78
#define M65_CH1_SHRAM_ACCESS_ADDR_LSB   0x7A
#define M65_CH1_SHRAM_DATA_ACCESS	    0x76

/* M65 Chan. 2 shared RAM access address */
#define M65_CH2_SHRAM_ACCESS_ADDR_MSB   0xF8
#define M65_CH2_SHRAM_ACCESS_ADDR_LSB   0xFA
#define M65_CH2_SHRAM_DATA_ACCESS	    0xF6

#define DP_SET_WINDOW( base_a08, window_start )\
 (*(volatile  u_int16 *)((u_int32)(base_a08)+G_accessAddrMsb)=(window_start>>16)), \
 (*(volatile  u_int16 *)((u_int32)(base_a08)+G_accessAddrLsb) = window_start)

#define DP_DATA( base_a08, type )  (*(volatile  type *)((u_int32)(base_a08)+G_accessData))

/* generic functions for indexed (via pointer access) read/write to Mxx Registers*/
#define DP_READ_INT16(base_a08,offs) \
 (DP_SET_WINDOW(base_a08,offs),DP_DATA( base_a08, u_int16 ))

#define DP_WRITE_INT16(base_a08,offs,data)\
 { DP_SET_WINDOW(base_a08,offs); OSS_MikroDelay(NULL, G_usDelay); DP_DATA(base_a08,u_int16)=data; }


LOCAL u_int32 G_accessAddrMsb=0;
LOCAL u_int32 G_accessAddrLsb=0;
LOCAL u_int32 G_accessData=0;


/*--- forward declarations ---*/
static u_int32 wordtest(), onestest();

/*----------------------------------------------------------+
| ramtest - main function                                   |
| If you need multiple passes, call it several times.       |
| returns: number of errors occurred or -1 if invalid mode  |
+----------------------------------------------------------*/
u_int32 ramtest( startadr, endadr, mode, infofunc, errorfunc )
u_int32 	startadr, endadr;		/* memory region to test */
u_int32		mode;					/* mode flags (ramtest.h) */
int			(*infofunc)();			/* function for info printouts */
int			(*errorfunc)();			/* function for error printouts */
{

	switch( mode ){

		case MD_WORD:
			return wordtest( startadr, endadr, mode, infofunc, errorfunc );

		case MD_ONEZERO:
			return onestest( startadr, endadr, mode, infofunc, errorfunc );

		default:
			return -1;
		}

}


/*---------------------------+
| macros to speed up program |
+---------------------------*/
#define RANDOM(n)\
{\
     volatile u_int32 a = n;\
\
    a <<= 11;\
    a += n;\
    a <<= 2;\
    n += a;\
    n += 13849;\
}

#define DESTRUCTIVETEST(access,WR_ACCESS,RD_ACCESS)\
 {\
	 volatile u_int32 randval=0xABCDEF02;\
	 volatile access rval;\
	 volatile u_int32 p;\
	 volatile u_int32 count=0, curlen;\
	u_int32 len = (endadr-startadr)/sizeof(access);\
	\
	p = (u_int32)startadr;\
\
	while( len > 0){\
		curlen = len;\
		if( curlen > INFOSTEP ) curlen = INFOSTEP;		\
		count += curlen;\
		len -= curlen;\
		while(curlen--){\
			RANDOM(randval);	/* create a new random value */\
			WR_ACCESS(base,p,(access)randval);	/* store value */\
			OSS_MikroDelay(NULL, G_usDelay);\
			p+=sizeof(access);\
		}\
		(*infofunc)(count*sizeof(access),errorcount);				\
	}\
	OSS_MikroDelay(NULL, G_usDelay);\
	p = (u_int32)startadr;\
	len = (endadr-startadr)/sizeof(access);\
	randval=0xABCDEF02;\
	count=0;\
	\
	while( len > 0){\
		curlen = len;\
		if( curlen > INFOSTEP ) curlen = INFOSTEP;		\
		count += curlen;\
		len -= curlen;\
		while(curlen--){\
			RANDOM(randval);\
			rval = RD_ACCESS(base,p);\
			OSS_MikroDelay(NULL, G_usDelay);\
			if( rval != (access)randval ){\
				printf("1. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
				rval = RD_ACCESS(base,p);\
				OSS_MikroDelay(NULL, G_usDelay);\
				if( rval != (access)randval ){\
					printf("2. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
					OSS_MikroDelay(NULL, G_usDelay);\
					rval = RD_ACCESS(base,p);\
					if( rval != (access)randval ){\
						printf("3. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
						OSS_MikroDelay(NULL, G_usDelay);\
					} \
				} \
				errorcount++;\
				if( (*errorfunc)( p, rval, (access)randval )){\
					return errorcount;		/* exit if abort */\
				}\
			}\
			p += sizeof(access);\
		}\
		(*infofunc)(count*sizeof(access),errorcount);				\
	}\
}

#define ONEZEROTEST(access,WR_ACCESS,RD_ACCESS)\
 {\
	 volatile u_int32 randval=0xffffffff;\
	 volatile access rval;\
	 volatile u_int32 p;\
	 volatile u_int32 count=0, curlen;\
	u_int32 len = (endadr-startadr)/sizeof(access);\
	\
	p = (u_int32)startadr;\
\
	while( len > 0){\
		curlen = len;\
		if( curlen > INFOSTEP ) curlen = INFOSTEP;		\
		count += curlen;\
		len -= curlen;\
		while(curlen--){\
			OSS_MikroDelay(NULL, G_usDelay);\
			randval=~randval;	/* invert all bits */\
			WR_ACCESS(base,p,(access)randval);	/* store value */\
			p+=sizeof(access);\
		}\
		(*infofunc)(count*sizeof(access),errorcount);				\
	}\
	p = (u_int32)startadr;\
	len = (endadr-startadr)/sizeof(access);\
	randval=0xffffffff;\
	count=0;\
	\
	while( len > 0){\
		curlen = len;\
		if( curlen > INFOSTEP ) curlen = INFOSTEP;		\
		count += curlen;\
		len -= curlen;\
		while(curlen--){\
			OSS_MikroDelay(NULL, G_usDelay);\
			randval=~randval;\
			rval = RD_ACCESS(base,p);\
			if( rval != (access)randval ){\
				printf("1. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
				rval = RD_ACCESS(base,p);\
				OSS_MikroDelay(NULL, G_usDelay);\
				if( rval != (access)randval ){\
					printf("2. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
					OSS_MikroDelay(NULL, G_usDelay);\
					rval = RD_ACCESS(base,p);\
					if( rval != (access)randval ){\
						printf("3. read error: IS 0x%04x SB 0x%04x   reread:\n",rval,(access)randval);\
						OSS_MikroDelay(NULL, G_usDelay);\
					} \
				} \
				errorcount++;\
				if( (*errorfunc)( p, rval, (access)randval )){\
					return errorcount;		/* exit if abort */\
				}\
			}\
			p += sizeof(access);\
		}\
		(*infofunc)(count*sizeof(access),errorcount);				\
	}\
}


void m57resettest(void)
{
	volatile u_int32 base = mmod_base + 0x600;
	int i;
	for (i=0; i < 10000; i++) {
		OSS_MikroDelay(NULL, 50);
		M57_RESET( base, 1 ); /* set 68331 CPU into reset */
		OSS_MikroDelay(NULL, 50);
		M57_RESET( base, 0 ); /* set 68331 CPU into reset */
	}
}



/*------------------------------------------------------------------+
| byte, word and long mem tests - see description in file header    |
+------------------------------------------------------------------*/

static u_int32 wordtest( startadr, endadr, mode, infofunc, errorfunc )
u_int32 startadr, endadr;
u_int32 mode;
int			(*infofunc)();			/* function for info printouts */
int			(*errorfunc)();			/* function for error printouts */
{
    volatile u_int32 base = mmod_base;
	u_int32 errorcount = 0;
	M57_RESET( base, 1 ); /* set 68331 CPU into reset */

	DESTRUCTIVETEST(u_int16,DP_WRITE_INT16,DP_READ_INT16);
	return errorcount;
}

static u_int32 onestest( startadr, endadr, mode, infofunc, errorfunc )
u_int32 startadr, endadr;
u_int32 mode;
int			(*infofunc)();			/* function for info printouts */
int			(*errorfunc)();			/* function for error printouts */
{
	volatile u_int32 base = mmod_base;
	u_int32 errorcount = 0;
	M57_RESET( base, 1 ); /* set 68331 CPU into reset */
	ONEZEROTEST(u_int16,DP_WRITE_INT16,DP_READ_INT16);
	return errorcount;
}
/*****************/

int pausemode=0;

int info( u_int32 count )
{
	printf( "%06ld \n", count/1024L );
	fflush(stdout);
	return 0;
}


int error( adr, is, shouldbe )
{
	char *fmt = "ERROR at $%08lx value is $%08lx, should be $%08lx\n";
	char c=0;
	printf( fmt, adr, is, shouldbe);
	fflush(stdout);

	if( opt_errpause ){

		printf("\npress a: abort c: cont\n");
		fflush(stdout);
		do {
			read(0,&c,1);
		} while( c!='a' && c!='c');
	}

	fflush(stdout);

	return c == 'a' ? -1 : 0;
}

void aborttest()
{
	printf("\naborted!\n");
	exit(0);
}


void usage( void )
{
	printf( "68331 based M-Module RAM test for pointer indexed access\n");
	printf( "Tool to run memory tests on the DRAM of e.g. M57 or M65 mounted on a F204/5 or D203, using A08 space.\n");
	printf( "Usage: mmod_ramtest [-s=<startadr>] [-e=<endadr>] [<opts>] \n\n");
	printf( "Options: \n");
	printf( "         -w                        word accesses (default)\n");
	printf( "         -i                        use autoincrement mode (only with -w)\n");
	printf( "         -a                        use alternating one/zero pattern\n");
	printf( "         \n");
	printf( "         -n=<num>                  number of runs\n");
	printf( "         -p                        exit (a) or pause (c) if error (hit a or c)\n");
	printf( "         -o=<M-Module slot>        [F204: 0, F205: 0-1, D203: 0-3] default: 1\n");
	printf( "         -d=<usec>                 delay between M-Module interface accesses     [50]\n");
	printf( "         -m=<0xnum>                M57 (A08-)base address.\n");
	printf( "                                   if omitted, base address (BAR0+M-Mod.slot 0)is autodetected.\n");
	printf( "         -s=<0xstartaddr>          start address relative to M57 base address      [0x0]\n");
	printf( "         -e=<0xendaddr>            end address relative to M57 base address    [0x40000]\n");
	printf( "         -t=<MMod-Type>            M-Module to test: 0=M57, 1=M65ch1, 2=M65ch2 [0]\n");
	printf( "                                   This selects pointer index address/data registers\n");
	printf( "         -v                        verbose output of passed options\n");

}

/************
 * The one and only main
 */
int main(int argc, char **argv)
{

	u_int32 start, end, mode = MD_WORD;
	int nruns = 1;
	int pciBus=0, pciDev=0,pciFunc=0;
	unsigned int bar0addr;
	char *tstmode="normal", *access="long", *dest="", *pat="random";
	int errorcount=0, pass=0;
	int opt_mmodslot=0;
	int opt_mmodtype=0;
	char *fmt, *optp;

	printf("mmod_ramtest built %s %s\n", __DATE__, __TIME__);

	/*
	* check passed options
	*/
	if(UTL_TSTOPT("?")) {
		usage();
		return 0;
	}

	opt_errpause = UTL_TSTOPT("p") ? 1 : 0;

	if( (optp=UTL_TSTOPT("s=")))
		sscanf( optp, "%lx", &start );
	else
		start = 0x0;

	if( (optp=UTL_TSTOPT("e=")))
		sscanf( optp, "%lx", &end );
	else
		end = 0x1000;

	if( (optp = UTL_TSTOPT("m="))) {
		sscanf(optp, "%lx", &mmod_base );
		printf("option -m passed, using Module base address 0x%lx\n", mmod_base);
		} else {
		printf("option -m omitted. Searching for F20x/D203 on PCIbus...\n");
		if ( pciFindDevice( 0x1172, 0xd203, 0, &pciBus, &pciDev, &pciFunc) == OK ) {
			printf("-> Found at PCI 0x%x/0x%x/0x%x ", pciBus, pciDev, pciFunc);
			if (pciConfigInLong(pciBus, pciDev, pciFunc, 0x10 /* BAR0 */, &bar0addr) == OK)
				printf(" BAR0 = 0x%x\n", bar0addr);
			mmod_base = bar0addr;
		}
	}

	if( UTL_TSTOPT("f")){
		tstmode = "fast";
		dest = "";
		pat = "fixed";
		mode = MD_FAST;
	}
	else {

		if( UTL_TSTOPT("a")) {
			access = "alternating";
			pat = "one-zero";
			mode = MD_ONEZERO;
		}

		if( UTL_TSTOPT("w")){
			access = "word";
			mode = MD_WORD;

		}
	}

	nruns        = ( optp=UTL_TSTOPT("n=")) ? atoi(optp) : 1;
	opt_mmodslot = ( optp=UTL_TSTOPT("o=")) ? atoi(optp) : 1;
	G_usDelay	 = ( optp=UTL_TSTOPT("d=")) ? atoi(optp) : 50;
	opt_mmodtype = ( optp=UTL_TSTOPT("t=")) ? atoi(optp) : 0;

	/* assertions */
	if ((opt_mmodtype < 0 ) || (opt_mmodtype > 2 )) {
		fprintf(stderr, "*** invalid M Module type %d! must be 0,1 or 2\n", opt_mmodtype);
		return -1;
	}
	if ((opt_mmodslot < 0 ) || (opt_mmodslot > 3 )) {
		fprintf(stderr, "*** invalid M Module Slot %d! must be 0..3\n", opt_mmodslot );
		return -1;
	}

	/* set data pointer / data registers according to M Module type */
	switch (opt_mmodtype) {
	case 0:   /* M57 */
		G_accessAddrMsb		=	M57_SHRAM_ACCESS_ADDR_MSB;
		G_accessAddrLsb		=	M57_SHRAM_ACCESS_ADDR_LSB;
		G_accessData		=	M57_SHRAM_DATA_ACCESS;
		break;

	case 1:   /* M65 ch 1 */
		G_accessAddrMsb		=	M65_CH1_SHRAM_ACCESS_ADDR_MSB;
		G_accessAddrLsb		=	M65_CH1_SHRAM_ACCESS_ADDR_LSB;
		G_accessData		=	M65_CH1_SHRAM_DATA_ACCESS;
		break;

	case 2:   /* M65 ch 2 */
		G_accessAddrMsb		=	M65_CH2_SHRAM_ACCESS_ADDR_MSB;
		G_accessAddrLsb		=	M65_CH2_SHRAM_ACCESS_ADDR_LSB;
		G_accessData		=	M65_CH2_SHRAM_DATA_ACCESS;
		break;

	default:
		fprintf(stderr, "*** internal error: invalid Mmod type!\n");
		return -1;
	}


	mmod_base+=(0x200+opt_mmodslot*0x400);

	if( UTL_TSTOPT("p"))
		pausemode = 1;

	if( (UTL_TSTOPT("v"))) {
		printf("debugging:\n");
		printf("start       		= 0x%lx\n", start );
		printf("end         		= 0x%lx\n", end );
		printf("mmod_base    		= 0x%lx\n", mmod_base);
		printf("nruns       		= %d\n", 	nruns );
		printf("MMOD slot      		= %d\n", 	opt_mmodslot );
		printf("halt/exit on errors = %s\n", 	opt_errpause ? "yes" : "no" );
		printf("mem delay   = %d us\n", (int)G_usDelay );
		printf("mode mask   = %08x\n",  (unsigned int)mode);
	}

	printf( "Testing memory from $%08lx to $%08lx with\n",	 start, end );
	printf( "%s-accesses and %s-pattern (%sdestructive %s)\n",	access, pat, dest, mode & MD_AUTOINCREMENT ? "autoinc.":"" );
	printf("\n");


	for( pass=0; (pass < nruns) || (nruns == 0); pass++ ) {
		printf("LOOP %d: \n", pass+1);
		if((errorcount += ramtest( start, end, mode, info, error )) == -1 ){
			fprintf(stderr,"\n");
			return 1;
		}
		printf("       ");
		fmt = "ERRORS=%6d\n";
		printf(fmt, errorcount);
	}
	printf("\n");
	return 0;
}

