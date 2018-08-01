/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sysTsi148.c
 *      Project: Tundra TSI148 PCI2VME bridge driver for VxWorks
 *
 *       Author: cs
 *        $Date: 2010/01/22 13:38:39 $
 *    $Revision: 1.9 $
 *
 *  Description: Support routines Tundra PCI to VME bridge Tsi148
 *
 *     Required:  -
 *     Switches:  SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE (non X86 only)
 *				     - if present, don't try to read "vme_irq" system parameter
 *                SYSTSI148_INT_PRIORITIES - interrupt query order (optional)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysTsi148.c,v $
 * Revision 1.9  2010/01/22 13:38:39  cs
 * R:1. TSI148_ITXAX_MASK_A* defines changed minor
 *   2. 2eVME and 2eSST also allowed in A32 mode
 * M:1. adapted use of TSI148_ITXAX_MASK_A*
 *   2. accept setting/use of 2eVME and 2eSST in A32 mode
 *
 * Revision 1.8  2008/12/01 11:51:07  ufranke
 * R1: VxWorks don't boot if TSI148 not initialized by PCI PnP
 * R2: function for init and slot0 function enabled status missing
 * M1: sysTsi148Init() BAR0 and command register will be validated now
 * M2: added sysTsi148ShowInitStatus()
 *
 * Revision 1.7  2008/09/09 11:05:59  cs
 * R: for non DBG built the variable functionName was missing
 * M: rework debug prints for cleanness and space
 *
 * Revision 1.6  2008/08/18 16:35:05  cs
 * R: x86 BSP use special routines to attach/handle VME interrupts
 * M: use sysVmeIntConnect instead of intConnect for VME ISRs for x86 BSPs
 *
 * Revision 1.5  2008/08/01 13:14:54  cs
 * R: 1. SYS_VME_INB_ATTR structure changed
 *    2. mailboxing and location monitor support was only very basic/incomplete
 * M: 1. adapted sysTsi148InboundWinSet() to work with new _INB_ATTR struct
 *    2a. add sysVmeMailboxIrqSetup(), sysTsi148InboundGcsrWinSet()
 *     b. changed API of sysVmeLocMonSetup() and enabled LMx interrupts
 *
 * Revision 1.4  2008/07/29 22:23:03  cs
 * R: driver had a lot of bugs and unknown behaviour
 * M: completely reworked
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

static const char *VME2PCI_TSI148_RCSid = "$Id: sysTsi148.c,v 1.9 2010/01/22 13:38:39 cs Exp $";

/*
DESCRIPTION
This driver provides TSI148 PCItoVME-specific routines.
*/


/* includes */
#include "vxWorks.h"
#include "version.h"
#include "memLib.h"
#include "string.h"
#include "stdio.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "time.h"
#include "vxLib.h"
#include "tyLib.h"
#include "pci.h"

/* Architecture-dependent includes */
#include "sysLib.h"
#include "config.h"

#include "cacheLib.h"
#include "private/vmLibP.h"

#if	(CPU_FAMILY != I80X86)
#include "arch/ppc/vxPpcLib.h"
#include "arch/ppc/mmu603Lib.h"
#endif

#include <MEN/men_typs.h>
#include <MEN/oss.h>

#ifdef MAC_IO_MAPPED
	#undef MAC_IO_MAPPED
#endif
#ifndef MAC_MEM_MAPPED
	#define  MAC_MEM_MAPPED
#endif
#include <MEN/maccess.h>
#include <MEN/tsi148.h>
#include <MEN/sysTsi148.h>

#include "sysTsi148_int.h"

#ifdef SYSTSI148_DBG
	#ifndef DBG
		#define DBG
	#endif /* DBG */
#endif /* SYSTSI148_DBG */

#include <MEN/dbg.h>

#ifdef SYSTSI148_DBG
	static DBG_HANDLE	 	*dbgHdl = 0;
	static unsigned long	dbgLev	= SYSTSI148_DBG;

	#define DBH				dbgHdl
	#define DBG_MYLEVEL 	dbgLev

	#define TSI148DBG_ERR(_x_)	DBGWRT_ERR(_x_)
	#define TSI148DBG_1(_x_)	DBGWRT_1(_x_)
	#define TSI148DBG_2(_x_)	DBGWRT_2(_x_)
	#define TSI148DBG_3(_x_)	DBGWRT_3(_x_)
	#define TSI148DBG_4(_x_)	DBGWRT_4(_x_)
	#define TSI148IDBG_ERR(_x_)	IDBGWRT_ERR(_x_)
	#define TSI148IDBG_1(_x_)	IDBGWRT_1(_x_)
	#define TSI148IDBG_2(_x_)	IDBGWRT_2(_x_)
	#define TSI148IDBG_3(_x_)	IDBGWRT_3(_x_)
	#define TSI148IDBG_4(_x_)	IDBGWRT_4(_x_)
#else  /* SYSTSI148_DBG */
	#define TSI148DBG_ERR(_x_)
	#define TSI148DBG_1(_x_)
	#define TSI148DBG_2(_x_)
	#define TSI148DBG_3(_x_)
	#define TSI148DBG_4(_x_)
	#define TSI148IDBG_ERR(_x_)
	#define TSI148IDBG_1(_x_)
	#define TSI148IDBG_2(_x_)
	#define TSI148IDBG_3(_x_)
	#define TSI148IDBG_4(_x_)
#endif /* SYSTSI148_DBG */

#include "sysLib_men.h"
#include "sysIntCtrl.h"
#include "drv/pci/pciConfigLib.h"	/* for PCI const     */
#include "drv/pci/pciIntLib.h"		/* for PCI interrupt */



/*-----------------------------+
|  DEFINES                     |
+-----------------------------*/

#define X86_INTERRUPT_OFFSET	0x20

#ifndef CPU_INT_LOCK
	#define CPU_INT_LOCK(pData) (*pData = intLock ())
#endif

#ifndef CPU_INT_UNLOCK
	#define CPU_INT_UNLOCK(data) (intUnlock (data))
#endif

#if	(CPU_FAMILY == I80X86)
	#define	SYSTSI148_VME_INTCONNECT(_num,_func,_arg) 	sysVmeIntConnect( _num, _func, _arg )
#else
	#define	SYSTSI148_VME_INTCONNECT(_num,_func,_arg) 	intConnect( INUM_TO_IVEC(_num), _func, _arg )
#endif

/*-----------------------------+
|  TYPEDEFS                    |
+-----------------------------*/

typedef struct _SYSTSI148_INTLEVEL_BIT_MAP
{
	int level;		/* interrupt level as called with intConnect() */
	u_int32 bit;		/* corresponding bit in interrupt registers */
} SYSTSI148_INTLEVEL_BIT_MAP;

typedef struct _SYSTSI148_DMA_CTRL_HDL
{
	SYS_VME_DMA_INTHANDLER userIntHandler;		/* user notification routine (ISR) */
	u_int32 channelBusy;						/* channel free, user informed about finish */
	SEM_ID dmaUserSem;				/* user access semaphore */
	SEM_ID dmaIrqSem;				/* interrupt semaphore */
	SEM_ID dmaUsrHdlSem;			/* userIntHandler protect semaphore */
} SYSTSI148_DMA_CTRL_HDL;

typedef struct
{
	int 	isInit;		/* flags sysTsi148Init() successful */
	/* int littleEndian; */
	int 	bus;		/* PCI bus number of bridge device */
	int 	dev;		/* PCI dev number of bridge device */
	int 	func;
	u_int32 base;		/* VME bridge registers */

	/* mailbox */
	FUNCPTR sysMailboxRoutine;
	int 	sysMailboxArg;
	int 	sysMailboxConnected;

	/* DMA controllers */
	int 	dmaIsInit;	/* flags sysTsi148DmaInit() successfull */
	SYSTSI148_DMA_CTRL_HDL dmaHdl[2];		/* PCI function number of bridge device */
}TSI148_DESC;

/*-----------------------------+
|  GLOBALS                     |
+-----------------------------*/


static SYSTSI148_INTLEVEL_BIT_MAP const G_Tsi148_IntLevelBitMap [] = {
	{SYSTSI148_INT_NUM_IRQ1,	TSI148_INTEX_IRQ1},
	{SYSTSI148_INT_NUM_IRQ2,	TSI148_INTEX_IRQ2},
	{SYSTSI148_INT_NUM_IRQ3,	TSI148_INTEX_IRQ3},
	{SYSTSI148_INT_NUM_IRQ4,	TSI148_INTEX_IRQ4},
	{SYSTSI148_INT_NUM_IRQ5,	TSI148_INTEX_IRQ5},
	{SYSTSI148_INT_NUM_IRQ6,	TSI148_INTEX_IRQ6},
	{SYSTSI148_INT_NUM_IRQ7,	TSI148_INTEX_IRQ7},
	{SYSTSI148_INT_NUM_ACFL,	TSI148_INTEX_ACFL},
	{SYSTSI148_INT_NUM_SYSFL,	TSI148_INTEX_SYSFL},
	{SYSTSI148_INT_NUM_IACK,	TSI148_INTEX_IACK},
	{SYSTSI148_INT_NUM_VIEEN,	TSI148_INTEX_VIE},
	{SYSTSI148_INT_NUM_VERR,	TSI148_INTEX_VERR},
	{SYSTSI148_INT_NUM_PERR,	TSI148_INTEX_PERR},
	{SYSTSI148_INT_NUM_MB0,	    TSI148_INTEX_MB0},
	{SYSTSI148_INT_NUM_MB1,	    TSI148_INTEX_MB1},
	{SYSTSI148_INT_NUM_MB2,	    TSI148_INTEX_MB2},
	{SYSTSI148_INT_NUM_MB3,	    TSI148_INTEX_MB3},
	{SYSTSI148_INT_NUM_LM0,	    TSI148_INTEX_LM0},
	{SYSTSI148_INT_NUM_LM1,	    TSI148_INTEX_LM1},
	{SYSTSI148_INT_NUM_LM2,	    TSI148_INTEX_LM2},
	{SYSTSI148_INT_NUM_LM3,	    TSI148_INTEX_LM3},
	{SYSTSI148_INT_NUM_DMA0,	TSI148_INTEX_DMA0},
	{SYSTSI148_INT_NUM_DMA1,	TSI148_INTEX_DMA1},
};

#define SYSTSI148_INTLEVELBITMAPSIZE (23)

static TSI148_DESC G_hdl;

/*-----------------------------+
|  EXTERN DECLARATIONS         |
+-----------------------------*/
#ifdef MEN_BSP_TEST
MACCESS sysVmeRegs;		/* VME bridge registers */
#endif /* MEN_BSP_TEST */

IMPORT SYS_INT_HANDLER_DESC * sysIntTbl [INTERRUPT_TABLESIZE];
IMPORT SYS_INT_HANDLER_DESC * sysVmeIntTbl [INTERRUPT_TABLESIZE];

IMPORT STATUS sysPciProbe (char *, int, int, char *);
IMPORT u_int32 sysMmParaIntGet( const char *, u_int32, u_int32 );

/*-----------------------------+
|  PROTOTYPES                  |
+-----------------------------*/
static void sysTsi148Intr( void );
static void sysTsi148IRQxIntr( int level );
static void sysMailboxInt( void );
static void sysTsi148DmaInt( int ctrlNum );

static void sysTsi148VeatLogClear( BOOL doLog );
static void sysTsi148ResetRegs( void );

static u_int32 sysTsi148MapLevelToBit( int level );
static int     sysTsi148MapBitToLevel( u_int32 bit );
static void    sysTsi148IntEnable( int level, int enable );
static STATUS  sysTsi148DmaInit( void );
static STATUS  sysTsi148ParseSrcDestAttrib(
					BOOL src,				/* 1: a source descriptor is parsed; 0: dest. descriptor */
					SYS_VME_DMA_ATTR  *dmaAttr,	/* pointer to structure to fill */
					u_int32 *dmaAttrImg	);		/* pointer to Source or Destination Attribute image */
static STATUS  sysTsi148BuildDmaRegImage(
					SYS_VME_DMA_DESC *dmaDesc,		/* descriptor for DMA (src/dest/..) */
					TSI148_DMA_LL_DESC  *dmaLlDesc );	/* pointer to structure to fill */
static STATUS  sysTsi148BuildDctlImage( SYS_VME_DMA_CTRL    * dmaCtrl );


/******************************************************************************
*
* sysMenPci2VmeIdent - returns the pointer to the ident string
*
* This routine returns the pointer to the ident string.
*
* RETURNS: pointer to the ident string
*
*/
char* sysTsi148Ident( void )
{
	return( (char*) VME2PCI_TSI148_RCSid );
}


/******************************************************************************
*
* sysTsi148Init - initialize VME bridge routines
*
* This routine tries to find the TSI148. It initializes the HW.
*
* All VME interrupt levels and windows will be disabled on x86 boards.
* On PPC boards the VME IRQ enable settings as defined by MENMON EE-VME_IRQ
* command and the windows will be taken from the MENMON setting.
*
* The A019 has a BIOS only. Therefore the Windows have to be set up using TBD.
*
* Connects and enables PCI interrupt for internal use.
*
* NOTE: Should be called from sysHwInit().
*
* RETURNS: OK or ERROR if bridge not present
*
* SEE ALSO: sysTsi148Init2()
*/
int sysTsi148Init( void )
{
	u_int16 command;

	bzero( (char*)&G_hdl, sizeof(G_hdl) );

#ifdef TSI148_TBD
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
	G_hdl.littleEndian = 1;
#else
	G_hdl.littleEndian = 0;
#endif
#endif

	if( pciFindDevice( TSI148_VEN_ID, TSI148_DEV_ID, 0,
					   &G_hdl.bus, &G_hdl.dev, &G_hdl.func ) == ERROR )
		return (ERROR);		/* no VME bridge found */

	pciConfigInWord( G_hdl.bus, G_hdl.dev, G_hdl.func, PCI_CFG_COMMAND, &command );

	/* retreive base address of Ctrl regs */
	pciConfigInLong (G_hdl.bus, G_hdl.dev, G_hdl.func, PCI_CFG_BASE_ADDRESS_0, (UINT32*)&G_hdl.base);

	if( G_hdl.base != 0xffffffff
		&& (G_hdl.base & PCI_MEMBASE_MASK) != 0x00000000
		&& command != 0x0000
	  )		/* If "TSI148" chip present and initialized */
	{
		G_hdl.base &= PCI_MEMBASE_MASK;	/* Mask off "space" bits */

		/* now convert it to a local address, we only have the bus address here */
		if( OK != sysBusToLocalAdrs(PCI_SPACE_MEM_PRI, (char *)G_hdl.base, (char **)&G_hdl.base) ) {
			return (ERROR);
		}
	} else {
		return (ERROR);
	}

	sysTsi148ResetRegs( );

	G_hdl.isInit = 1;

	return (OK);
}


