/*********************	P r o g r a m  -  M o d u l e ***********************/
/*!
 *		  \file  systemtest_stress.c
 *
 *		\author  René Lange
 *		  $Date: 2012/09/06 15:32:08 $
 *	  $Revision: 1.2 $
 *
 *		  \brief  stress test routines of SC20 vxWorks
 *
 *	   Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_stress.c,v $
 * Revision 1.2  2012/09/06 15:32:08  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:15  MKolpak
 * Initial Revision
 *
 * Revision 1.7  2011/03/11 10:16:48  RLange
 * R: Cosmetics
 * M: Diverse
 *
 * Revision 1.6  2010/12/15 08:05:59  rlange
 * R: Use of slower SD-Cards instead of USB-Flashes
 * M: Enlarge Stuck time for USB devices 1 and 4 (internal USB -> SD-card)
 *
 * Revision 1.5  2010/03/10 18:06:59  RLange
 * R: Log File problem, not all data are logged
 * M: Wrong pointer given to logging task
 *
 * Revision 1.4  2010/03/09 16:44:09  RLange
 * R: Improvements for Usage and Performance
 * M: WriteLogging is now task
 *	  Changed Masked for intuitive usage
 *	  Display if test is stuck
 *
 * Revision 1.3  2010/02/23 11:40:35  RLange
 * R: Support SpaceStation Environment
 * M: Added ACCU information
 *	  Changed output mask
 *	  Changed refresh behaviour
 *	  Added stuck error display
 *	  Added startup menu
 *
 * Revision 1.2  2010/01/22 13:15:12  RLange
 * R: Support for Testboard
 * M: Adapted calls for voltage calculation
 *	  Bugfix in pic TX routine that causes timeout errors
 *	  Added Display refresh to improve terminal outputs
 *
 * Revision 1.1  2009/12/10 15:30:12  RLange
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2010 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/


/*
 * TODO: - display task tends to lock when started over telnet
 *		   until then the system information (si) updates will be done in the
 *		   stressLogFile task
 */

#define STRESS_CFG_FILE_SIZE			0x200
#define STRESS_CFG_FILENAME_SIZE		30
#define STRESS_TASKNAME_SIZE			30

#define KB_SIZE 				( 1024 )

#define VERSION 	"$Revision: 1.2 $"

/* # positions of display columns # */
/* test summary */
#define STRESS_TAB_NAME 	1
#define STRESS_TAB_LOAD_N	8
#define STRESS_TAB_CPU		16
#define STRESS_TAB_DATA 	24
#define STRESS_TAB_STAT 	40
/* width of the fields - must be changed if rearranged! */
#define STRESS_WIDTH_DELIM	1
#define STRESS_WIDTH_NAME	(STRESS_TAB_LOAD_N - STRESS_TAB_NAME - STRESS_WIDTH_DELIM)
#define STRESS_WIDTH_LOAD_N (STRESS_TAB_CPU - STRESS_TAB_LOAD_N - STRESS_WIDTH_DELIM)
#define STRESS_WIDTH_CPU	(STRESS_TAB_DATA - STRESS_TAB_CPU - STRESS_WIDTH_DELIM)
#define STRESS_WIDTH_DATA	(STRESS_TAB_STAT - STRESS_TAB_DATA - STRESS_WIDTH_DELIM)
#define STRESS_WIDTH_STAT	(STRESS_TAB_SYSINFO - 3 - STRESS_TAB_DATA - STRESS_WIDTH_DELIM)
#define STRESS_WIDTH_SYSINFO (STRESS_TAB_MENU - 3 -STRESS_TAB_SYSINFO- STRESS_WIDTH_DELIM)
/* system info */
#define STRESS_TAB_SYSINFO		48
/*#define STRESS_TAB_SYSINFO_2	60*/
/* menu keys */
#define STRESS_TAB_MENU 		69
/* # positions of display rows # */
#define STRESS_LINE_RESULT		2
#define STRESS_LINE_SYSTIME 	4
#define STRESS_LINE_DBG 		15
#define STRESS_LINE_MENU_QUIT	25

#define STRESS_REFRESH_FREQ_SPY 1

#define STRESS_TASK_PREFIX	"tStb"

enum{
	STRESS_KEY_HALT = ' ',
	STRESS_KEY_CLEAR = '1',
	STRESS_KEY_DEBUG,
	STRESS_KEY_LOAD,
	STRESS_KEY_INFO,
	STRESS_KEY_LOG
};

enum
{
	STRESS_TYPE_RATE,
	STRESS_TYPE_IO
};

/* increment pointer off the blanks */
#define STRESS_BLANK_STEP(p) while(*p==' ') p++;
/* increment pointer until blank */
#define STRESS_CHAR_STEP(p)  while(*p!=' ') p++;

#define MIN(a,b)	min(a,b)
#define MAX(a,b)	max(a,b)


/* null device */
#if 0
int nullIn;
#endif
int nullOut;
int nullErr;


/*--------------------------------------------------------------+
 |	INCLUDES													|
 +-------------------------------------------------------------*/


/*--------------------------------------------------------------+
 |	GLOBALS 													|
 +-------------------------------------------------------------*/
