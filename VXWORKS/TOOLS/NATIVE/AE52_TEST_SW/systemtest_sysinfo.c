        
/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest_sysinfo.c
 *
 *      \author  Maximilian Kolpak
 *        $Date: 2012/09/06 15:32:03 $
 *    $Revision: 1.2 $
 *
 *        \brief  System information routines for stress test.
 *                Functions shall not invoke time consuming hardware routines.
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_sysinfo.c,v $
 * Revision 1.2  2012/09/06 15:32:03  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:07  MKolpak
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2011 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
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


/*--------------------------------------------------------------+
 |	GLOBALS														|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/

/* 
 * System information functions
 * last entry must be NULL!
 */
 STRESS_SI_TYPE G_stressSiList[] = 
 {
	{"TIME",	stress_si_time,	""},
	{"DATE",	stress_si_date,	""},
	{"TEST",	stress_si_ttime,""},
	{"T_CP",	stress_si_temp1,"°C"},
	{"T_AE",	stress_si_temp2,"°C"},
	{"T_SM",	stress_si_temp3,"°C"},
	{"INT",		stress_si_int, 	"%"},
	{"RAM",		stress_si_ram, 	"kB"},
	{"",		stress_si_space,""},
	{"1V  ",	stress_si_v1,	"V"},
	{"1VA ",	stress_si_v1a,	"V"},
	{"1,2V",	stress_si_v1_2,	"V"},
	{"1,2VA",	stress_si_v1_2a,"V"},
	{"1,8V",	stress_si_v1_8,	"V"},
	{"2,5V",	stress_si_v2_5,	"V"},
	{"3,3V",	stress_si_v3_3,	"V"},
	{"5,1V",	stress_si_v5_1,	"V"},
	{"-12V",	stress_si_vm12,	"V"},
	{"12V",		stress_si_v12,	"V"},
	{"-15V",	stress_si_vm15,	"V"},
	{"15V",	 	stress_si_v15,	"V"},
	{"", (STRESS_SI_FUNC) NULL}
 };

int stress_si_update(STRESS_LOG_HDL * lP)
{
	int i = 0;
	while(G_stressSiList[i].func != NULL)
	{
		G_stressSiList[i].func(G_stressSiList[i].valstr,lP);
		i++;
	}
	return OK;
}

/**********************************************************************/
/** Routine to print system time into a string buffer. 
 *  We expect the time structure has been updated.
 *
 *  \param sP	pointer to string buffer
 *  \param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_time(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;

	sprintf(sP,"%02d:%02d:%02d", 
			lP->curr_time.hour,
			lP->curr_time.min,
			lP->curr_time.sec );

	return OK;
}

/**********************************************************************/
/** Routine to print system date into a string buffer. 
 *  We expect the time structure has been updated.
 *
 *  \param sP	pointer to string buffer
 *  \param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_date(char * sP, STRESS_LOG_HDL * lP)
{	
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%02d.%02d.%02d", 
			lP->curr_time.day,
			lP->curr_time.mon,
			lP->curr_time.year );

	return OK;
}

/**********************************************************************/
/** Routine to print test time into a string buffer. 
 *  We expect the time structure has been updated.
 *
 *  \param sP	pointer to string buffer
 *  \param lP	pointer to STRESS_LOG_HDL
 *
 */
	int stress_si_ttime(char * sP, STRESS_LOG_HDL * lP)
	{	
		/* not nice - but legacy */
#ifdef STRESS_TESTTIME_LONG
		u_int8 years = 0;
		u_int8 months = 0;
#endif
		u_int8 days=0;
		u_int8 hours=0;
		u_int8 min=0;
		u_int8 sec=0;
		u_int32 temp = 0;
		u_int32 testSeconds = 0;
		char buf[30] = {0};
	
		if(sP == NULL)
			return ERROR;
	
		if( G_haltSystem )
		{
			sprintf( sP, "stopped");
			return OK;
		}
	
		testSeconds = getSecondsFromTime( &lP->curr_time ) - getSecondsFromTime( &lP->start_time);
	
#ifdef STRESS_TESTTIME_LONG
		/* get years */
		temp = testSeconds / (SECONDS_PER_HOUR*24*31*12);
		if( temp ){
			years = temp;
			testSeconds -= (years*(SECONDS_PER_HOUR*24*31*12));
		}
	
		/* get months */
		temp = testSeconds / (SECONDS_PER_HOUR*24*31);
		if( temp ){
			months = temp;
			testSeconds -= (months*(SECONDS_PER_HOUR*24*31));
		}
#endif
		/* get days */
		temp = testSeconds / (SECONDS_PER_HOUR*24);
		if( temp ){
			days = temp;
			testSeconds -= (days*(SECONDS_PER_HOUR*24));
		}
	
		/* get hours */
		temp = testSeconds / (SECONDS_PER_HOUR);
		if( temp ){
			hours = temp;
			testSeconds -= (hours*(SECONDS_PER_HOUR));
		}
	
		/* get minutes */
		temp = testSeconds / 60;
		if( temp ){
			min = temp;
			testSeconds -= 60*min;
		}
	
		sec = testSeconds;
		
#ifdef STRESS_TESTTIME_LONG
		if( years )
			sprintf( buf, "%2dY %02dM %d02D %02d:%02d:%02d", years, months, days, hours, min, sec );
		else if( months )
			sprintf( buf, "%02dM %02dD %02d:%02d:%02d", months, days, hours, min, sec );
		else
#endif
		if( days )
			sprintf( buf, "%2dD %02d:%02d:%02d", days, hours, min, sec );
		else if( hours )
			sprintf( buf, "%02d:%02d:%02d", hours, min, sec );
		else if( min )
			sprintf( buf, "%02d:%02d", min, sec );
		else if( sec )
			sprintf( buf, "%01d s", sec );
	
		strcpy(sP,buf);
	
		return OK;
	}

