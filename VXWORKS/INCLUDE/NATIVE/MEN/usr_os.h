/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: usr_os.h
 *
 *      $Author: ufranke $
 *        $Date: 2008/05/09 13:59:07 $
 *    $Revision: 1.11 $
 *
 *  Description: User mode system services
 *
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_os.h,v $
 * Revision 1.11  2008/05/09 13:59:07  ufranke
 * fixed
 *  - VxWorks 6.6 compiler error
 *    invalid storage class for 'internFunc'
 *    for main() replacement
 *
 * Revision 1.10  2006/07/25 14:35:55  ufranke
 * fixed
 *  - #define main PROG_FILE_NAME
 *
 * Revision 1.9  2006/06/08 14:09:25  ufranke
 * added
 *  + UOS_VxShowRevision()
 *
 * Revision 1.8  2006/06/07 16:57:25  ufranke
 * cosmetics
 *
 * Revision 1.7  2006/06/02 09:55:21  ufranke
 * changed
 *  + UOS signal handling
 *    UOS_SIG_USR3,4 are available now for VxWorks
 *
 * Revision 1.6  2005/12/23 15:19:37  UFRANKE
 * added
 *  - initial VxWorks RTP support
 *
 * Revision 1.5  2005/11/25 16:45:49  ts
 * Newline added at file end to avoid compiler warning
 *
 * Revision 1.4  2001/12/21 14:07:13  Franke
 * changed main replacing line argument separator
 *         now only tab and space, ',' removed
 *
 * Revision 1.3  1999/08/31 10:53:26  Franke
 * added CALLBACK, SHAREDMEM, LINEARGS
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _USR_OS_H_
#define _USR_OS_H_

/* some predefined signal codes */
#define UOS_SIG_USR1			1
#define UOS_SIG_USR2			2
#define UOS_SIG_USR3			3
#define UOS_SIG_USR4			4
#define UOS_SIG_USR_MAX_VXWORKS	0xFF
#define ERR_UOS      			0x2000

#ifndef __RTP__

/* kernel mode UOS */
#include <MEN/oss.h>
#include <sigLib.h>     /* signals                   */

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
#ifdef COMPILE_UOS_CALLBACK
 typedef struct UOS_CALLBACK_HANDLE UOS_CALLBACK_HANDLE;
#else
 typedef void UOS_CALLBACK_HANDLE;
#endif

/*-------------------+
| shared mem handle  |
+-------------------*/
typedef struct {
    u_int32	path;			/* device path number */
} UOS_SHMEM_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

#define UOS_OS_MAX_ARGS				30
#define UOS_OS_MAX_CMD_LINE			0x200

/* macro for main redefinition - all in one line for source code debugging */
#ifdef PROG_FILE_NAME
 	static int internFunc( int argc, char **argv );
	#define main 												\
	PROG_FILE_NAME( char *cmdLine ) 							\
	{															\
		char cmdLineCpy[UOS_OS_MAX_CMD_LINE];					\
		char *c_argv[UOS_OS_MAX_ARGS+1];						\
	 	int  c_argc;											\
	 	int  len = 0;											\
	 	if( cmdLine != NULL )									\
	 	{	len = strlen( cmdLine );	}						\
	 	if( len >= UOS_OS_MAX_CMD_LINE )						\
	 	{   printf("*** usr_os.h: command line to long\n"); }	\
	 	memset( cmdLineCpy, 0, UOS_OS_MAX_CMD_LINE );			\
	 	memcpy( cmdLineCpy, cmdLine, len );						\
	 	c_argc = UOS_OS_LineArgs( "uf", cmdLineCpy, c_argv, UOS_OS_MAX_ARGS, " \t", "\n" );	\
	 	if(c_argc==-1)											\
	 		return(-1);											\
		return( internFunc( c_argc, c_argv ) );					\
	}															\
 static int internFunc

#endif

#define UOS_KEY_STD_IO_DEFAULT      -1

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern int UOS_KeyStdIoFd; /* STD_IN, _OUT, _ERR of user programms, must differ from shell STD_IN */
extern const char UOS_SIGNAL_RCSid[];

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern int UOS_OS_LineArgs
(
	char *dummyArg,		/* dummy arg, normally the name of the called program */
	char *lp,			/* input line */
	char **argbuf,		/* ptr array for results (maxarg+1 entries !)*/
	int	 maxargs,		/* size of argbuf */
	char *separators,	/* argument separator chars */
	char *termchars  	/* termination characters */
);

extern int32 UOS_KeyStdIoCheck( void );
extern int32 UOS_KeyStdIoSetOfTaskSet( void );

#else
/* user mode UOS */
typedef struct UOS_CALLBACK_HANDLE UOS_CALLBACK_HANDLE;

typedef struct {
    u_int32	path;			/* device path number */
} UOS_SHMEM_HANDLE;

#endif /*__RTP__*/

extern void UOS_VxShowRevision( void );

#ifndef __RTP__
#ifdef __cplusplus
      }
#endif
#endif /*__RTP__*/

#endif /*_USR_OS_H_*/




