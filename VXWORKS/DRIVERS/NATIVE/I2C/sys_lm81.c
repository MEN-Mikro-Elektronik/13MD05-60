/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sys_lm81.c
 *      Project: VxWorks BSPs
 *
 *       Author: kp
 *        $Date: 2007/09/05 13:53:29 $
 *    $Revision: 1.5 $
 *
 *  Description: Support routines for LM81 HW monitor.
 *				 This file should be included in sysLib.c
 *
 *				 BSP must provide sysLm81AnalogInputDesc table
 *
 *     Required:  -
 *     Switches:  SYS_LM81_BUSNO - the SMB bus number where LM81
 *								   is connected to
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_lm81.c,v $
 * Revision 1.5  2007/09/05 13:53:29  cs
 * fixed:
 *   - device locking semaphore did not work at all for VxWorks 6.x
 *     reason: semInit() obsolete in VxWorks 6.x
 *             and no error handling was implemented
 *     now: use semBCreate() ...
 *
 * Revision 1.4  2007/08/30 13:39:12  cs
 * added:
 *   + Lm81ReadReg() also print actual error code on SMB error
 *
 * Revision 1.3  2006/09/14 11:43:56  cs
 * fixed for apigen (vxW6.x):
 *    - function headers
 *
 * Revision 1.2  2006/02/22 11:29:46  ts
 * Function header edited for html generation
 * (vxW6.2 make man)
 *
 * Revision 1.1  2005/07/05 12:28:52  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef  LM81_SMB_ADDR
#	define LM81_SMB_ADDR		0x5e
#endif


/*
 * semaphore to prevent concurr. access.
 */
LOCAL SEM_ID G_lm81Lock;
#ifdef __LM81_VXWORKS_INIT__
LOCAL int G_lm81Init = 0;
#endif
LOCAL STATUS Lm81ReadReg( u_int8 cmdAddr, u_int8 *dataP )
{
	int32 error = 0;
	if( (error = sysSmbWriteByte( SYS_LM81_BUSNO, LM81_SMB_ADDR, cmdAddr )) == 0 ) {

		if( (error = sysSmbReadByte( SYS_LM81_BUSNO, LM81_SMB_ADDR, dataP )) == 0 )
			return OK;
	}
	printf("*** LM81 ReadReg error 0x%04x\n", error);
	return error;
}

LOCAL STATUS Lm81WriteReg( u_int8 cmdAddr, u_int8 data )
{
	return (sysSmbWriteTwoByte( SYS_LM81_BUSNO, LM81_SMB_ADDR, cmdAddr, data )
			== 0) ? OK: ERROR;
}
#if 0
LOCAL u_int8 Lm81VoltageToBin( int32 in, int32 fullScale )
{
	return (u_int8)(in * 255 / fullScale);
}
#endif
LOCAL int32 Lm81BinToVoltage( int32 in, int32 fullScale )
{
	return in * fullScale / 255;
}

/**********************************************************************
* sysLm81Init - Init LM81 access semaphore
*
* RETURNS: OK / ERROR
*/
STATUS sysLm81Init( void )
{
#ifdef __LM81_VXWORKS_INIT__
	u_int8 data;
#endif

	STATUS rv = ERROR;

#ifdef __LM81_VXWORKS_INIT__
	if(G_lm81Init)
		return (OK);
#endif
	G_lm81Lock = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
	if( G_lm81Lock == NULL ){
		logMsg("*** sysLm81Init: Error initializing locking semaphore\n",1,2,3,4,5,6);
		return rv;
	}
#ifdef __LM81_VXWORKS_INIT__
	if( semTake( G_lm81Lock, WAIT_FOREVER ) == ERROR ){
		printf("*** sysLm81Init: ERROR locking lm81\n");
		return rv;
	}

	if( Lm81ReadReg( LM81_CONF, &data ) == OK ){
		if( !(data & 1) ){
			/*clear initialization bit, int_clear bit and set enable bit*/
			if( Lm81WriteReg( LM81_CONF, (data & 0x77) | 1 ) == OK ){
				taskDelay(sysClkRateGet() >> 1); /*wait for 0.5 s*/
				G_lm81Init = 1;
				rv = OK;
			}
			else
				printf("write lm81 error\n");
		}
	}

	semGive( G_lm81Lock );

	return rv;
#else
	return (OK);
#endif
}

/**********************************************************************
* sysLm81Temp - Get LM81 temperature
*
* RETURNS: temperature in degrees celsius (or SYS_LM81_BAD_TEMPERATURE)
* on error
*/
int sysLm81Temp( void )
{
	int8 val;
	int rv = SYS_LM81_BAD_TEMPERATURE;

#ifdef __LM81_VXWORKS_INIT__
	if(!G_lm81Init){
		if(sysLm81Init() != OK)
			return (ERROR);
	}
#endif

	if( semTake( G_lm81Lock, WAIT_FOREVER ) == ERROR ){
		printf("*** sysLm81Temp: ERROR locking lm81\n");
		return( rv );
	}

	if( Lm81ReadReg( LM81_TEMP, (u_int8 *)&val ) == OK )
		rv = (int)val;

	semGive( G_lm81Lock );

	return rv;
}

/*********************************************************************
* sysLm81Voltage - get value of one analog input channel
*
* input:	one of LM81_2_5V .. LM81_VCCP2
*
* RETURNS: voltage in mV (or SYS_LM81_BAD_VOLTAGE)
*/
int sysLm81Voltage(
	int input
)
{
	u_int8 val;
	const LM81_ANALOG_INPUT_DESC *desc;
	int32 volt = SYS_LM81_BAD_VOLTAGE;
#ifdef __LM81_VXWORKS_INIT__
	if(!G_lm81Init){
		if(sysLm81Init() != OK)
			return (ERROR);
	}
#endif
	if( input < LM81_2_5V || input > LM81_VCCP2 )
		return SYS_LM81_BAD_VOLTAGE;

	if( semTake( G_lm81Lock, WAIT_FOREVER ) == ERROR ){
		printf("*** sysLm81Voltage: ERROR locking lm81\n");
		return( volt );
	}

	if( Lm81ReadReg( input, (u_int8 *)&val ) == OK ){
		desc = &sysLm81analogInputDesc[ input - LM81_2_5V ];

		volt = Lm81BinToVoltage( val, desc->fullScale );
	}

	semGive( G_lm81Lock );
	return volt;
}

#ifdef INCLUDE_SHOW_ROUTINES
/*********************************************************************
* sysLm81Show - Show LM81 voltages and temperature
*
*/
int sysLm81Show(
	int numLoops
)
{
	u_int8 reg;
	const LM81_ANALOG_INPUT_DESC *desc;
	int mV;

#ifdef __LM81_VXWORKS_INIT__
	if(!G_lm81Init){
		if(sysLm81Init() != OK)
			return (ERROR);
	}
#endif

	if( numLoops == 0 )
		numLoops = 1;

	while( numLoops-- ){
		desc = sysLm81analogInputDesc;
		reg=LM81_2_5V;

		while( desc->descr ){
			if( desc->descr[0] ){
				mV = sysLm81Voltage( reg );

				printf("%-6s: %3d.%03d 0x%08x\n", desc->descr, mV/1000, mV%1000, mV);
			}
			desc++;
			reg++;
		}
		{
			u_int8 val;
			Lm81ReadReg( LM81_TEMP, (u_int8 *)&val );
			printf("TEMP: %3d\n", sysLm81Temp());
		}
	}
	return 0;
}

#endif /* INCLUDE_SHOW_ROUTINES */
