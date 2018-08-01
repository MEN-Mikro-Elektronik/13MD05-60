/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  vxdbgcga.c
 *
 *      \author  uf
 *        $Date: 2010/09/10 14:21:20 $
 *    $Revision: 1.3 $
 *
 *        \brief  X86 pre STDIO debugs to CGA memory
 *                BIOS must init the TEXT mode before.
 *            	  Writes directly into VGA text RAM.
 *                Expects the 25 * 80 text mode setup.
 *                      
 *                ToDo: -\b support
 *                      -\t support
 *                      
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: vxdbgcga.c,v $
 * Revision 1.3  2010/09/10 14:21:20  UFranke
 * R: boorom size optimization
 * M: test code disabled
 *
 * Revision 1.2  2009/07/30 11:05:33  ufranke
 * added
 *  + line mode
 *
 * Revision 1.1  2009/05/19 13:11:05  ufranke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2010 by MEN Mikro Elektronik GmbH, Nueremberg, Germany 
 ****************************************************************************/

static const char RCSid[]= 
	"$Id: vxdbgcga.c,v 1.3 2010/09/10 14:21:20 UFranke Exp $"
	"\n\tBuild " __DATE__ " " __TIME__"\n";

 
/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#include <MEN/men_typs.h>
#include <MEN/vxdbgcga.h> 

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct
{
	u_int8	x;
	u_int8	y;
}VXDBGCGA_CURSOR;
/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/

static char             VXDBGCGA_linebuf[0x1000];
static VXDBGCGA_CURSOR  cursor = { 0, 0 };

int						VXDBGCGA_firstLineMode = 1;
/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/
#define MAX_X	80
#define MAX_Y	25

#define MEM_BASE_CGA		0xB8000   /* CGA memory on a VGA card */

/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/



/**********************************************************************/
/** Checks the pointer and wrap around if necessary.
 *  Checks the pointer and wrap around if necessary.
 *  \param   p		current pointer to CGA memory
 *  \param   newP   location for new pointer to CGA memory
 *
 *  \return -
 */
static void _VXDBGCGA_SaveCursor( u_int8 *p, u_int8 **newP )
{
	int tempP = (int)p - MEM_BASE_CGA;

	/* can be filled for scrolling */
	if( (int)(tempP) >= (MAX_X * MAX_Y * sizeof( u_int16 ) ) )
	{
		*newP = (u_int8*)MEM_BASE_CGA; /* simple wrap around */
		cursor.x = 0;
		cursor.y = 0;
	}
	else
	{

		if( VXDBGCGA_firstLineMode )
		{
			cursor.x = 0;
			cursor.y = 0;
		}
		else
		{
			cursor.y = (u_int8)(tempP / ( MAX_X * sizeof(u_int16) ));
			cursor.x = (u_int8)(tempP - ( cursor.y * ( MAX_X * sizeof(u_int16) ) ));
	
			*newP = p;
		}/*if*/
	}
}

/**********************************************************************/
/** Writes a string like printf directly into the CGA memory.
 *  Writes a string like printf directly into the CGA memory.
 *  Converts \n.
 *  \param str		string
 *
 *  \return number of printed bytes
 */
static void _VXDBGCGA_Write( char *str )
{
	volatile u_int8 *p = (volatile u_int8*)MEM_BASE_CGA;

	p += cursor.y * MAX_X * sizeof(u_int16);
	p += cursor.x;

	while( *(str) != 0 )
	{
		if( *str == '\n' )
		{
			int i;
			volatile u_int8 *ph;

			while( ( (int)(p-MEM_BASE_CGA) % (MAX_X*2) ) )
			{
				*p = ' ';  /* clear the rest of the line */
				p += 2;
			}
			
			ph = p;
			for( i=1; i<MAX_X; i++ )
			{
				*ph = ' ';	 /* clear the new line first */
				ph += 2;
			}
			str++;
			_VXDBGCGA_SaveCursor( (u_int8*)p, (u_int8**)&p );
			continue;
		}
		
		*p = *str;
		p += 2;
		str++;
		_VXDBGCGA_SaveCursor( (u_int8*)p, (u_int8**)&p );
	}
}


/**********************************************************************/
/** Writes a string like printf directly into the CGA memory.
 *  Calls sprintf to convert the string and calls _VXDBGCGA_Write() then.
 *  The final string should be less than 0x1000 == sizeof(VXDBGCGA_linebuf).
 *  \param frmt		format string like printf
 *  \param arg1		first argument of the variable argument list
 *
 *  \return number of printed bytes
 */