/******************************************************************************
*
* sysTsi148Init2 - initialize VME bridge / connect interrupts, ...
*
* All VME interrupt levels and windows will be disabled with the exception of A015.
* In the A015 BSP the VME IRQ enable settings as defined by MENMON EE-VME-IRQ command
* and the A32 master prefix will be taken from the MENMON setting.
*
* The A019 has a BIOS only. Therefore the Windows have to be set up using TBD.
*
* Connects and enables PCI interrupt for internal use.
*
* NOTE: Should be called from sysHwInit2().
*
* RETURNS: OK or ERROR if bridge not present
*
* SEE ALSO: sysTsi148Init()
*/
int sysTsi148Init2( void )
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	u_int8 irq;

	if( G_hdl.isInit == 2 )
	{
		return OK; /* initialize only once */
	} else if( !G_hdl.isInit )
	{
		return ERROR;
	}

	DBGINIT((NULL,&DBH));

	TSI148DBG_1((DBH,"\n%s\n", functionName));

	/* enable busmastering for DMA */
	{
		u_int16 command;
		pciConfigInWord( G_hdl.bus, G_hdl.dev, G_hdl.func, PCI_CFG_COMMAND, &command );
		command |= PCI_CMD_MASTER_ENABLE;
		pciConfigOutWord( G_hdl.bus, G_hdl.dev, G_hdl.func, PCI_CFG_COMMAND, command );
	}

	/*--- connect the VME interrupt routine ---*/
	pciConfigInByte( G_hdl.bus, G_hdl.dev, G_hdl.func, PCI_CFG_DEV_INT_LINE, &irq );

	TSI148DBG_2((DBH,"\n---- %s IRQ:%d", functionName, irq));

#if	(CPU_FAMILY == I80X86)
	pciIntConnect( INUM_TO_IVEC((int)irq+X86_INTERRUPT_OFFSET), sysTsi148Intr, 0 );
	sysIntEnablePIC( irq );
#else /* PPC */
	pciIntConnect( INUM_TO_IVEC((int)irq), sysTsi148Intr, 0 );
	intEnable( irq );
#endif /* CPU_FAMILY == I80X86 */

	if( OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ1, sysTsi148IRQxIntr, 1 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ2, sysTsi148IRQxIntr, 2 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ3, sysTsi148IRQxIntr, 3 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ4, sysTsi148IRQxIntr, 4 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ5, sysTsi148IRQxIntr, 5 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ6, sysTsi148IRQxIntr, 6 ) ||
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_IRQ7, sysTsi148IRQxIntr, 7 ) )
	{
		return( ERROR );
	}

#if	(CPU_FAMILY != I80X86) && !defined(SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE)
  {
	u_int32 val;
	/* enable VME interrupts as defined by MENMON */
	val = sysMmParaIntGet("vme_irq", 16, 0x00);
	TSI148DBG_2((DBH,"%s: Enable VME bus Irqs 0x%04x\n", functionName, val));
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.inten, 0xff );
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.inteo, 0xff );
	SYSTSI148_CTRL_SETMASK( 32, lcsr.inteo, (val & 0xff) );
	SYSTSI148_CTRL_SETMASK( 32, lcsr.inten, (val & 0xff) );
  }
#endif /* SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE */

 	if( OK != sysTsi148DmaInit() )
		return ERROR ;

#ifdef MEN_BSP_TEST
	sysVmeRegs		= (MACCESS)G_hdl.base;		/* VME bridge registers */
#endif /* MEN_BSP_TEST */


	G_hdl.isInit = 2;
	return OK;
}



/******************************************************************************
* sysTsi148OutboundWinSetImg - set VME outbound window (from passed image)
*
* This routine initializes a VME outbound window from a passed image.
* !!! The passed settings are not checked before they are transfered to HW !!!
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148InboundWinSet()
*/
static STATUS sysTsi148OutboundWinSetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_OUTBOUND *win	/* window parameters to be passed to HW */
)
{
	TSI148DBG_3((DBH, "sysTsi148OutboundWinSetImg:\n"
					"\tOTSAX 0x%08x_%08x\n\tOTEAX 0x%08x_%08x\n\tOTOFX 0x%08x_%08x\n\tOTBS 0x%08x\n\tOTAT 0x%08x\n",
					win->otsau, win->otsal, win->oteau, win->oteal, win->otofu, win->otofl, win->otbs, win->otat));

	/* disable window first */
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.outbound[winNo].otat, TSI148_OTAT_EN );

	/* set passed settings */
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otsau, win->otsau);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otsal, win->otsal);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].oteau, win->oteau);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].oteal, win->oteal);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otofu, win->otofu);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otofl, win->otofl);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otbs,  win->otbs);
	SYSTSI148_CTRL_WRITE( 32, lcsr.outbound[winNo].otat,  win->otat);

	return OK;
} /* sysTsi148OutboundWinSetImg */

/******************************************************************************
* sysTsi148OutboundWinAttrImgCompile - compile VME outbound window attributes
*
* This routine compiles an image for the VME outbound window attribute register
* from the SYS_VME_OUTB_ATTR structure passed.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148OutboundWinSet()
*/
static STATUS sysTsi148OutboundWinAttrImgCompile
(
	SYS_VME_OUTB_ATTR *oAttr,	/* window parameters to be passed to HW */
	u_int32 *otat				/* attribute image to fill */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)

	/* start empty */
	*otat = 0x00000000;

	/* scan one by one */
	switch( oAttr->tm ) {
	case(SYS_VME_TM_SCT):
		*otat |= TSI148_OTAT_TM_SCT; break;
	case(SYS_VME_TM_BLT):
		*otat |= TSI148_OTAT_TM_BLT; break;
	case(SYS_VME_TM_MBLT):
		*otat |= TSI148_OTAT_TM_MBLT; break;
	case(SYS_VME_TM_2EVME):
		*otat |= TSI148_OTAT_TM_2EVME; break;
	case(SYS_VME_TM_2ESST160):
		*otat |= (TSI148_OTAT_TM_2ESST | TSI148_OTAT_2ESSTM_160); break;
	case(SYS_VME_TM_2ESST267):
		*otat |= (TSI148_OTAT_TM_2ESST | TSI148_OTAT_2ESSTM_267); break;
	case(SYS_VME_TM_2ESST320):
		*otat |= (TSI148_OTAT_TM_2ESST | TSI148_OTAT_2ESSTM_320); break;
	case(SYS_VME_TM_2ESSTB):
		*otat |= TSI148_OTAT_TM_2ESSTB;	break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal transfer mode 0x%08x\n", functionName, oAttr->tm ));
		return ERROR;
	}

	switch( oAttr->am ) {
	case(SYS_VME_A16):
		*otat |= TSI148_OTAT_AMODE_A16;	break;
	case(SYS_VME_A24):
		*otat |= TSI148_OTAT_AMODE_A24;	break;
	case(SYS_VME_A32):
		*otat |= TSI148_OTAT_AMODE_A32;	break;
	case(SYS_VME_A64):
		*otat |= TSI148_OTAT_AMODE_A64;	break;
	case(SYS_VME_CRCSR):
		*otat |= TSI148_OTAT_AMODE_CSR;	break;
	case(SYS_VME_UM1):
		*otat |= TSI148_OTAT_AMODE_UM1;	break;
	case(SYS_VME_UM2):
		*otat |= TSI148_OTAT_AMODE_UM2;	break;
	case(SYS_VME_UM3):
		*otat |= TSI148_OTAT_AMODE_UM3;	break;
	case(SYS_VME_UM4):
		*otat |= TSI148_OTAT_AMODE_UM4;	break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal address mode 0x%08x\n", functionName, oAttr->am ));
		return ERROR;
	}

	switch( oAttr->dm ) {
	case(SYS_VME_D16):
		*otat |= TSI148_OTAT_DBW_16; break;
	case(SYS_VME_D32):
		*otat |= TSI148_OTAT_DBW_32; break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal data mode 0x%08x\n", functionName, oAttr->dm ));
		return ERROR;
	}

	switch( oAttr->rdpf ) {
	case(SYS_VME_RDPFDIS):
		*otat |= TSI148_OTAT_MRPFD;  break;
	case(SYS_VME_RDPF2CL):
		*otat |= TSI148_OTAT_PFS_2;  break;
	case(SYS_VME_RDPF4CL):
		*otat |= TSI148_OTAT_PFS_4;  break;
	case(SYS_VME_RDPF8CL):
		*otat |= TSI148_OTAT_PFS_8;  break;
	case(SYS_VME_RDPF16CL):
		*otat |= TSI148_OTAT_PFS_16; break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal read prefeth setting 0x%08x\n", functionName, oAttr->rdpf ));
		return ERROR;
	}

	/* supervisor / non-priviledged mode */
	if( TRUE == oAttr->sup ) {
		*otat |= TSI148_OTAT_SUP;
	}

	/* data / program mode */
	if( TRUE == oAttr->pgm ) {
		*otat |= TSI148_OTAT_PGM;
	}

	TSI148DBG_2((DBH,"%s: otat=0x%08x\n", functionName, *otat ));
	return( OK );
} /* sysTsi148OutboundWinAttrImgCompile */

/******************************************************************************
* sysTsi148OutboundWinSet - set VME outbound window
*
* This routine initializes a VME outbound window.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148InboundWinSet()
*/
STATUS sysTsi148OutboundWinSet
(
	int winNo,					/* window 0 - 7 */
	u_int64 srcAdrs,			/* source (PCI) address */
	u_int64 dstAdrs,			/* destination (VME) address */
	u_int64 size,				/* size of Window */
	SYS_VME_OUTB_ATTR *outbAttr,/* outbound window attributes */
	u_int32 bs,					/* 2eSST broadcast select */
	BOOL enable					/* enable window */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	TSI148_OUTBOUND win;	/* window parameters to be passed to HW */
	u_int64 reg64;

	if( G_hdl.isInit != 2 ||
		winNo < 0 || winNo >= TSI148_OUTBOUND_NO )
	{
		return ERROR;
	}

	TSI148DBG_1((DBH, "%s: %d\n", functionName, winNo));

	/* get start address */
	DISASS64(srcAdrs, win.otsau, win.otsal);

	/* get end address */
	reg64 = srcAdrs + size - (u_int64)0x10000; /* bits A15 - A0 are not compared */
	DISASS64(reg64, win.oteau, win.oteal);

	/* get offset */
	reg64 = dstAdrs - srcAdrs;
	DISASS64(reg64, win.otofu, win.otofl);

	/* get 2eSST broadcast select */
	win.otbs = (bs & 0x001fffff);

	/* get attributes */
	if( OK != sysTsi148OutboundWinAttrImgCompile(outbAttr, &win.otat) ){
		TSI148DBG_ERR((DBH, "*** %s: %d: unable to parse atributes\n", functionName, winNo));
		return ERROR;
	}

	/* enable window? */
	if( TRUE == enable ){
		win.otat |= TSI148_OTAT_EN;
	}

	/* write image to HW */
	if( OK != sysTsi148OutboundWinSetImg(winNo, &win) ){
		TSI148DBG_ERR((DBH, "*** %s: %d: unable to write window settings to HW\n", functionName, winNo));
		return ERROR;
	}

	return OK;
} /* sysTsi148OutboundWinSet */

/******************************************************************************
* sysTsi148InboundWinAttrImgCompile - compile VME inbound window attributes
*
* This routine compiles an image for the VME inbound window attribute register
* from the SYS_VME_INB_ATTR structure passed.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148InboundWinSet()
*/
static STATUS sysTsi148InboundWinAttrImgCompile
(
	SYS_VME_INB_ATTR *iAttr,	/* window parameters to be passed to HW */
	u_int32 *itat				/* attribute image to fill */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)

	/* start empty */
	*itat = 0x00000000;

	/* scan one by one */
	/* transfer mode: bitmask of all supported modes */
	if( SYS_VME_TM_BLT == (iAttr->tm & SYS_VME_TM_BLT) )
		*itat |= TSI148_ITAT_BLT;
	if( SYS_VME_TM_MBLT == (iAttr->tm & SYS_VME_TM_MBLT) )
		*itat |= TSI148_ITAT_MBLT;

	switch( iAttr->am ) {
	case(SYS_VME_A16):
		*itat |= TSI148_ITAT_AS_A16;	break;
	case(SYS_VME_A24):
		*itat |= TSI148_ITAT_AS_A24;	break;
	case(SYS_VME_A32):
		*itat |= TSI148_ITAT_AS_A32;	break;
	case(SYS_VME_A64):
		*itat |= TSI148_ITAT_AS_A64;	break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal address space 0x%08x\n", functionName, iAttr->am ));
		return ERROR;
	}

	/* 2eVME and 2eSST only for A32 and A64 mode */
	if( SYS_VME_A32 == iAttr->am ||
		SYS_VME_A64 == iAttr->am )
	{
		if( SYS_VME_TM_2EVME == (iAttr->tm & SYS_VME_TM_2EVME) )
			*itat |= TSI148_ITAT_2EVME;

		if( SYS_VME_TM_2ESST320 == (iAttr->tm & SYS_VME_TM_2ESST320) )
			*itat |= (TSI148_ITAT_2ESST | TSI148_ITAT_2ESSTM_320);
		else if( SYS_VME_TM_2ESST267 == (iAttr->tm & SYS_VME_TM_2ESST267) )
			*itat |= (TSI148_ITAT_2ESST | TSI148_ITAT_2ESSTM_267);
		else if( SYS_VME_TM_2ESST160 == (iAttr->tm & SYS_VME_TM_2ESST160) )
			*itat |= (TSI148_ITAT_2ESST | TSI148_ITAT_2ESSTM_160);

		if( SYS_VME_TM_2ESSTB == (iAttr->tm & SYS_VME_TM_2ESSTB) )
			*itat |= TSI148_ITAT_2ESSTB;
	}

	switch( iAttr->vfs ) {
	case(64):
		*itat |= TSI148_ITAT_VFS_64;  break;
	case(128):
		*itat |= TSI148_ITAT_VFS_128;  break;
	case(256):
		*itat |= TSI148_ITAT_VFS_256;  break;
	case(512):
		*itat |= TSI148_ITAT_VFS_512;  break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal virtual FIFO size 0x%08x\n", functionName, iAttr->vfs ));
		return ERROR;
	}
	/* FIFO threshold */
	if( TRUE == iAttr->threshold ) {
		*itat |= TSI148_ITAT_TH;
	}

	/* access cycle mode: bitmask of all supported modes */
	if( SYS_VME_AC_SUPR == (iAttr->ac & SYS_VME_AC_SUPR) )
		*itat |= TSI148_ITAT_SUPR;
	if( SYS_VME_AC_NPRIV == (iAttr->ac & SYS_VME_AC_NPRIV) )
		*itat |= TSI148_ITAT_NPRIV;

	if( SYS_VME_AC_PGM == (iAttr->ac & SYS_VME_AC_PGM) )
		*itat |= TSI148_ITAT_PGM;
	if( SYS_VME_AC_DATA == (iAttr->ac & SYS_VME_AC_DATA) )
		*itat |= TSI148_ITAT_DATA;

	TSI148DBG_2((DBH,"%s: itat=0x%08x\n", functionName, *itat ));
	return( OK );
}

