/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: bk.c
 *      Project: MDIS Module Driver Interface System 4.x
 *               and other bbis using drivers
 *
 *       Author: uf
 *        $Date: 2009/03/31 10:57:06 $
 *    $Revision: 1.15 $
 *
 *  Description: VxWorks BBIS kernel
 *				 The BBIS kernel exist once per CPU and contains data
 *				 about the used base boards in the system.
 *
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: bk.c,v $
 * Revision 1.15  2009/03/31 10:57:06  ufranke
 * cosmetics
 *
 * Revision 1.14  2006/07/17 14:28:09  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 1.13  2006/02/10 16:17:32  UFRANKE
 * fixed
 *  - page fault due to NULL pointer access
 *    at PENTIUM under VxWorks 6.1
 *
 * Revision 1.12  2005/06/29 15:05:22  UFranke
 * fixed
 *  + BK_ConsistencyCheck()
 *
 * Revision 1.11  2005/06/20 17:03:08  UFranke
 * added
 *  + BK_ConsistencyCheck()
 *
 * Revision 1.10  2002/04/16 16:58:10  Franke
 * bugfix in BK_UnGetBBHandle() if the first initialize baseboard was'nt
 *        deinitialized at least (BK_BaseBoardListAnchor).
 *
 * Revision 1.9  2000/03/16 16:01:57  kp
 * removed comparison of first 4 chars between device name and driver name
 *
 * Revision 1.8  1999/08/30 11:21:53  Franke
 * added DEBUG_LEVEL_BK decoded
 *
 * Revision 1.7  1998/07/30 09:59:20  Franke
 * cosmetics
 *
 * Revision 1.6  1998/06/23 17:30:24  Franke
 * changed BBIS kernel semaphore is created by semBCreate()
 *         to support OSS MEMTEST facility
 *
 * Revision 1.5  1998/06/17 16:06:57  Franke
 * added   in BK_GetBBHandle() now check first 4 chars of
 *                             boardNameInstanz == HW_TYPE
 * changed MDIS 4.1 debug handling
 *
 * Revision 1.4  1998/03/19 14:34:30  franke
 * BK prepared for not MDIS drivers (linked list semaphore)
 * Bug in linked list handling removed
 *
 * Revision 1.3  1998/03/18 17:10:42  franke
 * BK_Show routine added
 *
 * Revision 1.2  1998/03/16 11:11:49  franke
 * BRD_HANDLER descriptor entry removed - now using HW_TYPE
 *
 * Revision 1.1  1998/03/10 12:05:38  franke
 * Added by mcvs
 *
 * Revision 1.1  1998/02/20 15:32:31  uf
 * initial
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ***************************************************************************/
char *BK_IdentString = "BK - BBIS Kernel: $Id: bk.c,v 1.15 2009/03/31 10:57:06 ufranke Exp $";

#include <MEN/men_typs.h>	/* men specific type definitions */
#include <string.h>
#include <MEN/mdis_err.h>   /* error codes */
#include <MEN/oss.h>        /* operating system services */
#include <MEN/bb_defs.h>	/* bbis definitions */

#define DBG_MYLEVEL		    BK_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include <MEN/desc.h>       /* descriptor manager */
#include <MEN/bb_entry.h>	/* bbis function entry structure */
#include <MEN/bk.h>         /* bbis kernel */

#ifdef DBG
	#define  DBH dbgHdl
	u_int32  BK_DbgLev = OSS_DBG_DEFAULT;
	static   DBG_HANDLE   *dbgHdl = NULL;
#endif /*DBG*/
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
static BK_BASE_BOARD_LIST_ELEMENT  BK_BaseBoardListAnchor = {NULL,NULL,NULL};
static SEM_ID  BK_SemHdl = NULL;
DBGCMD( static char errorStartStr[] = "*** "; )
DBGCMD( static char errorLineStr[]   = " (line ";     )
DBGCMD( static char errorEndStr[]   = ")\n";       )

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

/************************** BK_ConsistencyCheck **********************************
 *
 *  Description: Checks if bbDescSpec already exists.
 *
 *---------------------------------------------------------------------------
 *  Input......: boardNameInstanz board name e.g. "D201_1"
 *               bbDescSpec       BBIS descriptor specifier
 *
 *               return           0 or error code
 *
 *  Globals....: BK_BaseBoardListAnchor
 *
 ****************************************************************************/
