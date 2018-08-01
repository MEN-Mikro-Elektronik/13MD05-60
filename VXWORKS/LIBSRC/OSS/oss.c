/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  oss.c
 *
 *      \author  UFranke
 *        $Date: 2013/10/29 15:55:54 $
 *    $Revision: 1.29 $
 *
 *	   \project  MDIS4VxWorks
 *  	 \brief  Core functions of the OSS library for VxWorks
 *
 *    \switches  PCI, DBG, _SMP_COMPATIBLE_ONLY
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: oss.c,v $
 * Revision 1.29  2013/10/29 15:55:54  ts
 * R: OS startup time on EM10A had to be reduced on customer request
 * M: make a MEN_EM10A specific fixed value OSS_MikroDelayInit
 *
 * Revision 1.28  2009/05/29 10:59:21  ufranke
 * R: UP or SMP compatible now
 * M: changed _SMP_AND_UP_COMPATIBLE to _SMP_COMPATIBLE_ONLY
 *
 * Revision 1.27  2009/03/31 09:36:58  ufranke
 * cosmetics
 *
 * Revision 1.26  2008/09/26 14:19:11  ufranke
 * R: VxWorks 6.6 SMP
 * M: SMP prepared
 *
 * Revision 1.25  2008/09/05 13:29:46  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 1.24  2008/06/03 15:35:32  ufranke
 * fixed
 *  - OSS_Delay() now tickGet() rollover save
 *    happens if OSS_Delay() will be running
 *    during a tick rollover and will be interrupted
 *    by a signal before OSS_Delay() finished
 *
 * Revision 1.23  2007/03/07 12:00:04  cs
 * added casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.22  2006/07/19 14:24:04  ufranke
 * fixed
 *  - VxWorks 6.2 OSS_MemCheck() error
 *    VxWorks 6.2 clear not so much memory as VxWorks 5.5
 *    added missing dllInit() for memory list
 *
 * Revision 1.21  2006/07/17 12:13:44  ufranke
 * cosmetics
 *
 * Revision 1.20  2006/06/08 14:11:41  ufranke
 * added
 *  + OSS_VxMikroDelayReInit() for readjusting OSS_MikroDelay() after
 *    cache enable
 *
 * Revision 1.19  2006/06/02 11:04:51  ufranke
 * cosmetics
 *
 * Revision 1.18  2005/06/29 15:14:10  UFranke
 * added
 *  + OSS_MemCheckTerm missingFree counter display
 *
 * Revision 1.17  2005/03/03 18:41:11  CSchuster
 * added empty command (;) to switch-default
 *     (avoid warning with VxWorks 6.0 when no command in default branch)
 *
 * Revision 1.16  2004/07/30 12:35:20  ufranke
 * changed
 *  - Comments for Doxygen
 *  - OSS_MemCopy optimised replaced copy byte by byte by bcopy()
 *
 * Revision 1.15  2003/06/05 15:28:56  UFranke
 * changed
 *  - removed D201 memprobe / DIAB compiler warnings
 *
 * Revision 1.14  2003/06/02 12:00:02  UFranke
 * fixed
 *   - problems with memprobe and unloaded D201 FLEX
 *
 * Revision 1.13  2002/06/18 12:20:56  UFranke
 * - fixed OSS_MapPhysToVirtAddr() vxMemProbe for D201
 *   with SW loadable PLD access - probe only X86 DBG
 *
 * Revision 1.12  2002/05/08 14:39:05  Franke
 * - fixed OSS_MapPhysToVirtAddr() vxMemProbe for M-Modules
 *   with SW loadable PLD access to 0xFE ID-EEPROM register
 *
 * Revision 1.11  2002/05/02 17:32:17  Franke
 * + vxMemProbe added to OSS_MapPhysToVirtAddr()
 *
 * Revision 1.10  2001/12/04 12:04:43  Franke
 * +OSS_MemCheck() for user called memory check
 *
 * Revision 1.9  2001/11/12 15:12:38  Franke
 * bugfix OSS_MikroDelayInit() taskUnlock() on error missing
 *        delay to less on some machines / clockRates
 *
 * Revision 1.8  2000/03/16 16:05:22  kp
 * corrected OSS_Delay to delay at least the specified time
 * removed MMU_BASIC functionality in OSS_MemChk
 *
 * Revision 1.7  1999/08/30 11:03:19  Franke
 * added parameter addrSpace in OSS_UnMapVirtAddr()
 *
 * Revision 1.6  1999/05/05 11:11:11  Franke
 * added   documentation OSS_MikroDelayInit() test@processors
 *        OSS_Exit last exit message for DBG driver
 * changed OSS_SigCreate/Remove() and OSS_SemCreate/Remove() interrupt save
 *         OSS_MapPhysToVirtAddr : parameter 'addrSpace' added
 *
 * Revision 1.5  1998/07/30 09:57:48  Franke
 * avoid problems for big endian PCI targets in function OSS_PciGetConfig()
 *       for short and byte access ( untested at PowerPC!!! )
 *
 * Revision 1.4  1998/06/27 14:23:48  Franke
 * changed   OSS_AlarmTaskPriority  = 1 (Revision 1.3 - 70)
 *           OSS_IrqMask() / OSS_IrqUnMask() get the context information
 *
 * Revision 1.3  1998/06/24 10:48:53  Franke
 * added  MEMTEST facility in debug driver
 *
 * Revision 1.2  1998/06/17 14:28:08  Franke
 * added   OSS_Init(), OSS_Exit()
 *         OSS_AlarmXXX()
 * changed OSS_BusToPhysAddr() bustype VME arg4 now splitted D00..07 addr range
 *                                                           D08..16 data width
 *                             D08..D15 always ignored
 *         OSS_BusToPhysAddr() dos'nt support data width see sysBusToLocal()
 *         MDIS 4.1 debug handling
 *
 * Revision 1.1  1998/03/10 12:08:07  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char *IdentString="OSS "
#ifdef _SMP_COMPATIBLE_ONLY
	"SMP/"
#else
	"UP/"
#endif
#ifdef _DIAB_TOOL
	"diab"
#else
	"gnu"
#endif

 " - Operating System Services: $Id: oss.c,v 1.29 2013/10/29 15:55:54 ts Exp $";
