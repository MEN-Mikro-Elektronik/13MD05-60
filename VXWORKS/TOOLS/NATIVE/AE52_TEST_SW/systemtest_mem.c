/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest_mem.c
 *
 *      \author  MKolpak
 *        $Date: 2012/09/06 15:32:11 $
 *    $Revision: 1.2 $
 *
 *        \brief  stress Task for RAM/FLASH/NVSRAM
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_mem.c,v $
 * Revision 1.2  2012/09/06 15:32:11  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:19  MKolpak
 * Initial Revision
 *
 *
 * cloned from EP6 Revision 1.1  2011/02/25 09:38:10  rlange
 *---------------------------------------------------------------------------
 * (c) Copyright 2010..2011 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/


/*--------------------------------------------------------------+
 |	INCLUDE														|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	TYPEDEFS													|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	DEFINES 													|
 +-------------------------------------------------------------*/


#define STRESS_SDRAM_SIZE				0x5100000
#define STRESS_SDRAM_SIZE_ALT			0x4000000

#define EP06_FRAM_ADDR					0xF2000000
#define STRESS_FRAM_SIZE				0x5300
#define STRESS_FRAM_SIZE_ALT			0x8000
#define STRESS_FRAM_RETRY				2

#define STRESS_FLASH_MODE_LONG_FORWARD_INITIAL		0
#define STRESS_FLASH_MODE_LONG_FORWARD				1
#define STRESS_FLASH_MODE_LONG_BACKWARD				2
#define STRESS_FLASH_MODE_LONG_FORWARD_CACHED		3
#define STRESS_FLASH_MODE_LONG_BACKWARD_CACHED		4

/*--------------------------------------------------------------+
 |	GLOBALS														|
 +-------------------------------------------------------------*/
u_int32  stressFlashXor_G	 = 0x00000000;
u_int32  stressAppFlashXor_G = 0x00000000;
u_int32  stressSpiId_G		 = 0x00000000;

/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/
u_int32 stressFlashCalcExor( STRESS_TEST_HDL *h, int mode );

extern int spiFlashDevIdGet
(
	int cs    /* CS 0..3 */
);

extern void nvsram_write_pattern
(
	int which, /* 0 - ~address, 1 - address - 2 - loopcount + (address << 8) */
	int verbose
);
extern int nvsram_check_pattern
(
	int which, /* 0 - ~address, 1 - address - 2 - loopcount + (address << 8) */
	int verbose
);

/*--------------------------------------------------------------+
 |	INCLUDES  													|
 +-------------------------------------------------------------*/



