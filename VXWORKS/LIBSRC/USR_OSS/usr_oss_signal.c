/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  usr_oss_signal.c
 *
 *      \author  klaus.popp@men.de/uf
 *        $Date: 2009/03/31 10:52:15 $
 *    $Revision: 1.16 $
 * 
 * 	   \project  MDIS4 USR_OSS VxWorks lib
 *
 *  	 \brief  Signal handling routines in kernel/RTP mode.
 *               source code will be shared between kernel and RTP mode.
 *               For each task using UOS signals a signal handling task will be started.
 *               This one reads the "MDIS OSS signals" from a message queue in the namespace of the 
 *               task and calls the installed
 *               user signal handler with the "MDIS OSS signal" number as argument.
 *               The signal handle of a task will be stored in a linked list.
 *               For RTP mode OS2M_DrvInstall() installs the necessary system calls
 *               used by usr_oss_signal_rtp.c .
 *
 *    \switches  VXWORKS_RTP_MODE
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_signal.c,v $
 * Revision 1.16  2009/03/31 10:52:15  ufranke
 * R: warnings if compiled for RTP mode
 * M: fixed
 *
 * Revision 1.15  2008/09/26 14:25:03  ufranke
 * R: SMP support for VxWorks 6.6
 * M: make it SMP and UP compatible
 *
 * Revision 1.14  2008/09/15 16:45:58  ufranke
 * R: diab compiler warnings
 * M: cosmetics
 *
 * Revision 1.13  2008/09/04 15:40:40  ufranke
 * R: diab compiler support
 * M: cosmetics
 *
 * Revision 1.12  2006/07/20 15:04:54  ufranke
 * cosmetics
 *
 * Revision 1.11  2006/07/17 14:33:23  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 1.10  2006/06/08 14:24:43  ufranke
 * cosmetics for VxWorks 6.2
 *
 * Revision 1.9  2006/06/02 11:12:02  ufranke
 * - switched back to Revision 1.4 of this file
 *   for RTP support
 *   added message queue for queuing "MDIS signals"
 *   to fix the LIFO problem of VxWorks signals
 *
 * Revision 1.8  2005/07/05 10:45:02  kp
 * Now DBG_Init is referenced only if compiled with -DDBG
 *
 * Revision 1.7  2005/06/29 15:13:47  UFranke
 * fixed
 *  - memory leak UOS_VxWSigExit calls OSS_VxWSigFinalExit now
 * improved
 *  - error handling
 *
 * Revision 1.6  2003/04/10 09:26:49  Rlange
 * added call OSS_VxWSigInitialInit()
 *
 * Revision 1.5  2003/04/04 12:11:31  Rlange
 * totally reworked for using messageQ instead of signals
 *
 * Revision 1.4  2001/12/03 16:34:15  Franke
 * cosmetics
 *
 * Revision 1.3  2001/11/08 09:55:30  Franke
 * bugfix UOS_SigWait() no longer clearing signals before waiting
 *        so events may have been lost
 *
 * Revision 1.2  1999/08/31 10:53:51  Franke
 * added DBG
 *
 * Revision 1.1  1999/06/10 15:37:34  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2008 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char UOS_SIGNAL_RCSid[]="$Id: usr_oss_signal.c,v 1.16 2009/03/31 10:52:15 ufranke Exp $";

/*! \page linuossigusage

  \section linuossigusagesect Using USR_OSS Signals on VxWorks

  See \ref uossigusagesect for a general description of USR_OSS
  signal handling.

  Under VxWorks, each task using UOS signal handling creates its own signal
  handler task. This local signal handler gets the "MDIS OSS signals". 
  This "signal number" will be taken by msgQueueReceive(). 
  The user signal handler installed for the received signal will be 
  called than with the received signal number as argument.
  This happens in the local signal handler until all queued "MDIS OSS signals"
  are handled.

  The "MDIS OSS signals" *will* be queued in a message queue. (Totally max. 512)
  It's recommended to use only these signals \c UOS_SIG_USR1 and \c UOS_SIG_USR2
  if it is required to be source compatible with MDIS of other operating systems.
  MDIS OSS signals can be 1..255 under VxWorks.
*/


#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"
#ifndef VXWORKS_RTP_MODE
	#include <sigLib.h>
	#include <logLib.h>
#else
	#include <errnoLib.h>
	#include <signal.h>
	#include <msgQLib.h>
	#include <stdlib.h>
	#include <MEN/mdis_rtp.h>
	#include <syscall.h>