/*! \mainpage

 This is the documentation of the MEN OSS module (Operating System Services)
 for VxWorks.

 Refer to the \ref osscommonspec "OSS Common Specification" for the
 plain common specification.

 Under VxWorks, OSS is implemented as a library,
 named \b oss.a or \b oss_pci.a .

 \section natcalls Special OSS calls under VxWorks

 - \b Initialisation and termination: OSS_Init(), OSS_Exit()
 - \b Configuration routines (called by user):
    OSS_PciAddrTranslationInit(), OSS_SetIrqNum0()
 - \b Checking memory boundaries and for memory leaks:
    OSS_MemCheck(), OSS_MemCheckTerm()

*/

/*!
 \menimages
*/

/*-----------------------------------------+
|  INCLUDES                                |
+------------------------------------------*/

#include <MEN/men_typs.h>

#include <vxWorks.h>
#include <stdarg.h>
#include <sysLib.h>
#include <cacheLib.h>
#include <tickLib.h>
#include <taskLib.h>
#include <logLib.h>
#include <dllLib.h>
#include <errnoLib.h>
#include <vxLib.h>
#include <string.h>

#include <MEN/mdis_err.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev /**< */
#include <MEN/dbg.h>
#include "oss_intern.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define OSS_3TICKS          3

const u_int32 OSS_MEMCHECK_MAGIC_START = 0x12345678; /**< start pattern of a allocated memory */
const u_int32 OSS_MEMCHECK_MAGIC_END   = 0x87654321; /**< end pattern of a allocated memory */

#ifdef _SMP_COMPATIBLE_ONLY
	#define OSS_TASK_CPU_LOCK		taskCpuLock
	#define OSS_TASK_CPU_UNLOCK		taskCpuUnlock
#else
	#define OSS_TASK_CPU_LOCK		taskLock
	#define OSS_TASK_CPU_UNLOCK		taskUnlock
#endif



/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
#ifdef DBG
	DBG_HANDLE   *OSS_DbgHdl = NULL;  /**< debug handle of the OSS library */
#endif /*DBG*/

#ifndef DOXY_EXCLUDE
DBGCMD( char *OSS_ErrorStartStr = "*** "; )
DBGCMD( char *OSS_ErrorLineStr  = " (line "; )
DBGCMD( char *OSS_ErrorEndStr   = ")"; )
#endif

u_int32 OSS_DbgLev = OSS_DBG_DEFAULT; /**< current debug level of the OSS module */

u_int32 OSS_AllocCounter = 0;
u_int32 OSS_MapCounter = 0;

/** PCI geographical slot mapping
 * This array defines the mapping between geographical PCI slot numbers
 * and the corresponding PCI device number. It is used by
 * OSS_PciSlotToPciDevice(). Currently the D201 board driver will call
 * this function when PCI_BUS_SLOT is specified in the descriptor.
 *
 * It defaults to the standard mapping for CompactPCI backplanes.
 */
u_int32 OSS_PciSlot1DevNbr = 16; /**< PCI device number of slot 1 */
u_int32 OSS_PciSlot2DevNbr = 15; /**< PCI device number of slot 2 */

u_int32 OSS_MikroDelayIsInit = 0;
u_int32 OSS_MikroDelayDevider = 1;

static SEM_ID	G_dataSem = NULL;		/* global OSS data kernel semaphore */


/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static u_int32 OSS_InitCounter = 0; /**< counter for OSS_Init() OSS_Exit() calls */

static u_int32 MikroDelayStop1;
static u_int32 MikroDelayStop2;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

/*-----------------------------------------+
|  MEMTEST facility                        |
+------------------------------------------*/
#ifdef DBG
	/** a list element of the allocated memory list
	 * This structure is used if DBG switch is set.
	 * \sa OSS_MemGet(), OSS_MemFree(), OSS_MemCheckAddNode()
	 */
    typedef struct
    {
        DL_NODE node;       /**< double link list node */
        char    *addr;		/**< address of the allocated memory */
        u_int32 size;       /**< size of the allocated memory */
    }OSS_MEMCHECK_HDL;

	/** Anchor of the allocated memory list
	 * Used for memory leak and memory boundary checking.
	 * \sa OSS_MemCheck()
	 */
    DL_LIST OSS_MemCheckList;

    static void OSS_MemCheckInit( void );
    static void OSS_MemCheckAddNode( char *addr, u_int32 size );
    static void OSS_MemCheckFreeNode( char *addr, u_int32 size );
#endif /*DBG*/

/**********************************************************************/
/** Gets the pointer to ident string.
 * \copydoc oss_specification.c::OSS_Ident()
 */
char* OSS_Ident( void )
{
    return( (char*)IdentString );
}/*OSS_Ident*/


/**********************************************************************/
/** Locks the access to the OSS data via a global semaphore.
 *
 * \param  -
 *
 * \return -
 */