/**********************************************************************/
/** Routine to init RAM memory stress test.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressSdramInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;

	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "Sdram Init done;");
	}
}

/**********************************************************************/
/** Routine for RAM memory stress test task.
 *
 *  This RAM Test fills 0x04000000 byte with linear or linear invers
 *  pattern and read it back and check it.
 *  The CPU cache mode will not be changed ( default CACHE_COPYBACK and
 *  CACHE_SNOOP_ENABLE). After each compare loop, the RAM ECC counter 
 *  will be checked. Recoverable ECC errors will be count as error.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressSdram( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 *sdramP = NULL;
	u_int32 i;
	u_int32 k = 0;
	u_int32 *tmpP = NULL;
	u_int32 size = 0;
	int		eccErrorCount;
	int		eccErrorCountTotal = 0;

#ifdef _WRS_CONFIG_SMP                
		h->cpu = vxCpuIndexGet();
#endif

	taskDelay( sysClkRateGet()  + 5 * h->id );

	if( 0 ) /* § hard coded */
	{
		size = STRESS_SDRAM_SIZE;
	}
	else
	{
		size = STRESS_SDRAM_SIZE_ALT;
	}
	if( (tmpP = malloc( size )) == NULL )
	{
		ST_ERR("buffer=0x%x size %08x",tmpP, size );
		goto CLEANUP;
	}
	sdramP = tmpP;

	while( h->enabled )
	{
		/* were alive! */
		h->alive = 1;
		
		/* recalculate delay timing */
		if( h->load_act!= load )
		{
			load = h->load_act;
			if( load == 99 )
			{
				delay = 1;
			}
			else
			{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		tick = tickGet();


		/* write - linear pattern  - start value = loop count 
		 *       - ~linear pattern - start value = ~loop count 
		 *       - TODO 0x5a5a5a5a 
		 *       - TODO 0x55aa55aa 
		 *       - TODO 0x5555aaaa 
		 *       - TODO 0x55555555 0xAAAAAAAA
		 *       - TODO 0x00000000 0xFFFFFFFF
		 *       - TODO 0xFFFFFFFE walking zero
		 *       - TODO 0x00000001 walking one
		 *       - TODO pseudo random
		 */
		k 	   = h->loopCnt;
		sdramP = tmpP;
		for( i=0; i < (size - 4);i += sizeof(u_int32) )
		{
			if( !(h->loopCnt % 13) )
				*sdramP = ~k; /* fill */
			else
				*sdramP = k; /* fill */
			sdramP++;
			k++;
			if(!(i%0x80000))
			{
				h->bytenum += 0x80000 << 2; /* long */
				taskDelay( 1 ); /* be nice */
				if( !h->enabled )
					break;
			}
		}
		
		/* were alive! */
		h->alive = 1;

		/* read back */
		k 	   = h->loopCnt;
		sdramP = tmpP;
		for( i=0; i < (size - 4);i += sizeof(u_int32) )
		{
			int		verifyErr;
			u_int32	is;	
			u_int32	sb;	

			if( (h->bytenum ) <= 0 )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTick = tickGet();
			}

			is = *sdramP;
			if( !(h->loopCnt % 13) )
			{
				sb = ~k;
			}
			else
			{
				sb = k;
			}
			if( is != sb )
				verifyErr = 1;
			else
				verifyErr = 0;

			if( verifyErr )
			{
				stressDbg(NULL,STRESS_DBGLEV_1, "*** (SDRAM) verify @%08x %08x/%08x %d;", sdramP, is, sb, h->loopCnt );
				h->errCnt++;
			}

			sdramP++;
			k++;

			if(!(i%0x80000))
			{
				h->bytenum += 0x80000 << 2; /* long */
				taskDelay( 1 ); /* be nice */
				if( !h->enabled )
					break;
			}

			if(!(i%10000000))
			{
				stressDbg(NULL,STRESS_DBGLEV_3, "sdram alive;");
				/* restart measurement */
				tick = tickGet();
			}
		}


		eccErrorCount = sysMenMemEccGet( 0 /* all mem controllers */ );
		if( eccErrorCount )
		{
			sysMenMemEccClear(  0 /* all mem controllers */ );

			stressDbg(NULL,STRESS_DBGLEV_1, "*** (SDRAM) ECC err  ***;");
			h->errCnt += eccErrorCount;
			eccErrorCountTotal += eccErrorCount;
		}

		if( h->verbose & STRESS_DBGLEV_2 )
		{
			stressWrtDbg( hP->dbgP, "sdram alive;");
		}
		
		taskDelay( delay );
		
		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}

CLEANUP:
	if( tmpP )
	{
		free( tmpP );
	}

	/* printf( 
		\n>>> %s end after %d loops buff %08x size %08x errCnt %d eccErrorCount %d\n", 
		h->name, 
		h->loopCnt, 
		tmpP, 
		size, 
		h->errCnt, 
		eccErrorCountTotal );*/
	h->isFinished = 1; /* say finished to shutting down */

	return;
}


