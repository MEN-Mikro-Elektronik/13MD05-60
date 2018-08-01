/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *         \file looptest.c
 *       \author dieter.pfeuffer@men.de
 *        $Date: 2006/02/17 15:53:25 $
 *    $Revision: 1.2 $
 *
 *        \brief A404 looptest
 *
 *     Required: -
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: looptest.c,v $
 * Revision 1.2  2006/02/17 15:53:25  DPfeuffer
 * - enable A32 space fixed
 * - parameter <ext> removed
 *
 * Revision 1.1  2005/12/07 10:58:16  dpfeuffer
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>

#include <MEN/men_typs.h>
#include <MEN/maccess.h>
#include <MEN/a404_reg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define MTEST_JUSTVERIFY 	0x01
#define MTEST_JUSTWRITE		0x02

#define ERR_READ	1
#define ERR_WRITE	2
#define ERR_BERR	3
#define ERR_MIRR	4
#define ERR_RW		5

#define INFOSTEP	32768

#define SWAP16(word)	(((word)>>8) | ((word)<<8))

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static int just_verify;			/* don't write RAM, just verify */
static int just_write;			/* 1: write only mode */
static int max_errors;			/* max. errorcount until program abort */
static int tot_errors;			/* total errors occured */
static int pass;				/* test pass */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int a404mtest( u_int32 startadr, u_int32 endadr, u_int32 test, u_int32 opt, u_int32 run );
static int rxstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch );
static int txstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch );

/***************************************************************************/
/**  Print program usage
 */
static void usage(void)
{
	printf(
	"Usage:\n"
	"  a404_looptest <A24-brd-addr> <A32-VME-addr> <A32-brd-offset> <runs>\n"
	"Function:\n"
    "  - write pattern to TX12 space and verify\n"
    "  - then read pattern from RX1/RX2 space and verify\n"  
	"Parameters:\n"
	"  <A24-brd-addr>    A24 board address (A24 VMEbus space + A24 board offset)\n"
	"  <A32-VME-addr>    A32 VMEbus space (-1 for A24 access)\n"
	"  <A32-brd-offset>  A32 board offset (-1 for A24 access)\n"
	"  Note: A32 access requires A404 in enhanced mode\n"
	"        A32 access at <A32-VME-addr> + <A32-brd-offset>\n"
	"  <runs>            number of runs to execute [1...n]\n"
	"Note:\n"
    "  - requires external TX1/TX2->RX1/RX2 loop\n"	
	"\n"
	"(c) 2005 by MEN mikro elektronik GmbH\n\n");
}
 
/********************************* main ************************************/
/** a404_looptest entry
 *
 *  \return	          success (0) or error (1)
 */
