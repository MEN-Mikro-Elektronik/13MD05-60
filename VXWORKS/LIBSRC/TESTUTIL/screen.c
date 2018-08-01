/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: screen.c
 *      Project: test utility library (VXWORKS)
 *
 *      $Author: kp $
 *        $Date: 2001/05/14 10:37:00 $
 *    $Revision: 1.3 $
 *
 *  Description: Functions for screen output. Hardcoded to VT100 terminals
 *                      
 *               Global variable TU_delay can be set to do a taskdelay
 *               after cursor positioning
 *       
 *     Required: uti.l termlib.l
 *     Switches:  
 *
 *---------------------------[ Public Functions ]----------------------------
 *  
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: screen.c,v $
 * Revision 1.3  2001/05/14 10:37:00  kp
 * added global var TU_delay
 *
 * Revision 1.2  2000/01/13 11:51:38  kp
 * added taskDelay when positioning cursor (Wyse Terminal)
 *
 * Revision 1.1  2000/01/12 10:57:15  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static char *RCSid="$Header: /dd2/CVSR/VXWORKS/LIBSRC/TESTUTIL/screen.c,v 1.3 2001/05/14 10:37:00 kp Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <ioLib.h>
#include <time.h>
#include <MEN/men_typs.h>
#include <MEN/testutil.h>


/*---------------+
| Local defines  |
+---------------*/
#define MSGERR_TOP	2				/* top of user area of message/error wdw */
#define	PASS		45
#define	TIME		66

#define ESC			0x1b
#define CSI			"\x1b["

/*--- flags for tu_(v)print_xy ---*/
#define STANDOUT	0x1				/* put chars in standout mode */
#define NOFLUSH		0x2				/* don't flush stdout (not used here)*/

typedef struct {
	int left, top, width, height, present;
} wdw;

static void tu_intercept(int signal);
static int  tu_print_xy( int x, int y, int flags, char *fmt, ... );
static int  tu_vprint_xy( int x, int y, int flags, char *fmt, va_list ap );
static int  tu_print_wdw(int wdw_id, int x, int y, int flags, char *fmt, ... );
static int  tu_vprint_wdw(int wdw_id, int x, int y, int flags, char *fmt, 
						  va_list ap );
static wdw *get_wdw(int wdw_id);
static void clear_screen(void);
static void cursor_on(void);
static void cursor_off(void);
static void init_screen(void);
static void exit_screen(void);
static void move_cursor(int x, int y);
static void standout(int on);

/*------------------+
| Global variables  |
+------------------*/
int TU_termLines=24, TU_termColms=80;
int TU_delay=0;

static int screen_height;			/* total height of screen */
static int screen_width;			/* total width of screen */
static u_int32 start_time;			/* start time of test */

static u_int32 total_errors;		/* total error count */
static int no_error;				/* nth error in error window */
static int dont_stop_errors;		/* dont stop on subsequent errors */

static wdw w_message, w_error, w_result, w_key_line; /* window dimensions */

static char key_line_cur[80];		/* current key line */
static char key_line_sav[80];		/* saved key line */

/********************************** tu_init_screen **************************
 *
 *  Description: Initializes the screen for screen oriented output using 
 *  the functions from uti.l.                     
 *
 *  - set input to noecho
 *  - Draw basic screen layout
 *  - get the start time
 *---------------------------------------------------------------------------
 *  Input......:  
 *   title				user-defined string to print in headline
 *   error_wdw_width	width of error window  	(0=no error window)
 *   result_wdw_height  height of result window (0=no result window)
 *   key_line_flg		0=no key line, 1=key line present
 *
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/

void tu_init_screen	( char *title, int error_wdw_width, 
			     	  int result_wdw_height, int key_line_flg )
{
	extern char *_prgname();
	int i;

	total_errors = 0;
	no_error = 0;
	dont_stop_errors = 0;

	/*----------------------------+
    |  Initialize the screen I/O  |
    +----------------------------*/
	init_screen();
	
	screen_height = TU_termLines;		/* get size of screen */
	screen_width = TU_termColms;