#if 0
/**********************************************************************/
/** Routine to init FRAM memory stress test - not used for BARCO SC23.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressFramInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;
	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "Fram Init done;");
	}
}

/**********************************************************************/
/** Routine for Fram memory stress test task - not used for BARCO SC23.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressFram( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int8 *framP = NULL;
	u_int32 i;
	u_int8 k = 0;
	u_int8 *tmpP = (u_int8*)EP06_FRAM_ADDR;
	u_int32 retry = 0;
	u_int32 size = 0;
	
	if( G_PwrConsum ){
		size = STRESS_FRAM_SIZE;
	}
	else{
		size = STRESS_FRAM_SIZE_ALT;
	}
	
	while( h->enabled ){
		/* recalculate delay timing */
		if( h->load != load ){
			load = h->load;
			if( load == 99 ){
				delay = 1;
			}
			else{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		tick = tickGet();

		for( retry = 0; retry < STRESS_FRAM_RETRY; retry++ ){
			k = 0;
	
			framP = tmpP;
	
			for( i=0; i < (STRESS_FRAM_SIZE - 1);i++ ){
	
				if( (h->bytenum) <= 0 )
				{
					/* overflow occurred, clear startime to fix Xfer-Rate */
					h->startTime = tickGet();
				}
	
				h->bytenum += (sizeof(u_int8)*2);
	
				*framP = k;
				if( *framP != k ){
					if( h->verbose & STRESS_DBGLEV_1 ){
						stressWrtDbg( hP->dbgP, "*** (FRAM) verify ***;");
					}
					h->errCnt++;
				}
	
				framP++;
				k++;
			}
		}

		if( h->verbose & STRESS_DBGLEV_2 ){
			stressWrtDbg( hP->dbgP, "fram alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled ){
			taskDelay(1);
		};

	}
}


/**********************************************************************/
/** Routine to init FLASH memory stress test.
 *
 *  The inital XOR over boot FLASH at 0xee000000 size 0x02000000
 *  will be computed.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressFlashInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;
	
	
	stressFlashXor_G = stressFlashCalcExor( hP, STRESS_FLASH_MODE_LONG_FORWARD_INITIAL );

	printf( "%s: %s FLASH addr/size %08x/%08x xor %08x\n", __FUNCTION__, hP->name, FLASH_BASE_ADRS, MAIN_FLASH_SIZE, stressFlashXor_G );

	hP->bytenum = 0;
	
	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "Flash Init done;");
	}
}

/**********************************************************************/
/** Routine for FLASH memory stress test task.
 *
 *  The XOR over boot FLASH at 0xee000000 size 0x02000000
 *  will be computed and compared with the initial one.
 *  Addresses will be counter once forward and once backward.
 *  Boot FLASH ist not cached and guarded always.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressFlash( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 xor;
	int 	mode = -1;

	while( h->enabled )
	{
		/* recalculate delay timing */
		if( h->load != load )
		{
			load = h->load;
			if( load == 99 )
			{
				delay = 1;
			}
			else
			{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			/*memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
			sprintf( h->dbgP->buf, "(flash) calc prim 0x%x hdr=0x%x len=%d;", 
					(u_int32*)EP06_PMENMON_OFFS, &phdr->length, phdr->length );
			stressWrtDbg( h->dbgP, h->dbgP->buf );*/
		}

		
		tick = tickGet();

		/********************/
		/*  FLASH           */
		/********************/
		
	    /*  */
	    switch( mode )
	    {
	    	case STRESS_FLASH_MODE_LONG_BACKWARD_CACHED:
		    	mode = STRESS_FLASH_MODE_LONG_BACKWARD;
	    		break;

	    	case STRESS_FLASH_MODE_LONG_FORWARD_CACHED:
		    	mode = STRESS_FLASH_MODE_LONG_BACKWARD_CACHED;
	    		break;

	    	case STRESS_FLASH_MODE_LONG_FORWARD:
		    	mode = STRESS_FLASH_MODE_LONG_FORWARD_CACHED;
	    		break;

	    	case STRESS_FLASH_MODE_LONG_BACKWARD:
	    	default:
	    		mode = STRESS_FLASH_MODE_LONG_FORWARD;
	    		break;
	    }
	    
	    xor = stressFlashCalcExor( h, mode );
		if( xor != stressFlashXor_G )
		{

			if( h->verbose & STRESS_DBGLEV_1 )
			{
				memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
				sprintf( h->dbgP->buf, "*** (FLASH) verify %08x/%08x ***;", xor, stressFlashXor_G );
				stressWrtDbg( h->dbgP, h->dbgP->buf );
			}

			h->errCnt++;
		}
		else
		{
			if( h->bytenum <= 0 )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTime = tickGet();
			}
		}
		

		if( h->verbose & STRESS_DBGLEV_2 )
		{
			stressWrtDbg( hP->dbgP, "flash alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}


	printf( "\n>>> %s end after %d loops errCnt %d\n", h->name, h->loopCnt, h->errCnt );

	h->isFinished = 1; /* say finished to shutting down */

}

/**********************************************************************/
/** Compute checksum over 16MB of BOOT FLASH in the following modes
 *  forward long unchached and backward long unchached
 *  \return XOR checksum
 */
u_int32 stressFlashCalcExor( STRESS_TEST_HDL *h, int mode )
{
	u_int32 *p		= NULL;
	u_int32 size	= MAIN_FLASH_SIZE;
	u_int32 chkSum	= 0;

    size >>= 2;	/* long access */

	switch( mode )
	{
		case STRESS_FLASH_MODE_LONG_BACKWARD_CACHED:
			p 	 = (u_int32 *)FLASH_BASE_ADRS + size;
			p--;
			break;

		case STRESS_FLASH_MODE_LONG_BACKWARD:
			p 	 = (u_int32 *)FLASH_BASE_ADRS + size;
			p--;
			break;

		case STRESS_FLASH_MODE_LONG_FORWARD_CACHED:
			p 	 = (u_int32 *)FLASH_BASE_ADRS;
			break;

		case STRESS_FLASH_MODE_LONG_FORWARD_INITIAL:
		case STRESS_FLASH_MODE_LONG_FORWARD:
		default:
			p 	 = (u_int32 *)FLASH_BASE_ADRS;
			break;		
	}/*switch*/


	
    while( size )
    {

		if( mode == STRESS_FLASH_MODE_LONG_FORWARD 
			|| mode == STRESS_FLASH_MODE_LONG_FORWARD_CACHED
			|| mode == STRESS_FLASH_MODE_LONG_FORWARD_INITIAL
		  )
		{
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
		}
		else
		{
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
		}

		size -= 8;
		
		if( mode != STRESS_FLASH_MODE_LONG_FORWARD_INITIAL && !(size%0x100) )
		{
			h->bytenum += (0x100 << 2); /* long access */
			taskDelay(2);
			if( !h->enabled )
			{
				chkSum = stressFlashXor_G; /* cover stop */
				break;
			}
		}
	}

	return( chkSum );
}

#if 0
/**********************************************************************/
/** Compute checksum over 256MB of APP FLASH in the following modes
 *     forward long unchached
 */
u_int32 stressAppFlashCalcExor( STRESS_TEST_HDL *h, int mode )
{
	u_int32 *p		= NULL;
	u_int32 size	= APPL_FLASH_SIZE/256; /* TODO all size */
	u_int32 chkSum	= 0;

    size >>= 2;	/* long access */

	switch( mode )
	{
		case STRESS_FLASH_MODE_LONG_FORWARD_CACHED:
		default:
			p 	 = (u_int32 *)APPL_FLASH_BASE_ADRS;
			break;		
	}/*switch*/

#ifndef _SINGLE_ACCESS
 	/* switch FLASH to burst mode if read ID before
 	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_ADDR = 0x000020CF; msync
	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_DATA = 0x00600060; msync

 	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_ADDR = 0x000020CF; msync
	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_DATA = 0x00030003; msync

 	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_ADDR = 0x00000000; msync
	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_DATA = 0xFFFFFFFF; msync
	*/

    while( size )
    {
		if( mode == STRESS_FLASH_MODE_LONG_FORWARD 
			|| mode == STRESS_FLASH_MODE_LONG_FORWARD_CACHED
			|| mode == STRESS_FLASH_MODE_LONG_FORWARD_INITIAL
		  )
		{
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
			chkSum ^= *p++;
		}
		else
		{
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
			chkSum ^= *p--;
		}

		size -= 8;
		
		if( mode != STRESS_FLASH_MODE_LONG_FORWARD_INITIAL && !(size%0x100) )
		{
			h->bytenum += (0x100 << 2); /* long access */
			taskDelay(2);
			if( !h->enabled )
			{
				chkSum = stressAppFlashXor_G; /* cover stop */
				break;
			}
		}
	}/*while*/
#else
	/* index to 0 */
 	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_ADDR = 0x00000000; __asm("mbar 0");
	*p = APPL_FLASH_SINGLE_BASE_ADRS_DATA;

    while( size )
    {
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;
		chkSum ^= *p;

		size -= 8;
		
		if( mode != STRESS_FLASH_MODE_LONG_FORWARD_INITIAL && !(size%0x100) )
		{
			h->bytenum += (0x100 << 2); /* long access */
			taskDelay(2);
			if( !h->enabled )
			{
				chkSum = stressAppFlashXor_G; /* cover stop */
				break;
			}
		}
	}/*while*/
#endif /*_BURST_ACCESS */

	

	return( chkSum );
}

/**********************************************************************/
/** This routine reads the APP FLASH ID in single access mode.
 *  The local bus frequency will not be changed.
 *  At first the read ID commands 0x00000000 and 0x00900090 will be
 *  written. after that the ID will be read from FLASH.
 *  
 */
u_int32 stressAppFlashId( void )
{
	u_int32 id;

	__asm("sync");
	__asm("isync");
 	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_ADDR = 0x00000000; __asm("eieio");
	*(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_DATA = 0x00900090; __asm("eieio");
	id = *(u_int32*)APPL_FLASH_SINGLE_BASE_ADRS_DATA; __asm("eieio");
	
	return( id );
}

/**********************************************************************/
/** Routine to init application FLASH stress test.
 *
 *  The application FLASH ID will be read.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressAppFlashInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;
	u_int32 id = 0xdeadbeef;

	/* check FLASH ID single access */
	id = stressAppFlashId();
	
	/* calc XOR */
/*	stressAppFlashXor_G = stressAppFlashCalcExor( hP, STRESS_FLASH_MODE_LONG_FORWARD_INITIAL ); */

	printf( "%s: %s APP FLASH id %08x addr/size %08x/%08x xor %08x\n", __FUNCTION__, hP->name,
	        id, APPL_FLASH_BASE_ADRS, APPL_FLASH_SIZE/256 /*TODO*/, stressAppFlashXor_G );

	hP->bytenum = 0;
	
	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "APP FLASH Init done;");
	}
}

