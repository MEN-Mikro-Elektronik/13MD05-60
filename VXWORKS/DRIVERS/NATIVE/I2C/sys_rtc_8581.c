/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sys_rtc_8581.c
 *      Project: VxWorks BSPs
 *
 *       Author: kp
 *        $Date: 2007/09/05 13:56:39 $
 *    $Revision: 1.7 $
 *
 *  Description: Routines to read and set the real time clock - EPSON 8581
 *				 This file should be included in sysLib.c
 *
 *
 *     Required: -
 *     Switches: SYS_RTC_BUSNO - the SMB bus number where RTC is connected to
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_rtc_8581.c,v $
 * Revision 1.7  2007/09/05 13:56:39  cs
 * fixed:
 *   - device locking semaphore did not work at all for VxWorks 6.x
 *     reason: semInit() obsolete in VxWorks 6.x
 *             and no error handling was implemented
 *     now: use semBCreate() ...
 *
 * Revision 1.6  2006/09/14 11:44:14  cs
 * fixed for apigen (vxW6.x):
 *    - function headers
 *
 * Revision 1.5  2006/04/27 10:29:38  cs
 * changed: sysRtcShow(): display RTC even if Low Voltage detected (plus warning)
 *
 * Revision 1.4  2006/02/22 11:31:37  ts
 * Function header edited for html generation
 * (vxW6.2 make man)
 *
 * Revision 1.3  2006/02/15 15:52:34  cs
 * added RTC reset to sysRtcSet (RTC might else get into illegel state)
 *
 * Revision 1.2  2006/02/08 16:08:59  ts
 * added error Message for low Voltage detect
 *
 * Revision 1.1  2005/06/23 08:33:53  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <MEN/rtc8581.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#define BCDTOBIN(v) ((((v)&0xf0)>>4)*10+((v)&0xf))
#define BINTOBCD(v) (((v)/10)*16+(((v)%10)))


/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */
/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
const int sysRtc_IIC_RETRIES = 3;
u_int32	sysRtcRetries = 0;
u_int32 sysSmbAddr = RTC8581_SMB_ADDR; /* RTC SMB address */

static SEM_ID RTC8581_Sem;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

static int sysRtcHwInit( void );


/******************************************************************************
*
* sysRtcInit - Routine to create the RTC semaphore and init RTC HW
*
* RETURNS: n/a
*
*/
STATUS sysRtcInit( void )
{
	/* init and give semaphores */
	RTC8581_Sem = semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
	if( RTC8581_Sem == NULL ){
		logMsg("*** sysRtcInit: Error initializing locking semaphore\n",1,2,3,4,5,6);
		return( ERROR );
	}

	sysRtcHwInit();
	semGive(RTC8581_Sem);
 	return( OK );
}

/******************************************************************************
*
* sysRtcReadReg - Read access function for RTC and NVRAM
*
* RETURNS: 0=ok | SMB lib error code
*
* SEE ALSO: sysRtcWriteReg()
*/
static int32 sysRtcReadReg(
	u_int16 addr,				/* byte address within RTC */
	u_int8 *valP				/* place to store read data */
)
{
	int retries = 0;
	int32 error;

	do
	{
		error = sysSmbWriteByte( SYS_RTC_BUSNO, sysSmbAddr, addr );
		if( !error )
			error = sysSmbReadByte( SYS_RTC_BUSNO, sysSmbAddr, valP );
	}
	while( error && ( retries++ < sysRtc_IIC_RETRIES ) );

	sysRtcRetries += retries;

	return error;
}

/******************************************************************************
*
* sysRtcWriteReg - Write access function for RTC and NVRAM
*
* RETURNS: 0=ok | SMB lib error code
*
* SEE ALSO: sysRtcReadReg()
*/
static int32 sysRtcWriteReg(
	u_int16 addr,				/* byte address within RTC */
	u_int8 val					/* data to write */
	)
{
	int retries = 0;
	int32 error;

	do
	{
		error = sysSmbWriteTwoByte( SYS_RTC_BUSNO, sysSmbAddr,
									addr, val );
	}
	while( error && ( retries++ < sysRtc_IIC_RETRIES ) );

	sysRtcRetries += retries;

	return error;
}