/*???	p_echo(0,0);*/					/* set input to noecho */

	/*------------------+
    |  Compute windows  |
    +------------------*/

	if( error_wdw_width ){
		w_error.left = screen_width-error_wdw_width;
		w_error.present = 1;
		w_error.width = error_wdw_width;
		w_message.width = w_error.left-1;
	}
	else
		w_message.width = screen_width;

	if( result_wdw_height ){
		w_result.top = screen_height-key_line_flg-result_wdw_height;
		w_result.height = result_wdw_height;
		w_result.left = 0;
		w_result.width = screen_width;
		w_result.present = 1;
	}

	w_message.left = 0;
	w_message.top = MSGERR_TOP;
	w_message.height = w_result.top-1-MSGERR_TOP;
	w_message.present = 1;

	w_error.top = MSGERR_TOP;
	w_error.height = w_message.height;
	
	if( key_line_flg ){
		w_key_line.present = 1;
		w_key_line.top = screen_height-1;
		w_key_line.left = 0;
		w_key_line.width = screen_width-1;
		w_key_line.height = 1;
	}

	/*-------------------+
    |  Draw basic window |
    +-------------------*/
	cursor_off();
	clear_screen();
	move_cursor(0,0);

	/*--- Headline ---*/
	tu_print_xy( 0,0,NOFLUSH, "%-51s", title );

	/*--- Messages/Error Headline ---*/
	tu_print_wdw( WDW_MSG, 0,-1,NOFLUSH|STANDOUT, "%-*s", w_message.width,
				 "Messages:");
	if( w_error.present )
		tu_print_wdw( WDW_ERR, 0, -1, NOFLUSH|STANDOUT,"%-*s", w_error.width,
					 "Errors:");

	/*--- vertical line between Message/Error Window ---*/
	if( w_error.present )
		for(i=0; i<w_error.height; i++)
			tu_print_wdw( WDW_ERR, -1, i, NOFLUSH, "|" );

	/*--- horizontal line over result window ---*/
	if( w_result.present )
		tu_print_wdw( WDW_RESULT, 0, -1, STANDOUT, "%*s", screen_width, "" );

	/*-----------------+
    |  Get start time  |
    +-----------------*/
	start_time = tu_get_tick();
}

/********************************* tu_print *********************************
 *
 *  Description:  print the printf-like <fmt> and arguments into the 
 *  specified window, relative to the window positions                       
 *
 *  Note: x/y are not checked for validity
 *---------------------------------------------------------------------------
 *  Input......:  
 *    wdw_id		ID of window to print to
 *    x,y			relative position to print
 *	  fmt			printf like format string
 *    ...			arguments to printf
 *  Output.....:  length of string
 *  Globals....:  ---
 ****************************************************************************/
int tu_print(int wdw_id, int x, int y, char *fmt, ... )
{
	int rval;
	va_list ap;
	va_start (ap,fmt);
	rval = tu_vprint_wdw( wdw_id, x, y, 0, fmt, ap );
	va_end(ap);
	return rval;
}

/********************************** tu_print_revers **************************
 *
 *  Description:  print the printf-like <fmt> and arguments into the 
 *  specified window in revers, relative to the window positions
 *
 *  Note: x/y are not checked for validity
 *---------------------------------------------------------------------------
 *  Input......:  
 *    wdw_id		ID of window to print to
 *    x,y			relative position to print
 *	  fmt			printf like format string
 *    ...			arguments to printf
 *  Output.....:  length of string
 *  Globals....:  ---
 ****************************************************************************/
int tu_print_revers(int wdw_id, int x, int y, char *fmt, ... )
{
	int rval;
	va_list ap;
	va_start (ap,fmt);
	rval = tu_vprint_wdw( wdw_id, x, y, STANDOUT, fmt, ap );
	va_end(ap);
	return rval;
}

/********************************** tu_clear ********************************
 *
 *  Description:  Clear <n> chars in window <wdw_id> starting at position
 *  <x>, <y>. If <n> is 0, the entire line starting at <x> is cleared
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *    wdw_id		ID of window to print to
 *    x,y			relative position to print
 *	  n				number of chars to clear
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_clear (int wdw_id, int x, int y, int n )
{
	wdw *w = get_wdw(wdw_id);

	if( !w->present ) return;

	if( n==0 || (x+n > w->width)) n=w->width-x;
	tu_print( wdw_id, x, y, "%*s", n, "" );
}

/********************************** tu_clear_wdw ****************************
 *
 *  Description:  Clear entire window with ID <wdw_id>
 *---------------------------------------------------------------------------
 *  Input......:  
 *    wdw_id		ID of window to clear
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_clear_wdw (int wdw_id)
{
	int y=0;
	wdw *w = get_wdw(wdw_id);

	if( !w->present ) return;

	for(y=0; y<w->height; y++) tu_clear( wdw_id, 0, y, 0 );
}

/********************************** tu_print_errhead *************************
 *
 *  Description:  Set the headline of the error window. <line> is concatenated
 *                with "No". e.g (<line>="Pos     Detail)
 *
 *      No     Pos     Detail
 *      ------ ------- ------         
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *     line			user defined line
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_print_errhead (char *line)
{
	char lbuf[80];

	strcpy(lbuf, " No     ");
	strcat(lbuf, line );
	tu_print( WDW_ERR, 0, 0, lbuf );
	tu_print( WDW_ERR, 0, 1, tu_underline_string(lbuf) );	
}

/********************************** tu_underline_string **********************
 *
 *  Description:  Underline given string with '-' e.g:
 *  
 *      Error# Pos    Detail
 *      ------ ------ ------                   
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *     line			string to underline
 *  Output.....:  
 *     return		string with '-' chars
 *  Globals....:  ---
 ****************************************************************************/
