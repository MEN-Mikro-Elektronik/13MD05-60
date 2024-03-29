/****************************************************************************
 ************                                                    ************
 ************           MTEST                                    ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: kp
 *        $Date: 2005/06/23 13:00:37 $
 *    $Revision: 1.6 $
 *
 *  Description: Utility to test RAM
 *                      
 *                      
 *     Required: testutil.l usr_oss.l 
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mtest2.c,v $
 * Revision 1.6  2005/06/23 13:00:37  UFranke
 * fixed
 *  - GCC problem
 *
 * Revision 1.5  2005/06/22 15:29:27  kp
 * Free mem if mtest called without arguments
 *
 * Revision 1.4  2005/02/15 16:33:43  ufranke
 * cosmetics
 *
 * Revision 1.3  2003/10/01 12:42:22  UFranke
 * added
 *  - cache test
 *
 * Revision 1.2  2001/01/30 13:35:32  franke
 * Bugfix: memory will freed only when allocated
 *
 * Revision 1.1  2000/01/13 14:45:07  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
//static char RCSid[]="$Id: mtest2.c,v 1.6 2005/06/23 13:00:37 UFranke Exp $";

#include <vxWorks.h>
#include <string.h>
#include <memLib.h>
#include <cacheLib.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/testutil.h>

extern int32 sysMenPci2VmeDma();

#ifdef VMEBLT
#include <MEN/sysMenPci2Vme.h>
#endif

#define ERR_READ	1
#define ERR_WRITE	2
#define ERR_BERR	3
#define ERR_MIRR	4
#define ERR_RW		5

#define MSGWDW_WIDTH 39
#define ERRWDW_WIDTH 40

#define INFOSTEP	32768

#define OUTPUT_SCREEN		0
#define OUTPUT_SEQU_ERR		1
#define OUTPUT_SEQU			2
#define OUTPUT_SEQU_VERB	3

typedef struct {
    char test_id;
	char *action_line;
	int errcnt;
} test_descr;

test_descr test[] = {
	{ 'b', "Byte access, random pattern" },
	{ 'w', "Word access, random pattern" },
	{ 'l', "Long access, random pattern" },
	{ 'B', "Byte access, linear pattern" },
	{ 'W', "Word access, linear pattern" },
	{ 'L', "Long access, linear pattern" },
	{ 'c', "cacheline write/verify, random pattern" },
	{ 'C', "cacheline write/verify, linear pattern" },
#ifdef VMEBLT
	{ 'v', "VMEbus block xfer, random pattern" },
	{ 'V', "VMEbus block xfer, linear pattern" },
#endif
	{ 0 }
};

static void version(void);


/*------------------+
| Global variables  |
+------------------*/
static u_int32 startadr, endadr;			/* start and end address of memory */
static int pass;							/* test pass */
static int just_verify;					/* don't write RAM, just verify */
static int max_errors=(u_int32)(1<<31)-1;	/* max. errorcount until program abort */
static int tot_errors=0;					/* total errors occured */
static u_int32 blk_buf_addr=0xffffffff;	/* external buffer addr for blk/move16 */
static u_int32 hwtrigacc=0;				/* address to access in case of error  */
static int outputMode;					/* outputMode (see OUTPUT_XXX) */
static int just_write;					/* 1: write only mode */
#ifdef VMEBLT
static u_int32 G_bltModifier;			/* sysMenPci2VmeDma(modifier) */
static int     G_bltSram;				/* sysMenPci2VmeDma(viaSram) */
static int     G_bltTout;				/* sysMenPci2VmeDma(timeOutTicks) */
#define ILL_BLTMOD 0xffffffff
#endif

