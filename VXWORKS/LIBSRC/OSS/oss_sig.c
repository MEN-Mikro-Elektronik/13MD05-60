/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  oss_sig.c
 *
 *      \author  uf
 *        $Date: 2009/02/19 10:35:16 $
 *    $Revision: 1.12 $
 *
 *	   \project  MDIS OSS for VxWorks
 *  	 \brief  Signal routines
 *
 *    \switches  INCLUDE_MIPIOS_VX_TARGET for MIPIOS targets
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_sig.c,v $
 * Revision 1.12  2009/02/19 10:35:16  ufranke
 * R: G_ossSigMsgQlist not cleaned at UOS_SigInit()
 * M: OSS_VxSigInit() bzeros G_ossSigMsgQlist initialy
 * prepared
 *  + for MIPIOS
 *
 * Revision 1.11  2008/09/04 15:40:04  ufranke
 * R: diab compiler support
 * M: cosmetics
 *
 * Revision 1.10  2006/07/17 14:32:26  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 1.9  2006/06/08 14:15:48  ufranke
 * cosmetics for VxWorks 5.5
 *
 * Revision 1.8  2006/06/02 10:06:18  ufranke
 * changed
 *  - switched back to Revision 1.4 of this file
 *    for RTP support switched back to native VxWorks signal handling
 *    added message queue for queuing "MDIS signals"
 *    to fix the LIFO problem of VxWorks native signals
 *
 * Revision 1.7  2005/06/29 15:16:22  UFranke
 * fixed
 *  - memory leak OSS_SigRemove calls OSS_VxWSigFinalExit
 * improved
 *  - error handling
 *
 * Revision 1.6  2003/04/10 09:25:49  Rlange
 * Added call OSS_VxWSigInitialInit()
 *
 * Revision 1.5  2003/04/04 12:10:23  Rlange
 * reworked for using messagesQ instead of signals
 *
 * Revision 1.4  2000/06/16 11:28:44  Franke
 * changed OSS_SigRemove minor fix - set sigHdl to NULL now after parameter check
 *
 * Revision 1.3  2000/03/17 14:39:04  kp
 * file description changed
 *
 * Revision 1.2  1999/08/30 11:03:40  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:22  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999-2009 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static const char RCSid[]="$Id: oss_sig.c,v 1.12 2009/02/19 10:35:16 ufranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mdis_err.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#include <sigLib.h>
#include <msgQLib.h>
#include <taskLib.h>
#include <string.h>
#include <cacheLib.h>
#include <MEN/oss_vxworks_sig.h>

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

typedef struct
{
	OSS_DL_NODE node;
	int			taskId;
	MSG_Q_ID  	msgQueueId;
}OSS_VX_SIGMSQ_ENTRY;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static int		 G_ossVxSigIsInit = 0;
static SEM_ID	 G_ossSigSemId = NULL;

/** The double linked lists links entries of type OSS_VX_SIGMSQ_ENTRY */
static OSS_DL_LIST G_ossSigMsgQlist;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/*! \page linsigusage

  \section linsigusagesect VxWorks notes to OSS signals

  This implementation is straight forware. It uses VxWorks native
  signals. To avoid the LIFO effect of VxWorks in addition to the
  signal a message queue is used.

  See \ref osssigusagesect for more info.
*/


/**********************************************************************/
/** Init the OSS for VxWorks signal library.
 *  This function will be used internaly.
 *
 * \sa OSS_VxSigMsgQInstall
 *
 * \param  -
 *
 * \return 0 or error code
 */
int32 OSS_VxSigShow( void )
{
	OSS_VX_SIGMSQ_ENTRY  *sigMsgQEntryP;

	if( !G_ossVxSigIsInit )
	{
		printf("%s - not initialized\n", __FUNCTION__ );
		return( 0 );
	}

	sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)G_ossSigMsgQlist.head;
	while( sigMsgQEntryP )
	{
		printf("tid/msgQId %08x/%08x\n", sigMsgQEntryP->taskId, (int)sigMsgQEntryP->msgQueueId );
		sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)sigMsgQEntryP->node.next;
	}

	return( 0 );
}


