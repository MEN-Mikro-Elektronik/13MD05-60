/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_alarm.c
 *      Project: OSS library
 *
 *       Author: Franke
 *        $Date: 2008/09/26 13:51:09 $
 *    $Revision: 1.7 $
 *
 *  Description: OSS Alarm routines
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_alarm.c,v $
 * Revision 1.7  2008/09/26 13:51:09  ufranke
 * R: VxWorks 6.6 SMP
 * M: internal changes
 *
 * Revision 1.6  2007/03/07 12:00:11  cs
 * added casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.5  2006/02/14 17:11:13  ts
 * removed RCSid String due to compile error
 *
 * Revision 1.4  2000/03/17 14:38:55  kp
 * file description changed
 *
 * Revision 1.3  2000/03/16 16:05:24  kp
 * changed default priority of alarm thread to 0
 * (otherwise alarms cannot preempt tShell)
 *
 * Revision 1.2  1999/08/30 11:03:24  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:25  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mdis_err.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#include <sysLib.h>
#include <dllLib.h>       /* double link list lib */

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef struct
{
    DL_NODE node;       /* double link list */
    u_int32 ownMemSize;
    void (*funct)();
    void *arg;
    int  active;
    int  cyclic;
    u_int  priority;
    u_int  currentPriority;
}OSS_INT_ALARM_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
u_int32 OSS_AlarmTaskId        = 0;
u_int32 OSS_AlarmTaskPriority  = 0;
u_int32 OSS_AlarmTaskStackSize = 0x1000;
u_int32 OSS_AlarmInstCounter   = 0;
DL_LIST OSS_AlarmList;  /* link list of alarm handlers */

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
void OSS_AlarmScheduler( void );

/*************************** OSS_AlarmCreate **********************************
 *
 *  Description:  Creates the alarm handle.
 *                Allocates memory and fills the alarm structure.
 *                Spawns the alarm task at the first create.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   oss handle for complicated OS
 *                funct   alarm function to call
 *                arg     argument for the alarm function
 *                alarmP  pointer to variable where the alarm handle will stored
 *
 *  Output.....:  *alarmP alarm handle
 *                return  success (0) or error code
 *
 *  Globals....:  OSS_AlarmTaskId, OSS_AlarmTaskPriority,OSS_AlarmTaskStackSize
 *                OSS_AlarmInstCounter, OSS_AlarmList
 ****************************************************************************/
