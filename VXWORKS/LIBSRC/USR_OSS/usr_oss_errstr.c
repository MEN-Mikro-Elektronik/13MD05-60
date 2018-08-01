/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  usr_oss_errstr.c
 *
 *      \author  uf
 *        $Date: 2005/06/29 15:08:23 $
 *    $Revision: 1.3 $
 * 
 * 	   \project  VxWorks USR-OSS library
 *
 *  	 \brief  translating error UOS codes to human readable error strings
 *
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_errstr.c,v $
 * Revision 1.3  2005/06/29 15:08:23  UFranke
 * cosmetics
 *
 * Revision 1.2  2004/05/13 14:37:55  UFranke
 * added
 *  - UOS_ErrStringTs()
 *
 * Revision 1.1  1999/08/31 10:53:40  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2004 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

const char *UOS_ERRSTR_RCSid="$Id: usr_oss_errstr.c,v 1.3 2005/06/29 15:08:23 UFranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#include <MEN/usr_ers.h>   /* UOS error string table   */
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"


#define UOS_ERS_MAX_STR_SIZE 128
#define UOS_ERS_MAX_ENTRIES  10

static char UOS_ERS_errstring[UOS_ERS_MAX_STR_SIZE*UOS_ERS_MAX_ENTRIES];
static char *currErrStr = UOS_ERS_errstring;

/**********************************************************************/
/** Convert UOS error number to static string
 * 
 * \copydoc usr_oss_specification.c::UOS_ErrString()
 *
 * \vxworks No special functionality.
 *
 * \sa UOS_ErrnoGet, UOS_ErrStringTs
 */
char* UOS_ErrString(int32 errCode)
{
	currErrStr += UOS_ERS_MAX_STR_SIZE;
	/* wrap around */
	if( (currErrStr-UOS_ERS_errstring) == (UOS_ERS_MAX_STR_SIZE*UOS_ERS_MAX_ENTRIES) )
		currErrStr = UOS_ERS_errstring;

	
    return( UOS_ErrStringTs( errCode, currErrStr ) );
}/*UOS_ErrString*/


/**********************************************************************/
/** Convert UOS error number to static string
 * 
 * \copydoc usr_oss_specification.c::UOS_ErrStringTs()
 *
 * \vxworks No special functionality.
 *
 * \sa UOS_ErrnoGet, UOS_ErrString
 */
char* UOS_ErrStringTs(int32 errCode, char *strBuf)
{
    u_int32     n;

	*strBuf = 0;
	
	/* search in table */
	for (n=0; n < UOS_NBR_OF_ERR; n++)
	{
		if (errCode == UOS_ErrStrTable[n].errCode) 
		{
			sprintf(strBuf,"ERROR (UOS) 0x%04x:  %s", 
					(unsigned int)errCode, UOS_ErrStrTable[n].errString);
			break;
		}/*if*/
	}/*for*/

	if( *strBuf == 0 )
		sprintf(strBuf,"ERROR (UOS) 0x%04x:  unknown error", (unsigned int)errCode);

    return(strBuf);
}

