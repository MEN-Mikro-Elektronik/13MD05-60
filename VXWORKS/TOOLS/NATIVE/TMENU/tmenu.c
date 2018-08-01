/****************************************************************************
 ************                                                    ************
 ************                      T M E N U                     ************
 ************                                                    ************
 ****************************************************************************
 *
 *      $Author: kp $
 *        $Date: 2000/03/15 13:33:49 $
 *    $Revision: 1.3 $
 *
 *  Description: Test Menu Interpreter for VxWorks
 *
 *
 *     Required: usr_oss.a and symLib
 *     Switches:
 *
 *		   Note: tmenu is not reentrant because windShell is'nt it too
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: tmenu.c,v $
 * Revision 1.3  2000/03/15 13:33:49  kp
 * Added "L" flag (loop mode)
 *
 * Revision 1.2  2000/01/13 13:09:25  kp
 * 1) added cls() and taste()
 * 2) do resolveVariables before checking for implicit command
 *
 * Revision 1.1  1999/08/31 11:44:40  Franke
 * Initial Revision
 *
 * cloned from DOS tmenu.c Revision 1.2  1997/12/10 11:28:03  uf
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..99 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static char RCSid[]="TMENU for VxWorks $Id: tmenu.c,v 1.3 2000/03/15 13:33:49 kp Exp $\n";

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <symLib.h>
#include <taskLib.h>
#include <ioLib.h>
#include <usrLib.h>
#include <pipeDrv.h>
#include <shellLib.h>
#include <sysLib.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define MENUTEXTLEN         50
#define SELECTIONTEXTLEN    3
#define FLAGSLEN            10
#define MAXLINELEN          200
#define MAX_VAR_NAME        32
#define MAX_FILE_NAME		130

static const char pipeName[] = "/pipeTmenuToShell";
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
int tmenuDbg = 0;
int tmenuUnderScoreMode = -1;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static char menutext[MENUTEXTLEN+1];
static char flags[FLAGSLEN+1];
static FILE *logfp;
static int AUTOEXEC,PRINTITEMS;
static char functName[0x100];
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int check_flag( char );
static char *findnextmenuline( FILE *fp, char *selection );
static int echo( char *str );
static int sendToShell( char *str );
extern SYMTAB_ID sysSymTbl;

int tmenu( char *line );

/******************************* tmenuGetUnderScoreMode **************************
 *
 *  Description:  On some architecures a leading underscore is added to the symbol.
 *                This function get the underscore mode.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return   0 | 1
 *  Globals....:  underScoreMode
 ****************************************************************************/
int tmenuGetUnderScoreMode( void )
{
SYM_TYPE type;
char     *value;

	if( tmenuUnderScoreMode != -1 )
		return( tmenuUnderScoreMode );
		
		
	if( symFindByName( sysSymTbl, "_tmenuGetUnderScoreMode", &value, &type ) != OK )
	{
		tmenuUnderScoreMode = 0;
	}
	else
	{
		tmenuUnderScoreMode = 1;
	}
	if( tmenuDbg )
		printf("tmenuUnderScoreMode=%d\n", tmenuUnderScoreMode );

	return( tmenuUnderScoreMode );
}/*tmenuGetUnderScoreMode*/

/********************************** checkBuildIn *************************
 *
 *  Description:  Check and execute build in functions
 *
 *					ld			- load module
 *					echo		- print string
 *					moduleShow	- display loded modules
 *					set			- set a variable to a value (pipe to shell)
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  return 0 | error code
 *  Globals....:  -
 ****************************************************************************/
