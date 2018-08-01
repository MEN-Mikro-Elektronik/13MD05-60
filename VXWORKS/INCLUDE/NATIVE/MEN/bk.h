/***********************  I n c l u d e  -      F i l e  ************************
 *
 *        Name: bk.h
 *
 *     $Author: UFranke $
 *       $Date: 2005/06/20 17:04:19 $
 *   $Revision: 1.6 $
 *
 * Description: BBIS kernel
 *
 *    Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: bk.h,v $
 * Revision 1.6  2005/06/20 17:04:19  UFranke
 * added
 *  + BK_ConsistencyCheck()
 *
 * Revision 1.5  2005/06/20 15:01:59  UFranke
 * changed
 *  - increased BK_MAX_BBIS_BRD_NAME for
 *    longer CHAMELEON BBIS board names
 *
 * Revision 1.4  1999/08/30 11:21:50  Franke
 * added prototype BK_Show()
 *
 * Revision 1.3  1998/06/17 16:04:59  Franke
 * change  BK_GetBBHandle() BK_UnGetBBHandle() parameter dbgLev removed
 *
 * Revision 1.2  1998/03/16 11:11:44  franke
 * brdDrvName added to struct for M_MK_BLK getstat
 *
 * Revision 1.1  1998/03/10 12:05:28  franke
 * Added by mcvs
 *
 * Revision 1.1  1998/02/20 15:34:30  uf
 * initial
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _INC_BK_LIB
#   define _INC_BK_LIB

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
#define BK_MAX_BBIS_BRD_NAME   64
typedef struct
{
    char            *bbName;                            /* base board name in system */
    DESC_SPEC		descP;								/* for consistens check only */
    char            brdDrvName[BK_MAX_BBIS_BRD_NAME];   /* board handler name        */
    u_int32         strSize;                            /* size of string    memory  */
    BBIS_ENTRY      bbHdlEntrys;                        /* base board handler entrys */
    BBIS_HANDLE     *bbHdlData;                         /* Base Board handle         */
    u_int32         irqMode;                            /* shared / exclusive irq    */
    u_int32         linkCount;                          /* number of links           */
    u_int32         memSize;                            /* size of allocated memory  */
}BK_BASE_BOARD_DATA;

typedef struct baseBoardList
{
    BK_BASE_BOARD_DATA        *bbData;
    struct baseBoardList      *before;
    struct baseBoardList      *next;
    u_int32                   memSize; /* size of allocated memory  */
}BK_BASE_BOARD_LIST_ELEMENT;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern char *BK_IdentString;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern int32 BK_GetBBHandle
(
    DESC_SPEC                  *bbDescSpec,
    char                       *boardNameInstanz,
    BK_BASE_BOARD_LIST_ELEMENT **bbDataHdlP
);

extern int32 BK_UnGetBBHandle
(
    BK_BASE_BOARD_LIST_ELEMENT **bbDataHdlP
);

extern int BK_Show( void );

#ifdef __cplusplus
    }
#endif

#endif /*_INC_BK_LIB */




