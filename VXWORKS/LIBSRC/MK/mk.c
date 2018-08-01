/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: mk.c
 *      Project: MDIS - Module Driver Interface System
 *
 *       Author: uf/kp
 *        $Date: 2014/12/02 12:48:59 $
 *    $Revision: 2.35 $
 *
 *  Description: MK - The VxWorks MDIS Kernel.
 *
 *     Required: -
 *     Switches: INCLUDE_MIPIOS_VX_TARGET for MIPIOS targets
 *               _SMP_COMPATIBLE_ONLY
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mk.c,v $
 * Revision 2.35  2014/12/02 12:48:59  channoyer
 * R: pageFault if interrupt occured during the time where MDIS is doing
 *    the un initialization of a LL device
 * M: Check if dev->llDrvHdl is still initialized
 *
 * Revision 2.34  2013/11/26 13:32:19  ts
 * R: on F50P with vxW 6.4 IRQs were not installed for IRQ vector/level 0
 * M: removed "&& irqVect" from if-clause that calls OSS_IrqHdlCreate()
 *
 * Revision 2.33  2012/09/10 16:18:13  sy
 * Introduced Macor OSS_VXBUS_SUPPORT to support multiple PCI domains
 *
 * Revision 2.32  2010/09/02 16:33:46  UFranke
 * R: MDIS4/5 compatibility
 * M: MDIS4/MDIS5 3.0
 *
 * Revision 2.31  2010/05/11 16:42:59  ufranke
 * R: DIAB linker error SDA VxWorks 6.7 PPC due to access to global variable
 * M: OSS_IrqNum0 is static now, usage of OSS_GetIrqNum0()
 *
 * Revision 2.30  2010/03/29 16:34:19  cs
 * R: generalized/enhanced exclude of RTP support
 * M: define EX-/INCLUDE_RTP in MDIS rules.mak already, parse it only here
 *
 * Revision 2.29  2010/01/28 15:47:50  ufranke
 * R: VxWorks 6.8 GNU compiler error
 * M: inserted ; after jump label LEAVE_IT
 *
 * Revision 2.28  2009/05/29 13:32:59  ufranke
 * R: MK_SetSmpPciIrqNumToVirtualIrqNumRtn() must be used if APIC is in
 *    SYMMETRIC_IO_MODE always
 * M: enabled MK_SetSmpPciIrqNumToVirtualIrqNumRtn() for PENTIUM4 always
 *
 * Revision 2.27  2009/04/01 15:10:24  ufranke
 * R: RTP support always included
 * M: added EXCLUDE_RTP
 *
 * Revision 2.26  2009/03/30 16:14:15  ufranke
 * R: SMP PCI irq support missing
 * M: added MK_SetSmpPciIrqNumToVirtualIrqNumRtn()
 *
 * Revision 2.25  2009/02/19 09:57:30  ufranke
 * added
 *  + MIPIOS test target
 *
 * Revision 2.24  2008/11/19 12:04:12  ufranke
 * R1: on not zero initialized BSPs path tbl data may be wrong
 * R2: access outside of MK_PathDataTbl[] if MK_NUM_FILES reached
 * R3: path data currentCh, io_mode may be incorrect for a !first path
 *     to a device
 * M1: cleanup path tbl data at MK_Init()
 * M2: fixed at MK_Open()
 * M3: fixed currentCh, io_mode at MK_Open()
 *
 * Revision 2.23  2008/10/06 10:56:53  ufranke
 * MDIS4/2004 2.0-beta
 *
 * Revision 2.22  2008/09/26 14:18:18  ufranke
 * R: VxWorks 6.6 SMP
 * M: SMP prepared
 *
 * Revision 2.21  2008/09/05 13:52:04  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 2.20  2008/03/20 14:32:53  cs
 * R: a) MK_Open did not delete OSS_IrqHdl when it failed
 * M: a) call OSS_IrqHdlRemove when MK_Open fails and irqHandle still valid
 *
 * Revision 2.19  2008/01/25 14:11:36  ufranke
 * MDIS4/2004 1.7
 *
 * Revision 2.18  2006/12/11 17:14:53  ufranke
 * MDIS4/2004 1.6
 *  added
 *   + BB SetIrqHandle()
 *
 * Revision 2.17  2006/07/25 15:37:17  ufranke
 * MDIS4/2004 1.5
 *
 * Revision 2.16  2006/07/17 14:50:21  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 2.15  2006/06/08 14:09:55  ufranke
 * cosmetics
 *
 * Revision 2.14  2006/06/02 10:10:44  ufranke
 * added
 *  + VxWorks 6.2 support
 * fixed
 *  - 2 open RTP problem by using OSS_VXKB_semBcreate()
 * cosmetics
 *  - MK_Lock/UnlockDevice()
 *
 * Revision 2.13  2005/12/23 11:11:11  UFRANKE
 * added
 *  + VxWorks 6.x RTP support
 * changed
 *  - shared IRQ optimization - do not put unused IRQ handlers in list
 *    irq handlers will be put into the linked list of handler
 *    at first IRQ enable only
 * fixed
 *  - getstat M_MK_BLK_REV_ID buffer size check
 *  - getstat M_MK_IRQ_ENABLE former returns with IRQ count
 *
 * Revision 2.12  2005/06/29 15:07:15  UFranke
 * release MDIS4/2004 1.1
 * added
 *  + callback mode filled
 *
 * Revision 2.11  2004/07/30 13:39:40  ufranke
 * release MDIS4/2004
 *
 * Revision 2.10  2004/05/13 14:34:59  UFranke
 * release MDIS 4.3f
 *
 * MDIS 4.3f
 * Revision 2.9  2003/07/25 15:08:18  UFranke
 * cosmetics
 *
 * Revision 2.8  2002/07/16 10:08:42  UFranke
 * added Warning if intEnable calls fail
 *
 * Revision 2.7  2002/07/03 12:20:50  agromann
 * added extern functions: MK_GetIntConnectRtn, MK_GetIntEnableRtn,
 * MK_GetVmeIntEnableRtn
 *
 * Revision 2.6  2002/02/14 17:56:51  Franke
 * bugfix in MK_Close: MK_IrqRemove after LL_Exit call
 * MDIS 4.3e
 *
 * Revision 2.5  2002/02/08 17:56:32  Franke
 * MDIS 4.3d
 * adapted for nested OSS_IrqMask/UnMask
 *
 * Revision 2.4  2001/12/06 16:52:45  Franke
 * MDIS 4.3c
 * Bug fix open/close/open sequence of PCI devices (PC-MIPs)
 *     fails sometimes with an exception at interrupt level
 * +WindView instrumentation prepared
 *
 * Revision 2.3  2001/09/03 09:26:18  kp
 * Bug fix in setstat MK_IRQ_COUNT: unindented break removed
 *
 * Revision 2.2  2000/04/18 15:49:03  kp
 * Bug fix: intConnect was called multiple times on each open
 * Added bit field MK_irqConnected to flag if MK_Irq has been connected
 *
 * Revision 2.1  2000/03/24 12:14:43  kp
 * Printout debug warning when MK_SetIntEnableRtn functions haven't been called
 *
 * Revision 2.0  2000/03/17 15:07:41  kp
 * ====== MEGA CHANGE ========
 * MK no longer BSP specific (config.h not included)
 * added support for
 *   - PCI devices
 *   - Subdevices
 *   - MACCESS for PCI config space
 * all locking modes now supported correctly
 * added central MK_Irq routine: MK now uses its own IRQ link list
 * reformatted and cleaned up source code
 * MK again a bit faster since OS2M layer only used for Open and Close
 *
 * Revision 1.8  1999/08/31 12:14:39  Franke
 * MDIS 4.3
 * change MK_IrqInstall calls sysIntEnable() for bustype VME
 *        MK_IrqRemove  workaround for pciIntDisconnect() implemented
 *        replaced pciIomapLib.h by pciIntLib.h
 * added BBIS clrMIface() call at MK_Close()
 *       MK getstats M_MK_TICKRATE,
 *       MK_Show() function
 * removed bug at debug driver - possible access to uninitialized pointer
 *         bug renamed stdLib.h to stdlib.h, vxworks.h to vxWorks.h
 *         bug MK_open llDescHdl uninitialized pointer
 *
 * Revision 1.7  1998/07/30 10:06:00  Franke
 * cosmetics
 *
 * Revision 1.6  1998/07/16 12:01:03  Franke
 * changed MK_Open() -install irq only if needed for LL-Drv or BBIS exception irq
 *         MK_Close()-disable irq on module only if needed for LL-Drv
 *
 * Revision 1.5  1998/06/24 19:39:10  Franke
 * cosmetics
 *
 * Revision 1.4  1998/06/24 10:47:51  Franke
 * added   -  in MK_IrqService() - logMsg for bbis exceptions
 * changed -  MDIS 4.1 debug handling
 *            in MK_Open() if low level driver init fails, the low level driver
 *            handle is set to NULL now.
 *            in MK_Close() now checks for a valid low level driver handle
 *            before irq will disabled on device
 *
 * Revision 1.3  1998/03/16 13:24:24  franke
 * io_mode (auto increment channel) move from devData to path_data
 *
 * Revision 1.2  1998/03/16 09:59:00  franke
 * MK_DevHdlCreate() parameter llDrvName added
 * BLK GetStat codes supported:
 *     M_MK_BLK_PHYSADDR:
 *     M_MK_BLK_VIRTADDR:
 *     M_MK_BLK_BB_HANDLER:
 *     M_MK_BLK_BRD_NAME:
 *     M_MK_BLK_BRD_NAME:
 * M_open()  get M-Module characteristic from LL-Drv and set MIface in BBIS added
 *
 * Revision 1.1  1998/03/10 12:21:50  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ***************************************************************************/

static char *MK_IdentString = "MK - MDIS Kernel: $Id: mk.c,v 2.35 2014/12/02 12:48:59 channoyer Exp $";
static char *MK_RevisionString = "=== MDIS4/MDIS5 3.0 Build " __DATE__ " " __TIME__ " "
#ifdef INCLUDE_RTP
		"with RTP support "
#else
		"without RTP support "
#endif
#ifdef _DIAB_TOOL
		"DIAB "
#else
		"GNU "
#endif
#ifdef _SMP_COMPATIBLE_ONLY
		"SMP "
#else
		"UP "
#endif
#ifdef DBG
											    "(DBG) ===";
#else
												"===";
#endif

#include <MEN/men_typs.h>   /* men specific type definitions */

#include <stdio.h>       /* std defs */

#include <stddef.h>      /* std ansi defs */

#include <vxWorks.h>
#include <stdlib.h>
#include <taskLib.h>
#include <iosLib.h>
#include <string.h>
#include <sysLib.h>
#include <intLib.h>
#include <logLib.h>
#include <semLib.h>

#include <iv.h>			/* for INUM_TO_IVEC */

#ifdef _SMP_COMPATIBLE_ONLY
	#include <vxCpuLib.h>
#endif

#include <MEN/mdis_err.h>   /* error codes */
#include <MEN/oss.h>        /* operating system services */
#include <MEN/mbuf.h>       /* mdis buffer manager */
#include <MEN/mdis_api.h>   /* set/getstat codes ...*/
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* DBG_HANDLE */
#include <MEN/desc.h>       /* descriptor manager */

#include <MEN/mdis_com.h>

#include <MEN/ll_defs.h>
#include <MEN/ll_entry.h>

#include <MEN/bb_defs.h>
#include <MEN/bb_entry.h>

#include <MEN/os2m.h>
#include <MEN/bk.h>
#include <MEN/mk.h>

#define DBG_MYLEVEL		    dev->dbgLev
#include <MEN/dbg.h>        /* debug module */
#ifdef DBG
#define  DBH dbgHdl
static   DBG_HANDLE   *dbgHdl = NULL;
#endif /*DBG*/

#include <MEN/mdis_rtp.h>

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define CH0                   0
#define MSLOT_0               0
#define BUS_0                 0

#define IRQ_DISABLE           0
#define IRQ_ENABLE            1

#define MK_MAX_ADDR_SPACES	  8

#undef INCLUDE_WINDVIEW
#ifdef INCLUDE_WINDVIEW

 #define MK_WV_EV_OPEN_FIRST      		  M_MK_OF + 0x00
 #define MK_WV_EV_OPEN_BBIS_INIT  		  M_MK_OF + 0x01
 #define MK_WV_EV_OPEN_IRQ_INSTALL		  M_MK_OF + 0x02
 #define MK_WV_EV_OPEN_LL_INIT    		  M_MK_OF + 0x03
 #define MK_WV_EV_OPEN_IRQ_ENABLE 		  M_MK_OF + 0x04
 #define MK_WV_EV_OPEN_ADD_PATH   		  M_MK_OF + 0x05

 #define MK_WV_EV_CLOSE		  M_MK_OF + 0x10
 #define MK_WV_EV_CLOSE_IRQ	  M_MK_OF + 0x11
 #define MK_WV_EV_CLOSE_LL	  M_MK_OF + 0x12
 #define MK_WV_EV_CLOSE_BB	  M_MK_OF + 0x13
 #define MK_WV_EV_CLOSE_PATH  M_MK_OF + 0x13

 #define MK_WV_EV_IRQ		  M_MK_OF + 0x60
 #define MK_WV_EV_IRQ_BB	  M_MK_OF + 0x61
 #define MK_WV_EV_IRQ_LL	  M_MK_OF + 0x62

 extern int wvEvent( int usrEv, char * buf, int size );
 #define MKWVEV( x )	wvEvent( x ,0,0 )
 #define MKWVDELAY( x )	taskDelay(sysClkRateGet()*x)   /* add delay to test critical regions */
#else
 #define MKWVEV( x )
 #define MKWVDELAY( x )
#endif /* INCLUDE_WINDVIEW */


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/* Linked IRQ list entry */
typedef struct {
	OSS_DL_NODE		node;		/* linked list node */
	u_int32			gotsize;	/* allocated memory size */
	void			*isrArg;	/* argument to pass to ISR */
} MK_ISR_NODE;


typedef struct
{
    u_int32         callBackMode;         /* 0-none 1-direct 2-indirect */
} MK_COMMON_DEVICE_DATA;  /* data necessary in NATIVE drivers too */


typedef struct
{
    OS2M_DEV_HDR    os2mHdr;              /* starts with the header also
											 known in OS2M */
    u_int32         memSize;              /* allocated memory size */

    u_int32         ch_number;            /* number of channels  */
    u_int32         dbgLev;
    MACCESS         ma[MK_MAX_ADDR_SPACES];	/* maccess mostly device adrs */

    u_int32         irqCount;
    u_int32         irqShouldBeEnabled;
    OSS_IRQ_HANDLE  *irqHandle;           /* interrupt data */
	MK_ISR_NODE		*isrNode;			  /* isr node */

    u_int32			devBusType;		      /* bustype of the device to the
											 baseboard */
    u_int32			bbisBusType;		  /* bustype of the base board VME,
											 PCI, on board */

    u_int32         link_cnt;             /* device link counter */

	MK_COMMON_DEVICE_DATA cData;

    u_int32         mSlot;                /* device slot of base board */
	u_int32			irqSrvInitFunc;		  /* board has srvinit function */
	u_int32			irqSrvExitFunc;		  /* board has srvexit function */

	u_int32			pciBusNbr;			  /* pci data */
	u_int32			pciDevNbr;
	u_int32			pciFuncNbr;
	u_int32			pciVendorId;
	u_int32			pciDeviceId;
	u_int32			pciSubSysVendorId;
	u_int32			pciSubSysId;

	u_int32			subDevOffset; 		  /* sub device offset */
    DESC_SPEC       *llDescSpec;          /* low level desc spec */
    LL_ENTRY         llDrvEntrys;         /* low level driver entrys */
    LL_HANDLE       *llDrvHdl;            /* LowLevel Driver handle */
	char            llDrvName[M_MAX_NAME];

    BK_BASE_BOARD_LIST_ELEMENT  *bbListElement;	/* handle to base board list
												   element */
    DESC_SPEC       *bbDescSpec;          /* base board desc spec */

	int32			lockMode;			  /* device lockmode */
	OSS_SEM_HANDLE	*semDev;			  /* device semaphore */

	SEM_ID			semRead;			  /* call locking semaphores */
	SEM_ID			semWrite;			  /* call locking semaphores */
	SEM_ID			semGetStat;			  /* call locking semaphores */
	SEM_ID			semSetStat;			  /* call locking semaphores */
	SEM_ID			semGetBlock;		  /* call locking semaphores */
	SEM_ID			semSetBlock;		  /* call locking semaphores */

	SEM_ID			*semChan;			  /* channel locking sem array */
	u_int32			semChanMemsize;		  /* allocated mem for semChan */

	int				mifaceSet;			  /* flags setMiface issued */
	int				initialized; 		  /* flags MK_Open finished */
} MK_DEVICE_DATA;

typedef struct
{
    u_int32         currentCh;           /* current channel */
    u_int32         io_mode;             /* auto increment...   */
    MK_DEVICE_DATA  *deviceData;
} MK_PATH_DATA;