static int checkBuildIn( char *helpP, char *linebuf, int *isBuildInP )  /*nodoc*/
{
int 		argNo;
MODULE_ID	modId;

	if( tmenuDbg )
		printf("checkBuildIn:\n");
		
	if( *(linebuf) == 0 || linebuf == NULL )
		argNo=0;
	else
		argNo=1;

	*isBuildInP=1;

	/* set */		
	if( !strcmp( helpP, "set") )
	{
		return( sendToShell( linebuf ) );
	}/*if*/	

	/* ld */		
	if( !strcmp( helpP, "ld") )
	{
		modId = ld( 0, 0, linebuf ); /* load global syms, abort on error, from file linebuf */
		if( modId )
			return( OK );
		else
			return( ERROR );
	}/*if*/	

	/* echo */		
	if( !strcmp( helpP, "echo") )
	{
		return( echo( linebuf ) );
	}/*if*/	

	/* moduleShow */		
	if( !strcmp( helpP, "moduleShow") )
	{
		return( moduleShow( 0,0 ) );
	}/*if*/	

	/* tmenu */		
	if( !strcmp( helpP, "tmenu") )
	{
		return( tmenu( linebuf ) );
	}/*if*/	

	*isBuildInP=0;
	return( 0 );
}/*checkBuildIn*/

/******************************* resolveVariables ***************************
 *
 *  Description:  Scans the line for '$'.
 *---------------------------------------------------------------------------
 *  Input......:  buf  pointer to arguments e.g. "$DEV_NAME -b -t"
 *  Output.....:  return   0 | -1 on error
 *  Globals....:  functName
 ****************************************************************************/
static int resolveVariables( char *buf )  /*nodoc*/
{
char helpBuf[MAXLINELEN];
char varName[MAX_VAR_NAME+1];
char *p, *p2, *dst;
SYM_TYPE	type;
char		**val;
int			len;
	
	if( tmenuDbg )
		printf("resolveVariables: %s\n", buf );

	/* save the buffer first */
	bcopy( buf, helpBuf, MAXLINELEN );

	/* copy back */
	p   = helpBuf;
	dst = buf;
	while( p < (helpBuf+MAXLINELEN)
		   && dst < (buf+MAXLINELEN) 
		 )
	{
		if( *p == 0 )
		{
			*dst++ = *p++;
			break;
		}/*if*/

		if( *p != '$' )
		{
			*dst++ = *p++;
		}
		else
		{
			p++; /* crunch '$' */

			/* copy the variable name to varName */
			p2 = varName;
			while( p < (helpBuf+MAXLINELEN)
			       && *p != ' '
			       && *p != 0
			     )
			{
				*p2++ = *p++;
				if( p2 == (varName + MAX_VAR_NAME) )
				{
					printf("*** resolveVariables: variable name <%s> to long\n", varName );
					return( -1 );
				}/*if*/
			}/*while*/
			if( p2 == varName )
			{
				printf("*** resolveVariables: no variable name given\n" );
				return( -1 );
			}/*if*/
			*p2++ = 0;

			if( tmenuDbg )
				printf("varName = %s\n", varName );
		
			/* variables must be always a string */
			if( symFindByName( sysSymTbl, varName, (char**)&val, &type )
				== ERROR )
			{
				printf("*** resolveVariables: symFindByName: <%s> not found\n", varName );
				return( -1 );
			}/*if*/

			len = strlen( *val );
			if( len > ((buf+MAXLINELEN)-dst) )
			{
				printf("*** resolveVariables: <%s> string to long \n", varName );
				return( -1 );
			}/*if*/
			bcopy( *val, dst, len );
			dst += len;
		}/*if*/
	}/*while*/
	
	if( tmenuDbg )
		printf("%s\n", buf );

	return( 0 );
}/*resolveVariables*/


/******************************* systemVx ************************************
 *
 *  Description:  This is the VxWorks implementation of the function system.
 *				  The line buffer will be splitted into function name and
 *				  argument string.
 *				  The function pointer will be get by symFindByName().
 *				  
 *				  
 *
 *---------------------------------------------------------------------------
 *  Input......:  linebuf  pointer to comand with arguments e.g. "printf Hello World"
 *
 *  Output.....:  return   0 | -1 on error
 *
 *  Globals....:  functName
 *
 ****************************************************************************/