/**********************************************************************/
/** Routine for application FLASH memory stress test task.
 *
 *  The test will be done by cyclic calling of stressAppFlashId()
 *  and checking the ID 0x00890089.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressAppFlash( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 xor;
	int 	mode = -1;
	u_int32 id;
	int		i;

	while( h->enabled )
	{
		/* recalculate delay timing */
		if( h->load != load )
		{
			load = h->load;
			if( load == 99 )
			{
				delay = 1;
			}
			else
			{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			/*memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
			sprintf( h->dbgP->buf, "(flash) calc prim 0x%x hdr=0x%x len=%d;", 
					(u_int32*)EP06_PMENMON_OFFS, &phdr->length, phdr->length );
			stressWrtDbg( h->dbgP, h->dbgP->buf );*/
		}

		
		tick = tickGet();

		/********************/
		/*  FLASH           */
		/********************/
		
	    /*  */
	    switch( mode )
	    {
	    	default:
	    		mode = STRESS_FLASH_MODE_LONG_FORWARD_CACHED; /* forward cached always */
	    		break;
	    }
	    
	    taskDelay( sysClkRateGet() / 2 ); /* TODO */

	    
#if 1
		/* check FLASH ID single access */
	    for( i=0; i<1000; i++ )
	    {
		    id = stressAppFlashId();
			h->bytenum += 20;
			if( id != 0x00890089 )
			{
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
					sprintf( h->dbgP->buf, "*** (APP FLASH) id %08x/%08x ***;", id, 0x00890089 );
					stressWrtDbg( h->dbgP, h->dbgP->buf );
				}
	
				h->errCnt++;
			}
			else
			{
				if( h->bytenum <= 0 )
				{
					/* overflow occurred, clear startime to fix Xfer-Rate */
					h->startTime = tickGet();
				}
			}
		}/*for*/
