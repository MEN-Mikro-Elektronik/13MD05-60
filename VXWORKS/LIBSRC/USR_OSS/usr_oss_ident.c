/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_ident.c
 *      Project: User OSS functions
 *
 *      $Author: Franke $
 *        $Date: 1999/08/31 10:53:42 $
 *    $Revision: 1.1 $
 *
 *  Description: USER OSS IDENT - This module is used for version identification of UOS.
 *               It is a part of the USR_OSS library.
 *
 *     Required:
 *     Switches: --
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_ident.c,v $
 * Revision 1.1  1999/08/31 10:53:42  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char *IdentString="$Id: usr_oss_ident.c,v 1.1 1999/08/31 10:53:42 Franke Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"

/******************************** UOS_Ident *********************************
 *
 *  Description:  Return ident string of UOS module.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
char* UOS_Ident( void )
{
	return( (char*) IdentString );
}/*UOS_Ident*/