static int  systemVx( char *linebuf )  /*nodoc*/
{
int  		(*functPtr)(char *) = 0;
char 		*p, *helpP;
int  		n;
SYM_TYPE	dummy;
int 		error;
int 		isBuildIn;

	if( tmenuDbg )
		printf("systemVx: %s\n", linebuf );

	/* crunch the first spaces */
    while (*linebuf == ' ')
		linebuf++;

	p=functName;

	/* add '_' */
	if( tmenuGetUnderScoreMode() )
	{
		*p++ = '_';
	}/*if*/
	helpP = p; /*set helpP to function name without '_'*/
	
	/* copy the first string into functName */
	n=0;
	while( n < (sizeof(functName)-2)
		   && *linebuf != ' ' 
		   && *linebuf != 0
		 )
	{
		n++;
		*p++ = *linebuf++;
	}/*while*/
    
    *p = 0;

	if( tmenuDbg )
	{
		printf("%s\n", functName );

		if( *(linebuf-1) != 0 ) /* print linebuf only if given */
			printf("%s\n", linebuf );
	}/*if*/

	 /* crunch the one space between functionName and argumentString */
	if( *(linebuf-1) != 0 && *(linebuf) != 0 )
	{
		linebuf++;
		if( tmenuDbg )
			printf("%s\n", linebuf );
	}/*if*/

	if( *(linebuf) != 0 )
		if( resolveVariables( linebuf ) )
			return( -1 );

	/* check build in echo function */
	error = checkBuildIn( helpP, linebuf, &isBuildIn );
	if( isBuildIn )
		return( error );

	if( symFindByName( sysSymTbl, functName, (char**)&functPtr, &dummy )
		== ERROR )
	{
		printf("*** symFindByName: <%s> not found\n", functName );
		return( -1 );
	}/*if*/

	if( tmenuDbg )
		printf("%s at 0x%08x\n", functName, (int)functPtr );

	if( *(linebuf) != 0 )
	{
/*		if( resolveVariables( linebuf ) )
		return( -1 );*/
		return( functPtr( (linebuf) ) );
	}
	else
		return( functPtr( 0 ) );
	
	if( tmenuDbg )
	{
		printf("systemVx: executed\n" );
		taskDelay( sysClkRateGet() );
	}/*if*/
	return( 0 );
}/*systemVx*/


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static void trim( char *s )  /*nodoc*/
{
    char *t = &s[strlen(s)-1];

	if( tmenuDbg )
		printf("trim:\n");

    while( t >= s ){
        if( isspace(*t))
            *t = '\0';
        else
            break;
        t--;
    }
}


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int usage( int exitcode )  /*nodoc*/
{
    printf("Usage: tmenu <file> [<arg(s)] -p\n");
    printf("Function: Test Menu Generator\n");
    printf("Options:\n");
    printf("    file     menu description file\n");
    printf("    arg(s)   menu items (select keys) to AUTOEXECUTE\n");
    printf("    -p       print menu items before execute\n");
    printf("\n");
    printf("(c) 1997..1999 by MEN mikro elektronik GmbH\n");
    printf(RCSid);
	printf("\n\n");
    return( -1 );
}


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int isspecial_cmd( char *s )  /*nodoc*/
{
	if( tmenuDbg )
		printf("isspecial_cmd:\n");

    if( !strcmp( "BEG", s ))
        return 1;
    if( !strcmp( "END", s ))
        return 1;
    if( !strcmp( "OUT", s ))
        return 1;
    return 0;
}


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static void print_menu( FILE *fp, char *selection )  /*nodoc*/
{
	if( tmenuDbg )
		printf("print_menu:\n");

    rewind(fp);

    while( findnextmenuline(fp,selection)){
        if( !isspecial_cmd( selection )){
            if( selection[0] != '>' )
                printf( "<%3s> %s\n", selection , menutext );
            else
                printf( "%s\n", menutext );
        }
    }
    printf("<  q> end\n");
}