#else	
		/* check XOR */    
	    xor = stressAppFlashCalcExor( h, mode );
		if( xor != stressAppFlashXor_G )
		{

			if( h->verbose & STRESS_DBGLEV_1 )
			{
				memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
				sprintf( h->dbgP->buf, "*** (APP FLASH) verify %08x/%08x ***;", xor, stressAppFlashXor_G );
				stressWrtDbg( h->dbgP, h->dbgP->buf );
			}

			h->errCnt++;
		}
		else
		{
			if( h->bytenum <= 0 )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTime = tickGet();
			}
		}
#endif		

		if( h->verbose & STRESS_DBGLEV_2 )
		{
			stressWrtDbg( hP->dbgP, "APPFL alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}


	printf( "\n>>> %s end after %d loops errCnt %d\n", h->name, h->loopCnt, h->errCnt );

	h->isFinished = 1; /* say finished to shutting down */

}
#endif



/**********************************************************************/
/** Read SPI ID on chip select 0..3.
 * 
 */
u_int32 stressSpiFlashId
(
	int cs /* CS 0..3 */
)
{
	u_int32 id = 0xdeadbeef;

	/* Flash Manufacturer ID    -> 0x20
	   Flash Device ID          -> 0x2016
	   Flash Extended Device ID -> 0x1000
	*/

	id = spiFlashDevIdGet( cs );
	
	return( id );
}


/**********************************************************************/
/** Routine to init SPI FLASH stress test.
 *
 *  stressSpiFlashId() will be called for cs = 0..3.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressSpiInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;
	
	
	stressSpiId_G = stressSpiFlashId( 0 );

	/* TODO - switch to SPI */

	printf( "%s: %s SPI CS0 id %08x\n", __FUNCTION__, hP->name, stressSpiId_G );
	printf( "%s: %s SPI CS1 id %08x\n", __FUNCTION__, hP->name, stressSpiFlashId( 1 ) );
	printf( "%s: %s SPI CS2 id %08x\n", __FUNCTION__, hP->name, stressSpiFlashId( 2 ) );
	printf( "%s: %s SPI CS3 id %08x\n", __FUNCTION__, hP->name, stressSpiFlashId( 3 ) );
	/*printf( "%s: %s SPI addr/size %08x/%08x xor %08x\n", __FUNCTION__, hP->name, FLASH_BASE_ADRS, MAIN_FLASH_SIZE, stressFlashXor_G );*/

	hP->bytenum = 0;
	
	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "SPI Init done;");
	}
}