static char *usage_str =
"Syntax:   mtest [<opts>] [<startaddr>] [<endaddr>] [<opts>]\n"
"Function: memory test\n"
"System memory will be tested if startadr and endadr missing\n"
"Options:\n"
"    -a=<adr>   use external buffer for VMEblk/Move16 transfers\n"
"    -n=<num>   number of runs\n"
"    -q=<num>   abort after <num> errors\n"
"    -v         don't write to RAM, just verify\n"
"    -x=<adr>   access address for HW trigger in case of errors (byte acc)\n"
"    -o=<mode>  output mode:\n"
"                 0 = console screen (default)\n"
"                 1 = sequential output (errors only)\n"
"                 2 = sequential output\n"
"                 3 = sequential output (verbose)\n"
"    -w         write only (no read/verify)\n"
#ifdef VMEBLT    
"    -m=<mode>  modes for sysMenPci2VmeDma, comma separated\n"
"                 e.g. \"a24d16,sw,60\"n\n"
"                 MODES:\n"
"                 a24d16,a24d32,a32d32,a32d64 : VME space\n"
"                 sw                          : swapped\n"
"                 sram                        : blt via sram"
"                 <timeout>                   : timeout in system ticks\n\n"
"   "
#endif
"    -f=<txt>   print commentary in first line\n"
"               e. g. test_text\n"
"    -t=<lst>   list of tests to execute:\n\n";

static int usage( int no )
{
	test_descr *test_p = test;
	fprintf( stderr, "%s", usage_str );

	while(test_p->test_id){
		fprintf(stderr, "       %c: %s\n", test_p->test_id, 
				test_p->action_line);
		test_p++;
	}
	version();
	return 1;
}

static void version(void)
{
	char *rev = "$Revision: 1.6 $";
	char *p = rev+strlen("$Revision: ");

	fprintf(stderr, "\nV ");
	while( *p != '$' ) fprintf(stderr, "%c", *p++);
	fprintf(stderr, " (c) Copyright 1995-2005 by MEN GmbH\n");
	fprintf(stderr, " Build " __DATE__ " " __TIME__ "\n");
}


static void hw_trigger(void)
{
	u_int8 dummy;

	if( hwtrigacc ){
		dummy = *(volatile u_int8 *)hwtrigacc;
	}
}

static void action_info( char *info )
{
	int len;

  if( outputMode == OUTPUT_SEQU_VERB ){
  	printf("      action: %s\n", info);
  }else if( outputMode == OUTPUT_SCREEN ){
	len = tu_print( WDW_MSG, 0, 3, info );
	tu_print( WDW_MSG, len, 3, "%*s", MSGWDW_WIDTH-len, "");
  }
}

static void fill_info( u_int32 bytes )
{
  if( outputMode == OUTPUT_SEQU_VERB ){
  	printf("      filling %d bytes\n", bytes);
  }else if( outputMode == OUTPUT_SCREEN ){
	tu_print( WDW_MSG, 25, 3, "%6dK", bytes/1024 );
  }
}

static void ok_after_retries( u_int32 address, int num_tries )
{
  if( outputMode >= OUTPUT_SEQU  ){
  	printf("ok_after_retries: address=0x%x, num_tries=%d\n",
  		address, num_tries);
  }else if( outputMode == OUTPUT_SCREEN ){
	tu_clear( WDW_MSG, 0, 6, 0 );
	tu_clear( WDW_MSG, 0, 7, 0 );
	tu_print( WDW_MSG, 0, 6, "Addr %08x ok", address );
	tu_print( WDW_MSG, 0, 7, "after %d retries", num_tries );
  }
}

static void is_mirroring( u_int32 address, u_int32 size )
{
  if( outputMode >= OUTPUT_SEQU  ){
  	printf("is_mirroring: address=0x%x, size=%d\n",
  		address, size);
  }else if( outputMode == OUTPUT_SCREEN ){
	tu_clear( WDW_MSG, 0, 6, 0 );
	tu_clear( WDW_MSG, 0, 7, 0 );
	tu_print( WDW_MSG, 0, 6, "Addr  %08x mirroring", address );
	tu_print( WDW_MSG, 0, 7, "after %08x", size );
  }
}

static char *get_errtype_str( int err_type )
{
	switch(err_type){
	case ERR_READ: return "READ";
	case ERR_WRITE: return "WRIT";
	case ERR_BERR: return "BERR";
	case ERR_MIRR: return "MIRR";
	case ERR_RW:   return "RDWR";
	default:       return "????";
	}
}