/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static void tmenuSelect( FILE *fp, char *selectkey )  /*nodoc*/
{
int shellId;
int shellStdIn;
int shellPriority=1;

	if( tmenuDbg )
		printf("tmenuSelect:\n");

    printf("\n");
    printf( "Selection: " );

	/* if this task != shell && stdin == stdin of shell */
	shellId = taskNameToId( "tShell" );
	if( taskIdSelf() != shellId )
	{
		/* avoid char input crunching of the shell */
		shellStdIn = ioTaskStdGet( shellId, 0 );
		if( tmenuDbg )
			printf(" tmenu select: stdin = %d\n", ioTaskStdGet( taskIdSelf(), 0 ) );
		if( ioTaskStdGet( taskIdSelf(), 0 ) == shellStdIn )
		{
			taskPriorityGet( shellId, &shellPriority );
			taskPrioritySet( shellId, 255 );
		}/*if*/
	}/*if*/

    fgets( selectkey, 20, stdioFp(0) );

	/* if this task != shell && stdin == stdin of shell */
	if( taskIdSelf() != shellId )
	{
		/* avoid char input crunching of the shell */
		shellStdIn = ioTaskStdGet( shellId, 0 );
		if( ioTaskStdGet( taskIdSelf(), 0 ) == shellStdIn )
			taskPrioritySet( shellId, shellPriority );
	}/*if*/

    printf("%s", selectkey);
    selectkey[strlen(selectkey)-1] = 0;  /* remove '\n' */
}

/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int lineempty( char *linebuf )  /*nodoc*/
{
	if( tmenuDbg )
		printf("lineempty:\n");

    while( *linebuf ){
        if( !isspace(*linebuf))
            return 0;
        linebuf++;
    }
    return 1;
}


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int findnextvalidline( FILE *fp, char *linebuf )  /*nodoc*/
{

	if( tmenuDbg )
		printf("findnextvalidline:\n");

    while( fgets( linebuf, MAXLINELEN, fp ))
    {
        linebuf[strlen(linebuf)-1] = '\0';  /* remove '\n' */
		/* for DOS CRLF */
		if( linebuf[strlen(linebuf)-1] == '\r' )
	        linebuf[strlen(linebuf)-1] = '\0';  /* remove '\r' */

        if( linebuf[0] == '#' || linebuf[0] == '*' || lineempty(linebuf))
            continue;
        else return 0;
    }/*if*/
    return -1;
}

/******************************* findnextmenuline **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static char *findnextmenuline( FILE *fp, char *selection )  /*nodoc*/
{
    char linebuf[MAXLINELEN];
    int len;

	if( tmenuDbg )
		printf("findnextmenuline:\n");

    while( findnextvalidline( fp, linebuf ) != -1 ){
        if( isspace(linebuf[0]))
            continue;
        len = strlen( linebuf );

        strncpy( selection, &linebuf[0], SELECTIONTEXTLEN );
        if( len > SELECTIONTEXTLEN )
            strncpy( flags , &linebuf[SELECTIONTEXTLEN], FLAGSLEN );
        else flags[0] = '\0';
        if( len > SELECTIONTEXTLEN+FLAGSLEN )
            strncpy( menutext, &linebuf[SELECTIONTEXTLEN+FLAGSLEN], MENUTEXTLEN );
        else menutext[0] = '\0';
        trim( selection );

        return menutext;
    }
    return NULL;
}

/******************************* streq **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int streq( char *stg1, char *stg2 )  /*nodoc*/
{
	if( tmenuDbg )
	{
		printf("TMENU: streq(stg1=%08x, stg2=%08x)\n",
		               (int)stg1, (int)stg2);
		taskDelay(2);
		printf("TMENU: stg1=%s, stg2=%s\n",
		               stg1, stg2);
		taskDelay(2);
	}/*if*/

    while (*stg2 == ' ')
		stg2++;

	if (strncmp(stg1, stg2, SELECTIONTEXTLEN)==0)
		return 1;
    else
		return 0;
}/*streq*/                    

