/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_sharedmem.c
 *      Project: MDIS 4.x
 *
 *      $Author: ufranke $
 *        $Date: 2009/03/31 09:37:50 $
 *    $Revision: 1.2 $
 *
 *  Description: shared memory interface
 *
 *     Required:
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_sharedmem.c,v $
 * Revision 1.2  2009/03/31 09:37:50  ufranke
 * cosmetics
 *
 * Revision 1.1  1999/08/30 11:03:38  Franke
 * Initial Revision
 *
 * 
 * cloned from OS9 shmem.c Revision 1.3  1999/02/15 13:57:51  see
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
/*-----------------------------------------+
|  INCLUDES                                |
+------------------------------------------*/
#include <MEN/men_typs.h>

#include <vxWorks.h>

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
static int32 AddSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr,
	u_int32          size,
	void             *drvAddr);

static OSS_SHMEM_TBL *FindSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr);

static int32 RemoveSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr);

/*************************** OSS_SharedMemCreate *****************************
 *
 *  Description:  Create shared memory handle
 *
 *                The function creates the global shared memory handle for
 *                all subsequent OSS_SharedMemXXX() calls.
 *
 *                Allocates memory and initializes the handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss          oss handle
 *  Output.....:  smHdlP       pointer to shared memory handle
 *                return       success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemCreate(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE **smHdlP
)
{
DBGCMD( static const char	functionName[] = "OSS_SharedMemCreate"; )
OSS_SHMEM_HANDLE	*smHdl;
u_int32 			gotsize;

    DBGWRT_1((DBH,"%s()\n", functionName));
	*smHdlP = NULL;
	
	/*------------------------------+
	|  alloc/init handle            |
	+------------------------------*/
	smHdl = (OSS_SHMEM_HANDLE*) OSS_MemGet( oss, sizeof(OSS_SHMEM_HANDLE), &gotsize );

    if( smHdl == NULL )
    {
       return( ERR_OSS_MEM_ALLOC );
    }/*if*/

    /* fill turkey with 0 */
    OSS_MemFill( NULL, gotsize, (char*) smHdl, 0 );

    /* fill up the turkey */
	smHdl->memAlloc = gotsize;

	*smHdlP = smHdl;
	return(ERR_SUCCESS);
}/*OSS_SharedMemCreate*/

/*************************** OSS_SharedMemRemove *****************************
 *
 *  Description:  Remove shared memory handle
 *                
 *                The function removes implicitly all installed shared 
 *                memory areas and cleanup and deallocates the handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss        oss handle
 *                smHdlP     pointer to shared memory handle
 *  Output.....:  return     success (0) or error code
 *                *smHdlP    NULL
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemRemove(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE **smHdlP
)
{
DBGCMD( static const char	functionName[] = "OSS_SharedMemRemove"; )

    OSS_SHMEM_HANDLE	*smHdl;
    OSS_SHMEM_TBL 		*st;		/* point to root */
    OSS_SHMEM_TBL		*last;
	int32 error;

    DBGWRT_1((DBH,"%s()\n", functionName));

    smHdl	= *smHdlP;
	*smHdlP = NULL;

	if( smHdl == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: smHdl %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_HANDLE );
    }/*if*/

    st		= &smHdl->smTbl;		/* point to root */
    last	= st;


	/* find last chain entry */
	while(st != NULL) {
		last = st;
		st = st->next;
	}

	/* free shared memory chain entries (backward) */
	while(last != &smHdl->smTbl) {
		if (st != NULL) /* klocwork id4091 */
			DBGWRT_1((DBH,"  cleanup shared mem: smNr=0x%x size=%d addr=0x%08x\n", st->smNr, st->size, st->drvAddr));
		st = last->last;
		OSS_MemFree(oss, (void*)last->drvAddr, last->size);	/* free shared mem */
		last->drvAddr = NULL;
		OSS_MemFree(oss, (void*)last, last->memAlloc);		/* free entry */
		last = st;
	}
	  
	/* free handle */
	if ((error = OSS_MemFree(oss, (int8*)smHdl, smHdl->memAlloc)))
		return(error);

	smHdl = NULL;

    return(ERR_SUCCESS);
}/*OSS_SharedMemRemove*/


