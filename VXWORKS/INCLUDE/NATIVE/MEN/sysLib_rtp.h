/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysLib_rtp.h
 *
 *       Author: cs
 *        $Date: 2014/02/05 13:12:23 $
 *    $Revision: 2.12 $
 *
 *  Description: RTP interface (syscall) definitions for MEN BSP functions
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Supported functions:
 *   By default the following functions are supported:
 *
 *   sysGetTemp( )
 *   sysGetVoltage( )
 *   sysRtcGetDate( )
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysLib_rtp.h,v $
 * Revision 2.12  2014/02/05 13:12:23  ts
 * R: data passing from kernel to RTP not possible without shared data
 * M: changed sysUbootEnvGet API of envval parameter to char* instead char**
 *
 * Revision 2.11  2014/01/28 15:09:20  ts
 * R: additional RTP calls for U-Boot Env., MTD and fpga_load needed
 * M: added syscall defines, intermediate work checkin
 *
 * Revision 2.10  2012/01/13 19:32:40  ts
 * R: build error in EM10A_RTP.vxe project occured, sysParamGet was unknown
 * M: added variable error in sysEnableLvds, added sysParamGet inline function
 *
 * Revision 2.9  2012/01/11 17:48:36  ts
 * R: RTP app build failed
 * M: corrected errornous last checkin, updated LVDS sysCall
 *
 * Revision 2.8  2011/11/18 20:25:07  ts
 * R: additional sysCall LvdsEnable was necessary
 * M: added definition of routine
 *
 * Revision 2.7  2008/07/09 17:37:14  CSchuster
 * R: general
 * M: cleaned up code
 *
 * Revision 2.6  2006/12/22 15:13:26  cs
 * added syscall for sysRtcGet()
 *
 * Revision 2.5  2006/10/10 10:48:02  cs
 * RTP bug (presence check failed) fixed by WindRiver, enable checks again
 *
 * Revision 2.4  2006/09/14 11:55:07  cs
 * fixed:
 *    - SYSCALL numbers for VxW 6.3
 *    - syscallPresent() doesn't work for VxW6.3, disable checking
 *
 * Revision 2.3  2006/03/15 11:10:06  cs
 * added sysRtcSet() inline function and SysCall number define
 *
 * Revision 2.2  2006/02/16 10:49:24  cs
 * avoid negative return values for sysGetTemp syscall
 *
 * Revision 2.1  2006/02/14 22:08:07  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	_SYSLIB_RTP_H
#define	_SYSLIB_RTP_H

#ifdef __cplusplus
	extern "C" {
#endif

#define SYS_RTP_SYSCALL_GROUP         5
									/**< BSPs system call group default 5 */
#define SYS_RTP_SYSCALL_GROUP_NAME	"BSP_Group"
									/**< BSPs system call group name */

#define SYS_RTP_BOARD_OFFSET          20
									/**< Board/BSP specific functions offset */

#define SYS_RTP_SYSCALL_NUM(rtn) (SYSCALL_NUMBER(SYS_RTP_SYSCALL_GROUP,rtn))


/* functions 0 - 0x100 are reserved for functions common to all/most MEN BSPS */
#define SYS_RTP_SYSCALL_RTCGETDATE			(SYS_RTP_SYSCALL_NUM(0))
#define SYS_RTP_SYSCALL_RTCSET					(SYS_RTP_SYSCALL_NUM(1))
#define SYS_RTP_SYSCALL_RTCGET					(SYS_RTP_SYSCALL_NUM(2))
#define SYS_RTP_SYSCALL_GETTEMP					(SYS_RTP_SYSCALL_NUM(3))
#define SYS_RTP_SYSCALL_GETVOLTAGE			(SYS_RTP_SYSCALL_NUM(4))
#define SYS_RTP_SYSCALL_ENABLELVDS			(SYS_RTP_SYSCALL_NUM(5))
#define SYS_RTP_SYSCALL_UBOOTEGET				(SYS_RTP_SYSCALL_NUM(6))
#define SYS_RTP_SYSCALL_UBOOTESET				(SYS_RTP_SYSCALL_NUM(7))
#define SYS_RTP_SYSCALL_UBOOTESAVE			(SYS_RTP_SYSCALL_NUM(8))
#define SYS_RTP_SYSCALL_MTDPARTPRG			(SYS_RTP_SYSCALL_NUM(9))
#define SYS_RTP_SYSCALL_FPGALOAD				(SYS_RTP_SYSCALL_NUM(10))
#define SYS_RTP_SYSCALL_SYSPARAMGET			(SYS_RTP_SYSCALL_NUM(22))
#ifndef _WRS_KERNEL

/* DEFINE little helpers */
#define SYS_MEN_PRINT_SC_PRESENCE_ERROR(_func,_scnum) \
	printf("*** %s: syscall %x not present\n", _func, _scnum );

#define SYS_MEN_PRINT_SC_MATCH_ERROR(_func,_scnum,_scname)                 \
	printf( "*** %s syscall %x: name %s does not match - looks like BSPs " \
			"system calls not installed or function is not supported\n",   \
			_func, _scnum, _scname );

#define SYS_MEN_SC_PRESENCE_CHECK(_func,_scnum)                 \
  {                                                             \
	char scName[0x100];                                         \
	size_t  size = sizeof(scName);                              \
                                                                \
	/* check installed system call */                           \
	error = syscallPresent( _scnum, scName, &size );            \
	if( error )                                                 \
	{                                                           \
		SYS_MEN_PRINT_SC_PRESENCE_ERROR( _func, _scnum );       \
		goto CLEANUP;                                           \
	}                                                           \
	if( strncmp( scName, _func, sizeof(_func) ) )               \
	{                                                           \
		error = -1;                                             \
		SYS_MEN_PRINT_SC_MATCH_ERROR(_func, _scnum, scName );   \
		goto CLEANUP;                                           \
	}                                                           \
  }

__inline__ int sysGetTemp( void )
{
	int temp = -273;
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_GETTEMP )

	/* sysGetTemp SC */
	temp = syscall( 1, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_GETTEMP ) - 273;

CLEANUP:
	return ( temp );
}