/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int execute_selection( FILE *fp, char *selectkey, char *selection )  /*nodoc*/
{
    char linebuf[MAXLINELEN];
    char *p, *line;
    int error=0;

	if( tmenuDbg )
	{
		printf("TMENU: execute_selection: (fp=%08x, linebuf=%08x, selection=%08x )\n",
		               (int)fp, (int)linebuf, (int)selection );
		taskDelay(2);
	}/*if*/

again:
    rewind(fp);
    while( (line = findnextmenuline(fp,selection)) != NULL )
	{
        if( streq( selection, selectkey ))
		{
            if (PRINTITEMS && !isspecial_cmd( selection )) 
			{
                trim(line);
                printf("----- %s\n",line);
            }/*if*/

            while( findnextvalidline(fp, linebuf) != -1)
            {
				if( tmenuDbg>1 )
				{
					printf("TMENU: execute_selection() linebuf=%08x linebuf[0]=%02x\n", 
					               (int)linebuf, linebuf[0]);
					printf("%s", linebuf );
					taskDelay(2);
				}/*if*/

		        if( isspace((int)linebuf[0]) )
				{
					if( tmenuDbg>1 )
					{
						printf("TMENU: execute_selection() strchr\n");
						taskDelay(2);
					}/*if*/
                    if( (p = strchr( linebuf, '!' )) ){
                        if( logfp )
                            fprintf( logfp, "%s\n", p+1 );
                    }
                    else
                    {
						if( tmenuDbg>1 )
						{
							printf("TMENU: execute_selection() systemVx\n");
							taskDelay(2);
						}/*if*/
                        error = systemVx( linebuf );
						if( tmenuDbg>1 )
						{
							printf("TMENU: execute_selection() systemVx after\n");
							taskDelay(2);
						}/*if*/
                        if( error )
                        {
                            printf("Command terminated with error %d\n",
                                    error );
                            if( check_flag('E'))
                                break;
                        }/*if*/
                    }/*if*/
                }/*if*/
                else
					break;
            }/*while*/
            if( check_flag('L') ){
				printf("continue with loop\n");
				/* loop */
				goto again;
			}
            if( check_flag('P') )
			{
                printf("\nContinue with <RETURN> "); fflush(stdout);
                while(getchar() != '\n')
                    ;
            }/*if*/
            return error;
        }/*if*/
    }/*while*/

    printf("*** invalid selection <%s>\n", selectkey );
    return 1;
}/*execute_selection*/


/*******************************  **************************
 *
 *  Description:  
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  
 ****************************************************************************/
static int check_flag( char flg )  /*nodoc*/
{
    return (int)strchr( flags, flg );
}

/*
open_log( FILE *fp, char *desc_file )
{
    char logfile[150];

    strcpy( logfile, desc_file);
    *filename( logfile ) = '\0';

    rewind(fp);
    while( findnextmenuline(fp) ){
        if( !strcmp( "OUT", selection )){
            strcat( logfile, menutext );

            if( !( logfp = fopen( logfile, "w" )))
                printf( "can't open logfile %s", logfile );
        }
    }


}
*/

