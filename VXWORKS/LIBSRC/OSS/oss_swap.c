/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_swap.c
 *      Project: OSS library
 *
 *       Author: Schmidt
 *        $Date: 2008/09/05 13:48:19 $
 *    $Revision: 1.4 $
 *
 *  Description: Byte swap routines
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_swap.c,v $
 * Revision 1.4  2008/09/05 13:48:19  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 1.3  2005/04/12 16:56:13  kp
 * avoid name conflicts
 *
 * Revision 1.2  2000/10/17 11:08:10  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:17  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSidSwap[]="$Id: oss_swap.c,v 1.4 2008/09/05 13:48:19 ufranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

/****************************** OSS_Swap16 **********************************
 *
 *  Description:  Swap bytes in word.
 *
 *---------------------------------------------------------------------------
 *  Input......:  word      word to swap
 *  Output.....:  return    swapped word
 *  Globals....:  -
 ****************************************************************************/
u_int16 OSS_Swap16( u_int16 word )
{
    return( (u_int16)(word>>8) | (u_int16)(word<<8) );
}

/****************************** OSS_Swap32 **********************************
 *
 *  Description:  Swap bytes in double word.
 *
 *---------------------------------------------------------------------------
 *  Input......:  dword     double word to swap
 *  Output.....:  return    swapped word
 *  Globals....:  -
 ****************************************************************************/
u_int32 OSS_Swap32( u_int32 dword )
{
    return(  (dword>>24) | (dword<<24) |
             ((dword>>8) & 0x0000ff00) |
             ((dword<<8) & 0x00ff0000)   );
}