/******************************************************************************
* sysTsi148InboundWinSetImg - set VME inbound window (from passed image)
*
* This routine initializes a VME inbound window from a passed image.
* !!! The passed settings are not checked before they are transfered to HW !!!
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148OutboundWinSetImg()
*/
static STATUS sysTsi148InboundWinSetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_INBOUND *win	/* window parameters to be passed to HW */
)
{

	TSI148DBG_3((DBH, "sysTsi148InboundWinSetImg:\n"
					"\tITSAX 0x%08x_%08x\n\tITEAX 0x%08x_%08x\n\tITOFX 0x%08x_%08x\n\t\n\tITAT 0x%08x",
					win->itsau, win->itsal, win->iteau, win->iteal, win->itofu, win->itofl, win->itat));

	/* disable window first */
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.inbound[winNo].itat, TSI148_ITAT_EN );

	/* set passed settings */
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].itsau, win->itsau);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].itsal, win->itsal);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].iteau, win->iteau);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].iteal, win->iteal);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].itofu, win->itofu);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].itofl, win->itofl);
	SYSTSI148_CTRL_WRITE( 32, lcsr.inbound[winNo].itat,  win->itat);

	return OK;
} /* sysTsi148InboundWinSetImg */

/******************************************************************************
* sysTsi148InboundWinSet - set VME inbound window
*
* This routine initializes a VME inbound window.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148OutboundWinSet()
*/
STATUS sysTsi148InboundWinSet
(
	int winNo,					/* window 0 - 7 */
	u_int64 srcAdrs,			/* source (VME) address */
	u_int64 dstAdrs,			/* destination (PCI) address */
	u_int64 size,				/* size of Window */
	SYS_VME_INB_ATTR *inbAttr,  /* inbound window attributes */
	BOOL enable					/* enable window */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	TSI148_INBOUND win;	/* window parameters to be passed to HW */
	u_int64 reg64;

	if( G_hdl.isInit != 2 ||
		winNo < 0 || winNo >= TSI148_OUTBOUND_NO )
	{
		return ERROR;
	}

	TSI148DBG_1((DBH, "%s: %d\n", functionName, winNo));

	/* get start address */
	reg64 = srcAdrs & TSI148_ITXAX_MASK; /* just ignore bits 0-3 */
	DISASS64( reg64, win.itsau, win.itsal );

	/* get end address */
	reg64 = srcAdrs + size - (u_int64)0x01LL;
	DISASS64( reg64, win.iteau, win.iteal );

	/* set end address to granularity limit, bits 3-0 not relevant */
	win.iteal &= TSI148_ITXAX_MASK;

	/* check start and end address for plausability, consider granularity */
	switch(inbAttr->am){
	case(SYS_VME_A16):
		if( 0x00LL != (ASS64(win.itsau, win.itsal) & ~TSI148_ITXAX_MASK_A16) ||
			0x00LL != (ASS64(win.iteau, win.iteal) & ~TSI148_ITXAX_MASK_A16))
		{
			TSI148DBG_ERR((DBH,"*** %s: addresses out of range for A16\n", functionName, inbAttr->am ));
			TSI148DBG_ERR((DBH,"*** %s: itsax 0x%08x_%08x iteax 0x%08x_%08x\n", functionName, win.itsau, win.itsal, win.iteau, win.iteal ));
			return ERROR;
		}
		break;
	case(SYS_VME_A24):
		/* bits 11-0 not relevant */
		if( 0x00LL != (ASS64(win.itsau, win.itsal) & ~TSI148_ITXAX_MASK_A24) ||
			0x00LL != (ASS64(win.iteau, win.iteal) & ~(TSI148_ITXAX_MASK_A24 | (u_int64)0x0000000000000ff0LL)) )
		{
			TSI148DBG_ERR((DBH,"*** %s: addresses out of range for A24\n", functionName, inbAttr->am ));
			TSI148DBG_ERR((DBH,"*** %s: itsax 0x%08x_%08x iteax 0x%08x_%08x\n", functionName, win.itsau, win.itsal, win.iteau, win.iteal ));
			return ERROR;
		}
		break;
	case(SYS_VME_A32):
		/* set end address to granularity limit, bits 15-0 not relevant */
		if( 0x00LL != (ASS64(win.itsau, win.itsal) & ~TSI148_ITXAX_MASK_A32) ||
			0x00LL != (ASS64(win.iteau, win.iteal) & ~(TSI148_ITXAX_MASK_A32 | (u_int64)0x000000000000fff0LL)) )
		{
			TSI148DBG_ERR((DBH,"*** %s: addresses out of range for A32\n", functionName, inbAttr->am ));
			TSI148DBG_ERR((DBH,"*** %s: itsax 0x%08x_%08x iteax 0x%08x_%08x\n", functionName, win.itsau, win.itsal, win.iteau, win.iteal ));
			return ERROR;
		}
		break;
	case(SYS_VME_A64):
		/* all addresses acceptable */
		break;
	default:
		TSI148DBG_ERR((DBH,"*** %s: illegal address space 0x%08x\n", functionName, inbAttr->am ));
		return ERROR;
	}


	/* get offset: offset = pciAddr - vmeAddr */
	reg64 = (dstAdrs - srcAdrs) & TSI148_ITOFX_MASK;
	DISASS64(reg64, win.itofu, win.itofl);

	/* get attributes */
	if( OK != sysTsi148InboundWinAttrImgCompile(inbAttr, &win.itat) ){
		TSI148DBG_ERR((DBH, "%s: %d: unable to parse attributes\n", functionName, winNo));
		return ERROR;
	}

	/* enable window? */
	if( TRUE == enable ){
		win.itat |= TSI148_ITAT_EN;
	}

	/* write image to HW */
	if( OK != sysTsi148InboundWinSetImg(winNo, &win) ){
		TSI148DBG_ERR((DBH, "%s: %d: unable to write window settings to HW\n", functionName, winNo));
		return ERROR;
	}

	return OK;
}

/******************************************************************************
* sysTsi148OutboundWinGetImg - get image of VME outbound window
*
* This routine retreives an image of a VME outbound window.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148OutboundWinSetImg(); sysTsi148InboundWinGetImg()
*/
STATUS sysTsi148OutboundWinGetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_OUTBOUND *win	/* window image structure */
)
{
	if( G_hdl.isInit != 2 ||
		winNo < 0 || winNo >= TSI148_OUTBOUND_NO )
	{
		return ERROR;
	}

	/* get image from HW */
	win->otsau = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otsau );
	win->otsal = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otsal );
	win->oteau = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].oteau );
	win->oteal = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].oteal );
	win->otofu = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otofu );
	win->otofl = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otofl );
	win->otbs  = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otbs );
	win->otat  = SYSTSI148_CTRL_READ( 32, lcsr.outbound[winNo].otat );

	return OK;
} /* sysTsi148OutboundWinGetImg */

/******************************************************************************
* sysTsi148InboundWinGetImg - get image of VME inbound window
*
* This routine retreives an image of a VME inbound window.
*
* RETURNS: OK, or ERROR
*
* SEE ALSO: sysTsi148InboundWinSetImg(); sysTsi148OutboundWinGetImg()
*/
STATUS sysTsi148InboundWinGetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_INBOUND *win		/* window image structure */
)
{
	if( G_hdl.isInit != 2 ||
		winNo < 0 || winNo >= TSI148_INBOUND_NO )
	{
		return ERROR;
	}

	/* get image from HW */
	win->itsau = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].itsau );
	win->itsal = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].itsal );
	win->iteau = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].iteau );
	win->iteal = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].iteal );
	win->itofu = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].itofu );
	win->itofl = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].itofl );
	win->itat  = SYSTSI148_CTRL_READ( 32, lcsr.inbound[winNo].itat );

	return OK;
} /* sysTsi148InboundWinGetImg */

STATUS sysTsi148InboundGcsrWinSet
(
	u_int64 srcAdrs,			/* source (VME) address */
	SYS_VME_INB_ATTR *inbAttr,  /* inbound window attributes */
	BOOL enable					/* enable window */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	u_int64 reg64;
	u_int32 gcsrat, reg32u, reg32l;

	if( G_hdl.isInit != 2 )
	{
		return ERROR;
	}

	TSI148DBG_1((DBH, "%s\n", functionName));

	/* get start address */
	reg64 = srcAdrs & (u_int64)0xffffffffffffffe0LL; /* bits 0-4  ignored by HW */
	DISASS64( reg64, reg32u, reg32l );

	SYSTSI148_CTRL_WRITE( 32, lcsr.gcsrat, 0x00000000); /* disable first */
	SYSTSI148_CTRL_WRITE( 32, lcsr.gbau,   reg32u);
	SYSTSI148_CTRL_WRITE( 32, lcsr.gbal,   reg32l);

	/* fill vfs with anything (otherwise sysTsi148InboundWinAttrImgCompile() returns an error) */
	inbAttr->vfs = 64;
	/* get attributes */
	if( OK != sysTsi148InboundWinAttrImgCompile(inbAttr, &gcsrat) ){
		TSI148DBG_ERR((DBH, "*** %s: unable to parse attributes\n", functionName));
		return ERROR;
	}

	gcsrat &= ( TSI148_ITAT_AS_MASK | TSI148_ITAT_AC_MASK ); /* other values ignored */

	if( TRUE == enable )
		gcsrat |= TSI148_GCSRAT_EN;

	SYSTSI148_CTRL_WRITE( 32, lcsr.gcsrat, gcsrat);

	return OK;
}