/* table with testsdiscriptors */
static STRESS_TEST_HDL G_stressHdl[] = {
#if 0
	{
		"COM2 ",
		STRESS_TESTID_COM2,
		0,
		0,
		(VOIDFUNCPTR*)stressSerial,
		NULL,
		0,
		2,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"NET  ",
		STRESS_TESTID_NET,
		0,
		0,
		(VOIDFUNCPTR*)stressEthernet,
		&stressEtherInit,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM, /* prio */
		5, /* singleRunTime */
		1, /* cpu */
		STRESS_TYPE_RATE,
		0,0,0,0,0,0,0,0,0,0,0,0
	},
#endif
	{
		"EEP",
		STRESS_TESTID_EEP,
		0,
		0,
		(VOIDFUNCPTR*)stressEeprom,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		2, /* singleRunTime */
		0, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"SER",
		STRESS_TESTID_RS232,
		0,
		0,
		(VOIDFUNCPTR*)stressRS232,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		2, /* singleRunTime */
		0, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"ETH",
		STRESS_TESTID_ETH,
		0,
		0,
		(VOIDFUNCPTR*)stressEth,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		2, /* singleRunTime */
		0, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"SDRAM1",
		STRESS_TESTID_SDRAM,
		0,
		0,
		(VOIDFUNCPTR*)stressSdram,
		&stressSdramInit,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		2, /* singleRunTime */
		1, /* cpu */
		STRESS_TYPE_RATE,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"AE52RAM",
		STRESS_TESTID_SDRAM2,
		0,
		0,
		(VOIDFUNCPTR*)stressAE52Sdram,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		1, /* singleRunTime */
		2, /* cpu */
		STRESS_TYPE_RATE,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"LRU58",
		STRESS_TESTID_LRU58,
		0,
		0,
		(VOIDFUNCPTR*)stressLRU,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		1, /* singleRunTime */
		3, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"SCL",
		STRESS_TESTID_SCL,
		0,
		0,
		(VOIDFUNCPTR*)stressSCL,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_MEDIUM,
		1, /* singleRunTime */
		4, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"AE52IO",
		STRESS_TESTID_GPIO,
		0,
		0,
		(VOIDFUNCPTR*)stressAE52IO,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_LOW,
		1, /* singleRunTime */
		5, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"AE52API",
		STRESS_TESTID_API,
		0,
		0,
		(VOIDFUNCPTR*)stressAPI,
		0,
		100,
		0,
		STRESS_TASK_PRIORITY_LOW,
		1, /* singleRunTime */
		5, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	#if 0
	{
		"MIL",
		STRESS_TESTID_MIL,
		0,
		0,
		(VOIDFUNCPTR*)stressMIL,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_HIGH,
		12, /* singleRunTime */
		-1, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	#endif
	{
		"MILRAM",
		STRESS_TESTID_MIL,
		0,
		0,
		(VOIDFUNCPTR*)stressMILRAM,
		0,
		50,
		6,
		STRESS_TASK_PRIORITY_MEDIUM,
		5, /* singleRunTime */
		6, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"AE52SSD",
		STRESS_TESTID_SATA,
		0,
		0,
		(VOIDFUNCPTR*)stressSata,
		0,
		100,
		0,
		STRESS_TASK_PRIORITY_LOW,
		2, /* singleRunTime */
		5, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	#if 0
	{
		"LOAD7",
		STRESS_TESTID_LOAD7,
		0,
		0,
		(VOIDFUNCPTR*)stressLoad,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_LOWEST,
		1, /* singleRunTime */
		6, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"LOAD8",
		STRESS_TESTID_LOAD8,
		0,
		0,
		(VOIDFUNCPTR*)stressLoad,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_LOWEST,
		1, /* singleRunTime */
		7, /* cpu */
		STRESS_TYPE_IO,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	#endif
	{
		"DMY",
		STRESS_TESTID_DUMMY,
		0,
		0,
		(VOIDFUNCPTR*)stressDummy,
		0,
		50,
		0,
		STRESS_TASK_PRIORITY_HIGH,
		1, /* singleRunTime */
		7, /* cpu */
		STRESS_TYPE_RATE,
		0,0,0,0,0,0,0,0,0,0,0,0,0
	},
	{
		"END OF TABLE",
		STRESS_TESTID_ENDTBL,
		0,
		0,
		NULL,
		NULL,
		0,
		0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0
	}
};
static u_int32 G_IdlePercent = 0;
static u_int32 G_IntPercent = 0;
static u_int32 G_StressstartTick = 0;
static u_int32 G_debugChanged = 0;
static STRESS_DBG_HDL * G_StressDbg = NULL;
#define STRESS_SPY_BUFSIZE	0x2000
static char 	G_spyBuffer[STRESS_SPY_BUFSIZE] = {0};


/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/
static void stressStop( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picHdlP, STRESS_LOG_HDL *logP );
static void stressInitFrame( void );
static void stressInitWindowScreen( void );
static void stressDisplay( STRESS_TEST_HDL *h, STRESS_PIC_HDL *picP, STRESS_LOG_HDL *logP );
static void stressLogFile(STRESS_TEST_HDL *h,STRESS_PIC_HDL *dummy,STRESS_LOG_HDL *logP);
static void stressEditLoad( STRESS_TEST_HDL *hP );
static void stressPrintTbl( STRESS_TEST_HDL *h, u_int8 focus, char *load_nom );
static void stressChangeLoad( STRESS_TEST_HDL *hP, u_int8 focus, signed char diff );
static void stressChangeFocus( STRESS_TEST_HDL *hP, u_int8 *focus, u_int8 next );
static void stressEditWindowScreen( void );
static void stressDisplayBluetooth( STRESS_TEST_HDL *hP, u_int8 clear );
static void stressDisplayTestEnable( STRESS_TEST_HDL *hP, int line, int col, u_int8 inv_mode );
static int stressCheckInput( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picHdlP, STRESS_LOG_HDL *logP );
static void stressStartTest( STRESS_TEST_HDL *hp);
static void stressStopTest( STRESS_TEST_HDL *hP );
static char* calcDuration( u_int8 *start, u_int8 *end, char *buf );
static int	double2HR(double d, char * cP);
static STRESS_TEST_HDL* stressGetTaskHdlP( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picHdlP );
static void stressDisplayTaskHdl( STRESS_TEST_HDL *hP );
static void stressInitTaskInfoFrame( void );
static void stressIdle( void );
static void stressShowInputBox( u_int32 *value );
static void stressDisplayDebug( STRESS_DBG_HDL *dP );
/*static void stressShowAccuInfo( SC20_ACCU_INFO_STRUCT *infoP );*/
static void stressInitRuntime( STRESS_TEST_HDL * hP);
static void stressTemp1Update( int8 *temp );
static void stressTemp2Update( int8 *temp );
static void stressTemp3Update( int8 *temp );
static u_int32 getDecLong(u_int32 def, int col, int line);
static void stressLogSetIntvl(STRESS_LOG_HDL * logHdlP, u_int32 intvl);
static int stressSpyParse(const char * fmt, ...);
static int stressSpyPrint(const char * fmt, ...);




/**********************************************************************/
/** Routine to initialize test descriptors.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 *	\param *focus		pointer to focus of aktive task
 *	\param next 		next flag
 */
static void stressInitRuntime( STRESS_TEST_HDL * hP)
{ 
	hP->bytenum = 0;		/* < byte # */
	hP->bytenumSave = 0;	/* < stored byte # */
	hP->stuck = 0;			/* < test stuck flag */
	hP->startTick = 0;		/* < start system tick */
	hP->tickSave = 0;		/* < tick for xferrate calc */
	hP->allExeTime = 0; 	/* < over all execution time */
	hP->errCnt = 0; 		/* < number of errors occurred */
	hP->errCntSave = 0; 	/* < old number of errors occurred */
	hP->verbose = 0;		/* < verbose(debug) level 0..3 */
	hP->halt = 0;			/* < lock flag */
	hP->alive = 0;			/* < alive? */	
	hP->focus = 0;			/* <  */
	hP->dataIdx = 0;		/* < number of bytes in buffer */
	hP->dbgP = NULL;		/* < pointer to debug handle */
}


/**********************************************************************/
/** Routine to change focus.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 *	\param *focus		pointer to focus of aktive task
 *	\param next 		next flag
 */
static void stressChangeFocus( STRESS_TEST_HDL *hP, u_int8 *focus, u_int8 next )
{
	STRESS_TEST_HDL *h = hP;

	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			if( *focus == h->id ){
				if( next ){
					h++;
					if( h->id == STRESS_TESTID_ENDTBL ){
						/* end of table reached */
						return;
					}

					/* search up for next */
					do{
						if( h->enabled ){
							*focus = h->id;
							/* next found */
							return;
						}
						h++;
					} while( h->id < STRESS_TESTID_ENDTBL );
					/* No next found */
					return;
				}
				else{
					if( h->id == G_stressHdl[0].id ){
						/* beginning of table reached */
						return;
					}
					h--;
					/* search down for next */
					do{
						if( h->enabled ){
							*focus = h->id;
							/* next found */
							return;
						}

						h--;
					} while( h->id != G_stressHdl[0].id );
					/* No next found */
					return;
				}
			}
		}
		h++;
	}
}

/**********************************************************************/
/** Routine to change task load.
 *
 *	\param h			pointer to STRESS_TEST_HDL
 *	\param focus		focus to aktive task id
 *	\param diff 		change flag for unit 1
 *	\param moremore 	change flag for unit 10
 */
static void stressChangeLoad( STRESS_TEST_HDL *hP, u_int8 focus, signed char diff )
{
	STRESS_TEST_HDL *h = hP;

	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			if( focus == h->id )
			{
				h->load_nom += diff;
				h->load_nom = MIN(h->load_nom, 100);
				h->load_nom = MAX(h->load_nom, 0);
				return;
			}
		}
		h++;
	}
}

/**********************************************************************/
/** Routine to display task table with focus.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 *	\param focus		focus to aktive task id
 *	\param load 		cpu load string
 */
static void stressPrintTbl( STRESS_TEST_HDL *hP, u_int8 focus, char *load_nom )
{
	int line = 0;
	int i, j = 0;
	STRESS_TEST_HDL *h = hP;

	tu_clear_wdw(WDW_MSG);

	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			/* fill load string with blanks */
			memset( load_nom, 0x20, 50);

			/* fill load string */
			for( i = 0, j = 0 ; i < h->load_nom; i += 2, j++ ){
				load_nom[j] = '*';
			}

			/* print task name */
			if( focus == h->id ){
				tu_print_revers(WDW_MSG, 0, line++, " %-6s   %-50s (%d%%)", h->name, load_nom, h->load_nom );
			}
			else{
				tu_print(WDW_MSG, 0, line++, " %-6s   %-50s (%d%%)", h->name, load_nom, h->load_nom );
			}
		}
		h++;
	}
}