int VXDBGCGA_Write( char *frmt, ... )
{
    va_list argptr;
    int     length;

    va_start(argptr,frmt);
    length = vsprintf( VXDBGCGA_linebuf ,frmt, argptr );
    va_end(argptr);

	if( length >= sizeof(VXDBGCGA_linebuf) )
	{
		for(;;)
		{
			;  /* buffer overflow "nueremberg: we have a problem" */
		}
	}

	if( length )
	{
		_VXDBGCGA_Write( VXDBGCGA_linebuf );
	}
	
	return( length );
}

/**********************************************************************/
/** In first line mode each VXDBGCGA_Write() starts at position 0,0 .
 *  Changes the mode and set start position to 0,0 if called.
 *  \param enable	0 or enable first line mode
 *
 *  \return n/a
 */
void VXDBGCGA_FirstLineMode( int enable )
{
	VXDBGCGA_firstLineMode = enable;

	cursor.x = 0;
	cursor.y = 0;
}


#if 0
/* test code only */
void VXDBGCGA_Check( int check )
{

	switch( check )
	{
		default:
		case 0:
			VXDBGCGA_Write(
				   "1 - **************\n" 
	               "2 - **************\n"
	               "3 - **************\n"
	               "4 - **************\n"
	               "5 - **************\n"
	               "6 - **************\n"
	               "7 - **************\n"
	               "8 - **************\n"
	               "9 - **************\n"
	               "10 - **************\n"
	               "11 - **************\n"
	               "12 - **************\n"
	               "13 - **************\n"
	               "14 - **************\n"
	               "15 - **************\n"
	               "16 - **************\n"
	               "17 - **************\n"
	               "18 - **************\n"
	               "19 - **************\n"
	               "20 - **************\n"
	               "21 - **************\n"
	               "22 - **************\n"
	               "23 - **************\n"
	               "24 - **************\n"
	               "25 - **************\n" );
	    	break;

		case 1:
			VXDBGCGA_Write(
				   "1 - abc\n" 
	               "2 - abc\n"
	               "3 - abc\n"
	               "4 - abc\n"
	               "5 - abc\n"
	               "6\n"
	               "7\n"
	               "8\n"
	               "9\n"
	               "10\n"
	               "11\n"
	               "12\n"
	               "13\n"
	               "14\n"
	               "15\n"
	               "16\n"
	               "17\n"
	               "18\n"
	               "19\n"
	               "20\n"
	               "21\n"
	               "22\n"
	               "23\n"
	               "24\n"
	               "25 - abc\n"
	               "26 - abc\n"
	               "27 - abc\n"
	               "28 - abc\n"
	               "29 - abc\n"
	               "30"
	                );
	    	break;
	
		case 2:
			VXDBGCGA_Write( "1 - zdf\n" );
			VXDBGCGA_Write( "2 - ard\n" );
			VXDBGCGA_Write( "3 - zdf\n" );
			VXDBGCGA_Write( "4 - ard\n" );
			VXDBGCGA_Write( "5 - zdf\n" );
			VXDBGCGA_Write( "6 - ard\n" );
			VXDBGCGA_Write( "7 - zdf\n" );
			VXDBGCGA_Write( "8 - ard\n" );
			VXDBGCGA_Write( "9 - zdf\n" );
			VXDBGCGA_Write( "10 - ard\n" );
			VXDBGCGA_Write( "11 - zdf\n" );
			VXDBGCGA_Write( "12 - ard\n" );
			VXDBGCGA_Write( "13 - zdf\n" );
			VXDBGCGA_Write( "14 - ard\n" );
			VXDBGCGA_Write( "15 - zdf\n" );
			VXDBGCGA_Write( "16 - ard\n" );
			VXDBGCGA_Write( "17 - zdf\n" );
			VXDBGCGA_Write( "18 - ard\n" );
			VXDBGCGA_Write( "19 - zdf\n" );
			VXDBGCGA_Write( "20 - ard\n" );
			VXDBGCGA_Write( "21 - zdf\n" );
			VXDBGCGA_Write( "22 - ard\n" );
			VXDBGCGA_Write( "23 - zdf\n" );
			VXDBGCGA_Write( "24 - ard\n" );
			VXDBGCGA_Write( "25 - zdf\n" );
			VXDBGCGA_Write( "26 - ard\n" );
			VXDBGCGA_Write( "27 - zdf\n" );
			VXDBGCGA_Write( "28 - ard\n" );
			VXDBGCGA_Write( "29 - zdf\n" );
	    	break;
	}/*switch*/
}

#endif


