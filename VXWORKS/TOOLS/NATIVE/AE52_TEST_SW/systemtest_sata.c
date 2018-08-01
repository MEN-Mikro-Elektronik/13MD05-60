/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest_sata.c
 *
 *      \author  René Lange/uf
 *        $Date: 2012/04/13 09:37:10 $
 *    $Revision: 1.1 $
 *
 *        \brief  test routines for SATA stress task
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_sata.c,v $
 * Revision 1.1  2012/04/13 09:37:10  MKolpak
 * Initial Revision
 *
 * cloned from SC20 Revision 1.4  2010/12/15 09:07:42  rlange
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2011 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
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
#define STRESS_SATA_FRAME_SIZE		0x10000
#define STRESS_SATA_FILE_SIZE		STRESS_SATA_FRAME_SIZE*64		/* 512 KBytes */

#define PATTERN_1					0x12345678
#define PATTERN_2					0x456789AB
#define PATTERN_3					0x789ABCDE
#define PATTERN_4					0xF1E2D3C4

#define SATA_FILE_SEEK_RETRY			3

/*--------------------------------------------------------------+
 |	GLOBALS														|
 +-------------------------------------------------------------*/
const char stressSataFileName[4][0x20] =
{
	{"/s0p1:1/stress_sata0"},
	{"/s1p1:1/stress_sata1"},
	{"/s2p1:1/stress_sata2"},
	{ NULL }
};


/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	INCLUDES  													|
 +-------------------------------------------------------------*/

/**********************************************************************/
/** Routine to init SATA stress test environment.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param p   	    parameter
 *
 */
void stressSataInit( void *h, u_int32 p )
{
	STRESS_TEST_HDL *hP = (STRESS_TEST_HDL*)h;

	printf( "%s: %s file: %s\n", __FUNCTION__, hP->name, stressSataFileName[0] );

	if( hP->verbose & STRESS_DBGLEV_3 )
	{
		stressWrtDbg( hP->dbgP, "Sata Init done;");
	}
}