/**********************************************************************/
/** Routine to change the stress test task load.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 */
#define STARBAR_SIZE 51
static void stressEditLoad( STRESS_TEST_HDL *hP )
{
	STRESS_TEST_HDL *h = hP;
	int line = 0;
	u_int32 i,j = 0;
	char stars[STARBAR_SIZE];
	u_int8 focus = 0;
	int userInput = 0;
	int calc = 0;

	/* Empty the string */
	memset(stars,0,STARBAR_SIZE);

	taskLock();

	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			/* unlock task */
			h->halt = 1;
		}
		h++;
	}

	stressEditWindowScreen();

	h = hP;

	tu_print(WDW_MSG, 0, line++, " Task     Min Load                                  Max Load ");
	tu_print(WDW_MSG, 0, line++, " -----------------------------------------------------------------");
	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			if( !focus ){
				/* set focus to first entry */
				focus = h->id;
			}
			
			/* fill load string */
			for( i = 0, j = 0 ; i < h->load_nom; i += 2, j++ ){
				stars[j] = '*';
			}
			stars[j] = 0;

			/* print task name */
			if( focus == h->id ){
				tu_print_revers(WDW_MSG, 0, line++, " %-8s   %-50s (%d%%)", h->name, stars, h->load_nom );
			}
			else{
				tu_print(WDW_MSG, 0, line++, " %-8s   %-50s (%d%%)", h->name, stars, h->load_nom );
			}

		}
		h++;
	}

	h = hP;

	/* TODO: this is circuitous - change directly...no need for focus*/
	do{
		stressPrintTbl( h, focus, stars );
		userInput = UOS_KeyWait();
		switch( userInput ){
			case '+':
				stressChangeLoad( h, focus, 10 );
				break;
				
			case '-':
				stressChangeLoad( h, focus, -10);
				break;

			case 'd':
				stressChangeFocus( h, &focus, 1 );
				break;

			case 'u':
				stressChangeFocus( h, &focus, 0 );
				break;
		}
	} while( userInput != 'q' );

	tu_exit();
	h = hP;

	while( h->id < STRESS_TESTID_ENDTBL ){
		if( h->enabled ){
			/* reset handle */
			h->errCnt = 0;
			h->startTick = tickGet();
			h->bytenum = 0;
			h->allExeTime = 0;

			/* unlock task */
			h->halt = 0;
		}
		h++;
	}

	taskUnlock();
}


/**********************************************************************/
/** Routine to display stress test results.
 *
 *	\param h		pointer to STRESS_TEST_HDL
 *	\param picP 	pointer to STRESS_PIC_HDL
 *
 */

/*	TODO: all window refresh timeouts - with modulo operations -> roll-over secure! */
/*	refine and clean this */

#define REFRESH_TO_SYS_INF_S	1
#define REFRESH_TO_DEBUG_S		1
#define REFRESH_TO_PIC_S		5
#define REFRESH_TO_WIN_S		60

