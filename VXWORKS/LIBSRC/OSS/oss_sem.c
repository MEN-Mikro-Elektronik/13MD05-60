 /*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_sem.c
 *      Project: OSS library
 *
 *       Author: Franke
 *        $Date: 2009/03/27 13:34:32 $
 *    $Revision: 1.6 $
 *
 *  Description: Semaphore routines
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_sem.c,v $
 * Revision 1.6  2009/03/27 13:34:32  ufranke
 * cosmetics
 *
 * Revision 1.5  2006/06/02 10:05:11  ufranke
 * fixed
 * - 2 open RTP problem,
 *   semaphores must be created in kernel mode
 *   if it should be accessible in system call
 *   with 2 different RTP's
 *
 * Revision 1.4  2005/04/12 16:56:09  kp
 * avoid name conflicts
 *
 * Revision 1.3  2000/03/17 14:39:02  kp
 * file description changed
 *
 * Revision 1.2  1999/08/30 11:03:36  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:23  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSidSem[]="$Id: oss_sem.c,v 1.6 2009/03/27 13:34:32 ufranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#include <sysLib.h>

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

/***************************** OSS_SemCreate ********************************
 *
 *  Description: creates a binary or counting semaphore and allocates
 *               memory for the OSS_SEM_HANDLE structure.
 *
 *               NOTE for VxWorks:
 *               The queuing style for the blocked task is priority.
 *
 *---------------------------------------------------------------------------
 *  Input......: osHdl    pointer to os specific struct for complicated os
 *               semType  OSS_SEM_BIN | OSS_SEM_COUNT
 *               initVal  0 - empty   | >= 1 full
 *               semHandleP pointer to semaphore handle pointer        IN OUT
 *
 *  Output.....:  0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_SemCreate
(
    OSS_HANDLE *osHdl,
    int32          semType,
    int32          initVal,
    OSS_SEM_HANDLE **semHandleP
)
{
    DBGCMD( static const char functionName[] = "OSS_SemCreate"; )
    SEM_ID        semId;
    u_int32       gotsize;
    OSS_SEM_HANDLE *semHandle;

    DBGWRT_1((DBH,"%s()\n", functionName));
    *semHandleP = NULL;

    switch( semType )
    {
       case OSS_SEM_BIN:
          if( initVal == 0 || initVal == 1 )
          {
              semId = OSS_VXKB_semBCreate( SEM_Q_PRIORITY, (SEM_B_STATE)initVal );
          }
          else
          {
              DBGWRT_ERR( ( DBH, "%s%s: initVal %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
              return( ERR_OSS_ILL_PARAM );
          }/*if*/
          break;

       case OSS_SEM_COUNT:
          if( initVal < 0 )
          {
              DBGWRT_ERR( ( DBH, "%s%s: initVal %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
              return( ERR_OSS_ILL_PARAM );
          }
          else
          {
             semId = OSS_VXKB_semCCreate( SEM_Q_PRIORITY, initVal );
          }/*if*/
          break;

       default:
          DBGWRT_ERR( ( DBH, "%s%s: semType %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
          return( ERR_OSS_ILL_PARAM );
    }/*switch*/

    if( semId != 0 )
    {
       semHandle = (OSS_SEM_HANDLE*) OSS_MemGet( NULL, sizeof( OSS_SEM_HANDLE ),
                                                   &gotsize );
       if( semHandle == 0 )
       {
          semDelete( semId );
          return( ERR_OSS_MEM_ALLOC );
       }
       else
       {
          /* fill turkey with 0 */
          OSS_MemFill( NULL, gotsize, (char*)semHandle, 0 );

          /* fill up the turkey */
          (semHandle)->ownMemSize = gotsize;
          (semHandle)->semId      = semId;
          (semHandle)->semType    = semType;
       }/*if*/
    }
    else
    {
        DBGWRT_ERR( ( DBH, "%s%s: sem(B/C)Create() %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_MEM_ALLOC );
    }/*if*/

    DBGWRT_2( ( DBH, "%s: OSS_SEM_HANDLE = 0x%08x\n",
                  functionName, semHandle ));

	*semHandleP = semHandle;
    return( 0 );
}/*OSS_SemCreate*/

/**************************** OSS_SemRemove *********************************
 *
 *  Description:  deletes the semaphore and frees the allocated memory
 *                and set the handle to 0.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                semHandleP pointer to semaphore handle pointer        IN OUT
 *
 *  Output.....:  0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_SemRemove
(
    OSS_HANDLE *osHdl,
    OSS_SEM_HANDLE** semHandleP
)
{
    DBGCMD( static const char functionName[] = "OSS_SemRemove"; )
    STATUS semRet;
    OSS_SEM_HANDLE *semHandle;

    DBGWRT_1((DBH,"%s() OSS_SEM_HANDLE = 0x%08x\n", functionName, *semHandleP ));

	semHandle   = *semHandleP;
	*semHandleP = NULL;

    if( !semHandle )
    {
        DBGWRT_ERR( ( DBH, "%s%s: semHandle %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_HANDLE );
    }/*if*/

    semRet = semDelete( (semHandle)->semId );

    if( semRet == ERROR )
    {
        DBGWRT_ERR( ( DBH, "%s%s: semDelete() %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_HANDLE );
    }/*if*/

    OSS_MemFree( NULL, (int8*) semHandle, (semHandle)->ownMemSize );
    semHandle = NULL;

    return( 0 );
}/*OSS_SemRemove*/


/****************************** OSS_SemWait *********************************
 *
 *  Description:  Takes the semaphore or waits with timeout.
 *
 *                NOTE:
 *                Timeout will be rounded up to the system tick.
 *
 *                NOTE for VxWorks:
 *                A counting semaphore is count down if value is not 0.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                semHandle
 *                msec      OSS_SEM_NOWAIT
 *                        | OSS_SEM_WAITFOREVER
 *                        | > 0 milli sec for wait
 *
 *  Output.....:  return  0 | ERR_OSS_SIG_OCCURED | ERR_OSS_TIMEOUT
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_SemWait
(
    OSS_HANDLE      *osHdl,
    OSS_SEM_HANDLE  *semHandle,
    int32           msec
)
{
    DBGCMD( static const char functionName[] = "OSS_SemWait"; )
    STATUS        SemRet;
    long          TimeOutTicks;
    SEM_ID        SemId;


    DBGWRT_1((DBH,"%s() OSS_SEM_HANDLE = 0x%08x\n", functionName, semHandle ));

    if( semHandle == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: semHandle %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_HANDLE );
    }/*if*/

    SemId  = semHandle->semId;
    DBGWRT_2((DBH,"%s() SemId = 0x%08x\n", functionName, SemId ));

    switch( msec )
    {
       case OSS_SEM_WAITFOREVER:
          TimeOutTicks = WAIT_FOREVER;
          break;

       case OSS_SEM_NOWAIT:
          TimeOutTicks = NO_WAIT;
          break;

       default:
       TimeOutTicks = (msec * sysClkRateGet()) / 1000;
       if( TimeOutTicks == 0 )
          TimeOutTicks = 1;
    }/*switch*/

    DBGWRT_2( ( DBH,"    semid=%d ticks=%ld ...\n", SemId, TimeOutTicks ) );

    SemRet = semTake( SemId, TimeOutTicks );      /* sleep until tout or sig */

    if ( SemRet != OK)          /* aborted by timeout ? || not correct SemId */
    {
	    DBGWRT_2( ( DBH,"%s: semTake error errno=0x%x\n",
						functionName, errnoOfTaskGet(0) ));
        return( ERR_OSS_TIMEOUT );
    }/*if*/

    DBGWRT_2( ( DBH,"%s: (woken) semid=%d sig=%ld\n", functionName, SemId, SemRet));

    return(0);
}/*OSS_SemWait*/

/*************************** OSS_SemSignal **********************************
 *
 *  Description:  Signals ( releases) specified semaphore.
 *
 *                NOTE: Can be called by ISR.
 *
 *                NOTE for VxWorks:
 *                A countig semaphore is count up.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                OSS_SEM_HANDLE* semHandle
 *
 *  Output.....:  0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_SemSignal
(
    OSS_HANDLE *osHdl,
    OSS_SEM_HANDLE* semHandle
)
{
    DBGCMD( static const char functionName[] = "OSS_SemSignal"; )
    SEM_ID   SemId;

    DBGWRT_1((DBH,"%s() OSS_SEM_HANDLE = 0x%08x\n", functionName, semHandle ));

    SemId  = semHandle->semId;

    DBGWRT_2( ( DBH,"     semId=%d\n", SemId ) );

    semGive( SemId );               /* wake up process */

    return(0);
}/*OSS_SemSignal*/