/*************************** OSS_SharedMemCheckNr ****************************
 *
 *  Description:  Check shared memory area index
 *
 *                The function checks if the shared memory area index "smNr"
 *                in the given "smSet" structure is in the specified range.
 *
 *                Following data structure is passed via 'smSet':
 *
 *                typedef struct {				
 *                    u_int32    smNr;       IN:  index (0..n)
 *                	  u_int32    size;       (ignored)
 *                	  void       *drvAddr;   (ignored)
 *                } OSS_SHMEM_SET;
 *                
 *                The actual index (as defined in 'smSet') is returned.
 *                
 *                This function should be used for error checking and error
 *                handling in application triggered shared memory calls.
 *
 *                If the index is not in range, the function returns with 
 *                ERR_OSS_ILL_PARAM. 
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss      oss handle
 *                smHdl    shared memory handle
 *                smSet    data structure
 *                minNr    minimum index
 *                maxNr    maximum index
 *  Output.....:  smNr     actual index
 *                return   success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemCheckNr(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
    OSS_SHMEM_SET    *smSet,
	u_int32          minNr,
	u_int32          maxNr,
	u_int32          *smNr
)
{
DBGCMD( static const char	functionName[] = "OSS_SharedMemCheckNr"; )

	if (!IN_RANGE(smSet->smNr, minNr, maxNr)) {
        DBGWRT_ERR( ( DBH, "%s%s: smNr=0x%x not in %d..%d %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
					  smSet->smNr, minNr, maxNr,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_ILL_PARAM);
	}

	*smNr = smSet->smNr;

	return(ERR_SUCCESS);
}/*OSS_SharedMemCheckNr*/