static void stressDisplay(
	STRESS_TEST_HDL *h,
	STRESS_PIC_HDL *picP,
	STRESS_LOG_HDL *logP
)
{
	STRESS_TEST_HDL *hP = h;
	STRESS_DBG_HDL *dbgP = hP->dbgP;
	double xfer = 0;
	u_int32 timediff = 0, bytediff = 0;
	u_int32 sysLoad = 0;
	int line = 0, ret = 0;
	u_int8 abortFlag = 0;
	u_int32 cycleCnt = 0;
	u_int32 syssec = 0;
	u_int32 i = 0;
	char buf[40];
	char fmt[40];
	u_int8 updateLoad = 0;
	u_int32 errCycle = 0;
	u_int32 forceDisplay = 0;
	u_int32 checkStuck = 0;
	u_int32 ms_per_tick = 1000/sysClkRateGet();
#if 0
	/* set StdIo descriptors */
	if(ret = ioTaskStdSet(taskIdSelf(),STD_IN, ioGlobalStdGet(STD_IN)))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set display STD_IN(%d)\n",errnoGet());
	if(ret = ioTaskStdSet(taskIdSelf(),STD_OUT, ioGlobalStdGet(STD_OUT)))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set display STD_OUT(%d)\n",errnoGet());

	if(ret = ioTaskStdSet(taskIdSelf(),STD_ERR, ioGlobalStdGet(STD_ERR)))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set display STD_ERR(%d)\n",errnoGet());
#endif

#ifdef STRESS_DISPLAY
	stressInitWindowScreen();
	stressInitFrame();
	stressDisplayTestEnable( h, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
#endif	

	/* get start time/date */
	stressTimeUpdate( &logP->start_time );
	stress_si_update(logP);


	/* main loop */
	while(!abortFlag)
	{
		errCycle = 0;
		cycleCnt++;
		updateLoad = 0;
		forceDisplay = 0;

#ifdef STRESS_DISPLAY		
		/* # refresh debug window */
		if(G_debugChanged)
		{
			G_debugChanged = 0;
			stressDisplayDebug( dbgP );
		}
#endif

		/* only once a second */
		if(syssec != tickGet()/sysClkRateGet())
		{
			syssec = tickGet()/sysClkRateGet();

			/* # refresh system information */
			if( !(syssec%REFRESH_TO_SYS_INF_S))
			{
				stressTemp1Update( &logP->temp1);
				stressTemp2Update( &logP->temp2);
				stressTemp3Update( &logP->temp3);
				stressLoadUpdate( &logP->load, h);
				ml605_sysmon_voltages_read(); 
			}
			/* # refresh main window */
#ifdef STRESS_DISPLAY			
			if( !(syssec%REFRESH_TO_WIN_S) )
			{
				if( !logP->verbose )
				{
					tu_clear_wdw( WDW_RESULT );
					tu_clear_wdw( WDW_MSG );
					tu_clear_wdw(WDW_KEYS);
					stressInitFrame();
				}
				/* restart spy to make it more responsive */
				spyClkStopCommon();
				stressSpyInit();
				
				stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
				forceDisplay = 1;
			}

			/* # refresh system information */
			if(!(syssec%REFRESH_TO_SYS_INF_S) || forceDisplay )
			{
				forceDisplay = 0;
				/* # Display System information */
				line = STRESS_LINE_RESULT;
				i = 0;
				while(G_stressSiList[i].func != NULL)
				{
					tu_print( 
						WDW_MSG, 
						STRESS_TAB_SYSINFO,
						line++,
						"%-5s: %8s %-2s",
						G_stressSiList[i].name,
						G_stressSiList[i].valstr,
						G_stressSiList[i].unit);
					i++;
				}
					
			}
#endif
		}

		/* # get Systemload */
		sysLoad = 0;

		hP = h;
		line = STRESS_LINE_RESULT;
#if 0
		/* # Display task-info and ignore other outputs */
		if( G_taskInfoHdlP != NULL )
		{
			stressDisplayTaskHdl( G_taskInfoHdlP );
			abortFlag = stressCheckInput( h, picP, logP );
			continue;
		}
#endif

		/* # walk through all test tasks */ 
		while( hP->id < STRESS_TESTID_ENDTBL ){
			if( hP->enabled && !G_haltSystem ){

				/* check locked tasks */
				timediff = hP->singleRunTime * 2;
				checkStuck = !(syssec%timediff);

				/* ######################################## */
				/* ### Message Window with test summary ### */
				/* ######################################## */
				if(checkStuck && !hP->alive) 
				{
					hP->stuck = 1;
				}else if( forceDisplay || checkStuck )
				{
					hP->stuck = 0;
					hP->alive = 0;
				}

				/* # print state # */
#ifdef STRESS_DISPLAY
				if( hP->errCnt )
				{
					tu_print_revers( WDW_MSG, STRESS_TAB_STAT, line, "	ERR" );
				}else
				{
					tu_print( WDW_MSG, STRESS_TAB_STAT, line, "   ON" );
				}
				if(hP->stuck == 1) 
				{
					tu_print_revers( WDW_MSG, STRESS_TAB_STAT-1, line, "!" );
					hP->stuck = 1;
				}else if( forceDisplay || checkStuck )
				{
					tu_print( WDW_MSG, STRESS_TAB_STAT-1, line, " " );
				}
#endif

				/* check if new errors occurred and save error Counter */
				if( hP->errCnt != hP->errCntSave ){
					logP->errCycle++;
					hP->errCntSave = hP->errCnt;
				}
#ifdef STRESS_DISPLAY				
				/* # print name # */
				tu_print( WDW_MSG, STRESS_TAB_NAME, line, "%s", hP->name );
				/* # print cpu # */
				tu_print( WDW_MSG, STRESS_TAB_CPU, line, "%d", hP->cpu+1); 

				/* # print load and xfer rate or loop # */
				if( !hP->stuck ){ /* only if test not stuck */
					if(hP->type == STRESS_TYPE_RATE)
					{
						timediff = tickGet() - hP->tickSave;
						bytediff = hP->bytenum - hP->bytenumSave;
						xfer = ((double)bytediff * 1000) / ((double)timediff * ms_per_tick);						
						hP->bytenumSave = hP->bytenum;
						hP->tickSave = tickGet();
						/* print only if value is ok */
					
						if(!double2HR((double) xfer, buf))
						{
							/* the format string is created at runtime */
							sprintf(fmt,"%%%ds ",STRESS_WIDTH_DATA);
							tu_print( WDW_MSG, STRESS_TAB_DATA, line, fmt, buf );
						}
					}else
					{
						/* the format string is created at runtime */
						sprintf(buf,"Loop # %u",hP->loopCnt);
						sprintf(fmt,"%%-%ds ",STRESS_WIDTH_DATA);
						tu_print( WDW_MSG, STRESS_TAB_DATA, line, fmt, buf );
					}
					/* load */
					if(hP->load_act)
						tu_print( WDW_MSG, STRESS_TAB_LOAD_N, line, "%3d%% ", hP->load_act);
					else
						tu_print( WDW_MSG, STRESS_TAB_LOAD_N, line, " <1%% ");
				}
				else{
					/* clear xferrate and output warn if test stuck */
					tu_print( WDW_MSG, STRESS_TAB_LOAD_N, line, "     " );
					tu_print( WDW_MSG, STRESS_TAB_DATA, line, "stopped       " );
				}
#endif	
			}

			/* TEST */
			spyReportCommon(&stressSpyParse);

			line++;
			hP++;
#if 0
			/* check for abort */
			if( (abortFlag = stressCheckInput( h, picP, logP )) != 0 ){
				break;
			}
#endif
			/* set handle to last entry to prevent further outputs */
			if( G_taskInfoHdlP != NULL )
			{
				while( hP->id < STRESS_TESTID_ENDTBL )
				{
					hP++;
				}
			}
		}/* end while(hP->id) */

		/* # check for user input # */	   
		if( !abortFlag )
		{
			if((abortFlag = stressCheckInput( h, picP, logP )))
				continue;
		}
	
		hP = h;
		taskDelay(sysClkRateGet()); /* wait */
	}
	tu_print(
		WDW_MSG, 
		0, 
		STRESS_LINE_MENU_QUIT+5,
		"\n");
	/* restart shell */
#if 0
	i = taskNameToId("tShell0");
	taskRestart(i);
#endif
	tu_exit();
}

#ifdef SYSTEST_LOGGING
/**********************************************************************/
/** Routine to write a logging entry to file.
 *
 *	\param h		pointer to STRESS_TEST_HDL
 *	\param picP 	pointer to STRESS_PIC_HDL
 *	\param logP 	pointer to STRESS_LOG_HDL
 *
 */
#define INIT_HOUR	30	/* for init we use an illegal hour */
static void stressLogFile(
	STRESS_TEST_HDL *h,
	STRESS_PIC_HDL *dummy,
	STRESS_LOG_HDL * logP
)
{
	FILE *fileP = NULL;
	STRESS_TEST_HDL *hP = h;
	u_int32 startTick = tickGet();
	u_int32 startLogTime = 0;
	u_int32 startErrLogTimer = 0;
	UCHAR	logHour = INIT_HOUR;   
	u_int32 i = 0;
	u_int32 logID = rand()%100000;
	char * buf = h->dbgP->buf;
	EEPROD2 ee_cpu, ee_brd;

	stressDbg(NULL, STRESS_DBGLEV_4,"%s",__FUNCTION__);

	do
	{			   
		stressTimeUpdate( &logP->curr_time);
		stress_si_update(logP);
		/* one logfile per hour */
#ifdef MULTIPLE_LOGFILES
		if(logHour != logP->curr_time.hour)
#else
		if(logHour == INIT_HOUR)
#endif
		{
			/* 
				generate Logging file name
				we don't always have a set RTC so we add a random logID
			*/			
			sprintf(logP->fileName, "%s%02d%02d%02d_%02d%02d_%d.csv",
				logP->netLog ? STRESS_LOGGING_FILE_NET : STRESS_LOGGING_FILE_NFS,
				logP->curr_time.year, logP->curr_time.mon, logP->curr_time.day,
				logP->curr_time.hour, logP->curr_time.min, logID);
			stressDbg(NULL,STRESS_DBGLEV_4,"create logfile (%s)",logP->fileName);
			/* logging file */
			if( (fileP = fopen(logP->fileName, "a")) == NULL )
			{
				stressDbg(NULL,STRESS_DBGLEV_1,"*** can't open logfile (%s)",logP->fileName);
			}else
			{
				/* print headline */
				i = 0;
				
				/* sysinfo */
				if(sysReadEEPROD(&ee_cpu,&ee_brd) == OK)
				{
					fprintf( fileP,"Serials: XM51 %u AE52 %u\n",
						ee_cpu.pd_serial,
						ee_brd.pd_serial
						);
				}
				while(G_stressSiList[i].func != NULL)
				{
					if(*G_stressSiList[i].name != '\0')
					{
						fprintf( fileP,"%s ",G_stressSiList[i].name);
						if(strlen(G_stressSiList[i].unit) != 0)
							fprintf( fileP,"[%s]",G_stressSiList[i].unit);
						fputc(';',fileP);
					}
					i++;
				}
				/* tasks */
				hP = h;
				while( hP->id != STRESS_TESTID_ENDTBL )
				{
					fprintf( fileP, "%s_stuck;%s_stuckEvt;%s_Err;", hP->name, hP->name, hP->name );
					hP++;
				}
				/* save the hour we created the logfile */
				logHour = logP->curr_time.hour;
				/* init done */
				logP->initDone = 1;
				fprintf(fileP,"\n");
				fclose( fileP );
			}
		}

		
		/* logging file */
		if( (fileP = fopen(logP->fileName, "a")) == NULL )
		{
			logMsg("*** can't open logfile (%s)",logP->fileName);
			/*stressDbg(NULL,STRESS_DBGLEV_OFF,"*** can't open logfile (%s)",logP->fileName);*/
			continue;
		}

		/* write information */
		/* sysinfo */
		i = 0;
		while(G_stressSiList[i].func != NULL)
		{
			if(*G_stressSiList[i].name != '\0')
			{
				/* the update is done in the display task */
				fprintf( fileP,"%s;", G_stressSiList[i].valstr);
			}
			i++;
		}
		/* tasks */
		hP = h;
		while( hP->id != STRESS_TESTID_ENDTBL ){
			fprintf( fileP, "%d;%d;%d;",hP->stuck,0,hP->enabled ? hP->errCnt : -1);
			hP++;
		}

		/* done */
		fprintf(fileP,"\n");
		fclose( fileP );

		taskDelay((logP->time * sysClkRateGet())/1000);

		/* debug info */
		while( h->halt ){
			taskDelay(100);
		}
	}while( !logP->stop );

	stressDbg(NULL,STRESS_DBGLEV_4,"%s - exiting",__FUNCTION__);
	taskDelete(taskIdSelf());
}


static void stressLogSetIntvl(STRESS_LOG_HDL * logHdlP, u_int32 intvl)
{
	logHdlP->time = (intvl < STRESS_LOG_INTVL_MIN) ? STRESS_LOG_INTVL_MIN : intvl;
}
#endif



/**********************************************************************/
/** Routine to start the stress testbench.
 *
 *	\param netLog			flag to ena. logging to network device
 *	\param logIvl			logging interval in ms
 *	\param custMode 		customer mode
 *	\param cfgFileP 		pointer to config. file name
 */
static int stb( u_int8 netLog, u_int32 logIvl, u_int8 custMode, char * cfgFileP  )
{
	char	*cTaskName = NULL;
	char	*cCfgLine = NULL;
	char*	pcCfgData;
	FILE   *pCfgFile = NULL;
	STRESS_TEST_HDL *h = &G_stressHdl[0];
	STRESS_LOG_HDL *logHdlP = NULL;
	STRESS_DBG_HDL *dbgHdlP = NULL;
	int priority = 0;
	u_int8 tid = 0;
	int32 ledTaskId = 0;
	int32 idleTaskId = 0;
	#define STB_BUFFERSIZE	30
	char buf[STB_BUFFERSIZE] = {0};
/*	u_int32 logging = 0;*/
/*	int userInput = 0;*/
	u_int32 loggingTaskId = 0;


	/*******************/
	/* init globals    */
	/*******************/
	G_customerMode = custMode;
/*	G_NetMen = 1;*/
	G_stressRun = 1;
	G_LedFreq = 5;
	/* § HW dependent */
	G_gpioBattest = 0;
	G_gpioDcDcOnly = 0;
	G_picioBattLoad = 0;
	G_picioFanCtrl = 0;
	G_picioPwrOff = 0;
	srand(OSS_TickGet(NULL));
	
	/***************************/
	/* init configuration file */
	/***************************/
#ifdef SYSTEST_CONFIGFILE
	if(cfgFileP)
	{
		if( (cCfgLine = malloc(STRESS_CFG_FILE_SIZE)) == NULL ){
			printf("*** malloc failed ***\n");
			goto ERR_EXIT;
		}
		else{
			memset( cCfgLine, 0x0, STRESS_CFG_FILE_SIZE );
		}
		if( (cTaskName = malloc(STRESS_TASKNAME_SIZE)) == NULL ){
			printf("*** malloc failed ***\n");
			goto ERR_EXIT;
		}
		else{
			memset( cTaskName, 0x0, STRESS_TASKNAME_SIZE );
		}
	}
	/***************************/
	/* Read cofiguration file  */
	/***************************/
	if(cfgFileP)
	{
		pCfgFile = fopen(cfgFileP, "r");
		if(!pCfgFile)
		{
			printf("*** File %s not found ***\n",cfgFileP);
		}
		else
		{	
			debug("open config file %s ok\n",cfgFileP);
			/* analyze whole file */
			while(fgets(cCfgLine, STRESS_CFG_FILE_SIZE, pCfgFile) != NULL)
			{
				h = &G_stressHdl[0];
				/* search through whole table */
				while( h->id < STRESS_TESTID_ENDTBL )
				{
					/* match name */
					if(strncmp(cCfgLine, h->name, strlen(h->name)) == 0)
					{
						debug("%s found in config file\n",h->name);
						pcCfgData = strchr(cCfgLine, '=')+1;
						h->enabled = atoi(pcCfgData);
						pcCfgData = strchr(cCfgLine, ',')+1;
						h->load_nom = atoi(pcCfgData);
						break;
					}
				h++;
				}
			}
			fclose(pCfgFile);
		}
	}

#endif
	/**************************/
	/* init test debug handle */
	/**************************/
	if( (dbgHdlP = malloc(sizeof(STRESS_DBG_HDL))) == NULL ){
		printf("*** malloc failed ***\n");
		goto ERR_EXIT;
	}
	else{
		memset( dbgHdlP, 0x0, sizeof(STRESS_DBG_HDL) );
	}

#if 0
		logFdSet(nullFd);
#endif

	dbgHdlP->pos_x	  = 1;
	dbgHdlP->pos_y	  = STRESS_LINE_DBG;

	G_StressDbg = dbgHdlP;

	stressSpyInit();
#ifdef AE52
	if(ml605_sysmon_init())
	{
		printf("*** ml605_sysmon_init failed ***\n");
		goto ERR_EXIT;
	}
#endif

#ifdef SYSTEST_LOGGING
	/***********************/
	/* init Logging handle */
	/***********************/
	if( (logHdlP = malloc(sizeof(STRESS_LOG_HDL))) == NULL ){
		printf("*** malloc failed ***\n");
		goto ERR_EXIT;
	}
	else{
		memset( logHdlP, 0x0, sizeof(STRESS_LOG_HDL) );
	}

	/* check logging interval */
	stressLogSetIntvl(logHdlP,logIvl);
	
	logHdlP->dbgP	 = dbgHdlP;
	logHdlP->verbose = G_verbose;
	logHdlP->netLog = netLog;

	memset( buf, 0x0, STB_BUFFERSIZE );
/* §	SysParamGet( "sernbr", buf, STB_BUFFERSIZE); */
	logHdlP->serial  = atoi(buf);
#endif
	

	/***************************/
	/* Start tasks			   */
	/***************************/

	h = &G_stressHdl[0];
	while( h->id < STRESS_TESTID_ENDTBL )
	{
		/* call generic init routine */
		stressInitRuntime(h);

		h->verbose = G_verbose;
		h->dbgP = dbgHdlP;
		
		/* start task */
		if( h->enabled )
		{
			stressStartTest(h);
			debug("Task %s (%u) started\r\n",h->name, h->taskId);
		}
	
		h++;
	}


#ifdef SYSTEST_LOGFILE
	/***********************/
	/* Start Logging Task  */
	/***********************/
	loggingTaskId = taskSpawn(
		"stb_log", STRESS_TASK_PRIORITY_LOW, 0,
		0x8000,(FUNCPTR)stressLogFile,(int)&G_stressHdl[0],
		0,
		(int)logHdlP,
		0,0,0,0,0,0,0
	);
#endif

#if 0
	/***********************/
	/* Start IDLE Task	   */
	/***********************/
	idleTaskId = taskSpawn(
		"stb_idle",
		STRESS_TASK_PRIORITY_LOWEST,
		0,
		0x1000,
		(FUNCPTR)stressIdle,
		sysClkRateGet(),
		0,
		0,
		0,0,0,0,0,0,0
	);
#endif

	G_StressstartTick = tickGet();
	G_IdlePercent = 0;
	G_IntPercent = 0;
	
#if SYSTEST_LED_SIGNALLING
	/***********************/
	/* Start LED Task	   */
	/***********************/
	ledTaskId = taskSpawn(
		"stb_Led",
		STRESS_TASK_PRIORITY_HIGH,
		0,
		0x200,
		(FUNCPTR)stressLed,
		5,		/* 5 Hz flashing */
		0,
		0,
		0,0,0,0,0,0,0
	);
#endif

	/******************/
	/* Start Display  */
	/******************/
	stressDisplay( &G_stressHdl[0], NULL, logHdlP );
	stressStop( &G_stressHdl[0], NULL, logHdlP );

ERR_EXIT:
	/* wait a little bit to let tasks end */
	taskDelay( sysClkRateGet() * 3 );

	if( dbgHdlP ){
		free( dbgHdlP );
	}
	
#ifdef SYSTEST_LOGGING
	if( logHdlP ){
		free( logHdlP );
	}
#endif

/*	if( cCfgFile )*/
/*		free( cCfgFile ); */

#ifdef SYSTEST_CONFIGFILE
	if( cCfgLine ){
		free( cCfgLine );
	}
	if( cTaskName ){
		free( cTaskName );
	}
#endif
/*	taskDelete(idleTaskId); */

	G_customerMode = 0;

	return 0;
}

/**********************************************************************/
/** Routine to stop Stress task.
 *	The routine clears the enable flag in the global Test handle for
 *	each task
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 */
static void stressStop( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picHdlP, STRESS_LOG_HDL *logP )
{
	STRESS_TEST_HDL *h = hP;

	spyClkStopCommon();

	while( h->id < STRESS_TESTID_ENDTBL ){
		h->enabled = 0;
		h++;
	}
#ifdef SYSTEST_PIC
	picHdlP->stop = 1;
#endif
	logP->stop = 1;

	G_stressRun = 0;
}


/**********************************************************************/
/** Routine to init Window Screen for stress testbench.
 *
 */
static void stressInitWindowScreen( void )
{
	char headLine[80];

	sprintf( headLine, "  MEN  Stress Testbench %s",VERSION);
	tu_init_screen( headLine, 0, STRESS_WIDTH_DEBUG, 1);
	tu_clear_wdw( WDW_RESULT );
}

/**********************************************************************/
/** Routine to init empty frame for stress test mask.
 *
 */
static void stressInitFrame( void )
{

	int line = 0;
	char lineStr[100] = {0};

	strcpy(lineStr, 			 " TEST   %s)     CPU     DATA            STAT  |      SYSINFO       |  MENU(MEN) ");
	tu_print(WDW_MSG, 0, line++, lineStr,"(%");
	tu_print(WDW_MSG, 0, line++, tu_underline_string(lineStr) );
	sprintf(lineStr,			 "                                              |                    | %c=CLEAR ERR",STRESS_KEY_CLEAR);
	tu_print(WDW_MSG, 0, line++, lineStr);
	sprintf(lineStr,			 "                                              |                    | %c=DBG ON/FF",STRESS_KEY_DEBUG);
	tu_print(WDW_MSG, 0, line++, lineStr);
#ifdef STRESS_OTHER_KEYS
	sprintf(lineStr,			 "                                              |                    | %c=LOAD     ",STRESS_KEY_LOAD);
	tu_print(WDW_MSG, 0, line++, lineStr);
	sprintf(lineStr,			 "                                              |                    | %c=TASKINFO ",STRESS_KEY_INFO);
	tu_print(WDW_MSG, 0, line++, lineStr);
	sprintf(lineStr,			 "                                              |                    | %c=LOGGING  ",STRESS_KEY_LOG);
	tu_print(WDW_MSG, 0, line++, lineStr);
#else
	sprintf(lineStr,			 "                                              |                    |             ");
	tu_print(WDW_MSG, 0, line++, lineStr);
	sprintf(lineStr,			 "                                              |                    |             ");
	tu_print(WDW_MSG, 0, line++, lineStr);
	sprintf(lineStr,			 "                                              |                    |             ");
	tu_print(WDW_MSG, 0, line++, lineStr);
#endif
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "------------ DEBUG CONSOLE -------------------|                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | Q=QUIT     ");
	tu_print(WDW_MSG, 0, line++, "--------------------------------------------------------------------------------");
}

