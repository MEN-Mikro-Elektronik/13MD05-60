/* This file is generated automatically. Do not edit! */
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/bb_defs.h>
#include <MEN/bb_entry.h>
#include <bb_entryi.h>

typedef struct
{
    char *name;
    void (* getEntry) ( BBIS_ENTRY* drvP );
}DRV_GETENTRY;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static DRV_GETENTRY BB_DrvTable[] =
{

  { "CHAMELEON_PCITBL", CHAMELEON_GetEntry },
  { "ISA", ISA_GetEntry },
  { "SMBPCI_NAT", SMB_NATIVE_GetEntry },

 { NULL, NULL } /* do not remove this line */
};

/**************************** BB_FindEntry **********************************
 *
 *  Description:  gets the ll driver routine entrys.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devName    device name
 *
 *  Output.....:  drvP       ll drv entrys
 *                return 0 | error
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 BBIS_FindEntry( char* devName, BBIS_ENTRY* drvP )
{
u_int32 index;
int32 retVal;

    retVal = 0;

    for( index=0;; index++)
    {
       if( BB_DrvTable[index].name == 0 )
       {
           retVal = ERR_BK_NO_LLDRV; /* error - not found */
           break;
       }/*if*/

       if( !OSS_StrCmp( NULL, BB_DrvTable[index].name, devName) )
       {
           BB_DrvTable[index].getEntry( drvP );
           break;
       }/*if*/
    }/*for*/

    return( retVal );
}/*BB_FindEntry*/