#endif
#include <semLib.h>
#include <dllLib.h>       /* double link list lib */
#include <sysLib.h>
#include <taskLib.h>
#include <string.h>
#include <MEN/oss_vxworks_sig.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define UOS_SIG_MAX_MSG		0x200	/* max queued "signals" in the message queue */

#ifndef VXWORKS_RTP_MODE
	#define UOS_SIG_DL_LIST_MAX		0x40
	#define UOS_SIG_DL_LIST_MASK	UOS_SIG_DL_LIST_MAX-1
	static SEM_ID	G_dataSem = NULL;		/* global UOS sig kernel semaphore */
#else
	#define UOS_SIG_DL_LIST_MAX		1	 /* in RTP mode we have one DLL for each task */
#endif

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
typedef struct
{
    DL_NODE 	node;
	int			taskId;
	int			kernelTid;
	SEM_ID		sigWaitSem;
	MSG_Q_ID	msgQueueId;
	void    	(*usrSigHandler)( u_int32 sigNo );
    int			lastSignal;
	int			uosSigHdlTid;
}UOS_SIG_HDL;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
u_int32 UOS_SigHandlerCount = 0;
u_int32 UOS_SigHandlerMsgReceiveCount = 0;

/*--------------------------------------+
|   STATICS                             |
+--------------------------------------*/
/* array of 64 link lists to improve the performance of the UOS_SigHandler
 * if more task have installed UOS signal handlers 
 */
static DL_LIST UOS_SigList[UOS_SIG_DL_LIST_MAX];  /** lists of signal handles per task */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static UOS_SIG_HDL *getTaskSigHdl( void );
static void UOS_SigHandler( UOS_SIG_HDL *taskSigHdl );


/**********************************************************************/
/** Locks the access to the UOS sig DLL via a global semaphore.
 *  
 * \param  -
 *
 * \return -
 */