/*************************** OSS_SharedMemCheckSize **************************
 *
 *  Description:  Check shared memory area size
 *
 *                The function checks if the shared memory area size "size"
 *                in the given "smSet" structure is in the specified range.
 *
 *                Following data structure is passed via 'smSet':
 *
 *                typedef struct {				
 *                    u_int32    smNr;       (ignored)
 *                	  u_int32    size;       IN:  shared memory size [bytes]
 *                	  void       *drvAddr;   (ignored)
 *                } OSS_SHMEM_SET;
 *                
 *                The actual size (as defined in 'smSet') is returned.
 *                
 *                This function should be used for error checking and error
 *                handling in application triggered shared memory calls.
 *
 *                If the size is not in range, the function returns with 
 *                ERR_OSS_ILL_PARAM. 
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss      oss handle
 *                smHdl    shared memory handle
 *                smSet    data structure
 *                minSize  minimum size
 *                maxSize  maximum size
 *  Output.....:  smSize   actual size
 *                return   success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemCheckSize(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
    OSS_SHMEM_SET    *smSet,
	u_int32          minSize,
	u_int32          maxSize,
	u_int32          *smSize
)
{
DBGCMD( static const char	functionName[] = ""; )

	if (!IN_RANGE(smSet->size, minSize, maxSize)) {
        DBGWRT_ERR( ( DBH, "%s%s: size=%d not in %d..%d %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
					  smSet->size, minSize, maxSize,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_ILL_PARAM);
	}

	*smSize = smSet->size;

	return(ERR_SUCCESS);
}

/*************************** OSS_SharedMemSet ********************************
 *
 *  Description:  Create a shared memory area
 *
 *                For application triggered shared memory calls, this
 *                function has to be called on M_MK_BLK_SHMEM_SET setstat.
 *
 *                Following data structure is passed via 'smSet':
 *
 *                typedef struct {				
 *                    u_int32    smNr;       IN:  index (0..n)
 *                	  u_int32    size;       IN:  shared memory size [bytes]
 *                	  void       *drvAddr;   OUT: shared memory addr (driver)
 *                } OSS_SHMEM_SET;
 *                
 *                The function creates the shared memory area "smNr" with
 *                specified size "size" and links to it. The area is initially
 *                cleared with zero.
 *                
 *                After successful creation of the area, the drivers memory
 *                access address is returned via "drvAddr" parameter of the
 *                structure.
 *
 *                The 
 *
 *                If the shared memory area already exists, the function
 *                returns with ERR_OSS_SHMEM_SET. 
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss       oss handle
 *                smHdl     shared memory handle
 *                smSet     installation data structure
 *  Output.....:  return    success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemSet(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
    OSS_SHMEM_SET    *smSet
)
{
DBGCMD( static const char	functionName[] = "OSS_SharedMemSet()"; )
	u_int32 gotsize;
	void *drvAddr;
	int32 error;

    DBGWRT_1((DBH,"%s:\n", functionName));

	/* already existing ? */
	if (FindSharedMem(oss, smHdl, smSet->smNr)) {
        DBGWRT_ERR( ( DBH, "%s%s: smNr=0x%x already exists %s%d%s\n",
                      OSS_ErrorStartStr, functionName, 
					  smSet->smNr,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_SHMEM_SET);
	}

	/* alloc+clear shared memory */
    if ((drvAddr = (void*)OSS_MemGet(oss, smSet->size, &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

    /* fill turkey with 0 */
    OSS_MemFill( NULL, gotsize, (char*) drvAddr, 0 );

	/* add to shared memory chain */
	if ((error = AddSharedMem(oss, smHdl, smSet->smNr, gotsize, drvAddr)))
		return(error);

	/* init output params */
	smSet->drvAddr = drvAddr;

    DBGWRT_2((DBH," installed shared mem: smNr=0x%x size=%d addr=0x%08x\n",
			  smSet->smNr, smSet->size, smSet->drvAddr));

	return(ERR_SUCCESS);
}

/*************************** OSS_SharedMemLink ******************************
 *
 *  Description:  Link to an exisiting shared memory area
 *
 *                Has to be called on M_MK_BLK_SHMEM_LINK setstat calls.
 *
 *                Following data structure is passed via 'smLink':
 *
 *                typedef struct {				
 *                    u_int32    smNr;       IN:  index (0..n)
 *                	  u_int32    size;       OUT: shared memory size [bytes]
 *                	  void       *drvAddr;   OUT: shared memory addr (driver)
 *                } OSS_SHMEM_LINK;
 *                
 *                The function links to the shared memory area "smNr".
 *                
 *                After successful linking of the area, the drivers memory
 *                access address is returned via "drvAddr" parameter of the
 *                structure and the shared memory size via "size".
 *
 *                If the shared memory area does not exist, the function
 *                returns with ERR_OSS_SHMEM_LINK. 
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss       oss handle
 *                smHdl     shared memory handle
 *                smLink    installation data structure
 *  Output.....:  smNr      actual index
 *                smLinkCnt actual link counter
 *                return    success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemLink(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
    OSS_SHMEM_SET    *smLink
)
{
DBGCMD( static const char	functionName[] = "OSS_SharedMemLink()"; )
    OSS_SHMEM_TBL *st;

    DBGWRT_1((DBH,"%s:\n", functionName));

	/* not found ? */
	if ((st = FindSharedMem(oss, smHdl, smLink->smNr)) == NULL) {
        DBGWRT_ERR( ( DBH, "%s%s: smNr=0x%x not found %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
					  smLink->smNr,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_SHMEM_LINK);
	}

	st->linkCnt++;

	/* init output params */
	smLink->drvAddr = st->drvAddr;
	smLink->size    = st->size;

    DBGWRT_2((DBH," linked shared mem: smNr=0x%x (links=%d)\n",
			  smLink->smNr, st->linkCnt));

	return(ERR_SUCCESS);
}

/*************************** OSS_SharedMemClear ******************************
 *
 *  Description:  Unlink/Deinstall a shared memory area
 *
 *                Has to be called on M_MK_SHMEM_CLEAR setstat calls.
 *
 *                Following data structure is passed via 'smClear':
 *
 *                typedef struct {
 *                    u_int32    smNr;     IN:	index (0..n)
 *                    u_int32    linkCnt;  OUT: link counter
 *                } OSS_SHMEM_CLEAR;
 *                
 *                The function unlinks from the shared memory area "smNr".
 *                
 *                If the shared memory area has no more links, it is 
 *                deinstalled and the used memory is deallocated.  
 *                
 *                After successful unlinking, the actual link counter is
 *                returned via "linkCnt" parameter of the structure.
 *
 *                If the returned link counter has reached zero,
 *                no further access to the shared memory area is allowed !
 *
 *                If the shared memory area does not exist, the function
 *                returns with ERR_OSS_SHMEM_CLR. 
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss       oss handle
 *                smHdl     shared memory handle
 *                smClear   de-installation data structure
 *  Output.....:  smNr      actual index
 *                smLinkCnt actual link counter
 *                return    success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_SharedMemClear(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
    OSS_SHMEM_CLEAR  *smClear
)
{
DBGCMD( static const char functionName[] = "OSS_SharedMemClear()"; )
    OSS_SHMEM_TBL *st;
	int32 error;

    DBGWRT_1((DBH,"%s\n", functionName));

	/* not found ? */
	if ((st = FindSharedMem(oss, smHdl, smClear->smNr)) == NULL) {
        DBGWRT_ERR( ( DBH, "%s%s: smNr=0x%x not found %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
					  smClear->smNr,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_SHMEM_CLR);
	}

	st->linkCnt--;

	/*--------------------------------+
	|  remove shared memory area      |
	|  if last link                   |
	+--------------------------------*/
	if (st->linkCnt == 0) {
		DBGWRT_2((DBH," remove shared mem: smNr=0x%x\n", smClear->smNr));

		/* remove shared memory */
		if ((error = OSS_MemFree(oss, (int8*)st->drvAddr, st->size)))
			return(error);
		st->drvAddr = NULL;

		/* remove from shared memory chain */
		if ((error = RemoveSharedMem(oss, smHdl, smClear->smNr)))
			return(ERR_OSS_SHMEM_CLR);

	}
	else
		DBGWRT_2((DBH," unlink shared mem: smNr=0x%x (links=%d)\n",
				  smClear->smNr, st->linkCnt));

	/* init output params */
	smClear->linkCnt = st->linkCnt;

	return(ERR_SUCCESS);
}

/********************************* AddSharedMem *****************************
 *
 *  Description: Add shared memory entry to linked list
 *			   
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               smHdl      shared memory handle
 *               smNr	    shared memory index
 *               size	    shared memory size
 *               drvAddr    shared memory addr (driver)
 *  Output.....: return     success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 AddSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr,
	u_int32          size,
	void             *drvAddr
)
{
    OSS_SHMEM_TBL *st = &smHdl->smTbl;		/* point to root */
    OSS_SHMEM_TBL *last = NULL;
    u_int32 gotsize;

	/* find last entry in chain */
	while(st != NULL) {
		last = st;
		st = st->next;
	}

	/* alloc/clear new entry */
    if ((st = (OSS_SHMEM_TBL*)
		 OSS_MemGet(oss, sizeof(OSS_SHMEM_TBL), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

    /* fill turkey with 0 */
    OSS_MemFill( NULL, gotsize, (char*) st, 0 );

	st->memAlloc = gotsize;

	/* add entry */
	last->next 	= st;

	/* init entry */
	st->last	= last;
	st->smNr	= smNr;
	st->size	= size;
	st->drvAddr	= drvAddr;
	st->linkCnt	= 1;

	return(ERR_SUCCESS);
}/*AddSharedMem*/

/********************************* FindSharedMem ****************************
 *
 *  Description: Find shared memory entry in linked list
 *			   
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               smHdl      shared memory handle
 *               smNr	    shared memory index
 *  Output.....: return     ptr to shared memory entry or NULL if not found
 *  Globals....: -
 ****************************************************************************/
static OSS_SHMEM_TBL *FindSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr
)
{
    OSS_SHMEM_TBL *st = &smHdl->smTbl;		/* point to root */

	/* find matching entry */
	while((st = st->next) != NULL)
		if (st->smNr == smNr)
			return(st);

	return(NULL);
}

/********************************* RemoveSharedMem **************************
 *
 *  Description: Remove shared memory entry from linked list
 *			   
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               smHdl      shared memory handle
 *               smNr	    shared memory index
 *  Output.....: return     success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
static int32 RemoveSharedMem(
    OSS_HANDLE       *oss,
    OSS_SHMEM_HANDLE *smHdl,
	u_int32          smNr
)
{
    OSS_SHMEM_TBL *st = &smHdl->smTbl;		/* point to root */

	/* find matching entry */
	if ((st = FindSharedMem(oss, smHdl, smNr)) == NULL)
		return(1);

	/* remove entry */
	if (st->last)
		st->last->next = st->next;

	if (st->next)
		st->next->last = st->last;

	/* free mem */
	OSS_MemFree(oss, (void*)st, st->memAlloc);
	st = NULL;

	return(ERR_SUCCESS);
}