int32 BK_ConsistencyCheck
(
    char                       *boardNameInstanz,
    DESC_SPEC                  *bbDescSpec
)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
    int32 error = 0;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElBefore;


	/*--------------------------------------+
    |  looking if descriptor already exists |
    +--------------------------------------*/
    bbListElement = &BK_BaseBoardListAnchor;
    bbListElBefore = NULL;
    while( bbListElement && bbListElement->bbData )
    {
        if( bbListElement->bbData->descP
        	 == bbDescSpec
        	&& bbDescSpec != NULL
          )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BBIS descriptor for %s already exists in %s @%08x %s%d%s",
                          errorStartStr, functionName,
                          boardNameInstanz,
                          bbListElement->bbData->bbName,
                          bbDescSpec,
                          errorLineStr, __LINE__, errorEndStr ));

        	error = ERR_BBIS_ILL_PARAM;
            goto CLEANUP;
        }/*if*/
        bbListElBefore = bbListElement;
        bbListElement = bbListElement->next;
    }/*while*/



CLEANUP:
    return( error );
}

/************************** BK_GetBBHandle **********************************
 *
 *  Description: Looks for <boardNameInstanz> in the baseboard list.
 *               Creates a new element and initialises the base board
 *               if not in list.
 *				 Increments the link count.
 *				 Initializes the BK module if BK_SemId == NULL.
 *
 *---------------------------------------------------------------------------
 *  Input......: bbDescSpec       BBIS descriptor specifier
 *               boardNameInstanz board name e.g. "D201_1"
 *               bbDataHdlP       pointer to variable where pointer to
 *                                list element is stored
 *
 *  Output.....: *bbDataHdlP      pointer to list element
 *               return           0 or error code
 *
 *  Globals....: BK_BaseBoardListAnchor
 *
 ****************************************************************************/