/**********************************************************************/
/** Init the OSS for VxWorks signal library.
 *  This function will be used internaly.
 *
 * \sa OSS_VxSigMsgQInstall
 *
 * \param  -
 *
 * \return 0 or error code
 */
static int32 OSS_VxSigInit( void )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	STATUS error = 0;

	if( !G_ossVxSigIsInit )
	{
		/* clean anchor */
		bzero( (char*)&G_ossSigMsgQlist, sizeof(G_ossSigMsgQlist) );

		/* init semaphore */
		G_ossSigSemId =	semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
		if( G_ossSigSemId == NULL )
		{
			error = ERR_OSS_MEM_ALLOC;
	        DBGWRT_ERR(( DBH, "%s%s: semBCreate() %s%d%s\n",
	                      OSS_ErrorStartStr, functionName,
	                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}

	 	/* init list */
		OSS_DL_NewList( &G_ossSigMsgQlist );

		G_ossVxSigIsInit = 1; /* flag initialized */

		/* free semaphore */
		semGive( G_ossSigSemId );
	}

CLEANUP:
 	return( error );
}

/**********************************************************************/
/** Get the message queue corresponding to the taskId.
 *  This function will be used internaly.
 *
 * \sa AddListEntry, RemoveListEntry
 *
 * \param  taskId
 * \param  sigMsgQHdlP
 *
 * \return 0 or error code
 */
static int32 GetListEntry( int taskId, OSS_VX_SIGMSQ_ENTRY  **sigMsgQHdlP )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 error = 0;
	OSS_VX_SIGMSQ_ENTRY  *sigMsgQEntryP;

    DBGWRT_1((DBH,"%s taskId %08x\n", functionName, taskId));

	semTake( G_ossSigSemId, WAIT_FOREVER);
	sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)G_ossSigMsgQlist.head;
	while( sigMsgQEntryP )
	{
		if( sigMsgQEntryP->taskId == taskId )
		{
		    DBGWRT_2((DBH,"%s found taskId %08x msqQueueId %08x\n", functionName, taskId, sigMsgQEntryP->msgQueueId ));
			break;
		}
		sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)sigMsgQEntryP->node.next;
	}
	semGive( G_ossSigSemId );

	*sigMsgQHdlP = sigMsgQEntryP;

	if( !sigMsgQEntryP )
	{
		error = ERR_OSS_ILL_PARAM;
	}

	return( error );
}

/**********************************************************************/
/** Put the message queue corresponding to the taskId to list.
 *  This function will be used internally.
 *
 * \sa GetListEntry
 *
 * \param  taskId
 * \param  msgQid
 *
 * \return 0 or error code
 */
