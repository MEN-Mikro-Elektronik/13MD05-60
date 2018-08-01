/*********************	P r o g r a m  -  M o d u l e ***********************/
/*!
 *		  \file  systemtest.c
 *
 *		\author  MKolpak
 *		  $Date: 2012/09/06 15:31:56 $
 *	  $Revision: 1.2 $
 *
 *		  \brief  test routines of AE52
 *
 *	   Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest.c,v $
 * Revision 1.2  2012/09/06 15:31:56  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:02  MKolpak
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2010 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

/*
 DOCUMENTATION:
 Cfg. file name may not be supplied.

 Add new test:
 - include .c file in systemtest.c
 - add test descriptor (G_stressHdl[]) in systemtest_stress.c

 Add new SYSINFO:
 - provide function in systemtest_sysinfo.c
 - add function to G_stressSiList

*/

/*--------------------------------------------------------------+
 |	INCLUDE 													|
 +-------------------------------------------------------------*/
#include "vxWorks.h"
#include "string.h"
#include "ioLib.h"
#include "sioLib.h"
#include "iosLib.h"
#include <fioLib.h>
#include <stdio.h>
#include <taskLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <spyLib.h>
#include <math.h>
#include <MEN/men_typs.h>
/*#include <MEN/sc20_cfg.h>*/
#define MAC_MEM_MAPPED
#include <MEN/maccess.h>
/*#include <MEN/mgt5200.h>*/
#include <MEN/testutil.h>
#include <MEN/sysparam2.h>
#define MSCAN_IS_ODIN
#include <MEN/mscan.h>
#include <MEN/usr_oss.h>
#include <MEN/sys_rtc.h>

#include "systemtest_stress.h"
#include "systemtest_sysinfo.h"

/*-------------------------------------------------------------*/
/*	EXTERNALS												   */
/*-------------------------------------------------------------*/
extern int hdtest();
extern int mtest();
/*extern int sysLm75Temp( void );*/
extern STATUS	hostAdd (char *hostName, char *hostAddr);
extern STATUS	nfsMount (const char *host, const char *fileSystem,
						  const char *localName);

/*-------------------------------------------------------------*/
/*	DEFINES 												   */
/*-------------------------------------------------------------*/

/* central switch to en/disable debug functionalities. uncomment as needed. */
#define DEBUG

/* ### error reporting and debugging ###*/
#ifndef DBG_ST
#define DBG_ST	G_verbose
#include <MEN/dbg.h>		/* debug module */

#undef DEBUG_LOCAL