/**********************************************************************/
/** Routine to init empty frame for stress test info mask.
 *
 */
static void stressInitTaskInfoFrame( void )
{

	int line = 0;


	char* lineStr = "             TEST TASK INFORMATION             |      SYSINFO        |  MENU(MEN) ";
	tu_print(WDW_MSG, 0, line++, lineStr);
	tu_print(WDW_MSG, 0, line++,
			 tu_underline_string("                                              |         SYSINFO    |  MENU(MEN) ") );
	tu_print(WDW_MSG, 0, line++, "                                              |                    | O=LOAD     ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | R=CLEAR ERR");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | Z=DBG ON/FF");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | I=TASKINFO ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | G=LOGGING  ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | S=BatTest  ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | Y=UseBat   ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | W=PwrOffDut");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | N=FanOn    ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | L=ChargeBat");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | P=PICON/OFF");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    |            ");
	tu_print(WDW_MSG, 0, line++, "                                              |                    | Q=QUIT     ");
	tu_print(WDW_MSG, 0, line++, "--------------------------------------------------------------------------------");
	
}

/**********************************************************************/
/** Routine to display Edit Init screen.
 *
 */
static void stressEditWindowScreen( void )
{
	char headLine[80];

	sprintf( headLine, "  MEN Stress Testbench (c) 2011 by men mikro elektronik gmbh");
	tu_init_screen( headLine, 0, 2, 1);
	tu_clear_wdw( WDW_RESULT );
	tu_key_line( "'q'=Quit load: '+'=increase '-'=decrease	 cursor: 'd'=down 'u'=up" );
}