/**********************************************************************/
/** Routine for SATA stress task.
 *
 *  \param hP   pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressSata( STRESS_TEST_HDL *hP, u_int32 load, u_int8 inst )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 tickDiff = 0;
	int sata_id = 0;
	u_int32 i = 0;
	unsigned long pattern = 0;
	char 	filename[0x80];
	int 	creat_tmp = 0;
	u_int32 count = 0;
	u_int32 rCount = 0;
	u_int32 wCount = 0;
	u_int8  sataWrite = 0;
	int		seekErr	= 0;
	int		wrErr	= 0;	
	int		readErr	= 0;
	int		cmpErr	= 0;
	char *G_wrP = NULL;
	char *G_rdP = NULL;

	taskDelay( sysClkRateGet()  + 5 * h->id );

	/* filename */
	switch( inst )
	{
		case 0:
		case 1:
		case 2:
			sprintf(filename, stressSataFileName[inst] );
			break;

		default:
			stressWrtDbg( h->dbgP, "*** (sata) wrong inst ***;");
			goto CLEANUP;
			break;
	}


	/* alloc memory */
	if( (G_wrP = (char*)memalign( 4, STRESS_SATA_FRAME_SIZE)) == NULL )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			stressWrtDbg( h->dbgP, "*** (sata) couldn't allocate memory ***;");
		}
		h->errCnt++;
		goto CLEANUP;
	}

	if( (G_rdP = (char*)memalign( 4, STRESS_SATA_FRAME_SIZE)) == NULL )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			stressWrtDbg( h->dbgP, "*** (sata) couldn't allocate memory ***;");
		}
		h->errCnt++;
		goto CLEANUP;
	}


	/* file creation */
	remove( filename );
	if(( sata_id = open( filename, O_CREAT | O_RDWR, 0) ) == ERROR )
	{
		if( h->verbose & STRESS_DBGLEV_1 )
		{
			stressWrtDbg( h->dbgP, "*** (sata) couldn't open file ***;");
		}
		h->errCnt++;
		goto CLEANUP;
	}

	h->startTick = tickGet();

	/* write/read/compare loop */
	while( h->enabled )
	{
		
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

		tick = tickGet();

		rCount = count;
		wCount = count;


		switch( pattern )
		{
			case PATTERN_3:
				pattern = PATTERN_4;
				break;
			case PATTERN_2:
				pattern = PATTERN_3;
				break;
			case PATTERN_1:
				pattern = PATTERN_2;
				break;
			default:
				pattern = PATTERN_1;
				break;
		}

		/*************************/
		/* Open/Rewind file      */
		/*************************/
		if( lseek( sata_id, 0, SEEK_SET ) == ERROR )
		{
			seekErr++;
			if( h->verbose & STRESS_DBGLEV_1 )
			{
				stressWrtDbg( h->dbgP, "*** (sata) rewind file (wr) ***;");
			}
			h->errCnt++;
			goto CLEANUP;
		}

		/***********************/
		/* Write Loop          */
		/***********************/
		for(i = 0; i <= STRESS_SATA_FILE_SIZE; i += STRESS_SATA_FRAME_SIZE)
		{
			if( (h->bytenum + (STRESS_SATA_FRAME_SIZE)) <= (STRESS_SATA_FRAME_SIZE ) )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTick = tickGet();
			}

			_memsetlong( (long*)G_wrP, pattern, STRESS_SATA_FRAME_SIZE );

			*(u_int32*)G_wrP = wCount++;

			if( write(sata_id, G_wrP, STRESS_SATA_FRAME_SIZE ) != STRESS_SATA_FRAME_SIZE )
			{
				wrErr++;
				stressDbg(NULL,STRESS_DBGLEV_1, "*** (sata) write error (pos=%d) ***;",i);
				h->errCnt++;
			}
			else
			{
				h->bytenum += (STRESS_SATA_FRAME_SIZE);
			}
		}/* write loop */

		/*************************/
		/* Close/Rewind file      */
		/*************************/

		if( lseek( sata_id, 0, SEEK_SET ) == ERROR )
		{
			seekErr++;
			stressDbg(NULL,STRESS_DBGLEV_1, "*** (sata) rewind file (rd) ***;");
			h->errCnt++;
			goto CLEANUP;
		}

		tickDiff = tickGet() - tick;

		taskDelay( delay );

		tick = tickGet();

		/***********************/
		/* Read Loop           */
		/***********************/
		for(i=0; i <= STRESS_SATA_FILE_SIZE; i += STRESS_SATA_FRAME_SIZE)
		{
			if( (h->bytenum + (STRESS_SATA_FRAME_SIZE)) <= (STRESS_SATA_FRAME_SIZE ) )
			{
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTick = tickGet();
			}

			if( read(sata_id, G_rdP, STRESS_SATA_FRAME_SIZE ) != STRESS_SATA_FRAME_SIZE )
			{
				readErr++;
				stressDbg(NULL,STRESS_DBGLEV_1, "*** (sata) read error (pos=%d) ***;",i);
				h->errCnt++;
			}
			else
			{
				h->bytenum += (STRESS_SATA_FRAME_SIZE);
			}

			if( h->verbose & STRESS_DBGLEV_3 )
			{
				stressWrtDbg( h->dbgP, "sata read;");
			}

			/* verify buffer */
			_memsetlong( (long*)G_wrP, pattern, STRESS_SATA_FRAME_SIZE );
			*(u_int32*)G_wrP = rCount++;
			if( memcmp( G_wrP, G_rdP, STRESS_SATA_FRAME_SIZE) )
			{
				cmpErr++;
				if( h->verbose & STRESS_DBGLEV_1 )
				{
					u_int32 errByteCnt = 0;
					u_int32 firstErrByte = 0;
					u_int32 a=0;

					for( a=0; a < STRESS_SATA_FRAME_SIZE; a++ )
					{
						if( G_wrP[a] != G_rdP[a] )
						{
							errByteCnt++;
							firstErrByte = a;
						}
					}

					stressDbg(NULL,STRESS_DBGLEV_1, "*** (sata) verify failed (%d,0x%x!0x%x***;",
						errByteCnt, G_wrP[firstErrByte], G_rdP[firstErrByte]);
				}
				h->errCnt++;
			}
		}/* read loop */

		if( h->verbose & STRESS_DBGLEV_3 )
		{
			stressWrtDbg( h->dbgP, "sata alive;");
		}

		while( h->halt && h->enabled )
		{
			taskDelay(1);
		};

		h->loopCnt++;
	}

CLEANUP:
	if( sata_id > -1 )
		close( sata_id );

	/* remove(filename); */

	if( G_rdP )
	{
		free( G_rdP );
		G_rdP = NULL;
	}

	if( G_wrP )
	{
		free( G_wrP );
		G_wrP = NULL;
	}

	printf( "\n>>> %s end after %d loops file %s errCnt %d - s/w/r/c %d/%d/%d/%d \n", h->name, h->loopCnt, filename, h->errCnt,
			seekErr,	wrErr,	readErr,	cmpErr
	 );



	h->isFinished = 1; /* say finished to shutting down */

	return;
}