void _OSS_DataLock( void )
{
	if( G_dataSem == NULL )
	{
		G_dataSem = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		if( G_dataSem == NULL )
		{
			for(;;)
			{
				logMsg("*** %s semBCreate line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
				taskDelay( sysClkRateGet() *5 );
			}
		}
	}

	if( semTake( G_dataSem, WAIT_FOREVER ) != OK )
	{
		for(;;)
		{
			logMsg("*** %s semTake line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
			taskDelay( sysClkRateGet() *5 );
		}
	}
}

/**********************************************************************/
/** Unlocks the access to the OSS data via a global semaphore.
 *
 * \param  -
 *
 * \return -
 */
void _OSS_DataUnlock( void )
{
	if( G_dataSem == NULL || semGive( G_dataSem ) != OK )
	{
		for(;;)
		{
			logMsg("*** %s semGive line %d\n", (int)__FUNCTION__, __LINE__, 3,4,5,6 );
			taskDelay( sysClkRateGet() *5 );
		}
	}
}

/**********************************************************************/
/** Initializes the OSS library for VxWorks
 *
 * Initializes the debug handle and calls OSS_MikroDelayInit() if
 * OSS_InitCounter is zero. Counts up the OSS_InitCounter.
 *
 * \return 0 on success or \c ERR_OSS_xxx error code on error
 *
 * \vxworks if DBG switch set call OSS_MemCheckInit()
 *
 * \sideeffects OSS_InitCounter
 *
 * \sa OSS_Exit(), OSS_DbgHdl,
 */
int32  OSS_Init( void )
{
    DBGCMD( static const char functionName[] = "OSS_Init()"; )
    int32 retCode;

	if( OSS_InitCounter == 0 )
	{
        #ifdef DBG
        if(!DBH)
            DBGINIT((NULL,&DBH));
        #endif /*DBG*/

        OSS_DbgLev = OSS_DBG_DEFAULT;
        DBGWRT_2((DBH,"%s()\n", functionName));

        retCode = OSS_MikroDelayInit( OSS_VXWORKS_OS_HDL );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: OSS_MikroDelayInit() %s%d%s\n",
                          OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
            return( retCode );
        }/*if*/

		#ifdef DBG
            OSS_MemCheckInit();
            atexit(OSS_MemCheckTerm);
		#endif /*DBG*/

		retCode = OSS_VxKernelBridgeInit();
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: OSS_VxKernelBridgeInit() %s%d%s\n",
                          OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
            return( retCode );
        }/*if*/
    }/*if*/

    OSS_InitCounter++;
    return(ERR_SUCCESS);
}/*OSS_Init*/


/**********************************************************************/
/** Deinitializes the OSS library for VxWorks
 *
 * OSS_InitCounter will be count down.
 * Deinitializes the debug handle if OSS_InitCounter is zero.
 *
 * \return ERR_SUCCESS always
 *
 * \vxworks if DBG switch set and OSS_InitCounter is zero
 *          call OSS_MemCheckTerm()
 *
 * \sideeffects OSS_InitCounter OSS_DbgHdl
 *
 * \sa OSS_Init()
 */