#ifdef OSS_VXBUS_SUPPORT
/*return vxbus device ID of the PCI bus controller connecting MDIS FPGA*/
IMPORT VXB_DEVICE_ID sysGetMdisBusCtrlID(void);
#endif

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
static MK_PATH_DATA  MK_PathDataTbl[MK_NUM_FILES];/* path table */
static OSS_DL_LIST	 MK_irqTbl[MK_NUM_IRQ_VECT]; /* irq linked list headers */
static OSS_DL_LIST	 MK_boardSlotList; /* known board/slot combinations */

/* bitfield to flag if MK_Irq has ever been connected to system IRQ */
static u_int32		 MK_irqConnected[(MK_NUM_IRQ_VECT/32)+1];

/* function to connect IRQ routine. Use MK_SetIntConnectRtn to override */
static STATUS (*MK_intConnectRtn)
	(VOIDFUNCPTR *vector, VOIDFUNCPTR routine, int parameter ) = intConnect;

/* function to enable VME interrupt level, Use Use MK_SetVmeIntEnableRtn
   to set */
static STATUS (*MK_vmeIntEnableRtn)(int level) = NULL;

/* function to enable PIC interrupt level, Use Use MK_SetIntEnableRtn
   to set */
static STATUS (*MK_intEnableRtn)(int level) = NULL;

#if (CPU==PENTIUM4)
	static STATUS (*MK_smpPciIrqNumToVirtualIrqNumRtn)( int pciIrqNum, int *virtualIrqNumP );
#endif /*CPU==PENTIUM4*/
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
DBGCMD( static char errorStartStr[] = "*** "; )
DBGCMD( static char errorLineStr[]   = " (line ";     )
DBGCMD( static char errorEndStr[]   = ")\n";       )

static SEM_ID	G_dataSem = NULL;		/* global MK data kernel semaphore */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int32 MK_IrqInstall
(
    u_int32        dbgLev,
    u_int32        devBusType,
    u_int32        bbisBusType,
    int32          vectorNbr,
    int32          level,
    u_int32        irqMode,
    void           *newIsrArg,
	MK_ISR_NODE	   **isrNodeP
);

static int32 MK_IrqRemove
(
    u_int32        dbgLev,
	MK_ISR_NODE	   **isrNodeP
);

static int32 MK_BlkGetStat( MK_DEVICE_DATA *modData, int32 code,
							M_SETGETSTAT_BLOCK *blkStruct );

static void MK_IrqService( MK_DEVICE_DATA *modulData );

static void MK_Irq( int intNbr );
static int32 GetBoardParams(MK_DEVICE_DATA *dev, DESC_HANDLE *llDescHdl);
static int32 MK_MapAddrSpaces( MK_DEVICE_DATA *dev, DESC_HANDLE *llDescHdl );
static int32 CreateLockSems( MK_DEVICE_DATA *dev );
static int32 RemoveLockSems( MK_DEVICE_DATA *dev );
static int32 MK_LockDevice(
	MK_DEVICE_DATA *dev,
	SEM_ID callSem,
	int32 currCh);
static int32 MK_UnlockDevice(
	MK_DEVICE_DATA *dev,
	SEM_ID callSem,
	int32 currCh);
static int32 DevCheckIrqEnable( MK_DEVICE_DATA *dev);
static int32 DevCheckSetMIface( MK_DEVICE_DATA *dev);

#ifdef PCI
static int32 PciGetAddrReg(
	MK_DEVICE_DATA *dev,
	u_int32 pciBaseReg,
	void *addrP,
	u_int32 *sizeP);
static int32 CheckPciDev( MK_DEVICE_DATA *dev );
#endif /* PCI */

/********************************* MK_Init ***********************************
 *
 *  Description: Init MDIS kernel globals
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: returns:	currently always 0
 *  Globals....: -
 ****************************************************************************/
int32 MK_Init(void)
{
	OSS_DL_LIST *list;
	int32 i;

	/* cleanup path table for not zero initialized VxWorks systems */
	bzero( (char*) MK_PathDataTbl, sizeof(MK_PathDataTbl) );

	/*--- init irq list headers ---*/
	for( i=0,list=MK_irqTbl; i<MK_NUM_IRQ_VECT; i++,list++ )
		OSS_DL_NewList( list );

	OSS_DL_NewList( &MK_boardSlotList );

	return 0;
}

/********************************* MK_SetIntConnectRtn **********************
 *
 *  Description: Setup BSP specific routine to attach MDIS irq routines
 *
 *			   	 If this function is not called, MK will call intConnect().
 *				 You can set it to sysIntConnect for example
 *---------------------------------------------------------------------------
 *  Input......: intConnectRtn		new intConnectRtn to use
 *  Output.....: -
 *  Globals....: MK_intConnectRtn
 ****************************************************************************/
void MK_SetIntConnectRtn(
	STATUS (*intConnectRtn)
	  (VOIDFUNCPTR *vector, VOIDFUNCPTR routine, int parameter )
)
{
	MK_intConnectRtn = intConnectRtn;
}

/********************************* MK_GetIntConnectRtn **********************
 *
 *  Description: Get BSP specific routine to attach MDIS irq routines
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return       pointer MK_intConnectRtn
 *  Globals....: MK_intConnectRtn
 ****************************************************************************/
FUNCPTR MK_GetIntConnectRtn( void )
{
	return ((FUNCPTR) MK_intConnectRtn);
}

#if (CPU==PENTIUM4)
/*********************** MK_SetSmpPciIrqNumToVirtualIrqNumRtn ***************
 *
 *  Description: Setup BSP specific routine to convert PCI irqs.
 *               Only for more SMP systems with than one CPU needed.
 *
 *---------------------------------------------------------------------------
 *  Output.....: rtnP       pointer sysPciIrqNumToVirtualIrqNumRtn
 *  Output.....: -
 *  Globals....: MK_smpPciIrqNumToVirtualIrqNumRtn
 ****************************************************************************/
void MK_SetSmpPciIrqNumToVirtualIrqNumRtn
(
	STATUS (*rtnP)( int pciIrqNum, int *virtualIrqNumP )
)
{
	MK_smpPciIrqNumToVirtualIrqNumRtn = rtnP;
}

/******************* MK_GetSmpPciIrqNumToVirtualIrqNumRtn *******************
 *
 *  Description: Get BSP specific routine to convert PCI irqs.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return       pointer MK_smpPciIrqNumToVirtualIrqNumRtn
 *  Globals....: MK_smpPciIrqNumToVirtualIrqNumRtn
 ****************************************************************************/
FUNCPTR MK_GetSmpPciIrqNumToVirtualIrqNumRtn( void )
{
	return ((FUNCPTR) MK_smpPciIrqNumToVirtualIrqNumRtn );
}
#endif /*CPU==PENTIUM4*/

/********************************* MK_SetIntEnableRtn *********************
 *
 *  Description: Setup BSP specific routine to enable interrupt level
 *				 for PCI/ISA devices
 *
 *			   	 If this function is not called, MK will not call any
 *				 routine to enable interrupt levels.
 *
 *				 Typically set to sysIntEnable
 *---------------------------------------------------------------------------
 *  Input......: intEnableRtn		the routine to be called
 *  Output.....: -
 *  Globals....: MK_vmeIntEnableRtn
 ****************************************************************************/
void MK_SetIntEnableRtn( STATUS (*intEnableRtn)(int level))
{
	MK_intEnableRtn = intEnableRtn;
}

/********************************* MK_GetIntEnableRtn *********************
 *
 *  Description: Get BSP specific routine to enable interrupt level
 *				 for PCI/ISA devices
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return             pointer MK_intEnableRtn
 *  Globals....: MK_intEnableRtn
 ****************************************************************************/
FUNCPTR MK_GetIntEnableRtn( void )
{
	return ((FUNCPTR) MK_intEnableRtn);
}

/********************************* MK_SetVmeIntEnableRtn *********************
 *
 *  Description: Setup BSP specific routine to enable VME interrupt level
 *
 *			   	 If this function is not called, MK will not call any
 *				 routine to enable VME interrupt levels.
 *
 *				 Typically set to sysIntEnable
 *---------------------------------------------------------------------------
 *  Input......: intEnableRtn		the routine to be called
 *  Output.....: -
 *  Globals....: MK_vmeIntEnableRtn
 ****************************************************************************/
void MK_SetVmeIntEnableRtn( STATUS (*intEnableRtn)(int level))
{
	MK_vmeIntEnableRtn = intEnableRtn;
}

/********************************* MK_GetVmeIntEnableRtn *********************
 *
 *  Description: Get BSP specific routine to enable VME interrupt level
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: return             pointer MK_vmeIntEnableRtn
 *  Globals....: MK_vmeIntEnableRtn
 ****************************************************************************/
FUNCPTR MK_GetVmeIntEnableRtn(void)
{
	return ((FUNCPTR) MK_vmeIntEnableRtn);
}

/**************************** MK_DevHdlCreate ********************************
 *
 *  Description:  Creates and initializes the mdis kernel device handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgLev          debug level of the OS2M module
 *                llDescSpec      ll descriptor specifier
 *                bbDescSpec      bbis descriptor specifier
 *                llEntrys        pointer to ll-drv entry structure
 *                devHdlP         pointer to variable where mdis device handle
 *                                stored
 *                device          name of the device
 *
 *  Output.....: *devHdlP         mdis device handle
 *               return           0 or error code
 *
 *  Globals....: -
 *
 ****************************************************************************/
int32 MK_DevHdlCreate  /*nodoc*/
(
   u_int32           dbgLev,
   DESC_SPEC         *llDescSpec,
   DESC_SPEC         *bbDescSpec,
   char              *llDrvName,
   LL_ENTRY          *llEntrys,
   OS2M_MK_HANDLE    *devHdlP,
   char              *device
)
{
    u_int32        gotSize;
    MK_DEVICE_DATA *dev;

	*devHdlP = NULL;

    dev = (MK_DEVICE_DATA*) OSS_MemGet( OSS_VXWORKS_OS_HDL,
										sizeof(MK_DEVICE_DATA), &gotSize );
    if( dev == NULL )
    {
        return( ERR_OSS_MEM_ALLOC );
    }/*if*/

     /* fill the turkey with zero */
    OSS_MemFill(OSS_VXWORKS_OS_HDL, gotSize, (char*) dev, 0 );

    dev->memSize = gotSize;
    dev->dbgLev     = OSS_DBG_DEFAULT;
    dev->llDescSpec = llDescSpec;
    strncpy( dev->llDrvName, llDrvName, M_MAX_NAME - 1 ); /* klocwork id3154 */
    dev->llDrvName[M_MAX_NAME - 1] = (char)0;             /* klocwork 2nd id16780 */
    dev->llDrvEntrys = *llEntrys;
    dev->bbDescSpec = bbDescSpec;


	/* alloc memory for device name */
    dev->os2mHdr.devName = OSS_MemGet( OSS_VXWORKS_OS_HDL,
										   (strlen(device)+1), &gotSize );
    if( dev->os2mHdr.devName == NULL )
    {
        goto CLEANUP;
    }/*if*/

	dev->os2mHdr.devNameSize = gotSize;
    strcpy( dev->os2mHdr.devName, device );


	*devHdlP = (OS2M_MK_HANDLE*)dev;
    return( 0 );

CLEANUP:
	OSS_MemFree( OSS_VXWORKS_OS_HDL, (char*) dev, dev->memSize );
	dev = NULL;
    return( ERR_OSS_MEM_ALLOC );
}/*MK_DevHdlCreate*/

/**************************** MK_DevHdlRemove *********************************
 *
 *  Description:  Removes the mdis device handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgLev          debug level of the OS2M module
 *                devHdlP         pointer to variable where mdis device handle
 *                                stored
 *
 *  Output.....: *devHdlP         NULL
 *               return           0 or error code
 *
 *  Globals....: -
 *
 ****************************************************************************/
int32 MK_DevHdlRemove  /*nodoc*/
(
    u_int32          dbgLev,
    OS2M_MK_HANDLE   *devHdlP
)
{
    int32          retCode = 0;
    MK_DEVICE_DATA *dev;

    dev = (MK_DEVICE_DATA*) *devHdlP;
    *devHdlP = NULL;

    retCode = OSS_MemFree( OSS_VXWORKS_OS_HDL, dev->os2mHdr.devName, dev->os2mHdr.devNameSize );
    dev->os2mHdr.devName = NULL;
    if( retCode )
    {
        return( retCode );
    }/*if*/

    retCode = OSS_MemFree( OSS_VXWORKS_OS_HDL, dev, dev->memSize );
    dev = NULL;
    if( retCode )
    {
        return( retCode );
    }/*if*/


    return( retCode );
}/*MK_DevHdlRemove*/

/**************************** MK_GetDevHdl **********************************
 *
 *  Description:  Get the mdis device handle.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr         number of the path
 *                devHdlP         pointer to variable where mdis device handle
 *                                stored
 *
 *  Output.....: *devHdlP         mdis device handle | NULL
 *               return           0 or error code
 *
 *  Globals....: -
 *
 ****************************************************************************/
int32 MK_GetDevHdl( int32 pathNbr, OS2M_MK_HANDLE *devHdlP)   /*nodoc*/
{

    *devHdlP = MK_PathDataTbl[pathNbr].deviceData;
    if( *devHdlP == NULL )
        return( ERR_BAD_PATH );
    else
    	return( 0 );
}/*MK_GetDevHdl*/

/**************************** MK_Show **********************************
 *
 *  Description: Display MDIS kernel device or path table information.
 *
 *---------------------------------------------------------------------------
 *  Input......: pathNbr    path number 0..(MK_NUM_FILES-1)
 *                          or -1 for a short listing of the path table
 *
 *  Output.....: return     0 or error code
 *
 *  Globals....: MK_PathDataTbl
 *
 ****************************************************************************/
void MK_Show( int pathNbr )
{
int found,i;
MK_DEVICE_DATA *dev;

	found = 0;
	printf("\n\n=== MK Path Table ===\n");
	printf("path\tdevName\n");
	printf("----\t-------\n");
	if( pathNbr == -1 )
	{
		for( pathNbr=0; pathNbr<MK_NUM_FILES; pathNbr++ )
		{
    		dev = MK_PathDataTbl[pathNbr].deviceData;
    		if( dev )
	    	{
				printf("%3d\t\t%s\n", pathNbr, dev->os2mHdr.devName );
				found = 1;
    		}
    	}
	    if( !found )
			printf("\tno path open\n");
    }
    else
    {
   		dev = MK_PathDataTbl[pathNbr].deviceData;
   		if( dev )
    	{
			printf("%3d\t\t%s\n", pathNbr, dev->os2mHdr.devName );

			printf("  DEVICE DATA:\n" );
			printf("\tlockMode\t\t= %ld\n", dev->lockMode );
			printf("\tch number\t\t= %ld\n"   , dev->ch_number );

			for( i=0; i<MK_MAX_ADDR_SPACES; i++ )
			{
				if( dev->ma[i] == NULL )
					break;
				printf("\tmac[%d]\t\t= 0x%08lx\n", i,
					   (unsigned long)dev->ma[i] );
			}/*for*/

			printf("\tirq enabled\t\t= %ld\n" , dev->irqShouldBeEnabled );
			printf("\tirq count\t\t= %ld\n"   , dev->irqCount );
			printf("\tDEV  bus type\t\t= %ld\n", dev->devBusType );
			printf("\tBBIS bus type\t\t= %ld\n", dev->bbisBusType );
			printf("\tlink count\t\t= %ld\n"  , dev->link_cnt );
			printf("\tslot number\t\t= %ld\n" , dev->mSlot );
			printf("\tcallback mode\t\t= %ld\n" , dev->cData.callBackMode );
			printf("\tMK dbg level\t\t= 0x%08lx\n", dev->dbgLev );
			printf("\tll drv name\t\t= %s\n" , dev->llDrvName );
			printf("\tbb drv name\t\t= %s\n" ,
				   dev->bbListElement->bbData->brdDrvName );
			printf("\tll desc\t\t\t= 0x%08x\n",(int)dev->llDescSpec );
			printf("\tbb desc\t\t\t= 0x%08x\n",(int)dev->bbDescSpec );

			printf("  PATH DATA:\n" );
			printf("\tch current\t\t= %ld\n"  ,
				   MK_PathDataTbl[pathNbr].currentCh );
			printf("\tio mode\t\t\t= %ld\n"   ,
				   MK_PathDataTbl[pathNbr].io_mode );

			printf("\n\n");

   		}
   		else
			printf("\tpath %d not open\n", pathNbr );
    }/*if*/

}/*MK_Show*/

