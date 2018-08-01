/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysLib_em01_rtp.h
 *
 *       Author: cs
 *        $Date: 2008/07/09 17:36:16 $
 *    $Revision: 2.4 $
 *
 *  Description: RTP interface (syscall) definitions for MEN EM1 BSP functions
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Supported functions:
 *   By default the following functions are supported:
 *  sysLedUser()
 *  sysHBLSpawn()
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysLib_em01_rtp.h,v $
 * Revision 2.4  2008/07/09 17:36:16  CSchuster
 * R: 1. LM81 calibration was added to BSP
 *    2. general improvement
 * M: 1. add SCs for sysLm81Calib() and sysLm81CalibStatus()
 *    2a. reenabled checking of SC presence in system
 *     b. cleaned up code
 *
 * Revision 2.3  2006/09/14 11:54:53  cs
 * fixed:
 *    - SYSCALL numbers for VxW 6.3
 *    - syscallPresent() doesn't work for VxW6.3, disable checking
 *
 * Revision 2.2  2006/04/27 10:43:26  cs
 * added syscall sysParamGet()
 *
 * Revision 2.1  2006/02/14 22:08:08  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	_SYSLIB_EM01_RTP_H
#define	_SYSLIB_EM01_RTP_H

#ifndef _SYSLIB_RTP_H
	#include <MEN/sysLib_rtp.h>
#endif


#ifdef __cplusplus
	extern "C" {
#endif


/* functions 0 - SYS_RTP_BOARD_OFFSET (20) are reserved for functions
 * common to all/most MEN BSPs */
#define SYS_RTP_SYSCALL_LEDUSR				SYS_RTP_SYSCALL_NUM(SYS_RTP_BOARD_OFFSET + 0)
#define SYS_RTP_SYSCALL_HEARTBEATLED		SYS_RTP_SYSCALL_NUM(SYS_RTP_BOARD_OFFSET + 1)
#define SYS_RTP_SYSCALL_SYSPARAMGET			SYS_RTP_SYSCALL_NUM(SYS_RTP_BOARD_OFFSET + 2)
#define SYS_RTP_SYSCALL_SYSLM81CALIB		SYS_RTP_SYSCALL_NUM(SYS_RTP_BOARD_OFFSET + 3)
#define SYS_RTP_SYSCALL_SYSLM81CALIBSTATUS	SYS_RTP_SYSCALL_NUM(SYS_RTP_BOARD_OFFSET + 4)


#ifndef _WRS_KERNEL

__inline__ void sysLedUser( int on )
{
	int32 error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_LEDUSR )

	/* sysLedUser SC */
	error = syscall( (unsigned int)&on, 2, 3, 4, 5, 6, 7, 8,
					 SYS_RTP_SYSCALL_LEDUSR );

CLEANUP:
	return;
}

__inline__ void sysHBLSpawn( void )
{
	int32 error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_HEARTBEATLED )

	/* sysHBLSpawn SC */
	error = syscall( 1, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_HEARTBEATLED );

CLEANUP:
	return;
}

__inline__ int sysParamGet(
	const char *parName,
	char *buf,
	int bufSize)
{
	int error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_SYSPARAMGET )

	/* sysParamGet SC */
	error = syscall( (unsigned int)parName,
					 (unsigned int)buf,
					 (unsigned int)bufSize,
					 4, 5, 6, 7, 8,
					 SYS_RTP_SYSCALL_SYSPARAMGET );

CLEANUP:
	return error;
}

__inline__ STATUS sysLm81Calib(
	int meas_2V5,	/* measured/real 2.5 Volt in mV */
	int meas_1V5,	/* measured/real 1.5 Volt in mV */
	int meas_3V3,	/* measured/real 3.3 Volt in mV */
	int meas_5V0, 	/* measured/real 5   Volt in mV */
	int meas_1V2 	/* measured/real 1.2 Volt in mV */
)
{
	int32 error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_SYSLM81CALIB )

	/* sysLm81Calib SC */
	error = syscall( (unsigned int)meas_2V5,
					 (unsigned int)meas_1V5,
					 (unsigned int)meas_3V3,
					 (unsigned int)meas_5V0,
					 (unsigned int)meas_1V2,
					 6, 7, 8,
					 SYS_RTP_SYSCALL_SYSLM81CALIB );

CLEANUP:
	return error;
}

__inline__ BOOL sysLm81CalibStatus( void )
{
	int32 error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_SYSLM81CALIBSTATUS )

	/* sysLm81Calib SC */
	error = syscall( 1, 2, 3, 4, 5, 6, 7, 8,
					 SYS_RTP_SYSCALL_SYSLM81CALIBSTATUS );

CLEANUP:
	return error;
}

#endif /* _WRS_KERNEL */


#ifdef __cplusplus
	}
#endif

#endif /* _SYSLIB_MEN_RTP_H */


