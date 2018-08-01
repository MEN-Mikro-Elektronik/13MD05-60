/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  oss_vx_kernel_bridge.c
 *
 *      \author  uf
 *        $Date: 2008/09/05 13:28:56 $
 *    $Revision: 1.7 $
 *
 *	   \project  MDIS OSS and MEN BSP's for VxWorks
 *  	 \brief  VxWorks 6.x is unable to open a message queue or spawn a
 *               task in a system call.
 *               A task will be spawned in kernel mode...
 *
 *    \switches  -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_vx_kernel_bridge.c,v $
 * Revision 1.7  2008/09/05 13:28:56  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 1.6  2007/03/07 12:00:45  cs
 * renamed RCSid to unique identifier
 *
 * Revision 1.5  2006/10/11 18:24:33  cs
 * fixed:
 *   moved var definitions (for compile with MEN_VX_VERSION and DBG defined)
 *
 * Revision 1.4  2006/07/17 14:32:28  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 1.3  2006/06/09 09:51:28  ufranke
 * improvement
 *  + OSS_SemXCreate can be called by sysHwInit2() now ( without deadlock )
 *    Explanation: in sysHwInit2() the kernel bridge task can't be spawned
 *                 and the semXCreate() functions will be called directly
 *                 now.
 *    Requirement: OSS_Init() was not called
 *
 * Revision 1.2  2006/06/08 14:12:22  ufranke
 * cosmetics
 *
 * Revision 1.1  2006/06/02 11:04:53  ufranke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static const char RCSidVxKb[]="$Id: oss_vx_kernel_bridge.c,v 1.7 2008/09/05 13:28:56 ufranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mdis_err.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#ifdef MEN_VXWORKS_VERSION

#include <taskLib.h>
#include <msgQLib.h>
#include <string.h>

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define OSS_VXKB_ARG_BUFF_SIZE 0x200

#define OSS_VXKB_FUNCT_GET_MSGQID_SC_KERNEL		1
#define OSS_VXKB_FUNCT_SEMBCREATE_SC_KERNEL		2
#define OSS_VXKB_FUNCT_SEMCCREATE_SC_KERNEL		3
#define OSS_VXKB_FUNCT_MSGQOPEN_SC_KERNEL		4

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef struct
{
	u_int32 function;
	u_int32 args[OSS_VXKB_ARG_BUFF_SIZE/4];
	int		returnCode;
}VXKB_COMMAND;

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
static int G_vxKernelBridgeIsInit = 0;

static SEM_ID	 G_syncSem1Id = NULL;
static SEM_ID	 G_syncSem2Id = NULL;
static SEM_ID	 G_lockSemId = NULL;

static VXKB_COMMAND G_cmd;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/**********************************************************************/
/** This task waits for commands wich has to be executed in kernel mode.
 *  At first we wait for G_syncSem1Id which will be fired by a function
 *  of this module i.e. OSS_VXKB_GetMsgQIdScKernel() called in a system
 *  call.
 *  The given command will be decoded and executed in kernel mode.
 *  Then G_syncSem2Id will be fired to inform the waiting system call
 *  that the result is valid.
 *
 *  \sa		OSS_VxKernelBridgeInit
 *  \sa		OSS_VXKB_GetMsgQIdScKernel
 *
 *  \param G_cmd	global command/argument buffer
 *
 *  \return G_cmd.returnCode 0 or error code
 */