int32 BK_GetBBHandle
(
    DESC_SPEC                  *bbDescSpec,
    char                       *boardNameInstanz,
    BK_BASE_BOARD_LIST_ELEMENT **bbDataHdlP
)
{
    DBGCMD( static char functionName[] = "BK_GetBBHandle"; )
    int32 retCode = 0;
    int32 retCode2 = 0;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement = NULL;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElBefore = NULL;
    u_int32 gotsize1 = 0; /* klocwork id1101 */
    u_int32 gotsize2 = 0;
    u_int32 gotsize3 = 0;
    u_int32 strLen;
    DESC_HANDLE  *bbDescHdl = NULL;

#ifdef DBG
	if(!DBH)
	    DBGINIT((NULL,&DBH));
#endif /*DBG*/
    DBGWRT_1((DBH,"%s( 0x%08x, %s, 0x%08x)\n",
                   functionName, bbDescSpec, boardNameInstanz, bbDataHdlP));
    *bbDataHdlP = 0;

	/*----------------------+
	| BK semaphore exists ? |
	+----------------------*/
    if( BK_SemHdl == NULL )
	{
    	BK_SemHdl = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
	    if( BK_SemHdl == NULL )
		{
            DBGWRT_ERR( ( DBH, "%s%s: semBCreate() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		    goto END_END;
		}/*if*/
	}/*if*/

    /* BK semaphore take */
	retCode = semTake( BK_SemHdl, WAIT_FOREVER );
	if( retCode )
	{
        retCode = errnoGet();
        DBGWRT_ERR( ( DBH, "%s%s: semTake() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	    goto END_END;
	}/*if*/

    /*-----------------------------------------------------+
    |  looking for boardNameInstanz to the end of the list |
    +-----------------------------------------------------*/
    bbListElement = &BK_BaseBoardListAnchor;
    bbListElBefore = NULL;
    while( bbListElement && bbListElement->bbData )
    {
        if( !OSS_StrCmp( OSS_VXWORKS_OS_HDL, boardNameInstanz,
                         bbListElement->bbData->bbName )
          )
        {
		    DBGWRT_2((DBH,"%s(): %s in list\n", functionName, boardNameInstanz));
            break; /*found*/
        }/*if*/
        bbListElBefore = bbListElement;
        bbListElement = bbListElement->next;
    }/*while*/

    if( bbListElement == NULL || bbListElement->bbData == NULL )
    {
	    DBGWRT_2((DBH,"%s(): %s create handle\n", functionName, boardNameInstanz));

		/*------------------------+
        |  create new bbis handle |
        +------------------------*/
        if( bbListElement == NULL )
        {
            /* it is not the anchor */
            bbListElement = (BK_BASE_BOARD_LIST_ELEMENT*) OSS_MemGet( OSS_VXWORKS_OS_HDL, sizeof( BK_BASE_BOARD_LIST_ELEMENT ),
                                                                      &gotsize1 );
            if( !bbListElement )
            {
                DBGWRT_ERR( ( DBH, "%s%s: OSS_MemGet() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
                retCode = ERR_OSS_MEM_ALLOC;
                goto END;
            }/*if*/
            OSS_MemFill( OSS_VXWORKS_OS_HDL, gotsize1, (char*)bbListElement, 0 );
            bbListElement->memSize = gotsize1;
        }/*if*/

        bbListElement->bbData = (BK_BASE_BOARD_DATA*) OSS_MemGet( OSS_VXWORKS_OS_HDL, sizeof( BK_BASE_BOARD_DATA ), &gotsize2 );

        if( !bbListElement->bbData )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, gotsize1 );
            bbListElement = NULL;
            DBGWRT_ERR( ( DBH, "%s%s: OSS_MemGet() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            retCode = ERR_OSS_MEM_ALLOC;
            goto END;
        }/*if*/
        OSS_MemFill( OSS_VXWORKS_OS_HDL, gotsize2, (char*)bbListElement->bbData, 0 );
        bbListElement->bbData->memSize = gotsize2;

		retCode = BK_ConsistencyCheck( boardNameInstanz, bbDescSpec );
		if( retCode )
		{
                DBGWRT_ERR( ( DBH, "%s%s: BK_ConsistencyCheck() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
                goto END;
		}
        bbListElement->bbData->descP = bbDescSpec;

        /*-------------------------------------+
        |  get board name from bbis descriptor |
        +-------------------------------------*/
        if(( retCode = DESC_Init( bbDescSpec, OSS_VXWORKS_OS_HDL, &bbDescHdl )))
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize ) {
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
                bbListElement = NULL;
            }
            goto END;
        }/*if*/

        strLen = BK_MAX_BBIS_BRD_NAME;
        if( ( retCode = DESC_GetString( bbDescHdl,
                                        "",
                                        bbListElement->bbData->brdDrvName,
                                        &strLen,
                                        "HW_TYPE" )
            )
          )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize )
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
            	bbListElement = NULL;
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetString() BRD_HANLDER %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            goto END;
        }/*if*/

#ifdef DBG
        /*-------------------------------------+
        |   get DEBUG LEVEL                    |
        +-------------------------------------*/
        retCode = DESC_GetUInt32( bbDescHdl,
                                  OSS_DBG_DEFAULT,
                                  &DBG_MYLEVEL,
                                  "DEBUG_LEVEL_BK" );

        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize )
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
            	bbListElement = NULL;
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32() DEBUG_LEVEL_BK %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            goto END;
        }/*if*/
        retCode = 0;
#endif /*DBG*/


        if( ( retCode = DESC_Exit( &bbDescHdl ) ) )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize )
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
            	bbListElement = NULL;
            goto END;
        }/*if*/


	    DBGWRT_2((DBH,"%s(): driver name %s\n", functionName, bbListElement->bbData->brdDrvName));

		/*-----------------------------------------+
        | get BBIS function entrys - e.g. "A201"   |
        +-----------------------------------------*/
        retCode = BBIS_FindEntry( bbListElement->bbData->brdDrvName, &bbListElement->bbData->bbHdlEntrys );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BBIS_FindEntry() driver not found %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize ) {
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
                bbListElement = NULL;
            }
            goto END;
        }/*if*/

		/*-------------------------+
        |  create baseboard handle |
        +-------------------------*/
        retCode = bbListElement->bbData->bbHdlEntrys.init(
                                                      OSS_VXWORKS_OS_HDL,
                                                      bbDescSpec,
                                                      &bbListElement->bbData->bbHdlData );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BaseBoard init() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;

            if( bbListElement->memSize ) {
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
                bbListElement = NULL;
            }
            goto END;
        }/*if*/

		/*----------------------+
        |  initialize baseboard |
        +----------------------*/
        retCode = bbListElement->bbData->bbHdlEntrys.brdInit(
                                                      bbListElement->bbData->bbHdlData );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BaseBoard brdInit() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;
            if( bbListElement->memSize ) {
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
                bbListElement = NULL;
            }
            goto END;
        }/*if*/

		/*------------------------------------------+
        | copy the BASEBOARD NAME to bk system list |
        +------------------------------------------*/
        bbListElement->bbData->bbName = (char*) OSS_MemGet( OSS_VXWORKS_OS_HDL,
                                                            strlen( boardNameInstanz )+1,
                                                            &gotsize3 );
        if( !bbListElement->bbData->bbName )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, gotsize2 );
            bbListElement->bbData = NULL;

            if( bbListElement->memSize )
                OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
            	bbListElement = NULL;
            DBGWRT_ERR( ( DBH, "%s%s: OSS_MemGet() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            retCode = ERR_OSS_MEM_ALLOC;
            goto END;
        }/*if*/
        bbListElement->bbData->strSize = gotsize3;
        strcpy( bbListElement->bbData->bbName, boardNameInstanz );

		/*------------------------------+
        |  update the links in the list |
        +------------------------------*/
        bbListElement->before = bbListElBefore;
        if( bbListElBefore )
        {
            /* its not the anchor */
            bbListElement->next = NULL;
            bbListElBefore->next = bbListElement;
        }
        else
        {
            /*it's the anchor*/
            if( bbListElement->next != NULL )
            {
             	if( !bbListElement->next->bbData )
                	bbListElement->next = NULL;
            }
        }/*if*/
        bbListElement->bbData->linkCount=0;
    }/*if*/

    *bbDataHdlP = bbListElement;

    bbListElement->bbData->linkCount++;