int a404_looptest(
	u_int32 a24BrdBase,
	u_int32 a32VmeAddr,
	u_int32 a32BrdOff,
	u_int32 runs )
{
	u_int32 	start, size, end, em, n;
	u_int8 		ch, regVal;
	int			ret=0;
	MACCESS		ma;
	char		*t, tests[] = {'b','w','l',0};

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if( a24BrdBase == 0){
		usage();
		return -1;
	}
		
	if( runs == 0){
		printf("*** runs must be >0\n");
		return -1;
	}

	printf("a404_looptest: a24BrdBase=0x%lx, a32VmeAddr=0x%lx, "
		"a32BrdOff=0x%lx, runs=%lu\n", 
		a24BrdBase, a32VmeAddr, a32BrdOff, runs); 

	ma = (MACCESS)a24BrdBase;

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
    for( ch=1; ch<=2; ch++ ){ 
		/* config RX */
		MWRITE_D8( ma, A404_RXCTRL(em,ch), A404_RXCTRL_FFRST_NO );
		MWRITE_D8( ma, A404_RXSTAT(em,ch), 0x00 );
		/* config TX */
		MWRITE_D8( ma, A404_TXCTRL(em,ch), 0x00 );
			
		/* clear RX status */
		MWRITE_D8( ma, A404_RXSTAT(em,ch), 0x00 );
	
		/* clear TX status */
		MWRITE_D8( ma, A404_TXSTAT(em,ch), A404_TXSTAT_DATALOST |
									   	   A404_TXSTAT_CRCERR );	
	}

	if( a32VmeAddr != -1 ){
		
		if( !em ){
			printf("*** A32 access requires A404 in enhanced mode\n");
			return -1;		
		}
		
		/* set and enable A32 space */
		MWRITE_D16( ma, A404_VME_A32BASE_EM,
			SWAP16(A404_VME_A32BASE_ADDR(a32BrdOff) | A404_VME_A32BASE_ENBL ));
			
		printf("MWRITE_D16( 0x%x, 0x%x, 0x%x )\n",
			ma, A404_VME_A32BASE_EM,
			SWAP16(A404_VME_A32BASE_ADDR(a32BrdOff) | A404_VME_A32BASE_ENBL));
			
		ma = (MACCESS)(a32VmeAddr + a32BrdOff);
		printf("using A32 space, brd-addr=0x%lx\n", (u_int32)ma);
	}
	else{
		printf("using A24 space, brd-addr=0x%lx\n", (u_int32)ma);
	}

	/*--------------------+
    |  test               |
    +--------------------*/		
	size = A404_TRX_SIZE(em);

	for( n=0; n<runs; n++ ){

		t = tests;
		printf("\n----- run #%lu -----", n+1);

		while ( *t ){
			
			/* enhanced mode */
			if( em ){
			 	/* write/read/verify TX12 space */
			 	start = ((u_int32)ma) + A404_TX12(em);
			 	end = start + size-1;
				
				printf("\nwrite/read/verify TX12 space (address 0x%08lx...0x%08lx)\n", 
					 start, end);
		
			 	if( (ret = a404mtest( start, end, *t, 0x00, n )) )
			 		return ret;
			}
			/* compatibility mode */
			else {
			 	/* write/read/verify TX1 space */
			    for( ch=1; ch<=2; ch++ ){  	
				 	start = ((u_int32)ma) + A404_TX_CM(ch);
				 	end = start + size-1;
					
					printf("\nwrite/read/verify TX%d space (address 0x%08lx...0x%08lx)\n", 
						 ch, start, end);
			
				 	if( (ret = a404mtest( start, end, *t, 0x00, n )) )
				 		return ret;		
				}		
			}
				
		 	/* read/verify from RX1/RX2 space */
		    for( ch=1; ch<=2; ch++ ){  	
		 		start = ((u_int32)ma) + A404_RX(em,ch);
		 		end = start + size-1;
	
				printf("\nread/verify RX%d space (address 0x%08lx...0x%08lx)\n", 
					 ch, start, end);
	
			 	if( (ret = a404mtest( start, end, *t, MTEST_JUSTVERIFY, n )) )
			 		return ret;
			}
			t++;
		}
	}
	printf("\n");

	printf("\n----- TXSTAT -----\n");
    for( ch=1; ch<=2; ch++ ){  	
		regVal = txstatGetClr( (u_int32)ma, em, ch );
		
		if( regVal ){
			printf("*** TXSTAT%d indicates an error\n", ch);
			ret = -1;
		}
	}

	printf("\n----- RXSTAT -----\n");
    for( ch=1; ch<=2; ch++ ){  	
		regVal = rxstatGetClr( (u_int32)ma, em, ch );
		
		if( regVal != (A404_RXSTAT_CONNECT | A404_RXSTAT_FFFULL_NO) ){
			printf("*** RXSTAT%d indicates an error\n", ch);
			ret = -1;
		}
	}
	
	return ret;
}

/***************************************************************************/
/** Get and clear RXSTAT1/2
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  channel 1 or 2
 *
 *  \return		gotten value of RXSTAT1/2
 */
static int rxstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch )
{
	u_int8 regVal, fifoMask, fifoFlags;
	MACCESS	ma = (MACCESS)brdBase;

	/* get */
	regVal = MREAD_D8( ma, A404_RXSTAT(em,ch) );
	
	printf(" RXSTAT%d=0x%02x\n", (int)ch, regVal);
	regVal & A404_RXSTAT_CONNECT 	? printf(" - CONNECTED\n") : 0;
	regVal & A404_RXSTAT_CRCERR 	? printf(" - RCV_CRC_ERR\n") : 0;
	regVal & A404_RXSTAT_DATALOST 	? printf(" - RX_DATA_LOST\n") : 0;

	/* evaluate FIFO state */
	fifoMask = A404_RXSTAT_FFEMPTY_NO | A404_RXSTAT_FFFULL_NO;
	fifoFlags = regVal & fifoMask;

	if( fifoFlags == 
		(A404_RXSTAT_FFEMPTY_NO | A404_RXSTAT_FFFULL_NO) ){
		printf(" - FIFO: contains data (RCV_FF_EMPTY_n | RCV_FF_FULL_n)\n");
	}
	else if( fifoFlags ==
		A404_RXSTAT_FFEMPTY_NO ){
		printf(" - FIFO: full (RCV_FF_EMPTY_n)\n");
	}
	else if( fifoFlags ==
		A404_RXSTAT_FFFULL_NO ){
		printf(" - FIFO: empty (A404_RXSTAT_FFFULL_NO)\n");
	}
	else{ 
		printf(" - FIFO: *** illegal state (full AND empty)\n");
	}
	
	/* clear */
	MWRITE_D8( ma, A404_RXSTAT(em,ch), 0x00 );
	
	return regVal;
}

