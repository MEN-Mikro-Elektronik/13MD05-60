/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: mdis_api.c
 *      Project: MDIS for VxWorks
 *
 *      $Author: UFranke $
 *        $Date: 2010/09/02 16:33:55 $
 *    $Revision: 2.4 $
 *
 *  Description: MDIS_API - The MDIS user interface module.
 *               This module provides the common user callable MDIS functions.
 *				 Common means, that this interface is available on
 *               all operating system supporting MDIS like WinNT, OS9
 *               and VxWorks.
 *
 *     Required:  os2m.a, mk.a, oss.a
 *     Switches:  -
 *
 *---------------------------[ Public Functions ]----------------------------
 *
 *  int32  M_open(device)
 *  int32  M_close(path)
 *  int32  M_read(path,int_valueP)
 *  int32  M_write(path,int_value)
 *  int32  M_getstat(path,code,dataP)
 *  int32  M_setstat(path,code,data)
 *  int32  M_getblock(path,buffer,length)
 *  int32  M_setblock(path,buffer,length)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mdis_api.c,v $
 * Revision 2.4  2010/09/02 16:33:55  UFranke
 * R: DIAB linker error SDA VxWorks 6.7 PPC due to access to global variable
 * M: M_VX_RCSid is static now
 *
 * Revision 2.3  2009/03/31 10:04:23  ufranke
 * cosmetics
 *
 * Revision 2.2  2009/02/20 14:06:48  ufranke
 * added
 *  + prepared for MIPIOS
 *
 * Revision 2.1  2004/06/15 13:57:21  ufranke
 * changed
 *  - M_open, M_setblock new prototype
 *
 * Revision 2.0  2000/03/17 15:07:53  kp
 * Most MDIS_API functions call corresponding MK_ functions directly now
 *
 * Revision 1.2  1999/08/31 12:14:50  Franke
 * MDIS 4.3
 * removed use of the VxWorks I/O system
 *
 * Revision 1.1  1998/03/10 12:22:09  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1995-2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char *M_VX_RCSid="$Id: mdis_api.c,v 2.4 2010/09/02 16:33:55 UFranke Exp $";

#include <MEN/men_typs.h>   /* MEN type definitions      */

#include <vxWorks.h>
#include <iosLib.h>
#include <taskLib.h>
#include <string.h>
#include <errno.h>
#include <errnoLib.h>
#include <stdio.h>

#include <MEN/mdis_err.h>   /* MDIS error codes          */
#include <MEN/oss.h>        /* operating system services */
#include <MEN/dbg.h>        /* DBG_HANDLE  */
#include <MEN/desc.h>       /* descriptor manager        */
#include <MEN/os2m.h>       /* OS to MDIS glue           */
#include <MEN/mk.h>       	/* MDIS kernel protos        */

#include <MEN/mipios_vx_api.h>

#include <MEN/mdis_api.h>   /* MDIS api                  */

char* M_API_VX_RevId( void )
{
	return( (char*) M_VX_RCSid );
}

/******************************* M_open *************************************
 *
 *  Description:  Opens a path to a device.
 *
 *         Note:  Calls OS2M_Open().
 *                If it is the first path to the device it will initialized.
 *                ( see also MDIS Kernel      - MK_Open()
 *                           Low Level Driver - MXX_Init() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  device   name of the device e.g. "/m34/0"
 *
 *  Output.....:  return   ERROR or path number (file handle)
 *
 *  Globals....:  errno    error code
 *
 ****************************************************************************/
int32  M_open( const char *device )
{

    return( OS2M_Open( (char*)device ) );
}/*M_open*/

/******************************* M_close ************************************
 *
 *  Description:  Closes a path to a device.
 *
 *         Note:  Calls OS2M_Close().
 *                If it is the last open path of the device
 *                it will be deinitialized by the low level driver exit call.
 *                ( see also MDIS Kernel      - MK_Close()
 *                           Low Level Driver - MXX_Exit() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path        path number
 *
 *  Output.....:  return   ERROR or OK
 *
 *  Globals....:  errno    error code
 *
 ****************************************************************************/
int32  M_close( int32 path )
{
    return( OS2M_Close( path ) );
}/*M_close*/


/******************************* M_read *************************************
 *
 *  Description:  Reads a long from current channel of the path.
 *
 *         Note:  Calls MK_Read
 *                ( see also MDIS Kernel      - MK_Read()
 *                ( see also Low Level Driver - MXX_Read() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path        path number
 *                valueP      pointer to variable where read value stored
 *
 *  Output.....:  *int_valueP read value
 *                return      ERROR or OK
 *
 *  Globals....:  errno    error code
 *
 ****************************************************************************/
int32 M_read(int32 path, int32 *valueP )
{
	int32 error;

    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
		error = MK_Read( path, valueP );
	}
	else
    {
		error = MIPIOS_Read( path, valueP );
	}

    if( error )
    {
        OS2M_ErrnoSet( error );
        error = ERROR;
    }
	return error;
}/*M_read*/