int32 OSS_AlarmCreate
(
    OSS_HANDLE       *osHdl,
    void             (*funct)(),
    void             *arg,
    OSS_ALARM_HANDLE **alarmP
)
{
    DBGCMD( static const char functionName[] = "OSS_AlarmCreate"; )
    u_int32 gotsize;
    int32   retCode = ERR_SUCCESS;
    OSS_INT_ALARM_HANDLE *internAlarmHdl;

    DBGWRT_1((DBH,"%s()\n", functionName));

    *alarmP = NULL;
    /*----------------------+
    |  spawn the alarm task |
    +----------------------*/
    if( OSS_AlarmTaskId == 0 && OSS_AlarmInstCounter == 0 )
    {
        OSS_AlarmTaskId = taskSpawn( "tOssAlarmTask", OSS_AlarmTaskPriority,
                                     0, OSS_AlarmTaskStackSize,
                                     (FUNCPTR) OSS_AlarmScheduler, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
        if( (int32)OSS_AlarmTaskId == ERROR )
        {
            retCode = errnoGet();
            DBGWRT_ERR( ( DBH, "%s%s: taskSpawn() failed with errno 0x%08x %s%d%s\n",
                          OSS_ErrorStartStr, functionName, retCode, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ) );
            retCode = ERR_OSS_ALARM_CREATE;
            goto CLEANUP;
        }/*if*/
    }/*if*/
    OSS_AlarmInstCounter++;

    /*----------------------+
    |  alloc/init handle    |
    +----------------------*/
    internAlarmHdl = (OSS_INT_ALARM_HANDLE*) OSS_MemGet(osHdl, sizeof(OSS_INT_ALARM_HANDLE), &gotsize);
    if( internAlarmHdl == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: OSS_MemGet() failed %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        retCode = ERR_OSS_MEM_ALLOC;
        goto CLEANUP;
    }/*if*/

    /* fill turkey with 0 */
    OSS_MemFill( osHdl, gotsize, (char*) internAlarmHdl, 0 );

    internAlarmHdl->ownMemSize = gotsize;
    internAlarmHdl->funct    = funct;
    internAlarmHdl->arg      = arg;
    internAlarmHdl->active   = FALSE;
    internAlarmHdl->cyclic   = FALSE;
    internAlarmHdl->priority = 0;
    internAlarmHdl->currentPriority = 0;

	_OSS_DataLock();
    dllAdd( &OSS_AlarmList, &internAlarmHdl->node);
	_OSS_DataUnlock();

    *alarmP = (OSS_ALARM_HANDLE*) internAlarmHdl;

CLEANUP:
    return( retCode );
}/*OSS_AlarmCreate*/

/*************************** OSS_AlarmRemove **********************************
 *
 *  Description:  Remove a alarm handle.
 *                De-activate alarm if already installed.
 *                Deallocates alarm structure.
 *                Kills the alarm task at the last remove.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   oss handle for complicated OS
 *                alarmP  pointer to variable where the alarm handle is stored
 *
 *  Output.....:  *alarmP alarm handle
 *                return   success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_AlarmRemove
(
    OSS_HANDLE       *osHdl,
    OSS_ALARM_HANDLE **alarmP
)
{
    DBGCMD( static const char functionName[] = "OSS_AlarmRemove"; )
    int32 retCode;
    OSS_INT_ALARM_HANDLE *internAlarmHdl    = NULL;
    OSS_INT_ALARM_HANDLE *internAlarmHdlCmp = NULL;

    DBGWRT_1((DBH,"%s()\n", functionName));

    /*-----------------------+
    |  check alarm handle    |
    +-----------------------*/
    if( alarmP == NULL || internAlarmHdl == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: illegal alarm handle %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_PARAM );
    }/*if*/

    internAlarmHdl = (OSS_INT_ALARM_HANDLE*) *alarmP;

    /*-----------------------+
    |  delete the alarm task |
    +-----------------------*/
    OSS_AlarmInstCounter--;
    if( OSS_AlarmTaskId != 0 && OSS_AlarmInstCounter == 0 )
    {
        retCode = taskDelete( OSS_AlarmTaskId );
        if( retCode == ERROR )
        {
            retCode = errnoGet();
            DBGWRT_ERR( ( DBH, "%s%s: taskDelete() failed with errno 0x%08x %s%d%s\n",
                          OSS_ErrorStartStr, functionName, retCode, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ) );
            return( ERR_OSS_ALARM_REMOVE );
        }/*if*/
        OSS_AlarmTaskId = 0;
    }/*if*/

    /*-----------------------+
    |  de-activate alarm     |
    +-----------------------*/
    if( internAlarmHdl->active )
    {
        if( ( retCode = OSS_AlarmClear(osHdl, *alarmP) ) )
        {
            DBGWRT_ERR( ( DBH, "%s%s: OSS_AlarmClear() failed %s%d%s\n",
                          OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
            return( retCode );
        }/*if*/
    }/*if*/

    /*-----------------------+
    |  remove from list      |
    +-----------------------*/
    for( internAlarmHdlCmp = (OSS_INT_ALARM_HANDLE*) DLL_FIRST( &OSS_AlarmList );
         internAlarmHdlCmp != NULL;
         internAlarmHdlCmp = (OSS_INT_ALARM_HANDLE*) DLL_NEXT (&internAlarmHdlCmp->node)
       )
    {
        if( internAlarmHdl == internAlarmHdlCmp )
        {
			_OSS_DataLock();
            dllRemove( &OSS_AlarmList, &internAlarmHdlCmp->node);
			_OSS_DataUnlock();
            OSS_MemFree( osHdl, (int8*)internAlarmHdl, internAlarmHdl->ownMemSize );
            internAlarmHdl = NULL;
            return( ERR_SUCCESS );
        }/*if*/
    }/*for*/

    DBGWRT_ERR( ( DBH, "%s%s: not in OSS_AlarmList %s%d%s\n",
                  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    return( ERR_OSS_ALARM_REMOVE );
}/*OSS_AlarmRemove*/

/*************************** OSS_AlarmSet **********************************
 *
 *  Description:  Activates an alarm subroutine
 *                Returns an error if already active
 *
 *                Alarm time may rounded up to the systems minimum ticker
 *                resolution. This (rounded) time is retured via realMsec.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl      oss handle for complicated OS
 *                alarm      alarm handle
 *                msec       alarm time [msec]
 *                cyclic     cyclic (1) or single (0) alarm
 *                realMsecP  pointer to variable where rounded alarm time [msec]
 *                           stored
 *  Output.....:  *realMsecP rounded alarm time [msec]
 *                return   success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_AlarmSet
(
    OSS_HANDLE       *osHdl,
    OSS_ALARM_HANDLE *alarm,
    u_int32          msec,
    u_int32          cyclic,
    u_int32          *realMsecP
)
{
    DBGCMD( static const char functionName[] = "OSS_AlarmSet"; )
    u_int32 ticks;
    int     tickRate;
    OSS_INT_ALARM_HANDLE *internAlarmHdl = (OSS_INT_ALARM_HANDLE*) alarm;

    DBGWRT_1((DBH,"%s()\n", functionName));

    /* return error if already active */
    if( internAlarmHdl->active )
    {
        DBGWRT_ERR( ( DBH, "%s%s:  alarm already active %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return(ERR_OSS_ALARM_SET);
    }/*if*/

    /*----------------------+
    |  calculate ticks      |
    +----------------------*/
    /*--- round time, and correct for timer inaccuracy ---*/
    tickRate =  sysClkRateGet();
    if( tickRate == 0 )
    {
        DBGWRT_ERR( ( DBH, "%s%s:  illegal tickRate %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return(ERR_OSS_ALARM_SET);
    }/*if*/
    ticks = (msec * tickRate ) / 1000;

    if( (msec * tickRate ) % 1000 )
        ticks++;

    if (!cyclic)
        ticks++;

    if( ticks == 0 )
        ticks++;

    /*----------------------+
    |  install alarm        |
    +----------------------*/
    internAlarmHdl->cyclic          = cyclic;
    internAlarmHdl->priority        = ticks;
    internAlarmHdl->currentPriority = ticks;
    internAlarmHdl->active          = TRUE;

    /* calc rounded msec */
    *realMsecP = (ticks * 1000) / tickRate;

    return(ERR_SUCCESS);
}/*OSS_AlarmSet*/

/*************************** OSS_AlarmClear *********************************
 *
 *  Description:  De-activate an alarm subroutine
 *                Returns an error if not active.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   oss handle for complicated OS
 *                alarm   alarm handle
 *
 *  Output.....:  return  success (0) or error code
 *
 *  Globals....:  -
 ****************************************************************************/
int32 OSS_AlarmClear
(
    OSS_HANDLE       *osHdl,
    OSS_ALARM_HANDLE *alarm
)
{
    DBGCMD( static const char functionName[] = "OSS_AlarmClear"; )
    OSS_INT_ALARM_HANDLE *internAlarmHdl = (OSS_INT_ALARM_HANDLE*) alarm;

    DBGWRT_1((DBH,"%s\n", functionName));

    /*-----------------------+
    |  check alarm handle    |
    +-----------------------*/
    if( internAlarmHdl == NULL )
    {
        DBGWRT_ERR( ( DBH, "%s%s: illegal alarm handle %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_ILL_PARAM );
    }/*if*/

    /*-----------------------+
    |  check is active       |
    +-----------------------*/
    if( !internAlarmHdl->active )
    {
        DBGWRT_ERR( ( DBH, "%s%s: alarm not active %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return(ERR_OSS_ALARM_CLR);
    }/*if*/

    internAlarmHdl->active = FALSE;

    return(ERR_SUCCESS);
}/*OSS_AlarmClear*/

/***************************** OSS_AlarmScheduler ***************************
 *
 *  Description: Alarm handler scheduler.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: OSS_AlarmList
 ****************************************************************************/
void OSS_AlarmScheduler( void )
{
    DBGCMD( static const char functionName[] = "OSS_AlarmScheduler()"; )
    OSS_INT_ALARM_HANDLE *internAlarmHdl;

    FOREVER
    {
        taskDelay( 1 );
        /* look for installed and active alarms */
        for( internAlarmHdl = (OSS_INT_ALARM_HANDLE*) DLL_FIRST( &OSS_AlarmList );
             internAlarmHdl != NULL;
             internAlarmHdl = (OSS_INT_ALARM_HANDLE*) DLL_NEXT (&internAlarmHdl->node)
           )
        {
           if( internAlarmHdl->active )
           {
               internAlarmHdl->currentPriority--; /*count down*/

               if( internAlarmHdl->currentPriority == 0 )
               {
				   DBGWRT_2((DBH,">>>%s fire\n", functionName));
                   /* jump into installed function */
                   internAlarmHdl->funct( internAlarmHdl->arg );

                   if (!internAlarmHdl->cyclic)
                       internAlarmHdl->active = FALSE; /* mark single-alarm as inactive */
                   else
                       internAlarmHdl->currentPriority = internAlarmHdl->priority;
               }/*if*/
           }/*if*/
        }/*for*/
    }/*FOREVER*/
}/*OSS_AlarmScheduler*/