__inline__ int sysGetVoltage( int which )
{
	int mVolt = -1000;
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_GETVOLTAGE )

	/* sysGetVoltage SC */
	mVolt = syscall( (unsigned int)&which, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_GETVOLTAGE );

CLEANUP:
	return mVolt;
}

__inline__ int32 sysEnableLvds( int enableBoth )
{
	int32 error = OK;
	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_ENABLELVDS )

	syscall( (unsigned int)enableBoth, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_ENABLELVDS );

CLEANUP:
	return error;
}


__inline__ int sysParamGet(
	const char *parName,
	char *buf,
	int bufSize)
{
	int error = OK;

	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_SYSPARAMGET )

	/* sysParamGet SC */
	error = syscall( (unsigned int)parName, (unsigned int)buf, (unsigned int)bufSize, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_SYSPARAMGET );

CLEANUP:
	return error;
}

__inline__ int32 sysRtcGetDate(

u_int8 *yyP,				/* OUT: year   (0..99) */
	u_int8 *mmP,				/* OUT: month  (1..12) */
	u_int8 *ddP,				/* OUT: date   (1..31) */
	u_int8 *hourP,				/* OUT: hours  (0..23) */
	u_int8 *minP,				/* OUT: minutes(0..59) */
	u_int8 *secP				/* OUT: seconds(0..59) */
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_RTCGETDATE )

	/* sysRtcGetDate SC */
	error = syscall( (unsigned int)yyP, (unsigned int)mmP, (unsigned int)ddP, (unsigned int)hourP, (unsigned int)minP, (unsigned int)secP, 7, 8, SYS_RTP_SYSCALL_RTCGETDATE );
CLEANUP:
	return error;
}

__inline__ int32 sysRtcSet(
	u_int8 yy,				/* IN: year   (0..99) */
	u_int8 mm,				/* IN: month  (1..12) */
	u_int8 dd,				/* IN: date   (1..31) */
	u_int8 hour,			/* IN: hours  (0..23) */
	u_int8 min,				/* IN: minutes(0..59) */
	u_int8 sec				/* IN: seconds(0..59) */
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_RTCSET )

	/* sysRtcSet SC */
	error = syscall( (unsigned int)yy, (unsigned int)mm, (unsigned int)dd, (unsigned int)hour, (unsigned int)min, (unsigned int)sec, 7, 8, SYS_RTP_SYSCALL_RTCSET );

CLEANUP:
	return error;
}

__inline__ int32 sysRtcGet( void )
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_RTCGET )

	/* sysRtcGet SC */
	error = syscall( 1, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_RTCGET );

CLEANUP:
	return error;
}

__inline__ int32 sysUbootEnvGet(
	char *envname,				/* IN: variable name */
	char *envval,				/* IN: pointer to copy variable value to, if found */
	int  verbosity                          /* IN:0 or 1 */
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_UBOOTEGET )

	/* sysRtcSet SC */
	error = syscall( (unsigned int)envname,
			 (unsigned int)envval,
			 verbosity,
			 4, 5, 6, 7, 8,
			 SYS_RTP_SYSCALL_UBOOTEGET );

CLEANUP:
	return error;
}

__inline__ int32 sysUbootEnvSet(
	char *envname,				/* IN: variable name */
	char *envval,				/* IN: pointer to store value at */
	int  verbosity                          /* IN: 0 or 1, additional debugs */
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_UBOOTESET )

	/* sysRtcSet SC */
	error = syscall( (unsigned int)envname, (unsigned int)envval, verbosity, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_UBOOTESET );

CLEANUP:
	return error;
}

__inline__ int32 sysUbootEnvSave( void )
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_UBOOTESAVE )

	/* sysUbootEnvSave SC */
	error = syscall( 1, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_UBOOTESAVE );

CLEANUP:
	return error;
}

__inline__ int32 sysMtdPartProgram(
	char *partnameP,
	char *filenameP
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_MTDPARTPRG )

	error = syscall( (unsigned int)partnameP, (unsigned int)filenameP, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_MTDPARTPRG );
CLEANUP:
	return error;
}

__inline__ int32 fpga_load(
	char *optionsP
)
{
	int32 error = OK;

	/* check installed system call */
	SYS_MEN_SC_PRESENCE_CHECK( __FUNCTION__, SYS_RTP_SYSCALL_FPGALOAD )

	error = syscall( (unsigned int)optionsP, 2, 3, 4, 5, 6, 7, 8, SYS_RTP_SYSCALL_FPGALOAD );
CLEANUP:
	return error;
}

#endif /* _WRS_KERNEL */


#ifdef __cplusplus
	}
#endif

#endif /* _SYSLIB_RTP_H */


