/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_lineargs.c
 *      Project: User OSS functions
 *
 *      $Author: dpfeuffer $
 *        $Date: 2011/07/06 15:46:43 $
 *    $Revision: 1.2 $
 *
 *  Description: USER OSS LINEARGS - This module is used for lineargs and main() replacing.
 *               It is a part of the USR_OSS library.
 *
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_lineargs.c,v $
 * Revision 1.2  2011/07/06 15:46:43  dpfeuffer
 * R: disturbing debug warning for 16G215-01 design test
 * M: UOS_NO_REDIRECTION added to supress "*** main replace: ..." warning
 *
 * Revision 1.1  1999/08/31 10:53:45  Franke
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char UOS_LINEARGS_RCSid[]="$Id: usr_oss_lineargs.c,v 1.2 2011/07/06 15:46:43 dpfeuffer Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#include <string.h>

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


/********************************* UOS_OS_LineArgs **************************
 *
 *  Description: Scans a command string. Used by main() replacing.
 *
 *  attention: separators will be replaced with a '\0' character 
 *			   
 *---------------------------------------------------------------------------
 *  Input......: dummyArg	dummy
 *			     lp			input line
 *			     argbuf		ptr array for results (maxarg+1 entries !)
 *			     maxargs	size of argbuf
 *			     separators argument separator chars
 *			     termchars  termination characters 
 *			     
 *  Output.....: return  number of args < -1
 *  Globals....: -
 ****************************************************************************/
int UOS_OS_LineArgs
(
	char *dummyArg,
	char *lp,
	char **argbuf,
	int	 maxargs,
	char *separators,
	char *termchars
)
{
	char 	*p, **u;
	int		end = 0, term = 0;
	int 	argc = 1, flg = 0, quote=0, sep=0;

#ifndef UOS_NO_REDIRECTION
	if( UOS_KeyStdIoSetOfTaskSet() )
	{
		printErr("*** main replace: UOS_OS_LineArgs: UOS_KeyStdIoSetOfTaskSet failed\n*** Please set UOS_KeyStdIoFd!\n");
		return( -1 );
	}/*if*/
#endif

	/* argv[0] is normaly the name of the called program */
	u  = argbuf;
	*u = dummyArg;

	/* no more arguments */
	if( lp == NULL )
		return( argc );

	/* scan input line */
	for(u++, p = lp;  argc <= maxargs; p++)
	{ 	
		term = (int)index(termchars, *p);	/* termination character */
		sep  = (int)index(separators, *p);	/* separator */
		end	 = (*p == '\0');					/* end of string */

		if(*p == '"') 
			quote = !quote;		

		if(end && quote)	/* string end but opened '"' */
			return -1;		/* error */

		if(quote == 0 || flg == 0)	/* not in '"' */
		{
			if(end || term || sep)
			{
				if(flg == 1)
				{
			  		*p = 0;						/* found string end */
					u++;
			  	}	
			  	flg = 0;
			  	if(end || term) break;
			}	
			else
			{
			  	if(flg == 0) 
			  	{
					*u = p;						/* save string begin */
					argc++;
				}
			  	flg = 1;
			}
		}				
	}/*for*/
	
	*u = NULL;								/* flag end of args */
	return argc;
}/*UOS_OS_LineArgs*/

