/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: dbg.c
 *      Project: DBG library
 *
 *       Author: uf
 *        $Date: 2010/03/12 11:27:38 $
 *    $Revision: 1.24 $
 *
 *  Description: Debug functions for VxWorks
 *               for LowLevel Driver, MDIS and other
 *
 *				 Debug demon see DBG_Show().
 *
 *               For using a fix and crash save buffer use USER_RESERVED_MEM
 *               int config.h for getting a pice of memory wich is not
 *               under control of memory managment and will not be initialized
 *               (overwritten) at boot or startup time.
 *				 DBG_MappedToReservedMemP and DBG_MemSize must be initialized
 *               with the values of the reserved memory before the first 
 *               DBG_Init() call.
 *				 Take care that WindView or other tools do not use the same
 *				 pice of memory.
 *
 *		Example unsing crash save buffer:
 *            -> DBG_MapToReservedMem( sysMemTop() )
 *            -> DBG_SetMemSize( 0x1000 ) (i.e. size of USER_RESERVED_MEM)
 *            -> DBG_Show( 1 )
 *
 *     Required: -
 *     Switches: _SMP_COMPATIBLE_ONLY
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: dbg.c,v $
 * Revision 1.24  2010/03/12 11:27:38  ufranke
 * R: linker error SDA VxWorks 6.7 PPC due to access to global variables
 * M: modifying DBG_MappedToReservedMemP, DBG_MemSize by functions
 *    DBG_MapToReservedMem() and DBG_SetMemSize()
 *
 * Revision 1.23  2010/01/27 09:42:47  ufranke
 * R: DIAB compiler problem VxWorks 6.7 for PPC85xx
 * M: workaround no pointer increments in sprintf and write
 *
 * Revision 1.22  2009/05/29 10:56:23  ufranke
 * R: UP or SMP compatible now
 * M: changed _SMP_AND_UP_COMPATIBLE to _SMP_COMPATIBLE_ONLY
 *
 * Revision 1.21  2009/03/31 09:36:07  ufranke
 * R: trace count option not working
 * M: fixed
 *
 * Revision 1.20  2008/10/07 11:45:39  ufranke
 * R: G_irqHdl not static
 * M: set to static
 *
 * Revision 1.19  2008/10/06 10:40:35  ufranke
 * cosmetics
 *
 * Revision 1.18  2008/09/26 14:31:09  ufranke
 * R: SMP support for VxWorks 6.6
 * M: make it SMP and UP compatible
 *
 * Revision 1.17  2008/09/04 15:35:23  ufranke
 * R: diab compiler support
 * M: cosmetics
 *
 * Revision 1.16  2006/08/29 10:19:09  ufranke
 * cosmetics
 *
 * Revision 1.15  2006/07/25 15:21:52  ufranke
 * cosmetics
 *
 * Revision 1.14  2006/07/24 14:54:13  ufranke
 * changed
 *  - debugs can be used now also in sysHwInit()
 *    void sysHwInit2( void )
 *    {
 *      ...
 *      DBG_MappedToReservedMemP = (void*)0x2800000;
 *      DBG_MemSize=0x800000;
 *      DBG_doCacheFlush = 1;
 *      DBGINIT((NULL,&DBH));
 *      DBGWRT_ERR(( DBH, "%s line %d", __FUNCTION__ , __LINE__));
 *      ...
 *
 * Revision 1.13  2006/06/30 14:36:13  ufranke
 * added
 *  + DBG_withTraceCount for post mortem debug trace
 *
 * Revision 1.12  2006/03/10 14:57:51  UFRANKE
 * cosmetics
 *
 * Revision 1.11  2005/12/21 08:56:29  UFranke
 * fixed
 *  - DBG_Show() for VxWorks 6.x
 *
 * Revision 1.10  2005/09/08 15:00:14  UFranke
 * added
 *  + DBG_DisableOverwrite()
 *
 * Revision 1.9  2005/07/01 15:35:27  kp
 * Added possibility to flush cache after each write to ring buffer
 * (for post mortem dumps). Set DBG_doCacheFlush to TRUE if you
 * want this
 *
 * Revision 1.8  2005/05/19 15:11:42  ufranke
 * added
 *  + DBG_Clear()
 *
 * Revision 1.7  2005/05/12 15:01:59  ufranke
 * changed
 * - DBG_Show(1) for crash save
 *   can be called now without reboot
 *   to get the complete trace
 *
 * Revision 1.6  2003/01/15 14:22:46  kpftp
 * Reworked to handle concurrent DBG_Writes of normal and interrupt routines
 *
 * Revision 1.5  2002/09/13 14:07:44  UFranke
 * cosmetics
 *
 * Revision 1.4  2001/07/19 09:53:45  franke
 * cosmetics
 *
 * Revision 1.3  1999/08/30 11:15:03  Franke
 * cosmetics
 *
 * Revision 1.2  1999/06/29 15:30:50  Franke
 * removed use of logLib
 * added   crash save buffer possibility
 * 	DBG_Show(), DBG_ShowRb(), DBG_Stop()
 *
 * Revision 1.1  1998/06/17 14:31:35  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char DBG_RCSid[]="$Id: dbg.c,v 1.24 2010/03/12 11:27:38 ufranke Exp $";

#include <MEN/men_typs.h>

#include <memLib.h>
#include <stdlib.h>
#include <stdio.h>
#include <fioLib.h>
#include <errno.h>
#include <stdarg.h>
#include <intLib.h>
#include <taskLib.h>
#include <string.h>
#include <ioLib.h>
#include <cacheLib.h>

#include <MEN/dbg_vxworks.h> 

#ifdef _SMP_COMPATIBLE_ONLY
	#include <MEN/oss.h>
	static OSS_IRQ_HANDLE *G_irqHdl = NULL;
#else
	#ifdef _UP_COMPATIBLE_ONLY
	#else
		/* info about default mode */
		#warning "INFO: _UP_COMPATIBLE_ONLY"
	#endif
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define MAX_CHARS   4096

#ifndef DBG_MEM_SZ_MAX 
# define DBG_MEM_SZ_MAX 0x20000
#endif

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/*--- DBG handle ---*/
typedef struct 
{
    char    linebuf[MAX_CHARS+1];   /* line buffer */
} DBG_HANDLE;

DBG_HANDLE	DBG_hdl_G;

/* dummy definition for dbg.h */
#define DBG_MYLEVEL     null
/* say: don't declare DBG_HANDLE */
#define _NO_DBG_HANDLE
/* DBG_HANDLE needs to be declared BEFORE including dbg.h */
#include <MEN/dbg.h>

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
int DBG_IsInit = 0;
int DBG_OutFd  = 2; /* write to STDERR */
static unsigned char *DBG_MappedToReservedMemP = NULL;
unsigned int  DBG_Count   = 0;
static unsigned int  DBG_MemSize = 0x4000;
unsigned char *DBG_MemP   	= NULL;
unsigned char *DBG_MemEndP      = NULL;
unsigned char *DBG_WriteP 	= NULL;
unsigned char *DBG_ReadP  	= NULL;
static   int   DBG_OverWrtMode;
int DBG_ShowEndless    	= 1;
int DBG_WithTaskId 		= 0;
int DBG_withTraceCount 	= 0;
int DBG_traceCount 		= 0;
int DBG_doCacheFlush 	= FALSE;	/* if TRUE, flush cache after write */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
/* none */


/* Helpers to access and set pointer and other variables in this module over local
relative access boundaries (see: SDA21 linker error) */

/****************************** DBG_GetMemSize *********************************
 *
 *  Description:  Get the MemSize variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  value of local DBG_MemSize
 *  Globals....:  -
 ****************************************************************************/
unsigned int DBG_GetMemSize(void)
{
	return DBG_MemSize;
}

/****************************** DBG_SetMemSize *********************************
 *
 *  Description:  Set the MemSize variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  new value of local DBG_MemSize
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void DBG_SetMemSize( unsigned int newSize )
{
	volatile unsigned int memsz = newSize;
	
	if ( memsz > DBG_MEM_SZ_MAX ) {
		return;
	}

	DBG_MemSize = memsz;
	return;
}

/****************************** DBG_GetMemP *********************************
 *
 *  Description:  Get the MemP variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  value of local DBG_MemSize
 *  Globals....:  -
 ****************************************************************************/
unsigned char *DBG_GetMemP(void)
{
	return DBG_MemP;
}

/****************************** DBG_SetMemP *********************************
 *
 *  Description:  Set the MemP variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  new value of local DBG_MemSize, only checked for NULL!
 *  Output.....:  OK or ERROR if NULL pointer passed
 *  Globals....:  -
 ****************************************************************************/
STATUS DBG_SetMemP(unsigned char *memP)
{
	unsigned char *mP = memP;

	if (NULL == mP) {
		return ERROR;
	}

	DBG_MemP = mP;
	return OK;
}

/****************************** DBG_GetWriteP *********************************
 *
 *  Description:  Get the WriteP variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  value of local DBG_MemSize
 *  Globals....:  -
 ****************************************************************************/
unsigned char *DBG_GetWriteP(void)
{
	return DBG_WriteP;
}

/****************************** DBG_SetWriteP *********************************
 *
 *  Description:  Set the WriteP variable from external C modules, prevents
 *                SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  new value of local DBG_MemSize, only checked for NULL!
 *  Output.....:  OK or ERROR if NULL pointer passed
 *  Globals....:  -
 ****************************************************************************/
STATUS DBG_SetWriteP(unsigned char *WriteP)
{
	unsigned char *mP = WriteP;

	if (NULL == mP) {
		return ERROR;
	}

	DBG_WriteP = mP;
	return OK;
}

/************************** DBG_GetDoCacheFlush *****************************
 *
 *  Description:  query cache flushing variable from external C modules,
 *  			  prevents SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  current value of DBG_doCacheFlush
 *  Globals....:  -
 ****************************************************************************/
int DBG_GetDoCacheFlush(void)
{
	return !!DBG_doCacheFlush;
}

/************************** DBG_SetDoCacheFlush *****************************
 *
 *  Description:  set enable cache flushing variable from external C modules,
 *  			  prevents SDA21 linker error when accessing variable directly
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  current value of DBG_doCacheFlush
 *  Globals....:  -
 ****************************************************************************/
void DBG_SetDoCacheFlush(int dcf)
{
	volatile int cfl = !!dcf;
	DBG_doCacheFlush = cfl;
}

/****************************** DBG_MapToReservedMem *************************
 *
 *  Description:  Set the debug output to a special memory space.
 *
 *---------------------------------------------------------------------------
 *  Input......:  newMemP    new memory space
 *  Output.....:  -
 *  Globals....:  DBG_MappedToReservedMemP
 ****************************************************************************/
void DBG_MapToReservedMem( unsigned char *newMemP )
{
	DBG_MappedToReservedMemP = newMemP;
}


/****************************** DBG_Init ************************************
 *
 *  Description:  Initializes the debug output, allocates and returns debug
 *                handle.
 *
 *				  Allocates at first time (DBG_IsInit==FASLE) the DBG ring
 *                buffer.
 *                
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  name    name of debug output data module or NULL
 *				  dbgP    pointer to variable where debug handle will stored
 *
 *  Output.....:  *dbgP   debug handle
 *                return  success (0) or error code
 *  Globals....:  DBG_IsInit, DBG_MappedToReservedMemP, DBG_MemP, DBG_MemSize
 *				  DBG_Count, DBG_WriteP and DBG_ReadP, DBG_MemEndP,
 *				  DBG_OverWrtMode
 ****************************************************************************/
int32 DBG_Init
(
    char *name,
    DBG_HANDLE **dbgP
)
{
#ifdef _DIAB_TOOL
	char *dummy = name;
	dummy++;
#endif /*_DIAB_TOOL*/

    *dbgP = NULL;

	if( !DBG_IsInit )
	{
		if( DBG_MappedToReservedMemP != NULL )
		{
			DBG_MemP = DBG_MappedToReservedMemP;

			/*---------------------------------+
			| check writing to reserved memory |
			+---------------------------------*/
			*DBG_MemP = 0xAA;
			if( *DBG_MemP != 0xAA )
			{
				printErr("*** ERROR - DBG_Init() can't write top of reserved memory\n" );
		        return( ENOMEM );
			}/*if*/
			*DBG_MemP = 0x00;
			if( *DBG_MemP != 0x00 )
			{
				printErr("*** ERROR - DBG_Init() can't write top of reserved memory\n" );
		        return( ENOMEM );
			}/*if*/
			*(DBG_MemP+DBG_MemSize-1) = 0xAA;
			if( *(DBG_MemP+DBG_MemSize-1) != (u_int8)0xAA )
			{
				printErr("*** ERROR - DBG_Init() can't write end of reserved memory\n" );
		        return( ENOMEM );
			}/*if*/
			*(DBG_MemP+DBG_MemSize-1) = 0x00;
			if( *(DBG_MemP+DBG_MemSize-1) != 0x00 )
			{
				printErr("*** ERROR - DBG_Init() can't write end of reserved memory\n" );
		        return( ENOMEM );
			}/*if*/
		}
		else
		{
			DBG_MemP = malloc( DBG_MemSize );
			if( DBG_MemP == NULL )
			{
				printErr("*** ERROR - DBG_Init() can't alloc ring buffer memory\n" );
		        return( ENOMEM );
			}/*if*/

		}/*if*/
	
		/*------------------------------+
		| init the ring buffer pointers |
		+------------------------------*/
		DBG_MemEndP = DBG_MemP + DBG_MemSize;
		DBG_WriteP	= DBG_MemP;
		DBG_ReadP	= DBG_MemP;
		DBG_Count	= 0;
		DBG_OverWrtMode = 1;

	  #ifdef _SMP_COMPATIBLE_ONLY
		OSS_IrqHdlCreate( 0 /*dummy*/, 0/*dummy*/, &G_irqHdl );
	  #endif

		DBG_IsInit  = 1;
	}/*if*/
        
    *dbgP = &DBG_hdl_G;
    
    return(0);
}/*DBG_Init*/

/****************************** DBG_Exit ************************************
 *
 *  Description:  Terminate the debug output
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgP    pointer to variable where debug handle is stored
 *  Output.....:  return  0 | EINVAL
 *  Globals....:  -
 ****************************************************************************/
int32  DBG_Exit
(
    DBG_HANDLE **dbgP
)
{
    if( *dbgP == NULL )    /* valid ptr ? */
        return( EINVAL );

    *dbgP = NULL;
    return( 0 );
}/*DBG_Exit*/

/* workaround for P15 FRAM - slow down writes 
static void _memcpy( char *dst, char *src, int len )
{
	volatile int i, x=0;
	while( len--)
	{
		*dst++ = *src++;
		for(i=0; i<2; i++ )
			x = x + i;
					
	}
}
*/

/********************************* DbgAddString *****************************
 *
 *  Description: Copy string str into debug buffer
 *			   
 *	Interrupts must be masked when calling this function
 *---------------------------------------------------------------------------
 *  Input......: str	null-terminated string to copy
 *				 len	number of bytes in <str> 	
 *						if -1, call strlen()
 *  Output.....: -
 *  Globals....: DBG_Count, DBG_WriteP modified, DBG_OverWrtMode
 ****************************************************************************/
static void DbgAddString( const char *str, int len )
{
	int len1;

	if( len==-1 )
		len = strlen(str);

	DBG_Count += len;

	len1 = DBG_MemEndP - DBG_WriteP;
	if( len <= len1 )
		len1 = len; /* no wrap around */
	else
	{
		/* wrap */
		if( !DBG_OverWrtMode )
			return; /* no further write - to do overwrite with respect of read pointer */
	}
	len -= len1;

	memcpy( DBG_WriteP, str, len1 );	/* copy at most up to end of buffer */

	if( DBG_doCacheFlush )
		cacheFlush( DATA_CACHE, DBG_WriteP, len1 );
		
	if( len ) {
		/* if buffer wrapped around */
		str += len1;
		memcpy( DBG_MemP, str, len );

		if( DBG_doCacheFlush )
			cacheFlush( DATA_CACHE, DBG_MemP, len );

		DBG_WriteP = DBG_MemP + len1;
	}
	else {
		DBG_WriteP += len1;
	}

}

/*************************** DBG_Write **************************************
 *
 *  Description:  Print a debug message
 *
 *                NOTE: Floating point formatters are not supported.
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbg    debug handle
 *                *fmt   format string
 *                arg1
 *                ...
 *                argN
 *  Output.....:  return  0 | error code ( EINVAL, EMSGSIZE )
 *  Globals....:  DBG_Count, DBG_WriteP, DBG_MemEndP, DBG_MemP
 ****************************************************************************/
int32 DBG_Write
(
    DBG_HANDLE *dbg,
    char *frmt,
    ... 
)
{
    va_list argptr;
    int     length;
  #ifdef _SMP_COMPATIBLE_ONLY
    OSS_IRQ_STATE irqState;
  #else
    int     ps;
  #endif

    if( dbg == NULL )    /* valid ptr ? */
        return( EINVAL );

  #ifdef _SMP_COMPATIBLE_ONLY
  	irqState = OSS_IrqMaskR( OSS_VXWORKS_OS_HDL, G_irqHdl );  /* spinlock */
  #else
	ps = intLock();				/* mask interrupts */
  #endif

	if( DBG_WithTaskId ) {	
		if( intContext() ){
			DbgAddString( ">>> IRQ:", -1 );
		}
		else {
			length = sprintf( dbg->linebuf, "TID %08x:", taskIdSelf() );
			DbgAddString( dbg->linebuf, length );
		}
	}

	if( DBG_withTraceCount ) 
	{	
		length = sprintf( dbg->linebuf, "TC %d ", DBG_traceCount );
		DBG_traceCount++;
		DbgAddString( dbg->linebuf, length );
	}

    va_start(argptr,frmt);
    length = vsprintf(dbg->linebuf,frmt,argptr);
    va_end(argptr);

	DbgAddString( dbg->linebuf, length );

  #ifdef _SMP_COMPATIBLE_ONLY
  	OSS_IrqRestore( OSS_VXWORKS_OS_HDL, G_irqHdl, irqState );
  #else
	intUnlock( ps );			/* unmask interrupts */
  #endif

    return( 0 );
}/*DBG_Write*/

/*************************** DBG_Memdump ************************************
 *
 *  Description:  Print a formatted hex dump
 *
 *                Trailing blanks in the description string define the
 *                indention of all following debug lines.
 *
 *	Note: Doesn't work correctly when DBG_WithTaskId set
 *---------------------------------------------------------------------------
 *  Input......:  txt     description string or NULL (first line)
 *                buf     memory to dump
 *                len     nr of bytes
 *                fmt     dump format [bytes]
 *                        1=byte, 2=word, 4=long, 8=64bit 
 *  Output.....:  return  0 | error code ( EINVAL )
 *  Globals....:  -
 ****************************************************************************/
int32  DBG_Memdump
(
   DBG_HANDLE *dbg,
   char    *txt,
   void    *buf,
   u_int32 len,
   u_int32 fmt
)
{
    long i,line=1, blanks=0, n;
    char *k=txt, *k0, *kmax=(char*)buf+len;

    if( dbg == NULL )    /* valid ptr ? */
        return( EINVAL );
    
    if (txt) {
        while(*k++ == ' ')          /* count trailing blanks */
            blanks++;

        DBG_Write(dbg,"%s (%ld bytes)\n",txt,len);
    }

    for (k=k0=buf; k0<kmax; k0+=16,line++) {
        for (n=0; n<blanks; n++)    /* write trailing blanks */
            DBG_Write(dbg," ");
   
        DBG_Write(dbg,"%08x+%04x: ",(long)buf, (short)(k-(char*)buf));

        switch(fmt) {                                      /* dump hex: */
           case 8 : for (k=k0,i=0; i<16; i+=8, k+=8)       /* 64-bit aligned */
                       if (k<kmax)  DBG_Write(dbg,"%08x%08x ",
                                              *(long*)k,*(long*)k+4);
                       else         DBG_Write(dbg,"                 ");
                    break;
           case 4 : for (k=k0,i=0; i<16; i+=4, k+=4)       /* long aligned */
                       if (k<kmax)  DBG_Write(dbg,"%08x ",*(long*)k);
                       else         DBG_Write(dbg,"         ");
                    break;
           case 2 : for (k=k0,i=0; i<16; i+=2, k+=2)       /* word aligned */
                       if (k<kmax)  DBG_Write(dbg,"%04x ",*(short*)k & 0xffff);
                       else         DBG_Write(dbg,"     ");
                    break;
           default: for (k=k0,i=0; i<16; i++, k++)         /* byte aligned */
                       if (k<kmax)  DBG_Write(dbg,"%02x ",*k & 0xff);
                       else         DBG_Write(dbg,"   ");
        }

        for (k=k0,i=0; i<16 && k<kmax; i++, k++)           /* dump ascii's: */
            if ( ((u_int8)*k)>=32 && ((u_int8)*k)<=127 )
               DBG_Write(dbg,"%c", *k);
            else
               DBG_Write(dbg,".");

        DBG_Write(dbg,"\n");
    }

    return(0);
}/*DBG_Memdump*/

/****************************** DBG_DisableOverwrite *************************
 *
 *  Description: Disable overwrite mode
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: DBG_OverWrtMode
 ****************************************************************************/
void DBG_DisableOverwrite( void )
{
	DBG_OverWrtMode = 0;
}

/****************************** DBG_ShowRb **********************************
 *
 *  Description:  Displays the ring buffer informations.
 *
 *		   Note:  Not a common function. Only for use from VxWorks Shell.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void DBG_ShowRb( void )
{
	printf("DBG_MemP\t= 0x%08x\n"   , (int)DBG_MemP );	
	printf("DBG_MemSize\t= 0x%08x\n", DBG_MemSize );	
	printf("DBG_Count\t= 0x%08x\n"  , DBG_Count );	
	printf("DBG_WriteP\t= 0x%08x\n" , (int)DBG_WriteP );	
	printf("DBG_ReadP\t= 0x%08x\n\n", (int)DBG_ReadP);	
}/*DBG_ShowRb*/

/******************************* DBG_Show ************************************
 *
 *  Description:  Writes the log messages in the DBG ring buffer to DBG_OutFd.
 *                The default of DBG_OutFd is STDERR.
 *                It runs in the endless mode, if it is spawn as a task and
 *				  the parameter <crashSave> is FALSE.
 *
 *				  For dumping the logs after a crash, call DBG_Show() with
 *				  the parameter <crashSave> equal TRUE. If DBG_Init() was 
 *                called (DBG_IsInit is TRUE), set DBG_Count = DBG_MemSize +1
 *                to force to dump the whole ring buffer.
 *
 *		   Note:  Not a common function. Only for use from VxWorks Shell.
 *
 *---------------------------------------------------------------------------
 *  Input......:  crashSave
 *  Output.....:  -
 *  Globals....:  DBG_IsInit, DBG_MappedToReservedMemP, DBG_MemP, DBG_MemSize
 *				  DBG_Count, DBG_WriteP and DBG_ReadP, DBG_ShowEndless
 ****************************************************************************/
void DBG_Show( int crashSave )
{
int  size;
int  i;
char *tName;
	
	/*------------------------+
	| check if is initialized |
	+------------------------*/
	if( !DBG_IsInit )
	{
		/* initialize the ring buffer */
		if( DBG_Init( "", (void*) &i ) )
			return;

		/* free the allocated debug handle */
		DBG_Exit((void*)&i);

		if( !crashSave )
		{
			/* clear the ring buffer */
			bzero( (char*)DBG_MemP, DBG_MemSize );
		}/*if*/
	}/*if*/

	if( crashSave )
	{
		/* dump the whole buffer and stop then */
		DBG_ShowEndless = 0;
		DBG_Count = DBG_MemSize +1; /* force jump into buffer complete occupied	*/
		DBG_ReadP = DBG_MemP; /* force read at start */
		printf("force dump the complete buffer\n");
	}/*if*/


	/*------------------------+
	|  check TASK ID != SHELL |
	+------------------------*/
	/* only a spawn task runs in endless mode */
	tName = taskName( taskIdSelf());
	if( tName == NULL 
		|| !strncmp( "tShell", tName, 6 )  )
		DBG_ShowEndless = 0;
	else
		printf("DBG_Show running in loop mode - call DBG_Stop for exit.\n");
		
	printf("output redirected to DBG_OutFd = %d\n", DBG_OutFd );
	DBG_ShowRb();
		
		
	/*-------------------------+
	| check messages were lost |
	+-------------------------*/
	if( DBG_Count > DBG_MemSize )
	{
		/* buffer complete occupied	*/
		printErr("*** WARNING: DBG message buffer lost messages!\n" );
		size = DBG_MemSize;
		DBG_ReadP = DBG_WriteP;
		for( i=0; i<size; i-- )
		{
			if( *DBG_ReadP == '\n' )
			    break;
			DBG_ReadP++;
			size--;

			/* wrap around */
			if( DBG_ReadP == DBG_MemEndP )
				DBG_ReadP = DBG_MemP;
		}/*for*/
		DBG_Count=size;	
	}
	else
	{
		/* buffer not complete occupied	*/
		size = DBG_Count;
	}/*if*/

	/*-----------------+
	| flush the buffer |
	+-----------------*/
	for( i=0; i<size; i++ )
	{
		write( DBG_OutFd, (char*)(DBG_ReadP), 1 );
		DBG_ReadP++;
		/* wrap around */
		if( DBG_ReadP == DBG_MemEndP )
			DBG_ReadP = DBG_MemP;
	}/*while*/
	DBG_Count -= size;

	/*---------------+
	|  endless mode  |
	+---------------*/
	for(;DBG_ShowEndless;)
	{
		while( DBG_Count && DBG_ReadP != DBG_WriteP )
		{
			write( DBG_OutFd, (char*)(DBG_ReadP), 1 );
			DBG_ReadP++;

			DBG_Count--;
			/* wrap around */
			if( DBG_ReadP == DBG_MemEndP )
				DBG_ReadP = DBG_MemP;
		}/*while*/
		taskDelay(2);
	}/*for*/
	
	/*------------------------+
	|  restore endless mode   |
	+------------------------*/
	DBG_ShowEndless   =1;
	DBG_ShowRb();

}/*DBG_Show*/

/***************************** DBG_Stop *************************************
 *
 *  Description:  Stops the dumping of the ringbuffer by setting 
 *                DBG_ShowEndless to FALSE.
 *
 *		   Note:  Not a common function. Only for use from VxWorks Shell.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  DBG_ShowEndless
 ****************************************************************************/
void DBG_Stop( void )
{
	DBG_ShowEndless   =0;
}/*DBG_Stop*/


/***************************** DBG_Clear *************************************
 *
 *  Description:  Markes the ring buffer as empty.
 *
 *		   Note:  Not a common function. Only for use from VxWorks Shell.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  DBG_ReadP, DBG_WriteP, DBG_Count
 ****************************************************************************/
void DBG_Clear( void )
{
	DBG_ReadP = DBG_WriteP;
	DBG_Count = 0;
}/*DBG_Clear*/