/******************************************************************************
*
* sysVmeLocalToBusAdrs - convert a local address to a VME bus address
*
* This routine gets the VMEbus address that accesses a specified local
* memory address.
*
* RETURNS: OK, or ERROR if the address space is unknown or the mapping is not
* possible.
*
* SEE ALSO: sysBusToLocalAdrs(); sysVmeBusToLocalAdrs()
*/
STATUS sysVmeLocalToBusAdrs
(
	int 	adrsSpace,	/* bus address space where busAdrs resides */
	char *	localAdrs,	/* local address to convert */
	char **	pBusAdrs	/* where to return bus address */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	u_int32 i;
	u_int32 itatASxMask;
	u_int32 itatAcMask;
	u_int32 regVal;

	u_int64 startAdrs, endAdrs, trans;
	u_int64 vmeSpaceMask;
	u_int64 pciBusAdrs = (u_int32)localAdrs - CPU_2_PCI_MEM_ADRS;
	u_int64 vmeBusAdrs;
	u_int32 vmeBusAdrsH, vmeBusAdrsL;

	if( G_hdl.isInit != 2 )
	{
		return (ERROR);
	}/*if*/

	TSI148DBG_1((DBH, "%s space 0x%02x; pciAdrs 0x%08x_%08x)\n",
					functionName, adrsSpace, (u_int32)((pciBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(pciBusAdrs & (u_int64)0x00000000ffffffffLL) ));

	/*--- check if local address is main memory */
	if( localAdrs > (char *)((char *)sysPhysMemTop() - LOCAL_MEM_LOCAL_ADRS) )
	{
		return (ERROR);
	}/*if*/

	/*--- check for correct address space ---*/
	switch( adrsSpace )
	{
		case VME_AM_STD_SUP_DATA:
			/* fall through */
		case VME_AM_STD_USR_DATA:
			/* fall through */
		case VME_AM_STD_SUP_ASCENDING: /* block transfers */
			/* fall through */
		case VME_AM_STD_USR_ASCENDING:
			itatASxMask  = TSI148_ITAT_AS_A24;
			vmeSpaceMask = 0x0000000000ffffff;
			break;
		case VME_AM_EXT_SUP_DATA:
			/* fall through */
		case VME_AM_EXT_USR_DATA:
			/* fall through */
		case VME_AM_EXT_SUP_ASCENDING: /* block transfers */
			/* fall through */
		case VME_AM_EXT_USR_ASCENDING:
			itatASxMask  = TSI148_ITAT_AS_A32;
			vmeSpaceMask = 0x00000000ffffffff;
			break;
		case VME_AM_SUP_SHORT_IO:
			/* fall through */
		case VME_AM_USR_SHORT_IO:
			itatASxMask = TSI148_ITAT_AS_A16;
			vmeSpaceMask = 0x000000000000ffff;
			break;
		default:
			/* logMsg("*** %s line %d (unknown address space)\n", (int)__FUNCTION__, __LINE__,3,4,5,6 ); */
			TSI148DBG_ERR((DBH,"*** %s line %d (unknown address space)\n", functionName, __LINE__ ));
			return (ERROR);
	}

	/* check for correct SUPR/USR DATA/PGM flags */
	if( SYSTSI148_AM_IS_SUP( adrsSpace ) ){
		itatAcMask  = TSI148_ITAT_SUPR;
	} else {
		itatAcMask  = TSI148_ITAT_NPRIV;
	}

	if( SYSTSI148_AM_IS_PGM( adrsSpace ) ){
		itatAcMask  |= TSI148_ITAT_PGM;
	} else {
		itatAcMask  |= TSI148_ITAT_DATA;
	}

	/*--- check enabled inbound windows for maching translation space ---*/
	for(i = 0; i < TSI148_INBOUND_NO; i++ ) {
		regVal = SYSTSI148_CTRL_READ(32, lcsr.inbound[i].itat);

		if( 0 != ( regVal & TSI148_ITAT_EN ) &&
			( regVal & TSI148_ITAT_AS_MASK ) == itatASxMask  && /* address space matches */
			( regVal & itatAcMask ) == itatAcMask  )   			/* access cycle type is supported */
		{
			startAdrs  = ASS64(SYSTSI148_CTRL_READ(32, lcsr.inbound[i].itsau),SYSTSI148_CTRL_READ(32, lcsr.inbound[i].itsal));
			endAdrs    = ASS64(SYSTSI148_CTRL_READ(32, lcsr.inbound[i].iteau),SYSTSI148_CTRL_READ(32, lcsr.inbound[i].iteal));
			trans      = ASS64(SYSTSI148_CTRL_READ(32, lcsr.inbound[i].itofu),SYSTSI148_CTRL_READ(32, lcsr.inbound[i].itofl));

			/* addresses match? */
			if( pciBusAdrs >= ( startAdrs + trans ) &&
				pciBusAdrs <= ( endAdrs   + trans ) )
			{
					vmeBusAdrs = (pciBusAdrs - trans) & vmeSpaceMask ;
					DISASS64(vmeBusAdrs, vmeBusAdrsH, vmeBusAdrsL);
					*pBusAdrs = (char*)(U_INT32_OR_64)vmeBusAdrs;
					TSI148DBG_3((DBH,"%s iWin %d: loc 0x%08x_%08x bus 0x%08x_%08x\n", functionName, i,
									(u_int32)((pciBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(pciBusAdrs & (u_int64)0x00000000ffffffffLL),
									(u_int32)((vmeBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(vmeBusAdrs & (u_int64)0x00000000ffffffffLL) ));
					return OK;
			}
		}
	}

	TSI148DBG_ERR((DBH,"*** %s line %d (no matching window found for space 0x%02x pciAdrs 0x%08x_%08x)\n",
						functionName, __LINE__,adrsSpace,
						(u_int32)((pciBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(pciBusAdrs & (u_int64)0x00000000ffffffffLL) ));
	return (ERROR);
}


/******************************************************************************
*
* sysVmeBusToLocalAdrs - convert a VME bus address to a local address
*
* This routine gets the local address that accesses a specified VMEbus
* address.
*
* RETURNS: OK, or ERROR if the address is unknown or the mapping is not
* possible.
*
* SEE ALSO: sysLocalToBusAdrs(); sysVmeLocalToBusAdrs()
*/
STATUS sysVmeBusToLocalAdrs
(
	int  	adrsSpace, 	/* bus address space where busAdrs resides */
	char *	busAdrs,   	/* bus address to convert */
	char **	pLocalAdrs 	/* where to return local address */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__;)
	u_int32 i, regVal;
	u_int32 pciBusAdrsH, pciBusAdrsL;
	u_int64 vmeBusAdrs = (u_int64)(U_INT32_OR_64)busAdrs;
	u_int64 vmeSpaceMask;
	u_int64 pciBusAdrs;
	u_int64 startAdrs, endAdrs, trans;

	if( G_hdl.isInit != 2 )
	{
		return (ERROR);
	}/*if*/

	TSI148DBG_1((DBH, "%s: space 0x%02x; vmeAdrs 0x%08x_%08x)\n",
					functionName, adrsSpace, (u_int32)((vmeBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(vmeBusAdrs & (u_int64)0x00000000ffffffffLL) ));

	/*--- check enabled outbound windows for maching address range ---*/
	for(i = 0; i < TSI148_OUTBOUND_NO; i++ ) {
		regVal = SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otat);

		if( 0 == (regVal & TSI148_OTAT_EN) ) {
			/* skip disabled windows */
			continue;
		}

		switch (adrsSpace)
		{
		case VME_AM_EXT_SUP_PGM:
		case VME_AM_EXT_SUP_DATA:
		case VME_AM_EXT_USR_PGM:
		case VME_AM_EXT_USR_DATA:
			/* check for A32 enabled */
			if( TSI148_OTAT_AMODE_A32 == (regVal & TSI148_OTAT_AMODE_MASK) ||
				TSI148_OTAT_AMODE_CSR == (regVal & TSI148_OTAT_AMODE_MASK) )
			{
				vmeSpaceMask = (u_int64)0x00000000ffffffffLL;
				break;
			} else {
				/* doesn't match, next window */
				continue;
			}
			break;
		case VME_AM_STD_SUP_PGM:
		case VME_AM_STD_SUP_DATA:
		case VME_AM_STD_USR_PGM:
		case VME_AM_STD_USR_DATA:
			/* check for A24 enabled */
			if( TSI148_OTAT_AMODE_A24 == (regVal & TSI148_OTAT_AMODE_MASK) )
			{
				vmeSpaceMask = (u_int64)0x0000000000ffffffLL;
				break;
			} else {
				/* doesn't match, next window */
				continue;
			}
			break;
		case VME_AM_SUP_SHORT_IO:
		case VME_AM_USR_SHORT_IO:
			/* check for A16 enabled */
			if( TSI148_OTAT_AMODE_A16 == (regVal & TSI148_OTAT_AMODE_MASK) )
			{
				vmeSpaceMask = (u_int64)0x000000000000ffffLL;
				break;
			} else {
				/* doesn't match, next window */
				continue;
			}
			break;
		default: /* invalid address space */
			/* logMsg("*** %s line %d (unknown address space)\n", (int)__FUNCTION__, __LINE__,3,4,5,6 ); */
			TSI148DBG_ERR((DBH,"*** %s line %d (unknown address space)\n", functionName, __LINE__ ));
			return (ERROR);
		} /* switch */

		/* only accept addresses with size appropriate for selected address space */
		if( vmeBusAdrs & ~vmeSpaceMask ) {
			/* address is out of address space range */
			TSI148DBG_ERR((DBH, "*** %s line %d (bus address out of range 0x%08x_%08x)\n",
								functionName, __LINE__,
								(u_int32)((vmeBusAdrs & (u_int64)0xffffffff00000000LL) >> 32),
								(u_int32)( vmeBusAdrs & (u_int64)0x00000000ffffffffLL) ));
			return ERROR;
		}

		/* some window matched, check access cycle type
		 * check for correct SUPR/USR DATA/PGM flags */
		if( ( SYSTSI148_AM_IS_SUP( adrsSpace ) &&
			  (regVal & TSI148_OTAT_SUP) != TSI148_OTAT_SUP ) ||
			( SYSTSI148_AM_IS_PGM( adrsSpace ) &&
			  (regVal & TSI148_OTAT_PGM) != TSI148_OTAT_PGM ) )
		{
			/* logMsg("*** %s line %d (access cycle type does not match)\n", (int)__FUNCTION__, __LINE__,3,4,5,6 ); */
			TSI148DBG_3((DBH,"*** %s line %d (access cycle type does not match)\n", functionName, __LINE__ ));
			continue;
		} /* switch */

		/* some window matched, extract informations */
		startAdrs  = ASS64(SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otsau),SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otsal));
		endAdrs    = ASS64(SYSTSI148_CTRL_READ(32, lcsr.outbound[i].oteau),SYSTSI148_CTRL_READ(32, lcsr.outbound[i].oteal));
		endAdrs    |= (u_int64)0x000000000000ffffLL; /* address lines A15 - A0 are never compared */
		trans      = ASS64(SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otofu),SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otofl));

		/* addresses match? */
		if( vmeBusAdrs >= ( startAdrs + trans ) &&
			vmeBusAdrs <  ( endAdrs   + trans ) )
		{
				pciBusAdrs = (vmeBusAdrs - trans);
				DISASS64(pciBusAdrs, pciBusAdrsH, pciBusAdrsL);
				*pLocalAdrs = (char*)(U_INT32_OR_64)(pciBusAdrs + CPU_2_PCI_MEM_ADRS);
				TSI148DBG_3((DBH,"%s oWin %d: bus 0x%08x_%08x loc 0x%08x_%08x\n", functionName, i,
								(u_int32)((vmeBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(vmeBusAdrs & (u_int64)0x00000000ffffffffLL),
								 0, (pciBusAdrsL + CPU_2_PCI_MEM_ADRS) ));
				return OK;
		}

	} /* for */

	TSI148DBG_ERR((DBH,"*** %s line %d (no matching window found for space 0x%02x vmeAdrs 0x%08x_%08x)\n",
						functionName, __LINE__,adrsSpace,
						(u_int32)((vmeBusAdrs & (u_int64)0xffffffff00000000LL) >> 32), (u_int32)(vmeBusAdrs & (u_int64)0x00000000ffffffffLL) ));
	return ERROR;
}


/*******************************************************************************
*
* sysVmeBusIntAck - acknowledge a bus interrupt
*
* This routine acknowledges a specified VMEbus interrupt level.
*
* NOTE: no-op.
*       VME interrupts are re-enabled in interrupt handler, acknowledged by HW
*
* RETURNS: OK.
*
* SEE ALSO: sysVmeBusIntGen()
*/

int sysVmeBusIntAck
(
	int intLevel ARG_UNUSED	/* interrupt level to acknowledge */
)
{
	return (OK);
}


/*******************************************************************************
*
* sysVmeBusIntGen - generate VME bus interrupt
*
* This routine generates a VME bus interrupt. It does not wait for IACK to occur.
*
* RETURNS: OK - if interrupt could be triggered
*		   ERROR - if bad vector/level or interrupter busy
*
* SEE ALSO: sysBusIntAck(), sysBusIntGen()
*/
STATUS sysVmeBusIntGen
(
	int  level,        /* VMEbus interrupt level to generate (1-7) */
	int  vector        /* interrupt vector to generate (0-255)     */
)
{
	int            lockKey;
	u_int32		vicr;

	if( G_hdl.isInit != 2 )
		return (ERROR);

	/* Validate interrupt level */

	if (level < 1 || level > 7 || vector > 255 || vector < 2)
		return (ERROR);

	TSI148DBG_1((DBH, "sysVmeBusIntGen: level-%d vector-%d\n", level, vector ));

	/* lock interrupts so there will be no interference */

	CPU_INT_LOCK(&lockKey);

	/*--- check if interrupter busy ---*/
	vicr = SYSTSI148_CTRL_READ(32, lcsr.vicr);
	if( 0 != (vicr & TSI148_VICR_IRQS) ){
		CPU_INT_UNLOCK(lockKey);
		return (ERROR);
	}

	/*--- write vector and level, trigger interrupt ---*/
	vicr &= ~TSI148_VICR_STID_MASK; /* clear current vector */
	vicr |= vector; 				/* write new vector */
	SYSTSI148_CTRL_WRITE( 32, lcsr.vicr, vicr );
	SYSTSI148_CTRL_WRITE( 32, lcsr.vicr, (vicr | (level << TSI148_VICR_IRQL_BIT)) );

	CPU_INT_UNLOCK(lockKey);

	return OK;
}/*sysBusIntGen*/


/*******************************************************************************
*
* sysVmeIntEnable - enable a VME bus interrupt level
*
* This routine enables reception of a specified VMEbus interrupt level.
*
* RETURNS: OK, or ERROR if <intLevel> is not in the range 1 - 7.
*
* SEE ALSO: sysVmeIntDisable(), sysIntEnable()
*/

STATUS sysVmeIntEnable
(
	int intLevel        /* VME interrupt level to enable (1-7) */
)
{

	if( G_hdl.isInit != 2 )
		return (ERROR);

	if( intLevel < 1 || intLevel > 7 )
		return (ERROR);

	/* interrupt levels are numerically identical with
	 * coresponding bit in interrupt registers of TSI148 */
	sysTsi148IntEnable( sysTsi148MapBitToLevel(1<<intLevel), 1 );

	TSI148DBG_1(( DBH, "sysVmeIntEnable: VME Interrupt %d enabled \n", intLevel ));
	return OK;
} /* sysVmeIntEnable */


/*******************************************************************************
*
* sysVmeIntDisable - disable a bus interrupt level
*
* This routine disables reception of a specified VMEbus interrupt level.
*
* RETURNS: OK, or ERROR if <intLevel> is not in the range 1 - 7.
*
* SEE ALSO: sysVmeIntEnable(), sysIntDisable()
*/

STATUS sysVmeIntDisable
(
	int intLevel        /* interrupt level to disable (1-7) */
)
{

	if( G_hdl.isInit != 2 )
		return (ERROR);

	if( intLevel < 1 || intLevel > 7 )
		return (ERROR);

	/* interrupt levels are numerically identical with
	 * coresponding bit in interrupt registers of TSI148 */
	sysTsi148IntEnable( sysTsi148MapBitToLevel(1<<intLevel), 0 );

	TSI148DBG_1(( DBH, "sysVmeIntDisable: VME Interrupt %d disabled \n", intLevel ));
	return OK;
} /* sysVmeIntDisable */


/******************************************************************************
*
* sysMailboxConnect - connect a routine to the mailbox interrupt
*
* This routine specifies the interrupt service routine to be called at each
* mailbox interrupt.
*
* RETURNS: OK or ERROR, if mailboxes are not supported or intConnect fails.
*
* SEE ALSO: sysMailboxEnable()
*/

STATUS sysMailboxConnect
(
	FUNCPTR routine,    /* routine called at each mailbox interrupt */
	int     arg         /* argument with which to call routine      */
)
{

	if( !G_hdl.sysMailboxConnected &&
		OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_MB0, sysMailboxInt, 0 ) )
	{
		return ERROR;
	}

	G_hdl.sysMailboxArg       = arg;
	G_hdl.sysMailboxRoutine   = routine;
	G_hdl.sysMailboxConnected = TRUE;

	return (OK);
}

/******************************************************************************
*
* sysVmeMailboxIrqSetup - enable/disable a mailbox IRQ
*
* This routine enables a mailbox IRQ.
* The interrupt handler must be connected to <VME_MBOX_0/1/2/3_INT_VEC>.
*
* NOTE: For access to the mailbox the A16 slave must be enabled.
*       The PLDZ002 control registers and internal RAM with the
*       mailbox are accessable on A16 space.
*
* SEE ALSO: sysMailboxEnable()
*
* RETURNS: OK or ERROR
*
*/
STATUS sysVmeMailboxIrqSetup
(
	int mbox,   /* mailbox 0..3 */
	BOOL enable
)
{
	if( G_hdl.isInit != 2 )
		return (ERROR);

	if( mbox < 0 || mbox > 3 )
		return (ERROR);

	if( TRUE == enable ) {
		sysTsi148IntEnable( (SYSTSI148_INT_NUM_MB0 + mbox), 1 );
	} else {
		sysTsi148IntEnable( (SYSTSI148_INT_NUM_MB0 + mbox), 0 );
	}
	return OK;
}


/******************************************************************************
*
* sysMailboxEnable - enable the mailbox interrupt
*
* This routine connects and enables the mailbox interrupt sysMailboxInt()
* at vector SYSTSI148_INT_NUM_MB0.
*
* RETURNS: OK or ERROR if mailboxes are not yet initialized/connected.
*
* SEE ALSO: sysVmeMailboxIrqSetup(), sysMailboxConnect()
*/

STATUS sysMailboxEnable
(
	char *mailboxAdrs ARG_UNUSED         /* mailbox address - not used */
)
{
	/* enable MBOX 0 IRQ at read and write */
	if( 2 == G_hdl.isInit && G_hdl.sysMailboxConnected )
		sysVmeMailboxIrqSetup( 0, TRUE );
	else
		return( ERROR );

	return( OK );
}


/*******************************************************************************
*
* sysVmeAcFailIntEnable - enable ACFAIL interrupt
*
* This routine enables reception of the ACFAIL interrupt that is triggered
* when the VMEbus signal ACFAIL goes low.
*
* RETURNS: OK, or ERROR if VME bridge not initialized
*
* SEE ALSO: sysVmeAcFailIntDisable()
*/
STATUS sysVmeAcFailIntEnable(void)
{
	if( G_hdl.isInit != 2 )
		return (ERROR);

	sysTsi148IntEnable( SYSTSI148_INT_NUM_ACFL, 1 );
	return OK;
} /* sysVmeAcFailIntEnable */


/*******************************************************************************
*
* sysVmeAcFailIntDisable - disable ACFAIL interrupt
*
* This routine disables reception of the ACFAIL interrupt.
* RETURNS: OK, or ERROR if VME bridge not initialized
*
* SEE ALSO: sysVmeAcFailIntEnable()
*/
STATUS sysVmeAcFailIntDisable(void)
{
	if( G_hdl.isInit != 2 )
		return (ERROR);

	sysTsi148IntEnable( SYSTSI148_INT_NUM_ACFL, 0 );
	return OK;
} /* sysVmeAcFailIntDisable */


/*******************************************************************************
*
* sysVmeLocMonSetup - location monitor setup
*
* This routine sets up the location monitor interrupt.
* The interrupt handler(s) must be connected to <SYSTSI148_INT_NUM_LM0>,
* <SYSTSI148_INT_NUM_LM1>, <SYSTSI148_INT_NUM_LM2> and <SYSTSI148_INT_NUM_LM3>.
* The compared address bits are A16 15..5, A24 23..5, A32 31..5, A64 63..5.
*
* Depending on address bits 3-4 the corresponding LM interrupt is generated:
* VMEbus Address     |    INT_NUM_LMx
* ---------------------------------------
* LMBA + (0x00-0x07) |        LM0
* LMBA + (0x08-0x0F) |        LM1
* LMBA + (0x10-0x17) |        LM2
* LMBA + (0x18-0x1F) |        LM3
*
* RETURNS: OK, or ERROR if VME bridge not initialized
*/
STATUS sysVmeLocMonSetup
(
	SYS_VME_ADDRMODE  	am,		/* SYS_VME_Axx where xx=16/24/32/64 */
	u_int32				ac,		/* bitmask of SYS_VME_AC_xx where xx=DATA/PGM/NPRIV/SUPR  */
	u_int64 			adrs,	/* compare addr */
	u_int32				enable	/* bitmask of irqs to enable SYS_VME_LM_0-3 */
)
{
	int			 error = ERROR;
	u_int32      lmAttrReg = 0x00;
	u_int32 reg32u, reg32l;

	if( G_hdl.isInit != 2 )
		goto CLEANUP;

	/* disable LM IRQs and locatoion monitor */
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.inten, TSI148_INTEX_LMX_MASK );
	SYSTSI148_CTRL_CLRMASK( 32, lcsr.inteo, TSI148_INTEX_LMX_MASK );

	SYSTSI148_CTRL_WRITE( 32, lcsr.lmat, (u_int32)0x00000000 );

	/* set comparator addr */
	DISASS64( adrs, reg32u, reg32l );
	SYSTSI148_CTRL_WRITE( 32, lcsr.lmbau, reg32u );
	SYSTSI148_CTRL_WRITE( 32, lcsr.lmbal, reg32l );

	/* clear IRQ */
	SYSTSI148_CTRL_WRITE( 32, lcsr.intc, TSI148_INTEX_LMX_MASK );

	switch( am )
	{
		case SYS_VME_A16:
			 lmAttrReg |= TSI148_LMAT_AS_A16;
			 break;
		case SYS_VME_A24:
			 lmAttrReg |= TSI148_LMAT_AS_A24;
			 break;
		case SYS_VME_A32:
			 lmAttrReg |= TSI148_LMAT_AS_A32;
			 break;
		case SYS_VME_A64:
			 lmAttrReg |= TSI148_LMAT_AS_A64;
			 break;
		default:
			TSI148DBG_ERR(( DBH, "sysVmeLocMonSetup: TSI148: illegal address mode 0x%08x\n", am ));
			goto CLEANUP;
	}/*switch*/

	/* enable LM for selected types of access cycles */
	if( SYS_VME_AC_SUPR == (ac & SYS_VME_AC_SUPR) )
		lmAttrReg |= TSI148_LMAT_SUPR;
	if( SYS_VME_AC_NPRIV == (ac & SYS_VME_AC_NPRIV) )
		lmAttrReg |= TSI148_LMAT_NPRIV;

	if( SYS_VME_AC_PGM == (ac & SYS_VME_AC_PGM) )
		lmAttrReg |= TSI148_LMAT_PGM;
	if( SYS_VME_AC_DATA == (ac & SYS_VME_AC_DATA) )
		lmAttrReg |= TSI148_LMAT_DATA;

	/* write configuration */
	SYSTSI148_CTRL_WRITE( 32, lcsr.lmat, lmAttrReg );

	/* enable IRQs */
	if( enable & SYS_VME_LM_0 )
		sysTsi148IntEnable( SYSTSI148_INT_NUM_LM0, TRUE );
	if( enable & SYS_VME_LM_1 )
		sysTsi148IntEnable( SYSTSI148_INT_NUM_LM1, TRUE );
	if( enable & SYS_VME_LM_2 )
		sysTsi148IntEnable( SYSTSI148_INT_NUM_LM2, TRUE );
	if( enable & SYS_VME_LM_3 )
		sysTsi148IntEnable( SYSTSI148_INT_NUM_LM3, TRUE );

	/* enable location monitor */
	if( enable ) {
		SYSTSI148_CTRL_SETMASK( 32, lcsr.lmat, TSI148_LMAT_EN );
	} else {
		SYSTSI148_CTRL_CLRMASK( 32, lcsr.lmat, TSI148_LMAT_EN );
	}

	error = OK;

CLEANUP:
	return( error );
} /* sysVmeLocMonSetup */

/*******************************************************************************
*
* sysTsi148VeatLogClear - Clear an log VMEbus exception stored in VEAT register
*
* This routine will clear the exception stored in the VEAT register.
* The register can hold only one exception. When another one occurs the latter
* one is ignored and the overflow bit is set.
*
* RETURNS: N/A
*/
static void sysTsi148VeatLogClear
(
	BOOL doLog
)
{
	u_int32 regVal = SYSTSI148_CTRL_READ(32, lcsr.veat);

	if( 0 != (regVal & TSI148_VEAT_VES) )
	{
		if( TRUE == doLog )
			/*logMsg("\nTSI148: VMEbus exception clearing (VEAT = 0x%08x)\n", regVal, 2,3,4,5,6);*/
			TSI148DBG_ERR(( DBH, "\nTSI148: VMEbus exception clearing (VEAT = 0x%08x)\n", regVal ));
	}

	/* called clear, so do it */
	SYSTSI148_CTRL_WRITE(32, lcsr.veat, regVal | TSI148_VEAT_VESCL);

} /* sysTsi148VeatLogClear */

/*******************************************************************************
*
* sysTsi148Intr - VME bridge interrupt handler
*
* This is the main interrupt handler for the TSI148 PCI to VME bridge
* It is connected to a single PCI interrupt and examines the
* VME bridge to determine the interrupt level and calls the connected interrupt
* handler. The order of query is taken from SYSTSI148_INT_PRIORITIES
*
* RETURNS: N/A
*/
static void sysTsi148Intr (void)
{
	SYS_INT_HANDLER_DESC 	*currHandler;
	u_int32	istat;
	int 	level=0;

	static u_int32 irqPrioL[] = { SYSTSI148_INT_PRIORITIES };
	u_int32 *curIrqP = irqPrioL; /* point to first element */
	register u_int32 curIrq = *curIrqP;
	DBGCMD( static const char *functionName = __FUNCTION__ ;)

	TSI148IDBG_1(( DBH, ">>> %s\n", functionName ));

	/* read irq status reg, only look at enabled and unmasked interrupts */
	istat = SYSTSI148_CTRL_READ(32, lcsr.ints);
	istat &= SYSTSI148_CTRL_READ(32, lcsr.inten); /* ignore not enabled interrupts */
	istat &= SYSTSI148_CTRL_READ(32, lcsr.inteo); /* ignore masked interrupts */

	/* walk through priority table */
	/* terminate isr when finished */
	while( (curIrq != 0xff) && (istat != 0x00) ) {
		if( istat & curIrq )
		{
			TSI148IDBG_3(( DBH, ">>> %s found active IRQ bit 0x%08x \n", functionName, curIrq ));
			/* we found an active interrupt bit,
			   try to call associated interrupt handler */
			level = sysTsi148MapBitToLevel( curIrq );
			if( level )
			{
			  #if	(CPU_FAMILY == I80X86)
				/* now dispatch to connected (chained) routines */
				currHandler = sysVmeIntTbl[level];
				if( currHandler != NULL )
				{
					while (currHandler != NULL)
					{
						currHandler->vec (currHandler->arg);
						currHandler = currHandler->next;
					}
				} else {
					TSI148IDBG_ERR(( DBH, ">>>*** %s: Uninitialized VME Interrupt! vector 0x%02x level %d\n", functionName, level ));
				}
			  #else /* I80X86 */
				/* now dispatch to connected (chained) routines */
				currHandler = sysIntTbl[level];
				if( currHandler != NULL )
				{
					while (currHandler != NULL)
					{
						currHandler->vec (currHandler->arg);
						currHandler = currHandler->next;
					}
				} else {
					TSI148IDBG_ERR(( DBH,  ">>>*** %s: Uninitialized VME Interrupt! vector 0x%02x level %d\n", functionName, level ));
				}
			  #endif /* I80X86 */
			} else {
				/* unknown interrupt */
				TSI148IDBG_ERR(( DBH,  ">>>*** %s: unknown interrupt (INTS bit = 0x%08x)\n", functionName, curIrq ));
			}

			/* reset interrupt when no VMEbus IRQx (these handled differently) */
			if( curIrq & ~TSI148_INTEX_IRQX_MASK ){
				SYSTSI148_CTRL_WRITE(32, lcsr.intc, curIrq);
				_SYNC_;
			}

			/* mark es handled */
			istat &= ~curIrq;
		}
		curIrq = *(++curIrqP);
	}

	TSI148IDBG_1(( DBH, "<<< %s\n", functionName ));
	return;
} /* sysTsi148Intr */

/*******************************************************************************
*
* sysTsi148IRQxIntr - VMEbus IRQx interrupt handler
*
* This is the handler for VMEbus IRQx interruts. It is invoked by sysTsi148Intr.
* It determines the interrupt vector of the interrupt source.
* Having obtained the vector number, this routine then vectors into the
* system vector table to the specified interrupt handling routine.
*
* SEE ALSO: sysVmeIntEnable(), sysVmeIntEnable(), sysTsi148Intr()
*
* RETURNS: N/A
*/
static void sysTsi148IRQxIntr ( int irqNum )
{
	SYS_INT_HANDLER_DESC 	*currHandler;
	u_int32	veat;
	int 	vector;
	DBGCMD( static const char *functionName = __FUNCTION__ );

	TSI148IDBG_1(( DBH, ">>> %s: IRQ%d\n", functionName, irqNum ));
	if( 0 > irqNum || 7 < irqNum )
		return;

	/* log/clear present exception. VEAT can hold only one and we don't
	 * want to loose any occuring IACK exception */
	sysTsi148VeatLogClear( TRUE );

	/* get vector */
	/* since most VMEbus interrupters support 8-bit IACK cycles this one is created
	 * by doing an 8-bit read from the corresponding VIACK reg plus offset 3 */
	vector = MREAD_D8((MACCESS)G_hdl.base, (u_int32)OFFSETOF(TSI148_CRG, lcsr.viack[irqNum-1])+3);
	TSI148IDBG_3((DBH, ">>> %s: IRQ%d vector = 0x%08x\n", functionName, irqNum, vector ));

	/* check whether we raised an (IACK) exception */
	veat = SYSTSI148_CTRL_READ(32, lcsr.veat);
	if( veat & TSI148_VEAT_VES ){
		if( veat & TSI148_VEAT_IACK ){
			/* iack error. set vector to 0, no valid vector to be expected */
			vector = 0;
		}
		sysTsi148VeatLogClear( TRUE );
	}

	if( 0 != vector && 0xFF != vector ) {
		/* valid vector, dispatch handlers */
	  #if	(CPU_FAMILY == I80X86)
		/* now dispatch to connected (chained) routines */
		currHandler = sysVmeIntTbl[vector];
		if( currHandler != NULL )
		{
			/*logMsg ("call VME Interrupt! vector 0x%02x addr 0x%08x arg 0x%08x \n", vector, currHandler->vec, currHandler->arg,0,0,0);*/

			while (currHandler != NULL)
			{
				currHandler->vec (currHandler->arg);
				currHandler = currHandler->next;
			}
		} else {
			TSI148IDBG_ERR((DBH, ">>>*** %s: IRQ%d Uninitialized VME Interrupt! vector 0x%02x\n", functionName, irqNum, vector ));
		}
	  #else /* I80X86 */
		/* now dispatch to connected (chained) routines */
		currHandler = sysIntTbl[vector];
		if( currHandler != NULL )
		{
			while (currHandler != NULL)
			{
				currHandler->vec (currHandler->arg);
				currHandler = currHandler->next;
			}
		} else {
			TSI148IDBG_ERR((DBH, ">>>*** %s: IRQ%d Uninitialized VME Interrupt! vector 0x%02x\n", functionName, irqNum, vector ));
		}
	  #endif /* I80X86 */

	}
	TSI148IDBG_1(( DBH, "<<< %s: IRQ%d\n", functionName, irqNum ));
} /* sysTsi148IRQxIntr */

/*******************************************************************************
*
* sysMailboxInt - handle mailbox interrupt
*
* This routine handles mailbox interrupt.
*
* RETURNS: N/A.
*/

static void sysMailboxInt (void)
{
	if (G_hdl.sysMailboxRoutine != NULL)
		G_hdl.sysMailboxRoutine (G_hdl.sysMailboxArg);
}

/******************************************************************************
*
* sysMenPci2VmeSetA32MasterPrefix - sets the upper three bits of the A32 VME address
*
* This routine sets the upper three bits of the A32 VME address.
*
* RETURNS: OK or ERROR
*/
STATUS sysMenPci2VmeSetA32MasterPrefix
(
	u_int32 a32AddressPrefix /* A32 address prefix */
)
{

	if( G_hdl.isInit != 2 )
		return (ERROR);

	/* TSI148_TBD */

	return (OK);
}


/******************************************************************************
*
* sysAddrInVmeRange - in VME range
*
* This routine checks if the local address points to a valid VME range
*
* RETURNS: OK if in VME range or ERROR
*/
int sysAddrInVmeRange
(
	char   * adrs		/* local address */
)
{
	u_int32 i;
	u_int64 startAdrs,
			endAdrs;
	u_int64	cmpAdrs = (u_int64)(U_INT32_OR_64)adrs | (u_int64)0xFFFF;

	if( G_hdl.isInit != 2 )
		return (ERROR);

	/* check whether addr is covered by any outbound window */
	for(i = 0; i < TSI148_OUTBOUND_NO; i++ ) {

		if( 0 == (SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otat) & TSI148_OTAT_EN) ) {
			/* skip disabled windows */
			continue;
		}

		startAdrs  = ASS64(SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otsau),SYSTSI148_CTRL_READ(32, lcsr.outbound[i].otsal));
		endAdrs    = ASS64(SYSTSI148_CTRL_READ(32, lcsr.outbound[i].oteau),SYSTSI148_CTRL_READ(32, lcsr.outbound[i].oteal));

		/* addresses covered? */
		if( cmpAdrs >= startAdrs && cmpAdrs <= endAdrs )
		{
				return OK;
		}
	} /* for */

	return( ERROR );
}


/******************************************************************************
*
* sysVmeProbe - probe a VME bus address
*
* This routine probes an address on the VME bus. The routine checkes the
* BERR flag in the VME bridge. All probing is done with interrupts disabled.
*
* NOTE: <adrs> must be a valid VME address as seen by CPU.
*
* RETURNS: OK or ERROR if address cannot be probed
*/

STATUS sysVmeProbe
	(
	char   * adrs,	 /* address to be probed */
	int      mode,	 /* VX_READ or VX_WRITE */
	int      length, /* 1, 2 or 4 byte probe */
	char   * pVal	 /* address of value to write OR */
					 /* address of location to place value read */
	)
{
	STATUS status = ERROR;
	int      oldLevel = 0;
	u_int32 regVal;
	u_int8	reg8Val;
	DBGCMD( static const char *functionName = __FUNCTION__ ;)

	if( G_hdl.isInit != 2 )
		goto CLEANUP;

	if( !sysAddrInVmeRange( adrs ) )
	{
		TSI148DBG_ERR((DBH,"*** %s !sysAddrInVmeRange\n", functionName ));
		goto CLEANUP;
	}

	/* probe performed with interrupts disabled */
	CPU_INT_LOCK(&oldLevel);

	/* clear VME bridge bus error bit */
	sysTsi148VeatLogClear( FALSE );

	/* access/probe the address
	 * this can be done using sysPciProbe() since the TSI148 is used as an
	 * PCI2VME bridge only */
	/* flush PCI posted write buffer(s) */
	if ((status = vxMemArchProbe (adrs, mode, length, pVal)) == OK )
	{
		if (mode == VX_WRITE)
			reg8Val = *adrs;		/* flush the write */
	} else {
		goto CLEANUP;
	}
	/* unlock interrupts */
	CPU_INT_UNLOCK(oldLevel);
	oldLevel = 0;

	/* check if exception bit has been set */
	regVal = SYSTSI148_CTRL_READ(32, lcsr.veat);

	if( 0 != (regVal & TSI148_VEAT_VES) )
	{
		TSI148DBG_ERR((DBH,"*** %s bus error at %08x\n", functionName, adrs ));
		sysTsi148VeatLogClear( FALSE );
		goto CLEANUP;
	} else {
		status = OK;
	}

CLEANUP:
	if( 0 != oldLevel )
		CPU_INT_UNLOCK(oldLevel);

	return status;
}

/*******************************************************************************
*
* sysVmeIsSystemController - checks if the VME bridge is system controller
*
* This routine checks if the TSI148 brigde slot1 function is enabled.
*
* RETURNS: 0 or 1
*
*/
int sysVmeIsSystemController(void)
{
	u_int16       sysCtrl;

	if( G_hdl.isInit != 2 )
		return 0;

	sysCtrl = SYSTSI148_CTRL_READ(16, gcsr.gctrl);

	if( sysCtrl & TSI148_GCTRL_SCONS )
		return 1;
	else
		return 0;
}

#if 0 /* disable! normal reset is performed when complete board is resetted */
/*******************************************************************************
*
* sysVmeReset - resets the VMEbus
*
* This routine resets the VMEbus if sysVmeIsSystemController() is true.
*
* NOTE: After reset the VMEbus a delay is necessary before accessing the
*       VMEbus devices.
*
* RETURNS: OK or ERROR
*
*/
STATUS sysVmeReset(void)
{
	if( G_hdl.isInit != 2 )
		return (ERROR);

	if( sysVmeIsSystemController() )
	{
		SYSTSI148_CTRL_SETMASK( 32, lcsr.vctrl, TSI148_VCTRL_SRESET );
		return( OK );
	}
	else
		return (ERROR);
}
#endif /* 0 */


/*******************************************************************************
*
* sysVmeA16SlaveAddressSet - enable/disable the A16 slave window
*
* This routine enables the A16 slave window and setup the base address.
* The A16 window exports the bridge control registers to the VMEbus.
*
* SEE ALSO:
*
* RETURNS: OK or ERROR
*
*/
STATUS sysVmeA16SlaveAddressSet
(
	int     enable, /* enables or disables the A16 slave window */
	u_int16 slaveAddr   /* setup the base address A15..A12 i.e. 0xf000 */
)
{
	STATUS error = OK;

	/*  TSI148_TBD */

	return( error );
}

/*******************************************************************************
*
* sysVmeA24SlaveAddressSet - enable/disable the A24 slave windows
*
* This routine enables or disables a A24 slave window and
* sets up the VME address, the cpu address and the window size.
* MEN's PCI2VME bridge has two A24 slave windows one points to SRAM
* and the other one points to main memory.
*
*
* RETURNS: OK or ERROR
*
*/
STATUS sysVmeA24SlaveAddressSet
(
	u_int8  whichWindow, /* SYS_VME_WIN_SRAM or SYS_VME_WIN_MAIN_MEM */
	int     enable,      /* enables or disables the A24 slave window */
	u_int32 vmeAddr,     /* vme address A23..A16 i.e. 0x00f00000 - size aligned */
	u_int32 cpuAddr,     /* cpu address - size aligned - currently not used */
	u_int32 windowSize   /* window size 64kB, 128kB.. 1MB 0x10000..0x100000 power of 2 steps*/
)
{
	STATUS error = ERROR;

	/*  TSI148_TBD */

	return( error );
}


/*******************************************************************************
*
* sysVmeA32SlaveAddressSet - enable/disable the A32 slave window
*
* This routine enables or disables a A32 slave window and
* setups the VME address, the cpu address and the window size.
*
* RETURNS: OK or ERROR
*
*/
STATUS sysVmeA32SlaveAddressSet
(
	u_int8  whichWindow, /* SYS_VME_WIN_SRAM or SYS_VME_WIN_MAIN_MEM */
	int     enable,      /* enables or disables the A32 slave window */
	u_int32 vmeAddr,     /* vme address A31..A20 i.e. 0x10000000 - size aligned */
	u_int32 cpuAddr,     /* cpu address - size aligned - not used for SYS_VME_WIN_SRAM */
	u_int32 windowSize   /* window size 1MB, 2MB.. 256MB 0x100000..0x10000000 power of 2 steps*/
)
{
	STATUS error = OK;

	/*  TSI148_TBD */

	return( error );
}

/*---------------------------------+
|   HELPERS                        |
+---------------------------------*/

/*******************************************************************************
*
* sysTsi148MapLevelToBit - map given irq level to the corresp. register bit
*
* This routine maps a given IRQ level to the corresponding bit in the
* INTX registers.
*
* SEE ALSO: sysTsi148MapBitToLevel()
*
* RETURNS: 0 or bitmask of bit in registers (1 - 31)
*
*/
static u_int32 sysTsi148MapLevelToBit
(
	int level /* interrupt level */
)
{
	int i;
	for( i = 0; i < SYSTSI148_INTLEVELBITMAPSIZE; i++ ) {
		if( level == G_Tsi148_IntLevelBitMap[i].level )
			return( G_Tsi148_IntLevelBitMap[i].bit );
	}

	return( 0 );
} /* sysTsi148MapLevelToBit */

/*******************************************************************************
*
* sysTsi148MapBitToLevel - map given bit in registers to the corresp. IRQ level
*
* This routine maps a given bit from the INTx registers to the
* coresponding IRQ level.
*
* SEE ALSO: sysTsi148MapLevelToBit()
*
* RETURNS: 0 or IRQ level
*
*/
static int sysTsi148MapBitToLevel
(
	u_int32 bit		/* mask of bit in registers */
)
{
	int i;
	for( i = 0; i < SYSTSI148_INTLEVELBITMAPSIZE; i++ ) {
		if( bit == G_Tsi148_IntLevelBitMap[i].bit )
			return( G_Tsi148_IntLevelBitMap[i].level );
	}
	return( 0 );
} /* sysTsi148MapBitToLevel */


/*******************************************************************************
*
* sysTsi148IntEnable - enable/disable a specific TSI148 IRQ
*
* This routine enables a specific TSI148 IRQ level.
* Levels may be specified as would typically be called with intConnect()
*
* SEE ALSO: sysTsi148MapLevelToBit(); sysTsi148MapBitToLevel();
*
* RETURNS:
*
*/
static void sysTsi148IntEnable
(
	int level,		/* interrupt (INT level) to enable */
	int enable		/* 1: enable; 0: disable */
)
{
	u_int32	 bit = sysTsi148MapLevelToBit( level );

	TSI148DBG_3((DBH,"%sable Level %d, bit 0x%08x\n", (int)(enable? "en":"dis"),level,bit));
	if( enable )
	{
		SYSTSI148_CTRL_SETMASK( 32, lcsr.inteo, bit );
		SYSTSI148_CTRL_SETMASK( 32, lcsr.inten, bit );
	}
	else
	{
		SYSTSI148_CTRL_CLRMASK( 32, lcsr.inten, bit );
		SYSTSI148_CTRL_CLRMASK( 32, lcsr.inteo, bit );
	}
	return;
}

/*---------------------------------+
|   Reset Registers                |
+---------------------------------*/
/* Windows are left untouched for PPC (SetUp by MenMon)
* or disabled for x86 they can be setUp later using separate function
*
* RETURNS:
*
*/
static void sysTsi148ResetRegs( void )
{
	/* SYSTSI148_CTRL_WRITE(8,pcfs.mxla,0x54);
	   SYSTSI148_CTRL_WRITE(8,pcfs.mngn,0x01); */
	SYSTSI148_CTRL_WRITE(32, pcfs.pcixcap, 0x07007c00);

	/*--------------------+
	 |  RMW functions     |
	 +-------------------*/
	SYSTSI148_CTRL_WRITE(32, lcsr.rmwen, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.rmwau, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.rmwal, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.rmwc,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.rmws,  0x00000000);

	#if	(CPU_FAMILY == I80X86)
	/*--------------------------+
	 | In-/Outbound Translation |
	 +-------------------------*/
	/* Outbound Translation */
	{
		u_int32 i;
		for(i=0; i < TSI148_OUTBOUND_NO; i++) {
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otat,  0x00000000); /* disable window first */
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otsau, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otsal, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].oteau, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].oteal, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otofu, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otofl, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.outbound[i].otbs,  0x00000000);
		}
	}
	/* Inbound Translation */
	{
		u_int32 i;
		for(i=0; i < TSI148_INBOUND_NO; i++) {
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].itat,  0x00000000); /* disable window first */
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].itsau, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].itsal, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].iteau, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].iteal, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].itofu, 0x00000000);
			SYSTSI148_CTRL_WRITE(32, lcsr.inbound[i].itofl, 0x00000000);
		}
	}
	#endif	/* I80X86 */

	/*---------------------+
	 |  Outbound functions |
	 +--------------------*/

	/* VMEbus Control */
	SYSTSI148_CTRL_WRITE(32, lcsr.vmctrl, 0x00000707);
	SYSTSI148_CTRL_WRITE(32, lcsr.vctrl,  0x00000008);
	SYSTSI148_CTRL_WRITE(32, lcsr.vstat,  0x00008000);

	/*--------------------+
	 |  VME Filters       |
	 +-------------------*/
	/* VMEFL: The [ACKD] field, bits [25:24] must be set to 00 (or `slow')
	 * as per errata 2.6 "VMEbus corrupted write during wraparound RMW".
	 * This is necessary as the RMW version of sysBusTas is implemented using
	 * the wraparound RMW for the master node in a shared memory configuration.
	 */
	SYSTSI148_CTRL_WRITE(32, lcsr.vmefl, 0x00000f00);

	/* VME Exception: VME Exception Attributes (VESCL=1) */
	SYSTSI148_CTRL_WRITE(32, lcsr.veat,  0x20000000); /* VMEbus Exception Status Clear */

	/*  PCI Error: PCI Error Status */
	SYSTSI148_CTRL_WRITE(32, lcsr.edpat, 0x20000000); /* Error Diagnostic PCI/X Clear */

	/*--------------------+
	 |  Inbound functions |
	 +-------------------*/
	/* Inbound Translation GCSR */
	SYSTSI148_CTRL_WRITE(32, lcsr.gcsrat, 0x00000000); /* disable first */
	SYSTSI148_CTRL_WRITE(32, lcsr.gbau,   0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.gbal,   0x00000000);

	/* Inbound Translation CRG */
	SYSTSI148_CTRL_WRITE(32, lcsr.crgat,  0x00000000); /* disable first */
	SYSTSI148_CTRL_WRITE(32, lcsr.cbau,   0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.cbal,   0x00000000);

	/* Inbound Translation CR/CSR */
	SYSTSI148_CTRL_WRITE(32, lcsr.crou,   0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.crol,   0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.crat,   0x00000000);

	/* Inbound Translation Location Monitor */
	SYSTSI148_CTRL_WRITE(32, lcsr.lmat,   0x00000000); /* disable first */
	SYSTSI148_CTRL_WRITE(32, lcsr.lmbau,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.lmbal,  0x00000000);

	/*----------------------+
	 |  Interrupt functions |
	 +---------------------*/
	/* Local Bus Interrupt Control */
	SYSTSI148_CTRL_WRITE(32, lcsr.inten, 0x00000000);	/* disable interrupts */
	SYSTSI148_CTRL_WRITE(32, lcsr.inteo, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.intc,  0x03ff3f00);
	SYSTSI148_CTRL_WRITE(32, lcsr.intm1, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.intm2, 0x00000000);

	/* VMEbus Interrupt Control */
	SYSTSI148_CTRL_WRITE(32, lcsr.bcu64, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.bcl64, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.bpgtr, 0x00000022);
	SYSTSI148_CTRL_WRITE(32, lcsr.bpctr, 0x000003e8);
	SYSTSI148_CTRL_WRITE(32, lcsr.vicr,  0x0000000f);

	/*----------------------+
	 |  DMA controllers     |
	 +---------------------*/
	/* 0 */
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dctl,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dsau,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dsal,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].ddau,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].ddal,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dsat,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].ddat,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dnlau, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dnlal, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].dcnt,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[0].ddbs,  0x00000000);

	/* 0 */
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dctl,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dsau,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dsal,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].ddau,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].ddal,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dsat,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].ddat,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dnlau, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dnlal, 0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].dcnt,  0x00000000);
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[1].ddbs,  0x00000000);
}



