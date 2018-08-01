/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_callback.c
 *      Project: MDIS 4.x
 *
 *      $Author: ufranke $
 *        $Date: 2009/03/31 09:37:14 $
 *    $Revision: 1.3 $
 *
 *  Description: callback interface
 *
 *     Required:
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_callback.c,v $
 * Revision 1.3  2009/03/31 09:37:14  ufranke
 * cosmetics
 *
 * Revision 1.2  2006/02/17 09:41:28  cs
 * renamed struct NODE to CLIST_NODE (name occupied for VxW 6.2)
 *
 * Revision 1.1  1999/08/30 11:03:31  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
/*-----------------------------------------+
|  INCLUDES                                |
+------------------------------------------*/
#include <MEN/men_typs.h>

#include <vxWorks.h>

#include <MEN/mdis_err.h>

#define COMPILE_OSS_CALLBACK
#include <MEN/oss.h>
#include <string.h>
#include <sigLib.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CALL_HASH_SIZE	64
#define CH_IDX(n) ((n)&(CALL_HASH_SIZE-1))

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*--- common list entry ---*/
typedef struct {
	OSS_DL_NODE	n;				/* linked list node */
	u_int32 memAlloc;			/* allocated mem for this struct */
} CLIST_NODE;

/*--- structure to maintain list of known call numbers ---*/
typedef struct {
	CLIST_NODE n;						/* common list entry header */
	u_int32 callNr;				/* call number */
	void 	(*funct)(			/* callback function pointer */
		void *appArg,
		void *drvArg);
	void    *appArg;			/* function argument from application */
} CALL_NODE;

/*--- callback handle ---*/
struct OSS_CALLBACK_HANDLE { 	/*--- CALLBACK handle ---*/
    u_int32 memAlloc;			/* mem allocated for the handle itself*/
    u_int32	maxQueue;			/* max queued callbacks (per queue)   */
    u_int32	mode;				/* callback mode indirect/direct call */
    u_int32	mask;				/* callbacks mask at direct call mode */
	OSS_DL_LIST procList;		/* linked list of attached processes  */
	OSS_DL_LIST callHash[CALL_HASH_SIZE]; /* call hash table */
};

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
MSG_Q_ID OSS_CbMsgQid = NULL;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int32 ExecCallback(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    u_int32             callNr,
    void                *appArg,
    void                *drvArg,
	u_int32				argNum);
static int32 Install(
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32 callNr,
	void 	(*funct)(
		void *appArg,
		void *drvArg),
	void    *appArg,
	u_int32 mode);
static int32 DeInstall(
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32 callNr);
static CALL_NODE *FindHashCb(
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32          callNr);
static void FreeList( OSS_HANDLE *oss, OSS_DL_LIST *l );