char *tu_underline_string( char *line )
{
	static char buf[80];
	char *lp, *bp;
	int inflag=0;

	for(lp=line,bp=buf; *lp; lp++,bp++){
		if( *lp!=' ' ){
			if( !inflag && bp>buf) bp[-1]=' ';
			inflag=1;
		}
		else inflag=0;
		*bp='-';
	}
	*bp='\0';
	return buf;
}

/********************************** tu_print_error****************************
 *
 *  Description: Print error message into error window. Error window is 
 *  organized as a ring buffer.  
 *                         
 *  <toterr_update> is added to the total error count.
 * 
 *  <user_stop> flags if the program should stop after displaying the error.
 *  If so, tu_print_error changes the keyline to "c/w/q". 
 *---------------------------------------------------------------------------
 *  Input......:  
 *    toterr_update		update total errorcount by <toterr_update>
 *    user_stop			if <>0, waits for a continue/abort message from user
 *    fmt				printf like fmt string
 *	  ...				arguments to printf
 *  Output.....:  
 *    return			0=continue 
 *						1=quit	(in case of user_stop and 'q' entered)
 *  Globals....:  ---
 ****************************************************************************/
int tu_print_error (int toterr_update, int user_stop, char *fmt, ...)
{
	int y, retval=0,len;
	va_list ap;

	/*--- clear '>' of last recent error ---*/
	if( no_error )
		tu_clear(WDW_ERR, 0, ((no_error-1)%(w_error.height-2))+2, 1 );

	y = (no_error%(w_error.height-2))+2;

	/*--- print error line in error window ---*/
	len = tu_print(WDW_ERR, 0, y, ">%6d", no_error+1 );
	va_start(ap,fmt);
	len += tu_vprint_wdw( WDW_ERR, 8, y, 0, fmt, ap );
	tu_print( WDW_ERR, len+1, y, "%*s", w_error.width-len-1, "" );
	va_end(ap);
	no_error++;

	/*--- update total errors ---*/
	if( toterr_update ){
		total_errors += toterr_update;
		tu_print_wdw( WDW_ERR, 8, -1, STANDOUT, "%6d", total_errors );
	}
	/*--- check for user stop ---*/
	if( user_stop && !dont_stop_errors){
		tu_save_key_line();
		tu_key_line("Continue-until-next continue-Without-stop Quit");

		retval=-1;
		do {
			switch(tu_wait_key()){
			case 'w': dont_stop_errors=1;
			case 'c': retval=0;  break;
			case 'q': retval=1;  break;
			}
		} while(retval==-1);

		tu_restore_key_line();
	}
	return retval;
}

/********************************** tu_key_line ******************************
 *
 *  Description:  print <line> in key line. Every uppercase character is
 *  printed reverse (standout mode)                       
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *    line		  line to print
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_key_line (char *line)
{
	int stand=0;

	strcpy( key_line_cur, line );
	
	tu_clear_wdw(WDW_KEYS);
	tu_print(WDW_KEYS, 0,0,"");

	while(*line){
		if( *line>='A' && *line<='Z' ){
			if( !stand ) { standout(1); stand=1; }
		}
		else {
			if(stand) { standout(0); stand=0; }
		}
		printf("%c", *line++);
	}
}

/********************************** tu_save/restore_key_line *****************
 *
 *  Description:  tu_save_keyline saves the current keyline into a temporary
 *  buffer, tu_restore_keyline restores it.
 *  WARNING: only a single buffer available!                       
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_save_key_line (void)
{
	strcpy( key_line_sav, key_line_cur );
}

void tu_restore_key_line (void)
{
	tu_key_line(key_line_sav);
}


/********************************** tu_print_pass *****************************
 *
 *  Description:  print "Pass x of y" in headline. If total_pass is 0, only
 *  "Pass x" is printed.                       
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_print_pass (int current_pass, int total_pass)
{
	move_cursor(PASS,0);
	printf("Pass %3d", current_pass);
	if( total_pass ) 
		printf(" of %3d", total_pass );
}

/********************************** tu_print_elapsed_time ********************
 *
 *  Description:  print elapsed time in headline. (hh:mm:ss)
 *                         
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_print_elapsed_time (void)
{
	move_cursor(TIME,0);
	printf("Time %s", tu_tick2string(0, tu_get_tick()-start_time));
}


/********************************** tu_keybd_hit ******************************
 *
 *  Description:  Checks if a key was pressed on <stdin>. Returns the key
 *  or -1 if none. Chars are returned lower case always                       
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *    return	    key hit
 *  Globals....:  ---
 ****************************************************************************/