static int sysRtcHwInit( void )
{
	u_int8 val;
	int error;

	/*--- clear the test bit ---*/
	if( (error = sysRtcReadReg(  RTC8581_EXT, &val ))) {
		sysSmbAddr = RTC8803_SMB_ADDR;
		/* retry with 8803 address */
	} else if( (error = sysRtcReadReg(  RTC8581_EXT, &val )))
		return error;
	
	if( val & (RTC8581_EXT_TEST) ){
		val &= ~(RTC8581_EXT_TEST);
		if( (error = sysRtcWriteReg( RTC8581_EXT, val )))
			return error;
	}

	/*--- clear the stop/reset bit ---*/
	if( (error = sysRtcReadReg(  RTC8581_CONTROL, &val )))
		return error;

	if( val & (RTC8581_CONTROL_RESET | RTC8581_CONTROL_STOP) ){
		val &= ~(RTC8581_CONTROL_RESET | RTC8581_CONTROL_STOP);
		if( (error = sysRtcWriteReg( RTC8581_CONTROL, val )))
			return error;
	}

	return 0;
}

/******************************************************************************
*
* sysRtcGetDate - Read out date/time from RTC
*
* This routine does not update the system time.
* Returned time may be invalid if SYS_RTC_ERR_VLF error code returned. This
* error condition must be cleared by setting the RTC
*
* RETURNS: 0=ok | SMB lib error code | SYS_RTC_ERR_VLF (voltage low)
*
* SEE ALSO: sysRtcGet()
*/
int32 sysRtcGetDate(
	u_int8 *yyP,				/* OUT: year   (0..99) */
	u_int8 *mmP,				/* OUT: month  (1..12) */
	u_int8 *ddP,				/* OUT: date   (1..31) */
	u_int8 *hourP,				/* OUT: hours  (0..23) */
	u_int8 *minP,				/* OUT: minutes(0..59) */
	u_int8 *secP				/* OUT: seconds(0..59) */
)
{
	u_int8 regs[7], vlf, newSec;
	int error = 0;
	u_int32 i = 0;
	u_int32 timeout = 10000;

	/* lock RTC access */
	if( semTake( RTC8581_Sem, WAIT_FOREVER ) == ERROR ){
		logMsg("*** sysRtcGetDate: Error locking RTC access\n",1,2,3,4,5,6);
		goto CLEANUP;
	}

	if( (error = sysRtcReadReg( RTC8581_FLAG, &vlf )))
		goto CLEANUP;

	while( --timeout ){

		for( i=RTC8581_SEC; i <= RTC8581_YEAR; i++ ){
			if( (error = sysRtcReadReg( i, &regs[i] )))
				goto CLEANUP;
		}

		/* read second register again */
		if( (error = sysRtcReadReg( RTC8581_SEC, &newSec )))
			goto CLEANUP;

		if( newSec == regs[RTC8581_SEC] )
			/* RTC time is now stable */
			break;
	}

	*yyP 	= BCDTOBIN( regs[RTC8581_YEAR]);
	*mmP	= BCDTOBIN( regs[RTC8581_MONTH] 	& 0x1f );
	*ddP	= BCDTOBIN( regs[RTC8581_DAY] 	& 0x3f );
	*hourP	= BCDTOBIN( regs[RTC8581_HOUR] 	& 0x3f );
	*minP	= BCDTOBIN( regs[RTC8581_MIN] 	& 0x7f );
	*secP	= BCDTOBIN( regs[RTC8581_SEC]	& 0x7f );

	if( vlf & RTC8581_FLAG_VLF ){
		logMsg("*** sysRtcGetDate: RTC low Voltage detected!\n",
			   0 ,0 ,0 ,0 ,0 ,0 );
		error = SYS_RTC_ERR_VLF;
	}

CLEANUP:

	/* unlock RTC access */
	semGive( RTC8581_Sem );

	return error;
}

/******************************************************************************
*
* sysRtcShow - Display date and time of RTC
*
* RETURNS: N/A
*
* SEE ALSO: sysRtcGetDate()
*/
void sysRtcShow( void )
{
	int32 error;
	u_int8 yy, mm, dd, hour, min, sec;
	u_int32 year;

	error = sysRtcGetDate( &yy, &mm, &dd, &hour, &min, &sec);
	if( error == 0 || error == SYS_RTC_ERR_VLF ){
		year = yy + 1900;
		if( yy < 70 )
			year += 100;

		printf("%ld/%d/%d %02d:%02d:%02d\n",
			   year, mm, dd, hour, min, sec);

		if( error ) /* low voltage detected */
			printf("RTC low voltage detected! RTC may show wrong values!\n");
	}
	else {
		printf("Error %ld accessing RTC\n", error );
	}

}

