/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_isa_pnp.c
 *      Project: OSS library
 *
 *       Author: Franke
 *        $Date: 1999/08/30 11:05:26 $
 *    $Revision: 1.1 $
 *
 *  Description: ISA PNP support
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_isa_pnp.c,v $
 * Revision 1.1  1999/08/30 11:05:26  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSid[]="$Id: oss_isa_pnp.c,v 1.1 1999/08/30 11:05:26 Franke Exp $";

#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

/**************************** OSS_IsaGetConfig ******************************
 *
 *  Description:  Gets ISAPNP configuration data.
 *
 *         Note:  Do nothing, for linking only.
 *---------------------------------------------------------------------------
 *  Input......:  osHdl			os handle
 *                cardVendId	card vendor id (3 character)                 
 *                cardProdId	card product number (4 hex digit)            
 *                devVendId		device vendor id (3 character)                 
 *                devProdId		device product number (4 hex digit)            
 *                devNbr		device number (decimal 0..9)
 *                resType       resource type (OSS_ISA_...)
 *                resNbr		resource number (decimal 0..9)
 *
 *  Output.....:  valueP        output value 
 *				  return		error code
 *
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_IsaGetConfig
(
		OSS_HANDLE	*osHdl,
		int8		*cardVendId,
		int16		cardProdId,
		int8		*devVendId,
		int16		devProdId,
		int32       devNbr,
		int32       resType,
		int32       resNbr,
		u_int32		*valueP
)
{
    DBGCMD( static const char functionName[] = "OSS_IrqLevelToVector"; )

	DBGWRT_1((DBH,"%s\n", functionName));

    DBGWRT_ERR( ( DBH, "%s%s: not implemented %s%d%s\n",
                  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr,
                  __LINE__, OSS_ErrorEndStr ));

	return( ERR_OSS_ISA_NO_PNPDATA );
}/*OSS_IsaGetConfig*/