static void UOS_DataLock( void )
{
#ifndef VXWORKS_RTP_MODE
	if( G_dataSem == NULL )
	{
		G_dataSem = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		if( G_dataSem == NULL )
		{
			for(;;)
			{
				logMsg("*** %s semBCreate line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
				taskDelay( sysClkRateGet() *5 );
			}
		}
	}
	
	if( semTake( G_dataSem, WAIT_FOREVER ) != OK )
	{
		for(;;)
		{
			logMsg("*** %s semTake line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
			taskDelay( sysClkRateGet() *5 );
		}
	}
#endif
}

/**********************************************************************/
/** Unlocks the access to the UOS sig DLL via a global semaphore.
 *  
 * \param  -
 *
 * \return -
 */
static void UOS_DataUnlock( void )
{
#ifndef VXWORKS_RTP_MODE
	if(  G_dataSem == NULL || semGive( G_dataSem ) != OK )
	{
		for(;;)
		{
			logMsg("*** %s semGive line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
			taskDelay( sysClkRateGet() *5 );
		}
	}
#endif
}

/**********************************************************************/
/** Computes a 8bit hash value of the task ID.
 *  This function will be used internaly.
 *  
 * \param  tid	task ID
 *
 * \return 0..255 hash or 0 always in RTP mode 
 */
static __inline__ int taskHashGet( int tid )
{
#ifndef VXWORKS_RTP_MODE
	register int hash;

	hash = (tid >> 24) + ( (tid >> 16) & 0xff )  + ( (tid >> 8) & 0xff ) + (tid & 0xff );
	hash &= UOS_SIG_DL_LIST_MASK;
	return( hash );
#else
	return( 0 ); /* in RTP mode we have one DLL for each task */
#endif
}

#ifdef VXWORKS_RTP_MODE
/**********************************************************************/
/** RTP mode system call interface for OSS_VxSigMsgQInstall.
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
	char scName[0x100];
	size_t  size = sizeof(scName);
	int32 error;

	/* check installed system call */
	error = syscallPresent( MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL, scName, &size );
	if( error )
	{
		printf( "*** %s syscallPresent() syscall %x - looks like MDIS/UOS system calls not installed\n",
				__FUNCTION__,
		        MDIS_RTP_SYSCALL_M_OPEN );
		goto CLEANUP;
	}
	if( strncmp( scName, "OSS_VxSigMsgQInstall", 20 ) )
	{
		error = -1;
		printf( "*** %s syscall %x %s not OSS_VxSigMsgQInstall - looks like MDIS/UOS system calls not installed\n",
				__FUNCTION__,
				MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL, scName );
		goto CLEANUP;
	}

	/* OSS_VxSigMsgQInstall SC */
	error = syscall( taskId, (int)kernelTidP, maxMsgs, (int)kernelMsgQIdP,5,6,7,8, MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL );

CLEANUP:
	return( error );
}

/**********************************************************************/
/** RTP mode system call interface for OSS_VxSigMsgQRemove.
 *  
 * \sa UOS_SigExit, OSS_VxSigMsgQInstall
 *  
 * \param  taskId
 * \param  msgQid
 *
 * \return 0 or error code
 */
int32 OSS_VxSigMsgQRemove(  int taskId )
{
	char scName[0x100];
	size_t  size = sizeof(scName);
	int32 error;

	/* check installed system call */
	error = syscallPresent( MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_REMOVE, scName, &size );
	if( error )
	{
		printf( "*** %s syscallPresent() syscall %x - looks like MDIS/UOS system calls not installed\n",
				__FUNCTION__,
		        MDIS_RTP_SYSCALL_M_OPEN );
		goto CLEANUP;
	}
	if( strncmp( scName, "OSS_VxSigMsgQRemove", 20 ) )
	{
		error = -1;
		printf( "*** %s syscall %x %s not OSS_VxSigMsgQRemove - looks like MDIS/UOS system calls not installed\n",
				__FUNCTION__,
				MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_REMOVE, scName );
		goto CLEANUP;
	}

	/* OSS_VxSigMsgQInstall SC */
	error = syscall( taskId, 2,3,4,5,6,7,8, MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_REMOVE );

CLEANUP:
	return( error );
}
#endif /* VXWORKS_RTP_MODE */


/**********************************************************************/
/** Init signal handling
 *
 * \copydoc usr_oss_specification.c::UOS_SigInit()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigExit
 */
int32 UOS_SigInit(void (*sigHandler)(u_int32 sigCode))
{
	DBGCMD( static char functionName[] = "UOS_SigInit"; )
	UOS_SIG_HDL *taskSigHdl = NULL;
	SEM_ID		sigWaitSem 	= NULL;
	MSG_Q_ID	msgQueueId	= (MSG_Q_ID)NULL;
	int32		error;
	int			kernelTid = 0;
	char 		uosSigTaskName[0x40] = "tUosSig"; /* public message queue */
	char *str;

    DBGCMD( if( !UOS_DbgHdl ) DBG_Init( NULL, &UOS_DbgHdl ); );
    DBGWRT_1((DBH,"%s()\n", functionName));

    error = 0; 
    if( getTaskSigHdl() )
    {
   	    DBGWRT_ERR((DBH,"*** %s already installed line %d\n", functionName, __LINE__ ));
		error = ERR_UOS_BUSY;
        goto CLEANUP;
	}

    sigWaitSem = semBCreate( SEM_Q_PRIORITY, SEM_EMPTY  );
    if( sigWaitSem == NULL )
    {
   	    DBGWRT_ERR((DBH,"*** %s no mem line %d\n", functionName, __LINE__ ));
		error = ERR_UOS_MEM_ALLOC;
        goto CLEANUP;
	}

    taskSigHdl = (UOS_SIG_HDL*) malloc( sizeof(UOS_SIG_HDL) );
    if( taskSigHdl == NULL )
	{
   	    DBGWRT_ERR((DBH,"*** %s no mem line %d\n", functionName, __LINE__ ));
		error = ERR_UOS_MEM_ALLOC;
        goto CLEANUP;
	}/*if*/

    /* fill the turkey */
    memset( (char*)taskSigHdl, 0, sizeof(UOS_SIG_HDL) );
    taskSigHdl->sigWaitSem    = sigWaitSem;
	taskSigHdl->usrSigHandler = sigHandler;
	taskSigHdl->lastSignal    = 0;

	/* message queue */
    error = OSS_VxSigMsgQInstall( taskIdSelf(), &kernelTid, UOS_SIG_MAX_MSG, &msgQueueId );
    if( error || msgQueueId == (MSG_Q_ID)NULL || kernelTid == 0 )
    {
   	    DBGWRT_ERR((DBH,"*** %s OSS_VxSigMsgQInstall err/msgQId/kernelTid %d/%08x/%08x line %d\n", functionName,
   	    			(int)error, msgQueueId, kernelTid, __LINE__ ));
		error = ERR_UOS_ILL_PARAM;
        goto CLEANUP;
	}

    taskSigHdl->taskId = taskIdSelf();
    taskSigHdl->kernelTid = kernelTid;

#ifdef VXWORKS_RTP_MODE
	{
		/* get the message queue id in RTP mode */
		char msgQueueName[0x40] = "/ossSigMsgQ"; /* public message queue */
		void *context = NULL;
	
		str = msgQueueName + strlen(msgQueueName);
		sprintf( str, "%08x", kernelTid ); 

		/* create the signal message queue */
		msgQueueId = msgQOpen( msgQueueName, UOS_SIG_MAX_MSG, sizeof(u_int8),
				 MSG_Q_PRIORITY, OM_DELETE_ON_LAST_CLOSE, &context );
		if( msgQueueId == (MSG_Q_ID)NULL )
		{
			error = ERR_UOS_ILL_PARAM;
	    	DBGWRT_ERR((DBH,"*** %s msgQOpen( %s ) errno %d line %d\n", functionName, msgQueueName, errno, __LINE__ ));
    		goto CLEANUP;
		}
    	DBGWRT_1((DBH,"%s RTP msgQName %s msgQId %08x\n", functionName, msgQueueName, msgQueueId ));
	}
#endif /*VXWORKS_RTP_MODE*/

    taskSigHdl->msgQueueId    = msgQueueId;

	str = uosSigTaskName + strlen(uosSigTaskName);
	sprintf( str, "%08x", kernelTid ); 

    DBGWRT_1((DBH,"%s() taskId/kernelTid/msgQueueId %08x/%08x/%08x sigHdlTask %s\n", functionName, 
    				taskSigHdl->taskId, kernelTid, taskSigHdl->msgQueueId, uosSigTaskName ));
	
	taskSigHdl->uosSigHdlTid  = taskSpawn( uosSigTaskName,		/* name */
							0,      							/* priority */
							0,				   					/* options */
							0x1000,			   					/* stack size */
				(FUNCPTR)	UOS_SigHandler,						/* task */
							(int)taskSigHdl,2,3,4,5,6,7,8,9,10	/* args */
						);		
	if( taskSigHdl->uosSigHdlTid  == ERROR )
	{
		error = ERR_UOS_MEM_ALLOC;
        DBGWRT_ERR(( DBH, "%s%s: taskSpawn %s%d%s\n",
                      	UOS_ErrorStartStr, functionName, 
                      	UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
		goto CLEANUP;
	}
	
    UOS_DataLock();
    dllAdd( &UOS_SigList[taskHashGet(taskSigHdl->taskId)], &taskSigHdl->node );
    UOS_DataUnlock();
    
	return(0);

CLEANUP:
	if( sigWaitSem != NULL )
		semDelete( sigWaitSem );

	if( msgQueueId != (MSG_Q_ID)NULL )
		msgQDelete( msgQueueId );

    if( taskSigHdl != NULL )
		free( taskSigHdl );
		
	return( error );
}/*UOS_SigInit*/

/**********************************************************************/
/** Terminate signal handling
 *
 * \copydoc usr_oss_specification.c::UOS_SigExit()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigInit
 */
int32 UOS_SigExit(void)
{
DBGCMD( static char functionName[] = "UOS_SigExit"; )
UOS_SIG_HDL *taskSigHdl;
int32		error;

    DBGWRT_1((DBH,"%s()\n", functionName));

    error = 0;
	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
    {
	    DBGWRT_ERR((DBH,"*** %s line %d\n", functionName, __LINE__ ));
		return(ERR_UOS_NOT_INIZED);
	}

	/* remove it from linked list */
    UOS_DataLock();
    dllRemove( &UOS_SigList[taskHashGet(taskSigHdl->taskId)], &taskSigHdl->node );
    UOS_DataUnlock();

#ifdef VXWORKS_RTP_MODE
	{
		/* close the signal message queue */
		error = msgQClose( taskSigHdl->msgQueueId );
		if( error )
		{
			error = ERR_UOS_ILL_PARAM;
	    	DBGWRT_ERR((DBH,"*** %s msgQClose( %08x ) errno %d line %d\n", functionName, taskSigHdl->msgQueueId, errno, __LINE__ ));
	        return( error );
		}
	}
#endif /*VXWORKS_RTP_MODE*/

	if( taskSigHdl->uosSigHdlTid > 0 )
	{
		error = taskDelete( taskSigHdl->uosSigHdlTid );
		if( error )
		{
	   	    DBGWRT_ERR((DBH,"*** %s taskDelete( %08x ) errno %d line %d\n",
   	    			functionName,
   	    			taskSigHdl->uosSigHdlTid, errno, __LINE__ ));
			error = ERR_UOS_ILL_PARAM;
		}
	}

    error = OSS_VxSigMsgQRemove( taskSigHdl->kernelTid );
    if( error )
    {
   	    DBGWRT_ERR((DBH,"*** %s OSS_VxSigMsgQRemove( %08x )\n", functionName, taskSigHdl->kernelTid ));
		error = ERR_UOS_ILL_PARAM;
        return( error );
	}

	/* free sem */
	if( taskSigHdl->sigWaitSem )
		error |= semDelete( taskSigHdl->sigWaitSem );

	/* free it */
    free( taskSigHdl );

	if( error )
		return( ERROR );
	else
		return(0);
}/*UOS_SigExit*/

/**********************************************************************/
/** Install a signal to be received
 *
 * \copydoc usr_oss_specification.c::UOS_SigInstall()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \VxWorks No \c ERR_UOS_BUSY error is returned if signal already installed.
 * \sa UOS_SigRemove
 */
int32 UOS_SigInstall( u_int32 sigCode )
{
DBGCMD( static char functionName[] = "UOS_SigInstall"; )
UOS_SIG_HDL *taskSigHdl;
	
    DBGWRT_1((DBH,"%s sig=%d\n", functionName, (int)sigCode));

	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
    {
	    DBGWRT_ERR((DBH,"*** %s line %d\n", functionName, __LINE__ ));
		return(ERR_UOS_NOT_INIZED);
	}

	if( sigCode < UOS_SIG_USR1 || UOS_SIG_USR_MAX_VXWORKS < sigCode )
    {
	    DBGWRT_ERR((DBH,"*** %s line %d\n", functionName, __LINE__ ));
		return( ERR_UOS_ILL_SIG  );
	}

	return( 0 );
}/*UOS_SigInstall*/

/**********************************************************************/
/** Remove signal to be received
 *
 * \copydoc usr_oss_specification.c::UOS_SigRemove()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigInstall
 */
int32 UOS_SigRemove(u_int32 sigCode)
{
DBGCMD( static char functionName[] = "UOS_SigRemove"; )
UOS_SIG_HDL *taskSigHdl;
	
    DBGWRT_1((DBH,"%s sig=%d\n", functionName, (int)sigCode));

	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
    {
	    DBGWRT_ERR((DBH,"*** %s line %d\n", functionName, __LINE__ ));
		return(ERR_UOS_NOT_INIZED);
	}

	if( sigCode < UOS_SIG_USR1 || UOS_SIG_USR_MAX_VXWORKS < sigCode )
    {
	    DBGWRT_ERR((DBH,"*** %s line %d\n", functionName, __LINE__ ));
		return( ERR_UOS_ILL_SIG  );
	}

	return(0);
}/*UOS_SigRemove*/


/**********************************************************************/
/** Mask all signals
 *
 * \copydoc usr_oss_specification.c::UOS_SigMask()
 *
 * \VxWorks masks all signals installed by UOS_SigInstall()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigUnMask
 */
int32 UOS_SigMask(void)
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	UOS_SIG_HDL *taskSigHdl;
	int32	error;
	
    DBGWRT_1((DBH,"%s()\n", functionName));

	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
    {
	    DBGWRT_ERR((DBH,"*** %s not initialized line %d\n", functionName, __LINE__ ));
		return(ERR_UOS_NOT_INIZED);
	}

	error = taskSuspend( taskSigHdl->uosSigHdlTid );
	if( error )
	{
	    DBGWRT_ERR((DBH,"*** %s taskSuspend line %d\n", functionName, __LINE__ ));
		return( ERR_UOS_ILL_PARAM );
	}
	return( error );
}

/**********************************************************************/
/** Unmask all signals
 *
 * \copydoc usr_oss_specification.c::UOS_SigUnMask()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigMask
 */
int32 UOS_SigUnMask(void)
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	UOS_SIG_HDL *taskSigHdl;
	int32	error;
	
    DBGWRT_1((DBH,"%s()\n", functionName));

	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
    {
	    DBGWRT_ERR((DBH,"*** %s not initialized line %d\n", functionName, __LINE__ ));
		return(ERR_UOS_NOT_INIZED);
	}

	error = taskResume( taskSigHdl->uosSigHdlTid );
	if( error )
	{
	    DBGWRT_ERR((DBH,"*** %s taskSuspend line %d\n", functionName, __LINE__ ));
		return( ERR_UOS_ILL_PARAM );
	}
	return( error );
}

/**********************************************************************/
/** Wait until signal received
 *
 * \copydoc usr_oss_specification.c::UOS_SigWait()
 *
 * \VxWorks See \ref linuossigusagesect to see how signals handling is
 * implemented for VxWorks.
 *
 * \sa UOS_SigInit, UOS_SigInstall
 */
int32 UOS_SigWait(u_int32 msec, u_int32 *sigCodeP)
{
DBGCMD( static char functionName[] = "UOS_SigWait"; )
UOS_SIG_HDL *taskSigHdl;
int			ticksToWait;
int 		retCode;
	
    DBGWRT_1((DBH,"%s()\n", functionName));

	taskSigHdl = getTaskSigHdl();
    if( taskSigHdl == NULL )
		return(ERR_UOS_NOT_INIZED);

    if( msec )
	{
		ticksToWait = msec * sysClkRateGet()/1000;
		if( ticksToWait == 0 )
			ticksToWait++;
	}
	else
		ticksToWait = WAIT_FOREVER;

	/* unmask signals */
	UOS_SigUnMask();

    retCode = semTake( taskSigHdl->sigWaitSem, ticksToWait);
	if( retCode == OK )
		*sigCodeP = taskSigHdl->lastSignal;
	else
		return(ERR_UOS_TIMEOUT);

	return(0);
}/*UOS_SigWait*/


/****************************************************************************
 *
 * STATIC Functions
 * 
 */

/**********************************************************************/
/** Get the signal handle of the current task.
 *
 * Get the signal handle of the current task.
 *
 */
static UOS_SIG_HDL *getTaskSigHdl( void ) /* nodoc */
{
	UOS_SIG_HDL *taskSigHdl;
	UOS_SIG_HDL *returnTaskSigHdl;
	int			taskId;
	int         tHash;

    returnTaskSigHdl = NULL;
    taskId = taskIdSelf(); 
    tHash  = taskHashGet(taskId);

    /* looking for */
    for( taskSigHdl = (UOS_SIG_HDL*) DLL_FIRST( &UOS_SigList[tHash] );
         taskSigHdl != NULL;
         taskSigHdl = (UOS_SIG_HDL*) DLL_NEXT (&taskSigHdl->node)
       )
    {
		if( taskId == taskSigHdl->taskId )
		{
			returnTaskSigHdl = taskSigHdl;
			break;
		}/*if*/
    }/*for*/

	return( returnTaskSigHdl );
}/*getTaskSigHdl*/

/**********************************************************************/
/** UOS_OSS VxWorks signal handler.
 *  This local signal handler receives the "signal numbers" from the
 *  per task installed message queue and calls the user signal handler
 *  with this number as argument.
 */
static void UOS_SigHandler( UOS_SIG_HDL *taskSigHdl )
{
	DBGCMD( static char functionName[] = ">>> UOS_SigHandler:"; )
	u_int32 sigNo;
	u_int8  sig;
	int error;
	
    DBGWRT_1((DBH,"%s ", functionName));
    
    UOS_SigHandlerCount++;

    if( taskSigHdl == NULL )
		return;

    for(;;)
	{
		error = msgQReceive( taskSigHdl->msgQueueId, (char*)&sig, sizeof(u_int8), WAIT_FOREVER );
		if( error != ERROR )
		{
		    UOS_SigHandlerMsgReceiveCount++;

		    DBGWRT_1((DBH,"%s ", functionName));
		    DBGWRT_1((DBH," sig=%d ", (int)sig));
			sigNo = (u_int32) sig;

			/* call user signal handler if installed */
			if( taskSigHdl->usrSigHandler )
				taskSigHdl->usrSigHandler( sigNo );

			taskSigHdl->lastSignal = sigNo;
    		semGive( taskSigHdl->sigWaitSem );
		    DBGWRT_1((DBH,"<<<\n"));
    	}
    }

}/*UOS_SigHandler*/