/******************************************************************************
*
* sysRtcSet - Set date and time of RTC
* This routine sets the RTC. The system time is not touched.
*
* RETURNS: 0=ok | SMB lib error code
*
* SEE ALSO: sysRtcGetDate(), sysRtcGet()
*/
int32 sysRtcSet(
	u_int8 yy,					/* year   (0..99) */
	u_int8 mm,					/* month  (1..12) */
	u_int8 dd,					/* date   (1..31)  */
	u_int8 hour,				/* hours  (0..23) */
	u_int8 min,					/* minutes(0..59) */
	u_int8 sec					/* seconds(0..59) */
)
{
	u_int8 regs[8], val;
	int i, error = 0;

	/* lock RTC access */
	if( semTake( RTC8581_Sem, WAIT_FOREVER ) == ERROR ){
		logMsg("*** sysRtcSet: Error locking RTC access\n",1,2,3,4,5,6);
		goto CLEANUP;
	}

	/* stop and reset clock */
	if( (error = sysRtcReadReg( RTC8581_CONTROL, &val )))
		goto CLEANUP;

	if( (error = sysRtcWriteReg( RTC8581_CONTROL,
								 val |
								 RTC8581_CONTROL_STOP |
								 RTC8581_CONTROL_RESET)))
		goto CLEANUP;

	regs[RTC8581_SEC] 	= BINTOBCD(sec);
	regs[RTC8581_MIN] 	= BINTOBCD(min);
	regs[RTC8581_HOUR] 	= BINTOBCD(hour);
	regs[RTC8581_DAY] 	= BINTOBCD(dd);
	regs[RTC8581_MONTH] = BINTOBCD(mm);
	regs[RTC8581_YEAR] 	= BINTOBCD(yy);

	/*--- rewrite all registers ---*/
	for( i=RTC8581_SEC; i<=RTC8581_YEAR; i++ ){
		if( i != RTC8581_WEEK ){
			if( (error = sysRtcWriteReg( i, regs[i] )))
				goto CLEANUP;
		}
	}

	/*--- clear voltage low bit ---*/
	if( (error = sysRtcReadReg( RTC8581_FLAG, &val )))
		goto CLEANUP;

	val &= ~RTC8581_FLAG_VLF;
	if( (error = sysRtcWriteReg( RTC8581_FLAG, val)))
		goto CLEANUP;

CLEANUP:
	/* make sure clock not stopped */
	sysRtcHwInit();

	/* unlock RTC access */
	semGive( RTC8581_Sem );

	return error;
}

/******************************************************************************
*
* sysRtcGet - Get date and time from RTC and update system time
* This routine reads the time from the RTC. The retrieved date is then
* used as the system time for VxWorks
*
* RETURNS: 0=ok | SMB lib error code | ERROR if system clock set failed
*
* SEE ALSO: sysRtcGetDate(), sysRtcGet()
*/
int32 sysRtcGet( void )
{
	u_int8 yy, mm, dd, hour, min, sec;
	u_int32 year;
	struct timespec ts;
	struct tm tim;
	time_t secsSince1970;
	int32 error;

	error = sysRtcGetDate( &yy, &mm, &dd, &hour, &min, &sec);

	if( error == 0 ){
		year = yy;
		if( yy < 70 )
			year += 100;

		tim.tm_sec   =  sec;
		tim.tm_min   =  min;
		tim.tm_hour  =  hour;
		tim.tm_mday  =  dd;
		tim.tm_mon   =  mm - 1; /* january == 0 */
		tim.tm_year  =  year;
		tim.tm_isdst =  0;
		
		secsSince1970 = mktime( &tim ); /* calculate seconds since 1970 */

		if( secsSince1970 == (time_t)ERROR )
			return( ERROR );

		ts.tv_sec  = secsSince1970;
		ts.tv_nsec = 0;

		/* udate the system clock */
		error = clock_settime( CLOCK_REALTIME, &ts );
	}

	return error;
}

