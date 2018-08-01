/* This file is generated automatically. Do not edit! */
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/ll_defs.h>
#include <MEN/ll_entry.h>
#include <ll_entryi.h>

typedef struct
{
    char *name;
    void (* getEntry) ( LL_ENTRY* drvP );
}DRV_GETENTRY;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static DRV_GETENTRY LL_DrvTable[] =
{

  { "M22", M22_GetEntry },
  { "Z17", Z17_GetEntry },

 { NULL, NULL } /* do not remove this line */
};

/**************************** LL_FindEntry **********************************
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
int32 LL_FindEntry( char* devName, LL_ENTRY* drvP )
{
u_int32 index;
int32 retVal;

    retVal = 0;

    for( index=0;; index++)
    {
       if( LL_DrvTable[index].name == 0 )
       {
           retVal = ERR_MK_NO_LLDRV; /* error - not found */
           break;
       }/*if*/

       if( !OSS_StrCmp( NULL, LL_DrvTable[index].name, devName) )
       {
           LL_DrvTable[index].getEntry( drvP );
           break;
       }/*if*/
    }/*for*/

    return( retVal );
}/*LL_FindEntry*/