/* if STRESS_TEST_HDL == NULL we use the G_verbose, else the verbosity of the test */
void stressDbg( STRESS_TEST_HDL *dP,u_int8 lvl,const char * fmt, ...)
{
	va_list ap;
	char buf[STRESS_DBG_LINE_BUFSIZE] = {0};

	if(fmt == NULL)
		return;

	if(dP == NULL)
	{
		if(lvl > G_verbose)
			return;
	}else if(lvl > dP->verbose)
		return;
	
	
	va_start(ap,fmt);
	vsnprintf(buf,STRESS_DBG_LINE_BUFSIZE,fmt,ap);
	va_end(ap);
	stressWrtDbg(G_StressDbg, buf);
}


/**********************************************************************/
/** Routine to write a debug message to buffer. 
 *	All chars after STRESS_WIDTH_DEBUG are truncated.
 *
 *	\param dP			pointer to STRESS_DBG_HDL
 *	\param dataP		pointer to data to write
 */
static void stressWrtDbg( STRESS_DBG_HDL *dP, char *dataP )
{
	u_int32 c = 0, idx = 0;
	u_int32 len = strlen(dataP);
	
	if(G_stressRun )
	{
		len = (len >= STRESS_WIDTH_DEBUG) ? STRESS_WIDTH_DEBUG-1 : len;
		strncpy(dP->dataP[dP->line],dataP, len);
		dP->dataP[dP->line][len] = '\0';
		dP->line++;
		dP->line%=STRESS_HIGHT_DEBUG; /* wrap */
		G_debugChanged = 1;
	}else
	{
		printf( "%s\n", dataP );
	}
}

/**********************************************************************/
/** Routine to display debug data in window.
 *
 *	\param dP			pointer to STRESS_DBG_HDL
 */
static void stressDisplayDebug( STRESS_DBG_HDL *dP )
{
	u_int32 i = 0; 
	u_int32 c = 0; /* current line */
	u_int32 l = 0; /* lines output */

	char	buf[STRESS_WIDTH_DEBUG+1]; /* line buffer */
	
	if(dP == NULL)
	{
		printf("%s - ***dP NULL!\n",__FUNCTION__);
		return;
	}
	if(dP->dataP[0][0] == '\0')
	{
		if(G_verbose)
			tu_print( WDW_MSG, dP->pos_x, STRESS_LINE_DBG, "empty");
		return;
	}
	
	c = dP->line;
	for(l = 0; l < STRESS_HIGHT_DEBUG; l++)
	{	
		/* the format string is created at runtime */
		sprintf(buf,"%%-%ds ",STRESS_WIDTH_DEBUG-1);
		tu_print( WDW_MSG, dP->pos_x, STRESS_LINE_DBG+l, buf, dP->dataP[c]);
		c++;
		c%=STRESS_HIGHT_DEBUG;
	}
}

/**********************************************************************/
/** Routine to display Test dis/enable menu.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 *	\param line 		line position of menu
 *	\param col			col position of menu
 */
static void stressDisplayTestEnable( STRESS_TEST_HDL *hP, int line, int col, u_int8 inv_mode )
{
	STRESS_TEST_HDL *h = hP;
	int lineResultWindow = 2;
	char c = 'a';

	while( h->id < STRESS_TESTID_ENDTBL )
	{
		if( inv_mode ){
			tu_print( WDW_MSG, col, line--, "%c=%s %-3s", c++,
				h->name, h->enabled ? "OFF":"ON");
		}
		else{
			tu_print( WDW_MSG, col, line--, "%c=%s %-3s", c++,
				h->name, h->enabled ? "ON":"OFF");
		}
		h++;
	}
}

/**********************************************************************/
/** Routine to display Test dis/enable menu.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 *
 *	\return 0 if continue or 1 if quit
 */