/******************************* tmenu **************************************
 *
 *  Description:  Test script interpreter.
 *
 *         Note: See usage.
 *               Needs a separate stdin/out/err path to avoid
 *               conflicts windShell crunching keyboard input.
 *
 *      Example: sp tmenu, "host:D:/work/VXWORKS/BUILD/MDIS_MEN/ld_mdis.tst"
 *
 * Tips:  There are three main traps:
 * 1.
 *  Error message: 
 *  "*** UOS: UOS_KeyStdIoSetOfTaskSet: UOS_KeyStdIoFd=-1 invalid file descriptor"
 *  Solution:
 *   Set UOS_KeyStdIoFd to a valid io path e.g. "/tyCo/1" differ from tShell
 *   io path.(3 /tyCo/0  1 in out err)
 * 2.
 *  Error message: 
 *  "*** UOS: UOS_KeyStdIoSetOfTaskSet: UOS key functions must spawn in a\
 *   separate task"
 *  Solution:
 *    spawn tmenu
 * 3.
 *  Error message: 
 *  "undefined symbol: XYZ"
 *  Solution:
 *   download the correct file or if you use the Tornado Shell use
 *   the target/host symbol table synchronisation
 * 4.
 *  Error message: 
 *  "Error: File C:\pipeTmenuToShell does not exist"
 *  "TMENU: create pipe /pipeTmenuToShell"
 *  Solution:
 *   This is not realy an error. TMENU use a pipe for comunication with
 *   TSHELL.
 *
 *---------------------------------------------------------------------------
 *  Input......:  command line string e.g. "testscript.tst"
 *  Output.....:  return 0 or error code
 *  Globals....:  UOS_KeyStdIoFd
 ****************************************************************************/
int main( int argc, char *argv[ ] )
{
    int  n, error=0;
    char descr_file[MAX_FILE_NAME+1];
    FILE *fp=NULL;
    char selectkey[20];
    char selection[SELECTIONTEXTLEN+1];

	/*------------------------+
	| get and init arguments  |
	+------------------------*/
    if( argc < 2 ) 
		return( usage(1) );
	if( strlen(argv[1])> MAX_FILE_NAME )
	{
		printf("*** tmenu: <%s> to long\n", argv[1]);
		return( -1 );
	}/*if*/
    strcpy( descr_file, argv[1] );

    AUTOEXEC = (argc > 2 ? 1 : 0);

    if (strcmp(argv[argc-1],"-p")==0) {
        PRINTITEMS = 1;
        argc--;
    }
    else
        PRINTITEMS = 0;

	logfp = NULL;

	bfill( menutext,  sizeof(menutext), 0 );
	bfill( selection, sizeof(selection), 0 );
	bfill( flags,     sizeof(flags), 0 );



    fp = fopen( descr_file, "r" );
    if( !fp ) printf( "can't open %s", descr_file );

/*    open_log(fp, descr_file);*/
    execute_selection( fp, "BEG", selection );

    if (AUTOEXEC) {                         /* auto exec */
        for (n=2; n<argc; n++)
            if( (error = execute_selection( fp, argv[n], selection )) )
                break;
    }
    else {                                  /* user input */
        do {
            print_menu( fp, selection );
            tmenuSelect( fp, selectkey );

            if( streq( selectkey, "q" ))
                break;

            execute_selection( fp, selectkey, selection );
        } while(1);
    }

    execute_selection( fp, "END", selection );
	if( fp != NULL )
		fclose( fp );
    return(error);
}/*tmenu*/


/******************************* echo **************************
 *
 *  Description:  Print <str>.
 *
 *---------------------------------------------------------------------------
 *  Input......:  str	string to print
 *  Output.....:  return 0 always
 *  Globals....:  -
 ****************************************************************************/
static int echo( char *str )  /*nodoc*/
{
	printf("%s\n", str );
	return(0);
}/*echo*/

/******************************* writeToPipe **************************
 *
 *  Description:  Write buf to mesgQueue.
 *
 *---------------------------------------------------------------------------
 *  Input......:  fd	file descriptor
 *                buf	buffer
 *                size	buffer size
 *  Output.....:  return 0 | error code
 *  Globals....:  -
 ****************************************************************************/
static int writeToPipe( int fd, char *buf, int size )  /*nodoc*/
{
int error = 0;
int loopy;

	for( loopy=0; loopy < size; loopy++ )
	{
		error = write( fd, buf++, 1 );
		if( error == ERROR )
			break;
	}/*for*/
	
	if( error != ERROR )
		error = size;
		
	return( error );
}/*writeToPipe*/

