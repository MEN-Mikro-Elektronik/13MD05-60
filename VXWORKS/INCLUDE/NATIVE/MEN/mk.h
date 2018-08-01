/***********************  I n c l u d e  -      F i l e  ************************
 *
 *        Name: mk.h
 *
 *     $Author: ufranke $
 *       $Date: 2009/03/30 16:13:55 $
 *   $Revision: 2.5 $
 *
 * Description: MDIS V4.x kernel definitions
 *
 *    Switches: MK_NUM_FILES default 75
 *              INCLUDE_MIPIOS_VX_TARGET for MIPIOS targets
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mk.h,v $
 * Revision 2.5  2009/03/30 16:13:55  ufranke
 * R: SMP PCI irq support missing
 * M: added MK_SetSmpPciIrqNumToVirtualIrqNumRtn()
 *
 * Revision 2.4  2009/02/19 10:02:54  ufranke
 * prepared
 *  + for MIPIOS
 *
 * Revision 2.3  2008/01/25 14:11:30  ufranke
 * cosmetics
 *
 * Revision 2.2  2007/07/17 09:53:00  ufranke
 * changed
 *  - MK_NUM_FILES increased default from 50 to 75
 *    MK_NUM_FILES configurable by mdis Makefile now
 *       i.e. ADDED_CFLAGS = -DMK_NUM_FILES=...
 *
 * Revision 2.1  2002/07/03 12:21:03  agromann
 * added prototypes: MK_GetIntConnectRtn, MK_GetIntEnableRtn,
 * MK_GetVmeIntEnableRtn
 *
 * Revision 2.0  2000/03/17 15:07:36  kp
 * Added prototypes for MK 2.0
 *
 * Revision 1.4  1999/08/31 12:14:30  Franke
 * MDIS 4.3
 * added MK_Show()
 * changed MK_Close() added parameter devP
 *
 * Revision 1.3  1998/06/24 10:44:13  Franke
 * changed MDIS 4.1 debug handling
 *
 * Revision 1.2  1998/03/16 09:58:55  franke
 * MK_MkHdl renamed in MK_DevHdl
 *
 * Revision 1.1  1998/03/10 12:21:39  franke
 * Added by mcvs
 *
 * Revision 1.5  1998/02/20 15:37:00  uf
 * MAX_BBIS_BOARD_NAME removed
 *
 * Revision 1.4  1998/01/26 14:49:56  uf
 * include sequence os2m.h mk.h changed
 *
 * Revision 1.3  1998/01/23 14:09:09  uf
 * PCI support added/cosmetics
 *
 * Revision 1.2  1997/12/19 15:06:05  uf
 * initial PCI support
 *
 * Revision 1.1  1997/12/12 18:26:29  uf
 * initial
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1995..98 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _INC_MK_LIB
#   define _INC_MK_LIB

#ifdef __cplusplus
    extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#ifndef MK_NUM_FILES
	#define MK_NUM_FILES		75		/* number of MDIS "files" */
#endif /* MK_NUM_FILES */

#define MK_NUM_IRQ_VECT		256		/* size of MK IRQ table */

#ifndef _LL_ENTRY_H
typedef void *LL_ENTRY;
#endif

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

/*--- user callable routines ---*/
extern void MK_SetIntConnectRtn(
	STATUS (*intConnectRtn)(VOIDFUNCPTR *vector, VOIDFUNCPTR routine, int parameter )
);
extern void MK_SetIntEnableRtn( STATUS (*intEnableRtn)(int level));
extern void MK_SetVmeIntEnableRtn( STATUS (*intEnableRtn)(int level));
extern void MK_SetSmpPciIrqNumToVirtualIrqNumRtn( STATUS (*rtnP)( int pciIrqNum, int *virtualIrqNumP ) ); /* only needed for SMP with CPU# > 1 */
extern FUNCPTR MK_GetIntConnectRtn(void);
extern FUNCPTR MK_GetIntEnableRtn(void);
extern FUNCPTR MK_GetVmeIntEnableRtn(void);
extern void MK_Show( int pathNbr );
extern void MK_ShowIrqTbl();

/*--- end of user callable routines ---*/

extern int32 MK_Init(void);
extern int32 MK_Open(   OS2M_MK_HANDLE mkHdl,
                        int32          *pathNbrP );

extern int32 MK_Close(    int32 pathNbr );
extern int32 MK_GetStat(  int32 pathNbr, int32 code, int32 *dataP );
extern int32 MK_SetStat(  int32 pathNbr, int32 code, int32 data );
extern int32 MK_Read(     int32 pathNbr, int32 *valueP );
extern int32 MK_Write(    int32 pathNbr, int32 value );
extern int32 MK_GetBlock( int32 pathNbr, u_int8 *buffer, int32 size, int32 *nbrRdBytesP );
extern int32 MK_SetBlock( int32 pathNbr, u_int8 *buffer, int32 size, int32 *nbrWrBytesP );


extern int32 MK_DevHdlCreate(  u_int32           dbgLev,
                               DESC_SPEC         *llDescSpec,
                               DESC_SPEC         *bbDescSpec,
							   char              *llDrvName,
                               LL_ENTRY          *llEntrys,
                               OS2M_MK_HANDLE    *devHdlP,
                               char				 *device
                            );

	#ifdef INCLUDE_MIPIOS_VX_TARGET
		extern int32 MK_SetLlDescForMipios( OS2M_MK_HANDLE devHdl, DESC_SPEC *llDescSpec );
	#endif /*INCLUDE_MIPIOS_VX_TARGET*/

extern int32 MK_DevHdlRemove(  u_int32           dbgLev,
                               OS2M_MK_HANDLE    *devHdlP
                            );

extern int32 MK_GetDevHdl( int32 pathNbr, OS2M_MK_HANDLE *devHdlP);


#ifdef __cplusplus
    }
#endif

#endif /*_INC_MK_LIB */





