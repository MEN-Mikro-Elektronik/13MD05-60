/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: usr_oss_key.c
 *      Project: User OSS lib
 *
 *       Author: see
 *        $Date: 2009/03/31 10:52:37 $
 *    $Revision: 1.9 $
 *
 *  Description: USER OSS KEY - This module checks for user interaction.
 *               It is a part of the USR_OSS library.
 *               It provides a blocking and a non blocking function.
 *                      
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_key.c,v $
 * Revision 1.9  2009/03/31 10:52:37  ufranke
 * cosmetics
 *
 * Revision 1.8  2005/12/23 12:56:32  UFRANKE
 * changed
 *  - VxWorks 6.x shell name is now tShellX with x=0,1...
 *
 * Revision 1.7  2004/05/10 11:57:02  UFranke
 * changed
 *  - if task and shell uses the same stdio warning only UOS_KeyStdIoCheck()
 *
 * Revision 1.6  2001/12/03 16:33:53  Franke
 * added comment that UOS_KeyPressed() sometimes not work
 *
 * Revision 1.5  2000/03/24 12:13:14  kp
 * Stdio: Printout a warning if UOS_KeyStdIoFd is not -1 and running in tshell
 *
 * Revision 1.4  2000/03/16 16:08:06  kp
 * cosmetics
 *
 * Revision 1.3  2000/01/24 15:50:08  kp
 * Changed UOS_StdioKeyCheck to allow to execute on the same I/O paths
 * as the shell when started from shell
 *
 * Revision 1.2  1999/12/17 13:41:22  kp
 * Bug fix: UOS_Keypressed must return -1 if no char there
 *
 * Revision 1.1  1999/08/31 10:53:47  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
const char UOS_KEY_RCSid[]="$Id: usr_oss_key.c,v 1.9 2009/03/31 10:52:37 ufranke Exp $";

#include <MEN/men_typs.h>

#include <ioLib.h>
#include <ctype.h>
#include <string.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define UOS_KEY_OWN_TASK      0
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
int UOS_KeyStdIoFd = UOS_KEY_STD_IO_DEFAULT;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/********************************* UOS_KeyStdIoCheck ************************
 *
 *  Description: Checks that task stdin differs from shell stdin if program
 *				 spawned as a seperate task
 * 
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return 0 | error code
 *  Globals....: UOS_KeyStdIoFd
 ****************************************************************************/
int32 UOS_KeyStdIoCheck( void ) /*nodoc*/
{
	char *tName;
	int  taskIs_tShell = 0;
	int  taskStdIn;
	int  shellStdIn;
	int  shellStdIn0;
	int  shellStdIn1;
	int  shellStdIn2;

	tName = taskName( taskIdSelf() );
	if( tName == NULL || !strncmp( "tShell", tName, 6 )  )
		taskIs_tShell = 1;
	/*
	 * When spawned as a different task (not in context of tShell),
	 * then the stdio of tShell and UOS program should differ
	 */
	if( !taskIs_tShell )
	{
		taskStdIn  = ioTaskStdGet( UOS_KEY_OWN_TASK, STD_IN);
		shellStdIn = ioTaskStdGet( taskNameToId( "tShell" ), STD_IN );
		shellStdIn0 = ioTaskStdGet( taskNameToId( "tShell0" ), STD_IN );
		shellStdIn1 = ioTaskStdGet( taskNameToId( "tShell1" ), STD_IN );
		shellStdIn2 = ioTaskStdGet( taskNameToId( "tShell2" ), STD_IN );
		if( taskStdIn == shellStdIn	
			|| taskStdIn == shellStdIn0	
			|| taskStdIn == shellStdIn1	
			|| taskStdIn == shellStdIn2	
		)
		{
			printErr("*** UOS:WARNING: UOS_KeyStdIoCheck: Stdio of %s and tShell "
					 "should differ\n",
					 taskName( taskIdSelf() ) );
		}/*if*/
	}
	return( 0 );
}/*UOS_KeyStdIoCheck*/


/********************************* UOS_KeyStdIoSetOfTaskSet *****************
 *
 *  Description: If called in context of tShell and UOS_KeyStdIoFd==-1,
 *				 do nothing.
 *
 *				 Otherwise, set the task stdio to <UOS_KeyStdIoFd>.
 *               <UOS_KeyStdIoFd> must be valid file descriptor and should
 *               differ from shell input file descriptor if spawned in a
 *				 seperate task
 * 
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return 0 | error code
 *  Globals....: UOS_KeyStdIoFd
 ****************************************************************************/