/**********************************************************************/
/** Routine for SPI FLASH memory stress test task.
 *
 *  The test will be done by cyclic calling of stressSpiFlashId()
 *  and checking the ID.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressSpi( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 id;
	u_int32 cs;

	while( h->enabled )
	{
		/* recalculate delay timing */
		if( h->load != load )
		{
			load = h->load;
			if( load == 99 )
			{
				delay = 1;
			}
			else
			{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			/*memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
			sprintf( h->dbgP->buf, "(flash) calc prim 0x%x hdr=0x%x len=%d;", 
					(u_int32*)EP06_PMENMON_OFFS, &phdr->length, phdr->length );
			stressWrtDbg( h->dbgP, h->dbgP->buf );*/
		}

		
		tick = tickGet();

		/********************/
		/*  SPI             */
		/********************/
		
		if( !(h->loopCnt%1000) )
		{
	    	taskDelay( 1 ); /* TODO */
	    }

		for( cs=0; cs<4; cs++ )
		{
			id = stressSpiFlashId( cs );
			h->bytenum+=30;
			if( id != stressSpiId_G )
			{
	
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
					sprintf( h->dbgP->buf, "*** (SPI) verify %08x/%08x ***;", id, stressSpiId_G );
					stressWrtDbg( h->dbgP, h->dbgP->buf );
				}
	
				h->errCnt++;
			}
			else
			{
				if( h->bytenum <= 0 )
				{
					/* overflow occurred, clear startime to fix Xfer-Rate */
					h->startTime = tickGet();
				}
			}
		}/*for*/
		

		if( h->verbose & STRESS_DBGLEV_2 )
		{
			stressWrtDbg( hP->dbgP, "SPI alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}


	printf( "\n>>> %s end after %d loops errCnt %d\n", h->name, h->loopCnt, h->errCnt );

	h->isFinished = 1; /* say finished to shutting down */

}

/**********************************************************************/
/** Routine to init NVSRAM stress test.
 *
 *  The NVSRAM at 0xf1000000 with 0x0020000 byte will be tested.
 *
 *  \param h   pointer to STRESS_TEST_HDL
 *  \param p   parameter (not used)
 *
 */
static void stressNvsramInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;
	
	
	/* NVSRAM */
	printf( "%s: %s NVSRAM addr/size %08x/%08x\n", __FUNCTION__, hP->name, NV_SRAM_BASE_ADRS, NV_SRAM_SIZE );

	hP->bytenum = 0;
	
	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "NVSRAM Init done;");
	}
}