#ifdef DEBUG_LOCAL
	#define debug(fmt,args...) printf("%s - ",__FUNCTION__);printf (fmt ,##args)
	#define locate() printf("%s: line %u\n",__FILE__,__LINE__);OSS_Delay(NULL,1)	
	#define error(fmt,args...) printf("### %s - line %u: ",__FUNCTION__,__LINE__); \
								printf (fmt ,##args);OSS_Delay(NULL,1)
#else
	#define debug(fmt,args...) if(DBG_ST)DBG_Write(DBH,"%s - "fmt,__FUNCTION__,##args)
	#define locate()
	#define error(fmt,args...) DBG_Write(DBH,"### %s - line %u: "fmt,__FUNCTION__,__LINE__,##args);
#endif
#define DBH sysDbgHdlP
#endif /* #ifndef DBG_ST*/


/* for severe errors */
#define ST_ERR(fmt,args...) printf("%u: ### "fmt,tickGet(),##args);


#define SYSTEST_LOGGING
#ifdef SYSTEST_LOGGING	/* needs SYSTEST_LOGGING */ 
#define SYSTEST_LOGFILE
#define STRESS_LOG_INTVL_MIN	100
#endif
#define STRESS_DISPLAY
#define SYSTEST_CONFIGFILE
#undef SYSTEST_PIC
#undef SYSTEST_LED_SIGNALLING

#define PI (4*atan(1))

/*-------------------------------------------------------------*/
/*	LOCALS													   */
/*-------------------------------------------------------------*/
static u_int8 G_gpioBattest = 0;
static u_int8 G_gpioDcDcOnly = 0;
static u_int8 G_picioBattLoad = 0;
static u_int8 G_picioPwrOff = 0;
static u_int8 G_picioFanCtrl = 0;
static u_int8 G_haltSystem = 0;
static STRESS_TEST_HDL *G_taskInfoHdlP = NULL;
static u_int8 G_stressRun = 0;
static u_int8 G_customerMode = 0;

static u_int32 G_LedFreq = 0;

u_int8 G_verbose = 0;


/*-------------------------------------------------------------*/
/*	PROTOTYPES												   */
/*-------------------------------------------------------------*/
static void stressWrtDbg( STRESS_DBG_HDL *dP, char *dataP );
static void stressInit(void);

/*void sysLedSet( u_int8 state );*/

#include "systemtest_misc.c"
#include "systemtest_AE52.c"
#include "systemtest_mem.c"
#include "systemtest_sysinfo.c"
#include "systemtest_stress.c"
#if 0
#include "systemtest_net.c"
#include "systemtest_sata.c"
#include "systemtest_usb.c"
#include "systemtest_smb.c"
#include "systemtest_misc.c"
#include "systemtest_bluetooth.c"
#include "systemtest_serial.c"
#include "systemtest_pic.c"
#include "systemtest_mpc5200can.c"
#include "systemtest_stress.c"
#endif

/* sysinfo not correctly displayed with logIvl > 1000 */
void stressTestVibration(void)
{
	char * tName;

	/* make sure its not a serial connection */
	tName = taskName( taskIdSelf() );
	if( tName == NULL || !strncmp( "tShell0", tName, 7 ))
	{
		printf("This test must run from telnet.\n");
		return;
	}
	stressInit();
	/* change global stdio */
#if 0
	ioGlobalStdSet(STD_IN,nullIn);
#endif
	ioGlobalStdSet(STD_OUT,nullOut);
	ioGlobalStdSet(STD_ERR,nullErr);
	
	G_verbose = STRESS_DBGLEV_1;
	stb( 0, 1000, 0, "XM51_vibration.cfg");
	printf("This test must run from telnet.\n");
}

/* sysinfo not correctly displayed with logIvl > 1000 */
void stressTestBurnIn(void)
{
	int fd = 0;
	
	stressInit();
	/* stdin is wrong when test is started from script */
	fd = ioGlobalStdGet(STD_IN);
	ioTaskStdSet(taskIdSelf(),STD_IN,fd);
	G_verbose = STRESS_DBGLEV_1;
	stb( 0, 1000, 0, "XM51_burnin.cfg");
}


/* sysinfo not correctly displayed with logIvl > 1000 */
void stressTestDevelopment(void)
{
	stressInit();
	G_verbose = STRESS_DBGLEV_4;
	stb( 0, 1000, 0, "XM51_stress_DEV.cfg");
}


void STM(void)
{
	u_int32 userInput = 0;

	
	/*********************************/
	/* get test specific information */
	/*********************************/
	do{
		printf( "Systemtest - built %s %s \n\n",__DATE__,__TIME__);
		printf( "	Select Test\n" );
		printf( "===================\n");
		printf( "1. Vibration (telnet)\n" );
		printf( "2. Burn-In (RS232)\n" );
		printf( "3. Development\n" );
		printf( "q. Exit			\n" );
	
		printf( "\n Your choice : " );
	
		userInput = UOS_KeyWait();

		/* check for valid input */
		switch(userInput)
		{
			case '1':
			case '2':
				break;
			case 'q':
			case 'Q':
				return;
			default:
				continue;
		}

		/* Confirm test start */
		printf( "\nStart Test (j/n) ?: " );
		if( UOS_KeyWait() != 'j'  )
			continue;
		
		/* call test */
		switch(userInput)
		{
			case '1':
				stressTestVibration();
				return;
			case '2':
				stressTestBurnIn();
				return;
			case '3':
				stressTestDevelopment();
				return;
			default:
				return;
		}
	}while(1);

}

static void stressInit(void)
{
	/* get time and update rtc */
	sysMenTimeGet(0,1);
	
	/* change logging file descriptor to disable error messages */
#if 0
	nullIn = open("/null", O_RDONLY, 0644);
	if( nullIn == NULL ){
		printf("*** can't create nullIn ***\n");
	}
#endif
	nullOut = open("/null", O_WRONLY, 0644);
	if( nullOut == NULL ){
		printf("*** can't create nullOut ***\n");
	}
	nullErr = open("/null", O_WRONLY, 0644);
	if( nullErr == NULL ){
		printf("*** can't create nullErr ***\n");
	}
	/* TODO: change this to a valid file */
	logFdSet(nullErr);
}


/**********************************************************************/
/** Routine to set global G_customerMode.
 *
 *	\param mode   G_bbraunMode
 *
 */
void pt_SetCustomerMode( u_int8 mode )
{
	G_customerMode = mode;
}

/**********************************************************************/
/** Routine to set global verbose level for pt_Xxxx routines.
 *
 *	\param verbose	 Verbose level (0=No outputs, 1=all outputs)
 *
 */
void pt_SetDbgLev( u_int8 verbose )
{
	switch(verbose)
	{
		case STRESS_DBGLEV_1:
		case STRESS_DBGLEV_2:
		case STRESS_DBGLEV_3:
		case STRESS_DBGLEV_4:
			G_verbose = verbose;
			break;
		default:
			printf("*** Debug Level not supported ***\n");
	}
}

/**********************************************************************/
/** Routine to verify the LED function.
 *	This routines starts switching on/off the LED with a frequency of
 *	2 Hz for 10 seconds.
 *
 *	\return always 0
 */
#if 0
int pt_Led( void )
{
	u_int8 state = 0;
	u_int32 i=0;

	for(i=0;i<10;i++){
		if(state){
			sysLedSet( 0 );
		}
		else{
			sysLedSet( 1 );
		}
		state = ~state;
		taskDelay(125);
	}
	return 0;
}
#endif

/* move to MISC */
/**********************************************************************/
/** Routine to verify the RTC time.
 *	This routine expected two time string with format yy,mm,dd,hh,min,ss
 *	in a unsigned char structure. The recent time is expected last.
 *
 *	\param time 	pointer to time saved in TIME_STRUCT
 *
 *	\return 0 if time string is consistent or 6 for Error
 */
static int checkTime( TIME_STRUCT *time1P, TIME_STRUCT *time2P )
{
	int retval = 0;
	u_int8 end_val[] = { 99,12,31,23,59,59 };
	u_int8 start_val[] = { 0,1,1,0,0,0 };
	u_int32 i;
	u_int8 *time1 = (u_int8*)time1P;
	u_int8 *time2 = (u_int8*)time2P;

	for(i=0; i<6; i++)
	{
		/* digit incremented or same? */
		if(!(time1[i] <= time2[i]))
		{
			/* rollover? */
			if((time2[i] != start_val[i]) && 
				(time1[i] != end_val[i]))
			{
				/* not sane */
				return ERROR;
			}
		}
	}
	return OK;
}


/**********************************************************************/
/** Routine to verify the RTC function.
 *	\return always 0
 */
int pt_Rtc( void )
{
	TIME_STRUCT time1,time2;
	u_int8 weekDay = 0;

	u_int32 startTick = 0;
	u_int32 endTick = 0;

	/* clear time structure */
	memset( &time1.year, 0x0, sizeof(TIME_STRUCT) );
	memset( &time2.year, 0x0, sizeof(TIME_STRUCT) );

	/* Get RTC time */
	sysRtcGetDate( &time1.year,
				   &time1.mon,
				   &time1.day,
				   &time1.hour,
				   &time1.min,
				   &time1.sec );

	/* Wait for seconds to be changed to start measurement */
	do{
		taskDelay(1);
		sysRtcGetDate( &time2.year,
					   &time2.mon,
					   &time2.day,
					   &time2.hour,
					   &time2.min,
					   &time2.sec );
	} while( time2.sec == time1.sec );

	/* Start Time measurement */
	startTick = tickGet();

	do
	{
		taskDelay(1);
		if( checkTime( &time1, &time2) )
		{
			printf(
				"*** Time insanity ***\n\tt1:%02d:%02d:%02d %02d.%02d.%04d\tt2%02d:%02d:%02d %02d.%02d.%04d\n",
				time1.hour, time1.min, time1.sec, time1.day, time1.mon, time1.year+2000,
				time2.hour, time2.min, time2.sec, time2.day, time2.mon, time2.year+2000 );
			return ERROR;
		}
		sysRtcGetDate( &time2.year,
					   &time2.mon,
					   &time2.day,
					   &time2.hour,
					   &time2.min,
					   &time2.sec );

	} while(  time1.sec == time2.sec );
	endTick = tickGet();

	/* compare tick with rtc */
	if((endTick < startTick)) /* rollover ? */
	{
		if(endTick > STRESS_TICK_TOLERANCE) /* end must be near zero */
		{
			printf("*** SysTick error (s: %u,e: %d ) ***\n", startTick, endTick);
			return ERROR;
		}
	}
	/* check coherency */
	if((endTick-startTick) > (sysAuxClkRateGet() + STRESS_TICK_TOLERANCE))
	{
		printf("*** SysTick and RTC not coherent (s: %u,e: %d ) ***\n", startTick, endTick);
		return ERROR;
	}

	printf("RTC --> OK (1s => %u ticks)\n",endTick-startTick);
	return OK;
}