static int out_error( int err_type, u_int32 address, u_int32 is, 
				   u_int32 shouldbe, int access_size )
{
	u_int32 mask = (1<<(access_size*8))-1;
	char fmt[50], *p=fmt;
	
	shouldbe &= mask;			/* mask unwanted bits */
	is &= mask;

  if( outputMode ){
	p += sprintf(fmt, "%s", "      *** %s ERROR:  addr=0x%08x, ");

	if( err_type == ERR_READ || err_type == ERR_WRITE || err_type==ERR_RW){
		switch( access_size ){
		case 1:	sprintf( p, "%s", "is:0x%02x, shouldbe:0x%02x\n"); break;
		case 2:	sprintf( p, "%s", "is:0x%04x, shouldbe:0x%04x\n"); break;
		case 4:	sprintf( p, "%s", "is:0x%08x, shouldbe:0x%08x\n"); break;
		}
	}
	printf( fmt, get_errtype_str(err_type), address, is, shouldbe);

  } else{
	p += sprintf(fmt, "%s", "%-4s %08x ");

	if( err_type == ERR_READ || err_type == ERR_WRITE || err_type==ERR_RW){
		switch( access_size ){
		case 1:	sprintf( p, "%s", "      %02x       %02x"); 	break;
		case 2:	sprintf( p, "%s", "    %04x     %04x"); 		break;
		case 4:	sprintf( p, "%s", "%08x %08x"); 				break;
		}
	}

	if( tu_print_error( 1, 1, fmt, get_errtype_str(err_type), address, is, 
					    shouldbe )){
		return -1;
	}
  }
  
	if( ++tot_errors >= max_errors ){
		return -1;
	}	
	return 0;
}

static int get_sysmem(void)
{
	u_int32 size;
	startadr = (u_int32)malloc( (size = memFindMax()) );

	if( startadr == (u_int32)NULL )
		return 1;

	endadr = startadr+size;
	return 0;
}