/***************************************************************************/
/** Get and clear TXSTAT1/2
 *
 *  Requirements:
 *
 *  \param brdBase	\IN  A24 or A32 base address the board
 *  \param em		\IN  0: compatibility mode
 *                       1: enhanced mode
 *  \param ch		\IN  channel 1 or 2
 *
 *  \return		gotten value of TXSTAT1/2
 */
static int txstatGetClr( u_int32 brdBase, u_int32 em, u_int32 ch )
{
	u_int8 regVal;
	MACCESS	ma = (MACCESS)brdBase;

	/* get */
	regVal = MREAD_D8( ma, A404_TXSTAT(em,ch) );
	
	printf(" TXSTAT%d=0x%02x\n", (int)ch, regVal);
	regVal & A404_TXSTAT_DATALOST 	? printf(" - TX_DATA_LOST\n") : 0;
	regVal & A404_TXSTAT_CRCERR 	? printf(" - TX_CRCERR\n") : 0;
	
	/* clear DATALOST and CRCERR */
	MWRITE_D8( ma, A404_TXSTAT(em,ch), A404_TXSTAT_DATALOST |
	                                   A404_TXSTAT_CRCERR );
	
	return regVal;
}

/***************************** memory test functions **************************/

static void fill_info( u_int32 bytes )
{
#if DBG	
  	printf("      filling %lu bytes\n", bytes);
#endif
}

static void ok_after_retries( u_int32 address, int num_tries )
{
  	printf("ok_after_retries: address=0x%lx, num_tries=%d\n",
  		address, num_tries);
}

static void is_mirroring( u_int32 address, u_int32 size )
{
  	printf("is_mirroring: address=0x%lx, size=%lu\n",
  		address, size);
}

static void action_info( char *info )
{
  	printf("  %s ", info);
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

	p += sprintf(fmt, "%s", "      *** %s ERROR:  addr=0x%08x, ");

	if( err_type == ERR_READ || err_type == ERR_WRITE || err_type==ERR_RW){
		switch( access_size ){
		case 1:	sprintf( p, "%s", "is:0x%02x, shouldbe:0x%02x\n"); break;
		case 2:	sprintf( p, "%s", "is:0x%04x, shouldbe:0x%04x\n"); break;
		case 4:	sprintf( p, "%s", "is:0x%08x, shouldbe:0x%08x\n"); break;
		}
	}
	printf( fmt, get_errtype_str(err_type), address, is, shouldbe);
  
	if( ++tot_errors >= max_errors ){
		return -1;
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
	                | 1) retry access 3 times                   |\
	                | 2) Check for mirrored memory              |\
	                | 3) Display error to user                  |\
	                +------------------------------------------*/\
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

static int a404mtest( u_int32 startadr, u_int32 endadr, u_int32 test, u_int32 opt, u_int32 run )
{
	u_int32 errorcount = 0;
	
	just_verify = 0;
	just_write = 0;
	max_errors = 10;
	tot_errors = 0;
	pass = run;
		
	if( opt & MTEST_JUSTVERIFY )
		just_verify = 1;
	
	if( opt & MTEST_JUSTWRITE )
		just_write = 1;
	
	switch( test ){
	case 'b':
		printf("  byte access, random pattern:");
		BWL_TEST(volatile u_int8, mk_rand_pat); break;
	case 'w':
		printf("  word access, random pattern:");
		BWL_TEST(volatile u_int16, mk_rand_pat); break;
	case 'l':
		printf("  long access, random pattern:");
		BWL_TEST(volatile u_int32, mk_rand_pat); break;
	case 'B':
		printf("  byte access, linear pattern:");
		BWL_TEST(volatile u_int8, mk_lin_pat); break;
	case 'W':
		printf("  word access, linear pattern:");
		BWL_TEST(volatile u_int16, mk_lin_pat); break;
	case 'L':
		printf("  long access, linear pattern:");
		BWL_TEST(volatile u_int32, mk_lin_pat); break;
	default:
		printf("*** unknown test\n");
		errorcount++;
	}
		
	return errorcount;
}