void OSS_VXKB_Task( void )
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	int error;

	for(;;)
	{
		/* synchronize1 */
		semTake( G_syncSem1Id, WAIT_FOREVER);

    	error = -1;

		/* decode request / handle request */
		switch( G_cmd.function )
		{
			case OSS_VXKB_FUNCT_GET_MSGQID_SC_KERNEL:
				{
				MSG_Q_ID msgQId = NULL;
				void *context = NULL;
				msgQId = msgQOpen( (char*)G_cmd.args, 0x200, sizeof(u_int8), MSG_Q_PRIORITY, 0, &context );
			    DBGWRT_2((DBH,"%s GET_MSGQID msgQName %s msgQid %08x\n", functionName, G_cmd.args, msgQId ));
			    if( msgQId == NULL )
			    {
			        DBGWRT_ERR(( DBH, "%s%s: msgQOpen( %s ) msgQid %08x %s%d%s\n",
    	              		    OSS_ErrorStartStr, functionName,
		    	                G_cmd.args, msgQId,
        	              		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
				}
			    else
			    {
					error = msgQClose( msgQId );
					if( error )
					{
				        DBGWRT_ERR(( DBH, "%s%s: msgQClose( %08x ) %s%d%s\n",
    		                  		OSS_ErrorStartStr, functionName,
    		                  		msgQId,
        		              		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
					}
				}
				G_cmd.args[0] = (u_int32)msgQId;
				}
				break;

			case OSS_VXKB_FUNCT_MSGQOPEN_SC_KERNEL:
				{
				MSG_Q_ID msgQId = NULL;

			    DBGWRT_2((DBH,"%s MSGQOPEN msgQName %s maxMsgs %d maxMsgLength %d options %d mode %d\n", functionName,
			    			&G_cmd.args[5], G_cmd.args[0], G_cmd.args[1], G_cmd.args[2] ));
				G_cmd.args[0] = (u_int32)msgQId;

				msgQId = msgQOpen( (char*)&G_cmd.args[5],
									G_cmd.args[0],
									G_cmd.args[1],
									G_cmd.args[2],
									G_cmd.args[3],
									(void*)G_cmd.args[4]
									);
			    DBGWRT_2((DBH,"%s msgQName %s msgQid %08x\n", functionName, G_cmd.args[5], msgQId ));
			    if( msgQId == NULL )
			    {
			        DBGWRT_ERR(( DBH, "%s%s: msgQOpen( %s ) msgQid %08x %s%d%s\n",
    	              		    OSS_ErrorStartStr, functionName,
		    	                G_cmd.args[5], msgQId,
        	              		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
				}
				else
					error = 0;

				G_cmd.args[0] = (u_int32)msgQId;
				}
				break;

			case OSS_VXKB_FUNCT_SEMBCREATE_SC_KERNEL:
				{
				SEM_ID semId = NULL;
				semId = semBCreate( G_cmd.args[0], G_cmd.args[1] );
			    DBGWRT_2((DBH,"%s semBId %08x\n", functionName, G_cmd.args, semId ));
				G_cmd.args[0] = (u_int32)semId;
				if( semId == NULL )
				{
			        DBGWRT_ERR(( DBH, "%s%s: semBCreate() %s%d%s\n",
   		                  		OSS_ErrorStartStr, functionName,
       		              		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
				}
				else
					error = 0;
				}
				break;

			case OSS_VXKB_FUNCT_SEMCCREATE_SC_KERNEL:
				{
				SEM_ID semId = NULL;
				semId = semCCreate( G_cmd.args[0], G_cmd.args[1] );
			    DBGWRT_2((DBH,"%s semCId %08x\n", functionName, G_cmd.args, semId ));
				G_cmd.args[0] = (u_int32)semId;
				if( semId == NULL )
				{
			        DBGWRT_ERR(( DBH, "%s%s: semCCreate() %s%d%s\n",
   		                  		OSS_ErrorStartStr, functionName,
       		              		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
				}
				else
					error = 0;
				}
				break;

			default:
		        DBGWRT_ERR(( DBH, "%s%s: unknown function code %d  %s%d%s\n",
                      		OSS_ErrorStartStr, functionName,
                      		G_cmd.function,
                      		OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
				break;
		}

		/* synchronize2 */
		G_cmd.returnCode = error;
		semGive( G_syncSem2Id );
	}
}


/**********************************************************************/
/** This function gets the msgQId for a system call in kernel mode.
 *  The locks G_lockSemId will be taken and the command/ argument buffer
 *  will be prepared.
 *  The G_syncSem1Id will be fired to makes waiting OSS_VXKB_Task()
 *  running. It will be waited until OSS_VXKB_Task() fires G_syncSem2Id
 *  to indicate that the result is valid.
 *  The G_lockSemId will be given at the end.
 *
 * \sa OSS_VXKB_Task
 *
 * \param  msgQName public name of a message queue i.e. "/msg_queue_name1"
 *
 * \return 0 or error code
 */
MSG_Q_ID OSS_VXKB_GetMsgQIdScKernel( char *msgQName )
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	int error = -1;
	int len;
	MSG_Q_ID kernelMsgQId = NULL;

	if( G_vxKernelBridgeIsInit == 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: not initialized %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return( 0 );
	}
  #ifndef _DIAB_TOOL
	if( sizeof(MSG_Q_ID) != sizeof(u_int32) )
	{
        DBGWRT_ERR(( DBH, "%s%s: ??? u_int64 ???  %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return( 0 );
	}
  #endif /*!_DIAB_TOOL*/

	len = strlen( msgQName );
	if( len >= (OSS_VXKB_ARG_BUFF_SIZE-1) )
		goto CLEANUP;

	semTake( G_lockSemId, WAIT_FOREVER);

	/* prepare */
	G_cmd.function = OSS_VXKB_FUNCT_GET_MSGQID_SC_KERNEL;
	memcpy( G_cmd.args, msgQName, len+1 );

	/* call / wait for ready */
	semGive( G_syncSem1Id );
	semTake( G_syncSem2Id, WAIT_FOREVER);

	/* get results */
	error = G_cmd.returnCode;
	if( !error )
	{
		kernelMsgQId = (MSG_Q_ID)G_cmd.args[0];
	}

	semGive( G_lockSemId );

CLEANUP:
	return( kernelMsgQId );
}

#endif /*MEN_VXWORKS_VERSION*/

/**********************************************************************/
/** This function gets the msgQId for a system call in kernel mode.
 *  The locks G_lockSemId will be taken and the command/ argument buffer
 *  will be prepared.
 *  The G_syncSem1Id will be fired to makes waiting OSS_VXKB_Task()
 *  running. It will be waited until OSS_VXKB_Task() fires G_syncSem2Id
 *  to indicate that the result is valid.
 *  The G_lockSemId will be given at the end.
 *
 * \sa OSS_VXKB_Task
 *
 * \param  msgQName public name of a message queue i.e. "/msg_queue_name1"
 *
 * \return 0 or error code
 */
extern MSG_Q_ID OSS_VXKB_msgQOpen(	const char * name, int maxMsgs, int maxMsgLength, int options, int mode, void *context )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
    MSG_Q_ID msgQId = NULL;
#ifdef MEN_VXWORKS_VERSION
	int len;
    int error = -1;
#endif /* MEN_VXWORKS_VERSION */

    DBGWRT_1(( DBH, "%s\n", functionName ));

#ifdef MEN_VXWORKS_VERSION
	/* VxWorks >= 6x */
	if( G_vxKernelBridgeIsInit == 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: not initialized %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return( 0 );
	}
  #ifndef _DIAB_TOOL
	if( sizeof(MSG_Q_ID) != sizeof(u_int32) )
	{
        DBGWRT_ERR(( DBH, "%s%s: ??? u_int64 ???  %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return( 0 );
	}
  #endif /*!_DIAB_TOOL*/

	len = strlen( name );
	if( len >= (OSS_VXKB_ARG_BUFF_SIZE-0x20) )
		goto CLEANUP;

	semTake( G_lockSemId, WAIT_FOREVER);

	/* prepare */
	G_cmd.function = OSS_VXKB_FUNCT_MSGQOPEN_SC_KERNEL;
	G_cmd.args[0] = maxMsgs;
	G_cmd.args[1] = maxMsgLength;
	G_cmd.args[2] = options;
	G_cmd.args[3] = mode;
	G_cmd.args[4] = (u_int32)context;

	memcpy( &G_cmd.args[5], name, len+1 );


	/* call / wait for ready */
	semGive( G_syncSem1Id );
	semTake( G_syncSem2Id, WAIT_FOREVER);

	/* get results */
	error = G_cmd.returnCode;
	if( !error )
	{
		msgQId = (MSG_Q_ID)G_cmd.args[0];
	}
	else
	{
        DBGWRT_ERR(( DBH, "%s%s: OSS_VXKB_FUNCT_MSGQOPEN_SC_KERNEL %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}

	semGive( G_lockSemId );
#else
	/* VxWorks < 6x */
	msgQId = msgQCreate( maxMsgs, maxMsgLength, options );
	if( msgQId == NULL )
	{
        DBGWRT_ERR(( DBH, "%s%s: msgQCreate %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		goto CLEANUP;
	}
#endif /*MEN_VXWORKS_VERSION*/

CLEANUP:
	return( msgQId );
}


/**********************************************************************/
/** This function creates a binary semaphore for a system call in kernel mode.
 *  The locks G_lockSemId will be taken and the command/ argument buffer
 *  will be prepared.
 *  The G_syncSem1Id will be fired to makes waiting OSS_VXKB_Task()
 *  running. It will be waited until OSS_VXKB_Task() fires G_syncSem2Id
 *  to indicate that the result is valid.
 *  The G_lockSemId will be given at the end.
 *
 * \sa OSS_VXKB_Task
 *
 * \param  msgQName public name of a message queue i.e. "/msg_queue_name1"
 *
 * \return 0 or error code
 */
SEM_ID	OSS_VXKB_semBCreate( int options, SEM_B_STATE initialState )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
    SEM_ID semId = NULL;
#ifdef MEN_VXWORKS_VERSION
    int error;
#endif /* MEN_VXWORKS_VERSION */

    DBGWRT_1(( DBH, "%s\n", functionName ));

#ifdef MEN_VXWORKS_VERSION
	/* VxWorks >= 6x */

	if( G_vxKernelBridgeIsInit == 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: Warning not initialized calling semBCreate() %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}

	if( G_vxKernelBridgeIsInit )
	{
	  #ifndef _DIAB_TOOL
		if( sizeof(SEM_ID) != sizeof(u_int32) )
		{
            DBGWRT_ERR(( DBH, "%s%s: ??? u_int64 ???  %s%d%s\n",
  	  		             	OSS_ErrorStartStr, functionName,
                          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			return( 0 );
		}
	  #endif /*!_DIAB_TOOL*/

		semTake( G_lockSemId, WAIT_FOREVER);

		/* prepare */
		G_cmd.function = OSS_VXKB_FUNCT_SEMBCREATE_SC_KERNEL;
		G_cmd.args[0] = options;
		G_cmd.args[1] = initialState;

		/* call / wait for ready */
		semGive( G_syncSem1Id );
		semTake( G_syncSem2Id, WAIT_FOREVER);

		/* get results */
		error = G_cmd.returnCode;
		if( !error )
		{
			semId = (SEM_ID)G_cmd.args[0];
		}
		else
		{
            DBGWRT_ERR(( DBH, "%s%s: OSS_VXKB_FUNCT_SEMBCREATE_SC_KERNEL %s%d%s\n",
  	  		             	OSS_ErrorStartStr, functionName,
                          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		}

		semGive( G_lockSemId );
	}
	else
#endif /*MEN_VXWORKS_VERSION*/
	{
		/* VxWorks < 6x or G_vxKernelBridgeIsInit == 0 */
		semId = semBCreate( options, initialState );
		if( semId == NULL )
		{
    	    DBGWRT_ERR(( DBH, "%s%s: semBCreate %s%d%s\n",
  			             	OSS_ErrorStartStr, functionName,
            	          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		}
	}

	return( semId );
}


/**********************************************************************/
/** This function creates a counting semaphore for a system call in kernel mode.
 *  The locks G_lockSemId will be taken and the command/ argument buffer
 *  will be prepared.
 *  The G_syncSem1Id will be fired to makes waiting OSS_VXKB_Task()
 *  running. It will be waited until OSS_VXKB_Task() fires G_syncSem2Id
 *  to indicate that the result is valid.
 *  The G_lockSemId will be given at the end.
 *
 * \sa OSS_VXKB_Task
 *
 * \param  msgQName public name of a message queue i.e. "/msg_queue_name1"
 *
 * \return 0 or error code
 */
SEM_ID	OSS_VXKB_semCCreate( int options, SEM_B_STATE initialState )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
    SEM_ID semId = NULL;
#ifdef MEN_VXWORKS_VERSION
    int error;
#endif /* MEN_VXWORKS_VERSION */

    DBGWRT_1(( DBH, "%s\n", functionName ));

#ifdef MEN_VXWORKS_VERSION
	/* VxWorks >= 6x */

	if( G_vxKernelBridgeIsInit == 0 )
	{
        DBGWRT_ERR(( DBH, "%s%s: Warning not initialized calling semCCreate() %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}

	if( G_vxKernelBridgeIsInit )
	{
	  #ifndef _DIAB_TOOL
		if( sizeof(SEM_ID) != sizeof(u_int32) )
		{
            DBGWRT_ERR(( DBH, "%s%s: ??? u_int64 ???  %s%d%s\n",
  	  		             	OSS_ErrorStartStr, functionName,
                          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			return( 0 );
		}
	  #endif /*!_DIAB_TOOL*/

		semTake( G_lockSemId, WAIT_FOREVER);

		/* prepare */
		G_cmd.function = OSS_VXKB_FUNCT_SEMCCREATE_SC_KERNEL;
		G_cmd.args[0] = options;
		G_cmd.args[1] = initialState;

		/* call / wait for ready */
		semGive( G_syncSem1Id );
		semTake( G_syncSem2Id, WAIT_FOREVER);

		/* get results */
		error = G_cmd.returnCode;
		if( !error )
		{
			semId = (SEM_ID)G_cmd.args[0];
		}
		else
		{
            DBGWRT_ERR(( DBH, "%s%s: OSS_VXKB_FUNCT_SEMBCREATE_SC_KERNEL %s%d%s\n",
  	  		             	OSS_ErrorStartStr, functionName,
                          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		}

		semGive( G_lockSemId );
	}
	else
#endif /*MEN_VXWORKS_VERSION*/
	{
		/* VxWorks < 6x or G_vxKernelBridgeIsInit == 0 */
		semId = semCCreate( options, initialState );
		if( semId == NULL )
		{
            DBGWRT_ERR(( DBH, "%s%s: semCCreate %s%d%s\n",
  	  		             	OSS_ErrorStartStr, functionName,
                          	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		}
	}

	return( semId );
}


#ifdef MEN_VXWORKS_VERSION
void showVXKBsemaphores(void)
{
	printf("semaphore G_syncSem1Id: semID = %x :", 	(unsigned int)G_syncSem1Id );
	semShow(G_syncSem1Id,1);

	printf("semaphore G_syncSem2Id: semID = %x :", 	(unsigned int)G_syncSem2Id );
	semShow(G_syncSem2Id,1);

	printf("semaphore G_lockSemId: semID = %x :", 	(unsigned int)G_lockSemId );
	semShow(G_lockSemId,1);
}
#endif /*MEN_VXWORKS_VERSION*/

/**********************************************************************/
/** This function spawns the OSS VxWorks kernel bridge task.
 *  The lock and synchronize semaphores will be created.
 *  It will be called by OSS_Init().
 *
 * \sa OSS_VXKB_Task, OSS_Init
 *
 * \param  -
 *
 * \return 0 or error code
 */
int OSS_VxKernelBridgeInit( void )
{
	int error = 0;

#ifdef MEN_VXWORKS_VERSION
    DBGCMD( const char *functionName = __FUNCTION__; )

	if( G_vxKernelBridgeIsInit == 0 )
	{
		/* init semaphores */
		G_syncSem1Id =	semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
		if( G_syncSem1Id == NULL )
		{
			error = ERR_OSS_MEM_ALLOC;
	        DBGWRT_ERR(( DBH, "%s%s: semBCreate %s%d%s\n",
  		             	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}
		G_syncSem2Id =	semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
		if( G_syncSem2Id == NULL )
		{
			error = ERR_OSS_MEM_ALLOC;
	        DBGWRT_ERR(( DBH, "%s%s: semBCreate %s%d%s\n",
                      	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}
		G_lockSemId =	semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		if( G_lockSemId == NULL )
		{
			error = ERR_OSS_MEM_ALLOC;
	        DBGWRT_ERR(( DBH, "%s%s: semBCreate %s%d%s\n",
                      	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}

		/* init command buffer */
		bzero( (char*)&G_cmd, sizeof(G_cmd) );

		/* spawn VXKB task */
		error = taskSpawn( "tOssVXKB", 				/* name */
							0x25,      				/* priority */
							0,				   		/* options */
							0x1000,			   		/* stack size */
				(FUNCPTR)	OSS_VXKB_Task,			/* task */
							1,2,3,4,5,6,7,8,9,10	/* args */
						);
		if( error == ERROR )
		{
			error = ERR_OSS_MEM_ALLOC;
	        DBGWRT_ERR(( DBH, "%s%s: taskSpawn %s%d%s\n",
                      	OSS_ErrorStartStr, functionName,
                      	OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto CLEANUP;
		}
		else
			error = 0;

		G_vxKernelBridgeIsInit = 1;
	}
CLEANUP:
#endif /*MEN_VXWORKS_VERSION*/

	return( error );
}