static int stressCheckInput( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picP, STRESS_LOG_HDL *logP )
{
	STRESS_TEST_HDL *h = hP;
	int32	userInput;
	int 	returnCode = 0;
	u_int32 value = 0;


	userInput = -1;
	userInput =  tu_keybd_hit();
	switch( userInput ){
		case STRESS_KEY_CLEAR:	/* clear errors */
			tu_clear_wdw( WDW_RESULT );
			tu_clear_wdw( WDW_MSG );
			tu_clear_wdw(WDW_KEYS);

			/* Set NoError LED frequency */
			G_LedFreq = 5;

			stressInitFrame();
			stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
			h = hP;
			while( h->id < STRESS_TESTID_ENDTBL ){
				h->errCnt = 0;
				h->stuck = 0;
				h++;
			}
			logP->errCycle = 0;
			break;

		case STRESS_KEY_HALT:
			if( !G_haltSystem ){
				G_haltSystem = 1;
				tu_print( WDW_MSG, STRESS_TAB_SYSINFO, STRESS_LINE_SYSTIME, "stopped");
			}
			else{
				G_haltSystem = 0;
				tu_print( WDW_MSG, STRESS_TAB_SYSINFO, STRESS_LINE_SYSTIME, "       ");
			}

			while( h->id < STRESS_TESTID_ENDTBL ){
				h->halt = G_haltSystem;
				h++;
			}
			logP->halt = G_haltSystem;

			break;

#ifdef STRESS_OTHER_KEYS
		case STRESS_KEY_LOAD:	/* Change load of task */
			stressEditLoad( hP );
			G_StressstartTick = tickGet();

			tu_clear_wdw( WDW_RESULT );
			tu_clear_wdw( WDW_MSG );
			tu_clear_wdw(WDW_KEYS);
			stressInitFrame();
			stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
			break;

		case STRESS_KEY_INFO:	/* Task Info */
			if( G_taskInfoHdlP == NULL ){
				G_taskInfoHdlP = stressGetTaskHdlP( hP, picP );
			}
			else{
				G_taskInfoHdlP = NULL;
				tu_clear_wdw( WDW_RESULT );
				tu_clear_wdw( WDW_MSG );
				tu_clear_wdw(WDW_KEYS);
				stressInitFrame();
				stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
			}
			break;
		case STRESS_KEY_LOG: /* Logging time */
			stressShowInputBox( &value );
			stressLogSetIntvl(logP,value);
			tu_clear_wdw( WDW_RESULT );
			tu_clear_wdw( WDW_MSG );
			tu_clear_wdw( WDW_KEYS );
			stressInitFrame();
			stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
			break;
#endif
		case STRESS_KEY_DEBUG:
			{
				h = hP;

				stressShowInputBox( &value );
				while( h->id < STRESS_TESTID_ENDTBL ){
					h->verbose = value;
					h++;
				}
				logP->verbose = value;
				G_verbose = value;
			}
			tu_clear_wdw( WDW_RESULT );
			tu_clear_wdw( WDW_MSG );
			tu_clear_wdw( WDW_KEYS );
			stressInitFrame();
			stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
			break;
		case 'q':
		case 'Q':
			returnCode = 1;
			break;
		default:
			/* Start/Stop single test */
			if( userInput < 'a' || userInput > 'z')
				break;
		
			/* search test handle entry */
			while( userInput > 'a'){
				userInput--;
				h++;
			}

			if( h->routine ){
				if( h->enabled ){
					stressStopTest( h );
				}
				else{
					stressStartTest( h );
				}
			}
			stressInitFrame();
			stressDisplayTestEnable( hP, (STRESS_LINE_MENU_QUIT-1), STRESS_TAB_MENU, 1 );
	}

	return returnCode;
}

/**********************************************************************/
/** Routine to stop a single test.
 *
 *	\param hP			pointer to STRESS_TEST_HDL Test entry
 *
 */
static void stressStopTest( STRESS_TEST_HDL *hP )
{
	hP->enabled = 0;
	hP->errCnt = 0;
	hP->bytenum = 0;
	hP->startTick = 0;
	hP->allExeTime = 0;
}

/**********************************************************************/
/** Routine to start a single test.
 *
 *	\param h		pointer to STRESS_TEST_HDL Test entry
 *
 */
static void stressStartTest( STRESS_TEST_HDL * h )
{
	char	cTaskNameStr[40] = {0};
#ifdef _WRS_CONFIG_SMP
	cpuset_t affinity;
#endif

	/* call specific init routine if needed */
	if( h->init )
	{
		(*h->init)(h, 0);
	}

	h->enabled = 1;
	h->tickSave = tickGet();
	
	/* create task */
	sprintf(cTaskNameStr, STRESS_TASK_PREFIX"%s",h->name);
	h->taskId = taskCreate(
		cTaskNameStr,
		h->prio,
		0,
		0x10000,
		(FUNCPTR)h->routine,
		(int)h,
		h->load_nom,
		0,
		0,0,0,0,0,0,0);

	if (h->taskId  == NULL)
	{
		stressDbg(NULL,STRESS_DBGLEV_1,"Can't create task %s\n",cTaskNameStr);
		return;
	}
	
#ifdef _WRS_CONFIG_SMP
	/* -1 -> no affinity */
	if(h->cpu >= 0)
	{
		/* Clear the affinity CPU set and set index for CPU 1 */
		CPUSET_ZERO (affinity);
		CPUSET_SET	(affinity, h->cpu);

		if (taskCpuAffinitySet (h->taskId, affinity) == ERROR)
		{
			/* Either CPUs are not enabled or we are in UP mode */
			stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set affinity %u for task %s\n",
				affinity,
				cTaskNameStr);
			taskDelete (h->taskId);
			return;
		}
	}
#endif

	/* set StdIo descriptors to nirvana */
#if 0
	if(ioTaskStdSet(h->taskId,STD_IN, nullIn))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set STD_IN\n");
#endif
	if(ioTaskStdSet(h->taskId,STD_OUT, nullOut))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set STD_OUT\n");

	if(ioTaskStdSet(h->taskId,STD_ERR, nullErr))
		stressDbg(
				NULL,
				STRESS_DBGLEV_1,
				"Can't set STD_ERR\n");

	/* start task */
	taskActivate (h->taskId );
}

/**********************************************************************/
/** Routine to print a double "human readable"
 *
 *	\param time 		pointer to time string
 *
 *	\return char * 
 */
static int	double2HR(double d, char * cP)
{
	int ret = 0;

	if(!cP)
		return ERROR;

	if(d < KB_SIZE)
	{
		sprintf(cP,"%-4.2f	B/s",d);
		return OK;
	}else if(d < KB_SIZE * KB_SIZE)
	{
		sprintf(cP,"%-4.2f KB/s",d / KB_SIZE);
		return OK;
	}else if(d < KB_SIZE * KB_SIZE * KB_SIZE)
	{
		sprintf(cP,"%-4.2f MB/s",d /(KB_SIZE * KB_SIZE));
		return OK;
	}else
		return ERROR;
}


/**********************************************************************/
/** Routine to display Stress task information.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 */
static void stressDisplayTaskHdl( STRESS_TEST_HDL *hP )
{
	int line = 2;
	int col = 3;

	tu_print( WDW_MSG, col, line++, "Name         : %s", hP->name );
	tu_print( WDW_MSG, col, line++, "Test Status  : %s", hP->enabled ? "Enabled" : "Disabled" );
	tu_print( WDW_MSG, col, line++, "Task Status  : %s", hP->halt ? "Stopped" : "Running" );
	tu_print( WDW_MSG, col, line++, "Task ID      : %d", hP->taskId );
	tu_print( WDW_MSG, col, line++, "Table ID     : %d", hP->id );
	tu_print( WDW_MSG, col, line++, "Load         : %d %%", hP->load_nom );
	tu_print( WDW_MSG, col, line++, "Time overall : %d s", (hP->allExeTime / sysClkRateGet()) );
	tu_print( WDW_MSG, col, line++, "Bytes        : %d ", hP->bytenum );
	tu_print( WDW_MSG, col, line++, "Errors       : %d", hP->errCnt );

	taskDelay( 50 );
}

/**********************************************************************/
/** Routine to get the Handle of an test Stress task.
 *
 *	\param hP			pointer to STRESS_TEST_HDL
 */
static STRESS_TEST_HDL* stressGetTaskHdlP( STRESS_TEST_HDL *hP, STRESS_PIC_HDL *picP )
{
	STRESS_TEST_HDL *h = hP;
	STRESS_TEST_HDL *retHdlP = NULL;
	int userInput = 0;
	u_int32 userOffset = 0;
	int lineResultWindow = 2;

	tu_clear_wdw( WDW_MSG );
	stressInitTaskInfoFrame();

	while( h->id < STRESS_TESTID_ENDTBL ){
		tu_print( WDW_MSG, 0, lineResultWindow++,
			"%x: %s => %-3s", h->id, h->name, h->enabled ? "ON":"OFF" );
		h++;
	}

	h = hP;
	userInput = UOS_KeyWait();
	switch( userInput ){
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':

			if( userInput <= 0x39 ){
				userOffset = 0x30;
			}
			else{
				userOffset = 0x57;
			}

			/* search test handle entry */
			while( (h->id != (userInput - userOffset)) && (h->id < STRESS_TESTID_ENDTBL) ){
				h++;
			}

			if( h->id < STRESS_TESTID_ENDTBL ){
				retHdlP = h;
			}
			break;

		case 'q':
		default :
			break;
	}

	tu_clear_wdw( WDW_MSG );
	stressInitTaskInfoFrame();

	return retHdlP;
}