END:
    /* BK semaphore give */
	retCode2 = semGive( BK_SemHdl );
	if( retCode2 )
	{
        retCode2 = errnoGet();
        DBGWRT_ERR( ( DBH, "%s%s: semTake() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	}/*if*/
    if( !retCode )
	    retCode = retCode2;

END_END:
	if (bbDescHdl != NULL)
	{ /* klocwork 2nd id 9529 */
		OSS_MemFree(OSS_VXWORKS_OS_HDL, bbDescHdl, sizeof(DESC_INT_HDL));
		bbDescHdl = NULL;
	}
    return( retCode );
}/*BK_GetBBHandle*/

/**************************** BK_UnGetBBHandle ******************************
 *
 *  Description: Decrements the link count of the current base board.
 *				 Removes the list element if last link is deleted (link count==0)
 *               and deinitialises the base board.
 *
 *		   Note: *bbDataHdlP is always set to zero
 *
 *---------------------------------------------------------------------------
 *  Input......: bbDataHdlP       pointer to variable where pointer to
 *                                list element is stored
 *
 *  Output.....: *bbDataHdlP      NULL
 *               return           0 or error code
 *
 *  Globals....:  BK_BaseBoardListAnchor
 *
 ****************************************************************************/
int32 BK_UnGetBBHandle
(
    BK_BASE_BOARD_LIST_ELEMENT **bbDataHdlP
)
{
    DBGCMD( static char functionName[] = "BK_UnGetBBHandle"; )
    int32 retCode = 0;
    int32 retCode2 = 0;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement = *bbDataHdlP;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElBefore;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElAfter;

    DBGWRT_1((DBH,"%s( 0x%08x ) %s links %d\n", functionName,
              bbDataHdlP, bbListElement->bbData->bbName,
              bbListElement->bbData->linkCount ));

    /* BK semaphore take */
	retCode = semTake( BK_SemHdl, WAIT_FOREVER );
	if( retCode )
	{
        retCode = errnoGet();
        DBGWRT_ERR( ( DBH, "%s%s: semTake() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	    goto END_UNGET_END;
	}/*if*/

    bbListElement->bbData->linkCount--;

    if( bbListElement->bbData->linkCount == 0 )
    {
	    DBGWRT_2((DBH,"%s close baseboard\n", functionName ));

    	/*-----------------------+
        | deinitialize baseboard |
        +-----------------------*/
        retCode = bbListElement->bbData->bbHdlEntrys.brdExit(
                                                      bbListElement->bbData->bbHdlData );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BaseBoard brdExit() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            goto END_UNGET;
        }/*if*/

		/*-------------------------+
        | destroy baseboard handle |
        +-------------------------*/
        retCode = bbListElement->bbData->bbHdlEntrys.exit( &bbListElement->bbData->bbHdlData );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BaseBoard exit() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
            goto END_UNGET;
        }/*if*/

		/*-----------------------------+
        | update the links in the list |
        +-----------------------------*/
        bbListElBefore = bbListElement->before;
        bbListElAfter  = bbListElement->next;
        if( bbListElBefore )
        {
            bbListElBefore->next = bbListElAfter;
        }/*if*/
        if( bbListElAfter && bbListElement != &BK_BaseBoardListAnchor )
        {
            bbListElAfter->before = bbListElBefore;
        }/*if*/

		/*-------------------+
        |  free list element |
        +-------------------*/
        OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData->bbName, bbListElement->bbData->strSize );
        bbListElement->bbData->bbName = NULL;
        OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement->bbData, bbListElement->bbData->memSize );
        bbListElement->bbData = NULL;

        if( bbListElement != &BK_BaseBoardListAnchor )
        {
            OSS_MemFree( OSS_VXWORKS_OS_HDL, bbListElement, bbListElement->memSize );
            bbListElement = NULL;
        }
		else
		{
            if( BK_BaseBoardListAnchor.next == NULL )
            {
			    DBGWRT_2((DBH,"%s all baseboards deinitialized\n", functionName ));
            #ifdef DBG
            /* check - last base board unlinked - then close dbg handle */
            	if(DBH)
             	    DBGEXIT((&DBH));
            #endif /*DBG*/
            }/*if*/
        }/*if*/

        *bbDataHdlP = NULL;
    }/*if*/

END_UNGET:
    /* BK semaphore give */
	retCode2 = semGive( BK_SemHdl );
	if( retCode2 )
	{
        retCode2 = errnoGet();
        DBGWRT_ERR( ( DBH, "%s%s: semTake() %s%d%s",
                          errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	}/*if*/
    if( !retCode )
	    retCode = retCode2;

END_UNGET_END:
    return( retCode );
}/*BK_UnGetBBHandle*/

/******************************** BK_Show ***********************************
 *
 *  Description: Displays baseboard system and driver name and the link count.
 *
 *		   Note: Only initialized baseboards are in the baseboard system list.
 *				 A path must be open on a device to display the baseboard.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  return  always 0
 *
 *  Globals....:  BK_BaseBoardListAnchor
 *
 ****************************************************************************/
int BK_Show( void )
{
	int found;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement = &BK_BaseBoardListAnchor;

	found = 0;
    printf("\n\n=== BK Baseboard List ===\n");
    printf("NAME  \tDRIVER\tLNK-CNT\n");
    printf("------\t------\t-------\n");
    /* show all baseboards */
    while( bbListElement )
    {
        if( bbListElement->bbData )
        {
            printf("%s\t%s\t    %d\n",
            		bbListElement->bbData->bbName ?
                        bbListElement->bbData->bbName : "----",
                    bbListElement->bbData->brdDrvName,
                    (int) bbListElement->bbData->linkCount );
			found = 1;
        }/*if*/
        bbListElement = bbListElement->next;
    }/*while*/

    if( !found )
		printf("\tno active baseboard\n");

	return( 0 );
}/*BK_Show*/

/******************************** BK_Show2 ***********************************
 *
 *  Description: Displays baseboard system and driver name and the link count.
 *
 *		   Note: Only initialized baseboards are in the baseboard system list.
 *				 A path must be open on a device to display the baseboard.
 *
 *---------------------------------------------------------------------------
 *  Input......:  verbose  0 | 1
 *
 *  Output.....:  return   always 0
 *
 *  Globals....:  BK_BaseBoardListAnchor
 *
 ****************************************************************************/
int BK_Show2( int verbose )
{
	int found;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement = &BK_BaseBoardListAnchor;

	found = 0;
    printf("\n\n=== BK Baseboard List ===\n");

	printf("ANCHOR  %08x\n", (int)bbListElement );


    /* show all baseboards */
    while( bbListElement )
    {
		printf("element  %08x\n", (int)bbListElement );
		printf("  next   %08x\n", (int)bbListElement->next );
		printf("  before %08x\n", (int)bbListElement->before );
		printf("  data   %08x\n", (int)bbListElement->bbData );
        if( verbose && bbListElement->bbData )
        {
            printf("%s\t%s\t    %d\n", bbListElement->bbData->bbName,
                                       bbListElement->bbData->brdDrvName,
                                       (int) bbListElement->bbData->linkCount );
        }/*if*/
        bbListElement = bbListElement->next;
    }/*while*/
	return( 0 );
}/*BK_Show2*/