int32  OSS_Exit( void )
{
    DBGCMD( static const char functionName[] = "OSS_Exit()"; )

    DBGWRT_2((DBH,"%s()\n", functionName));

	if( OSS_InitCounter > 0 )
	{
	    OSS_InitCounter--;

    	if(	OSS_InitCounter == 0 )
		{
        	#ifdef DBG
    	    	DBGWRT_ERR( ( DBH, "%s final call\n", functionName )); /* not an error but OSS_DBG_DEFAULT is ERROR LEVEL */
	        	if(DBH)
            		DBGEXIT((&DBH));
        		OSS_MemCheckTerm();
			#endif /*DBG*/
		}
	}
	else
	{
    	DBGWRT_ERR( ( DBH, "%s%s: OSS_InitCounter %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}/*if*/


    return(ERR_SUCCESS);
}/*OSS_Exit*/


/**********************************************************************/
/** Allocate memory.
 * \copydoc oss_specification.c::OSS_MemGet()
 *
 * \vxworks if DBG switch set allocate additional memory for
 *          OSS_MEMCHECK_MAGIC_START and OSS_MEMCHECK_MAGIC_END
 *          pattern
 *
 * \sideeffects increase OSS_AllocCounter
 */
void* OSS_MemGet
(
    OSS_HANDLE  *oss,
    u_int32     size,
    u_int32     *gotsizeP
)
{
    DBGCMD( static const char functionName[] = "OSS_MemGet()"; )
    void  *addr;

    if (!size)
        return(NULL);

    #ifdef DBG			        /* add space for magic start and end */
        addr = memalign( 2, size + sizeof(u_int32) + sizeof(u_int32) );
    #else
        addr = memalign( 2, size );
	#endif /*DBG*/


    if( addr != NULL )
    {
        OSS_AllocCounter++;
        *gotsizeP = size;
		#ifdef DBG
		    addr = (void*) ( (char*)addr + sizeof(u_int32) );
            OSS_MemCheckAddNode( addr, size );
		#endif /*DBG*/
        DBGWRT_2((DBH,"%s( addr=0x%08x, size=%d )\n", functionName, addr, size));
    }
    else
    {
        DBGWRT_2((DBH,"%s( size=%d )\n", functionName, size));
        DBGWRT_ERR( ( DBH, "%s%s: memalign() %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        *gotsizeP = 0;
    }/*if*/

    return(addr);
}/*OSS_MemGet*/

/**********************************************************************/
/** Free memory.
 * \copydoc oss_specification.c::OSS_MemFree()
 *
 * \vxworks if DBG switch set call OSS_MemCheckFreeNode() for bound
 *          checking
 *
 * \sideeffects decrease OSS_AllocCounter
 * \sa     OSS_MemCheckFreeNode()
 */
int32 OSS_MemFree
(
    OSS_HANDLE *oss,
    void       *addr,
    u_int32    size
)
{
    DBGCMD( static const char functionName[] = "OSS_MemFree()"; )
    DBGWRT_2((DBH,"%s( addr=0x%08x, size=%d )\n", functionName, addr, size ));

    if( size )
    {
		#ifdef DBG
            OSS_MemCheckFreeNode( addr, size );
		    addr = (void*) ( (char*)addr - sizeof(u_int32) );
		#endif /*DBG*/
        free( addr );
        addr = NULL;
        OSS_AllocCounter--;
    }
    else
    {
          DBGWRT_ERR( ( DBH, "%s%s: parameter size is zero %s%d%s\n",
                        OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    }/*if*/

    return(0);
}/*OSS_MemFree*/

/**********************************************************************/
/** Checks access permissions.
 * \copydoc oss_specification.c::OSS_MemChk()
 *
 * \vxworks returns always 0
 */
int32 OSS_MemChk
(
    OSS_HANDLE *oss,
    void       *addr,
    u_int32    size,
    int32      mode
)
{
    DBGCMD( static const char functionName[] = "OSS_MemChk()"; )

    DBGWRT_2((DBH,"%s()\n", functionName));
    DBGWRT_2( ( DBH,"    %08x..%08x\n", addr, (long) addr+size-1) );

    if( !size )
       return( 0 );

	return( 0 );				/* this function does nothing */
}/*OSS_MemChk*/


/**********************************************************************/
/** Copy memory blocks.
 * \copydoc oss_specification.c::OSS_MemCopy()
 */
void OSS_MemCopy
(
   OSS_HANDLE *oss,
   u_int32    size,
   char       *src,
   char       *dest
)
{
    DBGCMD( static const char functionName[] = "OSS_MemCopy()"; )

    DBGWRT_2((DBH,"%s()\n", functionName));

    bcopy( src, dest, size );
}/*OSS_MemCopy*/

/**********************************************************************/
/** Fill memory block.
 * \copydoc oss_specification.c::OSS_MemFill()
 */
void OSS_MemFill
(
    OSS_HANDLE *oss,
    u_int32    size,
    char       *adr,
    int8       value
)
{
    DBGCMD( static const char functionName[] = "OSS_MemFill"; )
    int8 *p = (int8*)adr;

    DBGWRT_2((DBH,"%s()\n", functionName));

    while (size--)
        *p++ = value;
}/*OSS_MemFill*/

/**********************************************************************/
/** Get string length.
 * \copydoc oss_specification.c::OSS_StrLen()
 */
u_int32 OSS_StrLen
(
    OSS_HANDLE  *oss,
    char        *string
)
{
    DBGCMD( static const char functionName[] = "OSS_StrLen"; )
    u_int32 strLen = 0;

    DBGWRT_3((DBH,"%s()\n", functionName));

    while( *string != 0 )
    {
        strLen++;
        string++;
    }/*while*/

    return( strLen );
}/*OSS_StrLen*/

/**********************************************************************/
/** Compares strings.
 * \copydoc oss_specification.c::OSS_StrCmp()
 */
int32 OSS_StrCmp
(
    OSS_HANDLE  *oss,
    char        *str1,
    char        *str2
)
{
    DBGCMD( static const char functionName[] = "OSS_StrCmp"; )
    int32  retCode = 0;
    int32  end = 0;

    DBGWRT_3((DBH,"%s()\n", functionName));

    /* klocwork id1641 - check parameters str1, str2 */
    if (str1 == NULL) {
    	DBGWRT_ERR( ( DBH, "*** OSS_Str: str1 is NULL!\n"));
    	return -1;
    }

    if (str2 == NULL) {
    	DBGWRT_ERR( ( DBH, "*** OSS_Str: str2 is NULL!\n"));
    	return -1;
    }

    do
    {
         if( *str1 != *str2 )
         {
             if( *str1 > *str2 )
                 retCode = 1;
             else
                 retCode = -1;

             break;
         }/*if*/
         if( *str1 == 0 )
            end = 1;
         else
         {
             str1++;
             str2++;
         }/*if*/
    }while( !end && !retCode );

    return( retCode );
}/*OSS_StrCmp*/

/**********************************************************************/
/** Compares strings.
 * \copydoc oss_specification.c::OSS_StrNcmp()
 */
int32 OSS_StrNcmp
(
    OSS_HANDLE  *oss,
    char        *str1,
    char        *str2,
    u_int32     nbrOfBytes
)
{
    DBGCMD( static const char functionName[] = "OSS_StrNcmp"; )
    int32    retCode = 0;
    int32    end = 0;
    u_int32  nbrOfBytesAreEqual = 1;

    DBGWRT_3((DBH,"%s()\n", functionName));

    do
    {
         if( *str1 != *str2 )
         {
             if( *str1 > *str2 )
                 retCode = 1;
             else
                 retCode = -1;
             break;
         }/*if*/
         if( *str1 == 0 || nbrOfBytesAreEqual == nbrOfBytes )
            end = 1;
         else
         {
             str1++;
             str2++;
             nbrOfBytesAreEqual++;
         }/*if*/
    }while( !end && !retCode );

    return( retCode );
}/*OSS_StrNcmp*/

/**********************************************************************/
/** Breaks string into tokens.
 * \copydoc oss_specification.c::OSS_StrTok()
 */
char* OSS_StrTok
(
    OSS_HANDLE  *oss,
    char        *string,
    char        *separator,
    char        **lastP
)
{
    DBGCMD( static const char functionName[] = "OSS_StrTok"; )
    char  *startCmpP;
    char  *helpP;
    char  *retP;
    int32 strLen1 = 0;
    int32 strLen2 = 0;
    int32 i;

    DBGWRT_3((DBH,"%s()\n", functionName));

    if( *lastP == NULL )
    {
        if( string == NULL )
            return( NULL );
        startCmpP = string;
    }
    else
    {
        startCmpP = *lastP;
    }/*if*/

    /* store the pointer to return */
    retP = startCmpP;
    *lastP = startCmpP;

    /* determine the string len */
    helpP = startCmpP;
    while( *helpP != 0 )
    {
        strLen1++;
        helpP++;
    }/*while*/

    /* determine the tok len */
    helpP = separator;
    while( *helpP != 0 )
    {
        strLen2++;
        helpP++;
    }/*while*/

    if( strLen1 < strLen2 )
    {
       return( NULL );
    }
    else
    {
        /* look for tok */
        for( i=0; i < (strLen1-strLen2); i++ )
        {
            if( !OSS_StrNcmp(oss, startCmpP, separator, strLen2 ) )
            {
                /* found */
                *startCmpP = 0;                /* insert 0 */
                *lastP = startCmpP + strLen2;  /* return nest searchP */
                return( retP );
            }/*if*/
            startCmpP++;
        }/*for*/

        return( NULL ); /* no more tokens */
    }/*if*/

}/*OSS_StrTok*/

/**********************************************************************/
/** Print to string (variable arguments).
 * \copydoc oss_specification.c::OSS_Sprintf()
 */
int32 OSS_Sprintf
(
    OSS_HANDLE  *oss,
    char *str,
    const char *fmt,
    ...
)
{
    DBGCMD( static const char functionName[] = "OSS_Sprintf"; )
    va_list argptr;
    int32   lengs;

    DBGWRT_3((DBH,"%s()\n", functionName));

    va_start(argptr,fmt);
    lengs = OSS_Vsprintf(oss,str,fmt,argptr);
    va_end(argptr);

    return(lengs);
}/*OSS_Sprintf*/

/**********************************************************************/
/** Print to string (va_list argument list).
 * \copydoc oss_specification.c::OSS_Vsprintf()
 */
int32 OSS_Vsprintf
(
    OSS_HANDLE  *oss,
    char *str,
    const char *fmt,
    va_list arg
)
{
    DBGCMD( static const char functionName[] = "OSS_Vsprintf"; )
    DBGWRT_3((DBH,"%s()\n", functionName));

    return( vsprintf(str,fmt,arg) );
}/*OSS_Vsprintf*/

/**********************************************************************/
/** Copy strings.
 * \copydoc oss_specification.c::OSS_StrCpy()
 */
void OSS_StrCpy
(
    OSS_HANDLE  *oss,
    char        *from,
    char        *to
)
{
    DBGCMD( static const char functionName[] = "OSS_StrCpy"; )

    DBGWRT_3((DBH,"%s()\n", functionName));
    while( *from )
        *to++ = *from++;
}/*OSS_StrCpy*/

/**********************************************************************/
/** Set debug level.
 * \copydoc oss_specification.c::OSS_DbgLevelSet()
 *
 * \vxworks one namespace
 *          change the debug level of OSS for all drivers
 *
 * \sideeffects OSS_DbgLev
 */
void  OSS_DbgLevelSet
(
    OSS_HANDLE *oss,
    u_int32 newLevel
)
{
    DBGCMD( static const char functionName[] = "OSS_DbgLevelSet"; )
    DBGWRT_2((DBH,"%s()\n", functionName));
    OSS_DbgLev = newLevel;
}/*OSS_DbgLevelSet*/

/**********************************************************************/
/** Get debug level.
 * \copydoc oss_specification.c::OSS_DbgLevelGet()
 *
 * \sideeffects OSS_DbgLev
 */
u_int32  OSS_DbgLevelGet
(
    OSS_HANDLE *oss
)
{
    DBGCMD( static const char functionName[] = "OSS_DbgLevelGet"; )
    DBGWRT_2((DBH,"%s()\n", functionName));
    return( OSS_DbgLev );
}/*OSS_DbgLevelGet*/

/*************************** OSS_Delay **************************************
 *
 *  Description:  Let a process sleep for a specified time.
 *                Time may rounded up to the systems minimum ticker
 *                resolution. This (rounded) time is returned.
 *
 *                NOTE: The function may not be aborted by signals !
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl  pointer to os specific struct for complicated os
 *                msec
 *
 *  Output.....:  return rounded time [msec]
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
/**********************************************************************/
/** Delay at least a milli secound.
 * \copydoc oss_specification.c::OSS_Delay()
 *
 * \sideeffects OSS_DbgLev
 */
int32 OSS_Delay
(
    OSS_HANDLE *osHdl,
    int32      msec
)
{
    DBGCMD( static const char functionName[] = "OSS_Delay"; )
    ULONG    TicksToWait;
    int		 RestTicksToWait;
    unsigned long long int StopTime;
    ULONG    CurrentTime;
    int      RealMsec;
    STATUS   RetCode;
    long long int RestTicksToWait64;
    ULONG	      StartTime;

    DBGWRT_2((DBH,"%s()\n", functionName));
    DBGWRT_2( ( DBH,"    msec=%d...\n", msec) );

    TicksToWait = 0;

    if( msec )
    {
		/*--- round time, and correct for timer inaccuracy ---*/
        TicksToWait = (msec * sysClkRateGet()) / 1000;

		if( ((TicksToWait * 1000) / sysClkRateGet()) != (u_int32)msec )
			TicksToWait++;

		TicksToWait++;

        RestTicksToWait = TicksToWait;
        if( RestTicksToWait < 0 )
        {
        	for(;;)
        	{	/* block forever */
	        	logMsg("*** %s msec %lu - RestTicksToWait %d out of range\n", (int)__FUNCTION__ , (int)msec, (int)RestTicksToWait,0,0,0 );
        		taskDelay( sysClkRateGet() * 5 );
        	}
    	}

		StartTime = tickGet();
        StopTime  = StartTime + (unsigned long long int)TicksToWait;

        do
        {
        	RetCode = taskDelay( RestTicksToWait );
        	if( RetCode != OK )
          	{
        		/* signal received or called form ISR */
        		CurrentTime = tickGet();

				RestTicksToWait64 = StopTime - (unsigned long long int)CurrentTime;
		        if( CurrentTime < StartTime )
		        	RestTicksToWait64 -= 0xffffffff; /* CurrentTime rollover adjust */
		        RestTicksToWait = (int)RestTicksToWait64;

            	if( RestTicksToWait < 1 )
            	{
                	RestTicksToWait = 0;
            	}/*if*/
           }/*if*/
        }while( RetCode != OK && RestTicksToWait );
    }/*if*/

    RealMsec = (TicksToWait * 1000) / sysClkRateGet();

    return( RealMsec );
}/*OSS_Delay*/

/**********************************************************************/
/** Readjust OSS_MikroDelay().
 *
 * \copydoc oss_specification.c::OSS_MikroDelayInit()
 *
 * \VxWorks If OSS_Init()->OSS_MikroDelayInit() was called from
 *          sysHwInit2() the adjusment was done with disabled
 *          I/D-cache. OSS_MikroDelayReInit() should be called
 *          late to adjust the system with enabled caches.
 *
 * \sa OSS_MikroDelayInit
 */
int32 OSS_VxMikroDelayReInit
(
    OSS_HANDLE *osHdl
)
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	OSS_MikroDelayIsInit = 0;
    #ifdef DBG
	    if(!DBH)
            DBGINIT((NULL,&DBH));
    #endif /*DBG*/

    DBGWRT_2((DBH,"%s()\n", functionName));

	return( OSS_MikroDelayInit( osHdl ) );
}

#ifndef DOXY_EXCLUDE
/************************ OSS_MikroDelayInit *********************************
 *
 *  Description:  Calibrates the loop counter for OSS_MikroDelay.
 *                Disables interrupts and task rescheduling.
 *                Calibrates the counter for some system ticks.
 *
 *                NOTE for VxWorks:
 *                Disables and reenables task rescheduling.
 *
 *                Tested from MC68332@20MHz to PPC603e@200MHz.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *
 *  Output.....:  0 | ERR_OSS_NO_MIKRODELAY (Overflow)
 *
 *  Globals....:  OSS_MikroDelayDevider, MikroDelayStop1, MikroDelayStop2
 *                OSS_MikroDelayIsInit
 ****************************************************************************/

int32 OSS_MikroDelayInit
(
    OSS_HANDLE *osHdl
)
{
DBGCMD( static const char functionName[] = "OSS_MikroDelayInit"; )
#ifndef MEN_EM10A
u_int32 startTicks = 0;
u_int32 currentTicks;
int     clockRate;
u_int32 microSecFor3ticks;
u_int32 diffTicks;
#endif

    #ifdef DBG
	    if(!DBH)
            DBGINIT((NULL,&DBH));
    #endif /*DBG*/

    DBGWRT_2((DBH,"%s()\n", functionName));
    if( OSS_MikroDelayIsInit )
        return( 0 );

	MikroDelayStop1 = 0xffffffff;
	MikroDelayStop2 = 0xffffffff;

#ifndef MEN_EM10A
    if( OSS_TASK_CPU_LOCK() == OK )   /* disable rescheduling un this CPU */
    {
        clockRate = sysClkRateGet();
        microSecFor3ticks =  OSS_3TICKS * 1000 * 1000 / clockRate; /* wait 3 ticks */

        MikroDelayStop1 = 2;
        MikroDelayStop2 = 2;

        /*-----------------------------------------+
        |  a little benchmark                      |
        +------------------------------------------*/
        /* start with the next tick */
        currentTicks = tickGet();
        while( currentTicks == (startTicks = tickGet()) )
        	;

        /* pre benchmark - count 1 tick */
        diffTicks = 0;
        currentTicks = tickGet();
        while( currentTicks == (startTicks = tickGet()) )
        {
             if( diffTicks < 0xffffffff )
                 diffTicks++;
             else
	            goto OVERFLOW;
             startTicks = tickGet();
        }/*while*/

		/* scale to 60 ticks per second */
        diffTicks = diffTicks * sysClkRateGet() / 60;
        /* lookup table */
        if( 0 < diffTicks && diffTicks < 0xffffff )
        {
            OSS_MikroDelayDevider = 80000 / diffTicks;
            if( !OSS_MikroDelayDevider )
                OSS_MikroDelayDevider = 1;
        }
        else
        {
			OSS_MikroDelayDevider = 1;
        }/*if*/

        /*-----------------------------------------+
        |  calibrate now                           |
        +------------------------------------------*/
        do
        {
            OSS_MikroDelay( NULL, 1 ); /* dummy call for caching */

	        /* start with the next tick */
        	currentTicks = tickGet();
        	while( currentTicks == (startTicks = tickGet()) )
        		;

            /* delay a little bit */
            OSS_MikroDelay( NULL, microSecFor3ticks ); /* try delay 3 ticks */

            /* calculate waste time */
            currentTicks = tickGet();
            diffTicks = currentTicks - startTicks;

            /* calculate counter */
            if( diffTicks < OSS_3TICKS )
            {
               if( MikroDelayStop2 < 0xffffff00 )
               {
                   MikroDelayStop2 += 1;
               }
               else
               {
                   MikroDelayStop2  = 1;
                   MikroDelayStop1 += 1;
                   if( MikroDelayStop1 > 0xffffff00 )
			            goto OVERFLOW;
               }/*if*/
            }
            else
            {
                if( (100*OSS_3TICKS) < diffTicks )
                {
                    OSS_MikroDelayDevider += 50;
                }/*if*/
                else
                {
                    if( (10*OSS_3TICKS) < diffTicks )
                    {
                        OSS_MikroDelayDevider += 5;
                    }/*if*/
                    else
                    {
                        if( (3*OSS_3TICKS) < diffTicks )
                        {
                            OSS_MikroDelayDevider += 2;
                        }/*if*/
                    }/*if*/
                }/*if*/
            }/*if*/
        }while( diffTicks < OSS_3TICKS  || (3*OSS_3TICKS) < diffTicks );

        OSS_TASK_CPU_UNLOCK();
    }
    else
    {
        DBGWRT_ERR( ( DBH, "%s%s: task(CPU)Lock() %s%d%s\n",
                      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
        return( ERR_OSS_NO_MIKRODELAY );
    }/*if*/

#else /* MEN_EM10A ts@men: was fixed here to accelerate MDIS Initialization */
	OSS_MikroDelayDevider 	= 	1;
	MikroDelayStop1			=	2;
	MikroDelayStop2			=	21;
#endif

    DBGWRT_2((DBH,"  div %d, c1 %d, c2 %d\n", OSS_MikroDelayDevider, MikroDelayStop1, MikroDelayStop2 ));
    OSS_MikroDelayIsInit = 1;
    return(0);

#ifndef MEN_EM10A
OVERFLOW:
    OSS_TASK_CPU_UNLOCK();
#endif

    DBGWRT_ERR( ( DBH, "%s%s: overflow %s%d%s\n",
                 OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));

    return( ERR_OSS_NO_MIKRODELAY );

}/*OSS_MikroDelayInit*/


/*************************** OSS_MikroDelay **********************************
 *
 *  Description:  delays at least mikroSec in a loop.
 *
 *                NOTE: Can be called by ISR.
 *
 *                NOTE: may longer if code/ data are not in cache
 *                      or other tasks have a higher priority.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl    pointer to os specific struct for complicated os
 *                mikroSec number of micro seconds to delay
 *
 *  Output.....:  0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_MikroDelay
(
    OSS_HANDLE *osHdl,
    u_int32 mikroSec
)
{
DBGCMD( static const char functionName[] = "OSS_MikroDelay"; )
volatile u_int32 loopy0, loopy1, loopy2;

    DBGWRT_2((DBH,"%s()\n", functionName));

    mikroSec = mikroSec/OSS_MikroDelayDevider;
    if( !mikroSec )
         mikroSec = 1;

    for( loopy0 = 0; loopy0 < mikroSec; loopy0++)
        for( loopy1 = 0; loopy1 < MikroDelayStop1; loopy1++)
            for( loopy2 = 0; loopy2 < MikroDelayStop2; loopy2++);


    return(0);
}/*OSS_MikroDelay*/

/*************************** OSS_TickRateGet *********************************
 *
 *  Description:  Get the current tick rate.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *
 *  Output.....:  return current ticks per second
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_TickRateGet( OSS_HANDLE *osHdl )
{
    DBGCMD( static const char functionName[] = "OSS_TickRateGet"; )
    DBGWRT_2((DBH,"%s()\n", functionName));
    return( sysClkRateGet() );
}/*OSS_TickRateGet*/

/*************************** OSS_TickGet *************************************
 *
 *  Description:  Get the current system tick.
 *
 *---------------------------------------------------------------------------
 *  Input......:  oss     pointer to os specific struct for complicated os
 *  Output.....:  return  current tick
 *  Globals....:  -
 ****************************************************************************/
u_int32 OSS_TickGet(OSS_HANDLE *oss)
{
    DBGCMD( static const char functionName[] = "OSS_TickGet"; )
    DBGWRT_2((DBH,"%s()\n", functionName));

    return( tickGet() );
}/*OSS_TickGet*/


 /************************* OSS_AssignResources *****************************
 *
 *  Description:  Assign the resources e.g. memory space, interrupts... .
 *
 *                NOTE: This function is necessary for operating systems like
 *                      Windows NT which register all the resources that a
 *                      driver need. This function should be call before
 *                      'OSS_MapPhysToVirtAddr'.
 *
 *                      Currently, only memory spaces and interrupts are
 *                      supported as resources.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                busType OSS_BUSTYPE_NONE | VME | PCI | ISA
 *                busNbr  e.g. pci bus nbr. 5
 *                resNbr  number of resources
 *                res[]   resource field
 *
 *  Output.....:  0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_AssignResources
(
    OSS_HANDLE      *osHdl,
    int32           busType,
    int32           busNbr,
    int32           resNbr,
    OSS_RESOURCES   res[]
)
{
    DBGCMD( static const char functionName[] = "OSS_AssignResources"; )
    DBGWRT_2((DBH,"%s()\n", functionName));
    return( 0 );
}/*if*/


/************************* OSS_MapPhysToVirtAddr *****************************
 *
 *  Description:  Maps the physical address to the virtual address.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                physAddr
 *                size
 *                addrSpace	 OSS_ADDRSPACE_MEM | OSS_ADDRSPACE_IO
 *                busType    OSS_BUSTYPE_NONE | VME | PCI
 *                busNbr     e.g. pci bus nbr. 5
 *
 *  Output.....:  virtAddrP
 *                return 0 | error code
 *
 *  Globals....:  OSS_MapCounter
 *
 ****************************************************************************/
int32 OSS_MapPhysToVirtAddr
(
    OSS_HANDLE *osHdl,
    void       *physAddr,
    u_int32    size,
	int32      addrSpace,
    int32      busType,
    int32      busNbr,
    void       **virtAddrP
)
{
    DBGCMD( static const char functionName[] = "OSS_MapPhysToVirtAddr"; )
    int32	retVal = 0;
    int32	val;

    DBGWRT_2((DBH,"%s()\n", functionName));

    *virtAddrP = physAddr;

	if( addrSpace == OSS_ADDRSPACE_IO )
	{
		/* don't probe */
	}
	else
	{
		switch( busType )
		{
			case OSS_BUSTYPE_VME:
				/* 16bit read should work always */
				retVal = vxMemProbe( (char*)physAddr,
									 VX_READ,
									 sizeof(short),
									 (char*) &val );
				break;

			case OSS_BUSTYPE_PCI:
			    /* removed vxMemProbe() because of problems on D201
			     * with not loaded FLEX
			     */
				break;

			case OSS_BUSTYPE_MMODULE:
				/* 16bit read should work always */
				retVal = vxMemProbe( (char*)physAddr + 0xfe,  /* ID EEPROM Register */
									 VX_READ,
									 sizeof(short),
									 (char*) &val );
				break;
			default:
			/* don't probe - we have an on board device
			 * it should be mapped by the BSP
			 * and I don't know the possible access size
			 */
				;
		}/*switch*/
	}/*if*/

	if( retVal )
	{
		retVal = ERR_OSS_NO_PERM;
	    DBGWRT_ERR( ( DBH, "%s%s: vxMemProbe( 0x%08x, VX_READ, 2, ... ) failed\n"
	                       "      not mapped or wrong address %s%d%s\n",
    	             OSS_ErrorStartStr, functionName,
    	             (int) physAddr,
    	             OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}/*if*/

    OSS_MapCounter++;

    return( retVal );
}/*OSS_MapPhysToVirtAddr*/


/************************* OSS_UnMapVirtAddr *******************************
 *
 *  Description:  Unmaps the virtual address.
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl     pointer to os specific struct for complicated os
 *                virtAddr	pointer to variable where virtual address will be stored
 *                size		size in bytes
 *                addrSpace	OSS_ADDRSPACE_MEM | OSS_ADDRSPACE_IO
 *
 *  Output.....:  0 | error code
 *                *virtAddr	virtual address
 *
 *  Globals....:  OSS_MapCounter
 *
 ****************************************************************************/
int32 OSS_UnMapVirtAddr
(
    OSS_HANDLE *osHdl,
    void       **virtAddr,
    u_int32    size,
	int32      addrSpace
)
{
    DBGCMD( static const char functionName[] = "OSS_UnMapVirtAddr"; )
    DBGWRT_2((DBH,"%s()\n", functionName));

    OSS_MapCounter--;

    /* nothing to do */
    return(0);
}/*OSS_UnMapVirtAddr*/

#ifdef DBG
/***************************** OSS_MemCheckInit ***************************
 *
 *  Description: Do nothing.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void OSS_MemCheckInit( void )
{
	dllInit( &OSS_MemCheckList );
}/*OSS_MemCheckInit*/

/***************************** OSS_MemCheckAddNode ***************************
 *
 *  Description: Add a memory node element.
 *               Insert OSS_MEMCHECK_MAGIC_START and OSS_MEMCHECK_MAGIC_END
 *				 before start and after end of allocated memory.
 *
 *---------------------------------------------------------------------------
 *  Input......: addr  address of allocated memory
 *				 size  size of allocated memory
 *  Output.....: -
 *  Globals....: OSS_MemCheckList
 ****************************************************************************/
static void OSS_MemCheckAddNode( char *addr, u_int32 size )
{
OSS_MEMCHECK_HDL *memCheckHdl;
u_int32 *magicP;

    memCheckHdl = (OSS_MEMCHECK_HDL*) malloc( sizeof(OSS_MEMCHECK_HDL) );

    if (memCheckHdl == NULL)
    	return;

    memCheckHdl->addr = addr;
    memCheckHdl->size = size;

    /* set start of memory to magic value */
    addr -= sizeof(u_int32);
    magicP = (u_int32*) addr;
    *magicP = OSS_MEMCHECK_MAGIC_START;

    /* set end of memory to magic value */
    addr += size + sizeof(u_int32);
    magicP = (u_int32*) addr;
    *magicP = OSS_MEMCHECK_MAGIC_END;

	_OSS_DataLock();
    dllAdd( &OSS_MemCheckList, &memCheckHdl->node);
	_OSS_DataUnlock();
}/*OSS_MemCheckAddNode*/

/***************************** OSS_MemCheckFreeNode *************************
 *
 *  Description: Remove a memory node element. Check node alement is valid.
 *               Check OSS_MEMCHECK_MAGIC_START and OSS_MEMCHECK_MAGIC_END
 *				 before start and after end of allocated memory.
 *               Log message for pointer underrun/overrun and not valid addr
 *               or size.
 *
 *---------------------------------------------------------------------------
 *  Input......: addr  address of allocated memory
 *				 size  size of allocated memory
 *  Output.....: -
 *  Globals....: OSS_MemCheckList
 ****************************************************************************/
static void OSS_MemCheckFreeNode( char *addr, u_int32 size )
{
OSS_MEMCHECK_HDL *memCheckHdl;
u_int32 *magicP;

    for( memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_FIRST( &OSS_MemCheckList );
         memCheckHdl != NULL;
         memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_NEXT (&memCheckHdl->node)
       )
    {
        if( memCheckHdl->addr == addr
            && memCheckHdl->size == size )
        {
            /* check magic start */
            addr -= sizeof(u_int32);
            magicP = (u_int32*) addr;
            if( *magicP != OSS_MEMCHECK_MAGIC_START )
            {
                logMsg( "*** ERROR - OSS_MemCheckFreeNode() pointer underrun at 0x%08x size=%d\n",
                        (int)addr, size,0,0,0,0 );
            }/*if*/

            /* check magic end */
            addr += size + sizeof(u_int32);
            magicP = (u_int32*) addr;
            if( *magicP != OSS_MEMCHECK_MAGIC_END )
            {
                logMsg( "*** ERROR - OSS_MemCheckFreeNode() pointer overflow at 0x%08x size=%d\n",
                        (int)addr, size,0,0,0,0 );
            }/*if*/

			_OSS_DataLock();
            dllRemove( &OSS_MemCheckList, &memCheckHdl->node);
			_OSS_DataUnlock();
            free( memCheckHdl );
            return;
        }/*if*/
    }/*for*/
    logMsg( "*** ERROR - OSS_MemCheckFreeNode() at 0x%08x size=%d\n",(int)addr, size,0,0,0,0 );
}/*OSS_MemCheckFreeNode*/

/****************************** OSS_MemCheckTerm ****************************
 *
 *  Description: Check empty memory list.
 *               Log message for not freed list elements.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: OSS_MemCheckList
 ****************************************************************************/
void OSS_MemCheckTerm( void )
{
	OSS_MEMCHECK_HDL *memCheckHdl;
	int missingFree = 0;

    if( dllCount( &OSS_MemCheckList ) != 0 )
    {
        for( memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_FIRST( &OSS_MemCheckList );
             memCheckHdl != NULL;
             memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_NEXT (&memCheckHdl->node)
           )
        {
             logMsg( "*** ERROR - OSS_MemCheckTerm() no free at 0x%08x size=%d\n",
                     (int )memCheckHdl->addr,
                     memCheckHdl->size, 0,0,0,0 );
             missingFree++;
        }/*for*/

		if( missingFree )
             logMsg( "*** ERROR - OSS_MemCheckTerm() missing free %d\n",
                     missingFree,0,0,0,0,0 );

    }/*if*/
}/*OSS_MemCheckTerm*/


/******************************** OSS_MemCheck ******************************
 *
 *  Description: Check the memory list for over/underrun.
 *
 *---------------------------------------------------------------------------
 *  Input......: verbose
 *  Output.....: return 0 | 1 on error
 *  Globals....: OSS_MemCheckList
 ****************************************************************************/
int OSS_MemCheck( int verbose )
{
OSS_MEMCHECK_HDL *memCheckHdl;
int	error = 0;
char    *addr;
u_int32 size;
u_int32 *magicP;


    if( dllCount( &OSS_MemCheckList ) != 0 )
    {
    	for( memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_FIRST( &OSS_MemCheckList );
             memCheckHdl != NULL;
             memCheckHdl = (OSS_MEMCHECK_HDL*) DLL_NEXT (&memCheckHdl->node)
           )
        {
	        if( ( addr = memCheckHdl->addr )
    	        && (  size = memCheckHdl->size )
        	  )
        	{
            	/* check magic start */
                addr -= sizeof(u_int32);
                magicP = (u_int32*) addr;
                if( *magicP != OSS_MEMCHECK_MAGIC_START )
                {
                	error = 1;
                    logMsg( "*** ERROR - OSS_MemCheckFreeNode() pointer underrun at 0x%08x size=%d\n",
                            (int)addr, size,0,0,0,0 );
                }/*if*/

                /* check magic end */
                addr += size + sizeof(u_int32);
                magicP = (u_int32*) addr;
                if( *magicP != OSS_MEMCHECK_MAGIC_END )
                {
                	error = 1;
                    logMsg( "*** ERROR - OSS_MemCheckFreeNode() pointer overflow at 0x%08x size=%d\n",
                            (int)addr, size,0,0,0,0 );
                }/*if*/
            }/*if*/
        }/*for*/
    }/*if*/

	return( error );
}/*OSS_MemCheck*/

#endif /*DBG*/

#endif /* DOXY_EXCLUDE */