/********************************* MK_ShowIrqTbl *****************************
 *
 *  Description: Show MK interrupt table
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void MK_ShowIrqTbl(void)
{
	OSS_DL_LIST *list;
	MK_ISR_NODE *isrNode;
	MK_DEVICE_DATA *dev;

	int32 i;

	printf("==== MK IRQ TABLE (max intNbr=%d) ===\n", MK_NUM_IRQ_VECT);

	for( i=0,list=MK_irqTbl; i<MK_NUM_IRQ_VECT; i++,list++ ){

		isrNode = (MK_ISR_NODE *)list->head;
		if( MK_irqConnected[i/32] & (1L<<(i&31)) ){

			printf("IRQ %ld:\n", i );

			while( isrNode->node.next ){
				dev = (MK_DEVICE_DATA *)isrNode->isrArg;
				printf("  %s lev=%d\n", dev->os2mHdr.devName,
					    (int)OSS_IrqHdlGetLevel( dev->irqHandle ) );
				isrNode = (MK_ISR_NODE *)isrNode->node.next;
			}
		}
	}
}

#ifdef INCLUDE_MIPIOS_VX_TARGET
	/* this function will be used for MIPIOS target test only */
	int32 MK_SetLlDescForMipios
	(
	    OS2M_MK_HANDLE   devHdl,
	    DESC_SPEC		 *llDescSpec
	)
	{
	    DBGCMD( const char *functionName = __FUNCTION__; )
	    MK_DEVICE_DATA *dev = (MK_DEVICE_DATA*) devHdl;
	    int32 error = ERR_MK_ILL_PARAM;

		if( dev == NULL || llDescSpec == NULL )
		{
	        DBGWRT_ERR( ( DBH, "%s%s: NULL pointer dev/llDesc %08/%08 %s%d%s", errorStartStr, functionName,
	        				dev, llDescSpec, errorLineStr, __LINE__, errorEndStr ));
	        goto CLEANUP;
		}

		dev->llDescSpec = llDescSpec;
		error = 0;

	CLEANUP:
		return( error );
	}
#endif /*INCLUDE_MIPIOS_VX_TARGET*/