/**********************************************************************/
/** Routine to print temperature into a string buffer. 
 *  We expect the temperature has been updated.
 *
 *  \param sP	pointer to string buffer
 *  \param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_temp1(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%d", lP->temp1);

	return OK;
}
int stress_si_temp2(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%d", lP->temp2);

	return OK;
}
int stress_si_temp3(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%d", lP->temp3);

	return OK;
}



/**********************************************************************/
/** Routine to print logging interval into a string buffer. 
 *
 *  \param sP	pointer to string buffer
 *  \param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_logint(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	if( lP->time == 0 )
	{
		sprintf(sP,"DISABLED" );
	}
	else if( lP->time == 0xFFFFFFFF )
	{
		sprintf(sP,"ERROR" );
	}
	else
	{
		sprintf(sP,"%d", lP->time );
	}

	return OK;
}
	
/**********************************************************************/
/** Routine to print idle percentage into a string buffer. 
 *
 *	\param sP	pointer to string buffer
 *	\param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_idle(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%2d", stressGetIdle());

	return OK;
}
	
/**********************************************************************/
/** Routine to print interrupt percentage  into a string buffer. 
 *
 *	\param sP	pointer to string buffer
 *	\param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_int(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"%2d", stressGetInt());

	return OK;
}

int stress_si_ram(char * sP, STRESS_LOG_HDL * lP)
{
	static MEM_PART_STATS memstat = {0};
	
	if(sP == NULL)
		return ERROR;

	memInfoGet(&memstat);
	
	sprintf(sP,"%u", memstat.numBytesFree >> 10);

	return OK;
}


/**********************************************************************/
/** Routine to print empty line into a string buffer. 
 *
 *	\param sP	pointer to string buffer
 *	\param lP	pointer to STRESS_LOG_HDL
 *
 */
int stress_si_space(char * sP, STRESS_LOG_HDL * lP)
{
	if(sP == NULL)
		return ERROR;
				
	sprintf(sP,"\n");

	return OK;
}

int stress_si_v1(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 0;
	
	if(sP == NULL)
		return ERROR;
	
	sprintf(sP," %7.3f",g_voltages[i]);

	return OK;
}

int stress_si_v1a(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 1;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);

	return OK;
}

int stress_si_v1_2(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 2;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);

	return OK;
}

int stress_si_v1_2a(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 3;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);

	return OK;
}

int stress_si_v1_8(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 4;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_v2_5(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 5;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_v3_3(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 6;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_v5_1(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 7;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_vm12(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 8;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}


int stress_si_v12(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 9;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_vm15(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 10;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

int stress_si_v15(char * sP, STRESS_LOG_HDL * lP)
{
	char i = 11;
	
	if(sP == NULL)
		return ERROR;
				
	
	sprintf(sP," %7.3f",g_voltages[i]);
	
	return OK;
}

