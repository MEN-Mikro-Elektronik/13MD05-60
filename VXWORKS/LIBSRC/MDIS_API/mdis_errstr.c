/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: mdis_errstr.c
 *      Project: MDIS for VxWorks
 *
 *      $Author: ufranke $
 *        $Date: 2009/03/31 10:02:22 $
 *    $Revision: 1.5 $
 *
 *  Description: MDIS_ERRSTR - This module is a part of the common MDIS User-Interface.
 *               It provides a translation table and a function converting error codes
 *               to human readable error strings.
 *
 *     Required:  -
 *     Switches:  -
 *
 *---------------------------[ Public Functions ]----------------------------
 *
 *  char*  M_errstring( errCode )
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mdis_errstr.c,v $
 * Revision 1.5  2009/03/31 10:02:22  ufranke
 * cosmetics
 *
 * Revision 1.4  2006/07/20 14:51:56  ufranke
 * cosmetics
 *
 * Revision 1.3  2006/07/19 10:36:41  ufranke
 * changed
 *  - displaying low level driver error codes as device specific
 *    not longer as unkown MIDS errors
 *
 * Revision 1.2  2004/05/13 14:35:57  UFranke
 * added
 *  + M_errstringTs()
 *
 * Revision 1.1  1999/08/31 12:14:53  Franke
 * Initial Revision
 *
 * cloned by Revision 1.1  1998/03/10 12:22:09  franke of mdis_api.c
 * Because object level linking the big errstr table will only linked
 * to the image now, if needed.
 * Workaround for not reentrant M_errstring() function.
 * 
 *---------------------------------------------------------------------------
 * (c) Copyright 1995..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char *M_ERS_RCSid="$Id: mdis_errstr.c,v 1.5 2009/03/31 10:02:22 ufranke Exp $";

#include <MEN/men_typs.h>   /* MEN type definitions      */

#include <vxWorks.h>
#include <string.h>
#include <errno.h>
#include <errnoLib.h>
#include <stdio.h>

#include <MEN/mdis_err.h>   /* MDIS error codes          */
#include <MEN/mdis_ers.h>   /* MDIS error string table   */
#include <MEN/mdis_api.h>   /* MDIS api                  */

#define M_ERS_MAX_STR_SIZE 128
#define M_ERS_MAX_ENTRIES  10

static char M_ERS_errstring[M_ERS_MAX_STR_SIZE*M_ERS_MAX_ENTRIES];
static char *currErrStr = M_ERS_errstring;


/**********************************************************************/
/** Convert MDIS error code to static string
 * 
 * \copydoc mdis_api_specification.c::M_errstring()
 * \sa M_errstringTs
 */
char* M_errstring( int32 errCode )
{
	currErrStr += M_ERS_MAX_STR_SIZE;
	/* wrap around */
	if( (currErrStr-M_ERS_errstring) == (M_ERS_MAX_STR_SIZE*M_ERS_MAX_ENTRIES) )
		currErrStr = M_ERS_errstring;

    return( M_errstringTs( errCode, currErrStr ) );
}/*M_errstring*/

/**********************************************************************/
/** Convert MDIS error code to string
 * 
 * \copydoc mdis_api_specification.c::M_errstringTs()
 * \sa M_errstring
 */
char* M_errstringTs(int32 errCode, char *strBuf)
{
    u_int32     ii;

	*strBuf = 0;

    /*----------------------+
    |  known MDIS error     |
    +----------------------*/
	for(ii = 0; ii < MDIS_NBR_OF_ERR; ii++)
    {
    	if ( errCode == errStrTable[ii].errCode )
        {
		    sprintf(strBuf, "*** ERROR 0x%04x  %s", (int) errCode, errStrTable[ii].errString);
            goto CLEANUP;
        }/*if*/
    }/*for*/

    /*----------------------+
    |  device specific      |
    +----------------------*/
    if( errCode >= ERR_DEV  && errCode < ERR_DEV + 0xFF )
    {
        sprintf(strBuf,"*** ERROR 0x%04x  device specific error", (int) errCode);
        goto CLEANUP;
    }

    /*----------------------+
    |  unknown MDIS error   |
    +----------------------*/
    if( errCode >= ERR_OS  && errCode < ERR_END )
    {
        sprintf(strBuf,"*** ERROR 0x%04x  unknown MDIS error", (int) errCode);
        goto CLEANUP;
    }

    /*----------------------+
    |  else: system error   |
    +----------------------*/
    if( *strBuf == 0 )
        sprintf(strBuf,"*** ERROR 0x%x  operating system error", (int) errCode );

CLEANUP:
    return(strBuf);
}