static int32 AddListEntry( int taskId, MSG_Q_ID msgQid )
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	OSS_VX_SIGMSQ_ENTRY *sigMsgQEntryP;
	int32 error = 0;

    DBGWRT_2((DBH,"%s taskId %08x msgQid %08x\n", functionName, taskId, msgQid ));

	/* check task ID */
	if( taskId != taskIdSelf() )
	{
		error = ERR_OSS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: taskId/sigTID %08x/%08x %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      taskIdSelf(), taskId,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		goto CLEANUP;
	}

	/* check if task is already installed */
	{
		error = GetListEntry( taskId, &sigMsgQEntryP );
		if( sigMsgQEntryP != NULL || !error )
		{
			error = ERR_OSS;
    	    DBGWRT_ERR(( DBH, "%s%s: already installed taskId %08x %s%d%s\n",
        	              OSS_ErrorStartStr, functionName,
            	          taskId,
                	      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}
		else
			error = 0; /* reset error */
	}

	/* alloc entry */
	sigMsgQEntryP = malloc( sizeof(OSS_VX_SIGMSQ_ENTRY) );
	if( !sigMsgQEntryP )
	{
		error = ERR_OSS_MEM_ALLOC;
        DBGWRT_ERR(( DBH, "%s%s: malloc %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		goto CLEANUP;
	}

	/* fill the turkey */
	bzero( (char*)sigMsgQEntryP, sizeof(OSS_VX_SIGMSQ_ENTRY) );
	sigMsgQEntryP->taskId 		= taskId;
	sigMsgQEntryP->msgQueueId 	= msgQid;

	/* add to linked list */
	semTake( G_ossSigSemId, WAIT_FOREVER);
	OSS_DL_AddTail( &G_ossSigMsgQlist, &sigMsgQEntryP->node );
	semGive( G_ossSigSemId );

CLEANUP:
	return( error );
}

/**********************************************************************/
/** Get the message queue corresponding to the taskId.
 *  This function will be used internaly.
 *
 * \sa AddListEntry
 *
 * \param  taskId
 * \param  msgQIdP
 *
 * \return 0 or error code
 */
static int32 RemoveListEntry( int taskId, MSG_Q_ID *msgQIdP )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 error = 0;
	OSS_VX_SIGMSQ_ENTRY  *sigMsgQEntryP;

    DBGWRT_1((DBH,"%s taskId %08x\n", functionName, taskId));

	*msgQIdP = NULL;

	semTake( G_ossSigSemId, WAIT_FOREVER);
	sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)G_ossSigMsgQlist.head;
	while( sigMsgQEntryP )
	{
		if( sigMsgQEntryP->taskId == taskId )
		{
			*msgQIdP = sigMsgQEntryP->msgQueueId;
		    DBGWRT_2((DBH,"%s found taskId %08x msqQueueId %08x\n", functionName, taskId, sigMsgQEntryP->msgQueueId ));
			break;
		}
		sigMsgQEntryP = (OSS_VX_SIGMSQ_ENTRY*)sigMsgQEntryP->node.next;
	}

	if( !sigMsgQEntryP )
	{
		error = ERR_OSS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: taskId %08x not found %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      taskId,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		goto CLEANUP;
	}

	/* remove from linked list */
	OSS_DL_Remove( &sigMsgQEntryP->node );

	/* safety clear */
	bzero( (char*)sigMsgQEntryP, sizeof(OSS_VX_SIGMSQ_ENTRY) );

	/* free entry */
	free( sigMsgQEntryP );

CLEANUP:
	semGive( G_ossSigSemId );
	return( error );
}

/**********************************************************************/
/** This function installs the signal message queue for a task.
 *  The message queue is public and named as "/ossSigMsgQ" + calling
 *  task ID in hex. The message queue will be deleted on last close.
 *
 * \sa UOS_SigInit, OSS_VxSigMsgQRemove
 *
 * \param  taskId	  task id
 * \param  kernelTidP task id in kernel mode( used for getting msgQName in user mode )
 * \param  maxMsgs	  max messages in queue
 *
 * \return 0 or error code
 */
int32 OSS_VxSigMsgQInstall( int taskId, int *kernelTidP, int maxMsgs, MSG_Q_ID *kernelMsgQIdP )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 error = 0;
	MSG_Q_ID msgQId_virt = NULL;
	MSG_Q_ID msgQId_phys = NULL;
	int  kernelTid;
#ifdef MEN_VXWORKS_VERSION
	/* VxWorks >= 6x */
	char msgQueueName[0x40] = "/ossSigMsgQ"; /* public message queue */
	char *str;
	void *context = NULL;
#endif

	kernelTid	= taskIdSelf(); /* taskId's differ in RTP and kernel mode */
	*kernelTidP = kernelTid;
	*kernelMsgQIdP = NULL;

#ifdef MEN_VXWORKS_VERSION
	/* VxWorks >= 6x */
	str = msgQueueName + strlen(msgQueueName);
	sprintf( str, "%08x", kernelTid );

	/* create the signal message queue */
	msgQId_virt = msgQOpen( msgQueueName, maxMsgs, sizeof(u_int8),
			 MSG_Q_PRIORITY, OM_CREATE | OM_DELETE_ON_LAST_CLOSE, &context );
	if( msgQId_virt == NULL )
	{
		error = ERR_OSS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: msgQOpen( %s ) errno %d %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      msgQueueName, errnoGet(),
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    	goto CLEANUP;
	}

	if( kernelTid == taskId )
	{
		/* called from a kernel task */
		msgQId_phys = msgQId_virt;
	}
	else
	{
		/* called from a RTP */
		msgQId_phys = OSS_VXKB_GetMsgQIdScKernel( msgQueueName );
		if( msgQId_phys == NULL )
		{
			error = ERR_OSS_ILL_PARAM;
	        DBGWRT_ERR(( DBH, "%s%s: OSS_VXKB_GetMsgQIdScKernel %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    		goto CLEANUP;
		}
	}
    DBGWRT_1((DBH,"%s taskId %08x msgQName %s virt/phys msgQId %08x/%08x\n", functionName, taskId, msgQueueName, msgQId_virt, msgQId_phys ));
#else
	/* < VxWorks 6.x */
	msgQId_virt = msgQCreate( maxMsgs, sizeof(u_int8), MSG_Q_PRIORITY );
	if( msgQId_virt == NULL )
	{
		error = ERR_OSS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: msgQCreate() errno %d %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      errnoGet(),
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    	goto CLEANUP;
	}

	/* called from a kernel task */
	msgQId_phys = msgQId_virt;
    DBGWRT_1((DBH,"%s taskId %08x msgQ virt/phys msgQId %08x/%08x\n", functionName, taskId, msgQId_virt, msgQId_phys ));
#endif


	/* check if lib is initialized */
	if( G_ossVxSigIsInit == 0 )
	{
		error = OSS_VxSigInit();
		if( error )
		{
	        DBGWRT_ERR(( DBH, "%s%s: OSS_VxSigInit() %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			error = ERR_OSS_ILL_HANDLE;
   	    	goto CLEANUP;
		}
	}

	/* install message queue for task/RTP */
	error = AddListEntry( kernelTid, msgQId_phys );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: AddListEntry() %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    	goto CLEANUP;
	}

	*kernelMsgQIdP = msgQId_phys;

CLEANUP:
	if( error && msgQId_phys )
	{
		int err2;
		#ifdef MEN_VXWORKS_VERSION
			/* VxWorks >= 6x */
			err2 = msgQClose( msgQId_phys );
			if( err2 )
			{
	        	DBGWRT_ERR(( DBH, "%s%s: msgQClose( %08x ) %s %s%d%s\n",
                	      OSS_ErrorStartStr, functionName,
                    	  msgQId_phys, msgQueueName,
	                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			}
		#else
			err2 = msgQDelete( msgQId_phys );
			if( err2 )
			{
	        	DBGWRT_ERR(( DBH, "%s%s: msgQDelete( %08x ) %s%d%s\n",
                	      OSS_ErrorStartStr, functionName,
                    	  msgQId_phys,
	                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			}
		#endif

	}
	return( error );
}

/**********************************************************************/
/** This function deinstalls a message queue ID for a task.
 *  The message queue will be closed.
 *
 * \sa UOS_SigInit, OSS_VxSigMsgQInstall
 *
 * \param  taskId	task id
 *
 * \return 0 or error code
 */
int32 OSS_VxSigMsgQRemove(  int taskId )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 error = 0;
	MSG_Q_ID msgQId = NULL;
	char msgQueueName[0x40] = "/ossSigMsgQ"; /* public message queue */
	char *str;

	str = msgQueueName + strlen(msgQueueName);
	sprintf( str, "%08x", taskId );
    DBGWRT_1((DBH,"%s taskId %08x msgQName %s\n", functionName, taskId, msgQueueName ));

	if( G_ossVxSigIsInit == 0 )
	{
		error = ERR_OSS_ILL_HANDLE;
        DBGWRT_ERR(( DBH, "%s%s: not initialized %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
   	    goto CLEANUP;
	}

	/* remove message queue entry for task/RTP */
	error = RemoveListEntry( taskId, &msgQId );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: RemoveListEntry() TID %08x %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      taskId,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    	goto CLEANUP;
	}

	/* close the signal message queue */
	#ifdef MEN_VXWORKS_VERSION
		/* VxWorks >= 6x */
		error = msgQClose( msgQId );
		if( error )
		{
			error = ERR_OSS_ILL_HANDLE;
        	DBGWRT_ERR(( DBH, "%s%s: msgQClose( %08x ) %s %s%d%s\n",
            	          OSS_ErrorStartStr, functionName,
                	      msgQId, msgQueueName,
                    	  OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    	goto CLEANUP;
		}
	#else
		error = msgQDelete( msgQId );
		if( error )
		{
			error = ERR_OSS_ILL_HANDLE;
        	DBGWRT_ERR(( DBH, "%s%s: msgQDelete( %08x ) %s%d%s\n",
            	          OSS_ErrorStartStr, functionName,
                	      msgQId,
                    	  OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    	goto CLEANUP;
		}
	#endif


CLEANUP:
	return( error );
}

/**********************************************************************/
/** Create signal handle.
 *
 * \copydoc oss_specification.c::OSS_SigCreate()
 *
 * See \ref linsigusagesect for more info.
 *
 * \VxWorks The calling task ID will be stored.
 *
 * \sa OSS_SigRemove, OSS_SigSend
 */
int32 OSS_SigCreate
(
    OSS_HANDLE       *osHdl,
    int32            value,
    OSS_SIG_HANDLE   **sigHandleP
)
{
    DBGCMD( static const char functionName[] = "OSS_SigCreate"; )
    u_int32           	gotsize;
    OSS_SIG_HANDLE   	*sigHandle;
	OSS_VX_SIGMSQ_ENTRY	*sigMsgQHdl;
	int					currentTaskId;
	int32				error;

    DBGWRT_1((DBH,"%s()\n", functionName));

	currentTaskId = taskIdSelf();

	if( G_ossVxSigIsInit == 0 )
	{
		error = ERR_OSS_ILL_HANDLE;
        DBGWRT_ERR(( DBH, "%s%s: not initialized %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
   	    goto CLEANUP;
	}

	/* get/check sigMsgQHdl for current task */
	error = GetListEntry( currentTaskId, &sigMsgQHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: GetListEntry() - TID %08x %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      currentTaskId,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    	goto CLEANUP;
	}

	*sigHandleP = NULL;
    sigHandle   = (OSS_SIG_HANDLE*) OSS_MemGet( NULL, sizeof(OSS_SIG_HANDLE), &gotsize );
    if( sigHandle == NULL )
    {
		error = ERR_OSS_MEM_ALLOC;
        DBGWRT_ERR(( DBH, "%s%s: OSS_MemGet() %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
       	return( ERR_OSS_MEM_ALLOC );
    }/*if*/

    /* fill turkey with 0 */
    OSS_MemFill( NULL, gotsize, (char*) sigHandle, 0 );

    /* fill up the turkey */
    sigHandle->ownMemSize 	= gotsize;
    sigHandle->taskId  		= currentTaskId;
    sigHandle->sigNbr  		= value;
	sigHandle->msgQueueId 	= sigMsgQHdl->msgQueueId;

  #ifdef INCLUDE_MIPIOS_VX_TARGET
	error = OSS_MIPIOS_VX_SigCreate( osHdl, value, &sigHandle->mipiosSigHdl );
	if( error )
	{
		error = ERR_OSS_MEM_ALLOC;
        DBGWRT_ERR(( DBH, "%s%s: OSS_MIPIOS_VX_SigCreate() %s%d%s\n", OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
       	goto CLEANUP;
	}
  #endif /*INCLUDE_MIPIOS_VX_TARGET*/

	*sigHandleP = sigHandle;

  CLEANUP:
    return( error );
}/*OSS_SigCreate*/


/**********************************************************************/
/** Send signal to a process
 *
 * \copydoc oss_specification.c::OSS_SigSend()
 *
 * See \ref linsigusagesect for more info.
 *
 * \sa OSS_SigCreate, OSS_SigRemove
 */
int32 OSS_SigSend
(
    OSS_HANDLE *osHdl,
    OSS_SIG_HANDLE* sigHandle
)
{
    DBGCMD( static const char functionName[] = "OSS_SigSend"; )
    STATUS error;
    u_int8 sig;

    IDBGWRT_1((DBH,"%s()\n", functionName));

    if( sigHandle == NULL )
    {
        IDBGWRT_ERR(( DBH, "%s%s: sigHandle %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        error = ERR_OSS_ILL_HANDLE;
        goto CLEANUP;
    }/*if*/

	sig = (u_int8)sigHandle->sigNbr;
    IDBGWRT_2( ( DBH,"%s() - taskId/msgQid %08x/%08x  sig=%d \n", functionName,
    			sigHandle->taskId, sigHandle->msgQueueId, (int)sig ) );

  #ifdef INCLUDE_MIPIOS_VX_TARGET
	error = OSS_MIPIOS_VX_SigSend( osHdl, sigHandle->mipiosSigHdl );
	if( error )
	{
       	IDBGWRT_ERR(( DBH, "%s%s: OSS_MIPIOS_VX_SigSend( %08x ) to task %08x %s%d%s\n", OSS_ErrorStartStr, functionName,
                      sigHandle->msgQueueId, sigHandle->taskId, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		error = ERR_OSS_SIG_SEND;
        goto CLEANUP;
	}
  #endif /*INCLUDE_MIPIOS_VX_TARGET*/

	error = msgQSend( sigHandle->msgQueueId, (char*)&sig , sizeof(u_int8), NO_WAIT, MSG_PRI_NORMAL );
    if( error )
    {
       	IDBGWRT_ERR(( DBH, "%s%s: msgQsend( %08x ) to task %08x errno %d %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      sigHandle->msgQueueId,
                      sigHandle->taskId,
                      errno,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		error = ERR_OSS_SIG_SEND;
        goto CLEANUP;
    }/*if*/

CLEANUP:
	return( error );
}/*OSS_Signal*/

/**********************************************************************/
/** Destroy signal handle.
 *
 * \copydoc oss_specification.c::OSS_SigRemove()
 *
 * See \ref linsigusagesect for more info.
 *
 * \VxWorks The function fails if the removing taskID is not equal the installing taskID.
 *
 * \sa OSS_SigCreate, OSS_SigSend
 */
int32 OSS_SigRemove
(
    OSS_HANDLE     *osHdl,
    OSS_SIG_HANDLE **sigHandleP
)
{
    DBGCMD( static const char functionName[] = "OSS_SigRemove"; )
    OSS_SIG_HANDLE   *sigHandle;
	int32			 error = 0;

    DBGWRT_1((DBH,"%s()\n", functionName));

    sigHandle   = *sigHandleP;

    if( sigHandle->taskId  != (u_int32)taskIdSelf() )  /* from the installing task */
    {
        DBGWRT_ERR(( DBH, "%s%s: taskId/sigTID %08x/%08x %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      taskIdSelf(), sigHandle->taskId,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        error = ERR_OSS_SIG_CLR;                /* can't clear ! */
        goto CLEANUP;
    }/*if*/

  #ifdef INCLUDE_MIPIOS_VX_TARGET
  	error = OSS_MIPIOS_VX_SigRemove( osHdl, &sigHandle->mipiosSigHdl );
	if( error )
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_MIPIOS_VX_SigRemove() %s%d%s\n", OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        goto CLEANUP;
	}
  #endif /*INCLUDE_MIPIOS_VX_TARGET*/

    *sigHandleP = NULL;
    OSS_MemFree( NULL, (int8*) sigHandle, sigHandle->ownMemSize );
    sigHandle = NULL;

CLEANUP:
    return( error );
}/*OSS_SigRemove*/

/**********************************************************************/
/** Get info about signal
 *
 * \copydoc oss_specification.c::OSS_SigInfo()
 */
int32 OSS_SigInfo
(
    OSS_HANDLE     *osHdl,
    OSS_SIG_HANDLE *sigHdl,
    int32          *signalNbrP,
    int32          *processIdP
)
{
    DBGCMD( static const char functionName[] = "OSS_SigInfo"; )
    DBGWRT_1((DBH,"%s()\n", functionName));
    if( sigHdl )
    {
         *signalNbrP = sigHdl->sigNbr;
         *processIdP = sigHdl->taskId;
    }
    else
    {
         *signalNbrP = 0;
         *processIdP = 0;
    }/*if*/
    return( 0 );
}/*OSS_SigInfo*/


