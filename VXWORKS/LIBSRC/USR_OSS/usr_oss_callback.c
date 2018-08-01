/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_callback.c
 *      Project: User OSS lib
 *
 *       Author: uf
 *        $Date: 2010/10/27 10:22:20 $
 *    $Revision: 1.5 $
 *
 *  Description: USER OSS CALLBACK - Routines to manage callbacks.
 *
 *               Callback routines are user state functions which are triggered
 *               from the driver interrupt routine.
 *
 *               A callback routine can be installed from the application.
 *				 When a callback routine is installed
 *               an application argument for the routine can be defined.
 *
 *               The callback routine gots as input parameters the installed
 *               application argument and a driver specific argument:
 *
 *                  void funct(void *appArg, void *drvArg)
 *
 *               Callback routines are identified via a number, the so-called
 *               "function index" (callNr).
 *
 *               The INIT function creates a global callback handle for all
 *               subsequent calls.
 *
 *               Typical usage:
 *
 *                    Installation:
 *                    	UOS_CallbackInit()
 *                    	UOS_CallbackSet()
 *                    De-Installation:
 *                    	UOS_CallbackClear()
 *                    	UOS_CallbackExit()
 *
 *               Callback routines can be masked by using the
 *               UOS_CallbackMask()/UOS_CallbackUnMask() functions.
 *
 *               The functions correspond with the OSS_CallbackXXX functions
 *               of the OSS driver library.
 *
 *               VxWorks specific:
 *               -----------------
 *
 *     Required: -
 *     Switches: -
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_callback.c,v $
 * Revision 1.5  2010/10/27 10:22:20  cs
 * R: adapt prototype of UOS_CallbackInit to new definition in usr_oss.h
 * M: use INT32_OR_64 for MDIS path
 *
 * Revision 1.4  2010/09/02 14:30:14  UFranke
 * R: DIAB linker error SDA VxWorks 6.7 PPC due to access to global variable
 * M: UOS_CB_RCSid is static now
 *
 * Revision 1.3  2006/06/08 14:18:33  ufranke
 * cosmetics
 *
 * Revision 1.2  2005/06/29 15:08:07  UFranke
 * cosmetics
 *
 * Revision 1.1  1999/08/31 10:53:37  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char UOS_CB_RCSid[]="$Id: usr_oss_callback.c,v 1.5 2010/10/27 10:22:20 cs Exp $";

#include <MEN/men_typs.h>

#include <MEN/mdis_api.h>

#define COMPILE_UOS_CALLBACK

#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef struct UOS_CALLBACK_HANDLE
{
	u_int32 	ownMemSize;     /* allocated memory */
    u_int32		dbgHdl;			/* debug handle */
    int32		path;			/* device path number */
	u_int32 	mode;			/* callback mode */
	u_int32 	maskCnt;		/* callback mask counter - indirect mode */
}_UOS_CALLBACK_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
u_int32 UOS_CbTaskId        = 0x00000000;
u_int32 UOS_CbTaskPriority  = 1;
u_int32 UOS_CbTaskStackSize = 0x1000;
u_int32 UOS_CbInstCounter   = 0;

int	     UOS_CbMsgQSize	    = 100;   /* size of callback entries */
int	     UOS_CbMsgMaskCnt	= 0;     /* global mask counter - indirect mode  */


/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
void UOS_CbMsgReceiver( void );

char* UOS_CallbackIdent( void )
{
	return( (char*)UOS_CB_RCSid );
}