/*******************************************************************************
*
* sysTsi148DmaInt - handle mailbox interrupt
*
* This routine handles an DMA interrupt.
*
* RETURNS: N/A.
*/

static void sysTsi148DmaInt
(
	int ctrlNum
)
{
	SYSTSI148_DMA_CTRL_HDL *dmaHdl;

	TSI148IDBG_1((DBH,">>> sysTsi148DmaInt: %d\n", ctrlNum ));
	dmaHdl = &G_hdl.dmaHdl[ctrlNum];
	if( (int)dmaHdl->userIntHandler == WAIT_FOREVER ) {
		semGive (dmaHdl->dmaIrqSem);
	} else if( dmaHdl->userIntHandler != NULL) {
		dmaHdl->userIntHandler(ctrlNum);
	}
	semGive( dmaHdl->dmaUsrHdlSem );

	TSI148IDBG_1((DBH,"<<< sysTsi148DmaInt: %d\n", ctrlNum ));
	return;
}

/***********************************************************************/
/** Initialize DMA controllers and connect their interrupt routines
 *
 */
static STATUS sysTsi148DmaInit( void )
{
	static const char *functionName = __FUNCTION__;
	int ctrlNum = 0;
	SYSTSI148_DMA_CTRL_HDL *dmaHdl;

	/* logMsg("%s: enter\n",(int)functionName,2,3,4,5,6); */

	for( ctrlNum = 0; ctrlNum < TSI148_DMACTL_NO; ctrlNum++ ) {
		dmaHdl = &G_hdl.dmaHdl[ctrlNum];

		dmaHdl->userIntHandler = NULL;
		dmaHdl->channelBusy = 0;
		dmaHdl->dmaUserSem = semBCreate (SEM_Q_PRIORITY, SEM_FULL); /* block user routine */
		if( dmaHdl->dmaUserSem == NULL ){
			logMsg("*** %s: Error initializing DMA user locking semaphore\n",(int)functionName,2,3,4,5,6);
			return( ERROR );
		}
		dmaHdl->dmaUsrHdlSem = semBCreate (SEM_Q_FIFO, SEM_FULL); 	 /* block access to userHandler */
		if( dmaHdl->dmaUsrHdlSem == NULL ){
			logMsg("*** %s: Error initializing DMA user handler locking semaphore\n",(int)functionName,2,3,4,5,6);
			return( ERROR );
		}
		dmaHdl->dmaIrqSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY); 	 /* wait for DMA IRQ */
		if( dmaHdl->dmaIrqSem == NULL ){
			logMsg("*** %s: Error initializing DMA IRQ locking semaphore\n",(int)functionName,2,3,4,5,6);
			return( ERROR );
		}
	}

	if( OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_DMA0, sysTsi148DmaInt, 0 ) )
		return( ERROR );
	sysTsi148IntEnable( SYSTSI148_INT_NUM_DMA0, 1 );

	if( OK != SYSTSI148_VME_INTCONNECT( SYSTSI148_INT_NUM_DMA1, sysTsi148DmaInt, 1 ) )
		return( ERROR );
	sysTsi148IntEnable( SYSTSI148_INT_NUM_DMA1, 1 );

	/* logMsg("%s: exit\n",(int)functionName,2,3,4,5,6); */

	G_hdl.dmaIsInit = TRUE;
	return( OK );
}