/**********************************************************************/
/** Routine for NVSRAM memory stress test task.
 *  
 *  Four different linear patterns will be written to the IC and
 *  read back and checked. The test don't calls software store 
 *  command.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressNvsram( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 errCnt = 0;

	while( h->enabled )
	{
		/* recalculate delay timing */
		if( h->load != load )
		{
			load = h->load;
			if( load == 99 )
			{
				delay = 1;
			}
			else
			{
				delay = (100 - load) * sysClkRateGet() / 100;
			}
		}

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			/*memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
			sprintf( h->dbgP->buf, "(flash) calc prim 0x%x hdr=0x%x len=%d;", 
					(u_int32*)EP06_PMENMON_OFFS, &phdr->length, phdr->length );
			stressWrtDbg( h->dbgP, h->dbgP->buf );*/
		}

		
		tick = tickGet();

		/********************/
		/*  NVSRAM          */
		/********************/
		
		nvsram_write_pattern( h->loopCnt%3, 0 /* quite */ );
		h->bytenum+=NV_SRAM_SIZE;

		errCnt = nvsram_check_pattern( h->loopCnt%3, 0 /* quite */ );
		h->bytenum+=NV_SRAM_SIZE;
		if( errCnt)
		{

			if( h->verbose & STRESS_DBGLEV_1 )
			{
				memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
				sprintf( h->dbgP->buf, "*** (NVSRAM) errCnt ***;", errCnt );
				stressWrtDbg( h->dbgP, h->dbgP->buf );
			}

			h->errCnt++;
		}
		else
		{
			if( h->bytenum <= 0 )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTime = tickGet();
			}
		}
		

		if( h->verbose & STRESS_DBGLEV_2 )
		{
			stressWrtDbg( hP->dbgP, "NVSRAM alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}


	printf( "\n>>> %s end after %d loops errCnt %d\n", h->name, h->loopCnt, h->errCnt );

	h->isFinished = 1; /* say finished to shutting down */

}


#endif