int tu_keybd_hit(void)
{
	int nBytes;

	ioctl( 0, FIONREAD, (int)&nBytes);

	if( nBytes > 0 )
		return tu_wait_key();

	return -1;
}

/********************************** tu_wait_key ******************************
 *
 *  Description: Waits until a key is pressed on <stdin>. Returns the key
 *  or -1. Chars are returned lower case always 
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *    return	    key hit
 *  Globals....:  ---
 ****************************************************************************/
int tu_wait_key(void)
{
	char c;

	read(0,&c,1);
	if( c>='A' && c<='Z' ) c += 'a'-'A';
	return c;
}

/********************************** tu_exit *********************************
 *
 *  Description:  Exit the screen oriented I/O
 *                         
 *
 *---------------------------------------------------------------------------
 *  Input......:  
 *  Output.....:  
 *  Globals....:  ---
 ****************************************************************************/
void tu_exit(void)
{
	exit_screen();
	cursor_on();
	/*p_echo(0,1);???*/
}

static int tu_print_xy( int x, int y, int flags, char *fmt, ... )
{
	int rval;
	va_list ap;
	va_start (ap,fmt);

	rval = tu_vprint_xy( x, y, flags, fmt, ap );
	va_end(ap);
	return rval;
}

static int tu_vprint_xy( int x, int y, int flags, char *fmt, va_list ap )
{
	int rval;
	move_cursor(x,y);
	if( flags & STANDOUT ) standout(1);
	rval = vprintf( fmt, ap );
	if( flags & STANDOUT ) standout(0);
	return rval;
}

static int tu_print_wdw(int wdw_id, int x, int y, int flags, char *fmt, ... )
{
	int rval;
	va_list ap;
	va_start (ap,fmt);
	rval = tu_vprint_wdw( wdw_id, x, y, flags, fmt, ap );
	va_end(ap);
	return rval;
}

static int tu_vprint_wdw(int wdw_id, int x, int y, int flags, char *fmt, 
						  va_list ap )
{
	wdw *w = get_wdw(wdw_id);

	if( !w->present ) return 0;

	return tu_vprint_xy( w->left+x, w->top+y, flags, fmt, ap );
}


static wdw *get_wdw(int wdw_id)
{
	switch( wdw_id ){
	case WDW_MSG:	return &w_message;
	case WDW_ERR:	return &w_error;
	case WDW_RESULT:return &w_result;
	case WDW_KEYS:	return &w_key_line;
	default:		return NULL;
	}
}
static void printnulls(int n)
{
	int i;

	/* Only needed for Wyse terminal */
	if( TU_delay )
		taskDelay( TU_delay * n );
}

static void clear_screen(void)
{
	printf("%c!p", ESC );		/* reset terminal modes */
	move_cursor(0,0);
	printf("%s2J", CSI );
}

static void cursor_on(void)
{
	printf("%s25h", CSI );
}

static void cursor_off(void)
{
	printf("%s25l", CSI );
}

static void exit_screen(void)
{	
	move_cursor( 0, screen_height-1 );
	standout(0);
	/*--- switch terminal back ---*/
	ioctl( 0, FIOSETOPTIONS, OPT_TERMINAL );	
}

static void init_screen(void)
{	
	/*--- switch terminal to raw mode ---*/
	ioctl( 0, FIOSETOPTIONS, OPT_RAW );
}

static void move_cursor(int x, int y)
{	
	printf("%s%d;%dH", CSI, y+1, x+1 );
	printnulls(1);
}

static void standout(int on)
{
	if( on )
		printf("%s7m", CSI );
	else
		printf("%s27m", CSI );
	printnulls(1);
}