/******************************* sendToShell *********************************
 *
 *  Description:  Send a shell command.
 *				  It opens and create if neccessary a pipe.
 *				  Then it writes the string + shell reredirect commands to the
 *				  pipe.
 *				  The shell STD_IN will then redirected to the pipe and all
 *                three commands should be executed.
 *				  The function polls all 100ms the current STD_IN file
 *				  descriptor for synchronisation and finish if the STD_IN
 *				  is set to the original STD_IN.
 *
 *---------------------------------------------------------------------------
 *  Input......:  str	command string
 *  Output.....:  return 0 always
 *  Globals....:  -
 ****************************************************************************/
static int sendToShell( char *str )
{
int error = 0;
int error2 = 0;
int error3 = 0;
int fd;
char cmdStr[MAXLINELEN+2];
int  ioShellStdInSave;

	if( tmenuDbg )
		printf("sendToShell:\n");

	/* try open, create, open */
	fd = open( pipeName, O_WRONLY, 0 );
	if( fd == ERROR )
	{
		printErr("TMENU: create pipe %s\n", pipeName);
		if( pipeDevCreate( (char*)pipeName, 2*MAXLINELEN, 1 ) == ERROR )
		{
			printErr("*** TMENU: sendToShell: pipeDevCreate %s failed\n", pipeName);
			return( ERROR );
		}/*if*/
		fd = open( pipeName, O_WRONLY, 0 );
	}/*if*/

	if( fd == ERROR )
	{
		printErr("*** TMENU: sendToShell: open %s O_WRONLY failed\n", pipeName);
		return( ERROR );
	}/*if*/
	
	/* save shell input file descriptor */
	ioShellStdInSave = ioTaskStdGet( taskNameToId( "tShell" ), STD_IN );

	sprintf( cmdStr, "%s\n", str );
	error  = writeToPipe( fd, cmdStr, strlen(cmdStr) );

	/* command restore shell input file descriptor */
	sprintf( cmdStr, "shellOrigStdSet( 0, %d )\n", ioShellStdInSave );
	error2 = writeToPipe( fd, cmdStr, strlen(cmdStr) );
	sprintf( cmdStr, "excJobAdd( shellRestart,1,0,0,0,0,0)\n" );
	error3 = writeToPipe( fd, cmdStr, strlen(cmdStr) );

	close( fd ); /* close O_WRONLY */
	if( error == ERROR || error2 == ERROR || error3 == ERROR  )
	{
		printErr("*** TMENU: sendToShell: write to %s failed\n", pipeName);
		error = ERROR;
	}
	else
	{
		fd = open( pipeName, O_RDONLY, 0 ); /* open O_RDONLY */
		if( fd == ERROR )
		{
			printErr("*** TMENU: sendToShell: open %s O_RDONLY failed\n", pipeName);
			return( ERROR );
		}/*if*/
		error = 0; /* overwrite nbr of bytes was written */

		/* set shell input to pipe */
		shellOrigStdSet( STD_IN, fd );
		excJobAdd( shellRestart, TRUE, 0,0,0,0,0 );

		/* sync - wait until all 3 commands was exceuted 
		   - shell input file descriptor must not equal as pipe file descriptor */
		while( ioTaskStdGet( taskNameToId( "tShell" ), 0) == fd )
			UOS_Delay(100);

		close( fd );  /* close O_RDONLY */

		/* restore shell io file descriptors */
		ioTaskStdSet( taskNameToId( "tShell" ), STD_IN , ioShellStdInSave );

	}/*if*/
	
	return( error );
}/*sendToShell*/


/*
 * MISC routines for tmenu users
 */

/* clear screen (VT100) */
#define ESC			0x1b
#define CSI			"\x1b["
int cls(void)
{
	printf("%c!p", ESC );		/* reset terminal modes */
	printf("%s%d;%dH", CSI, 1, 1 );
	printf("%s2J", CSI );
	return 0;
}

int taste(void)
{
	printf("Hit any key to continue...");
	UOS_KeyWait();
	printf("\n");
	return 0;
}