/*************************** OSS_CallbackCreate *****************************
 *
 *  Description:  Create callback handle
 *
 *                This function creates a callback handle for subsequent
 *                OSS_CallbackXXX() calls.
 *
 *                maxQueue defines the maximum nr of queued callbacks
 *
 *                Allocates memory for the handle, and fills the handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss          oss handle
 *                maxQueue 	   max queued callbacks
 *  Output.....:  cbHdlP       pointer to callback handle
 *                return       success (0) or error code
 *							   ERR_OSS_MEM_ALLOC
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_CallbackCreate(
    OSS_HANDLE           *oss,
    u_int32	             maxQueue,
    OSS_CALLBACK_HANDLE  **cbHdlP
)
{
    OSS_CALLBACK_HANDLE  *cbHdl;
    u_int32 reqsize, gotsize, i;

    DBGWRT_1((DBH,"OSS - OSS_CallbackCreate\n"));
	DBGWRT_2((DBH," init callbacks: max=%d\n",maxQueue));

	/*------------------------------+
	|  alloc/init handle            |
	+------------------------------*/
	reqsize = sizeof(OSS_CALLBACK_HANDLE);

    if ((*cbHdlP = cbHdl = (OSS_CALLBACK_HANDLE*)
		 OSS_MemGet(oss, reqsize, &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	OSS_MemFill( oss, reqsize, (char*)cbHdl, 0x00);	/* clear handle */

	cbHdl->memAlloc	= gotsize;
	cbHdl->maxQueue	= maxQueue;
	cbHdl->mode		= OSS_CALLBACK_MODE_INDIRECT; /* default mode */
	cbHdl->mask		= 0;

	for(i=0; i<CALL_HASH_SIZE; i++ )
		OSS_DL_NewList( &cbHdl->callHash[i] );

	return(ERR_SUCCESS);
}

/*************************** OSS_CallbackRemove *****************************
 *
 *  Description:  Remove callback handle
 *
 *                Removes all installed callbacks and deallocates the handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss        oss handle
 *                cbHdlP     pointer to callback handle
 *  Output.....:  return     success (0) or error code
 *                           ERR_OSS_ILL_HANDLE
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_CallbackRemove(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE **cbHdlP
)
{
    OSS_CALLBACK_HANDLE  *cbHdl = *cbHdlP;
	u_int32 i;

    DBGWRT_1((DBH,"OSS - OSS_CallbackRemove\n"));
	DBGWRT_2((DBH," cleanup callbacks\n"));

	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	/*------------------------------+
	|  cleanup tables               |
	+------------------------------*/

	/*--- free hash table ---*/
	for(i=0; i<CALL_HASH_SIZE; i++ )
		FreeList( oss, &cbHdl->callHash[i] );

	/*--- delete handle ---*/
	OSS_MemFree(oss, (int8*)cbHdl, cbHdl->memAlloc);
	*cbHdlP = NULL;

    return(ERR_SUCCESS);
}


/*************************** OSS_CallbackExec *******************************
 *
 *  Description:  Execute callback routine (with app/drv arguments)
 *
 *                Callback routine with index 'callNr' is triggered/executed:
 *
 *                   void funct(void *appArg, void *drvArg)
 *
 *                where 'appArg' is the value defined with OSS_CallbackSet()
 *                and 'drvArg' the value passed to this function.
 *
 *                Systems with direct callbacks:
 *                => function with specified index is called
 *
 *                Systems with indirect callbacks:
 *                => function pointer and argument are put into
 *                   the calling process callback-queue
 *                => trigger signal is send to application
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss        oss handle
 *                cbHdl      callback handle
 *                callNr     callback function index (0..n)
 *                drvArg     callback function argument
 *  Output.....:  return     success (0) or error code
 *							 ERR_OSS_ILL_HANDLE
 *							 ERR_OSS_ILL_PARAM  	- call number not existant
 *							 ERR_OSS_CALLBACK_OVER  - queue full
 *							 ERR_OSS_SIG_SEND		- can't send signal
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_CallbackExec(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    u_int32             callNr,
    void                *drvArg
)
{
    IDBGWRT_1((DBH,"OSS - OSS_CallbackExec\n"));

	return( ExecCallback(oss, cbHdl, callNr, NULL, drvArg, 1) );
}

/*************************** OSS_CallbackExec2 ******************************
 *
 *  Description:  Execute callback routine (with drv/drv arguments)
 *
 *                Callback routine with index 'callNr' is triggered/executed:
 *
 *                   void funct(void *appArg, void *drvArg)
 *
 *                where 'appArg' and 'drvArg' are the values passed to this
 *                function.
 *
 *                Systems with direct callbacks:
 *                => function with specified index is called
 *
 *                Systems with indirect callbacks:
 *                => function pointer and argument are put into
 *                   the calling process callback-queue
 *                => trigger signal is send to application
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss        oss handle
 *                cbHdl      callback handle
 *                callNr     callback function index (0..n)
 *                appArg     callback function argument
 *                drvArg     callback function argument
 *  Output.....:  return     success (0) or error code
 *							 ERR_OSS_ILL_HANDLE
 *							 ERR_OSS_ILL_PARAM  	- call number not existant
 *							 ERR_OSS_CALLBACK_OVER  - queue full
 *							 ERR_OSS_SIG_SEND		- can't send signal
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_CallbackExec2(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    u_int32             callNr,
    void                *appArg,
    void                *drvArg
)
{
    IDBGWRT_1((DBH,"OSS - OSS_CallbackExec2\n"));

	return( ExecCallback(oss, cbHdl, callNr, appArg, drvArg, 2) );
}

/*************************** ExecCallback ***********************************
 *
 *  Description:  Execute callback routine
 *                (for handling OSS_CallbackExec / OSS_CallbackExec2)
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss        oss handle
 *                cbHdl      callback handle
 *                callNr     callback function index (0..n)
 *                appArg     callback function argument
 *                drvArg     callback function argument
 *                argNum     nr of arguments
 *                           1 = use stored 'appArg' and given 'drvArg'
 *                           2 = use given  'appArg' and given 'drvArg'
 *  Output.....:  return     success (0) or error code
 *  Globals....:  -
 ****************************************************************************/
static int32 ExecCallback(		/* nodoc */
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    u_int32             callNr,
    void                *appArg,
    void                *drvArg,
	u_int32				argNum
)
{
    DBGCMD( static const char functionName[] = "OSS - ExecCallback()"; )
	CALL_NODE *call;
	void      *arg0;
	OSS_CB_QUEUE_ENTRY cbQentry;

    IDBGWRT_1((DBH,"%s: callNr=0x%x appArg=%x drvArg=0x%x\n", functionName,
			        callNr, appArg, drvArg));

	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	/*--- lookup callNr in hash table ---*/
	if( (call = FindHashCb( oss, cbHdl, callNr )) == NULL )
	{
        IDBGWRT_ERR( ( DBH, "%s%s: callNr=0x%x not found %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      callNr,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return(ERR_OSS_ILL_PARAM);
	}/*if*/

	if( argNum == 2 )
		arg0 = appArg;
	else
		arg0 = call->appArg;

	if( cbHdl->mode == OSS_CALLBACK_MODE_DIRECT )
	{
		if( cbHdl->mask )
		{
	        IDBGWRT_ERR( ( DBH, "%s%s: callbacks masked %s%d%s\n",
    	                  OSS_ErrorStartStr, functionName,
        	              OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			return(ERR_OSS);
		}
		else
		{
			call->funct( arg0, drvArg );
		}/*if*/
	}
	else
	{
		cbQentry.funct = call->funct;
		cbQentry.arg0  = arg0;
		cbQentry.arg1  = drvArg;
	    IDBGWRT_2((DBH," msgQSend CbQid=0x%08x functP=0x%08x arg0=0x%08x arg1=0x%08x\n",
	    				OSS_CbMsgQid, cbQentry.funct, cbQentry.arg0, cbQentry.arg1));
		if( msgQSend( OSS_CbMsgQid, (char*) &cbQentry, sizeof(cbQentry),
					  NO_WAIT, MSG_PRI_NORMAL )
		  )
		{
	        IDBGWRT_ERR( ( DBH, "%s%s: msgQSend with error 0x%08x %s%d%s\n",
    	                  OSS_ErrorStartStr, functionName,
						  errnoGet(),
        	              OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			return(ERR_OSS);
		}/*if*/
	}/*if*/


	return(ERR_SUCCESS);
}/*ExecCallback*/

/********************************* FreeList **********************************
 *
 *  Description: Free all nodes of a list
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *				 l			list header
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void FreeList( OSS_HANDLE *oss, OSS_DL_LIST *l )/* nodoc */
{
	CLIST_NODE *node, *node2;

	for( node=(CLIST_NODE *)l->head; ; node=node2){

		node2 = (CLIST_NODE *)node->n.next;

		if( node2 == NULL )
			break;

		OSS_DL_Remove( &node->n );

		OSS_MemFree( oss, (int8*)node, node->memAlloc );
		node = NULL;

	}
}

/********************************* OSS_CallbackSetStat ***********************
 *
 *  Description: Generic SetStat call for callback control
 *
 *			   	 Has to be called on M_MK_BLK_CALLBACK setstat calls.
 *
 *               OSS_CB_INSTALL:
 *                Function 'funct' is installed as callback routine with
 *                index 'callNr'. Argument 'appArg' will be passed to 'func'
 *                as first argument:
 *
 *                   void funct(void *appArg, void *drvArg)
 *
 *                Subsequent calls of OSS_CallbackExec() refer to this index.
 *	              Returns an error if routine already installed.
 *
 *               OSS_CB_DEINSTALL:
 *                Callback routine with index 'callNr' is removed.
 *	              Returns an error if routine not installed.
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *				 cbHdl		callback handle
 *				 arg		ptr to SetStat args
 *  Output.....: return     success (0) or error code
 *					ERR_OSS_ILL_HANDLE
 *
 *				 	OSS_CB_INIT_PROCESS:
 *						ERR_OSS_MEM_ALLOC
 *					OSS_CB_EXIT_PROCESS:
 *						ERR_OSS					- process not installed
 *					OSS_CB_INSTALL:
 *						ERR_OSS_MEM_ALLOC
 *						ERR_OSS_CALLBACK_SET	- callNr exists already
 *						ERR_OSS					- process not installed
 *					OSS_CB_DEINSTALL:
 *						ERR_OSS_CALLBACK_CLR	- callNr not installed
 *
 *  Globals....: -
 ****************************************************************************/
int32 OSS_CallbackSetStat(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	OSS_CALLBACK_SETSTAT *arg)
{
    DBGCMD( static const char functionName[] = "OSS_CallbackSetStat()"; )
	int32 error;

    DBGWRT_1((DBH,"%s action=0x%08x\n", functionName, arg->action));

	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	error = ERR_OSS;
	switch( arg->action )
	{
		case OSS_CB_INSTALL:
			error = Install( oss, cbHdl, arg->p.install.callNr,
						 				 arg->p.install.funct,
						 				 arg->p.install.appArg,
						 				 arg->p.install.mode );
			break;

		case OSS_CB_DEINSTALL:
			error = DeInstall( oss, cbHdl, arg->p.install.callNr );
			break;

		case OSS_CB_MASK:
			if( cbHdl->mode == OSS_CALLBACK_MODE_DIRECT )
			{
				cbHdl->mask = 1;
				error = ERR_SUCCESS;
			}/*if*/
			break;

		case OSS_CB_UNMASK:
			if( cbHdl->mode == OSS_CALLBACK_MODE_DIRECT )
			{
				cbHdl->mask = 0;
				error = ERR_SUCCESS;
			}/*if*/
			break;

		default:
	    	DBGWRT_ERR( ( DBH, "%s%s: bad action code=0x%08x %s%d%s\n",
                        OSS_ErrorStartStr, functionName,
                        arg->action,
                        OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}/*switch*/
	return( error );
}/*OSS_CallbackSetStat*/

/********************************* OSS_CallbackGetStat ***********************
 *
 *  Description: Generic GetStat call for callback control
 *
 *			   	 Has to be called on M_MK_BLK_CALLBACK getstat calls.
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *				 cbHdl		callback handle
 *				 arg		ptr to GetStat args
 *  Output.....: return     success (0) or error code
 *							ERR_OSS_ILL_HANDLE	- illegal handle
 *							ERR_OSS 			- process not installed
 *							ERR_OSS_CALLBACK_EMPTY - no entries in queue
 *  Globals....: -
 ****************************************************************************/
int32 OSS_CallbackGetStat(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	OSS_CALLBACK_GETSTAT *arg)
{
	int32 error=ERR_SUCCESS;

	DBGWRT_1((DBH,"OSS - OSS_CallbackGetStat \n"));

	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	error = ERR_OSS;
	return error;
}


/********************************* OSS_CallbackSet ***************************
 *
 *  Description:  Install Callback function from driver
 *
 *			      This function is only used when bypassing the callback
 *				  getstat/setstat interface (e.g. CanOpen)
 *
 *                Function 'funct' is installed as callback routine with
 *                index 'callNr'. Argument 'appArg' will be passed to 'func'
 *                as first argument:
 *
 *                   void funct(void *appArg, void *drvArg)
 *
 *                Subsequent calls of OSS_CallbackExec() refer to this index.
 *	              Returns an error if routine already installed.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               cbHdl      callback handle
 *				 cbSet		callback set arguments
 *  Output.....: return     success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 OSS_CallbackSet(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    OSS_CALLBACK_SET    *cbSet)
{
	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	return Install( oss, cbHdl, cbSet->callNr, cbSet->funct, cbSet->appArg, OSS_CALLBACK_MODE_INDIRECT );
}

/********************************* OSS_CallbackClear ************************
 *
 *  Description:  Remove callback function from driver
 *
 *			      This function is only used when bypassing the callback
 *				  getstat/setstat interface (e.g. CanOpen)
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               cbHdl      callback handle
 *				 callNr		callnumber to be removed
 *  Output.....: return     success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 OSS_CallbackClear(
    OSS_HANDLE          *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
    u_int32             callNr)
{
	if( cbHdl == NULL )
		return ERR_OSS_ILL_HANDLE;

	return DeInstall( oss, cbHdl, callNr );
}



/********************************* Install ***********************************
 *
 *  Description: Install a callback function and set callback mode.
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               cbHdl      callback handle
 *               callNr	    callback function index
 *				 funct		callback function ptr (in user state!)
 *				 appArg		application arg to call
 *  Output.....: return     success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 Install(/* nodoc */
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32 callNr,
	void 	(*funct)(
		void *appArg,
		void *drvArg),
	void    *appArg,
	u_int32 mode
)
{
	OSS_DL_LIST *hashList = &cbHdl->callHash[CH_IDX(callNr)];
	CALL_NODE *call;
	u_int32 gotsize;

    DBGWRT_1((DBH,"OSS - CB:Install callNr=%d\n", callNr));

	/* already existing ? */
	if ( FindHashCb(oss, cbHdl, callNr) != NULL ) {
		DBGWRT_ERR((DBH,"*** OSS - CB:Install: callNr=0x%x already "
					"installed\n", callNr));
		return(ERR_OSS_CALLBACK_SET);
	}

	if( cbHdl->mode != mode )
	{
	    DBGWRT_2((DBH,"OSS - CB:Install change callback mode to =%d\n", mode));
		cbHdl->mode = mode;
	}/*if*/

	/*----------------------+
    |  install callback     |
    +----------------------*/

	/*--- allocate CALL_NODE ---*/
	if( (call = (CALL_NODE *)OSS_MemGet(oss, sizeof(*call), &gotsize))
		== NULL )
		return(ERR_OSS_MEM_ALLOC);

	OSS_MemFill( oss, gotsize, (char*)call, 0x00);	/* clear handle */

	call->n.memAlloc 	= gotsize;
	call->callNr 		= callNr;
	call->funct			= funct;
	call->appArg		= appArg;

    DBGWRT_2((DBH," install callNr=0x%x func=0x%x appArg=0x%x mode=%d\n",
			  callNr, funct, appArg,mode));

	/*--- add it to hash table ---*/
	OSS_DL_AddTail( hashList, &call->n.n );

	return(ERR_SUCCESS);
}

/********************************* DeInstall *********************************
 *
 *  Description: DeInstall a callback function
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               cbHdl      callback handle
 *               callNr	    callback function index
 *  Output.....: return     success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 DeInstall(/* nodoc */
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32 callNr)
{
	CALL_NODE *call;

    DBGWRT_1((DBH,"OSS - CB:DeInstall: callNr=%d\n", callNr));

	/*----------------------+
    |  remove callback      |
    +----------------------*/
	if ( (call = FindHashCb(oss, cbHdl, callNr)) == NULL ){
		DBGWRT_ERR((DBH," *** OSS - CB:DeInstall: callNr=0x%x not found\n",
					callNr));
		return(ERR_OSS_CALLBACK_CLR);
	}

	/* remove from list */
	OSS_DL_Remove( &call->n.n );

	/* free memory */
	OSS_MemFree( oss, call, call->n.memAlloc );
	call = NULL;

	return ERR_SUCCESS;
}/*DeInstall*/


/********************************* FindHashCb ********************************
 *
 *  Description: Find callback node using hash table
 *
 *---------------------------------------------------------------------------
 *  Input......: oss        oss handle
 *               cbHdl      callback handle
 *               callNr	    callback function index
 *  Output.....: return     ptr to CALL_NODE or NULL if not found
 *  Globals....: -
 ****************************************************************************/
static CALL_NODE *FindHashCb(/* nodoc */
    OSS_HANDLE       *oss,
    OSS_CALLBACK_HANDLE *cbHdl,
	u_int32          callNr)
{
	OSS_DL_LIST *hashList = &cbHdl->callHash[CH_IDX(callNr)];
	CALL_NODE *call;

	for( call=(CALL_NODE *)hashList->head; call->n.n.next;
		 call = (CALL_NODE *)call->n.n.next ){

		if( call->callNr == callNr )
			return call;
	}
	return NULL;
}