/********************************* UOS_CallbackInit *************************
 *
 *  Description: Create callback handle
 *
 *	             This function has to be called before any callback function
 *               can be installed.
 *
 *               Possible errors:
 *               ERR_UOS_MEM_ALLOC       no free memory to create handle
 *               ERR_UOS_GETSTAT         the above status call failed
 *
 *               If error ERR_UOS_GETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: path        path number
 *  Output.....: cbHdlP		 pointer to callback handle | NULL
 *               return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 UOS_CallbackInit
(
	INT32_OR_64 path,
	UOS_CALLBACK_HANDLE **cbHdlP
)
{
    DBGCMD( static char functionName[] = "UOS_CallbackInit"; )
    u_int32             gotsize;
	UOS_CALLBACK_HANDLE  *cbHdl;
	OSS_CALLBACK_GETSTAT cbGetStat;
   	M_SG_BLOCK           blk;
	int32                error;

	/* init UOS debug handle */
    DBGCMD( if( !UOS_DbgHdl ) DBG_Init( NULL, &UOS_DbgHdl ); );
    DBGWRT_1((DBH,"%s()\n", functionName));

	*cbHdlP = NULL;

	/* alloc+clear mem for handle */
    cbHdl   = (UOS_CALLBACK_HANDLE*) OSS_MemGet( NULL, sizeof(UOS_CALLBACK_HANDLE),
                                                 &gotsize );
    if( cbHdl == NULL )
    {
       return( ERR_UOS_MEM_ALLOC );
    }/*if*/

    /* fill turkey with 0 */
    OSS_MemFill( NULL, gotsize, (char*) cbHdl, 0 );

	/* init handle */
	cbHdl->ownMemSize = gotsize;
	cbHdl->path       = path;
	cbHdl->maskCnt    = 0;

    /* clear struct to avoid unpredictable behaviour */
    OSS_MemFill( NULL, sizeof(cbGetStat), (char*) &cbGetStat, 0 );

	/* query callback mode */
	blk.size = sizeof(cbGetStat);
	blk.data = (void*)&cbGetStat;
	cbGetStat.action = OSS_CB_MODE;
	if( M_getstat(path, M_LL_BLK_CALLBACK, (int32*)&blk) < 0 )
	{
        DBGWRT_ERR( ( DBH, "%s%s: query callback mode %s%d%s\n",
                      UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
        DBGWRT_ERR( ( DBH, "*** with error %s\n", M_errstring(UOS_ErrnoGet()) ));
		error = ERR_UOS_GETSTAT;
		goto CLEANUP;
	}
	cbHdl->mode = cbGetStat.p.mode;

	if( cbHdl->mode == OSS_CALLBACK_MODE_INDIRECT )
	{
		/*----------------------+
		|  create the msg queue |
		+----------------------*/
    	if( UOS_CbInstCounter == 0 )
    	{
		    DBGWRT_2((DBH," msgQCreate OSS_CbMsgQid\n"));
    		OSS_CbMsgQid = msgQCreate( UOS_CbMsgQSize, sizeof(OSS_CB_QUEUE_ENTRY), MSG_Q_PRIORITY );
    	    if( OSS_CbMsgQid == NULL )
	        {
            	error = errnoGet();
        	    DBGWRT_ERR( ( DBH, "%s%s: msgQCreate(() failed with errno 0x%08x %s%d%s\n",
    	                      UOS_ErrorStartStr, functionName,
    	                      error,
    	                      UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ) );
	            error = ERR_UOS;
            	goto CLEANUP;
        	}/*if*/
		    DBGWRT_2((DBH,"  OSS_CbMsgQid = 0x%08x\n", OSS_CbMsgQid));
    	}/*if*/

	    /*----------------------------------+
    	|  spawn the callback receiver task |
	    +----------------------------------*/
    	if( UOS_CbTaskId == 0 && UOS_CbInstCounter == 0 )
    	{
		    DBGWRT_2((DBH," spawn tUosCbTask\n"));
       		UOS_CbTaskId = taskSpawn( "tUosCbTask", UOS_CbTaskPriority,
                                     0, UOS_CbTaskStackSize,
                                     (FUNCPTR) UOS_CbMsgReceiver, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    	    if( UOS_CbTaskId == ERROR )
	        {
            	error = errnoGet();
        	    DBGWRT_ERR( ( DBH, "%s%s: taskSpawn() failed with errno 0x%08x %s%d%s\n",
    	                      UOS_ErrorStartStr, functionName,
    	                      error,
    	                      UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ) );
	            error = ERR_UOS;
            	goto CLEANUP;
        	}/*if*/
		    DBGWRT_2((DBH,"  UOS_CbTaskId = 0x%08x\n", UOS_CbTaskId));
    	}/*if*/

	    /* increment the link counter */
	    UOS_CbInstCounter++;
    }/*if*/



	*cbHdlP = cbHdl;
	return(ERR_UOS_SUCCESS);


CLEANUP:
	if( UOS_CbInstCounter == 0 )
	{
		/* free mem */
		if( UOS_CbTaskId )
			taskDelete( UOS_CbTaskId );
		if( OSS_CbMsgQid )
			msgQDelete( OSS_CbMsgQid );
	}/*if*/
    OSS_MemFree( NULL, (int8*) cbHdl, cbHdl->ownMemSize );
    cbHdl = NULL;

	return( error );
}/*UOS_CallbackInit*/

/********************************* UOS_CallbackExit *************************
 *
 *  Description: Remove callback handle
 *
 *---------------------------------------------------------------------------
 *  Input......: cbHdlP		 pointer to callback handle
 *  Output.....: return		 success (0) or error code
 *               *cbHdlP     NULL
 *  Globals....: -
 ****************************************************************************/
int32 UOS_CallbackExit
(
	UOS_CALLBACK_HANDLE **cbHdlP
)
{
    DBGCMD( static char functionName[] = "UOS_CallbackExit"; )
	UOS_CALLBACK_HANDLE *cbHdl = *cbHdlP;
    int32 error;

    DBGWRT_1((DBH,"%s()\n", functionName));

	if( !cbHdl )
	{
        DBGWRT_ERR( ( DBH, "%s%s: cbHdl null pointer %s%d%s\n",
                      UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
        return( ERR_UOS_ILL_PARAM );
	}/*if*/

	if( cbHdl->mode == OSS_CALLBACK_MODE_INDIRECT )
	{
	    /*-----------------------------------+
    	|  delete the callback receiver task |
	    +-----------------------------------*/
    	UOS_CbInstCounter--;
	    if( UOS_CbTaskId != 0 && UOS_CbInstCounter == 0 )
    	{
		    DBGWRT_2((DBH," delete tUosCbTask\n"));
        	error = taskDelete( UOS_CbTaskId );
	        if( error == ERROR )
    	    {
        	    error = errnoGet();
            	DBGWRT_ERR( ( DBH, "%s%s: taskDelete() failed with errno 0x%08x %s%d%s\n",
                	          UOS_ErrorStartStr, functionName,
                    	      error,
                        	  UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ) );
	            return( ERR_UOS );
    	    }/*if*/
        	UOS_CbTaskId = 0;
	    }/*if*/

	    /*---------------------------+
    	|  delete the callback queue |
	    +---------------------------*/
    	if( UOS_CbInstCounter == 0 )
   		{
		    DBGWRT_2((DBH," msgQDelete OSS_CbMsgQid\n"));
			error = msgQDelete( OSS_CbMsgQid );
	        if( error == ERROR )
   		    {
       		    error = errnoGet();
           		DBGWRT_ERR( ( DBH, "%s%s: msgQDelete() failed with errno 0x%08x %s%d%s\n",
               		          UOS_ErrorStartStr, functionName,
                   		      error,
                       		  UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ) );
	            return( ERR_UOS );
	   	    }/*if*/
	   	    OSS_CbMsgQid = NULL;
	   	}
	   	else
	   	{
	   		/* let's other callbacks work, if masked by this handle */
	   		while( cbHdl->maskCnt )
	   			UOS_CallbackUnMask( cbHdl );
   	    }/*if*/
    }/*if*/


	/* free mem */
    OSS_MemFree( NULL, (int8*) cbHdl, cbHdl->ownMemSize );
    cbHdl = NULL;

	return(ERR_UOS_SUCCESS);
}/*UOS_CallbackExit*/

/********************************* UOS_CallbackSet **************************
 *
 *  Description: Install callback function
 *
 *               The function installs the specified callback function
 *               via M_LL_BLK_CALLBACK setstat at the driver.
 *
 *               Possible errors:
 *               ERR_UOS_SETSTAT         the above status call failed
 *
 *               If error ERR_UOS_SETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: cbHdl		 callback handle
 *               callNr      callback function index (0..maxCallback-1)
 *               funct       callback function pointer
 *               appArg      callback function argument
 *  Output.....: return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 UOS_CallbackSet
(
	UOS_CALLBACK_HANDLE *cbHdl,
    u_int32    callNr,
    void       (*funct)(),
    void       *appArg
)
{
    DBGCMD( static char functionName[] = "UOS_CallbackSet"; )
	OSS_CALLBACK_SETSTAT cbSetStat;
   	M_SG_BLOCK blk;

    DBGWRT_1((DBH,"%s() install callNr=0x%x\n", functionName, callNr));

    /* clear struct to avoid unpredictable behaviour */
    OSS_MemFill( NULL, sizeof(cbSetStat), (char*) &cbSetStat, 0 );

	/* fill structure */
	cbSetStat.action = OSS_CB_INSTALL;
	cbSetStat.p.install.callNr = callNr;
	cbSetStat.p.install.funct  = funct;
	cbSetStat.p.install.appArg = appArg;
	cbSetStat.p.install.mode   = cbHdl->mode;

	/* install callback */
	blk.size = sizeof(OSS_CALLBACK_SETSTAT);
	blk.data = (void*)&cbSetStat;

	if (M_setstat(cbHdl->path, M_LL_BLK_CALLBACK, (int32)&blk) < 0)
	{
        DBGWRT_ERR( ( DBH, "%s%s: can't set OSS_CB_INSTALL %s%d%s\n",
                      UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
        DBGWRT_ERR( ( DBH, "*** with error %s\n", M_errstring(UOS_ErrnoGet()) ));
		return(ERR_UOS_SETSTAT);
	}/*if*/

	return(ERR_UOS_SUCCESS);
}/*UOS_CallbackSet*/

/********************************* UOS_CallbackClear **************************
 *
 *  Description: De-Install callback function
 *
 *               The function de-installs the specified callback function
 *               via M_LL_BLK_CALLBACK setstat at the driver.
 *
 *               Possible errors:
 *               ERR_UOS_SETSTAT         the above status call failed
 *
 *               If error ERR_UOS_SETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: cbHdl		 callback handle
 *               callNr      callback function index (0..maxCallback-1)
 *  Output.....: return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 UOS_CallbackClear
(
	UOS_CALLBACK_HANDLE *cbHdl,
    u_int32    callNr
)
{
    DBGCMD( static char functionName[] = "UOS_CallbackClear"; )
	OSS_CALLBACK_SETSTAT cbSetStat;
   	M_SG_BLOCK blk;

    DBGWRT_1((DBH,"%s() deinstall callNr=0x%x\n", functionName, callNr));

    /* clear struct to avoid unpredictable behaviour */
    OSS_MemFill( NULL, sizeof(cbSetStat), (char*) &cbSetStat, 0 );

	/* fill structure */
	cbSetStat.action = OSS_CB_DEINSTALL;
	cbSetStat.p.deinstall.callNr = callNr;

	/* deinstall callback */
	blk.size = sizeof(OSS_CALLBACK_SETSTAT);
	blk.data = (void*)&cbSetStat;

	if (M_setstat(cbHdl->path, M_LL_BLK_CALLBACK, (int32)&blk) < 0)
	{
        DBGWRT_ERR( ( DBH, "%s%s: can't set OSS_CB_DEINSTALL %s%d%s\n",
                      UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
        DBGWRT_ERR( ( DBH, "*** with error %s\n", M_errstring(UOS_ErrnoGet()) ));
		return(ERR_UOS_SETSTAT);
	}/*if*/

	return(ERR_UOS_SUCCESS);
}/*UOS_CallbackClear*/

/********************************* UOS_CallbackMask *************************
 *
 *  Description: Prevent application from beeing interrupted by callbacks
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: cbHdl		 callback handle
 *  Output.....: return		 success (0) or error code
 *  Globals....: UOS_CbMsgMaskCnt
 ****************************************************************************/
int32 UOS_CallbackMask( UOS_CALLBACK_HANDLE *cbHdl )
{
    DBGCMD( static char functionName[] = "UOS_CallbackMask"; )

    DBGWRT_1((DBH,"%s()\n", functionName));

	if( cbHdl->mode == OSS_CALLBACK_MODE_DIRECT )
	{
		OSS_CALLBACK_SETSTAT cbSetStat;
   		M_SG_BLOCK blk;

	    /* clear struct to avoid unpredictable behaviour */
    	OSS_MemFill( NULL, sizeof(cbSetStat), (char*) &cbSetStat, 0 );

		/* fill structure */
		cbSetStat.action = OSS_CB_MASK;

		/* install callback */
		blk.size = sizeof(OSS_CALLBACK_SETSTAT);
		blk.data = (void*)&cbSetStat;

		if (M_setstat(cbHdl->path, M_LL_BLK_CALLBACK, (int32)&blk) < 0)
		{
        	DBGWRT_ERR( ( DBH, "%s%s: can't set OSS_CB_MASK %s%d%s\n",
            	          UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
	        DBGWRT_ERR( ( DBH, "*** with error %s\n", M_errstring(UOS_ErrnoGet()) ));
			return(ERR_UOS_SETSTAT);
		}/*if*/
	}
	else
	{
		if( UOS_CbMsgMaskCnt == 0 )
		{
			/* suspend task */
			DBGWRT_2((DBH," suspend tUosCbTask\n"));
			taskSuspend( UOS_CbTaskId );
		}/*if*/
		cbHdl->maskCnt++;
		UOS_CbMsgMaskCnt++;
	}/*if*/

	return(ERR_UOS_SUCCESS);
}/*UOS_CallbackMask*/

/********************************* UOS_CallbackUnMask ************************
 *
 *  Description: Allow callbacks to occurr
 *
 *	Executes any pending callback that has been arrived while callbacks
 *  were masked
 *---------------------------------------------------------------------------
 *  Input......: cbHdl		 callback handle
 *  Output.....: return		 success (0) or error code
 *  Globals....: UOS_CbMsgMaskCnt
 ****************************************************************************/
int32 UOS_CallbackUnMask( UOS_CALLBACK_HANDLE *cbHdl )
{
    DBGCMD( static char functionName[] = "UOS_CallbackUnMask"; )

    DBGWRT_1((DBH,"%s()\n", functionName));

	if( cbHdl->mode == OSS_CALLBACK_MODE_DIRECT )
	{
		OSS_CALLBACK_SETSTAT cbSetStat;
   		M_SG_BLOCK blk;

	    /* clear struct to avoid unpredictable behaviour */
    	OSS_MemFill( NULL, sizeof(cbSetStat), (char*) &cbSetStat, 0 );

		/* fill structure */
		cbSetStat.action = OSS_CB_UNMASK;

		/* install callback */
		blk.size = sizeof(OSS_CALLBACK_SETSTAT);
		blk.data = (void*)&cbSetStat;

		if (M_setstat(cbHdl->path, M_LL_BLK_CALLBACK, (int32)&blk) < 0)
		{
	        DBGWRT_ERR( ( DBH, "%s%s: can't set OSS_CB_UNMASK %s%d%s\n",
            	          UOS_ErrorStartStr, functionName, UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
        	DBGWRT_ERR( ( DBH, "*** with error %s\n", M_errstring(UOS_ErrnoGet()) ));
			return(ERR_UOS_SETSTAT);
		}/*if*/
	}
	else
	{
		cbHdl->maskCnt--;
		UOS_CbMsgMaskCnt--;
		if( UOS_CbMsgMaskCnt == 0 )
		{
			/* resume task */
			DBGWRT_2((DBH," resume tUosCbTask\n"));
			taskResume( UOS_CbTaskId );
		}/*if*/
	}/*if*/

	return(ERR_UOS_SUCCESS);
}/*UOS_CallbackUnMask*/



/***************************** UOS_CbMsgReceiver ***************************
 *
 *  Description: Callback message receiver task.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: UOS_CbList
 ****************************************************************************/
void UOS_CbMsgReceiver( void )
{
    int error;
	OSS_CB_QUEUE_ENTRY cbQentry;
    DBGCMD( static const char functionName[] = "UOS_CbMsgReceiver()"; )

    DBGWRT_1((DBH,"%s() task spawned\n", functionName));
	/*init msg buffer*/
	cbQentry.funct = NULL;
	cbQentry.arg0  = NULL;
	cbQentry.arg1  = NULL;

    FOREVER
    {
		error = msgQReceive(OSS_CbMsgQid, (char*) &cbQentry, sizeof(cbQentry), WAIT_FOREVER);
    	if( error == sizeof(cbQentry) )
    	{
			DBGWRT_2((DBH,">>>%s fire\n", functionName));
			cbQentry.funct( cbQentry.arg0, cbQentry.arg1 );
		}
		else
		{
			error = errnoGet();
			if( error == ERROR )
			{
		        DBGWRT_ERR( ( DBH, "%s%s: msgQReceive with error 0x%08x %s%d%s\n",
    	        	          UOS_ErrorStartStr, functionName,
    	        	          error,
    	        	          UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
			}
			else
			{
		        DBGWRT_ERR( ( DBH, "%s%s: msgQReceive wrong msg size %s%d%s\n",
    	        	          UOS_ErrorStartStr, functionName,
    	        	          UOS_ErrorLineStr, __LINE__, UOS_ErrorEndStr ));
			}/*if*/
       		taskDelay(1);
        }/*if*/
    }/*FOREVER*/
}/*UOS_CbMsgReceiver*/