/***********************************************************************/
/** Start DMA with the scatter list setup by sysTsi148Dma
 *
 */
static int sysTsi148DmaStart
(
	u_int32 ctrlNum,
	u_int32 dctl, 				/* image of DMA control register */
	TSI148_DMA_LL_DESC *llDesc	/* DMA register image */
)
{
	u_int32 status;
	U_INT32_OR_64 dnla;
	u_int64 dnla64;
	u_int32 regH, regL;

	if( !G_hdl.dmaIsInit || ctrlNum >= TSI148_DMACTL_NO )
		return ERROR;

	status = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsta);
	if( status & TSI148_DSTA_BSY )
	{
		TSI148DBG_ERR((DBH,"*** sysTsi148DmaStart:DMA busy! %08x\n", status ));
		return -EBUSY;
	}

	/* check for linked list or direct mode DMA transfer */
	if( 0 != (llDesc->dnlau | (llDesc->dnlal & ~0x07)) ) {
		/* linked list mode */
		TSI148DBG_2((DBH," sysTsi148DmaStart: linked list DMA!\n" ));

		/* make sure MOD bit in DCTL register is cleared */
		dctl &= ~TSI148_DCTL_MOD;

		/* set linked list registers do LL_DESC,
		   has to be 8 byte aligned already */
		dnla   = (U_INT32_OR_64)llDesc;
		dnla64 = dnla;
		DISASS64(dnla64, regH, regL);

		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dnlau, regH );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dnlal, regL );
	} else {
		/* direct mode */
		TSI148DBG_2((DBH," sysTsi148DmaStart: direct mode DMA!\n" ));

		/* make sure MOD bit in DCTL register is set for direct mode */
		dctl |= TSI148_DCTL_MOD;

		/* just push image to controller */
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dsau,  llDesc->dsau );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dsal,  llDesc->dsal );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].ddau,  llDesc->ddau );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].ddal,  llDesc->ddal );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dsat,  llDesc->dsat );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].ddat,  llDesc->ddat );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dnlau, llDesc->dnlau );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dnlal, llDesc->dnlal );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dcnt,  llDesc->dcnt );
		SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].ddbs,  llDesc->ddbs );
	}

	/* enable corresponding DMA interrupt */
	SYSTSI148_CTRL_SETMASK( 32, lcsr.inteo, (TSI148_INTEX_DMA0 << ctrlNum) );
	SYSTSI148_CTRL_SETMASK( 32, lcsr.inten, (TSI148_INTEX_DMA0 << ctrlNum) );

	/* start DMA  */
	SYSTSI148_CTRL_WRITE(32, lcsr.dmactl[ctrlNum].dctl,  dctl |= TSI148_DCTL_DGO );

	return OK;
}

