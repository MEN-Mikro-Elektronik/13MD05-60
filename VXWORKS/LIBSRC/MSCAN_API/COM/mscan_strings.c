/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mscan_strings.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2003/04/02 08:37:13 $
 *    $Revision: 1.2 $
 * 
 *  	 \brief  number to string conversion routines for MSCAN_API
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mscan_strings.c,v $
 * Revision 1.2  2003/04/02 08:37:13  kp
 * added __MAPILIB to all lib functions
 *
 * Revision 1.1  2003/01/29 14:03:15  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static const char _RCSid[] = "$Id: mscan_strings.c,v 1.2 2003/04/02 08:37:13 kp Exp $";

#include <stdio.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/mdis_api.h>
#include <MEN/mscan_api.h>
#include <MEN/mscan_drv.h>

/**********************************************************************/
/** Convert API error code to string
 *	
 * This converts the MSCAN or MDIS error codes into strings, i.e. those
 * errors contained in \em errno.
 *
 * \remark A pointer to a static string buffer is returned. Not thread safe!
 *
 * \param 	error      error code returned from MSCAN API functions
 * \return	error message string (static buffer!)
 *
 */
char * __MAPILIB mscan_errmsg(int32 error)
{
	char *str;
	static char errMsg[128];

	switch(error) {
	case MSCAN_ERR_BADSPEED:		str="bitrate not supported"; break;
	case MSCAN_ERR_NOMESSAGE:     	str="no frame in receive buffer"; break;
	case MSCAN_ERR_BADTMDETAILS:	str="illegal timing details"; break;
	case MSCAN_ERR_BADMSGNUM:		str="illegal message object number"; break;
	case MSCAN_ERR_BADDIR:			str="illegal message object direction"; 
		break;
	case MSCAN_ERR_QFULL:			str="message FIFO full"; break;
	case MSCAN_ERR_SIGBUSY:			str="signal installation problem"; break;
	case MSCAN_ERR_BADPARAMETER:	str="bad parameter"; break;
	case MSCAN_ERR_NOTINIT: str="controller not completely initialized"; break;
	case MSCAN_ERR_ONLINE:			str="controller not disabled"; break;
	default:
		str = NULL;
	}

	if( str != NULL ){
		sprintf(errMsg,"ERROR (MSCAN) 0x%04lx:  %s",error, str);
		return errMsg;
	}
	/*--- if unknown, use MDIS error message ---*/
	return M_errstring(error);
}

/**********************************************************************/
/** Convert code from error FIFO to string
 *	
 * This converts the codes reported from the error FIFO into strings
 *
 * \remark Constant strings are returned. Thread safe.
 *
 * \param 	errCode		error code from error FIFO
 *
 * \return	error message string
 *
 */
const char * __MAPILIB mscan_errobj_msg( u_int32 errCode  )
{
	const char *msgs[] = {
		"(undefined)",
		"controller entered bus off state",
		"controller left bus off state",
		"controller entered warning state",
		"controller left warning state",
		"object's receive fifo overflowed",
		"controller's FIFO overflowed"
	};

	if( errCode > MSCAN_DATA_OVERRUN )
		errCode = 0;
	
	return msgs[errCode];
}