static int checkabort(void)
{
	int c;
	
  if( outputMode )
    return 0;	
	
	switch( c = tu_keybd_hit()){
	case 0x3:					/* CTRL-C */
	case 0x4:					/* CTRL-D */
	case 0x5:					/* CTRL-E */
	case 0x1b:					/* ESC */
		return 1;				/* abort */
	}
	return 0;
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
static u_int32 mk_rand_pat( u_int32 address, u_int32 oldpat, u_int8 size )
{
	u_int32 pattern;

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

/********************************** mk_lin_pat *****************************
 *
 *  Description:  Creates a linear testpattern which corresponds to the
 *				  given address                         
 *
 *---------------------------------------------------------------------------
 *  Input......:  address - used to produce the test pattern
 *
 *				  oldpat  - not used, just for compat. with mk_rand_pat
 *				  size    - required size for pattern
 *					        1=byte 2=word 4=long
 *
 *  Output.....:  return  - 8/16/32 bit linear pattern
 *				
 *  Globals....:  ---
 ****************************************************************************/
static u_int32 mk_lin_pat( u_int32 address, u_int32 oldpat, u_int8 size )
{
	switch(size){
	case 4: return address;
	case 2: return (address&0x2) ? (address&~0x2) : address>>16;
	case 1: 
		switch(address & 0x3 ){
		case 0:	return address>>24;
		case 1: return address>>16;
		case 2: return address>>8;
		case 3: return address&~0x3;
		}
	}
	return 0;
}
			


/********************************** BWL_TEST ********************************
 *
 *  Description:  Macro for Byte/Word/Long tests
 *                         
 *
 *---------------------------------------------------------------------------
 *  Input......:  access - test access type (u_int8, u_int16, u_int32)
 *				  pat_f  - function name to build test pattern (mk_xxx_pat)
 *  Output.....:  
 *  Variables..:  startadr, endadr, pass, just_verify
 ****************************************************************************/
#define BWL_TEST(access,pat_f)\
{\
	register u_int32 randval=0xABCDEF02+pass;\
	register access rval;\
	register access *p;\
	register u_int32 count=0, curlen;\
	u_int32 len = (endadr-startadr)/sizeof(access);\
	\
	p = (access*)startadr;\
	\
	/*--------------+\
    |  Fill memory  |\
    +--------------*/\
	if( !just_verify){\
     action_info("Filling Memory");\
	 while( len > 0){\
		if( checkabort()) return -1;\
		curlen = len;\
		if( curlen > INFOSTEP ) curlen = INFOSTEP;\
		count += curlen;\
		len -= curlen;\
		while(curlen--){\
			/* create a new test pattern */\
			randval = pat_f((u_int32)p,randval,sizeof(access));\
			*p++ = (access)randval;	/* store value */\
		}\
		fill_info(count*sizeof(access));\
	 }\
    }\
	if( !just_write ){\
		/*----------------+\
	    |  Verify Memory  |\
	    +----------------*/\
		p = (access*)startadr;\
		len = (endadr-startadr)/sizeof(access);\
		randval=0xABCDEF02+pass;\
		count=0;\
		\
	    action_info("Verify Memory");\
		while( len > 0){\
			if( checkabort()) return -1;\
			curlen = len;\
			if( curlen > INFOSTEP ) curlen = INFOSTEP;\
			count += curlen;\
			len -= curlen;\
			while(curlen--){\
				  /* create a new test pattern */\
	 			  randval = pat_f((u_int32)p,randval,sizeof(access));\
				  if( (rval = *p++) != (access)randval ){\
					int err_type=ERR_WRITE;\
					p--;\
					errorcount++;\
					/*------------------------------------------+\
	                | Verify error detected..                   |\
	                | 1) Set HW Trigger                         |\
	                | 3) retry access 3 times                   |\
	                | 2) Check for mirrored memory              |\
	                | 4) Display error to user                  |\
	                +------------------------------------------*/\
					hw_trigger();\
					{\
						/*--- retry access ---*/\
						int i=0;\
					    for(i=1; i<=3; i++)\
							if( *p == (access)randval){\
								ok_after_retries((u_int32)p,i);\
							    err_type=ERR_READ;\
								break;\
							}\
				    }\
					if( err_type == ERR_WRITE ){\
						access *t = p;\
						u_int32 step=1;\
						/*--- detect mirrored RAM ---*/\
						while( &t[step]<(access *)endadr ){\
							if( t[step]==(access)rval){\
								is_mirroring( (u_int32)p, (&t[step])-p );\
								err_type = ERR_MIRR;\
								break;\
							}\
							step<<=1;\
						}\
					}\
					if( out_error( err_type, (u_int32)p, rval, (access)randval,\
									  sizeof(access))) return -1;\
					p++;\
				}\
			}\
			fill_info(count*sizeof(access));\
		}\
	}\
}

#ifdef VMEBLT
/*-------------------------------------------+
| VMEbus block transfer using _osmen_vme_blt |
+-------------------------------------------*/
static int vmeblt( void *src, void *dst, u_int32 len, int direction )
{
	extern int errno;

	if( G_bltModifier == ILL_BLTMOD ){
		fprintf(stderr, "sysMenPci2VmeDma() requires -m=<mode>\n");
		return -1;
	}

	/*printf(	"sysMenPci2VmeDma(\n"
     		"direction=%d, src=0x%x, dst=0x%x, len=0x%x,\n" 
     		"G_bltSram=%d, G_bltModifier=%d, G_bltTout=%d )\n",
     		direction, (u_int32*)src, (u_int32*)dst, len, 
     		G_bltSram, G_bltModifier, G_bltTout);*/

	if( (errno = sysMenPci2VmeDma( direction, (u_int32*)src, 
								   (u_int32*)dst, len, 
     				G_bltSram, G_bltModifier, G_bltTout ))){

		fprintf(stderr, "sysMenPci2VmeDma() failed\n");
		return -1;
	}
	return 0;
}
   
static int blk_test(int rand_pattern, int (*readfunc)(), int (*writefunc)())
{
	u_int32 blk_size = INFOSTEP, cur_size;
	u_int32 pattern=0xabcdef02+pass;
	u_int32 *buf;
	u_int32 address, i, *p;
	int errcnt=0;

	if( blk_buf_addr == 0xffffffff ){
		buf = (u_int32 *)malloc(blk_size);
	
		if( !buf ){
			fprintf(stderr, "can't allocate block buffer\n");
			return -1;
		}
	}
	else {
		buf = (u_int32 *)blk_buf_addr;
	}

	address = startadr;

	/*---------------+
    |  Write memory  |
	+---------------*/
	action_info("Filling Memory");
	while( address < endadr ){
		if( checkabort()){
			errcnt = -1;
			goto CLEANUP;
		}
		/*--------------------+
        |  Fill write buffer  |
        +--------------------*/
		if( rand_pattern )
			for(p=buf,i=0; i<blk_size/4; i++)
				*p++ = pattern = mk_rand_pat(0, pattern, 4 );
		else
			for(p=buf,i=0; i<blk_size/4; i++)
				*p++ = address+(i<<2);

		/*----------------------+
        |  Write buffer to mem  |
        +----------------------*/
		cur_size = blk_size;
		if( cur_size > endadr-address )
			cur_size = endadr-address;
		if( (*writefunc)( buf, address, cur_size+1, 1) ){
			errcnt = -1;
			goto CLEANUP;
		}

		address += cur_size;

		fill_info(address-startadr);
	}

	address = startadr;
	pattern=0xabcdef02+pass;

	/*---------------+
    |  Verify memory |
	+---------------*/
    action_info("Verify Memory");
	while( address < endadr ){
		if( checkabort()){
			errcnt = -1;
			goto CLEANUP;
		}
		/*------------------------+
        |  Read to buffer memory  |
        +------------------------*/
		cur_size = blk_size;
		if( cur_size > endadr-address )
			cur_size = endadr-address;

		memset( buf, 0x55, cur_size ); /* clear before reading */

		if( (*readfunc)( address, buf, cur_size+1, 0 ) ){
			errcnt = -1;
			goto CLEANUP;
		}

		/*-----------------+
        |  Compare buffer  |
        +-----------------*/
		for(p=buf,i=0; i<cur_size/4; p++,i++){
			if( rand_pattern )
				pattern = mk_rand_pat(0, pattern, 4 );
			else
				pattern = address+(i<<2);
			
			if( *p != pattern ){
				errcnt++;
				hw_trigger();
				if( out_error( ERR_RW, address+(i<<2), *p, pattern, 4 )){
					errcnt = -1;
					goto CLEANUP;
				}
			}
		}
		address += cur_size;

		fill_info(address-startadr);
	}
	
CLEANUP:	
	if( blk_buf_addr == 0xffffffff) 
		free(buf);

	return errcnt;
}
#endif

int cache_test(int rand_pattern)
{
	u_int32 blk_size = 32, cacheLineCount=0;
	u_int32 pattern=0xabcdef02+pass;
	u_int32 address, i, *p;
	int errcnt=0;


	address = startadr;

	/*---------------+
    |  Write memory  |
	+---------------*/
	action_info("Testing Memory");
	while( address < endadr ){
		u_int32 startpattern = pattern;
		/*--------------------+
        |  Fill cache line    |
        +--------------------*/
		p = (u_int32 *)address;
		if( rand_pattern )
			for(i=0; i<blk_size/4; i++)
				*p++ = pattern = mk_rand_pat(0, pattern, 4 );
		else
			for(i=0; i<blk_size/4; i++)
				*p++ = address+(i<<2);

/*		cacheFlush( DATA_CACHE ,(void*)address, blk_size ); */

		/*--------------------+
        |  Reread cache line  |
        +--------------------*/
		pattern = startpattern;

		p = (u_int32 *)address;
		for(i=0; i<blk_size/4; i++){
			if( rand_pattern )
				pattern = mk_rand_pat(0, pattern, 4 );
			else
				pattern = address+(i<<2);

			if( *p != pattern ){
				errcnt++;
				hw_trigger();
				out_error( ERR_RW, address+(i<<2), *p, pattern, 4 );
			}
			p++;
		}

		address += blk_size;

		if( (cacheLineCount % 4096) == 0 )
			fill_info(address-startadr);
		cacheLineCount++;
	}


	return errcnt;
}
				
static int do_test(char test_id)
{
	u_int32 errorcount=0;

	/* install exeception handler???*/

	switch( test_id ){
	case 'b':
		BWL_TEST(volatile u_int8, mk_rand_pat); break;
	case 'w':
		BWL_TEST(volatile u_int16, mk_rand_pat); break;
	case 'l':
		BWL_TEST(volatile u_int32, mk_rand_pat); break;
	case 'B':
		BWL_TEST(volatile u_int8, mk_lin_pat); break;
	case 'W':
		BWL_TEST(volatile u_int16, mk_lin_pat); break;
	case 'L':
		BWL_TEST(volatile u_int32, mk_lin_pat); break;
#if 0
	case 'm':
		errorcount = blk_test( 1, move16_xfer, move16_xfer); break;
	case 'M':
		errorcount = blk_test( 0, move16_xfer, move16_xfer); break;
#endif
#ifdef VMEBLT
	case 'v':
		errorcount = blk_test( 1, vmeblt, vmeblt); break;
	case 'V':
		errorcount = blk_test( 0, vmeblt, vmeblt); break;
#endif
	case 'c':
		errorcount = cache_test(1); break;
	case 'C':
		errorcount = cache_test(0); break;
	}


	return errorcount;
}

#ifdef VMEBLT
int get_blt_mode( int argc, char *argv[] )
{
	char *optp;
	G_bltModifier = ILL_BLTMOD;
	
	if( ( optp = UTL_TSTOPT("m="))){
		char *tok;

		u_int32 swap = 0;
		int i;
		char buf[200];
		extern char *strtok();
	
		struct modStruct{
			char 	*mode;
			u_int32 val;
			u_int32 valSw;
		} modTup[] = {
			{ "a24d16", SYS_VME_A24_D16, SYS_VME_A24_D16_SWAPPED },
			{ "a24d32", SYS_VME_A24_D32, SYS_VME_A24_D32_SWAPPED },
			{ "a32d32",	SYS_VME_A32_D32, SYS_VME_A32_D32_SWAPPED },
			{ "a32d64",	SYS_VME_A32_D64, SYS_VME_A32_D64_SWAPPED },
			{ NULL }
		};

		struct modStruct *modP=NULL;

		/* defaults */
		G_bltSram = 0;	/* noswap */

		if( outputMode >= OUTPUT_SEQU  ){
			printf("VMEbus block xfer mode: %s\n",optp);
		}else if( outputMode == OUTPUT_SCREEN ){  
			tu_print(WDW_MSG, 0,2, "%s", optp );
		}

		/*-------------+
		|  Parse mode  |
		+-------------*/
		strcpy(buf,optp);

		tok = strtok( buf, "," );

		while( tok ){
			/*printf("tok=%s\n", tok );*/

			for( i=0; modTup[i].mode; i++ ){
				if( strcmp( tok, "sw" ) == 0 ){
					swap = 1;
				}
				else if( strcmp( tok, "sram" ) == 0 ){
					G_bltSram = 1;
				}
				else if( strcmp( tok, modTup[i].mode ) == 0 ){
					modP = &modTup[i];
				}
				else{
					G_bltTout = atoi(tok);
				} 
			}

			tok = strtok( NULL, "," );
		}
		
		if( modP == NULL ){
			fprintf(stderr,"Unknown modifier for VMEbus block xfer\n");
			return 1;
		}
		
		if( G_bltTout == 0 )
			G_bltTout = -1;	/* wait forever */
		
		if( swap )
			G_bltModifier = modP->valSw;
		else
			G_bltModifier = modP->val;		
	}
		
	return 0;
}
#endif

/* int main( int argc, char *argv[] ) */
/* ts: this can not be called from vxShell! */
int mtest2( int argc, char *argv[] )
{
    static char *test_txt={" "}, *str_txt; 
	int i, total_pass, len;
	char *optp, *testlist;
	test_descr *test_p;
	char *test_id;
	int errors=0, use_sysmem=0;

	printf("mtest2 called with argc=%d\n", argc);
	for (i=0;i<argc;i++)
		printf("argv[%d]=%s\n", i, argv[i]);

	if( UTL_TSTOPT("?")) {
		usage(0);	
		return 1;
	}

	/*-----------------------------+
    |  Parse command line options  |
    +-----------------------------*/
	pass = 0;
	just_verify = 0;
	just_write = 0;
	max_errors=(u_int32)(1<<31)-1;
	tot_errors=0;
	blk_buf_addr=0xffffffff;
	hwtrigacc=0;
	startadr = endadr = 0xffffffff;

	for(i=1; i<argc; i++ )
		if( *argv[i]!='-'){
			if( startadr == 0xffffffff ) 
				sscanf( argv[i], "%lx", &startadr );
			else
				sscanf( argv[i], "%lx", &endadr );
		}

	if( startadr==0xffffffff || endadr==0xffffffff ){
		if( get_sysmem() )
			return 1;

		use_sysmem=1;
	}

	total_pass = 1;
	if( (optp=UTL_TSTOPT("n="))) total_pass=atoi(optp);
	
	just_verify = (int)UTL_TSTOPT("v");
	outputMode = ((optp = UTL_TSTOPT("o=")) ? atoi(optp) : 0);
	just_write = (int)UTL_TSTOPT("w");
	testlist = UTL_TSTOPT("t=");
	testlist="v";

	if( !testlist ) {
		usage(1);
		goto DONE;
	}

	if( (optp=UTL_TSTOPT("q="))) max_errors=atoi(optp);
	if( (optp=UTL_TSTOPT("a="))) sscanf(optp, "%lx", &blk_buf_addr);
	if( (optp=UTL_TSTOPT("x="))){
		sscanf(optp, "%lx", &hwtrigacc );
	}

#ifdef VMEBLT
	if( get_blt_mode(argc, argv) )
		return 1;
#endif

    /* Test Text option */
	str_txt = ((test_txt = UTL_TSTOPT("f=")) ? (test_txt): " ");

	/*------------------+
    |  Init screen I/O  |
    +------------------*/
  if( outputMode >= OUTPUT_SEQU  ){
	printf("testing address 0x%08x...0x%08x\n", 
			 startadr, endadr);
  }else if( outputMode == OUTPUT_SCREEN ){  

    /* Test Text option */
    if(test_txt == NULL){      
        	tu_init_screen( "", 40, 10, (u_int32 )1 );
        }
        else
        {
        	tu_init_screen( test_txt, 40, 10, (u_int32 )1 );
        }

	tu_print_errhead("Type Address  Is       ShouldBe");
	{
		char *res_str="Test                            Errors";
		tu_print(WDW_RESULT,0,0, res_str);
		tu_print(WDW_RESULT,0,1, tu_underline_string(res_str));
	}

	/*----------------+
    |  Execute tests  |
    +----------------*/
	tu_print(WDW_MSG, 0,0, "Memory %08x .. %08x", 
			 startadr, endadr );
  }

	for( pass=0; pass<total_pass || total_pass==0; pass++ ){
  
	  if( outputMode >= OUTPUT_SEQU  ){
	  	printf("  pass=%d/%d\n", pass+1, total_pass);	
	  }else if( outputMode == OUTPUT_SCREEN ){
  		tu_print_pass( pass+1, total_pass );
  	  }

		for( i=0,test_id=testlist; *test_id; test_id++,i++ ){
			for( test_p=test; test_p->test_id; test_p++ )
				if( test_p->test_id == *test_id )
					break;
			
			test_p->errcnt = 0;
					
			if( test_p->test_id ){
              if( outputMode >= OUTPUT_SEQU  ){
              	printf("    - action: %s\n", test_p->action_line);
              }else if( outputMode == OUTPUT_SCREEN ){
  				len = tu_print(WDW_MSG, 0, 1, "%s", test_p->action_line );
				tu_print(WDW_MSG, len, 1, "%*s", MSGWDW_WIDTH-len, "");
              }
				if( (errors = do_test( *test_id )) < 0 )
					break;

				test_p->errcnt += errors;
				
              if( outputMode ){
              	if( test_p->errcnt )
              		printf("      errors=%d\n", test_p->errcnt);
              } else {
              	tu_print(WDW_RESULT,0,2+i, "%-32s %5d", test_p->action_line,
						 test_p->errcnt);
				tu_print_elapsed_time();
			  }
			}
		}
		if( (errors) < 0 )
			break;
	}
if( !outputMode )	
	tu_exit();
 DONE:
	if( use_sysmem )
	{
		/*printf("free %08x\n", startadr );*/
		free((void *)startadr);
	}

	return tot_errors ? 1 : 0;
}


int mtestcall( void )
{
	char *args[]={"-t=vV", "-m=a24d16,sw,60", "-n=1", "0xfe000000", "0xfe010000"};

	return 	mtest2( 5, args );
}