/**********************************************************************/
/** Routine of Idle Stress task.
 *
 */
static void stressShowInputBox( u_int32 *value )
{
	int line = 8;

	/* clear message area on screen and display message frame */
	tu_print_revers(WDW_MSG, 20, line++, "                                         ");
	tu_print_revers(WDW_MSG, 20, line++, "                                         ");
	tu_print_revers(WDW_MSG, 20, line++, "                                         ");
	tu_print_revers(WDW_MSG, 20, line++, "                                         ");
	tu_print_revers(WDW_MSG, 20, line++, "                                         ");

	line = 8;

	tu_print(WDW_MSG, 26, line++, "Input Box");
	tu_print(WDW_MSG, 22, line++, "                                 ");
	tu_print(WDW_MSG, 22, line++, "                                 ");
	tu_print(WDW_MSG, 22, line++, "                                 ");

	line = 10;

	*value = getDecLong( *value, 22, line);
	stressInitWindowScreen();
}

/**********************************************************************/
/** Routine to get the seconds from a time string.
 *
 *	\param tP			pointer to time structure
 *
 *	\return number of seconds
 */
u_int32 getSecondsFromTime( TIME_STRUCT * tP)
{
	u_int32 dwSeconds;

	dwSeconds = (u_int32)tP->sec +
				(u_int32)tP->min*60 +
				(u_int32)tP->hour*SECONDS_PER_HOUR +
				(u_int32)tP->day*SECONDS_PER_HOUR*24 +
				(u_int32)tP->mon*SECONDS_PER_HOUR*24*31 +
				(u_int32)tP->year*SECONDS_PER_HOUR*24*31*12;

	return dwSeconds;
}


/**********************************************************************/
/** Routine to read date and time from the RTC.
 *
 *	\param sP Pointer to TIME_STRUCT
 *
 */
void stressTimeUpdate(TIME_STRUCT * sP )
{
	sysRtcGetDate( 
			&sP->year, 
			&sP->mon, 
			&sP->day, 
			&sP->hour, 
			&sP->min, 
			&sP->sec );
}

/**********************************************************************/
/** Routine to read Temperature from Sensor.
 *
 *	\param temp 	pointer to store temp data
 *
 */

static void stressTemp1Update( int8 *temp )
{
	*temp = sysLm75Temp();
}

static void stressTemp2Update( int8 *temp )
{
	
	*temp = sysLm75_2Temp();
}

static void stressTemp3Update( int8 *temp )
{
	float ftemp = 0;
	
	ml605_sysmon_ctemp_get(&ftemp);
	*temp = (int8) ftemp;
}

/**********************************************************************/
/** Routine to update system load.
 *
 *	\param temp 	pointer to store temp data
 *
 */
/* TODO: throw this out */
void stressLoadUpdate( u_int8 * load, STRESS_TEST_HDL *h )
{
	*load = (u_int8) stressGetIdle();
}


u_int32 stressGetIdle(void)
{
	return G_IdlePercent;
}

u_int32 stressGetInt(void)
{
	return G_IntPercent;
}


#if 0
/**********************************************************************/
/** Routine to handle LED flashing.
 *	Routine shall be run in task context.
 *
 *	\param freq 	Frequency of LED flashing in Hz [1..100]
 */
static void stressLed( u_int32 freq )
{
	static u_int8 state = 0;
	u_int32 delay = 0;

	/* calculate delay time */
	if( freq == 0 || freq > 100 ){
		freq = 1;
	}

	G_LedFreq = freq;

	while( G_stressRun ){


		if( state == 0 ){
			state = 1;
			sysLedSet( state );

			delay = 1000 / (G_LedFreq * 2);
		}
		else{
			state = 0;
			sysLedSet( state );
		}
		taskDelay( delay );
	}
}
#endif
/**********************************************************************/
/** Routine to check two value for limit difference.
 *	\return 0 if data is valid and 1 if data range exceeded
 */
static int checkAbsValue( float data, float newdata, float limit )
{
	/* printf("\nChecking %2.3f with %2.3f Range %2.3f ", data, newdata, limit); */
	if( data >= newdata ){
		if( (data - newdata) > limit ){
			return 1;
		}
	}
	else{
		if( ( newdata - data ) > limit ){
			return 1;
		}
	}

	return 0;
}

/**********************************************************************/
/** Routine to get a Decimal Long value from the console.
 *
 *	\param def		old value
 *	\param col		column of input cursor
 *	\param line 	line of Input cursor
 *
 *	\return read input value
 */
static u_int32 getDecLong(u_int32 def, int col, int line)
{
	char buf[20];
	u_int32 val;
	u_int32 i = 0;
	u_int8 gotit = 0;
	char c = 0;

	tu_print( WDW_MSG, col+3, line, "[%010u]: ", def );
	tu_print(WDW_MSG, col+15, line, "> ");

	do{
		c = UOS_KeyWait();
		tu_print(WDW_MSG, col+17+i, line, "%c", c);
		if( c == 0xd ){
			buf[i] = 0x0;
			gotit++;
		}
		else if( c == 0x8 ){
			i--;
			tu_print(WDW_MSG, col+17+i, line, "   ");
			tu_print(WDW_MSG, col+17+i, line, "" );
		}
		else{
			buf[i++] = c;
		}
	} while( !gotit );

	if( sscanf( buf, "%d", &val ) == 1 ){
		return val;
	}
	else{
		return def;
	}
}



static int stressSpyParse(const char * fmt, ...)
{
	va_list ap;
	char name[20] = {0};
	char * p = NULL;
	STRESS_TEST_HDL * h = &G_stressHdl[0];
	int len = 0;
	
	va_start(ap,fmt);
	vsnprintf(G_spyBuffer,STRESS_SPY_BUFSIZE,fmt,ap);
	va_end(ap);

	/* look for task names */
	while( h->id < STRESS_TESTID_ENDTBL )
	{
		len = sprintf(name,"%s%s",STRESS_TASK_PREFIX,h->name);
		if(strncmp(G_spyBuffer,name,len) == 0)
		{
			/* move pointer behind "NAME" */
			p = G_spyBuffer + len;
			/* move pointer to "ENTRY" */ 
			STRESS_BLANK_STEP(p);
			/* move pointer to "TID" */ 
			STRESS_CHAR_STEP(p);
			STRESS_BLANK_STEP(p);
			/* move pointer to "PRIO" */ 
			STRESS_CHAR_STEP(p);
			STRESS_BLANK_STEP(p);
			/* move pointer to "TOTAL %" */ 
			STRESS_CHAR_STEP(p);
			STRESS_BLANK_STEP(p);
			/* read value */
			sscanf(p,"%u%%",(u_int32 *) &h->load_act);
			stressDbg(NULL, STRESS_DBGLEV_4,"%s %u%%",name, h->load_act);
			return 0;
		}
		h++;
	}
	/* or is it the IDLE info? */
	len = sprintf(name,"IDLE");
	if(strncmp(G_spyBuffer,name,len) == 0)
	{
		p = G_spyBuffer + len;
		STRESS_BLANK_STEP(p);
		sscanf(p,"%u%%",&G_IdlePercent);
		stressDbg(NULL, STRESS_DBGLEV_4,"%s %u%%",name, G_IdlePercent);
		return 0;
	}
	/* or is it the INT info? */
	len = sprintf(name,"INTERRUPT");
	if(strncmp(G_spyBuffer,name,len) == 0)
	{
		p = G_spyBuffer + len;
		STRESS_BLANK_STEP(p);
		sscanf(p,"%u%%",&G_IntPercent);
		stressDbg(NULL, STRESS_DBGLEV_4,"%s %u%%",name, G_IntPercent);
		return 0;
	}
	return 0;
}

static int stressSpyPrint(const char * fmt, ...)
{
	va_list ap;
	
	va_start(ap,fmt);
	vsnprintf(G_spyBuffer,STRESS_SPY_BUFSIZE,fmt,ap);
	va_end(ap);

	return 0;
}

int stressSpyInit(void)
{
	return spyClkStartCommon(STRESS_REFRESH_FREQ_SPY,&stressSpyPrint);
}