/******************************* MK_Open *************************************
 *
 *  Description:  Open path on device and return path number.
 *                If it is the first path to the device:
 *                1. gets the base board handle from bbis kernel (BK) and
 *                2. the low level driver init function is called.
 *                Increments the device link counter.
 *				  The current channel is set to 0 for this path.
 *
 *    The following descriptor keys are mandatory:
 *
 *    Descriptor key        Default          Range/Description
 *    --------------------  ---------------  -------------------------------
 *    BOARD_NAME            -				 e.g. "A201_1"
 *											 max length BK_MAX_BBIS_BRD_NAME
 *    DEVICE_SLOT           -                0..0xFFFFFFFF
 *											 e.g. 0x1000 for on board dev 0
 *
 *    The following descriptor keys are optional:
 *
 *    Descriptor key        Default          Range/Description
 *    --------------------  ---------------  -------------------------------
 *    DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  0..0xFFFFFFFF see dbg.h
 *    DEBUG_LEVEL_OSS       OSS_DBG_DEFAULT  current  value OxC0008000
 *	  DEBUG_LEVEL_MK		OSS_DBG_DEFAULT  means DBG on ERROR at IRQ
 *											 and TASK level
 *    VXWORKS_CALLBACK_MODE 1                1..2 1-indirect 2-direct call
 *                                           are currently relevant
 *											 for CANopen only
 *    IRQ_ENABLE            0                0..1 1-enables IRQ at MK_Open()
 *
 *
 *---------------------------------------------------------------------------
 *  Input......:  devHdl     mdis device handle
 *                pathNbrP   pointer to variable where path nbr stored
 *
 *  Output.....:  *pathNbrP  path nbr | -1
 *                return     0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_Open
(
    OS2M_MK_HANDLE   devHdl,
    int32            *pathNbrP
)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
    MK_DEVICE_DATA *dev = (MK_DEVICE_DATA*) devHdl;
    int32      err2, retCode=0;
    int32        (*setcall)(LL_HANDLE*, int32, int32, int32);
    int32        (*irqEnable)(BBIS_HANDLE*,u_int32,u_int32);

    int32        path;
    DESC_HANDLE  *llDescHdl = NULL;
    char         boardNameInst[BK_MAX_BBIS_BRD_NAME];
    u_int32      strLen;
    u_int32      llUseIrq;
  u_int32      irqVect;
    u_int32      irqLevel;
    u_int32      addrModeChar, dataModeChar;
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement;
    u_int32     dbgLevelDesc;
    u_int32     dbgLevelOss;

    *pathNbrP = -1;

	for( path=0; path <= MK_NUM_FILES; path++ )  /* get new path number */
	{
        if( path == MK_NUM_FILES )
            return(ERR_PATH_FULL);

        if( MK_PathDataTbl[path].deviceData == NULL )
        	break;
	}


    /* will be displayed for lnkCount > 0 */
    DBGWRT_1((DBH,"%s\n", functionName));

	/* cleanup path data */
	if( MK_PathDataTbl[path].deviceData == NULL )
	{
		bzero( (char*)&MK_PathDataTbl[path], sizeof(MK_PathDataTbl[path]) );
	}

    /*-------------------------+
    |  check device data       |
    +-------------------------*/
    if( dev->llDrvEntrys.init == NULL )            /* no jump table ? */
        return( ERR_MK_NO_LLDRV );

    /*-----------------------------------------------------------------------+
    |  first path is being opened ?                                          |
    +-----------------------------------------------------------------------*/
    if( dev->link_cnt == 0 ) /* no pathes opened */
    {
		MKWVEV(MK_WV_EV_OPEN_FIRST);
        #ifdef DBG
    	if(!DBH)
	        DBGINIT((NULL,&DBH));
        #endif /*DBG*/

        if( ( retCode = DESC_Init( dev->llDescSpec,
                                 OSS_VXWORKS_OS_HDL,
                                 &llDescHdl )
            )
          )
        {
            goto CLEANUP;
        }/*if*/


        /*--------------------------+
        |  defaults                 |
        +--------------------------*/
		dev->irqCount = 0;
        dev->initialized = FALSE;

        /*-------------------+
        |   get DEBUG LEVELs |
        +-------------------*/
        retCode = DESC_GetUInt32( llDescHdl,
                                  OSS_DBG_DEFAULT,
                                  &dbgLevelDesc,
                                  "DEBUG_LEVEL_DESC",
                                  NULL );
        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 DEBUG_LEVEL_DESC %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }
    	DESC_DbgLevelSet(llDescHdl, dbgLevelDesc);

        retCode = DESC_GetUInt32( llDescHdl,
                                  OSS_DBG_DEFAULT,
                                  &dbgLevelOss,
                                  "DEBUG_LEVEL_OSS",
                                  NULL );
        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 DEBUG_LEVEL_OSS %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }
    	OSS_DbgLevelSet(OSS_VXWORKS_OS_HDL, dbgLevelOss);

        retCode = DESC_GetUInt32( llDescHdl,
                                  OSS_DBG_DEFAULT,
                                  &dev->dbgLev,
                                  "DEBUG_LEVEL_MK" );
        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 DEBUG_LEVEL_MK %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }
        retCode = 0;

        /* if lnkCount == 0 */
        DBGWRT_1((DBH,"%s\n", functionName));

       /*--------------------+
        | link board handler  |
        +--------------------*/
		MKWVDELAY(1);
        MKWVEV(MK_WV_EV_OPEN_BBIS_INIT);
        strLen = BK_MAX_BBIS_BRD_NAME;
        if( ( retCode = DESC_GetString( llDescHdl,
                                        "",
                                        boardNameInst,
                                        &strLen,
                                        "BOARD_NAME" )
            )
          )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetString BOARD_NAME %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/


        if( ( retCode = BK_GetBBHandle(  dev->bbDescSpec,
                                         boardNameInst,
                                         &bbListElement) )
          )
        {
            DBGWRT_ERR( ( DBH, "%s%s: BK_GetBBHandle( %s ) %s%d%s",
                          errorStartStr, functionName, boardNameInst,
						  errorLineStr, __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/

        /* klocwork id3122 */
        if (bbListElement == NULL) {
            DBGWRT_ERR(( DBH, "*** %s: bbListElement == NULL\n", functionName ));
            goto CLEANUP;
        }/*if*/

        dev->bbListElement = bbListElement;

        /*------------------+
        | get callback mode |
        +------------------*/
        retCode = DESC_GetUInt32( llDescHdl, OSS_CALLBACK_MODE_INDIRECT,
								  &dev->cData.callBackMode,
                                  "VXWORKS_CALLBACK_MODE");
        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 VXWORKS_CALLBACK_MODE "
						  "%s%d%s", errorStartStr, functionName,
						  errorLineStr, __LINE__, errorEndStr ));
            goto CLEANUP;
        }

        /*-----------------+
        | get device slot  |
        +-----------------*/
        retCode = DESC_GetUInt32( llDescHdl, 0, &dev->mSlot,
                                  "DEVICE_SLOT");
        if( retCode != 0 )
        {
            DBGWRT_ERR( ( DBH, "%s%s: DESC_GetUInt32 DEVICE_SLOT %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }

		/*------------------------+
		|  Get board parameters   |
		+------------------------*/
		if( (retCode = GetBoardParams( dev, llDescHdl )))
			goto CLEANUP;

		/* for PCI devices, check if device present and vend/devId match */
		if( dev->devBusType == OSS_BUSTYPE_PCI ){

#ifdef PCI
			if( (retCode = CheckPciDev(dev)))
				goto CLEANUP;
#else
			DBGWRT_ERR((DBH,"%s%s: MDIS kernel without PCI support %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
			goto CLEANUP;
#endif /* PCI */
		}


        /*------------------------+
        | get/map addr spaces     |
        +------------------------*/
		if( (retCode = MK_MapAddrSpaces( dev, llDescHdl )))
			goto CLEANUP;


        /*-------------------------+
        | interrupt installation   |
        +-------------------------*/
		MKWVDELAY(1);
		MKWVEV(MK_WV_EV_OPEN_IRQ_INSTALL);
        retCode = DESC_GetUInt32( llDescHdl, 0, &dev->irqShouldBeEnabled,
                                  "IRQ_ENABLE");
        if( retCode != 0 && retCode != ERR_DESC_KEY_NOTFOUND ) goto CLEANUP;

        retCode = dev->llDrvEntrys.info( LL_INFO_IRQ, &llUseIrq );
        if( retCode != 0 )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL info %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }


		if( dev->devBusType != OSS_BUSTYPE_PCI ){
			/*----------------------------+
			|  Non PCI devices, ask BBIS  |
			+----------------------------*/
			retCode = bbListElement->bbData->bbHdlEntrys.cfgInfo(
				bbListElement->bbData->bbHdlData,
				BBIS_CFGINFO_IRQ,
				dev->mSlot,
				&irqVect,
				&irqLevel,
				&(bbListElement->bbData->irqMode) );

			if( retCode != 0 )
			{
				DBGWRT_ERR( ( DBH, "%s%s: BB cfgInfo BBIS_CFGINFO_IRQ %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
				goto CLEANUP;
			}
		}
#ifdef PCI
		else {
			int32 intLine;

			/*----------------------------------------+
			|  PCI device, get IRQ from config space  |
			+----------------------------------------*/
			if( (retCode = OSS_PciGetConfig(
					OSS_VXWORKS_OS_HDL,
					dev->pciBusNbr, dev->pciDevNbr,
				dev->pciFuncNbr, OSS_PCI_INTERRUPT_LINE,
				&intLine ))){


				DBGWRT_ERR( ( DBH, "%s%s: can't get PCI_INTERRUPT_LINE"
							  "%s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
				goto CLEANUP;
			}

			retCode = OSS_IrqLevelToVector( OSS_VXWORKS_OS_HDL,
											OSS_BUSTYPE_PCI,
											intLine, (int32*)&irqVect);
			if( retCode ) goto CLEANUP;
			irqLevel = intLine;
		}
#endif /* PCI */
		MKWVDELAY(1);

      if( (llUseIrq || (bbListElement->bbData->irqMode & BBIS_IRQ_EXP))) /* irq's supported ? */
        {
	  DBGWRT_2((DBH," calling OSS_IrqHdlCreate for irqVect=%d irqLevel=%d\n", irqVect, irqLevel ));

       		retCode = OSS_IrqHdlCreate( irqVect, irqLevel, &dev->irqHandle );
			if( retCode )
			{
				DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqHdlCreate %s%d%s",
						errorStartStr, functionName, errorLineStr,
					__LINE__, errorEndStr ));
                goto CLEANUP;
			}/*if*/
        }
        else
        {
	  DBGWRT_2((DBH," llUseIrq=%d or BBIS_IRQ_EXP not set. Setting irqHandle=NULL\n", llUseIrq ));
            dev->irqHandle = 0;
        }/*if*/

        /*----------------------------------------+
        |  tell BBIS the irqHandle - if supported |
        +----------------------------------------*/
		if( bbListElement->bbData->bbHdlEntrys.setIrqHandle != NULL )
		{
			retCode = bbListElement->bbData->bbHdlEntrys.setIrqHandle( bbListElement->bbData->bbHdlData, dev->irqHandle );
			if( retCode )
			{
				DBGWRT_ERR( ( DBH, "%s%s: BB: setIrqHandle %s%d%s",	errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
                goto CLEANUP;
			}/*if*/
		}

        /*---------------------------------------+
        |  set M-Module interface on base board  |
        +---------------------------------------*/
        /* get M-Module characteristic from LL-Drv */
        retCode = dev->llDrvEntrys.info( LL_INFO_HW_CHARACTER,
                                            &addrModeChar,
                                            &dataModeChar );

        if( retCode != 0 )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL info LL_INFO_HW_CHARACTER %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/


		if( DevCheckSetMIface( dev ) ){
			/* set M-Module Interface in the BBIS handler */
			retCode = bbListElement->bbData->bbHdlEntrys.setMIface(
				bbListElement->bbData->bbHdlData,
				dev->mSlot,
				addrModeChar,
				dataModeChar );

			if( retCode != 0 )
			{
				DBGWRT_ERR( ( DBH, "%s%s:  BB setMIface %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
				goto CLEANUP;
			}/*if*/
		}
		dev->mifaceSet = TRUE;

		/*------------------------------------------------+
		|  Create device lock semaphore, leave it locked  |
		+------------------------------------------------*/
		retCode = OSS_SemCreate( OSS_VXWORKS_OS_HDL, OSS_SEM_BIN, 0,
								 &dev->semDev );

        if( retCode != 0 )
        {
            DBGWRT_ERR( ( DBH, "%s%s: can't create DevSem %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/



        /*-----------------+
        | call LL-init()   |
        +-----------------*/

		MKWVEV(MK_WV_EV_OPEN_LL_INIT);
        retCode = dev->llDrvEntrys.init( dev->llDescSpec,
                        OSS_VXWORKS_OS_HDL,
                        dev->ma,
                        dev->semDev,
                        dev->irqHandle,
                        &dev->llDrvHdl );

		OSS_SemSignal( OSS_VXWORKS_OS_HDL, dev->semDev ); /* unlock dev */

        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL-Init %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            dev->llDrvHdl = NULL;
            goto CLEANUP;
        }

        if( dev->llDrvHdl == NULL )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL-Init - no handle %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            retCode = ERR_OSS_MEM_ALLOC;
            goto CLEANUP;
        }

		/*-------------------------+
		|  get channel lockmode    |
		+-------------------------*/
		retCode = dev->llDrvEntrys.info( LL_INFO_LOCKMODE, &dev->lockMode );
		if( retCode == ERR_LL_ILL_PARAM )
			dev->lockMode = LL_LOCK_CALL; /* support old low level drivers */

        /*-------------------------+
        |  get ll_drv chan count   |
        +-------------------------*/
        retCode = dev->llDrvEntrys.getStat( dev->llDrvHdl,
                                            	M_LL_CH_NUMBER,
                                            	CH0,
                                            	(int32*) &dev->ch_number );
        if( retCode )
        {
            DBGWRT_ERR( ( DBH, "%s%s: LL-Getstat M_LL_CH_NUMBER %s%d%s",
                          errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
            goto CLEANUP;
        }/*if*/


		/*-----------------------------------------+
		|  Create channel/call locking semaphores  |
		+-----------------------------------------*/
		retCode = CreateLockSems( dev );

        /*--------------------------+
        |  set path data defaults   |
        +--------------------------*/
		MK_PathDataTbl[path].currentCh	= CH0;      /* default channel 0 */
        MK_PathDataTbl[path].io_mode = M_IO_EXEC;	/* default no autoinc */

		dev->initialized = TRUE; 		 /* flag init ok for MK_IrqService */

        /*----------------------------------+
        | IRQ ENABLE if set in descriptor   |
        +----------------------------------*/

		MKWVEV(MK_WV_EV_OPEN_IRQ_ENABLE);
        if( dev->irqHandle )   /* irq's supported ? */
        {

            setcall   = dev->llDrvEntrys.setStat;
            irqEnable = bbListElement->bbData->bbHdlEntrys.irqEnable;

            /* enable/disable at board and device */
            if( dev->irqShouldBeEnabled )
            {
	            /* low level driver or base board needs irq and irqVector is ok */
    	        if( ( retCode = MK_IrqInstall( dev->dbgLev,
        	    							   dev->devBusType,
            								   dev->bbisBusType,
                	                           irqVect,
                    	                       irqLevel,
                        	                   bbListElement->bbData->irqMode,
                            	               (void*) dev,
											   &dev->isrNode) ) != 0 )
	            {
    	            DBGWRT_ERR( ( DBH, "%s%s: MK_IrqInstall %s%d%s",
        	                  errorStartStr, functionName, errorLineStr,
								  __LINE__, errorEndStr ));
                	goto CLEANUP;
            	}/*if*/

				if( DevCheckIrqEnable( dev )){
					irqEnable( bbListElement->bbData->bbHdlData,
							   dev->mSlot, IRQ_ENABLE );
				}

                setcall( dev->llDrvHdl,
                         M_MK_IRQ_ENABLE,
                         CH0,
                         IRQ_ENABLE );
            }
            else
            {
				int32 rv1=-1, rv2;

                OSS_IrqMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );

				if( DevCheckIrqEnable( dev )){
					rv1 = irqEnable( bbListElement->bbData->bbHdlData,
									 dev->mSlot, IRQ_DISABLE );
				}
				rv2 = setcall( dev->llDrvHdl,(int32)M_MK_IRQ_ENABLE,
                              CH0, IRQ_DISABLE );

				if( rv1!=0 && rv2!=0 ){
                    /* can't disable: say enabled */
                    dev->irqShouldBeEnabled = TRUE;
                }/*if*/

                OSS_IrqUnMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );
            }
        }
        else
        {
            dev->irqShouldBeEnabled = FALSE; /* not enabled */
        }
	}
	else
	{
        /*---------------------------------------------------------+
        |  set path data defaults for a second path to the device  |
        +---------------------------------------------------------*/
		MK_PathDataTbl[path].currentCh	= CH0;      	/* default channel 0 */
        MK_PathDataTbl[path].io_mode 	= M_IO_EXEC;	/* default no autoinc */
    }/*if*/

	MKWVEV(MK_WV_EV_OPEN_ADD_PATH);
    /*----------------------+
    | add path to table     |
    +----------------------*/
    MK_PathDataTbl[path].deviceData = dev;

    dev->link_cnt++;            /* increment link counter */


CLEANUP:
    /*-------------------------+
    |  llDescHdl was created   |
    +-------------------------*/
    if( llDescHdl )
    {
       err2 = DESC_Exit( &llDescHdl );

       /* store error code if no error before */
       if( !retCode )
           retCode = err2;
    }/*if*/

    /*----------------------+
    | error - clean up      |
    +----------------------*/
    if( retCode )
    {
    	/* baseboard is linked
    	   and low level handle exists */
		if( dev->bbListElement &&
		    dev->llDrvHdl )
		{
	        if( dev->link_cnt < 1 )
    	    {
        	    MK_PathDataTbl[path].currentCh = 0;
            	MK_PathDataTbl[path].deviceData = dev;
	        }/*if*/
    	    dev->link_cnt = 1;    /* say: last link */
        	MK_Close(path);  /* cleanup .. */
        }
        else
        {
			if( dev->llDrvHdl )
    	    	dev->llDrvEntrys.exit( &dev->llDrvHdl );

			if( dev->bbListElement )
				BK_UnGetBBHandle( &dev->bbListElement );
    	}/*if*/

		/* destroy irqHandle if available */
    	if( dev->irqHandle )
    	{
    		if( OSS_IrqHdlRemove( &dev->irqHandle ) )
			{
				DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqHdlRemove %s%d%s",
								errorStartStr, functionName, errorLineStr,
								__LINE__, errorEndStr ));
			}
		}

		/* cleanup semaphores */
		RemoveLockSems( dev );

		return(retCode);         /* return error */
    }/*if*/

    *pathNbrP = path;
    return( 0 );
}/*MK_Open*/


/********************************* MK_MapAddrSpaces *************************
 *
 *  Description: Map all address spaces required from device
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev		device data
 *  Output.....: return  		success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 MK_MapAddrSpaces( MK_DEVICE_DATA *dev, DESC_HANDLE *llDescHdl)
{
    DBGCMD( static char functionName[] = "MK_MapAddrSpaces"; )
    u_int32      addrMode=0, dataMode=0, requestedSize=0, availSize=0;
    u_int32      i;
	u_int32 	 nbrOfAddrSpaces, subDevOff;
	int32		 retCode = 0;
	BK_BASE_BOARD_LIST_ELEMENT *bbListElement = dev->bbListElement;

	/*--------------------+
	| get addr spaces     |
	+--------------------*/
	retCode = dev->llDrvEntrys.info( LL_INFO_ADDRSPACE_COUNT,
										 &nbrOfAddrSpaces );
	if( retCode != 0 )
		return retCode;

	if( nbrOfAddrSpaces > MK_MAX_ADDR_SPACES )
	{
		DBGWRT_ERR( ( DBH, "%s%s:  nbrOfAddrSpaces out of range %s%d%s",
					  errorStartStr, functionName, errorLineStr,
					  __LINE__, errorEndStr ));
		return ERR_MK_ILL_PARAM;
	}/*if*/

	for( i=0; i<nbrOfAddrSpaces; i++ )
	{
		/*------------------------------------------+
        |  get addr space [n] required from device  |
        |  (device addr space)                      |
        +------------------------------------------*/
		retCode = dev->llDrvEntrys.info( LL_INFO_ADDRSPACE, i,
											 &addrMode, &dataMode,
											 &requestedSize );
		if( retCode != 0 )
		{
			DBGWRT_ERR( ( DBH, "%s%s: LL info LL_INFO_ADDRSPACE %s%d%s",
						  errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
			return retCode;
		}/*if*/

		DBGWRT_2((DBH," addrSpace %d required: addrMode=0x%04x "
				  "dataMode=0x%04x size=%d\n",
				  i, addrMode, dataMode, requestedSize));

		if( addrMode == MDIS_MA_PCICFG ){
			/*------------------------------------------------+
			|  LL driver requests access to PCI config space  |
			+------------------------------------------------*/
			if( dev->devBusType != OSS_BUSTYPE_PCI ){
			    DBGWRT_ERR((DBH,"%s%s: MDIS_MA_PCICFG allowed %s%d%s"
							"on PCI devices only!\n",
							errorStartStr, functionName,
							errorLineStr, __LINE__, errorEndStr));

				return ERR_BBIS_ILL_ADDRMODE;
			}

			dev->ma[i] = (MACCESS)
				((dev->pciBusNbr << 16) |
				(dev->pciDevNbr  << 11) |
				(dev->pciFuncNbr << 8));

			goto mapped;		/* we're done */
		}

		if( dev->devBusType != OSS_BUSTYPE_PCI ){
			/*------------------------------------------------+
			|  NON PCI device, ask BBIS for address and size  |
			+------------------------------------------------*/

			/*------------------------------------------+
			|  get addr space [n] available on board    |
			|  (bus addr space)                         |
			+------------------------------------------*/
			if( (retCode = bbListElement->bbData->bbHdlEntrys.getMAddr(
				bbListElement->bbData->bbHdlData,
				dev->mSlot,
				addrMode,
				dataMode,
				(void**)&dev->ma[i],
				&availSize ))){

				DBGWRT_ERR((DBH,"%s%s: can't getMAddr space %d %s%d%s",
							errorStartStr, functionName, i, errorLineStr,
							__LINE__, errorEndStr));
				return(retCode);
			}
		}
#ifdef PCI
		else {
			/*----------------------------------------------+
			|  PCI device, query the PCI base address regs  |
			+----------------------------------------------*/
			u_int32 pciBaseReg;

			/*--- check if PCI_BASEREG_ASSIGN specified in desc ---*/
			if ((retCode = DESC_GetUInt32(llDescHdl, i, &pciBaseReg,
										"PCI_BASEREG_ASSIGN_%d", i)) &&
				retCode != ERR_DESC_KEY_NOTFOUND){

				DBGWRT_ERR((DBH,"%s%s: error getting PCI_BASEREG_ASSIGN_%d "
							"%s%d%s", errorStartStr, functionName, i,
							errorLineStr, __LINE__, errorEndStr));

				return(retCode);
			}
			DBGWRT_2((DBH,"  Using PCI base reg %d for addrspace %d\n",
					  pciBaseReg, i ));


			/*--- get size and address of PCI base reg ---*/
			if( (retCode = PciGetAddrReg( dev, pciBaseReg,
										 (void *)&dev->ma[i],
										 &availSize)))
			{
				DBGWRT_ERR((DBH,"%s%s: PciGetAddrReg %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr));
				return retCode;
			}

		}
#endif /* PCI */
		DBGWRT_2((DBH,"  available: addr=0x%08x size=%d\n",
				  dev->ma[i], availSize));

		/*------------------------------------------+
		|  Check if descriptor SUBDEVICE_OFFSET_x   |
		|  is present, add offset if so             |
		+------------------------------------------*/
		subDevOff = 0;
		if ((retCode = DESC_GetUInt32(llDescHdl, 0, &subDevOff,
									"SUBDEVICE_OFFSET_%d", i)) &&
			retCode != ERR_DESC_KEY_NOTFOUND){

			DBGWRT_ERR((DBH,"%s%s: error getting SUBDEVICE_OFFSET %s%d%s\n",
						errorStartStr, functionName, errorLineStr, __LINE__,
						errorEndStr));
			return(retCode);
		}

		if( subDevOff ){
			DBGWRT_2((DBH,"  subdevoff=0x%x\n", subDevOff ));
			dev->ma[i] =
				(void *)((char *)dev->ma[i] + subDevOff);
		}
		if( i==0 )
			dev->subDevOffset = subDevOff;

		/*------------------------------------------+
        |  check size                               |
        +------------------------------------------*/
		if ( availSize < requestedSize + subDevOff) {
			DBGWRT_ERR((DBH,"%s%s size conflict "
						"space[%d]: %s%d%s", errorStartStr, functionName, i,
						errorLineStr, __LINE__, errorEndStr ));
			DBGWRT_ERR((DBH," required=%d <==> available=%d\n",
						requestedSize, availSize));
			return(ERR_MK_ILL_MSIZE);
		}


		/* the OSS_MapPhysToVirtAddr() is for future use only
		   - eventually for Optional Virtual Mem Support */
		retCode = OSS_MapPhysToVirtAddr(
			OSS_VXWORKS_OS_HDL,
			(void*) dev->ma[i],
			availSize,
			/* !!! 30.04.99 addr space is fix */
			OSS_ADDRSPACE_MEM,
			dev->devBusType,
			BUS_0,
			(void**) &dev->ma[i]
			);

		if( retCode != 0 )
		{
			DBGWRT_ERR( ( DBH, "%s%s: OSS_MapPhysToVirtAddr %s%d%s",
						  errorStartStr, functionName, errorLineStr,
						  __LINE__, errorEndStr ));
			return retCode;
		}/*if*/
mapped:
		DBGWRT_2((DBH,"  mapped to: addr=0x%08x\n",dev->ma[i]));

	}/*for*/

	return retCode;
}

/******************************* MK_Close ************************************
 *
 *  Description:  Closes path on a MDIS device.
 *                It decrements the device link counter.
 *                If the link counter is equal zero, the last path to the
 *                device will be closed. The low level driver exit function
 *                will called then and the base board handle will be removed.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr   path number
 *
 *  Output.....:  return    0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_Close( int32 pathNbr )
{
    DBGCMD( static char functionName[] = "MK_Close"; )
    MK_PATH_DATA   *path_ptr;
    MK_DEVICE_DATA *dev;
    int32 ret;
    int32 (*exitcall)( LL_HANDLE **LLDat );
    int32 (*setcall)(LL_HANDLE*, int32, int32, int32);
    int32 (*irqEnable)(BBIS_HANDLE*,u_int32,u_int32);
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement;
    u_int32      llUseIrq;

    ret = 0;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s\n", functionName));
	MKWVEV(MK_WV_EV_CLOSE);
    /*-----------------------------------------------------------------------+
    |  last path is being closed ?                                           |
    +-----------------------------------------------------------------------*/
    if ((--dev->link_cnt) == 0)
    {
        bbListElement = dev->bbListElement;

		/*------------------------------------+
		|  lock device semaphore if present   |
		+------------------------------------*/
		if( dev->semDev )
			OSS_SemWait( OSS_VXWORKS_OS_HDL, dev->semDev,
						 OSS_SEM_WAITFOREVER );


        /*----------------------+
        |  disable interrupt    |
        +----------------------*/
        if( dev->irqHandle )  /* irq installed ? */
        {
			MKWVEV(MK_WV_EV_CLOSE_IRQ);
            dev->llDrvEntrys.info( LL_INFO_IRQ, &llUseIrq );

            setcall    = dev->llDrvEntrys.setStat;
            irqEnable  = bbListElement->bbData->bbHdlEntrys.irqEnable;

            OSS_IrqMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );
            if( dev->llDrvHdl != 0 && llUseIrq )
            {
                /* disable irq on device */
                setcall( dev->llDrvHdl,
                         (int32)M_MK_IRQ_ENABLE,
                         CH0,
                         IRQ_DISABLE );
            }/*if*/

			if( DevCheckIrqEnable(dev)){

				/* disable on board */
				irqEnable( bbListElement->bbData->bbHdlData,
						   dev->mSlot, IRQ_DISABLE );
			}
			dev->irqShouldBeEnabled = FALSE;

            OSS_IrqUnMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );
        }/*if*/

        /*----------------------+
        |  call LL-exit()       |
        +----------------------*/
        exitcall = dev->llDrvEntrys.exit;

        if( dev->llDrvHdl != 0 )
        {
			MKWVEV(MK_WV_EV_CLOSE_LL);
            ret = exitcall( &dev->llDrvHdl );
            if( ret != 0 )
            {
                return(ret);
            }/*if*/
        }/*if*/

        /*----------------------+
        |  remove interrupt     |
        +----------------------*/
        if( dev->irqHandle )  /* irq installed ? */
        {
        	if( dev->isrNode )
        	{
	            /* remove irq service from system */
    	        ret = MK_IrqRemove( dev->dbgLev, &dev->isrNode );
        	    if( ret != 0 )
            	{
                	DBGWRT_ERR( ( DBH, "%s%s: MK_IrqRemove %s%d%s",
                    	      errorStartStr, functionName, errorLineStr,
								  __LINE__, errorEndStr ));
                	return( ret );
            	}/*if*/
            }

	    	if( OSS_IrqHdlRemove( &dev->irqHandle ) )
	    	{
                DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqHdlRemove %s%d%s",
                          errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
    	    	return( ERR_MK_IRQ_REMOVE );
			}
         }/*if*/


		/*----------------------------+
		|  Remove locking semaphores  |
		+----------------------------*/
		RemoveLockSems( dev );

        /*----------------------+
        |  unlink board handler |
        +----------------------*/
        if( dev->bbListElement )
        {
			MKWVEV(MK_WV_EV_CLOSE_BB);
			if( DevCheckSetMIface( dev )){
				/* reset M-Module Interface in the BBIS handler */
				ret = bbListElement->bbData->bbHdlEntrys.clrMIface(
					bbListElement->bbData->bbHdlData,
					dev->mSlot );
				if( ret )
				{
					DBGWRT_ERR( ( DBH, "%s%s: BB clrMIface %s%d%s",
								  errorStartStr, functionName, errorLineStr,
								  __LINE__, errorEndStr ));
					return( ret );
				}/*if*/
			}
			dev->mifaceSet = FALSE;

            if( (ret = BK_UnGetBBHandle( &dev->bbListElement )
                )
              )
            {
                DBGWRT_ERR( ( DBH, "%s%s: BK_UnGetBBHandle %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                return( ret );
            }/*if*/
        }/*if*/
        #ifdef DBG
           	if(DBH)
         	    DBGEXIT((&DBH));
        #endif /*DBG*/
    }/*if*/

    /*-------------------------+
    |  remove path from table  |
    +-------------------------*/
	MKWVEV(MK_WV_EV_CLOSE_PATH);
    MK_PathDataTbl[pathNbr].deviceData = NULL; /* remove from path table */

    return(0);
}/*MK_Close*/

/******************************* MK_SetStat **********************************
 *
 *  Description:  Sets status of specified code.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr   path number
 *                code      status code
 *                data      new status value
 *
 *  Output.....:  return 0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_SetStat
(
    int32 pathNbr,
    int32 code,
    int32 data
)
{
    DBGCMD( static char functionName[] = "MK_SetStat"; )
    MK_PATH_DATA *path_ptr;
    MK_DEVICE_DATA   *dev;
    int32 success = 0;
    int32        (*irqEnable)(BBIS_HANDLE*,u_int32,u_int32);
    BK_BASE_BOARD_LIST_ELEMENT *bbListElement;
	int32   error=0, error2=0;
	u_int32 currCh;


	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1(( DBH,"%s path %d code/data 0x%x/0x%x\n", functionName, pathNbr, code, data ));

	currCh = path_ptr->currentCh;

	/*----------------------+
	| handle LL codes first |
	+----------------------*/
	if( ( M_LL_OF <= code && code <= (M_DEV_OF + 0xff) )
		|| ( M_LL_BLK_OF <= code && code <= (M_DEV_BLK_OF + 0xff) )) {

		/* lock device */
		if( (error = MK_LockDevice( dev, dev->semSetStat, currCh )))
			return error;

		DBGWRT_2((DBH,"%s LL setstat\n", functionName));
		error = dev->llDrvEntrys.setStat( dev->llDrvHdl,
										  code,
										  currCh,
										  data );
		error2 = MK_UnlockDevice( dev, dev->semSetStat, currCh );

		if( error )
		{
			DBGWRT_ERR( ( DBH, "%s%s: LL setstat error=0x%08x %s%d%s",
						  errorStartStr, functionName,
						  error,
						  errorLineStr, __LINE__, errorEndStr ));
		}/*if*/
		if( error2 && !error)
			error = error2;
		goto CLEANUP;
	}

    switch( code )
    {
        /*--------------------------------------------------------------------+
        | internal MDIS codes                                                 |
        +--------------------------------------------------------------------*/
        case M_MK_CH_CURRENT:
            if( data < 0 || ((u_int32)data >= dev->ch_number) )
            {
                /* invalid ch ? */
                error = ERR_MK_ILL_PARAM;
                goto CLEANUP;
            }/*if*/

            path_ptr->currentCh = data;                /* set new channel */
            break;
        case M_MK_IO_MODE:
            path_ptr->io_mode = data;
            break;

        case M_MK_IRQ_ENABLE  :
            bbListElement = dev->bbListElement;
            irqEnable     = bbListElement->bbData->bbHdlEntrys.irqEnable;

            if( !dev->irqHandle)                /* not installed ? */
            {
                error = ERR_MK_IRQ_ENABLE;
                goto CLEANUP;
            }/*if*/

			OS2M_DevTblLock();

            if( data == 0 )
            {    /* disable IRQ: 1) on device, 2) on board */
                OSS_IrqMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );
                success  = ( dev->llDrvEntrys.setStat( dev->llDrvHdl,
                                      (int32)M_MK_IRQ_ENABLE,
                                      path_ptr->currentCh,
                                      IRQ_DISABLE ) != 0 ?  0 : 1);

				if( DevCheckIrqEnable(dev)){
					success += ( irqEnable( bbListElement->bbData->bbHdlData,
											dev->mSlot, IRQ_DISABLE )
								 != 0 ?  0 : 1);

				}
				else success++;

                OSS_IrqUnMask( OSS_VXWORKS_OS_HDL, dev->irqHandle );
            }
            else
            {
		        if( dev->isrNode == NULL )
        		{
				   u_int32 irqVect  = OSS_IrqHdlGetVector( dev->irqHandle );
                   u_int32 irqLevel = OSS_IrqHdlGetLevel( dev->irqHandle );

		            /* low level driver or base board needs irq and irqVector is ok */
    		        if( ( error = MK_IrqInstall( dev->dbgLev,
        		    							   dev->devBusType,
            									   dev->bbisBusType,
												   irqVect,
                    		                       irqLevel,
                        		                   bbListElement->bbData->irqMode,
                            		               (void*) dev,
												   &dev->isrNode) ) != 0 )
	            	{
    	            	DBGWRT_ERR( ( DBH, "%s%s: MK_IrqInstall %s%d%s",
    	    	            	      errorStartStr, functionName, errorLineStr,
									  __LINE__, errorEndStr ));
						OS2M_DevTblUnLock();
                		goto CLEANUP;
	            	}/*if*/
            	}/*if*/

            	/* enable IRQ: 1) on board, 2) on device */
				if( DevCheckIrqEnable(dev)){
					success += ( irqEnable( bbListElement->bbData->bbHdlData,
											dev->mSlot, IRQ_ENABLE )
								 != 0 ?  0 : 1);
				}
				else
					success++;

				/*---- enable IRQ on device ---*/
				if( (error = MK_LockDevice( dev, dev->semSetStat, currCh )))
				{
    	           	DBGWRT_ERR( ( DBH, "%s%s: MK_LockDevice %s%d%s",
    	    	            	      errorStartStr, functionName, errorLineStr,
									  __LINE__, errorEndStr ));
					OS2M_DevTblUnLock();
					goto CLEANUP;
				}
                success += ( dev->llDrvEntrys.setStat( dev->llDrvHdl,
                                             (int32)M_MK_IRQ_ENABLE,
                                             path_ptr->currentCh,
                                             IRQ_ENABLE ) != 0 ?  0 : 1);
				MK_UnlockDevice( dev, dev->semSetStat, currCh );
            }/*if*/


            if (!success)                   /* no success ? */
            {
                error = ERR_MK_IRQ_ENABLE;
				OS2M_DevTblUnLock();
                goto CLEANUP;
            }/*if*/

            dev->irqShouldBeEnabled = data;	/* init enable flag */
			OS2M_DevTblUnLock();
            break;

        case M_MK_IRQ_COUNT :
            if (!dev->irqHandle)
            {
                error = ERR_MK_IRQ_ENABLE;
                goto CLEANUP;
            }/*if*/
            dev->irqCount = data;
            break;

        case M_MK_DEBUG_LEVEL:
            dev->dbgLev = data;
            break;

        case M_MK_OSS_DEBUG_LEVEL:
            OSS_DbgLevelSet( OSS_VXWORKS_OS_HDL, data );
            break;

        /*--------------------------------------------------------------------+
        | MDIS BLK, LL-DRV, BBIS set/getstats                                 |
        +--------------------------------------------------------------------*/
        default:
            /*--------------------+
            |  MDIS BLK           |
            +--------------------*/
            if( M_MK_BLK_OF <= code && code <= (M_MK_BLK_OF + 0xff) )
            {
			    DBGWRT_2((DBH,"%s MK_BlkSetStat\n", functionName));
                error = ERR_MK_UNK_CODE;
                goto CLEANUP;
            }
            /*--------------------+
            | BBIS codes          |
            +--------------------*/
            if(    ( M_BB_OF <= code && code <= (M_BRD_OF + 0xff) )
                || ( M_BB_BLK_OF <= code && code <= (M_BRD_BLK_OF + 0xff) )
              )
            {
			    DBGWRT_2((DBH,"%s BB setstat\n", functionName));
                bbListElement = dev->bbListElement;
                error = bbListElement->bbData->bbHdlEntrys.setStat(
                                  bbListElement->bbData->bbHdlData,
                                  dev->mSlot,
                                  code,
                                  data );
				if( error )
				{
           			DBGWRT_ERR( ( DBH, "%s%s: BB setstat error=0x%08x %s%d%s",
                    	errorStartStr, functionName,
						error,
						errorLineStr, __LINE__, errorEndStr ));
				}/*if*/
                goto CLEANUP;
            }
            /*--------------------+
            | unknown codes       |
            +--------------------*/
            else
            {
                DBGWRT_ERR( ( DBH, "%s%s: unknown code %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                error = ERR_MK_UNK_CODE;
                goto CLEANUP;
            }/*if*/
    }/*switch*/

CLEANUP:

    return(error);
}/*MK_SetStat*/

/******************************* MK_GetStat **********************************
 *
 *  Description:  Gets status of specified code.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr   path number
 *                code      status code
 *                dataP     pointer to variable where status data stored
 *
 *  Output.....:  *dataP    gotten status data
 *                return    0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 ****************************************************************************/
int32 MK_GetStat
(
    int32 pathNbr,
    int32 code,
    int32 *dataP
)
{
    DBGCMD( static char functionName[] = "MK_GetStat"; )
    MK_PATH_DATA     *path_ptr;
    MK_DEVICE_DATA   *dev;
    BK_BASE_BOARD_LIST_ELEMENT   *bbListElement;
	int32   error=0, error2=0;
	u_int32 currCh;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s\n", functionName));

    currCh = path_ptr->currentCh;

	/*----------------------+
	| handle LL codes first |
	+----------------------*/
	if(( M_LL_OF <= code && code <= (M_DEV_OF + 0xff) )
	   || ( M_LL_BLK_OF <= code && code <= (M_DEV_BLK_OF + 0xff))){

		/* lock device */
		if( (error = MK_LockDevice( dev, dev->semGetStat,currCh )))
			return error;

		DBGWRT_2((DBH,"%s LL getstat\n", functionName));
		error = dev->llDrvEntrys.getStat( dev->llDrvHdl,
										  code,
										  currCh,
										  dataP );
		error2 = MK_UnlockDevice( dev, dev->semGetStat, currCh );

		if( error  )
		{
			/* CALLBACK mode query ? - handled in MK only */
			if( code == M_LL_BLK_CALLBACK )
			{
				DBGWRT_2((DBH,"%s MK_BlkGetStat "
						  "M_LL_BLK_CALLBACK\n", functionName));
				error = MK_BlkGetStat( dev, code,
									   (M_SETGETSTAT_BLOCK*)dataP);
				if( error )
					DBGWRT_ERR( ( DBH, "%s%s: MK_BlkGetStat "
								  "M_LL_BLK_CALLBACK %s%d%s",
								  errorStartStr, functionName,
								  errorLineStr, __LINE__,
								  errorEndStr ));
			}
			else
			{
				DBGWRT_ERR( ( DBH, "%s%s: LL getstat %s%d%s",
							  errorStartStr, functionName,
							  errorLineStr, __LINE__,errorEndStr));
			}/*if*/
		}/*if*/

		if( error2 && !error)
			error = error2;

	}
	else {
		switch( code )
		{
		/*---------------------------------------------------------------+
		| internal MDIS codes                                            |
		+---------------------------------------------------------------*/
        case M_MK_NBR_ADDR_SPACE:
			error = dev->llDrvEntrys.info( LL_INFO_ADDRSPACE_COUNT,
										   dataP );
			break;
        case M_MK_CH_CURRENT: *dataP = path_ptr->currentCh;    break;
        case M_MK_IO_MODE   : *dataP = path_ptr->io_mode;     break;
        case M_MK_IRQ_ENABLE: *dataP = dev->irqShouldBeEnabled; break;
        case M_MK_IRQ_COUNT :  *dataP = dev->irqCount;     break;
        case M_MK_DEBUG_LEVEL: *dataP = dev->dbgLev;       break;
        case M_MK_OSS_DEBUG_LEVEL:
			*dataP = OSS_DbgLevelGet(OSS_VXWORKS_OS_HDL); break;

        case M_MK_TICKRATE:
			*dataP = OSS_TickRateGet( OSS_VXWORKS_OS_HDL ); break;
        case M_MK_REV_SIZE:
			*dataP = 0x2000; break; /* lazy */

		/*--------------------------------------------------------------------+
        | MDIS BLK, LL-DRV, BBIS set/getstats                                 |
        +--------------------------------------------------------------------*/
        default:
            /*--------------------+
            |  MDIS BLK           |
            +--------------------*/
            if( M_MK_BLK_OF <= code && code <= (M_MK_BLK_OF + 0xff) )
            {

			    DBGWRT_2((DBH,"%s MK_BlkGetStat\n", functionName));
                error = MK_BlkGetStat( dev, code,
									   (M_SETGETSTAT_BLOCK*) dataP );
            }
            else if(( M_BB_OF <= code && code <= (M_BRD_OF + 0xff) )
					|| ( M_BB_BLK_OF <= code &&
						 code <= (M_BRD_BLK_OF + 0xff))){

				/*--------------------+
				| BBIS codes          |
				+--------------------*/

				DBGWRT_2((DBH,"%s BB getstat\n", functionName));
				bbListElement = dev->bbListElement;
				error = bbListElement->bbData->bbHdlEntrys.getStat(
					bbListElement->bbData->bbHdlData,
					dev->mSlot,
					code,
					dataP );
			}
			else {
				/*--------------------+
				| unknown codes       |
				+--------------------*/
				DBGWRT_ERR( ( DBH, "%s%s: unknown code %s%d%s",
							  errorStartStr, functionName,
							  errorLineStr, __LINE__, errorEndStr ));
				error = ERR_MK_UNK_CODE;
			}
			break;
		} /* switch */
	} /* if */

	return(error);
}/*MK_GetStat*/

/******************************* MK_Read *************************************
 *
 *  Description:  Read value from current channel
 *                - call LL-read()
 *                - if auto increment: currentCh++
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr   path number
 *                valueP    pointer to variable where read value stored
 *
 *  Output.....:  *valueP   read value
 *                return    0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 ****************************************************************************/
int32 MK_Read
(
    int32 pathNbr,
    int32 *valueP
)
{
    DBGCMD( static char functionName[] = "MK_Read"; )
    MK_PATH_DATA    *path_ptr;
    MK_DEVICE_DATA  *dev;
	int32   error, error2;
	u_int32 currCh;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s\n", functionName));

	/*--------------+
	| lock device   |
	+---------------*/
    currCh = path_ptr->currentCh;
    if( (error = MK_LockDevice( dev, dev->semRead, currCh )))
		return error;

    /*-------------------+
    | call LL-read()     |
    +-------------------*/
    error = dev->llDrvEntrys.read( dev->llDrvHdl, currCh, valueP );
    if( error )
        goto CLEANUP;


    if( path_ptr->io_mode == M_IO_EXEC_INC )                 /* increment ? */
    {
        if( path_ptr->currentCh== (dev->ch_number-1) ) /* last channel ? */
            path_ptr->currentCh = 0;                          /* wrap around */
        else
            path_ptr->currentCh++;                     /* next */
    }

CLEANUP:
	/*--- unlock device ---*/
	error2 = MK_UnlockDevice( dev, dev->semRead, currCh );
	if( error2 && !error)
		error = error2;

    return(error);
}/*MK_Read*/

/******************************* MK_Write ************************************
 *
 *  Description:  Write value to the current channel
 *                - call LL-write()
 *                - if auto increment: currentCh++
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr   path number
 *                value     value to write
 *
 *  Output.....:  return    0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_Write
(
    int32 pathNbr,
    int32 value
)
{
    DBGCMD( static char functionName[] = "MK_Write"; )
    MK_PATH_DATA    *path_ptr;
    MK_DEVICE_DATA  *dev;
	int32   error, error2;
	u_int32 currCh;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s()\n", functionName));

	/*--------------+
	| lock device   |
	+---------------*/
    currCh = path_ptr->currentCh;
    if( (error = MK_LockDevice( dev, dev->semWrite, currCh )))
		return error;

    /*-------------------+
    | call LL-write()    |
    +-------------------*/
    error = dev->llDrvEntrys.write( dev->llDrvHdl, currCh, value );
    if( error )
        goto CLEANUP;


    if( (path_ptr->io_mode) == M_IO_EXEC_INC )                 /* increment ? */
   	{
        if( path_ptr->currentCh== (dev->ch_number-1) ) /* last channel ? */
            path_ptr->currentCh = 0;                          /* wrap around */
        else
            path_ptr->currentCh++;                     /* next */
    }


CLEANUP:
	/*--- unlock device ---*/
	error2 = MK_UnlockDevice( dev, dev->semWrite, currCh );
	if( error2 && !error)
		error = error2;

    return(error);
}/*MK_Write*/

/******************************** MK_GetBlock ********************************
 *
 *  Description:  Reads data block.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr      path number
 *                buffer       pointer to buffer
 *                size         requested size
 *                nbrRdBytesP  pointer to variable where number of read bytes
 *                             stored
 *
 *  Output.....:  *nbrRdBytesP number of read bytes
 *                return       0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_GetBlock
(
    int32  pathNbr,
    u_int8 *buffer,
    int32  size,
    int32  *nbrRdBytesP
)
{
    DBGCMD( static char functionName[] = "MK_GetBlock"; )
    MK_PATH_DATA *path_ptr;
    MK_DEVICE_DATA   *dev;
	int32   error, error2;
	u_int32 currCh;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s\n", functionName));

	/*--------------+
	| lock device   |
	+---------------*/
    currCh = path_ptr->currentCh;
    if( (error = MK_LockDevice( dev, dev->semGetBlock, currCh )))
		return error;

	/*---------+
	| LL Call  |
	+---------*/
    error = dev->llDrvEntrys.blockRead( dev->llDrvHdl,
                                            currCh,
                                            buffer,
                                            size,
                                            nbrRdBytesP );


	/*--- unlock device ---*/
	error2 = MK_UnlockDevice( dev, dev->semGetBlock, currCh );
	if( error2 && !error)
		error = error2;

    return(error);
}/*MK_GetBlock*/

/******************************** MK_SetBlock ********************************
 *
 *  Description:  Writes data block.
 *
 *---------------------------------------------------------------------------
 *  Input......:  pathNbr      path number
 *                buffer       pointer to buffer
 *                size         requested size
 *                nbrWrBytesP  pointer to variable where number of
 *							   written bytes will be stored
 *
 *  Output.....:  *nbrWrBytesP number of written bytes
 *                return       0 | error code
 *
 *  Globals....:  MK_PathDataTbl
 *
 ****************************************************************************/
int32 MK_SetBlock
(
   int32   pathNbr,
   u_int8* buffer,
   int32   size,
   int32   *nbrWrBytesP
)
{
    DBGCMD( static char functionName[] = "MK_SetBlock"; )
    MK_PATH_DATA *path_ptr;
    MK_DEVICE_DATA   *dev;
	int32   error, error2;
	u_int32 currCh;

	/*---------------+
	| check pathNbr  |
	+---------------*/
    if( pathNbr < 0 || MK_NUM_FILES <= pathNbr )
    {
        return( ERR_BAD_PATH );
    }/*if*/

	/*---------------+
	| get path data  |
	+---------------*/
    path_ptr = &MK_PathDataTbl[pathNbr];
    dev = path_ptr->deviceData;
    if( dev == NULL)
    {
        return( ERR_BAD_PATH );
    }/*if*/

    DBGWRT_1((DBH,"%s\n", functionName));

	/*--------------+
	| lock device   |
	+---------------*/
    currCh = path_ptr->currentCh;
    if( (error = MK_LockDevice( dev, dev->semSetBlock, currCh )))
		return error;


	/*---------+
	| LL Call  |
	+---------*/
    error = dev->llDrvEntrys.blockWrite( dev->llDrvHdl,
                                             currCh,
                                             buffer,
                                             size,
                                             nbrWrBytesP );

	/*--- unlock device ---*/
	error2 = MK_UnlockDevice( dev, dev->semSetBlock, currCh );
	if( error2 && !error)
		error = error2;

    return(error);
}/*MK_SetBlock*/

/********************************* MK_Irq ***********************************
 *
 *  Description: Main MK interrupt service routine. called for all MDIS devs
 *
 *			   	 Loops through the linked list of installed ISRs for the
 *				 given vector and calls MK_IrqService for each device
 *---------------------------------------------------------------------------
 *  Input......: irq			interrupt number (index in MK_irqTbl)
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void MK_Irq( int irq )
{
	MK_ISR_NODE *isrNode;

	for( isrNode = (MK_ISR_NODE *)MK_irqTbl[irq].head;
		 isrNode->node.next;
		 isrNode = (MK_ISR_NODE *)isrNode->node.next ){

		MK_IrqService( (MK_DEVICE_DATA *)isrNode->isrArg );
	}

}

/************************** MK_IrqService ***********************************
 *
 *  Description: Global interrupt service entry.
 *               Increments the device data irq counter.
 *
 *               Global MDIS interrupt handler/dispatcher:
 *               - calling board handler service init (irqSrvcInit)
 *               - dispatch to low-level-handler      (MXX_Irq)
 *               - calling board handler service exit (irqSrvcExit)
 *
 *---------------------------------------------------------------------------
 *  Input......: dev    pointer to the device data
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void MK_IrqService( MK_DEVICE_DATA *dev )
{
    DBGCMD( static char functionName[] = "MK_IrqService"; )
	BK_BASE_BOARD_DATA *bbData = 	dev->bbListElement->bbData;
	BBIS_ENTRY *bbEntry = &bbData->bbHdlEntrys;
	BBIS_HANDLE *bbHdl  = bbData->bbHdlData;
    int32            	irqFromBB, irqFromLL;

    IDBGWRT_1((DBH,">>> %s vector 0x%x \n", functionName,
			    OSS_IrqHdlGetVector( dev->irqHandle ) ));

	MKWVEV(MK_WV_EV_IRQ);

    /*-----------------------------+
    |  board handler service init  |
    +-----------------------------*/
    irqFromBB = bbEntry->irqSrvInit( bbHdl, dev->mSlot );

	if( irqFromBB & BBIS_IRQ_EXP )
	    logMsg(">>> *** ERROR - bbis exception at %s ***\n",
			   (int) bbData->bbName, 0,0,0,0,0 );

    /*-----------------------------+
    |  ll irq call                 |
    +-----------------------------*/
    if( (irqFromBB & BBIS_IRQ_UNK) || (irqFromBB & BBIS_IRQ_YES))
    {
		MKWVEV(MK_WV_EV_IRQ_BB);

        /* board detected irq on device (or board doesn't know it) */
      if(dev->initialized && dev->llDrvHdl)
		{	/* device initialisation finished? */
	        IDBGWRT_1( ( DBH,">>> MK-LL-Call Slot=0x%x ma[0]=0x%x\n",
						 dev->mSlot, dev->ma[0]) );
			MKWVEV(MK_WV_EV_IRQ_LL);
			irqFromLL = dev->llDrvEntrys.irq( dev->llDrvHdl );

			if( irqFromLL == LL_IRQ_DEVICE )
				dev->irqCount++;
		}/*if*/
    }/*if*/

    /*-----------------------------+
    |  board handler service exit  |
    +-----------------------------*/
	bbEntry->irqSrvExit( bbHdl, dev->mSlot );

}/*MK_IrqService*/


/**********************************************************************/
/** Locks the access to the OSS data via a global semaphore.
 *
 * \param  -
 *
 * \return -
 */
static void MK_DataLock( void )
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
static void MK_DataUnlock( void )
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

/************************** MK_IrqInstall ************************************
 *
 *  Description:  Installs interrupt service routine in system
 *                MK installs always MK_Irq using MK_intConnectRtn and
 *				  maintains its own linked list of attached irqs
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgLev      current debug level
 *                devBusType  bustype OSS_BUSTYPE _NONE | _VME | _PCI |
 *												  _ISA | _ISAPNP
 *                bbisBusType bustype OSS_BUSTYPE _NONE | _VME | _PCI |
 *												  _ISA | _ISAPNP
 *                intNbr      interrupt number (including INT_NUM_IRQ0 offs.)
 *                level       interrupt level
 *                irqMode	  not used
 *                newIsrArg   argument for interrupt service routine
 *				  isrNodeP	  pointer to variable where irq node stored
 *
 *  Output.....:  *isrNodeP	  created isr Node
 *                return    0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
static int32 MK_IrqInstall  /*nodoc*/
(
    u_int32        dbgLev,
    u_int32        devBusType,
    u_int32        bbisBusType,
    int32          intNbr,
    int32          level,
    u_int32        irqMode,
    void           *newIsrArg,
	MK_ISR_NODE	   **isrNodeP
)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	MK_ISR_NODE*	isrNode=NULL;
    u_int32         gotsize;
    STATUS          vxRetCode = 0;
    int32           retCode = 0;
	int				doIntConnect = FALSE;
	int32			ossIrqNum0 = OSS_GetIrqNum0();
	int32			irq = intNbr - ossIrqNum0;	/* subtract INT_NUM_IRQ0 */
	int				isLocked = 0;

    DBGWRT_LEV1(dbgLev,(DBH, "%s:    intNbr %d, irq %d, level %d, vector 0x%x, with arg 0x%08x\n",
			               functionName, intNbr, irq, level, INUM_TO_IVEC(intNbr), newIsrArg ));

	/*---------------------------------+
	| Check if ossIrqNum0 initialized  |
	+----------------------------------*/
	if( ossIrqNum0 == -1 ){
		DBGWRT_LEVERR(dbgLev,(DBH,"%s%s: ossIrqNum0 uninitialized! %s%d%s",
					errorStartStr, functionName, errorLineStr,
					__LINE__, errorEndStr ));
        return ERR_MK_IRQ_INSTALL;
	}

	/*----------------+
	|  Make ISR node  |
	+----------------*/
  if(isrNodeP != NULL)
	*isrNodeP = NULL;
    if( (isrNode = (MK_ISR_NODE*) OSS_MemGet( OSS_VXWORKS_OS_HDL,
											  sizeof(MK_ISR_NODE),
											  &gotsize )) == NULL ){
		DBGWRT_LEVERR(dbgLev,(DBH,"%s%s: Can't alloc isrNode %s%d%s",
					errorStartStr, functionName, errorLineStr,
					__LINE__, errorEndStr ));
        retCode = ERR_OSS_MEM_ALLOC;
        goto IRQ_INST_ERR;
	}

	isrNode->isrArg	= newIsrArg;
	isrNode->gotsize = gotsize;

	/*------------------------------------------------+
	|  Attach device to linked list of MK interrupts  |
	+------------------------------------------------*/

	/*--- check if intNbr in range ---*/
	if( irq >= MK_NUM_IRQ_VECT ){
		DBGWRT_LEVERR(dbgLev,(DBH,"%s%s: irq %d out of range %s%d%s",
					errorStartStr, functionName, irq, errorLineStr,
					__LINE__, errorEndStr ));
        retCode = ERR_MK_IRQ_INSTALL;
        goto IRQ_INST_ERR;
	}

	/* lock critical section */
	MK_DataLock();
	isLocked = 1;

	/*--- check if list of this irq was empty before ---*/
	if( (MK_irqConnected[irq/32] & (1L<<(irq & 31))) == 0 )
		doIntConnect = TRUE;


	/*--- add ISR node ---*/
	OSS_DL_AddTail( &MK_irqTbl[irq], &isrNode->node );

	/*
	 * if this was the first MK ISR for this intNbr, attach MK_Irq
	 * to the system's irq handler for that vector
	 */
	if( doIntConnect )
	{
		#if (CPU==PENTIUM4)
		{
			/* map PCI IRQ number to virtual irq number
			   if >= vxWorks 6.6 && PENTIUM && SMP && mapping function exists
			*/
			int pciIrqNum;
			int virtualIrqNum;
			int error;

			/* sanity check */
			if( MK_smpPciIrqNumToVirtualIrqNumRtn == NULL )
			{
				#ifdef _SMP_COMPATIBLE_ONLY
					if( vxCpuConfiguredGet() < 2 )
					{
						goto LEAVE_INFO; /* nothing to do for UP BSP */
					}

					logMsg("*** MK: MK_SetSmpPciIrqNumToVirtualIrqNumRtn() setup call missing\n", 1,2,3,4,5,6 );
		        	DBGWRT_LEVERR(dbgLev,(DBH, "%s%s: MK_SetSmpPciIrqNumToVirtualIrqNumRtn() setup call missing %s%d%s",
		        								 errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		        	DBGWRT_LEVERR(dbgLev,(DBH, "INFO: please add something like BUILD/MDIS/MEN_F15_SMP/sysPentiumPciIrqNumToVirtualIrqNum.c to your BSP also\n" ));
		        	retCode = ERR_MK_IRQ_INSTALL;
					OSS_DL_Remove( &isrNode->node );
		        	goto IRQ_INST_ERR;

		        LEAVE_INFO:
				#endif
		        	/* info only */
		        	DBGWRT_LEVERR(dbgLev,(DBH, "INFO: BSP APIC SYMMETRIC_IO_MODE not allowed Interrupt Mode - check your BSP to be sure\n" ));
		        	DBGWRT_LEVERR(dbgLev,(DBH, "INFO: or add something like BUILD/MDIS/MEN_F15_SMP/sysPentiumPciIrqNumToVirtualIrqNum.c to your BSP also\n" ));
		        	DBGWRT_LEVERR(dbgLev,(DBH, "INFO: and configure with MK_SetSmpPciIrqNumToVirtualIrqNumRtn( sysPentiumPciIrqNumToVirtualIrqNum )\n" ));
					goto LEAVE_IT;
			}

			pciIrqNum = irq; /* irq=intNbr-ossIrqNum0 see above:practicaly ossIrqNum0 has
								no influence in this case */

			error = MK_smpPciIrqNumToVirtualIrqNumRtn( pciIrqNum, &virtualIrqNum );
			if( error )
			{
	        	DBGWRT_LEVERR(dbgLev,(DBH, "%s%s: MK_smpPciIrqNumToVirtualIrqNumRtn %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
	        	retCode = ERR_MK_IRQ_INSTALL;
				OSS_DL_Remove( &isrNode->node );
	        	goto IRQ_INST_ERR;
			}
			intNbr = virtualIrqNum;

			LEAVE_IT:
				;
		}
		#endif /*(CPU==PENTIUM4)*/

		DBGWRT_LEVERR(dbgLev,(DBH," attach MK_Irq to intNbr %d vector %d\n", intNbr, INUM_TO_IVEC(intNbr) ));

		/* ts@men: here regular A21 obmmod IRQ is installed
		logMsg("%s: connecting MK_Irq(). &MK_Irq = %x\n", __FUNCTION__,(int)MK_Irq,3,4,5,6); */

        vxRetCode = (*MK_intConnectRtn)( (VOIDFUNCPTR*) INUM_TO_IVEC(intNbr), (VOIDFUNCPTR) MK_Irq, irq );

	    if( vxRetCode != OK )
    	{
        	DBGWRT_LEVERR(dbgLev,(DBH, "%s%s: MK_intConnectRtn %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
        	retCode = ERR_MK_IRQ_INSTALL;
			OSS_DL_Remove( &isrNode->node );
        	goto IRQ_INST_ERR;
    	}

		/*--- flag MK_Irq connected for that irq ---*/
		MK_irqConnected[irq/32] |= (1L<<(irq & 31));

		/*
		 * Enable the IRQ "level". This is somewhat confusing in VxWorks
		 * since each BSP has it's own view about levels. We distinguish
		 * here at least between VME levels and other (PCI/ISA/PIC) levels
		 */

		/*--- VME ---*/
		if( bbisBusType == OSS_BUSTYPE_VME  ){

			if( MK_vmeIntEnableRtn ){
				DBGWRT_LEV2(dbgLev,(DBH,"%s Enable VME int level %d\n", functionName, level ));
				if( (*MK_vmeIntEnableRtn)(level) == ERROR )
				{
					DBGWRT_LEVERR(dbgLev,(DBH,"*** MK: Warning: MK_vmeIntEnableRtn failed"));
				}/*if*/
			}
			else {
				DBGWRT_LEVERR(dbgLev,(DBH,"*** MK: Warning: No VME interrupt "
									  "enable function installed. Call "
									  "MK_SetVmeIntEnableRtn()\n"));
			}
		}

		/*--- other ---*/
    	if( ((devBusType == OSS_BUSTYPE_ISA) ||
			 (devBusType == OSS_BUSTYPE_ISAPNP) ||
			 (devBusType == OSS_BUSTYPE_PCI ) ||
			 (bbisBusType == OSS_BUSTYPE_PCI ))){

			if( MK_intEnableRtn )
			{
				DBGWRT_LEVERR(dbgLev,(DBH,"%s: Enable generic int level %d\n", functionName, level ));
				if( (*MK_intEnableRtn)(level) == ERROR )
				{
					DBGWRT_LEVERR(dbgLev,(DBH,"*** MK: Warning: MK_intEnableRtn failed\n"));
				}/*if*/
			}
			else {
				DBGWRT_LEVERR(dbgLev,(DBH,"*** MK: Warning: No generic interrupt enable function "
										  "installed. Call MK_SetIntEnableRtn()\n"));
			}

		}
	}

	/* unlock critical section */
	MK_DataUnlock();
	isLocked = 0;

  if(isrNodeP != NULL)
	*isrNodeP = isrNode;

    return( retCode );

IRQ_INST_ERR:
	if( isrNode )
	{
		OSS_MemFree( OSS_VXWORKS_OS_HDL, (int8*) isrNode, isrNode->gotsize );
		isrNode = NULL;
	}

	/* unlock critical section */
	if( isLocked )
	{
		MK_DataUnlock();
	}

    return( retCode );
}/*MK_IrqInstall*/

/************************** MK_IrqRemove **************************************
 *
 *  Description:  Removes the irq routine from MK irq list
 *                and frees allocated memory.
 *
 *                MK_Irq is still attached to system because VxWorks
 *				  provides no clean way to deinstall interrupt
 *
 *---------------------------------------------------------------------------
 *  Input......:  dbgLev      current debug level
 *				  isrNodeP	  ptr to ptr to isr node
 *
 *  Output.....:  *isrNodeP	  NULL
 *                return      0 | error code
 *
 *  Globals....:  -
 *
 ****************************************************************************/
static int32 MK_IrqRemove  /*nodoc*/
(
    u_int32        dbgLev,
	MK_ISR_NODE	   **isrNodeP
)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	MK_ISR_NODE* 	isrNode   = *isrNodeP;
	int32			error = OK;

    DBGWRT_LEV1(dbgLev,(DBH,"%s\n", functionName));

	MK_DataLock();

	OSS_DL_Remove( &isrNode->node ); /* remove from linked list */

	MK_DataUnlock();

	*isrNodeP = NULL;
    OSS_MemFree( OSS_VXWORKS_OS_HDL, (int8*) isrNode, isrNode->gotsize );
   	isrNode = NULL;

    return( error );
}/*MK_IrqRemove*/

/************************* MK_BlkGetStat ***********************************
 *
 *  Description:  MDIS kernel blockgetstats.
 *
 *    supported  code:          structure of data block
 *      M_MK_BLK_PHYSADDR       gets physical address and size
 *       blockStruct->size          size of the data buffer >
 *														sizeof(M_ADDRSPACE)
 *       blockStruct->data          pointer to user buffer where
 *                                  structure M_ADDRSPACE will stored
 *
 *      M_MK_BLK_VIRTADDR       gets virtual address and size
 *       blockStruct->size          size of the data buffer >
 *														sizeof(M_ADDRSPACE)
 *       blockStruct->data          pointer to user buffer where
 *                                  structure M_ADDRSPACE will stored
 *
 *      M_MK_BLK_BB_HANDLER     gets board handler name e.g. A201
 *       blockStruct->size          size of the data buffer
 *       blockStruct->data          pointer to user buffer where
 *
 *      M_MK_BLK_BRD_NAME       gets board name e.g. A201_1
 *       blockStruct->size          size of the data buffer
 *       blockStruct->data          pointer to user buffer where
 *
 *      M_MK_BLK_LL_HANDLER     gets low level driver name e.g. M055
 *       blockStruct->size          size of the data buffer
 *       blockStruct->data          pointer to user buffer where
 *
 *      M_MK_BLK_REV_ID         gets ident strings 0 terminated
 *       blockStruct->size          size of the data buffer
 *       blockStruct->data          pointer to user buffer where ID data stored
 *
 *---------------------------------------------------------------------------
 *  Input......:  dev    pointer to the device data
 *                code       status code
 *                blkStruct  pointer to M_SETGETSTAT_BLOCK structure
 *
 *  Output.....:
 *                return     0 | error code
 *
 *  Globals....:  -
 *
 ****************************************************************************/
static int32 MK_BlkGetStat
(
    MK_DEVICE_DATA *dev,
    int32 code,
    M_SETGETSTAT_BLOCK *blkStruct
)
{
	DBGCMD( static char functionName[] = "MK_BlkGetStat"; )
	int32 retCode;
	BK_BASE_BOARD_LIST_ELEMENT       *bbListElement = dev->bbListElement;
	MDIS_IDENT_FUNCT_TBL *llIdFuncTbl;
	MDIS_IDENT_FUNCT_TBL *bbIdFuncTbl;
	u_int32 currSize;
	u_int32 maxSize;
	char    *stringP;
	char    *identStr;
	char    *newLine="\n";
	int     i;
	u_int32 dbgLev;
	MACCESS ma[2];
	u_int32 addrMode, dataMode, requestedSize, availSize;
	OSS_CALLBACK_GETSTAT *cbGetStatP;

	dbgLev  = dev->dbgLev;
    retCode = 0;

    DBGWRT_1((DBH,"%s\n", functionName));

    switch( code )
    {
        /*--------------------+
        |  ident table        |
        +--------------------*/
        case M_MK_BLK_REV_ID:
           retCode = dev->llDrvEntrys.getStat( dev->llDrvHdl,
                                                   code,
                                                   CH0,
                                                   (void*) &llIdFuncTbl );
           if( retCode )
               break;

           retCode = bbListElement->bbData->bbHdlEntrys.getStat(
                                              bbListElement->bbData->bbHdlData,
                                              MSLOT_0,
                                              code,
                                              (void*) &bbIdFuncTbl );
           if( retCode )
               break;
           currSize = 0;
           maxSize = blkStruct->size;
           stringP = (char*) blkStruct->data;

           /* MK id */
           currSize += strlen( MK_RevisionString ) + 1;
           if( currSize < maxSize )
           {
               strcpy( stringP, MK_RevisionString );
               strcat( stringP, newLine );
           }/*if*/

           currSize += strlen( MK_IdentString ) + 1;
           if( currSize < maxSize )
           {
               strcat( stringP, MK_IdentString );
               strcat( stringP, newLine );
           }/*if*/

           /* BK id */
           currSize += strlen( BK_IdentString ) + 1;
           if( currSize < maxSize )
           {
               strcat( stringP, BK_IdentString );
               strcat( stringP, newLine );
           }/*if*/

           /* OS2M id */
           currSize += strlen( OS2M_Ident() ) + 1;
           if( currSize < maxSize )
           {
               strcat( stringP, OS2M_Ident() );
               strcat( stringP, newLine );
           }
           else
               break;

		   /* MAPI SC id */
           currSize += strlen( MAPI_SC_RCSid ) + 1;
           if( currSize < maxSize )
           {
               strcat( stringP, MAPI_SC_RCSid );
               strcat( stringP, newLine );
           }
           else
               break;

           /* LL id */
           for( i=0; i < MDIS_MAX_IDCALLS; i++ )
           {
               if( llIdFuncTbl->idCall[i].identCall == NULL )
                   break;

               identStr = llIdFuncTbl->idCall[i].identCall();
               currSize += strlen( identStr ) + 1;
               if( currSize < maxSize )
               {
                   strcat( stringP, identStr );
                   strcat( stringP, newLine );
               }
               else
                   break;
           }/*for*/

           /* BB id */
           for( i=0; i < MDIS_MAX_IDCALLS; i++ )
           {
               if( bbIdFuncTbl->idCall[i].identCall == NULL )
                   break;

               identStr = bbIdFuncTbl->idCall[i].identCall();
               currSize += strlen( identStr ) + 1;
               if( currSize < maxSize )
               {
                   strcat( stringP, identStr );
                   strcat( stringP, newLine );
               }
               else
                   break;
           }/*for*/
           break;

        /*--------------------+
        |  addresses          |
        +--------------------*/
        case M_MK_BLK_PHYSADDR:
        case M_MK_BLK_VIRTADDR:
        {
            M_ADDRSPACE *addrSpaceP =  blkStruct->data;

            if( blkStruct->size < sizeof(M_ADDRSPACE) )
            {
                DBGWRT_ERR( ( DBH, "%s%s: blkStruct->size to less %s%d%s",
                          errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                retCode = ERR_MK_ILL_PARAM;
            }
            else
            {
                retCode = dev->llDrvEntrys.info( LL_INFO_ADDRSPACE,
													 addrSpaceP->space,
                                                     &addrMode, &dataMode,
													 &requestedSize );
                if( retCode != 0 )
                {
                    DBGWRT_ERR( ( DBH, "%s%s: LL_INFO_ADDRSPACE %s%d%s",
                          errorStartStr, functionName, errorLineStr,
								  __LINE__, errorEndStr ));
                    goto CLEANUP;
                }/*if*/
                retCode = bbListElement->bbData->bbHdlEntrys.getMAddr(
					bbListElement->bbData->bbHdlData,
					dev->mSlot,
					addrMode,
					dataMode,
					(void**)&ma[0],
					&availSize );

                if( retCode != 0 )
                {
                    DBGWRT_ERR( ( DBH, "%s%s: BB getMAddr %s%d%s",
								  errorStartStr, functionName, errorLineStr,
								  __LINE__, errorEndStr ));
                    goto CLEANUP;
                }/*if*/

                /* VXWORKS 1:1 translation of phys and virt addr */
                addrSpaceP->addr =  (int)ma[0];

                if( code == M_MK_BLK_PHYSADDR )
                    /* requested size of LL-Drv */
                    addrSpaceP->size = requestedSize;
                else
                    /* available size of BaseBoard */
                    addrSpaceP->size = availSize;

            }/*if*/
            break;
        }

        case M_MK_BLK_BB_HANDLER:
            if( blkStruct->size < strlen( bbListElement->bbData->brdDrvName ) )
            {
                DBGWRT_ERR( ( DBH, "%s%s: blkStruct->size to less %s%d%s",
                          errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                retCode = ERR_MK_ILL_PARAM;
            }
			else
      		   strcpy( blkStruct->data, bbListElement->bbData->brdDrvName );
            break;
        case M_MK_BLK_BRD_NAME:
            if( blkStruct->size < strlen( bbListElement->bbData->bbName ) )
            {
                DBGWRT_ERR( ( DBH, "%s%s: blkStruct->size to less %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                retCode = ERR_MK_ILL_PARAM;
            }
			else
      		   strcpy( blkStruct->data, bbListElement->bbData->bbName );
            break;

        case M_MK_BLK_LL_HANDLER:
            if( blkStruct->size < strlen( dev->llDrvName ) )
            {
                DBGWRT_ERR( ( DBH, "%s%s: blkStruct->size to less %s%d%s",
							  errorStartStr, functionName, errorLineStr,
							  __LINE__, errorEndStr ));
                retCode = ERR_MK_ILL_PARAM;
            }
			else
      		   strcpy( blkStruct->data, dev->llDrvName );
            break;

		case M_LL_BLK_CALLBACK:
		    cbGetStatP = blkStruct->data;
		    if( cbGetStatP->action == OSS_CB_MODE )
		    	cbGetStatP->p.mode = dev->cData.callBackMode;
			else
                retCode = ERR_MK_ILL_PARAM;
            break;
        default:
           retCode = ERR_MK_UNK_CODE;

    }/*switch*/

CLEANUP:
    return( retCode );
}/*MK_BlkGetStat*/


/******************************* GetBoardParams ***************************
 *
 *  Description: Get fixed board parameters via BrdInfo() function
 *
 *---------------------------------------------------------------------------
 *  Input......: lu 	 unit static storage
 *  Output.....: return  success (0) or error code
 *  Globals....:
 ****************************************************************************/

static int32 GetBoardParams(MK_DEVICE_DATA *dev, DESC_HANDLE *llDescHdl) /* nodoc */
{
    int32 error;     /* error holder */
	BBIS_ENTRY 	*bbEntry = &dev->bbListElement->bbData->bbHdlEntrys;
#ifdef PCI
	BBIS_HANDLE *bbHdl	 = dev->bbListElement->bbData->bbHdlData;
#endif
	/* bus system type */
	if ((error = bbEntry->brdInfo(BBIS_BRDINFO_BUSTYPE,
								  &dev->bbisBusType))) {
		DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_BRDINFO_BUSTYPE\n"));
		return(error);
	}

	/*
	 * If BBIS_BRDINFO_DEVBUSTYPE not supported by BBIS handler, use
	 * OSS_BUSTYPE_MMODULE
	 */
	if ((error = bbEntry->brdInfo(BBIS_BRDINFO_DEVBUSTYPE,
								  dev->mSlot, &dev->devBusType))) {
		DBGWRT_ERR((DBH," WARNING: GetBoardParams: can't get "
					"BBIS_BRDINFO_DEVBUSTYPE\n"));
		dev->devBusType = OSS_BUSTYPE_MMODULE;
	}


	DBGWRT_2((DBH," board bustype=%d, dev bustype=%d\n",
			  dev->bbisBusType,dev->devBusType));

#if 0
	/* ??? irq info */
	if ((error = bbEntry->brdInfo(BBIS_BRDINFO_INTERRUPTS, dev->mSlot,
								  &dev->irqInfo))) {
		DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_BRDINFO_INTERRUPTS\n"));
		return(error);
	}

	DBGWRT_2((DBH," board irq: info=%d\n",dev->irqInfo));
#endif

	/* function info */
	if ((error = bbEntry->brdInfo(BBIS_BRDINFO_FUNCTION,
										BBIS_FUNC_IRQSRVINIT,
										&dev->irqSrvInitFunc))) {
		DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_BRDINFO_FUNCTION BBIS_FUNC_IRQSRVINIT\n"));
		return(error);
	}

	if ((error = bbEntry->brdInfo(BBIS_BRDINFO_FUNCTION,
										BBIS_FUNC_IRQSRVEXIT,
										&dev->irqSrvExitFunc))) {
		DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_BRDINFO_FUNCTION BBIS_FUNC_IRQSRVEXIT\n"));
		return(error);
	}

#ifdef PCI
	/*-------------------------------------+
	|  Request parameters for PCI device   |
	+-------------------------------------*/
	if( dev->bbisBusType == OSS_BUSTYPE_PCI ||
		dev->devBusType == OSS_BUSTYPE_PCI){

		/*--- get PCI bus number from board  ---*/
		if ((error = bbEntry->cfgInfo(bbHdl,
									  BBIS_CFGINFO_BUSNBR,
									  &dev->pciBusNbr,
									  dev->mSlot))){
			DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_CFGINFO_BUSNBR\n"));
			return(error);
		}
	}


	if( dev->devBusType == OSS_BUSTYPE_PCI ){
		/*--- get PCI device number from board ---*/
		if ((error = bbEntry->cfgInfo(bbHdl,
									  BBIS_CFGINFO_PCI_DEVNBR,
									  dev->mSlot,
									  &dev->pciDevNbr))){
			DBGWRT_ERR((DBH," *** GetBoardParams: can't get "
					"BBIS_CFGINFO_BUSNBR\n"));
			return(error);
		}

		/*--- get PCI function number from descriptor ---*/
		if ((error = DESC_GetUInt32(llDescHdl, 0, &dev->pciFuncNbr,
									"PCI_FUNCTION")) &&
			error != ERR_DESC_KEY_NOTFOUND){

			DBGWRT_ERR((DBH," *** GetBoardParams: error "
						"getting PCI_FUNCTION\n"));
			return(error);
		}

		DBGWRT_2((DBH," PCI bus=%d dev=%d\n",
				  dev->pciBusNbr, dev->pciDevNbr));

		/*--- get PCI Vendor/Device ID from descriptor (required) ---*/
		if ( (error = DESC_GetUInt32(llDescHdl, 0, &dev->pciVendorId,
									 "PCI_VENDOR_ID")) ||
			 (error = DESC_GetUInt32(llDescHdl, 0, &dev->pciDeviceId,
									 "PCI_DEVICE_ID"))){

			DBGWRT_ERR((DBH," *** GetBoardParams: error "
						"getting PCI_VENDOR/DEVICE_ID\n"));
			return(error);
		}
		/*--- get PCI SubVendor/SubSys ID from descriptor (optional) ---*/
		if ( ((error = DESC_GetUInt32(llDescHdl, 0xffffffff,
									  &dev->pciSubSysVendorId,
									 "PCI_SUBSYS_VENDOR_ID")) &&
			  error != ERR_DESC_KEY_NOTFOUND ) ||

			 ((error = DESC_GetUInt32(llDescHdl, 0xffffffff,
									  &dev->pciSubSysId,
									  "PCI_SUBSYS_ID")) &&
			  error != ERR_DESC_KEY_NOTFOUND )){
			DBGWRT_ERR((DBH," *** GetBoardParams: error "
						"getting PCI_SUBSYS_VENDOR/SUBSYS_ID\n"));
			return(error);
		}

		DBGWRT_2((DBH," PCI desc params: func=%d vendID=0x%4x devID=0x%4x "
				  "subvendID=0x%4x subdevID=0x%4x\n",
				  dev->pciFuncNbr, dev->pciVendorId, dev->pciDeviceId,
				  dev->pciSubSysVendorId, dev->pciSubSysId ));
	}
#endif /* PCI */


	return(ERR_SUCCESS);
}

#ifdef PCI
/********************************* CheckPciDev ******************************
 *
 *  Description: Check if PCI device present and vendor/devId matches
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev	 device data
 *  Output.....: return  success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 CheckPciDev( MK_DEVICE_DATA *dev )	/* nodoc */
{
	u_int32 vendorId, deviceId, subSysVendorId, subSysId;
	int32 error;

	DBGWRT_1((DBH, "MK - CheckPciDev\n"));
	if( (error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL,
								   dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, OSS_PCI_VENDOR_ID,
								   (int32 *)&vendorId )) ||
		(error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL,
								   dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, OSS_PCI_DEVICE_ID,
								   (int32 *)&deviceId )) ||
		(error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL,
				                   dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, OSS_PCI_SUBSYS_VENDOR_ID,
								   (int32 *)&subSysVendorId )) ||
		(error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL,
								   dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, OSS_PCI_SUBSYS_ID,
				   (int32 *)&subSysId )))
      {
		DBGWRT_ERR((DBH," *** CheckPciDev: Error 0x%x reading cfg space\n",
					error ));
		return error;
	}

	DBGWRT_2((DBH, " PCI params from device: vendID=0x%04x devID=0x%04x "
			  "subSysVendID=0x%04x subSysID=0x%04x\n",
			  vendorId, deviceId, subSysVendorId, subSysId));

	/*--- check if device present ---*/
	if( vendorId == 0xffff && deviceId == 0xffff ){
		DBGWRT_ERR(( DBH, " *** CheckPciDev: PCI device not present!\n"));
		return ERR_OSS_PCI_NO_DEVINSLOT; /* due to lack of better error code */
	}

	/*--- check if device matches vendor/device ID ---*/
	if( (vendorId != dev->pciVendorId) || (deviceId != dev->pciDeviceId)){
		DBGWRT_ERR(( DBH, " *** CheckPciDev: wrong vend/devId 0x%x/0x%x!\n",
					 vendorId, deviceId));
		return ERR_OSS_PCI_ILL_DEV; /* due to lack of better error code */
	}

	/*--- check if device matches subsystem vendor/ID ---*/
	if( (dev->pciSubSysVendorId != 0xffffffff &&
		 (dev->pciSubSysVendorId != subSysVendorId)) ||
		(dev->pciSubSysId != 0xffffffff &&
		 (dev->pciSubSysId != subSysId))){

		DBGWRT_ERR(( DBH, " *** CheckPciDev: wrong subsys vend/Id "
					 "0x%x/0x%x!\n",
					 subSysVendorId, subSysId));
		return ERR_OSS_PCI_ILL_DEV; /* due to lack of better error code */
	}

	return ERR_SUCCESS;
}

/********************************* PciGetAddrReg ***************************
 *
 *  Description: Determine if PCI address register present and determine size
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev	device data
 *				 pciBaseReg	base address register index (0..5)
 *  Output.....: return  	success (0) or error code
 *				 *addrP		physical address
 *				 *sizeP		size of region
 *  Globals....: -
 ****************************************************************************/
static int32 PciGetAddrReg /* nodoc */
(
	MK_DEVICE_DATA *dev,
	u_int32 pciBaseReg,
	void *addrP,
	u_int32 *sizeP
)
{
    DBGCMD( static const char *functionName = __FUNCTION__; )
	int32 			error;
	u_int32 		currentVal;
	u_int32 		which;
	u_int32 		mask;
	u_int32 		size = 0;
    OSS_IRQ_HANDLE  *irqDummyHandle = NULL;
    OSS_IRQ_STATE   irqState;

	/*--- convert to which parameter for OSS_PciGetConfig ---*/
	switch( pciBaseReg )
	{
		case 0: which = OSS_PCI_ADDR_0; break;
		case 1: which = OSS_PCI_ADDR_1; break;
		case 2: which = OSS_PCI_ADDR_2; break;
		case 3: which = OSS_PCI_ADDR_3; break;
		case 4: which = OSS_PCI_ADDR_4; break;
		case 5: which = OSS_PCI_ADDR_5; break;
		default:
            DBGWRT_ERR( ( DBH, "%s%s: pciBaseReg %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
			return ERR_OSS_PCI_UNK_REG;
	}

	/* create a dummy IRQ handle */
	error = OSS_IrqHdlCreate( 0, 0, &irqDummyHandle );
	if( error )
	{
		DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqHdlCreate %s%d%s",	errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		goto CLEANUP;
	}/*if*/

	/*-----------------------------+
	| lock critical section        |
	+-----------------------------*/
	irqState = OSS_IrqMaskR( OSS_VXWORKS_OS_HDL, irqDummyHandle );

	/*--- determine address and size ---*/
	if( (error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL, dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, which,
								   (int32 *)&currentVal )))
	{
        DBGWRT_ERR( ( DBH, "%s%s: OSS_PciGetConfig %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		goto CLEANUP_WITH_UNMASK;
	}


	/*--- write ffs to PCI address reg ---*/
	if(	(error = OSS_PciSetConfig( OSS_VXWORKS_OS_HDL, dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, which, (int)(0xffffffffUL) )))
	{
        DBGWRT_ERR( ( DBH, "%s%s: OSS_PciSetConfig %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		goto CLEANUP_WITH_UNMASK;
	}

	/*--- read mask to determine its size ---*/
	if( (error = OSS_PciGetConfig( OSS_VXWORKS_OS_HDL, dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, which,(int32 *)&mask)))
	{
        DBGWRT_ERR( ( DBH, "%s%s: OSS_PciGetConfig %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		goto CLEANUP_WITH_UNMASK;
	}

	/*--- rewrite original value ---*/
	if(	(error = OSS_PciSetConfig( OSS_VXWORKS_OS_HDL, dev->pciBusNbr,
								   dev->pciDevNbr,
								   dev->pciFuncNbr, which,
								   (int32)currentVal )))
	{
        DBGWRT_ERR( ( DBH, "%s%s: OSS_PciSetConfig %s%d%s", errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		goto CLEANUP_WITH_UNMASK;
	}

	if( mask & 1 )				/* mask out non-address bits */
		mask &= ~0x3;
	else
		mask &= ~0xf;

	size = ~mask + 1;
	DBGWRT_3((DBH, "   pci addr reg %d = 0x%08x, mask 0x%08x, size=0x%x type=%s\n", pciBaseReg, currentVal, mask, size, (mask & 1) ? "I/O" : "MEM" ));

	if( mask == 0 )
	{
		DBGWRT_ERR((DBH," *** PciGetAddrReg: Address reg %d non existant!\n", pciBaseReg ));
		error = ERR_OSS_PCI_ILL_ADDRNBR;
		goto CLEANUP_WITH_UNMASK;
	}

	/*--- now get the physical address (CPU view) ---*/
	if( (error = OSS_BusToPhysAddr( OSS_VXWORKS_OS_HDL, OSS_BUSTYPE_PCI, addrP,
									dev->pciBusNbr, dev->pciDevNbr,
									dev->pciFuncNbr, pciBaseReg )))
	{
		DBGWRT_ERR((DBH," *** PciGetAddrReg: OSS_BusToPhysAddr failed!\n"));
		goto CLEANUP_WITH_UNMASK;
	}

	/*-----------------------------+
	| unlock critical section      |
	+-----------------------------*/
CLEANUP_WITH_UNMASK:
	OSS_IrqRestore( OSS_VXWORKS_OS_HDL, irqDummyHandle, irqState );

CLEANUP:
	if( irqDummyHandle )
	{
		if( OSS_IrqHdlRemove( &irqDummyHandle ) )
		{
			DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqHdlRemove %s%d%s",	errorStartStr, functionName, errorLineStr, __LINE__, errorEndStr ));
		}
	}

	*sizeP = size;
	return error;
}

#endif /* PCI */

/********************************* CreateLockSems *****************************
 *
 *  Description: Create channel/call locking semaphores
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev			device data
 *  Output.....: return  	success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 CreateLockSems( MK_DEVICE_DATA *dev ) /* nodoc */
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	DBGWRT_1((DBH,"MK - %s: lockmode %d\n", functionName, dev->lockMode ));

	if( dev->lockMode == LL_LOCK_CALL ){
		/*-----------------+
		|  Call lock mode  |
		+-----------------*/
		dev->semRead 	= OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		dev->semWrite 	= OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		dev->semGetStat = OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		dev->semSetStat = OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		dev->semGetBlock= OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		dev->semSetBlock= OSS_VXKB_semBCreate( SEM_Q_PRIORITY, SEM_FULL );

		if( !dev->semRead || !dev->semWrite || !dev->semGetStat ||
			!dev->semSetStat || !dev->semGetBlock || !dev->semSetBlock ){

	        DBGWRT_ERR(( DBH, "%s%s: create call locks errno %d %s%d%s\n",
    	           		errorStartStr, functionName,
                    	errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
			return ERR_OSS_MEM_ALLOC;
		}

		DBGWRT_1((DBH,"MK - %s: r/w gs/ss gb/sb %08x/%08x  %08x/%08x  %08x/%08x\n", functionName,
					dev->semRead,
					dev->semWrite,
					dev->semGetStat,
					dev->semSetStat,
					dev->semGetBlock,
					dev->semSetBlock
				 ));

	}
	else if( dev->lockMode == LL_LOCK_CHAN ){
		/*--------------------+
		|  Channel lock mode  |
		+--------------------*/
		int i;
		SEM_ID *sid;

		/* Allocate array of SEM_IDs */
		dev->semChan = (SEM_ID *)OSS_MemGet( OSS_VXWORKS_OS_HDL,
											 sizeof(SEM_ID) * dev->ch_number,
											 &dev->semChanMemsize);

		if( dev->semChan == NULL ){
			DBGWRT_ERR((DBH,"*** MK - CreateLockSems: can't create chan-"
						"lock array\n" ));
			return ERR_OSS_MEM_ALLOC;
		}

		/*--- clear array ---*/
		OSS_MemFill( OSS_VXWORKS_OS_HDL, dev->semChanMemsize,
					 (char*)dev->semChan, 0 );

		for( i=0,sid=dev->semChan; i<dev->ch_number; i++,sid++ )
		{
			*sid = semBCreate( SEM_Q_PRIORITY, SEM_FULL );

			if( *sid == NULL )
			{
	        	DBGWRT_ERR(( DBH, "%s%s: create chan %d lock  errno %d %s%d%s\n",
    	           		errorStartStr, functionName,
                    	i, errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
				return ERR_OSS_MEM_ALLOC;
			}
		}
	}
	return ERR_SUCCESS;
}

/********************************* RemoveLockSems ****************************
 *
 *  Description: Remove channel/call locking semaphores
 *			     Note: also removes the device locking semaphore
 *
 *---------------------------------------------------------------------------
 *  Input......: dev			device data
 *  Output.....: return  	success (0) or error code
 *  Globals....: -
 ****************************************************************************/
static int32 RemoveLockSems( MK_DEVICE_DATA *dev ) /* nodoc */
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int 	error = 0;
	int		line  = 0;
	SEM_ID  semId = (SEM_ID)0xdeadbeef;


	DBGWRT_1((DBH,"MK - %s: lockmode %d\n", functionName, dev->lockMode ));
	if( dev->lockMode == LL_LOCK_CALL )
	{
		if( dev->semRead )
		{
			error = semDelete( dev->semRead );
			if( error )
			{
				semId 	= dev->semRead;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}
		if( dev->semWrite )
		{
			error |= semDelete( dev->semWrite );
			if( error )
			{
				semId 	= dev->semWrite;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}

		if( dev->semGetStat )
		{
			error |= semDelete( dev->semGetStat );
			if( error )
			{
				semId 	= dev->semGetStat;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}
		if( dev->semSetStat )
		{
			error |= semDelete( dev->semSetStat );
			if( error )
			{
				semId 	= dev->semSetStat;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}
		if( dev->semGetBlock )
		{
			error |= semDelete( dev->semGetBlock );
			if( error )
			{
				semId 	= dev->semGetBlock;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}
		if( dev->semSetBlock )
		{
			error |= semDelete( dev->semSetBlock );
			if( error )
			{
				semId 	= dev->semSetBlock;
				line 	= __LINE__;
				goto ERR_CALL;
			}
		}

ERR_CALL:
		if( error )
		{
			error = ERR_MK;
       		DBGWRT_ERR(( DBH, "%s%s: semDelete( %08x ) %s%d%s\n",
   	           		errorStartStr, functionName,
   	           		semId,
				  	errorLineStr, line, errorEndStr ));
			goto CLEANUP;
		}
	}
	else if( dev->lockMode == LL_LOCK_CHAN )
	{
		if( dev->semChan )
		{
			int i;
			SEM_ID *sid;

			for( i=0,sid=dev->semChan; i<dev->ch_number; i++,sid++ )
			{
				if( *sid )
					error = semDelete( *sid );
				if( error )
				{
	        		DBGWRT_ERR(( DBH, "%s%s: semDelete chan %d errno %d %s%d%s\n",
    	           		errorStartStr, functionName,
                    	i, errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
					error = ERR_MK;
					goto CLEANUP;
				}
			}

			error = OSS_MemFree( OSS_VXWORKS_OS_HDL, (int8*)dev->semChan, dev->semChanMemsize );
			dev->semChan = NULL;

			if( error )
			{
        		DBGWRT_ERR(( DBH, "%s%s: OSS_MemFree %s%d%s\n",
   	           		errorStartStr, functionName,
				  	errorLineStr, __LINE__, errorEndStr ));
				goto CLEANUP;
			}
			dev->semChan = NULL;
			dev->semChanMemsize = 0;
		}
	}

	/* remove device lock sem */
	if( dev->semDev )
		error = OSS_SemRemove( OSS_VXWORKS_OS_HDL, &dev->semDev );

CLEANUP:
	return error;
}

/********************************* MK_LockDevice ****************************
 *
 *  Description: Lock semaphores depending on lockmode
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev			device
 *				 callSem		ptr to semaphore for specified call
 *								(e.g semRead)
 *				 currCh			current channel
 *  Output.....: error code
 *  Globals....: -
 ****************************************************************************/
static int32 MK_LockDevice(
	MK_DEVICE_DATA *dev,
	SEM_ID callSem,
	int32 currCh)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 vxRetCode=0, retCode, bCallSemIsTaken = 0, bChanSemIsTaken = 0;

	DBGWRT_1((DBH,"%s: devHdl %08x semId %08x chan %d taskId %08x\n", functionName,
			dev, callSem, currCh, taskIdSelf() ));

	/*--------------------------+
	|  Do call/channel locking  |
	+--------------------------*/
	if( dev->lockMode == LL_LOCK_CALL )
	{
		vxRetCode = semTake( callSem, WAIT_FOREVER );
		if( vxRetCode )
		{
	        DBGWRT_ERR(( DBH, "%s%s: semTake( %08x ) failed(1) errno %d TID %08x %s%d%s\n",
    	           		errorStartStr, functionName,
                    	callSem, errnoGet(),
                    	taskIdSelf(),
					  	errorLineStr, __LINE__, errorEndStr ));
			return ERR_MK;			/* better error code? */
		}
		bCallSemIsTaken = 1;
	}
	else if( dev->lockMode == LL_LOCK_CHAN )
	{
		vxRetCode = semTake( dev->semChan[currCh], WAIT_FOREVER  );
		if( vxRetCode )
		{
	        DBGWRT_ERR(( DBH, "%s%s: semTake() failed(1) errno %d TID %08x %s%d%s\n",
                  		errorStartStr, functionName,
                    	errnoGet(),
                    	taskIdSelf(),
					  	errorLineStr, __LINE__, errorEndStr ));
			return ERR_MK;			/* better error code? */
		}
		bChanSemIsTaken = 1;
	}

	/*-------------------------+
	|  Claim device semaphore  |
	+-------------------------*/
	retCode = OSS_SemWait( OSS_VXWORKS_OS_HDL, dev->semDev,
						   OSS_SEM_WAITFOREVER );

	if( retCode ){
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemWait() failed(2) errno %d %s%d%s\n",
                  		errorStartStr, functionName,
                    	errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
        /* if an error occurs here but above Semaphores are locked, unlock them - klocwork id3165 */
        if (bChanSemIsTaken == 1)
        	semGive(dev->semChan[currCh]);

        if (bCallSemIsTaken == 1)
        	semGive(callSem);

        return retCode;
	}

	return ERR_SUCCESS;
}

/********************************* MK_UnlockDevice ****************************
 *
 *  Description: UnLock semaphores depending on lockmode
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev			device
 *				 callSem		ptr to semaphore for specified call
 *								(e.g semRead)
 *				 currCh			current channel
 *  Output.....: error code
 *  Globals....: -
 ****************************************************************************/
static int32 MK_UnlockDevice(
	MK_DEVICE_DATA *dev,
	SEM_ID callSem,
	int32 currCh)
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int32 vxRetCode=0, retCode;

	DBGWRT_1((DBH,"%s: devHdl %08x semId %08x chan %d taskId %08x\n", functionName,
			dev, callSem, currCh, taskIdSelf() ));

	/*--------------------------+
	|  Do call/channel locking  |
	+--------------------------*/
	if( dev->lockMode == LL_LOCK_CALL )
	{
		vxRetCode = semGive( callSem );
		if( vxRetCode )
		{
	        DBGWRT_ERR(( DBH, "%s%s: semGive() failed(1) errno %d %s%d%s\n",
                  		errorStartStr, functionName,
                    	errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
			return ERR_MK;			/* better error code? */
		}
	}
	else if( dev->lockMode == LL_LOCK_CHAN )
	{
		vxRetCode = semGive( dev->semChan[currCh]);
		if( vxRetCode )
		{
        	DBGWRT_ERR(( DBH, "%s%s: semGive() failed(1) errno %d %s%d%s\n",
                  		errorStartStr, functionName,
                    	errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
			return ERR_MK;			/* better error code? */
		}
	}

	/*-------------------------+
	|  Claim device semaphore  |
	+-------------------------*/
	retCode = OSS_SemSignal( OSS_VXWORKS_OS_HDL, dev->semDev );
	if( retCode ){
        DBGWRT_ERR(( DBH, "%s%s: OSS_SemSignal() failed(2) errno %d %s%d%s\n",
                  		errorStartStr, functionName,
                    	errnoGet(),
					  	errorLineStr, __LINE__, errorEndStr ));
		return retCode;
	}

	return ERR_SUCCESS;
}

/********************************* DevCheckIrqEnable *************************
 *
 *  Description: Determine wether Irq can/must be enabled on (sub)device
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev		device to be enabled/disabled
 *  Output.....: returns	0=don't do it, 1=do it
 *  Globals....: -
 ****************************************************************************/
static int32 DevCheckIrqEnable( MK_DEVICE_DATA *dev) /* nodoc */
{
	int i, doit=TRUE;
	MK_DEVICE_DATA *dev2;

	for( i=0; i<OS2M_MAX_DEVS; i++ ){

		/* check if device from table is on same board/slot as our device */
		if( ((dev2 = (MK_DEVICE_DATA *)OS2M_DevTbl[i]) != NULL ) &&
			(dev2->bbDescSpec == dev->bbDescSpec) &&
			(dev2->mSlot == dev->mSlot ) &&
			(dev2 != dev)){

			if( dev2->irqShouldBeEnabled ){
				/* irq still/already enabled on another subdev, don't change!*/
				doit = FALSE;
				break;
			}
		}
	}

	DBGWRT_2((DBH,"MK - DevCheckIrqEnable: bbDesc 0x%08x slot 0x%x "
			  "doit=%d\n", dev->bbDescSpec, dev->mSlot, doit));
	return doit;
}

/********************************* DevCheckSetMIface *************************
 *
 *  Description: Determine wether SetMIface can/must be done on (sub)device
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: dev		device to be setup
 *				 set		0=clear miface 1=set miface
 *  Output.....: returns	0=don't do it, 1=do it
 *  Globals....: -
 ****************************************************************************/
static int32 DevCheckSetMIface( MK_DEVICE_DATA *dev) /* nodoc */
{
	int i, doit=TRUE;
	MK_DEVICE_DATA *dev2;

	for( i=0; i<OS2M_MAX_DEVS; i++ ){

		/* check if device from table is on same board/slot as our device */
		if( ((dev2 = (MK_DEVICE_DATA *)OS2M_DevTbl[i]) != NULL ) &&
			(dev2->bbDescSpec == dev->bbDescSpec) &&
			(dev2->mSlot == dev->mSlot ) &&
			(dev2 != dev)){

			if( dev2->mifaceSet ){
				/* miface setup for another subdev, don't change!*/
				doit = FALSE;
				break;
			}
		}
	}

	DBGWRT_2((DBH,"MK - DevCheckSetMIface: bbDesc 0x%08x slot 0x%x "
			  "doit=%d\n", dev->bbDescSpec, dev->mSlot, doit));
	return doit;
}





















