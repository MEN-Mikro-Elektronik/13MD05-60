/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  mdis_api_rtp.c
 *
 *      \author  uf
 *        $Date: 2006/10/13 23:12:58 $
 *    $Revision: 1.4 $
 *
 *        \brief  libmdis_rtp.a MDIS system call wrapper for VxWorks 6.x RTPs
 *
 *            	  Some more detailed description please enter here
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mdis_api_rtp.c,v $
 * Revision 1.4  2006/10/13 23:12:58  cs
 * adapted call to MDIS_RTP_SYSCALL_M_OPEN to match new prototype
 *
 * Revision 1.3  2006/06/08 14:10:17  ufranke
 * cosmetics
 *
 * Revision 1.2  2006/06/02 10:11:10  ufranke
 * cosmetics
 *
 * Revision 1.1  2005/12/23 11:17:33  UFRANKE
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/
static const char RCSid[]= "$Id: mdis_api_rtp.c,v 1.4 2006/10/13 23:12:58 cs Exp $";

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>


#include <sysLib.h> /* user mode sysLib.h */
#include <stdlib.h>
#include <syscall.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_rtp.h>

/**********************************************************************/
/** Open path to device
 *
 * \copydoc mdis_api_specification.c::M_open()
 *
 * \vxworks Userland Wrapper.
 *          Checks if MDIS system call is installed.
 *
 * \sa M_close
 */
int32  M_open( const char *device )
{
	char scName[0x100];
	size_t  size = sizeof(scName);
	int32 error;

	/* check installed system call */
	error = syscallPresent( MDIS_RTP_SYSCALL_M_OPEN, scName, &size );
	if( error )
	{
		printf("*** %s syscallPresent() syscall %x\n",
				__FUNCTION__,
		        MDIS_RTP_SYSCALL_M_OPEN );
		goto CLEANUP;
	}
	if( strncmp( scName, "M_open", 6 ) )
	{
		error = -1;
		printf("*** %s syscall %x %s not M_open - looks like MDIS system calls not installed\n",
				__FUNCTION__,
				MDIS_RTP_SYSCALL_M_OPEN, scName );
		goto CLEANUP;
	}

	/* M_open SC */
	error = syscall( (int)strlen(device),
					 (int)device,
					 0,0,0,0,0,0,
					 MDIS_RTP_SYSCALL_M_OPEN );

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Close path to device
 *
 * \copydoc mdis_api_specification.c::M_close()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_open
 */
int32  M_close( int32 path )
{
	return( syscall( path,2,3,4,5,6,7,8, MDIS_RTP_SYSCALL_M_CLOSE ));
}

/**********************************************************************/
/** Read 32-bit integer value from device
 *
 * \copydoc mdis_api_specification.c::M_read()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_getblock, M_write
 */
int32  M_read( int32 path, int32 *valueP )
{
	return( syscall( path, (int)valueP,3,4,5,6,7,8, MDIS_RTP_SYSCALL_M_READ ));
}

/**********************************************************************/
/** Write 32-bit integer value to device
 *
 * \copydoc mdis_api_specification.c::M_write()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_setblock, M_read
 */
int32  M_write( int32 path, int32 value )
{
	return( syscall( path, value,3,4,5,6,7,8, MDIS_RTP_SYSCALL_M_WRITE ));
}

/**********************************************************************/
/** Get status from device
 *
 * \copydoc mdis_api_specification.c::M_getstat()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_setstat
 */
int32 M_getstat( int32 path, int32 code, int32 *dataP )
{
	return( syscall( path, code, (int)dataP,4,5,6,7,8, MDIS_RTP_SYSCALL_M_GETSTAT ));
}

/**********************************************************************/
/** Set status of device
 *
 * \copydoc mdis_api_specification.c::M_setstat()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_getstat
 */
int32 M_setstat( int32 path, int32 code, int32 data )
{
	return( syscall( path, code, data,4,5,6,7,8, MDIS_RTP_SYSCALL_M_SETSTAT ));
}

/**********************************************************************/
/** Read data block from device
 *
 * \copydoc mdis_api_specification.c::M_getblock()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_read, M_setblock
 */
int32 M_getblock( int32 path, u_int8 *buffer, int32 length )
{
	return( syscall( path, (int)buffer, length,4,5,6,7,8, MDIS_RTP_SYSCALL_M_GETBLOCK ));
}

/**********************************************************************/
/** Write data block to device
 *
 * \copydoc mdis_api_specification.c::M_setblock()
 *
 * \vxworks Userland Wrapper.
 *
 * \sa M_write, M_getblock
 */
int32 M_setblock( int32 path, const u_int8 *buffer, int32 length )
{
	return( syscall( path, (int)buffer, length,4,5,6,7,8, MDIS_RTP_SYSCALL_M_SETBLOCK ));
}

