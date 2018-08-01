/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest_net.c
 *
 *      \author  René Lange/uf
 *        $Date: 2012/04/13 09:37:12 $
 *    $Revision: 1.1 $
 *
 *        \brief  test routines for SC23 stress testbench
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_net.c,v $
 * Revision 1.1  2012/04/13 09:37:12  MKolpak
 * Initial Revision
 *
 * cloned from EP6 Revision 1.1  2011/02/25 09:38:12  rlange
 *---------------------------------------------------------------------------
 * (c) Copyright 2010..2011 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

/*--------------------------------------------------------------+
 |	INCLUDE														|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	EXTERNALS													|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	DEFINES 													|
 +-------------------------------------------------------------*/
#define STRESS_NET_IFNAME "dtsec"
#define STRESS_NET_IFUNIT 2

/*--------------------------------------------------------------+
 |	GLOBALS														|
 +-------------------------------------------------------------*/
static u_int32	* byte;

/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/
void lfCallback(u_int32 );

/*--------------------------------------------------------------+
 |	INCLUDES  													|
 +-------------------------------------------------------------*/



/**********************************************************************/
/** Routine to init Ethernet stress test environment.
 *
 *  Print out the file name.
 *  The file name will be computed by stresstest_sc23_ + serial _number
 *  + test id. For each network interface the test has a different ID.
 *  i.e. /NET1/stresstest_sc23_sn1234_id12 with /NET1 as NFS mounted
 *  directory.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param p   	    parameter
 *
 */
void stressEtherInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;

	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "Ethernet Init done;");
	}
}


void stressEthernet( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	int creat_tmp = 0;
	u_int32 i = 0;
	u_int8 k =0;
	int fh = -1;

	taskDelay( sysClkRateGet() + 5 * h->id );
	h->startTick= tickGet();
	byte = &h->bytenum;

	LfInit(STRESS_NET_IFNAME,STRESS_NET_IFUNIT);

	while( h->enabled )
	{

#ifdef _WRS_CONFIG_SMP                
		h->cpu = vxCpuIndexGet();
#endif

#if 0	
		/* recalculate delay timing */
		if( h->load_act != load )
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
#endif
		/* were alive! */
		h->alive = 1;

		delay = 1;

		k = h->loopCnt + h->id;

		/**********************/
		/* Run test           */
		/**********************/
		if( Loopframes(
				STRESS_NET_IFNAME,
				STRESS_NET_IFUNIT,
				5,
				0,
				0,
				0,
				lfCallback
				) != OK )
		{
			h->errCnt++;
		}

		/* taskDelay( delay );*/

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}

	h->isFinished = 1;

	return;
}

void lfCallback(u_int32 count)
{
	*byte = count;
	/* stressDbg(NULL, STRESS_DBGLEV_3,"%s byte:%u","NET ", byte);*/
}


#if 0
/**********************************************************************/
/** Routine to test Ethernet interface for stress test.
 *
 *  The test cyclic writes a file with different linear patterns and 
 *  read it back and check it.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of test task
 *  \param netId   	Network ID
 *
 */