/***********************************************************************/
/** Abort DMA
 */
STATUS sysTsi148DmaAbort
(
	int ctrlNum
)
{
	SYSTSI148_DMA_CTRL_HDL *dmaHdl;
	u_int32 status;
	STATUS	error = OK;

	if( !G_hdl.dmaIsInit || ctrlNum >= TSI148_DMACTL_NO ){
		TSI148DBG_ERR((DBH,"*** sysTsi148DmaAbort: illegal channel %d\n", ctrlNum ));
		return ERROR;
	}

	dmaHdl = &G_hdl.dmaHdl[ctrlNum];
	semTake( dmaHdl->dmaUserSem, WAIT_FOREVER );

	status = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsta);
	if( status & TSI148_DSTA_BSY )
	{
		/* stop DMA, clear flags and disable DMA interrupt */
		SYSTSI148_CTRL_SETMASK( 32, lcsr.dmactl[ctrlNum].dctl, TSI148_DCTL_ABT );
		SYSTSI148_CTRL_CLRMASK( 32, lcsr.inten, (TSI148_INTEX_DMA0 << ctrlNum) );
		SYSTSI148_CTRL_CLRMASK( 32, lcsr.inteo, (TSI148_INTEX_DMA0 << ctrlNum) );
	} else {
		error = ERROR;
		TSI148DBG_ERR((DBH,"*** sysTsi148DmaAbort: channel %d not active\n", ctrlNum ));
	}

	semGive( dmaHdl->dmaUserSem );

	/* for some reason no interrupt is generated when an abort is performed
	 * ergo: semaphore for user Handler is not released. Do it here for now */
	semGive( dmaHdl->dmaUsrHdlSem );

	return OK;
}

/***********************************************************************/
/** Retreive status of current/last DMA transfer
 */
STATUS sysTsi148DmaStatus
(
	int ctrlNum,					 /* select DMA channel (0/1) */
	SYS_VME_DMA_STATUS *dmaStatus /* structure to be filled with current status of controller */
)
{
	SYSTSI148_DMA_CTRL_HDL *dmaHdl;

	if( !G_hdl.dmaIsInit || ctrlNum >= TSI148_DMACTL_NO ){
		TSI148DBG_ERR((DBH,"*** sysTsi148DmaStatus: illegal channel %d\n", ctrlNum ));
		return ERROR;
	}
	if( NULL == dmaStatus ) {
		TSI148DBG_ERR((DBH,"*** sysTsi148DmaStatus: illegal status struct pointer\n" ));
		return ERROR;
	}

	dmaHdl = &G_hdl.dmaHdl[ctrlNum];
	semTake( dmaHdl->dmaUserSem, WAIT_FOREVER );

	/* just get image from controller */
	dmaStatus->dctl			   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dctl);
	dmaStatus->dsta			   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsta);
	dmaStatus->dcsau		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dcsau);
	dmaStatus->dcsal		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dcsal);
	dmaStatus->dcdau		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dcdau);
	dmaStatus->dcdal		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dcdal);
	dmaStatus->dclau		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dclau);
	dmaStatus->dclal		   = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dclal);
	dmaStatus->dmaLlDesc.dsau  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsau);
	dmaStatus->dmaLlDesc.dsal  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsal);
	dmaStatus->dmaLlDesc.ddau  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].ddau);
	dmaStatus->dmaLlDesc.ddal  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].ddal);
	dmaStatus->dmaLlDesc.dsat  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dsat);
	dmaStatus->dmaLlDesc.ddat  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].ddat);
	dmaStatus->dmaLlDesc.dnlau = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dnlau);
	dmaStatus->dmaLlDesc.dnlal = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dnlal);
	dmaStatus->dmaLlDesc.dcnt  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].dcnt);
	dmaStatus->dmaLlDesc.ddbs  = SYSTSI148_CTRL_READ(32, lcsr.dmactl[ctrlNum].ddbs);

	semGive( dmaHdl->dmaUserSem );

	/* check some flags */
	if( TSI148_DSTA_DON == (dmaStatus->dsta & TSI148_DSTA_DON) ){
		dmaStatus->status = SYS_VME_DMA_STATE_DONE;
	} else if( TSI148_DSTA_BSY == (dmaStatus->dsta & TSI148_DSTA_BSY) ){
		dmaStatus->status = SYS_VME_DMA_STATE_BUSY;
	} else if( TSI148_DSTA_ERR == (dmaStatus->dsta & TSI148_DSTA_ERR) ){
		dmaStatus->status = SYS_VME_DMA_STATE_ERROR;
	} else if( TSI148_DSTA_ABT == (dmaStatus->dsta & TSI148_DSTA_ABT) ){
		dmaStatus->status = SYS_VME_DMA_STATE_ABT;
	} else if( TSI148_DSTA_PAU == (dmaStatus->dsta & TSI148_DSTA_PAU) ){
		dmaStatus->status = SYS_VME_DMA_STATE_PAU;
	}

	return( OK );
}

/******************************************************************************
*
* sysTsi148Dma - performs a DMA.
*
* This routine performs a DMA of the following types:
*                PCI -> VME (main memory at PCI starts at 0x00000000)
*                VME -> PCI (main memory at PCI starts at 0x00000000)
*                Pattern -> VME
*                Pattern -> PCI
*
* The user can pass an user handler to indicate the action to be taken on
* completion of a DMA transfer:
* NULL: 		          - return immediately, no further action to be taken
* WAIT_FOREVER:           - wait until the DMA is finished before returning
* valid function pointer: - return immediately but call userHdl on completion
*                           !!! userHdl will be called on interrupt level !!!
*
* RETURNS: OK or ERROR
*/

STATUS sysTsi148Dma
(
	int chan,						/* select DMA channel (0/1) */
	SYS_VME_DMA_CTRL    *dmaCtrl,	/* control structure for DMA */
	SYS_VME_DMA_DESC *dmaDesc,	/* descriptor for DMA (src/dest/..) */
	SYS_VME_DMA_INTHANDLER userHandler,	/* userHandler */
	int toutTicks					/* timeout when channel is busy */
)
{
	DBGCMD(static const char *functionName = __FUNCTION__ ;)
	STATUS 					error 		  = ERROR;
	SYSTSI148_DMA_CTRL_HDL 	*dmaHdl		  = &G_hdl.dmaHdl[chan];
	SYS_VME_DMA_DESC 	*dmaDescFirst = dmaDesc;
	SYS_VME_DMA_DESC 	*dmaDescCurr;
	SYS_VME_DMA_DESC 	*loopDescP    = dmaDesc;
	u_int32 				loopDescAdv   = 0;
	TSI148_DMA_LL_DESC  	*dmaLlDesc;
	u_int32 				dctl;			/* image of DMA control register */
	u_int32					status = 0;

	if( !G_hdl.dmaIsInit || chan >= TSI148_DMACTL_NO ){
		TSI148DBG_ERR((DBH,"*** %s: illegal channel %d or DMA not initialized\n", functionName, chan ));
		return ERROR;
	}

	/* first part is independant of any HW, no locking needed */
	TSI148DBG_1((DBH,"%s src/dst (%s)0x%08x_%08x/(%s)0x%08x_%08x size %08x\n", functionName,
				(SYS_VME_DMA_PCIX == dmaDesc->srcAttr.dmaType) ? "PCI" : ((SYS_VME_DMA_VME == dmaDesc->srcAttr.dmaType) ? "VME" : "PATTERN"),
				(u_int32)((dmaDesc->srcAddr & 0xffffffff00000000LL)>>32),(u_int32)((dmaDesc->srcAddr & 0x00000000ffffffffLL)),
				(SYS_VME_DMA_PCIX == dmaDesc->dstAttr.dmaType) ? "PCI" : "VME",
				(u_int32)((dmaDesc->dstAddr & 0xffffffff00000000LL)>>32),(u_int32)((dmaDesc->dstAddr & 0x00000000ffffffffLL)),
				dmaDesc->dmaSize ));

	/* first parse dmaCtrl to compose image for DMA control register */
	dctl = sysTsi148BuildDctlImage( dmaCtrl );

	/* compose register image
	 * - for linked lists follow links to the last one
	 * - the descriptors will be 8byte aligned here
	 */
	SYSTSI148_DMA_LLDESC_ALIGN8BYTE( dmaLlDesc, dmaDescFirst );

	error = sysTsi148BuildDmaRegImage( dmaDescFirst, dmaLlDesc );
	if( error != OK ) {
		TSI148DBG_ERR((DBH,"*** %s failed to build DMA register image!\n", functionName ));
		goto CLEANUP_NO_SEM_GIVE;
	}

	/* flush image to RAM. TSI148 will take it directly from there for linked list transfers */
	cacheFlush(DATA_CACHE, (void*) dmaLlDesc, sizeof(TSI148_DMA_LL_DESC));

	dmaDescCurr = dmaDescFirst->next;

	/* now handle linked list of descriptors if any */
	while( NULL != dmaDescCurr ){

		/* detect loops */
		if( loopDescP == dmaDescCurr ) {
			/* we detected a circular linked list! Abort here */
			logMsg("*** %s detected a circular linked DMA list!\n",(int)__FUNCTION__, 2,3,4,5,6);
			TSI148DBG_ERR((DBH,"*** %s detected a circular linked DMA list!\n", functionName ));
			break;
		}

		/* align image address to 8byte here */
		SYSTSI148_DMA_LLDESC_ALIGN8BYTE( dmaLlDesc, dmaDescCurr );

		error = sysTsi148BuildDmaRegImage( dmaDescCurr, dmaLlDesc );
		if( error != OK ) {
			TSI148DBG_ERR((DBH,"*** %s failed to build DMA register image!\n", functionName ));
			goto CLEANUP_NO_SEM_GIVE;
		}

		/* flush image to RAM. TSI148 will take it directly from there for linked list transfers */
		cacheFlush(DATA_CACHE, (void*) dmaLlDesc, sizeof(TSI148_DMA_LL_DESC));

		dmaDescCurr = dmaDescCurr->next;

		/* loopDescP Pointer runs only half as fast as the list is processed */
		if( 0 != (loopDescAdv++ & 0x1) ){
			loopDescP   = dmaDescCurr;
		}
	}

	/* when a circular loop was detected don't do any DMA transfers */
	if( loopDescP == dmaDescCurr ) {
		/* we detected a circular linked list! Abort here */
		logMsg("*** %s detected a circular linked DMA list! No transfer is initiated!\n",(int)__FUNCTION__, 2,3,4,5,6);
		TSI148DBG_ERR((DBH,"*** %s detected a circular linked DMA list! No transfer is initiated!\n", functionName ));
		error = ERROR;
		goto CLEANUP_NO_SEM_GIVE;
	}

	/* now we need to lock, the following may touch HW, ... */
	error = semTake( dmaHdl->dmaUserSem, toutTicks );
	if( OK != error )
	{
		TSI148DBG_ERR((DBH,"*** %s semTake user chan %d\n", functionName, chan ));
		goto CLEANUP_NO_SEM_GIVE;
	}

	/* Only proceed when channel is not busy */
	status = SYSTSI148_CTRL_READ(32, lcsr.dmactl[chan].dsta);
	if( 0 != (status & TSI148_DSTA_BSY) )
	{
		TSI148DBG_ERR((DBH,"*** %s: dmaStart: DMA %d busy! %08x\n", functionName, chan, status ));
		error = -EBUSY;
		goto CLEANUP;
	}

	/* user is already informed about last finish? */
	error = semTake( dmaHdl->dmaUsrHdlSem, toutTicks );
	if( OK != error )
	{
		TSI148DBG_ERR((DBH,"*** %s semTake for user handler, chan %d\n", functionName, chan ));
		goto CLEANUP;
	}

	/* If so save user handler for current transfer */
	dmaHdl->userIntHandler = userHandler;

	/* write the assembled register images to HW */
	/* align image address to 8byte, is also done when turkey is filled! */
	SYSTSI148_DMA_LLDESC_ALIGN8BYTE( dmaLlDesc, dmaDescFirst );
	error = sysTsi148DmaStart( chan, dctl, dmaLlDesc );
	if( OK != error )
	{
		TSI148DBG_ERR((DBH,"*** %s sysTsi148DmaStart chan %d failed\n", functionName, chan ));
		semGive( dmaHdl->dmaUsrHdlSem );

		goto CLEANUP;
	}

	/* DMA started
	 * now either wait for ready or return to user */
	if( WAIT_FOREVER == (int)userHandler ) {
		TSI148DBG_2((DBH,"%s waiting for DMA Transfer on chan %d to finish\n", functionName, chan ));
		semTake( dmaHdl->dmaIrqSem, WAIT_FOREVER );

		status = SYSTSI148_CTRL_READ(32, lcsr.dmactl[chan].dsta);
		if( TSI148_DSTA_DON == (status & TSI148_DSTA_DON) ) {
			TSI148DBG_2((DBH,"%s DMA Transfer on chan %d finished successfully\n", functionName, chan ));
		} else {
			TSI148DBG_ERR((DBH,"*** %s DMA Transfer on chan %d finished (status = 0x%08x)\n", functionName, chan, status ));
			error = ERROR;
		}
	} else if( NULL == userHandler  ) {
		TSI148DBG_2((DBH,"%s no user handler installed, returning\n", functionName, chan ));
	}

CLEANUP:
	semGive( dmaHdl->dmaUserSem );
CLEANUP_NO_SEM_GIVE:
	TSI148DBG_1((DBH," %s DMA Transfer on chan %d finished (error = 0x%08x)\n", functionName, chan, error ));
	return( error );
}

