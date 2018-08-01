/***********************  I n c l u d e  -      F i l e  ************************
 *
 *        Name: dbg_vxworks.h
 *
 *     $Author: ufranke $
 *       $Date: 2010/03/12 11:27:54 $
 *   $Revision: 1.8 $
 *
 * Description: VxWorks specific definitions of the DBG lib/tool
 *				The global DBG_MappedToReservedMemP is used to write
 *				the log messages to a post mortem save buffer. This buffer
 *              is not cleared at startup and the dbg tool is able to read
 *              the buffer after a crash.
 *
 *    Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: dbg_vxworks.h,v $
 * Revision 1.8  2010/03/12 11:27:54  ufranke
 * R: linker error SDA VxWorks 6.7 PPC due to access to global variables
 * M: modifying DBG_MappedToReservedMemP, DBG_MemSize by functions
 *    DBG_MapToReservedMem() and DBG_SetMemSize()
 *
 * Revision 1.7  2006/06/30 14:36:34  ufranke
 * added
 *  + DBG_withTraceCount
 *
 * Revision 1.6  2005/09/08 14:55:27  UFranke
 * added
 *  + DBG_DisableOverwrite()
 *
 * Revision 1.5  2005/07/01 15:35:58  kp
 * +DBG_doCacheFlush
 *
 * Revision 1.4  2005/05/19 15:11:53  ufranke
 * added
 *  + DBG_Clear()
 *
 * Revision 1.3  2003/03/17 12:34:21  UFranke
 * added
 *   + prototypes DBG_Show() DBG_Stop()
 *
 * Revision 1.2  2001/07/19 09:56:13  franke
 * added export of DBG_WithTaskId
 *
 * Revision 1.1  1999/06/29 15:30:53  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _INC_DBGVX_LIB
#   define _INC_DBGVX_LIB

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern int  DBG_IsInit;
extern int  DBG_OutFd; /* write default to STDERR */
extern unsigned char *DBG_MemP;		/* ringbuffer memory */
extern unsigned char *DBG_WriteP;	/* current write pointer */
extern unsigned char *DBG_ReadP;	/* current read pointer */
extern int  DBG_WithTaskId;
extern int  DBG_doCacheFlush;
extern int  DBG_withTraceCount;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern void DBG_Show( int crashSave );
extern void DBG_Stop( void );
extern void DBG_Clear( void );
extern void DBG_DisableOverwrite( void );

extern void DBG_MapToReservedMem( unsigned char *newMemP );
extern void DBG_SetMemSize( unsigned int newSize );
extern unsigned int DBG_GetMemSize(void);
extern unsigned char *DBG_GetMemP(void);
extern STATUS DBG_SetMemP(unsigned char *memP);
extern unsigned char *DBG_GetWriteP(void);
extern STATUS DBG_SetWriteP(unsigned char *WriteP);
extern int DBG_GetDoCacheFlush(void);
extern void DBG_SetDoCacheFlush(int dcf);

#ifdef __cplusplus
    }
#endif

#endif /*_INC_DBGVX_LIB */