void stressEthernet( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	char filename[0x80];
	int creat_tmp = 0;
	char *tmpP = NULL;
	char *verP = NULL;
	u_int32 i = 0;
	u_int8 k =0;
	int fh = -1;

	taskDelay( sysClkRateGet() + 5 * h->id );
	
	filename[0] = 0;
	sprintf( filename, "/%s/%s_sn%d_id%d", h->name, stressFileName, sysMenSerialNumberGet(), h->id );

	if( (tmpP = (char*)malloc(STRESS_NET_FRAME_SIZE)) == NULL )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			stressWrtDbg( h->dbgP, "*** (net) couldn't allocate memory ***;");
		}
		h->errCnt++;
		goto CLEAN_UP;
	}

	if( (verP = (char*)malloc(STRESS_NET_FRAME_SIZE)) == NULL )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			stressWrtDbg( h->dbgP, "*** (net) couldn't allocate memory ***;");
		}
		h->errCnt++;
		goto CLEAN_UP;
	}
	
	remove(filename);
	if( (fh = open(filename, O_CREAT | O_RDWR, 0 )) == ERROR )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
			sprintf( h->dbgP->buf, "*** %s couldn't open %s;", h->name, filename );
			stressWrtDbg( h->dbgP, h->dbgP->buf );
		}
		h->errCnt++;
		goto CLEAN_UP;
	}
	
	h->startTime = tickGet();

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

		tick = tickGet();

		k = h->loopCnt + h->id;

		/**********************/
		/* Rewind file        */
		/**********************/
		if( lseek( fh, 0, SEEK_SET ) == ERROR )
		{
			if( h->verbose & STRESS_DBGLEV_1 )
			{
				memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
				sprintf( h->dbgP->buf, "*** %s rewind %s;", h->name, filename );
				stressWrtDbg( h->dbgP, h->dbgP->buf );
			}
			h->errCnt++;
			goto CLEAN_UP;
		}

		/**********************/
		/* write file         */
		/**********************/
		for(i = 0; i <= STRESS_NET_FILE_SIZE; i += STRESS_NET_FRAME_SIZE)
		{

			if( (h->bytenum + (STRESS_NET_FRAME_SIZE * 2)) <= (STRESS_NET_FRAME_SIZE * 2) )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTime = tickGet();
			}

			memset( tmpP, k++, STRESS_NET_FRAME_SIZE );

			if( write(fh, tmpP, STRESS_NET_FRAME_SIZE ) == ERROR )
			{
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
					sprintf( h->dbgP->buf, "*** %s write (pos=%d) %s;", h->name, filename, i );
					stressWrtDbg( h->dbgP, h->dbgP->buf );
				}
				h->errCnt++;
			}
			else
			{
				h->bytenum += STRESS_NET_FRAME_SIZE;
			}
		}


		/**********************/
		/* Rewind file        */
		/**********************/
		if( lseek( fh, 0, SEEK_SET ) == ERROR )
		{
			if( h->verbose & STRESS_DBGLEV_1 )
			{
				stressWrtDbg( h->dbgP, "*** (net) rewind file (read) ***;");
			}
			h->errCnt++;
			goto CLEAN_UP;
		}

		k = h->loopCnt + h->id;

		/**********************/
		/* read file          */
		/**********************/
		for(i=0; i <= STRESS_NET_FILE_SIZE; i += STRESS_NET_FRAME_SIZE)
		{
			memset( tmpP, 0x0, STRESS_NET_FRAME_SIZE);
			if( read(fh, tmpP, STRESS_NET_FRAME_SIZE ) == ERROR )
			{
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					memset( h->dbgP->buf, 0x0, h->dbgP->bufSize );
					sprintf( h->dbgP->buf, "*** (net) read error (pos=%d)***;",i);
					stressWrtDbg( h->dbgP, h->dbgP->buf );
				}
				h->errCnt++;
			}
			else
			{
				h->bytenum += STRESS_NET_FRAME_SIZE;
			}

			/* prepare verify buffer */
			memset( verP, k++, STRESS_NET_FRAME_SIZE);

			if( memcmp( tmpP, verP, STRESS_NET_FRAME_SIZE) )
			{
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					stressWrtDbg( h->dbgP, "*** (net) verify failed ***;");
				}
				h->errCnt++;
			}
		}

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			stressWrtDbg( h->dbgP, "net alive;");
		}

		taskDelay( delay );

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}

CLEAN_UP:
	if( tmpP )
	{
		free( tmpP );
	}

	if( verP )
	{
		free( verP );
	}

	if( fh > 0 )
		close(fh);

	printf( "\n>>> %s end after %d loops file %s errCnt %d\n", h->name, h->loopCnt, filename, h->errCnt );

	h->isFinished = 1; /* say finished to shutting down */

	return;
}
#endif