/******************************* M_write ************************************
 *
 *  Description:  Writes a long from current channel of the path.
 *
 *         Note:  Calls MK_Write.
 *                ( see also MDIS Kernel      - MK_Write()
 *                ( see also Low Level Driver - MXX_Write() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path       path number
 *                value      value to write
 *
 *  Output.....:  return     ERROR or OK
 *
 *  Globals....:  errno      error code
 *
 ****************************************************************************/
int32 M_write(int32 path, int32 value )
{
	int32 error;

    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
		error = MK_Write( path, value );
	}
	else
	{
		error = MIPIOS_Write( path, value );
	}

   	if( error )
    {
   	    OS2M_ErrnoSet( error );
       	error = ERROR;
   	}
	return error;	
}/*M_write*/

/******************************* M_getstat **********************************
 *
 *  Description:  Gets a status parameter of the path. e.g. current channel
 *
 *         Note:  Calls MK_GetStat().
 *                ( see also MDIS Kernel      - MK_GetStat()
 *                ( see also Low Level Driver - MXX_GetStat() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path    path number
 *                code    specifies the obtained parameter ( see mdis_api.h )
 *                dataP   pointer to variable where status stored
 *
 *  Output.....:  *dataP  value of parameter
 *                return  ERROR or OK
 *
 *  Globals....:  errno   error code
 *
 ****************************************************************************/
int32 M_getstat( int32 path, int32 code, int32 *dataP )
{
	int32 error;

    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
		error = MK_GetStat( path, code, dataP );
	}
	else
	{
		error = MIPIOS_GetStat( path, code, dataP );
	}
	
    if( error )
    {
        OS2M_ErrnoSet( error );
        error = ERROR;
    }
	return error;	
}/*M_getstat*/

/******************************* M_setstat **********************************
 *
 *  Description:  Sets a status parameter of the path. e.g. current channel
 *
 *         Note:  Calls MK_SetStat().
 *                ( see also MDIS Kernel      - MK_SetStat()
 *                ( see also Low Level Driver - MXX_SetStat() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path    path number
 *                code    specifies the obtained parameter ( see mdis_api.h )
 *                data    new value of parameter
 *
 *  Output.....:  return  ERROR or OK
 *
 *  Globals....:  errno   error code
 *
 ****************************************************************************/
int32 M_setstat( int32 path, int32 code, int32 data )
{

    							 /* int - is machine depend
                                 thatsway BlockSetStat should work also
                                 on DEC alpha (64bit)
                                 the cutting because int32 should not
                                 a problem if the memory size is lower
                                 then 4GB and physical Addr = logical Addr */

	int32 error;

    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
    {
		error = MK_SetStat( path, code, data );
	}
	else
	{
		error = MIPIOS_SetStat( path, code, data );
	}

    if( error )
    {
        OS2M_ErrnoSet( error );
        error = ERROR;
    }
	return error;	
}/*M_setstat*/

/******************************** M_getblock ********************************
 *
 *  Description:  Reads a block of maximum <length> bytes from the path.
 *
 *         Note:  Calls MK_GetBlock().
 *                ( see also MDIS Kernel      - MK_GetBlock()
 *                ( see also Low Level Driver - MXX_BlockRead() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path     path number
 *                buffer   pointer to a buffer where data will be stored
 *                length   nbr of read bytes
 *
 *  Output.....:  return   ERROR or nbr of read bytes
 *
 *  Globals....:  errno    error code
 *
 ****************************************************************************/
int32 M_getblock( int32 path, u_int8 *buffer, int32 length )
{
	int32 nbrReadBytes, error;

    if( length )
    {
	    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
	    {
			error = MK_GetBlock( path, buffer, length, &nbrReadBytes );
		}
		else
		{
			error = MIPIOS_GetBlock( path, buffer, length, &nbrReadBytes );
		}

		if( error )
		{
			OS2M_ErrnoSet( error );
			return ERROR;
		}
			
		return nbrReadBytes;
    }
    else
    	return( 0 );
}/*M_getblock*/

/******************************** M_setblock ********************************
 *
 *  Description:  Writes a block of maximum <length> bytes to the path.
 *
 *         Note:  Calls MK_SetBlock.
 *                ( see also MDIS Kernel      - MK_SetBlock()
 *                ( see also Low Level Driver - MXX_BlockWrite() )
 *
 *---------------------------------------------------------------------------
 *  Input......:  path     path number
 *                buffer   pointer to a buffer where data are stored
 *                length   nbr of read bytes
 *
 *  Output.....:  return   ERROR or nbr of written bytes
 *
 *  Globals....:  errno    error code
 *
 ****************************************************************************/
int32 M_setblock( int32 path, const u_int8 *buffer, int32 length )
{
	int32 nbrWrittenBytes, error;

    if( length )
    {
	    if( path < OS2M_MIPIOS_FILE_HANDLE_START )
	    {
			error = MK_SetBlock( path, (u_int8*)buffer, length, &nbrWrittenBytes);
		}
		else
		{
			error = MIPIOS_SetBlock( path, (u_int8*)buffer, length, &nbrWrittenBytes);
		}
		
		if( error )
		{
			OS2M_ErrnoSet( error );
			return ERROR;
		}
			
		return nbrWrittenBytes;
    }
    else
    	return( 0 );
}/*M_setblock*/