/******************************************************************************
*
* sysTsi148BuildDctlImage
*
* This function generates a valid DMA control register image from an
* SYS_VME_DMA_CTRL structure
*
* RETURNS: OK or ERROR
*/
static STATUS sysTsi148BuildDctlImage
(
	SYS_VME_DMA_CTRL    * dmaCtrl
)
{
	u_int32 dctl = 0; /* start with empty image */
	u_int8 mask;

	/* parse parameters one by one */
	if( TRUE == dmaCtrl->vfar ) {
		dctl |= TSI148_DCTL_VFAR;
	}

	if( TRUE == dmaCtrl->pfar ) {
		dctl |= TSI148_DCTL_PFAR;
	}

	/* VMEbus block size */
	for (mask = 0; mask <= 7; mask++){
		if( (32 * (1 << mask)) >= (dmaCtrl->vBlkSize ))
			break;
	}
	dctl |= (mask << TSI148_DCTL_VBKS_BIT);

	if( 0 == dmaCtrl->vbot ) {
		mask = 0;
	} else {
		for (mask = 1; mask <= 7; mask++){
			if( (1 << (mask-1)) >= (dmaCtrl->vbot ))
				break;
		}
	}
	dctl |= (mask << TSI148_DCTL_VBOT_BIT);

	/* PCI bus block size */
	for (mask = 0; mask <= 7; mask++){
		if( (32 * (1 << mask)) >= (dmaCtrl->pBlkSize ))
			break;
	}
	dctl |= (mask << TSI148_DCTL_PBKS_BIT);

	if( 0 == dmaCtrl->pbot ) {
		mask = 0;
	} else {
		for (mask = 1; mask <= 7; mask++){
			if( (1 << (mask-1)) >= (dmaCtrl->pbot ))
				break;
		}
	}
	dctl |= (mask << TSI148_DCTL_PBOT_BIT);

	TSI148DBG_4((DBH,"sysTsi148BuildDctlImage: compiled dctlx: 0x%08x\n", dctl ));
	return( dctl );

}

/******************************************************************************
*
* sysTsi148ParseSrcDestAttrib
*
* This function parses the SYS_VME_DMA_ATTR structures for the DMA source and
* destination attribute registers
*
* RETURNS: OK or ERROR
*/
static STATUS sysTsi148ParseSrcDestAttrib
(
	BOOL src,				/* 1: a source descriptor is parsed; 0: dest. descriptor */
	SYS_VME_DMA_ATTR  *dmaAttr,	/* pointer to structure to fill */
	u_int32 *dmaAttrImg			/* pointer to Source or Destination Attribute image */
)
{
	*dmaAttrImg = 0; /* start with clean image */
	if( TRUE == src ) {
		switch( dmaAttr->dmaType ) {
		case(SYS_VME_DMA_PCIX):
			*dmaAttrImg |= TSI148_DXAT_TYP_PCIX; break;
		case(SYS_VME_DMA_VME):
			*dmaAttrImg |= TSI148_DXAT_TYP_VME; break;
		case(SYS_VME_DMA_PATTERN_SZ32_I):
			*dmaAttrImg |= TSI148_DXAT_TYP_PATTERN_SZ32_I;  break;
		case(SYS_VME_DMA_PATTERN_SZ32_NI):
			*dmaAttrImg |= TSI148_DXAT_TYP_PATTERN_SZ32_NI; break;
		case(SYS_VME_DMA_PATTERN_SZ8_I):
			*dmaAttrImg |= TSI148_DXAT_TYP_PATTERN_SZ8_I;   break;
		case(SYS_VME_DMA_PATTERN_SZ8_NI):
			*dmaAttrImg |= TSI148_DXAT_TYP_PATTERN_SZ8_NI;  break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA type 0x%x for source\n", dmaAttr->dmaType ));
			return ERROR;
		}
	} else { /* destination descriptor, only VME and PCI applicable */
		switch( dmaAttr->dmaType ) {
		case(SYS_VME_DMA_PCIX):
			*dmaAttrImg |= TSI148_DXAT_TYP_PCIX; break;
		case(SYS_VME_DMA_VME):
			*dmaAttrImg |= TSI148_DXAT_TYP_VME;  break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA type 0x%x for destination\n", dmaAttr->dmaType ));
			return ERROR;
		}
	}

	/* only parse the folowing fields when the type is VME */
	if( SYS_VME_DMA_VME == dmaAttr->dmaType ) {
		switch( dmaAttr->dmaTm ) {
		case(SYS_VME_TM_SCT):
			*dmaAttrImg |= TSI148_DXAT_TM_SCT; break;
		case(SYS_VME_TM_BLT):
			*dmaAttrImg |= TSI148_DXAT_TM_BLT; break;
		case(SYS_VME_TM_MBLT):
			*dmaAttrImg |= TSI148_DXAT_TM_MBLT; break;
		case(SYS_VME_TM_2EVME):
			*dmaAttrImg |= TSI148_DXAT_TM_2EVME; break;
		case(SYS_VME_TM_2ESST160):
			*dmaAttrImg |= (TSI148_DXAT_TM_2ESST | TSI148_DXAT_SSTM_2ESST160); break;
		case(SYS_VME_TM_2ESST267):
			*dmaAttrImg |= (TSI148_DXAT_TM_2ESST | TSI148_DXAT_SSTM_2ESST267); break;
		case(SYS_VME_TM_2ESST320):
			*dmaAttrImg |= (TSI148_DXAT_TM_2ESST | TSI148_DXAT_SSTM_2ESST320); break;
		case(SYS_VME_TM_2ESSTB):
			*dmaAttrImg |= TSI148_DXAT_TM_2ESSTB; break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA transfer mode 0x%x\n", dmaAttr->dmaTm ));
			return ERROR;
		}

		switch( dmaAttr->dmaDm ) {
		case(SYS_VME_D16):
			*dmaAttrImg |= TSI148_DXAT_DBW_16; break;
		case(SYS_VME_D32):
			*dmaAttrImg |= TSI148_DXAT_DBW_32; break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA data mode 0x%x\n", dmaAttr->dmaDm ));
			return ERROR;
		}

		switch( dmaAttr->dmaAm ) {
		case(SYS_VME_A16):
			*dmaAttrImg |= TSI148_DXAT_AM_16; break;
		case(SYS_VME_A24):
			*dmaAttrImg |= TSI148_DXAT_AM_24; break;
		case(SYS_VME_A32):
			*dmaAttrImg |= TSI148_DXAT_AM_32; break;
		case(SYS_VME_A64):
			*dmaAttrImg |= TSI148_DXAT_AM_64; break;
		case(SYS_VME_CRCSR):
			*dmaAttrImg |= TSI148_DXAT_AM_CRCSR; break;
		case(SYS_VME_UM1):
			*dmaAttrImg |= TSI148_DXAT_AM_UM1; break;
		case(SYS_VME_UM2):
			*dmaAttrImg |= TSI148_DXAT_AM_UM2; break;
		case(SYS_VME_UM3):
			*dmaAttrImg |= TSI148_DXAT_AM_UM3; break;
		case(SYS_VME_UM4):
			*dmaAttrImg |= TSI148_DXAT_AM_UM4; break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA address mode 0x%x\n", dmaAttr->dmaAm ));
			return ERROR;
		}

		switch( dmaAttr->dmaAc ) {
		case(SYS_VME_AC_DATA):
			break; /* nothing to be done */
		case(SYS_VME_AC_NPRIV):
			break; /* nothing to be done */
		case(SYS_VME_AC_SUPR):
			*dmaAttrImg |= TSI148_DXAT_SUP; break;
		case(SYS_VME_AC_PGM):
			*dmaAttrImg |= TSI148_DXAT_PGM; break;
		default:
			TSI148DBG_ERR((DBH,"*** sysTsi148ParseSrcDestAttrib: illegal DMA access mode 0x%x\n", dmaAttr->dmaAc ));
			return ERROR;
		}

	}
	return OK;
}
/******************************************************************************
*
* sysTsi148BuildDmaRegImage
*
* This function generates a valid DMA registers image from an
* SYS_VME_DMA_DESC structure
*
* WARNING:
* ! It will already fill the Next Link Address registers even though the next
* ! descriptor is not yet valid! When the parse fails don't do any transfer!!
*
* RETURNS: OK or ERROR
*/
static STATUS sysTsi148BuildDmaRegImage
(
	SYS_VME_DMA_DESC *dmaDesc,	/* descriptor for DMA (src/dest/..) */
	TSI148_DMA_LL_DESC  *dmaLlDesc	/* pointer to structure to fill */
)
{

	/* begin with an empty descriptor, just in case */
	bzero( (char*)dmaLlDesc, sizeof(TSI148_DMA_LL_DESC) );

	/* parse dmaDesc one by one */
	/* source address */
	DISASS64(dmaDesc->srcAddr, dmaLlDesc->dsau,dmaLlDesc->dsal);

	/* destination address */
	DISASS64(dmaDesc->dstAddr,  dmaLlDesc->ddau,dmaLlDesc->ddal);

	/* Source Attribute */
	if( OK != sysTsi148ParseSrcDestAttrib( TRUE, &dmaDesc->srcAttr, &dmaLlDesc->dsat ) )
		return ERROR;

	/* Destination Attribute */
	if( OK != sysTsi148ParseSrcDestAttrib( FALSE, &dmaDesc->dstAttr, &dmaLlDesc->ddat ) )
		return ERROR;

	/* DMA Next Link Address */
	if( NULL == dmaDesc->next ) {
		/* last order, set flag */
		dmaLlDesc->dnlal |= TSI148_DNLAL_LLA;
	} else {
		/* another transfer follows, set link address accordingly
		 * even though descriptor pointed to is not yet valid */
		TSI148_DMA_LL_DESC  *dmaLlDesc;
		U_INT32_OR_64 		llDescP;
		u_int64 			llDescP64;

		/* align LL Descriptor to 8byte */
		SYSTSI148_DMA_LLDESC_ALIGN8BYTE( dmaLlDesc, dmaDesc );
		llDescP   = (U_INT32_OR_64)dmaLlDesc;
		llDescP64 = (u_int64)llDescP;
		DISASS64(llDescP64, dmaLlDesc->dnlau,dmaLlDesc->dnlal);
	}

	/* DMA Count */
	dmaLlDesc->dcnt = dmaDesc->dmaSize;

	/* DMA Destination Broadcast Select */
	dmaLlDesc->ddbs = dmaDesc->dmaDbs2esst;

	return( OK );
}


/******************************************************************************
*
* sysTsi148ShowInitStatus
*
* This function displays the init status and slot0 function enable status.
*
* RETURNS: N/A
*/
void sysTsi148ShowInitStatus( void )
{
	printf(" TSI148 %sinitialized - slot0 function %sabled\n",
		    ( G_hdl.isInit == 2 )       ? "" : "not ",
		    sysVmeIsSystemController()  ? "en" : "dis"
		  );
}

#ifdef INCLUDE_SHOW_ROUTINES
	#include "sysTsi148_show.c"
#endif /* INCLUDE_SHOW_ROUTINES */