int32 UOS_KeyStdIoSetOfTaskSet( void )
{
	char *tName;
	int  taskIs_tShell = 0;

	tName = taskName( taskIdSelf() );
	if( tName == NULL || !strncmp( "tShell", tName, 6 )  )
		taskIs_tShell = 1;
	
	
	if( (UOS_KeyStdIoFd == UOS_KEY_STD_IO_DEFAULT) && taskIs_tShell )
		return 0;
		

	if( UOS_KeyStdIoFd == UOS_KEY_STD_IO_DEFAULT )
	{
		printErr("*** UOS:WARNING: UOS_KeyStdIoSetOfTaskSet: "
				 "Set UOS_KeyStdIoFd to a valid file descriptor "
				 "when not running in tShell!\n" );
		return( 0 );
	}
	else 
	{
		if( taskIs_tShell )
		{
			printErr("*** UOS: "
					 "WARNING: Target shell I/O redirected to fd #%d\n",
					 UOS_KeyStdIoFd);
		}
	}
	ioTaskStdSet( UOS_KEY_OWN_TASK, STD_IN , UOS_KeyStdIoFd );
	ioTaskStdSet( UOS_KEY_OWN_TASK, STD_OUT, UOS_KeyStdIoFd );
	ioTaskStdSet( UOS_KEY_OWN_TASK, STD_ERR, UOS_KeyStdIoFd );

	if( UOS_KeyStdIoCheck() )
	{
		printErr("*** UOS: UOS_KeyStdIoSetOfTaskSet: UOS_KeyStdIoCheck() failes\n");
		return( -1 );
	}/*if*/

	
	return( 0 );
}/*UOS_KeyStdIoSet*/


/********************************* UOS_KeyPressed ***************************
 *
 *  Description: Check if any key pressed
 *               
 *			     This function checks if any character is buffered
 *               in the stdin path. If so, the char this read and returned
 *               as integer value. If not, -1 is returned.
 *
 * 				 Task id of the calling task must differ from the tShell task
 *               id and stdin of the calling task must differ from stdin of
 *               tShell.
 *			   
 *		   Note: non blocking
 *			     The function does'nt work like expected often. Depends on the
 *			     moment of pressing key, this event may be detected or lost.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return		error (-1) or key code
 *                          -1      no key pressed
 *                          0..255	char code of pressed key
 *  Globals....: -
 ****************************************************************************/
int32 UOS_KeyPressed( void )
{
int32 c;
int  nBytesUnread=0;
int  nBytesUnwrite;
int  status;
int  taskId;	
int  outFd;	
int  inFd;	

	c=-1;

	if( UOS_KeyStdIoCheck() )
	{
		return((int32)c);		/* return char */
	}/*if*/

	/* get task and shell parameters */
	taskId		= taskIdSelf();
	inFd		= ioTaskStdGet( taskId, 0 ); /*stdin*/

	/*delay that all output is written
	  before switched to RAW mode */
	outFd = ioTaskStdGet( taskIdSelf(), 1 ); /*stdout*/
	do
	{
		taskDelay(1);
		status = ioctl( outFd, FIONWRITE, (int)&nBytesUnwrite);
		status = ioctl( inFd, FIONREAD, (int)&nBytesUnread);
		if( nBytesUnread )
		{
	    	c=getchar();
	    	return( c );
		}/*if*/
	}while( nBytesUnwrite );
	


	/* switch to raw mode */
	status = ioctl( inFd, FIOSETOPTIONS, OPT_RAW );

	/* check nbr of bytes and get */
	status = ioctl( inFd, FIONREAD, (int)&nBytesUnread);
	if( nBytesUnread )
	{
	    c=getchar();
	}
	else
	{
		taskDelay(1);
		status = ioctl( inFd, FIONREAD, (int)&nBytesUnread);
		if( nBytesUnread )
		{
	    	c=getchar();
		}
	}/*if*/
	
	/* restore terminal mode */
	status = ioctl( inFd, FIOSETOPTIONS, OPT_TERMINAL );

	return((int32)c);		/* return char */
}/*UOS_KeyPressed*/

/********************************* UOS_KeyWait ***************************
 *
 *  Description: Wait until any key pressed
 *               
 *			     This function waits until any character could be read
 *               from the stdin path.
 *			   
 * 				 Task id of the calling task must differ from the tShell task
 *               id and stdin of the calling task must differ from stdin of
 *               tShell.
 *			   
 *		   Note: blocking
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return		error (-1) or key code
 *                          -1      read error
 *                          0..255	char code of pressed key
 *  Globals....: -
 ****************************************************************************/
int32 UOS_KeyWait( void )
{
int32 c;
int  nBytesUnread;
int  status;
int  taskId;	
int  outFd;	
int  inFd;	

	c = -1;
	if( UOS_KeyStdIoCheck() )
	{
		return((int32)c);		/* return char */
	}/*if*/

	/*delay that all output is written
	  before switched to RAW mode */
	outFd = ioTaskStdGet( taskIdSelf(), 1 ); /*stdout*/
	do
	{
		taskDelay(1);
		status = ioctl( outFd, FIONWRITE, (int)&nBytesUnread);
	}while( nBytesUnread );
	
	/* get task and shell parameters */
	taskId		= taskIdSelf();
	inFd		= ioTaskStdGet( taskId, 0 ); /*stdin*/

	/* switch to raw mode */
	status = ioctl( inFd, FIOSETOPTIONS, OPT_RAW );

	/* clear the input buffer */
	fflush( stdin );
	do
	{
		taskDelay(1);
		status = ioctl( inFd, FIONREAD, (int)&nBytesUnread);
		if( nBytesUnread )
		    c=getchar();
	}while( nBytesUnread );

	c=' ';
    c=getchar();

	fflush( stdin );
	status = ioctl( inFd, FIONREAD, (int)&nBytesUnread);
	status = ioctl( inFd, FIOSETOPTIONS, OPT_TERMINAL );
	if( isalpha( c ) )
		printf( "%c \n", (int)c );

	return((int32)c);		/* return char */
}/*UOS_KeyWait*/


