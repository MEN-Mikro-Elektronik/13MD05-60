/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sysMenPci2VmePldZ002.c
 *      Project: PLDZ002 PCI2VME bridge driver for VxWorks
 *
 *       Author: uf/ts
 *    $Revision: Git
 *
 *  Description: Support routines MEN's PCI to VME bridge PLDZ002
 *
 *     Required:  	-
 *     Switches:  SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE (non X86 only)
 *		  			- if present, don't try to read "vmeirqen" system parameter
 *                MEN_A021  
 *                  - build driver for MEN A21 (different addressmap)
 *                
 * MEN_VME_VXB_SUPPORT	- select vxBus driver model, also uses vxbPciConfig... functions 
 *                        instead legacy ones. OSS_VXBUS_SUPPORT is not used by intention to keep the VME
 *                        driver support independent from OSS settings.
 * MEN_A021				- BSP build switch to tell the driver about the special, different address maps
 *                        on A021 FPGA. Here for example the nonswapped address spaces do not exist.
 *                        
 *-------------------------------[ History ]---------------------------------
 *
 * 2015/05/07  ts:  added support for A21 FPGA. There PLDZ002 is part of the chameleon table.
 *
 * end of mcvs controlled versioning...
 * $Log: sysMenPci2VmePldZ002.c,v $
 * Revision 1.14  2009/02/06 21:07:22  cs
 * R: no API was provided for setting bus release and request modi
 * M: implement API for setting bus release and request modi
 *     (sysVmeBusReleaseMode(), sysVmeBusRequesterMode())
 *
 * Revision 1.13  2006/10/10 10:48:04  cs
 * added some globals for BSP test routines (10F001N60 BSP)
 *
 * Revision 1.12  2006/09/11 10:41:57  cs
 * fixed to avoid warnings:
 *    - removed define of DBG in this file
 *    - cosmetics
 *
 * Revision 1.11  2006/04/27 10:41:35  cs
 * cosmetics for APIGEN and VxWorks 6.x (function prototypes)
 *
 * Revision 1.10  2005/12/19 13:38:56  UFranke
 * cosmetics
 *
 * Revision 1.9  2005/12/05 11:53:48  UFranke
 * added
 *  + A24D32 and A24D32 swapped in sysAddrInVmeRange()
 *
 * Revision 1.8  2005/11/30 12:51:19  UFranke
 * changed
 *  - sysVmeA32SlaveAddressSet(), sysVmeA24SlaveAddressSet() supports
 *    finer resolution of the > R18 FPGA
 * fixed 
 *  - sysVmeLocMonSetup() compare address for big endian processors
 *
 * Revision 1.7  2005/11/25 15:01:35  UFranke
 * intermediate checkin
 * changed
 *  - bridge revision must be greater than 17
 *    On the PLD rev. > 17 the last 2kB of the SRAM is used now as
 *    PLD registers i.e. mailbox and DMA buffer descriptors
 *  - sysVmeProbe() checks if address is inside of a VME window
 *  - renamed sysMenPci2VmeSetA32Mode() into sysMenPci2VmeSetA32MasterPrefix
 * added
 *  + DMA support
 *  + access directly to main memory from VMEbus
 *  + sysAddrInVmeRange() added A32 and swapped windows
 * fixed
 *  - sysVmeBusToLocalAdrs() highest possible address returns an error
 *  - documentation sysMenPci2VmeSetA32MasterPrefix()
 *    set the upper three bits of the A32 address
 *
 * Revision 1.6  2005/07/05 12:28:49  kp
 * removed some unwanted logMsg() calls.
 * + switch SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE
 *
 * Revision 1.5  2005/05/31 15:44:37  AJendrusch
 * added: sysMenPci2VmeSram(); sysMenPci2VmeSetA32Mode();
 * modified: sysVmeBusToLocalAdrs (at VME_AM_EXT_USR_DATA)
 *
 * Revision 1.4  2005/05/09 14:28:35  AJendrusch
 * Integrate functionality for x86 CPUs
 *
 * Revision 1.3  2004/02/05 10:38:00  UFranke
 * added
 *  + sysVmeA32SlaveAddressSet(), sysVmeA24SlaveAddressSet() and
 *    sysVmeA16SlaveAddressSet()
 *
 * Revision 1.2  2003/12/19 12:30:47  UFranke
 * cosmetics
 *
 * Revision 1.1  2003/12/18 15:59:03  UFranke
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char *VME2PCI_PLDZ002_RCSid = "$Id: sysMenPci2VmePldZ002.c,v 1.15beta 2015/05/12 Git $";

/*
  DESCRIPTION
  This driver provides MEN PCItoVME-specific routines.
  =======

  The bridge provides 5 windows to the VME bus one A16 to the
  bridge control registers i.e. mailbos registers. One A24 to
  the bridge SRAM, one A32 to the bridge SRAM, one A24 to the
  main memory and one A32 to the same space in the main memory.

  ===============================================================
  VMEbus
  ===============================================================
  ||               ||        ||            ||        ||
  +=======+        +=======+ +=======+     +=======+ +=======+
  | A16   |        | A24   | | A32   |     | A24   | | A32   |
  |Window |        | SRAM  | | SRAM  |     | MEM   | | MEM   |
  |       |        |Window | |Window |     |Window | |Window |
  +=======+        +=======+ +=======+     +=======+ +=======+
  ||               ||        ||            ||        ||
  +=======+        +=================+     +=================+
  |Bridge |        |                 |     |                 |
  |Control|        |  Bridge         |     |  Translation    |
  |i.e.   |        |  SRAM           |     |  Window         |
  |Mailbox|        |                 |     |                 |
  +=======+        +=================+     +=================+
  ||
  ||  PCI
  ||  Bus
  ||
  +=======+
  |CPU    |
  |MAIN   |
  |MEMORY |
  |       |
  +=======+


*/

#include "vxWorks.h"
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
#define  MAC_MEM_MAPPED
#include <MEN/maccess.h>
#ifdef MEN_A021
#include <MEN/pldz002-cham.h>
#else
#include <MEN/pldz002.h>
#endif
#include <MEN/vme4vx.h>

#include <MEN/dbg.h>
/* static DBG_HANDLE	 	*dbgHdl = 0; */
static unsigned long	dbgLev	= 0;

#define DBH				dbgHdl
#define DBG_MYLEVEL 	dbgLev

#include "sysLib_men.h"
#include "sysIntCtrl.h"
#include <drv/pci/pciConfigLib.h>	/* for PCI const     */
#include <drv/pci/pciIntLib.h>		/* for PCI interrupt */
#include <vxBusLib.h>				/* for vxbPciFindDevice etc, */
/* central functions exported from sysLib.c */
IMPORT VXB_DEVICE_ID sysGetPciCtrlID(int Inst);
IMPORT STATUS vxbPciFindDevice(VXB_DEVICE_ID busCtrlID,   
int           vendorId,    /* vendor ID */
int           deviceId,    /* device ID */
int           index,       /* desired instance of device */
int *         pBusNo,      /* bus number */
int *         pDeviceNo,   /* device number */
int *         pFuncNo      /* function number */
);   

#ifndef MEN_PLD_Z002
#error "*** missing define MEN_PLD_Z002"
#else
#include "MEN/sysMenPci2Vme.h"
#endif

/*-------------------------------+
  |  DEFINES                     |
  +-----------------------------*/

#define PLDZ002_A32_SIZE		PLDZ002_A32D32_SIZE  /* 512 MB A32/D32 */
#define PLDZ002_A24_SIZE		PLDZ002_A24Dxx_SIZE
#define PLDZ002_A16_SIZE		PLDZ002_A16Dxx_SIZE

#define X86_INTERRUPT_OFFSET	0x20

#define VME4VX_SPC_NEEDED_PLD_REV	18

#define PLDZ002_MAX_DMA_SIZE	0x40000  /* max 256 kB per DMA */

#ifdef MEN_A021  /* see men_a021.h for defines */
# define Z002INTERN_A24D16_OFFS 	PLDZ002_MEN_A21_A24D16_SPACE
# define Z002INTERN_A24D32_OFFS     PLDZ002_MEN_A21_A24D32_SPACE
# define Z002INTERN_A16D16_OFFS  	PLDZ002_MEN_A21_A16D16_SPACE
# define Z002INTERN_A16D32_OFFS  	PLDZ002_MEN_A21_A16D32_SPACE
# define Z002INTERN_LRAM_OFFS    	PLDZ002_MEN_A21_LRAM_SPACE
# define Z002INTERN_CTRL_OFFS    	PLDZ002_MEN_A21_CTRL_SPACE
# define Z002INTERN_IACK_OFFS    	PLDZ002_MEN_A21_IACK_SPACE
#else /* the old, "well known" offsets as in PLDZ002 spec... */
# define Z002INTERN_A24D16_OFFS 	PLDZ002_A24D16_SPACE
# define Z002INTERN_A16D16_OFFS  	PLDZ002_A16D16_SPACE
# define Z002INTERN_A16D32_OFFS  	PLDZ002_A16D32_SPACE
# define Z002INTERN_LRAM_OFFS    	PLDZ002_LRAM_SPACE
# define Z002INTERN_CTRL_OFFS    	PLDZ002_CTRL_SPACE
# define Z002INTERN_IACK_OFFS    	PLDZ002_IACK_SPACE	
#endif

#define CPU_INT_LOCK(pData) (*pData = intCpuLock())
#define CPU_INT_UNLOCK(data) (intCpuUnlock(data))

#if (_BYTE_ORDER == _LITTLE_ENDIAN)
# define VME_CTRL_READ_D16(ma,reg)   MREAD_D16(ma,reg)
# define VME_CTRL_READ_D32(ma,reg)   MREAD_D32(ma,reg)
# define VME_CTRL_WRITE_D16(ma,reg,val)   MWRITE_D16(ma,reg,val)
# define VME_CTRL_WRITE_D32(ma,reg,val)   MWRITE_D32(ma,reg,val)
#else
# define VME_CTRL_READ_D16(ma,reg)   OSS_SWAP16(MREAD_D16(ma,reg))
# define VME_CTRL_READ_D32(ma,reg)   OSS_SWAP32(MREAD_D32(ma,reg))
# define VME_CTRL_WRITE_D16(ma,reg,val)   MWRITE_D16(ma,reg,OSS_SWAP16(val))
# define VME_CTRL_WRITE_D32(ma,reg,val)   MWRITE_D32(ma,reg,OSS_SWAP32(val))
#endif

/*-----------------------------+
  |  GLOBALS                     |
  +-----------------------------*/

/* for confort provide the VXB_DEVICE_ID of the VME FPGA in a global variable */

#ifdef MEN_VME_VXB_SUPPORT
VXB_DEVICE_ID G_vxbVmeId;
#endif

typedef struct
{
    int 	isInit;		/* flags sysMenPci2VmeInit() successful */
    int		littleEndian;   /* CPU is a little endian type */
#ifdef MEN_VME_VXB_SUPPORT
    VXB_DEVICE_ID busCtrlID;
#endif
    MACCESS base;		/* VME bridge registers */
    u_int32 sram;		/* local base of VME SRAM */
    u_int32 sramSize;	/* size of VME SRAM */
    u_int32 iack;		/* local base of VME IACK space */
    u_int32 a24d16;		/* local base of VME A24 space */
    u_int32 a16d16;		/* local base of VME A16 space */
    u_int32 a32d32;		/* local base of VME A32 space */
    u_int32 crcsr;      /* local base of VME CR/CSR space */
    u_int32 a32maBase;	/* upper 3 address bits on VMEbus */
    u_int32 a24d16sw;	/* local base of VME A24 space - swapped */
    u_int32 a16d16sw;	/* local base of VME A16 space - swapped */
    u_int32 a32d32sw;	/* local base of VME A32 space - swapped */
    u_int32 a24d32;		/* local base of VME A24 space */
    u_int32 a24d32sw;	/* local base of VME A24 space - swapped */
    u_int32 addrModifier;   /* address modifier */
                            /*  0 (00b) non-privileged data access (default) */
                            /*  1 (01b) non-privileged program access */
                            /*  2 (10b) supervisory data access */
                            /*  3 (11b) supervisory program access */
}PLDZ002_DESC;

/* ts@men this supports just one 16Z002 within a system but that should do _for now_ ... */
static PLDZ002_DESC G_pld;

/*--- variable to check the version of the FGPA ---*/ 
static char         G_chamVariant;
static int          G_chamRevision;

static FUNCPTR sysMailboxRoutine   = NULL;
static int sysMailboxArg = 0;
static int sysMailboxConnected = 0;
static SEMAPHORE G_dmaUserSem;

SEMAPHORE G_dmaIrqSem;

/*-----------------------------+
  |  EXTERN DECLARATIONS         |
  +-----------------------------*/
#ifdef MEN_BSP_TEST
MACCESS sysVmeRegs;		/* VME bridge registers */
u_int32 sysVmeSram;		/* local base of VME SRAM */
u_int32 sysVmeSramSize;	/* size of VME SRAM */
VXB_DEVICE_ID sysVmeVxCtrlID;	/* VXB_DEVICE_ID of PCIe controller with FPGA, e.g. A21 */
u_int32 sysVmeA24;		/* local base of VME A24 space */
u_int32 sysVmeA16;		/* local base of VME A16 space */
#endif /* MEN_BSP_TEST */

IMPORT SYS_INT_HANDLER_DESC * sysIntTbl [INTERRUPT_TABLESIZE];
IMPORT SYS_INT_HANDLER_DESC * sysVmeIntTbl [INTERRUPT_TABLESIZE];

/*-----------------------------+
  |  PROTOTYPES                  |
  +-----------------------------*/

void sysMenPci2VmeIntr(void);


__inline__ void PLDWRITE_D32( void *base, int offs, u_int32 val )
{
#ifdef _BIG_ENDIAN_
    MWRITE_D32(base,offs, OSS_SWAP32(val) );
#else
    MWRITE_D32(base,offs,val);
#endif
}

/***************************************************************************/
/* helpers to encapsulate PCI accesses on legacy and vxBus BSPs. 
 * Its assumed that sysVmeInit was called and G_pld.busCtrlID contains
 * the matching VXB_DEVICE_ID of the vxBus PCI controller driver instance.
 *
 */

/**************************************/
/* PCI config space read byte wrapper
 */
LOCAL STATUS men_vme_pcicfg_in_byte( u_int32 b, u_int32 d, u_int32 f, u_int32 idx, u_int8 *val ) 
{
    STATUS retCode;

    if (G_pld.busCtrlID == 0)
	return ERROR;

#ifdef MEN_VME_VXB_SUPPORT
    retCode = vxbPciConfigInByte( G_pld.busCtrlID, b, d, f, idx, val);
#else
    retCode = pciConfigInByte(  b, d, f, idx, val);
#endif
    return retCode;
}
/**************************************/
/* PCI config space read word wrapper
 */

LOCAL STATUS men_vme_pcicfg_in_word(  u_int32 b, u_int32 d, u_int32 f, int32 idx, u_int16 *val )
{
    STATUS retCode;
    if (G_pld.busCtrlID == 0)
	return ERROR;

#ifdef MEN_VME_VXB_SUPPORT
    retCode = vxbPciConfigInWord( G_pld.busCtrlID, b, d, f, idx, val);
#else
    retCode = pciConfigInWord(  b, d, f, idx, val);
#endif
    return retCode;
}

/**************************************/
/* PCI config space read long wrapper
 */
static STATUS men_vme_pcicfg_in_long( u_int32 b, u_int32 d, u_int32 f, int32 idx, UINT32 *val )
{
    STATUS retCode;
    if (G_pld.busCtrlID == 0)
	return ERROR;

#ifdef MEN_VME_VXB_SUPPORT
    retCode = vxbPciConfigInLong( G_pld.busCtrlID, b, d, f, idx, val);
#else
    retCode = pciConfigInLong( b, d, f, idx, val);
#endif
    return retCode;
}

/**************************************/
/* PCI config space write word wrapper
 */
static STATUS men_vme_pcicfg_out_word( u_int32 b, u_int32 d, u_int32 f, int32 idx, u_int16 val )
{
    STATUS retCode;
    if (G_pld.busCtrlID == 0)
	return ERROR;

#ifdef MEN_VME_VXB_SUPPORT
    retCode = vxbPciConfigOutWord( G_pld.busCtrlID, b, d, f, idx, val);
#else
    retCode = pciConfigOutWord( b, d, f, idx, val);
#endif
    return retCode;
}


/******************************************************************************
 *
 * sysMenPci2VmeIdent - returns the pointer to the ident string
 *
 * This routine returns the pointer to the ident string.
 *
 * RETURNS: pointer to the ident string
 *
 */
char* sysMenPci2VmeIdent( void )
{
    return( (char*) VME2PCI_PLDZ002_RCSid );
}


/* static wrapper for initializing either A21 or all other (standard-PLDZ002) devices  */
static STATUS menpci2vmeinit_intern( u_int32 bar0, u_int32 bar1, u_int32 bar2, u_int32 bar3, u_int32 bar4 )
{

/* For A21 the address offsets are different to all other PLDZ002 boards */
	
    /*  A16/A24/SRAM/CTRL. Values taken from last user manual */
    G_pld.a24d16 	= bar0 + Z002INTERN_A24D16_OFFS;
    G_pld.a16d16 	= bar0 + Z002INTERN_A16D16_OFFS;
    G_pld.sram 	    = bar0 + Z002INTERN_LRAM_OFFS;
    G_pld.base 	    = (MACCESS)(bar0 + Z002INTERN_CTRL_OFFS);
    G_pld.iack	    = bar0 + Z002INTERN_IACK_OFFS;
    G_pld.sramSize  = PLDZ002_LRAM_SIZE;

    /* A32 */
    G_pld.a32d32 	= bar1;
    
#ifdef MEN_A021
    G_pld.a24d32    = bar0 + Z002INTERN_A24D32_OFFS;
    /* CR/CSR */
    G_pld.crcsr     = bar2;
#else /* MEN_A021 */
    /* this isnt needed for A21, the vxbDrvMenFpga takes care of IRQs */
    /* A16/ A24 swapped */
    G_pld.a24d16sw 	= bar2 + PLDZ002_A24D16_SPACE;
    G_pld.a16d16sw 	= bar2 + PLDZ002_A16D16_SPACE;

    /* A32 swapped */
    G_pld.a32d32sw 	= bar3;    	
    G_pld.a32maBase = MREAD_D8(G_pld.base, PLDZ002_LONGADD );

    /* A24D32 + swapped */
    G_pld.a24d32sw 	= bar4+PLDZ002_A24D32SW_SPACE;
    G_pld.a24d32 	= bar4+PLDZ002_A24D32_SPACE;

    /* disable all VME interrupts first */
    MWRITE_D8(G_pld.base, PLDZ002_IMASK, 0 );

    /*--- connect the VME interrupt routine ---*/
    men_vme_pcicfg_in_byte( bus, dev, func, PCI_CFG_DEV_INT_LINE, &irq );

# if (CPU_FAMILY == I80X86)
    printf("\n---- sysMenPci2VmdInit IRQ:%d", irq);
    pciIntConnect( INUM_TO_IVEC((int)irq+X86_INTERRUPT_OFFSET), sysMenPci2VmeIntr, 0 );
    sysIntEnablePIC( irq );
# else
    intConnect( INUM_TO_IVEC((int)irq), sysMenPci2VmeIntr, 0 );
    intEnable( irq );

#  ifndef SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE
    {
	char parbuf[20];
	u_int32 val;
	/* enable VME interrupts as defined by MENMON */
	if( sysMmParaSearchKey( "vmeirqenb=", parbuf ) != NULL )
	    {
		sysStringToInt( 16, parbuf, &val );
		MWRITE_D8( G_pld.base, PLDZ002_IMASK, val );
	    }
    }
#  endif /* SYS_MEN_PCI2VME_NO_INITIAL_VME_IRQ_ENABLE */
# endif /* CPU_FAMILY == I80X86 */
#endif /* MEN_A021 */

   
#ifdef MEN_BSP_TEST
    sysVmeRegs		= G_pld.base;		/* VME bridge registers */
    sysVmeSram		= G_pld.sram;		/* local base of VME SRAM */
    sysVmeSramSize	= G_pld.sramSize;	/* size of VME SRAM */
    sysVmeA24		= G_pld.a24d16;		/* local base of VME A24 space */
    sysVmeA16		= G_pld.a16d16;		/* local base of VME A16 space */
    sysVmeVxCtrlID  = G_pld.busCtrlID;  /* the VXB_DEVICE_ID of the PCI controller with FPGA */
#endif /* MEN_BSP_TEST */

    MWRITE_D8( G_pld.base, PLDZ002_MSTR, PLDZ002_MSTR_IBERREN );

    semBInit( &G_dmaUserSem, SEM_Q_FIFO, SEM_FULL );  /* block user routine */
    semBInit( &G_dmaIrqSem,  SEM_Q_FIFO, SEM_EMPTY ); /* wait for DMA IRQ */

    G_pld.isInit = TRUE;
       
    return OK;
}

/******************************************************************************
 *
 * sysMenPci2VmeInit - initialize VME bridge PLDZ002 routines
 *
 * This routine tries to find the MEN PCI to VME bridge. If found,
 * it queries the base address registers 0 and sets up global vars.
 * It checks if the bridge revision is greater than 17.
 *
 * All VME interrupt levels and windows will be disabled with the exception of A015.
 * In the A015 BSP the VME IRQ enable settings as defined by MENMON EE-VME-IRQ command,
 * the A24 SRAM window and the A32 master prefix will be taken from the MENMON setting.
 *
 * The A013 has a BIOS only. Therefore the A24 SRAM window setting for shared memory
 * network will be done in sysMenPci2VmeProcNum0Init() for the A013.
 *
 * Connects and enables PCI interrupt for internal use.
 * 
 * addendum for A21: 
 * On A21 the environment is different, the whole PLDZ002 Registers are part of the general
 * chameleon FPGA. Therefore, two more build switches were introduced:
 * 
 * MEN_VME_VXB_SUPPORT	- select vxBus driver model, also uses vxbPciConfig... functions 
 *                        instead legacy ones
 * MEN_A021				- BSP build switch to tell the driver about the special, different address maps
 *                        on A021 FPGA. Here for example the nonswapped address spaces do not exist.
 *
 * NOTE: This is called from sysHwInit2()
 *
 * RETURNS: OK or ERROR if bridge not present
 *
 * SEE ALSO: sysMenPci2VmeProcNum0Init()
 *           sysVmePLDZ002setEndianMode() to change the endian mode
 *
 */

#ifdef MEN_VME_VXB_SUPPORT
int   	sysMenPci2VmeInit( VXB_DEVICE_ID pciVxbCtrlID )
#else
int   	sysMenPci2VmeInit( void )
#endif
{
    int bus=-1, dev=-1, func=-1;
    UINT32 bar0=0, bar1=0, bar2=0, bar3=0, bar4=0; 
    u_int16 command;
    
    bzero( (char*)&G_pld, sizeof(G_pld) );
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
    G_pld.littleEndian = 1;
#else
    G_pld.littleEndian = 0;
#endif

    DBG_MYLEVEL = OSS_DBG_DEFAULT;
    DBGINIT((NULL,&DBH));

    /* find the FPGA */
#if defined(MEN_VME_VXB_SUPPORT) && defined(MEN_A021)     
    if (pciVxbCtrlID == 0) {
    	printf("*** invalid VXB_DEVICE_ID of PCI controller with PLDZ002 FPGA (check PCI controller driver IDs in vxBusShow).\n");
    }
    
    if( vxbPciFindDevice(pciVxbCtrlID, CHAMELEON_PCI_VENID_MEN, CHAMELEON_PCI_DEV_ID, 0, &bus, &dev, &func ) != OK ) 
    {
        DBGWRT_ERR((DBH,"*** sysMenPci2VmeInit: FPGA not found! \n" ));            
	    return ERROR;
    }
    
    if( CHAMELEON_UNIT_NOT_FOUND == sysChameleonGlobalIdent( &G_chamVariant, &G_chamRevision, 0 ) )
    {
        return ERROR;
    }
    else {
        printf( "MEN A021 FPGA found at VXB_DEVICE ID 0x%x, bus %d dev %d func %d\n"
                "Chameleon Variant %c revision %d\n",
                pciVxbCtrlID, 
                bus, 
                dev, 
                func,
                G_chamVariant,
                G_chamRevision );
    }
    
    G_pld.busCtrlID = pciVxbCtrlID;
#else
    /* check for old FPGAs (0x1172/0x5056) */
    if( pciFindDevice( PLDZ002_VEN_ID, PLDZ002_DEV_ID, 0, &bus, &dev, &func ) == ERROR ) {
        DBGWRT_ERR((DBH,"*** sysMenPci2VmeInit: FPGA not found! \n", ));
	return ERROR;		/* no VME bridge found */
    }   
    DBGWRT_1((DBH,"sysMenPci2VmeInit: FPGA found at b/d/f %d/%d/%d, ok.\n", 
	bus, dev,func ));

    /* check bridge */
    men_vme_pcicfg_in_word( bus, dev, func, PCI_CFG_SUB_VENDER_ID, &vendor );
    men_vme_pcicfg_in_word( bus, dev, func, PCI_CFG_SUB_SYSTEM_ID, &device );
    men_vme_pcicfg_in_byte( bus, dev, func, PCI_CFG_REVISION,      &rev );

    /* on A21 the PLD revision is "always high enough" since that board is from 2013. 
       So the check is just needed on older boards. */
    if( (vendor != PLDZ002_SUBSYS_VEN_ID) || (device != PLDZ002_SUBSYS_ID) || rev < SYS_VME_NEEDED_PLD_REV ) {
	    DBGWRT_ERR((DBH,"*** sysMenPci2VmeInit: Z002 bridge too old. (rev=0x%02x)\n", rev ));
	    return ERROR;		/* illegal VME bridge */
	}   
    
#endif

    /* read BARs. On A21 only BAR0/1 are used because we need no nonswapped VME space. on all
       other (standard) PDLZ002 FPGAs there are 5 BARs used to cover swapped/nonswapped spaces */
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_0, &bar0 );
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_1, &bar1 );
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_2, &bar2 ); 
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_3, &bar3 ); /* <- should be 0x00000000 on A21 */
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_4, &bar4 ); /* <- should be 0x00000000 on A21 */

    /* enable busmastering for DMA 
       ts@men dont know if thats still needed with newer vxworks, but it doesnt harm. */
    men_vme_pcicfg_in_word( bus, dev, func, PCI_CFG_COMMAND, &command );
    command |= PCI_CMD_MASTER_ENABLE;
    men_vme_pcicfg_out_word( bus, dev, func, PCI_CFG_COMMAND, command );

    /* we have the addresses, now go to work... */
    return menpci2vmeinit_intern( bar0, bar1, bar2, bar3, bar4 );

}



/******************************************************************************
 *
 * sysVmeLocalToBusAdrs - convert a local address to a VME bus address PLDZ002
 *
 * This routine gets the VMEbus address that accesses a specified local
 * memory address.
 *
 * RETURNS: OK, or ERROR if the address space is unknown or the mapping is not
 * possible.
*
 * SEE ALSO: sysBusToLocalAdrs()
 */
STATUS sysVmeLocalToBusAdrs
(
 int 	adrsSpace,	/* bus address space where busAdrs resides */
 char *	localAdrs,	/* local address to convert */
 char **	pBusAdrs	/* where to return bus address */
 )
{
    u_int32 slvCtrl;
    u_int32 busAddr;

    if( !G_pld.isInit )
	{
	    return ERROR;
	}/*if*/

    /*--- check if local address is main memory or SRAM ---*/
    if( (localAdrs > (char *)0x2000000 /* 512MB */ && localAdrs < (char *)G_pld.sram)
	|| (localAdrs >= (char *)(G_pld.sram + G_pld.sramSize ))
	)
	{
	    return ERROR;
	}/*if*/

    /*--- check for correct address space ---*/
    switch( adrsSpace )
	{
	case VME_AM_STD_SUP_DATA:
	case VME_AM_STD_USR_DATA:
	case VME_AM_STD_SUP_ASCENDING: /* block transfers */
	case VME_AM_STD_USR_ASCENDING:
	    if( localAdrs >= (char *)G_pld.sram )
		slvCtrl = VME_CTRL_READ_D16( G_pld.base, PLDZ002_SLV24 );
	    else
		{
		    slvCtrl = VME_CTRL_READ_D16( G_pld.base, PLDZ002_SLV24_PCI );
		}
	    /*--- check if slave access is enabled ---*/
	    if( !( slvCtrl & PLDZ002_SLV24_SLEN24 ) )
		{
		    return ERROR;
		}/*if*/
	    busAddr = (u_int32)(slvCtrl & PLDZ002_SLV24_BASE_MASK_UPPER) << PLDZ002_SLV24_BASE_MASK_UPPER_SHIFT
		| (slvCtrl & PLDZ002_SLV24_BASE_MASK_LOWER) << PLDZ002_SLV24_BASE_MASK_LOWER_SHIFT;
	    break;
	case VME_AM_EXT_SUP_DATA:
	case VME_AM_EXT_USR_DATA:
	case VME_AM_EXT_SUP_ASCENDING: /* block transfers */
	case VME_AM_EXT_USR_ASCENDING:
	    if( localAdrs >= (char *)G_pld.sram )
		slvCtrl = VME_CTRL_READ_D32( G_pld.base, PLDZ002_SLV32 );
	    else
		slvCtrl = VME_CTRL_READ_D32( G_pld.base, PLDZ002_SLV32_PCI );

	    /*--- check if slave access is enabled ---*/
	    if( !( slvCtrl & PLDZ002_SLVxx_EN ) )
		{
		    printf("*** %s line %d\n", __FUNCTION__, __LINE__ );
		    return ERROR;
		}/*if*/
	    busAddr = (u_int32)(slvCtrl & PLDZ002_SLV32_BASE_MASK_UPPER) << PLDZ002_SLV32_BASE_MASK_UPPER_SHIFT
		| (slvCtrl & PLDZ002_SLV32_BASE_MASK_LOWER) << PLDZ002_SLV32_BASE_MASK_LOWER_SHIFT;
	    break;
	default:
	    printf("*** %s line %d\n", __FUNCTION__, __LINE__ );
	    return ERROR;
	}

    busAddr += (u_int32)localAdrs;
    if( localAdrs >= (char *)G_pld.sram )
	busAddr -= G_pld.sram;

    *pBusAdrs = (char *)busAddr;

    return OK;
}

/******************************************************************************
 *
 * sysVmeBusToLocalAdrs - convert a VME bus address to a local address PLDZ002
 *
 * This routine gets the local address that accesses a specified VMEbus
 * address. Depending on the endian mode the swapping or non swapping
 * window will be selected.
 *
 * RETURNS: OK, or ERROR if the address is unknown or the mapping is not
 * possible.
 *
 * SEE ALSO: sysLocalToBusAdrs(), sysVmePLDZ002setEndianMode()
 */
STATUS sysVmeBusToLocalAdrs
(
 int    adrsSpace,  /* bus address space where busAdrs resides */
 char * busAdrs,    /* bus address to convert */
 char **    pLocalAdrs  /* where to return local address */
 )
{
    u_int32 offset;
    u_int8  a32AddrsShift;
    
    if( G_chamVariant == 'B' && G_chamRevision >= 27 )
    {
        a32AddrsShift = 24;
    }
    else
    {
        a32AddrsShift = 29;
    }

    if( !G_pld.isInit )
    {
        return ERROR;
    }/*if*/

    switch (adrsSpace)
    {
    case OSS_VME_AM_16UD:
        if( (int)busAdrs >= PLDZ002_A16_SIZE )
        {
            return ERROR;
        }/*if*/
        if( G_pld.littleEndian )
        *pLocalAdrs = G_pld.a16d16sw + busAdrs;
        else
        *pLocalAdrs = G_pld.a16d16 + busAdrs;
        break;

    case OSS_VME_AM_24UD:
    case OSS_VME_AM_24UB:
        if( (int)busAdrs >= PLDZ002_A24_SIZE )
        {
            return ERROR;
        }/*if*/
        if( G_pld.littleEndian )
        *pLocalAdrs = G_pld.a24d16sw + busAdrs;
        else
        *pLocalAdrs = G_pld.a24d16 + busAdrs;
        break;

    case OSS_VME_AM_32UD:
    case OSS_VME_AM_32UB:
    case OSS_VME_AM_32UB64:
        offset = G_pld.a32maBase << a32AddrsShift;
        if( (char*)busAdrs < (char*) offset
        || (char*)busAdrs > (char*)(PLDZ002_A32_SIZE-1) + offset )
        {
            return ERROR;
        }/*if*/
        if( G_pld.littleEndian )
        *pLocalAdrs = G_pld.a32d32sw + (busAdrs - offset);
        else
        *pLocalAdrs = G_pld.a32d32 + (busAdrs - offset);
        break;

    default:
        return ERROR;
    }/*switch*/
    return OK;
}


/*******************************************************************************
 *
 * sysVmeBusIntGen - generate VME bus interrupt PLDZ002
 *
 * This routine generates a VME bus interrupt. It does not wait for IACK
 * to occur.
 *
 * NOTE: B11 has only one interrupter, so you cannot trigger multiple
 * levels at the same time.
 *
 * RETURNS: OK - if interrupt could be triggered
 *		   ERROR - if bad vector/level or interrupter busy
 *
 * SEE ALSO: sysBusIntAck(), sysBusIntGen()
 */
STATUS sysVmeBusIntGen(
		       int  level,        /* VMEbus interrupt level to generate (1-7) */
		       int  vector        /* interrupt vector to generate (0-255)     */
		       )
{
    int            lockKey;

    if( !G_pld.isInit )
	return ERROR;

    /* Validate interrupt level */

    if (level < 1 || level > 7 || vector > 255 || vector < 2)
        return (ERROR);

    /* lock interrupts so there will be no interference */
    CPU_INT_LOCK(&lockKey);

    /*--- check if interrupter busy ---*/
    if( MREAD_D8( G_pld.base, PLDZ002_INTR ) & PLDZ002_INTR_INTEN ){
	CPU_INT_UNLOCK(lockKey);
	return (ERROR);
    }

    /*--- write vector and level, trigger interrupt ---*/
    MWRITE_D8(G_pld.base, PLDZ002_INTID, vector );
    MWRITE_D8(G_pld.base, PLDZ002_INTR, level |PLDZ002_INTR_INTEN );

    CPU_INT_UNLOCK(lockKey);

    return OK;
}/*sysBusIntGen*/

/*******************************************************************************
 *
 * sysVmeIntEnable - enable a VME bus interrupt level PLDZ002
 *
 * This routine enables reception of a specified VMEbus interrupt level.
 *
 * RETURNS: OK, or ERROR if <intLevel> is not in the range 1 - 7.
 *
 * SEE ALSO: sysVmeIntDisable(), sysIntEnable()
 */

STATUS sysVmeIntEnable(
		       int intLevel        /* interrupt level to enable (1-7) */
		       )
{

    if( !G_pld.isInit )
	return ERROR;

    if( intLevel < 1 || intLevel > 7 )
	return ERROR;

    MSETMASK_D8( G_pld.base, PLDZ002_IMASK, (1<<intLevel) );
    /*logMsg ("VME Interrupt %d enabled \n", intLevel, 0,0,0,0,0 );*/
    return OK;
}

/*******************************************************************************
 *
 * sysVmeIntDisable - disable a bus interrupt level PLDZ002
 *
 * This routine disables reception of a specified VMEbus interrupt level.
 *
 * RETURNS: OK, or ERROR if <intLevel> is not in the range 1 - 7.
 *
 * SEE ALSO: sysVmeIntEnable(), sysIntDisable()
 */

STATUS sysVmeIntDisable(
			int intLevel        /* interrupt level to disable (1-7) */
			)
{

    if( !G_pld.isInit )
	return ERROR;

    if( intLevel < 1 || intLevel > 7 )
	return ERROR;

    MCLRMASK_D8( G_pld.base, PLDZ002_IMASK, (1<<intLevel) );
    return OK;
}

#ifndef MEN_A021

/* ts@men: on A21 the VME IRQs are handled by the separate vxBus 
   driver vxbMenA021Fpga.o, see kernel konfigurator item DRV_MEN_A021_FPGA */

/*******************************************************************************
 *
 * sysMenPci2VmeIntr - VME bridge interrupt handler PLDZ002 for non-A21
 *
 * This is the VMEbus interrupt handler for MEN's PCI to VME bridge
 * It is connected to a single PCI interrupt and examines the
 * VME bridge to determine the interrupt level and vector of the
 * interrupt source.  Having obtained the vector number, this routine then
 * vectors into the system vector table to the specified interrupt handling
 * routine.
 *
 * ACFAILs, if enabled, will cause a fixed vector VME_ACFAIL_INT_VEC to be
 * generated
 *
 * RETURNS: N/A
 */
void sysMenPci2VmeIntr (void)
{
    SYS_INT_HANDLER_DESC 	*currHandler;
    register MACCESS 		ma;
    u_int8 	istat;
    int 	level=0;
    int 	vector;
    int 	mbox;
    u_int8  dmaIrqStatus;

    ma = G_pld.base;

    /* check if DMA IRQ */
    dmaIrqStatus = MREAD_D8( ma, PLDZ002_DMASTA );
    if( dmaIrqStatus & PLDZ002_DMASTA_IRQ )
	{
	    /* logMsg("DMA IRQ %02x\n", dmaIrqStatus, 0,0,0,0,0 ); */
	    /* clear IRQ - don't touch ERR */
	    MWRITE_D8( ma, PLDZ002_DMASTA, PLDZ002_DMASTA_IRQ );
	    semGive( &G_dmaIrqSem );
	    return;
	}

    /*--- get pending interrupt levels. Only unmasked levels will show up ---*/
    istat = MREAD_D8( ma, PLDZ002_ISTAT );

    /*--- decode level ---*/
    if( istat & 0x80 )
	level = 7;
    else if( istat & 0x40 )
	level = 6;
    else if( istat & 0x20 )
	level = 5;
    else if( istat & 0x10 )
	level = 4;
    else if( istat & 0x08 )
	level = 3;
    else if( istat & 0x04 )
	level = 2;
    else if( istat & 0x02 )
	level = 1;
    else if( istat & 0x01 )
	{
	    /* ACFAIL interrupt */
	    vector = VME_ACFAIL_INT_VEC;
	    MSETMASK_D8( ma, PLDZ002_ISTAT, PLDZ002_ISTAT_ACFST );
	    goto HAVE_VECTOR;
	}
    else
	{

	    istat = MREAD_D8( G_pld.base, PLDZ002_MAIL_IRQ_STAT );
	    for( mbox = 0; mbox < 4; mbox ++ )
		{
		    if( istat & (PLDZ002_MAIL_IRQ_RD | PLDZ002_MAIL_IRQ_WR) )
			{
			    vector = VME_MBOX_0_INT_VEC + mbox;
			    MSETMASK_D8( ma, PLDZ002_MAIL_IRQ_STAT,
					 ((PLDZ002_MAIL_IRQ_RD | PLDZ002_MAIL_IRQ_WR) << mbox) );
			    goto HAVE_VECTOR;
			}/*if*/

		    istat >>= 2; /* next mbox bits */
		}/*for*/


	    /* check location monitor IRQ's */
	    istat =	MREAD_D8( G_pld.base, PLDZ002_LM_STAT_CTRL_0 );
	    /*		logMsg(" LM0 %02x\n", istat, 0,0,0,0,0 ); */
	    if( istat & PLDZ002_LM_STAT_CTRL_IRQ )
		{
		    vector = VME_LOCMON_0_INT_VEC;
		    MSETMASK_D8( ma, PLDZ002_LM_STAT_CTRL_0, PLDZ002_LM_STAT_CTRL_IRQ );
		    goto HAVE_VECTOR;
		}/*if*/

	    istat =	MREAD_D8( G_pld.base, PLDZ002_LM_STAT_CTRL_1 );
	    /*		logMsg(" LM1 %02x\n", istat, 0,0,0,0,0 ); */
	    if( istat & PLDZ002_LM_STAT_CTRL_IRQ )
		{
		    vector = VME_LOCMON_1_INT_VEC;
		    MSETMASK_D8( ma, PLDZ002_LM_STAT_CTRL_1, PLDZ002_LM_STAT_CTRL_IRQ );
		    goto HAVE_VECTOR;
		}/*if*/

	    /*logMsg("---- no vector, nothing to do!\n", 0, 0,0,0,0,0 );*/
	    return;
	}/*if*/

    /*--- Get the interrupt vector from IACK space ---*/
    /* clear bus error bit to detect spurious interrupts */
    MSETMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_BERR );

    /* fetch vector */
    vector = *(volatile u_int8 *)(G_pld.iack + (level<<1) + 1 );

    if( MREAD_D8( ma, PLDZ002_MSTR ) & PLDZ002_MSTR_BERR )
	logMsg ("Spurious VME Interrupt! level %d\n", level, 0,0,0,0,0 );
    else
	{
	HAVE_VECTOR:
	    /*logMsg ("got VME Interrupt level %d vector %x\n", level, vector,0,0,0,0);*/
#if	(CPU_FAMILY == I80X86)
	    /* now dispatch to connected routines */
	    if ((currHandler = sysVmeIntTbl[vector]) == NULL)
		logMsg ("Uninitialized VME Interrupt! vector 0x%02x level %d\n",
			vector, level ,0,0,0,0);
	    else
		{

		    while (currHandler != NULL)
			{
			    currHandler->vec (currHandler->arg);
			    currHandler = currHandler->next;
			}
		}

#else
	    /* now dispatch to connected routines */
	    if ((currHandler = sysIntTbl[vector]) == NULL)
		logMsg ("Uninitialized VME Interrupt! vector 0x%02x level %d\n",
			vector, level ,0,0,0,0);
	    else
		{
		    while (currHandler != NULL)
			{
			    currHandler->vec (currHandler->arg);
			    currHandler = currHandler->next;
			}
		}
#endif
	}
}

#endif /* MEN_A021 */


/******************************************************************************
 *
 * sysMenPci2VmeSram - gets the address and size of the SRAM on the PCI2VME bridge.
 *
 * This routine gets the address and size of the SRAM on the PCI2VME bridge.
 *
 * RETURNS: address or NULL
 */
u_int32 sysMenPci2VmeSram
(
 int   *sramSizeP		/* pointer to variable where size of SRAM will be stored */
 )
{

    sramSizeP = 0;

    if( !G_pld.isInit )
	return 0;

    *sramSizeP = G_pld.sramSize;
    return (G_pld.sram);
}

/******************************************************************************
 *
 * sysMenPci2VmeSetA32MasterPrefix - sets the upper three bits of the A32 VME address
 *
 * This routine sets the upper three bits of the A32 VME address.
 *
 * RETURNS: OK or ERROR
 */
u_int32 sysMenPci2VmeSetA32MasterPrefix( u_int32 a32AddressPrefix )
{
    u_int32 addrsMask;
    u_int8 addrsShift;

    if( !G_pld.isInit )
    {
        return ERROR;
    }

    if( G_chamVariant == 'B' && G_chamRevision >= 27 )
    {
        addrsMask = 0xF8000000;
        addrsShift = 24;
    }
    else
    {
        addrsMask = 0xE0000000;
        addrsShift = 29;
    }

    if( a32AddressPrefix & ~addrsMask )
    {
        return ( ERROR ); /* more than the upper three bits was set */
    }

    G_pld.a32maBase = a32AddressPrefix >> addrsShift;
    MWRITE_D8( G_pld.base,
               PLDZ002_LONGADD,
               G_pld.a32maBase );

    return ( OK );
}


/******************************************************************************
 *
 * sysAddrInVmeRange - in VME range PLDZ002
 *
 * This routine checks if the local address points to a valid VME range
 * depends on endian mode.
 *
 * RETURNS: FALSE if NOT in VME range or TRUE if in VME range
 */
int sysAddrInVmeRange
(
 char   * adrs		/* local address */
 )
{
    if( !G_pld.isInit || NULL == adrs )
	{
        return ERROR;
	}

    /* A32 D32 */
    if( (u_int32)adrs >= G_pld.a32d32 && (u_int32)adrs < (G_pld.a32d32+PLDZ002_A32D32_SIZE) )
	{
	    return( TRUE );
	}
    if( (u_int32)adrs >= G_pld.a32d32sw && (u_int32)adrs < (G_pld.a32d32sw+PLDZ002_A32D32_SIZE) )
	{
	    return( TRUE );
	}

    /* A24 D16 */
    if( (u_int32)adrs >= G_pld.a24d16 && (u_int32)adrs < (G_pld.a24d16+0x1000000) )
	{
	    return( TRUE );
	}
    if( (u_int32)adrs >= G_pld.a24d16sw && (u_int32)adrs < (G_pld.a24d16sw+0x1000000) )
	{
	    return( TRUE );
	}

    /* A16 D16 */
    if( (u_int32)adrs >= G_pld.a16d16 && (u_int32)adrs < (G_pld.a16d16+0x10000) )
	{
	    return( TRUE );
	}
    if( (u_int32)adrs >= G_pld.a16d16sw && (u_int32)adrs < (G_pld.a16d16sw+0x10000) )
	{
	    return( TRUE );
	}

    /* A24 D32 */
    if( (u_int32)adrs >= G_pld.a24d32 && (u_int32)adrs < (G_pld.a24d32+0x1000000) )
	{
	    return( TRUE );
	}
    if( (u_int32)adrs >= G_pld.a24d32sw && (u_int32)adrs < (G_pld.a24d32sw+0x1000000) )
	{
	    return( TRUE );
	}

    return( FALSE );
}


/******************************************************************************
 *
 * sysVmeProbe - probe a VME bus address PLDZ002
 *
 * This routine probes an address on the VME bus. The routine checkes the
 * BERR flag in the VME bridge.
 * All probing is done with interrupts disabled.
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
    DBGCMD( static const char *functionName = __FUNCTION__ ;)
	STATUS status = OK;
    int      oldLevel;
    MACCESS ma2 = (MACCESS)adrs;
    MACCESS ma = (MACCESS)G_pld.base;

    if( !G_pld.isInit )
	{
        return ERROR;
	}

    status = sysAddrInVmeRange( adrs );
    if( FALSE == status || ERROR == status )
	{
	    DBGWRT_ERR((DBH,"*** %s sysAddrInVmeRange\n", functionName ));
	    return ERROR;
	}

    /* lock interrupts */
    oldLevel = intCpuLock ();

    /* clear VME bridge bus error bit */
    MSETMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_BERR );

    /* access the address */
    switch( mode ){
    case VX_READ:
	switch( length ){
	case 1:	*(u_int8  *)pVal = MREAD_D8 ( ma2, 0 ); break;
	case 2:	*(u_int16 *)pVal = MREAD_D16( ma2, 0 ); break;
	case 4:	*(u_int32 *)pVal = MREAD_D32( ma2, 0 ); break;
	default: status = ERROR; break;
	}
	break;
    case VX_WRITE:
	switch( length ){
	case 1:	MWRITE_D8 ( ma2, 0, *(u_int8  *)pVal); break;
	case 2:	MWRITE_D16( ma2, 0, *(u_int16 *)pVal); break;
	case 4:	MWRITE_D32( ma2, 0, *(u_int32 *)pVal); break;
	default: status = ERROR; break;
	}
	break;
    default: status = ERROR; break;
    }

    /* check if BERR bit has been set */
    if( MREAD_D8( ma, PLDZ002_MSTR ) & PLDZ002_MSTR_BERR ){
	MSETMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_BERR );
	DBGWRT_ERR((DBH,"*** %s bus error at %08x\n", functionName, adrs ));
	status = ERROR;
    }

    /* unlock interrupts */
    intCpuUnlock (oldLevel);

    return status;
}

/*
  MenMon> pcid 19
  Single Function Dev
  ADDR.     VALUE        DESCRIPTION
  =====     =====        ===========
  0x00      0x1172       Vendor ID
  0x02      0x5056       Device ID

  0x08      0x07         Revision ID

  0x10      0x88000000   Base Address Register 0
  0x14      0xA0000000   Base Address Register 1
  0x18      0xE0000000   Base Address Register 2
  0x1C      0xC0000000   Base Address Register 3
  0x20      0x8C000000   Base Address Register 4

  0x2C      0x3032       Subsystem Vendor ID
  0x2E      0x7A30       Subsystem ID

  0x3C      0x0A         Interrupt line
*/

/******************************************************************************
 *
 * sysVmeShow - displays information about PLDZ002
 *
 * This routine displays information about PLDZ002. When invoked on A21, the 
 * previous VXB_DEVICE_ID passed in sysVmeInit is used. There the unit is always 0
 *
 * RETURNS: N/A
 */
#ifdef MEN_VME_VXB_SUPPORT
	IMPORT void sysVmeShow( VXB_DEVICE_ID pciCtrl, int unit  )
#else
	IMPORT void sysVmeShow( int unit  )
#endif
{
    int bus, dev, func;
    UINT8 rev;
    UINT16 vendor, device;
    UINT32 base;
    u_int32 mask;
    u_int32 size;
    u_int32 localAddr;
    u_int32 vmeAddr;

    printf("%s\n", VME2PCI_PLDZ002_RCSid );

#ifdef MEN_VME_VXB_SUPPORT
    if ( vxbPciFindDevice(G_pld.busCtrlID, 
    					  CHAMELEON_PCI_VENID_MEN, 
    					  CHAMELEON_PCI_DEV_ID, 
    					  0, &bus, &dev, &func ) != OK) {
            DBGWRT_ERR((DBH,"*** sysMenPci2VmeInit: FPGA not found! \n" ));
	    return;
        }
    
    printf("VME bridge #%d %04x/%04x found at PCI ctrl. 0x%x pci b/d/f %02x/%02x/%02x\n",
	   unit, CHAMELEON_PCI_VENID_MEN, CHAMELEON_PCI_DEV_ID, G_pld.busCtrlID, bus, dev, func );        	

#else
    if( pciFindDevice( PLDZ002_VEN_ID, PLDZ002_DEV_ID, unit, &bus, &dev, &func ) == ERROR )	{
	    printf(" no VME bridge found\n");
	    return;
	}
    printf("VME bridge #%d %04x/%04x found at pci b/d/f %02x/%02x/%02x\n",
	   unit, PLDZ002_VEN_ID, PLDZ002_DEV_ID, bus, dev, func );        	
#endif

    
    printf("control register at %08x\n", (int)G_pld.base );

    /* check device */
    men_vme_pcicfg_in_word( bus, dev, func, PCI_CFG_SUB_VENDER_ID, &vendor );
    men_vme_pcicfg_in_word( bus, dev, func, PCI_CFG_SUB_SYSTEM_ID, &device );
    men_vme_pcicfg_in_byte( bus, dev, func, PCI_CFG_REVISION,      &rev );

    printf("   PLDZ002 sub ven/dev %04x/%04x rev %d\n",
	   vendor, device, rev );
    printf("Slot 1 function %sabled\n", sysVmeIsSystemController() ? "en" : "dis" );

#ifndef MEN_A021
    if( vendor != PLDZ002_SUBSYS_VEN_ID || device != PLDZ002_SUBSYS_ID )
	printf("*** illegal sub id's\n");

    if( rev < VME4VX_SPC_NEEDED_PLD_REV )
	{
	    printf("*** illegal PLD revision %d\n", rev);
	}
#endif
    
    if( G_pld.littleEndian == 1 )
	printf("byte order little endian\n");
    else
	printf("byte order big endian\n");

    /* get BAR's */
    printf("======================================================================\n" );
    printf("Master Windows\tLocal Base\tSize\t\tVME Base\tNote\n");
    printf("======================================================================\n" );

    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_0, &base );
      
    printf("VMEA24D16\t%08x\t%08x\t0x000000\n", 	base+Z002INTERN_A24D16_OFFS, PLDZ002_A24Dxx_SIZE);
    printf("VMEA16D16\t%08x\t%08x\t0x0000\n", 		base+Z002INTERN_A16D16_OFFS, PLDZ002_A16Dxx_SIZE);
    printf("VMEA16D32 (P2)\t%08x\t%08x\t0x0000\n", 	base+Z002INTERN_A16D32_OFFS, PLDZ002_A16Dxx_SIZE);
    printf("VME SHARED RAM\t%08x\t%08x\n", 			base+Z002INTERN_LRAM_OFFS, 	 PLDZ002_LRAM_SIZE );
    printf("VME CTRL regs\t%08x\n", 				base+Z002INTERN_CTRL_OFFS);

    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_1, &base );
    if( G_chamVariant == 'B' && G_chamRevision >= 27 )
    {
        size = PLDZ002_A32D32_SIZE_B;
    }
    else
    {
        size = PLDZ002_A32D32_SIZE;
    }
    printf("VMEA32D32 (P2)\t%08x\t%08x\t%01X0000000\n", base, size, (int)G_pld.a32maBase << 1);

#ifndef MEN_A021
    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_2, &base );
    printf("VMEA24D16\t%08x\t%08x\t0x000000\tnon swapped\n", 	base+PLDZ002_A24D16_SPACE, PLDZ002_A24Dxx_SIZE);
    printf("VMEA16D16\t%08x\t%08x\t0x0000\t\tnon swapped\n", 	base+PLDZ002_A16D16_SPACE, PLDZ002_A16Dxx_SIZE);
    printf("VMEA16D32 (P2)\t%08x\t%08x\t0x0000\t\tnon swapped\n", base+PLDZ002_A16D32_SPACE, PLDZ002_A16Dxx_SIZE);

    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_3, &base );
    printf("VMEA32D32 (P2)\t%08x\t%08x\t%01X0000000\tnon swapped\n", base, PLDZ002_A32D32_SIZE,
	   (int)G_pld.a32maBase << 1 );

    men_vme_pcicfg_in_long( bus, dev, func, PCI_CFG_BASE_ADDRESS_4, &base );
    printf("VMEA24D32 (P2)\t%08x\t%08x\t0x000000\n",              base+PLDZ002_A24D32SW_SPACE, PLDZ002_A24Dxx_SIZE );
    printf("VMEA24D32 (P2)\t%08x\t%08x\t0x000000\tnon swapped\n", base+PLDZ002_A24D32_SPACE, PLDZ002_A24Dxx_SIZE );
#endif

    printf("======================================================================\n" );
    printf("Slave Windows\tVME Base\tSize\t\tLocal Base\tNote\n");
    printf("======================================================================\n" );

    mask = VME_CTRL_READ_D16( G_pld.base, PLDZ002_SLV24 );
    if( mask & PLDZ002_SLVxx_EN )
	{
	    vmeAddr =  (PLDZ002_SLV24_BASE_MASK_UPPER & mask) << PLDZ002_SLV24_BASE_MASK_UPPER_SHIFT
		| (PLDZ002_SLV24_BASE_MASK_LOWER & mask) << PLDZ002_SLV24_BASE_MASK_LOWER_SHIFT;
	    size = mask & PLDZ002_SLV24_SIZE_MASK;
	    switch( size )
		{
		case PLDZ002_SLV24_SIZE_64K:
		    size = 0x10000;
		    break;
		case PLDZ002_SLV24_SIZE_128K:
		    size = 0x20000;
		    break;
		case PLDZ002_SLV24_SIZE_256K:
		    size = 0x40000;
		    break;
		case PLDZ002_SLV24_SIZE_512K:
		    size = 0x80000;
		    break;
		default:
		case PLDZ002_SLV24_SIZE_1MB:
		    size = 0x100000;
		    break;
		}

	    localAddr = G_pld.sram;
	    printf("VMEA24SA\t%08x\t%08x\t%08x\tSRAM\n", (int)vmeAddr, (int)size, (int)localAddr );
	}
    else
	printf("VMEA24SA\tdisabled\n");


    mask = VME_CTRL_READ_D16( G_pld.base, PLDZ002_SLV24_PCI );
    if( mask & PLDZ002_SLVxx_EN )
	{
	    vmeAddr =  (PLDZ002_SLV24_BASE_MASK_UPPER & mask) << PLDZ002_SLV24_BASE_MASK_UPPER_SHIFT
		| (PLDZ002_SLV24_BASE_MASK_LOWER & mask) << PLDZ002_SLV24_BASE_MASK_LOWER_SHIFT;
	    size = mask & PLDZ002_SLV24_SIZE_MASK;
	    switch( size )
		{
		case PLDZ002_SLV24_SIZE_64K:
		    size = 0x10000;
		    break;
		case PLDZ002_SLV24_SIZE_128K:
		    size = 0x20000;
		    break;
		case PLDZ002_SLV24_SIZE_256K:
		    size = 0x40000;
		    break;
		case PLDZ002_SLV24_SIZE_512K:
		    size = 0x80000;
		    break;
		default:
		case PLDZ002_SLV24_SIZE_1MB:
		    size = 0x100000;
		    break;
		}

	    localAddr = 0;
	    printf("VMEA24SA-PCI\t%08x\t%08x\t%08x\tPCI\n", (int)vmeAddr, (int)size, (int)localAddr );
	}
    else
	printf("VMEA24SA-PCI\tdisabled\n");


    mask = VME_CTRL_READ_D32( G_pld.base, PLDZ002_SLV32 );
    if( mask & PLDZ002_SLVxx_EN )
	{
	    vmeAddr = (mask & PLDZ002_SLV32_BASE_MASK_UPPER) << PLDZ002_SLV32_BASE_MASK_UPPER_SHIFT
		| (PLDZ002_SLV32_BASE_MASK_LOWER & mask) << PLDZ002_SLV32_BASE_MASK_LOWER_SHIFT;

	    size = mask & PLDZ002_SLV32_SIZE_MASK;
	    switch( size )
		{
		case PLDZ002_SLV32_SIZE_1MB:
		    size = 0x100000;
		    break;
		case PLDZ002_SLV32_SIZE_2MB:
		    size = 0x200000;
		    break;
		case PLDZ002_SLV32_SIZE_4MB:
		    size = 0x400000;
		    break;
		case PLDZ002_SLV32_SIZE_8MB:
		    size = 0x800000;
		    break;
		case PLDZ002_SLV32_SIZE_16MB:
		    size = 0x1000000;
		    break;
		case PLDZ002_SLV32_SIZE_32MB:
		    size = 0x2000000;
		    break;
		case PLDZ002_SLV32_SIZE_64MB:
		    size = 0x4000000;
		    break;
		case PLDZ002_SLV32_SIZE_128MB:
		    size = 0x8000000;
		    break;
		default:
		case PLDZ002_SLV32_SIZE_256MB:
		    size = 0x10000000;
		    break;
		}

	    localAddr = G_pld.sram;
	    printf("VMEA32SA\t%08x\t%08x\t%08x\tSRAM\n", (int)vmeAddr, (int)size, (int)localAddr );
	}
    else
	printf("VMEA32SA\tdisabled\n");

    mask = VME_CTRL_READ_D32( G_pld.base, PLDZ002_SLV32_PCI );
    if( mask & PLDZ002_SLVxx_EN )
	{
	    vmeAddr = (mask & PLDZ002_SLV32_BASE_MASK_UPPER) << PLDZ002_SLV32_BASE_MASK_UPPER_SHIFT
		| (PLDZ002_SLV32_BASE_MASK_LOWER & mask) << PLDZ002_SLV32_BASE_MASK_LOWER_SHIFT;

	    size = mask & PLDZ002_SLV32_SIZE_MASK;
	    switch( size )
		{
		case PLDZ002_SLV32_SIZE_1MB:
		    size = 0x100000;
		    break;
		case PLDZ002_SLV32_SIZE_2MB:
		    size = 0x200000;
		    break;
		case PLDZ002_SLV32_SIZE_4MB:
		    size = 0x400000;
		    break;
		case PLDZ002_SLV32_SIZE_8MB:
		    size = 0x800000;
		    break;
		case PLDZ002_SLV32_SIZE_16MB:
		    size = 0x1000000;
		    break;
		case PLDZ002_SLV32_SIZE_32MB:
		    size = 0x2000000;
		    break;
		case PLDZ002_SLV32_SIZE_64MB:
		    size = 0x4000000;
		    break;
		case PLDZ002_SLV32_SIZE_128MB:
		    size = 0x8000000;
		    break;
		default:
		case PLDZ002_SLV32_SIZE_256MB:
		    size = 0x10000000;
		    break;
		}

	    localAddr = 0;
	    printf("VMEA32SA-PCI\t%08x\t%08x\t%08x\tPCI\n", (int)vmeAddr, (int)size, (int)localAddr );
	}
    else
    	printf("VMEA32SA-PCI\tdisabled\n");

	printf(
			"======================================================================\n");
	printf("(P2) \t\t- P2 connector is needed for D32 access\n");
	printf(
			"non swapped\t- this windows should be used for little endian CPU's\n");
	printf(
			"======================================================================\n");
	printf("\nVME Control Registers:\n");
	printf("VME Interrupt Control Register    INTR       (0x0000) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_INTR));
	printf("VME Interrupt STATUS/ID Register  INTID      (0x0004) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_INTID));
	printf("Interrupt Status Register (r)     ISTAT      (0x0008) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_ISTAT));
	printf("Interrupt Mask Register           IMASK      (0x000C) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_IMASK));
	printf("Master Control Register           MSTR       (0x0010) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_MSTR));
	printf("Slave Control Register A24        SLV24      (0x0014) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SLV24));
	printf("System Controller Register        SYSCTL     (0x0018) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SYSCTL));
	printf(
			"======================================================================\n");
	printf("Additional Features:\n");
	printf("Upper 3 Address Bits for A32      LONGADD    (0x001c) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_LONGADD));
	printf("Mailbox Irq Enable Register       MAIL_IRQE  (0x0020) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_MAIL_IRQ_CTRL));
	printf("Mailbox Irq Request Register      MAIL_IRQ   (0x0024) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_MAIL_IRQ_STAT));
	printf("PCI Offset Address                PCI_OFFSET (0x0028) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_PCI_OFFSET));
	printf("DMA Status Register               DMASTA     (0x002C) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_DMASTA));
	printf("Slave Control Register A16        SLV16      (0x0030) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SLV16));
	printf("Slave Control Register A32        SLV32      (0x0034) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SLV32));
	printf("Location Status Register          LOCSTA_0   (0x0038) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_LM_STAT_CTRL_0));
	printf("Location Status Register          LOCSTA_1   (0x003c) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_LM_STAT_CTRL_1));
	printf("Location Monitor Address Register LOCADR_0   (0x0040) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_LM_ADDR_0));
	printf("Location Monitor Address Register LOCADR_1   (0x0044) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_LM_ADDR_1));
	printf("A24 Slave Base Address for PCI    SLV24_PCI  (0x0048) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SLV24_PCI));
	printf("A32 Slave Base Address for PCI    SLV32_PCI  (0x004c) = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.base, PLDZ002_SLV32_PCI));
	printf(
			"======================================================================\n");
	printf("Mailboxes:\n");
	printf("Mailbox 1       = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.sram, 0xFF800));
	printf("Mailbox 2       = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.sram, 0xFF800));
	printf("Mailbox 3       = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.sram, 0xFF800));
	printf("Mailbox 4       = 0x%08x\n",
			VME_CTRL_READ_D32(G_pld.sram, 0xFF800));
}

/*******************************************************************************
 *
 * sysVmeAcFailIntEnable - enable ACFAIL interrupt PLDZ002
 *
 * This routine enables reception of the ACFAIL interrupt that is triggered
 * when the VMEbus signal ACFAIL goes low. The special interrupt
 * <VME_ACFAIL_INT_VEC> is triggered (see men_824x_ali.h).
 *
 * NOTE: ACFAIL handler is N/A on B011 and A012 rev 00.xx.xx boards
 *
 * RETURNS: OK, or ERROR if VME bridge not initialized
 *
 * SEE ALSO: sysVmeAcFailIntDisable()
 */
STATUS sysVmeAcFailIntEnable(void)
{
    if( !G_pld.isInit )
	return ERROR;

    MSETMASK_D8( G_pld.base, PLDZ002_IMASK, 0x1 );
    return OK;
}

/*******************************************************************************
 *
 * sysVmeAcFailIntDisable - disable ACFAIL interrupt PLDZ002
 *
 * This routine disables reception of the ACFAIL interrupt.
 * RETURNS: OK, or ERROR if VME bridge not initialized
 *
 * SEE ALSO: sysVmeAcFailIntEnable()
 */
STATUS sysVmeAcFailIntDisable(void)
{
    if( !G_pld.isInit )
	return ERROR;

    MCLRMASK_D8( G_pld.base, PLDZ002_IMASK, 0x1 );
    return OK;
}


/*******************************************************************************
 *
 * sysVmeLocMonSetup - location monitor setup  PLDZ002
 *
 * This routine setups the location monitor interrupt.
 * The interrupt handler must be connected to <VME_LOCMON_0_INT_VEC> or
 * <VME_LOCMON_1_INT_VEC> .
 * The compared address bits are A16 15..10, A24 23..10 and A32 31..10.
 *
 * RETURNS: OK, or ERROR if VME bridge not initialized
 */
STATUS sysVmeLocMonSetup
(
 int  	mon,			/* monitor 0 or 1 */
 u_int8  storeAddrBits,	/* not used */
 int  	wr,				/* monitor write access */
 int  	rd,				/* monitor read access */
 int  	addrMode,		/* SYS_VME_LM_ADDR_MODE_Axx xx 16/24/32 */
 u_int32 addr,			/* compare addr */
 int  	enable 			/* 1 - enable irq */
 )
{
    int 		 lmOffsCtrl;
    int 		 lmOffsAddr;
    u_int8       val;
    int			 error = ERROR;

    if( !G_pld.isInit )
	goto CLEANUP;

    if( mon == 0 )
	{
	    lmOffsCtrl = PLDZ002_LM_STAT_CTRL_0;
	    lmOffsAddr = PLDZ002_LM_ADDR_0;
	}
    else
	{
	    lmOffsCtrl = PLDZ002_LM_STAT_CTRL_1;
	    lmOffsAddr = PLDZ002_LM_ADDR_1;
	}/*if*/

    /* disable IRQ */
    MCLRMASK_D8( G_pld.base, lmOffsCtrl, PLDZ002_LM_STAT_CTRL_IRQ_EN );

    /* set comparator addr */
    VME_CTRL_WRITE_D32( G_pld.base, lmOffsAddr, addr );

    /* clear IRQ */
    val = PLDZ002_LM_STAT_CTRL_IRQ;

    switch( addrMode )
	{
	case SYS_VME_LM_ADDR_MODE_A16:
	    val |= PLDZ002_LM_STAT_CTRL_A16_MODE;
	    break;
	case SYS_VME_LM_ADDR_MODE_A24:
	    val |= PLDZ002_LM_STAT_CTRL_A24_MODE;
	    break;
	case SYS_VME_LM_ADDR_MODE_A32:
	    val |= PLDZ002_LM_STAT_CTRL_A32_MODE;
	    break;

	default:
	    goto CLEANUP;
	}/*switch*/

    if( wr )
	val |= PLDZ002_LM_STAT_CTRL_WR;

    if( rd )
	val |= PLDZ002_LM_STAT_CTRL_RD;

    MWRITE_D8( G_pld.base, lmOffsCtrl, val );

    /* enable IRQ */
    if( enable )
	MSETMASK_D8( G_pld.base, lmOffsCtrl, PLDZ002_LM_STAT_CTRL_IRQ_EN );

    error = OK;

 CLEANUP:
    return( error );
}


/*******************************************************************************
 *
 * sysVmeIsSystemController - checks if the VME bridge is system controller PLDZ002
 *
 * This routine checks if the PCI2VME brigde slot1 function is enabled.
 *
 * RETURNS: 0 or 1
 *
 */
int sysVmeIsSystemController(void)
{
    u_int8       sysCtrl;

    if( !G_pld.isInit )
	return 0;

    sysCtrl = MREAD_D8( G_pld.base, PLDZ002_SYSCTL );

    if( sysCtrl & PLDZ002_SYSCTL_SYSCON )
	return 1;
    else
	return 0;
}

/*******************************************************************************
 *
 * sysVmeReset - resets the VMEbus  PLDZ002
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
    if( !G_pld.isInit )
	return ERROR;

    if( sysVmeIsSystemController() )
	{
	    MSETMASK_D8( G_pld.base, PLDZ002_SYSCTL, PLDZ002_SYSCTL_SYSRES );
	    return( OK );
	}
    else
	return ERROR;
}

/*******************************************************************************
 *
 * sysVmeAddrModifierSet - Set VMEbus address modifier for the space:
 *    0 (00b) non-privileged data access (default)
 *    1 (01b) non-privileged program access
 *    2 (10b) supervisory data access
 *    3 (11b) supervisory program access
 *
 * RETURNS: OK or ERROR if chameleon revision is lower than B27 or error during
 * modification of the address modifier 
 *
 */
STATUS sysVmeAddrModifierSet( u_int32 addrSpace, u_int8 modifier )
{
    int error = OK;
    u_int32 addrModifier = 0;
    
    if( G_chamVariant != 'B' && G_chamRevision < 27 )
    {
        return ERROR;
    }
    
    G_pld.addrModifier = modifier;
    
    switch( addrSpace )
    {
        case VME4VX_SPC_A16_D16:
        case VME4VX_SPC_A16_D32:
            addrModifier &=~VME4l_SPC_A16_AM_MASK;
            addrModifier |= (modifier & 0x03) << 0;
            MWRITE_D8( G_pld.base,
                       PLDZ002_AMOD,
                       addrModifier );
            break;
            
        case VME4VX_SPC_A24_D16:
        case VME4VX_SPC_A24_D32:
            addrModifier &=~VME4l_SPC_A24_AM_MASK;
            addrModifier |= (modifier & 0x03) << 2;
            MWRITE_D8( G_pld.base,
                       PLDZ002_AMOD,
                       addrModifier );
            break;
            
        case VME4VX_SPC_A32_D32:
            addrModifier &=~VME4l_SPC_A32_AM_MASK;
            addrModifier |= (modifier & 0x03) << 4;
            MWRITE_D8( G_pld.base,
                       PLDZ002_AMOD,
                       addrModifier );
            break;
            
        case VME4VX_SPC_A24_D16_BLT:           
        case VME4VX_SPC_A24_D32_BLT:
        case VME4VX_SPC_A32_D32_BLT:            
        case VME4VX_SPC_A24_D64_BLT:
        case VME4VX_SPC_A32_D64_BLT:
            /* just set the modifier in the G_pld global variable */
            /* the address modifier will be applied in sysMenPci2VmeDma() */
            break;
            
        default:
            printf("Address space access error");
            error = ERROR;
    }

    return error;
}

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
    u_int8       mask;

    if( !G_pld.isInit )
	return ERROR;

    if( !enable )
	{
	    mask = 0;
	}
    else
	{
	    mask = PLDZ002_SLVxx_EN | ( slaveAddr >> 12 );
	}/*if*/

    MWRITE_D8( G_pld.base, PLDZ002_SLV16, mask );


    return( error );
}

/*******************************************************************************
 *
 * sysVmeA24SlaveAddressSet - enable/disable the A24 slave windows
 *
 * This routine enables or disables a A24 slave window and
 * setups the VME address, the cpu address and the window size.
 * MEN's PCI2VME bridge has two A24 slave windows one points to SRAM
 * and the other one points to main memory.
 *
 * NOTE: If whichWindow is SYS_VME_WIN_MAIN_MEM for A013 this
 *       routine returns ERROR always because of a deadlock condition.
 *       The VME and the CPU addresses must be size aligned.
 *       The cpuAddress of sysVmeA24SlaveAddressSet() and
 *       sysVmeA32SlaveAddressSet() must be equal because only
 *       one translation register is used to translate the A24 and A32
 *       VME accesses to the main memory of the CPU.
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
    u_int16  mask;

    if( !G_pld.isInit || vmeAddr > 0x00FFFFFF )
	{
	    goto CLEANUP;
	}
    
    /* PCI offset is 4kbyte aligned */
    if( cpuAddr & 0xFFF )
    {
    	return ERROR;
    }

    mask = PLDZ002_SLVxx_EN
	| ( (vmeAddr & 0x00f00000) >> PLDZ002_SLV24_BASE_MASK_UPPER_SHIFT )
	| ( (vmeAddr & 0x000f0000) >> PLDZ002_SLV24_BASE_MASK_LOWER_SHIFT );

    switch( windowSize )
	{
	case 0x10000:
	    mask |= PLDZ002_SLV24_SIZE_64K;
	    break;
	case 0x20000:
	    mask |= PLDZ002_SLV24_SIZE_128K;
	    break;
	case 0x40000:
	    mask |= PLDZ002_SLV24_SIZE_256K;
	    break;
	case 0x80000:
	    mask |= PLDZ002_SLV24_SIZE_512K;
	    break;
	default:
	case 0x100000:
	    mask |= PLDZ002_SLV24_SIZE_1MB;
	    break;
	}

    if( !enable )
	{
	    mask = 0;
	}/*if*/

    if( whichWindow == SYS_VME_WIN_SRAM )
    {
    	VME_CTRL_WRITE_D16( G_pld.base, PLDZ002_SLV24, mask );
    }
    else
    {
    	VME_CTRL_WRITE_D16( G_pld.base, PLDZ002_SLV24_PCI, mask );
    	VME_CTRL_WRITE_D32( G_pld.base, PLDZ002_PCI_OFFSET, (cpuAddr & 0xFFFFF000) );
    }
    error = OK;


 CLEANUP:
    return( error );
}


/*******************************************************************************
 *
 * sysVmeA32SlaveAddressSet - enable/disable the A32 slave window
 *
 * This routine enables or disables a A32 slave window and
 * setups the VME address, the cpu address and the window size.
 *
 * NOTE: If whichWindow is SYS_VME_WIN_MAIN_MEM for A013 this
 *       routine returns ERROR always because of a deadlock condition.
 *       The VME and the CPU addresses must be size aligned.
 *       The cpuAddress of sysVmeA24SlaveAddressSet() and
 *       sysVmeA32SlaveAddressSet() must be equal because only
 *       one translation register is used to translate the A24 and A32
 *       VME accesses to the main memory of the CPU.
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
    u_int32       mask;

    if( !G_pld.isInit )
	return ERROR;
    
    /* PCI offset is 4kbyte aligned */
    if( cpuAddr & 0xFFF )
    {
    	return ERROR;
    }

    mask = PLDZ002_SLVxx_EN
	| ( (vmeAddr & 0xF0000000) >> PLDZ002_SLV32_BASE_MASK_UPPER_SHIFT )
	| ( (vmeAddr & 0x0FF00000) >> PLDZ002_SLV32_BASE_MASK_LOWER_SHIFT );

    switch( windowSize )
	{
	case 0x100000:
	    mask |= PLDZ002_SLV32_SIZE_1MB;
	    break;
	case 0x200000:
	    mask |= PLDZ002_SLV32_SIZE_2MB;
	    break;
	case 0x400000:
	    mask |= PLDZ002_SLV32_SIZE_4MB;
	    break;
	case 0x800000:
	    mask |= PLDZ002_SLV32_SIZE_8MB;
	    break;
	case 0x1000000:
	    mask |= PLDZ002_SLV32_SIZE_16MB;
	    break;
	case 0x2000000:
	    mask |= PLDZ002_SLV32_SIZE_32MB;
	    break;
	case 0x4000000:
	    mask |= PLDZ002_SLV32_SIZE_64MB;
	    break;
	case 0x8000000:
	    mask |= PLDZ002_SLV32_SIZE_128MB;
	    break;
	default:
	case 0x10000000:
	    mask |= PLDZ002_SLV32_SIZE_256MB;
	    break;
	}


    if( !enable )
	{
	    mask = 0;
	}

    if( whichWindow == SYS_VME_WIN_SRAM )
    {
    	VME_CTRL_WRITE_D32( G_pld.base, PLDZ002_SLV32, mask );
    }
    else
    {
    	VME_CTRL_WRITE_D32( G_pld.base, PLDZ002_SLV32_PCI, mask );
    	VME_CTRL_WRITE_D32( G_pld.base, PLDZ002_PCI_OFFSET, (cpuAddr & 0xFFFFF000) );
    }

    return( error );
}


/*******************************************************************************
 *
 * sysVmeMailboxIrqSetup - enable/disable a mailbox IRQ  PLDZ002
 *
 * This routine enables a mailbox IRQ.
 * The interrupt handler must be connected to <VME_MBOX_0/1/2/3_INT_VEC>.
 *
 * NOTE: For access to the mailbox the A16 slave must be enabled.
 *       The PLDZ002 control registers and internal RAM with the
 *       mailbox are accessable on A16 space.
 *
 * SEE ALSO: sysMailboxEnable(), sysVmeA16SlaveAddressSet()
 *
 * RETURNS: OK or ERROR
 *
 */
STATUS sysVmeMailboxIrqSetup
(
 int mbox,   /* mailbox 0..3 */
 int wr,		/* at write */
 int rd,		/* at read */
 int enable
 )
{
    u_int8       mask = 0;

    if( !G_pld.isInit ||
	mbox < 0 || 3 < mbox
	)
	return ERROR;

    if( enable )
	{
	    if( wr )
	    	mask = PLDZ002_MAIL_IRQ_WR;
	    if( rd )
	    	mask |= PLDZ002_MAIL_IRQ_RD;

	    mask <<= (mbox*2);
	    MSETMASK_D8( G_pld.base, PLDZ002_MAIL_IRQ_CTRL, mask );

	}
    else
	{
	    mask = PLDZ002_MAIL_IRQ_RD | PLDZ002_MAIL_IRQ_WR;
	    mask <<= (mbox*2);
	    MCLRMASK_D8( G_pld.base, PLDZ002_MAIL_IRQ_CTRL, mask );
	}

    return OK;
}



/*******************************************************************************
 *
 * sysMailboxInt - handle mailbox interrupt  PLDZ002
 *
 * This routine handles mailbox interrupt.
 *
 * RETURNS: N/A.
 */

LOCAL void sysMailboxInt (void)
{
    if (sysMailboxRoutine != NULL)
        sysMailboxRoutine (sysMailboxArg);
}

/******************************************************************************
 *
 * sysMailboxConnect - connect a routine to the mailbox interrupt  PLDZ002
 *
 * This routine specifies the interrupt service routine to be called at each
 * mailbox interrupt.
 *
 * RETURNS: OK or ERROR, if mailboxes are not supported.
 *
 * SEE ALSO: sysMailboxEnable()
 */

STATUS sysMailboxConnect
(
 FUNCPTR routine,    /* routine called at each mailbox interrupt */
 int     arg         /* argument with which to call routine      */
 )
{

    sysMailboxArg       = arg;
    sysMailboxRoutine   = routine;

    return (OK);
}


/******************************************************************************
 *
 * sysMailboxEnable - enable the mailbox interrupt  PLDZ002
 *
 * This routine connects and enables the mailbox interrupt sysMailboxInt()
 * at vector VME_MBOX_0_INT_VEC.
 *
 * RETURNS: OK or ERROR if mailboxes are not supported.
 *
 * SEE ALSO: sysVmeMailboxIrqSetup(), sysMailboxConnect()
 */

STATUS sysMailboxEnable
(
 char *mailboxAdrs           /* mailbox address - not used */
 )
{
    int error;

    if (!sysMailboxConnected)
	{

    	intConnect( INUM_TO_IVEC(VME_MBOX_0_INT_VEC), sysMailboxInt, 0 );
    	
	    sysMailboxConnected = TRUE;
	}

    /* enable MBOX 0 IRQ at read and write */
    error = sysVmeMailboxIrqSetup( 0, 1, 1, 1 );

    return( OK );
}


/******************************************************************************
 *
 * sysVmePLDZ002setEndianMode - sets the endian mode.
 *
 * This routine sets the endian mode of the driver.
 *
 * SEE ALSO: sysVmeBusToLocalAdrs()
 *
 * RETURNS: OK or ERROR
 */
void sysVmePLDZ002setEndianMode(
				int littleMode
				)
{
    G_pld.littleEndian = littleMode;
}

/******************************************************************************
 *
 * sysVmeBusReleaseMode - gets/sets bus release mode.
 *
 * This routine sets the bus release mode.
 *
 * NOTE: for older verions of PLDZ002:
 *       when bit not supported old/default mode is returned
 *
 * SEE ALSO: sysVmeBusRequesterMode()
 *
 * RETURNS: current release mode (SYS_VME_REL_MODE_*) or -1 on error
 */
SYS_VME_REL_MODE sysVmeBusReleaseMode
(
    SYS_VME_REL_MODE relMode
 )
{
    MACCESS ma = (MACCESS)G_pld.base;

    if( !G_pld.isInit )
	return( ERROR );

    switch( relMode ) {
    case( SYS_VME_REL_MODE_GET ):
	/* not set action requested */
	break;
    case( SYS_VME_REL_MODE_ROR ):
	/* Release-On-Request */
	MCLRMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_REQ );
	break;
    case( SYS_VME_REL_MODE_RWD ):
	/* Release-When-Done */
	MSETMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_REQ );
	break;
    default:
	logMsg("unknown VMEbus Release mode\n",1,2,3,4,5,6);
	return( ERROR );
    }

    /* return actually set mode */
    if( MREAD_D8( ma, PLDZ002_MSTR ) & PLDZ002_MSTR_REQ )
	return( SYS_VME_REL_MODE_RWD );
    else
	return( SYS_VME_REL_MODE_ROR );
}

/******************************************************************************
 *
 * sysVmeBusRequesterMode - gets/sets bus requester mode.
 *
 * This routine sets the bus release mode.
 *
 * NOTE: for older verions of PLDZ002 (not supporting this bit):
 *       when bit not supported old/default mode is returned
 *
 * SEE ALSO: sysVmeBusReleaseMode()
 *
 * RETURNS: current requester mode (SYS_VME_REQ_MODE_*) or -1 on error
 */
SYS_VME_REQ_MODE sysVmeBusRequesterMode
(
    SYS_VME_REQ_MODE reqMode
 )
{
    MACCESS ma = (MACCESS)G_pld.base;

    if( !G_pld.isInit )
	return( ERROR );

    switch( reqMode ) {
    case( SYS_VME_REQ_MODE_GET ):
	/* not set action requested */
	break;
    case( SYS_VME_REQ_MODE_FAIR ):
	/* Fair request mode */
	MSETMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_VFAIR );
	break;
    case( SYS_VME_REQ_MODE_DEMD ):
	/* standard request mode */
	MCLRMASK_D8( ma, PLDZ002_MSTR, PLDZ002_MSTR_VFAIR );
	break;
    default:
	logMsg("unknown VMEbus Requester mode\n",1,2,3,4,5,6);
	return( ERROR );
    }

    /* return actually set mode */
    if( MREAD_D8( ma, PLDZ002_MSTR ) & PLDZ002_MSTR_VFAIR )
	return( SYS_VME_REQ_MODE_FAIR );
    else
	return( SYS_VME_REQ_MODE_DEMD );
}

/***********************************************************************/
/** Start DMA with the scatter list setup by dmaSetup
 *
 */
static int DmaStart( void )
{
    /* start DMA and enable DMA interrupt and clear bus error */
    MWRITE_D8( G_pld.base, PLDZ002_DMASTA, PLDZ002_DMASTA_EN | PLDZ002_DMASTA_IEN | PLDZ002_DMASTA_ERR );
    return 0;
}

/***********************************************************************/
/** Stop DMA
 */
static int DmaStop(	void )
{
    /* stop DMA, clear flags and disable DMA interrupt */
    MWRITE_D8( G_pld.base, PLDZ002_DMASTA, PLDZ002_DMASTA_IRQ | PLDZ002_DMASTA_ERR );

    return 0;
}


/******************************************************************************
 *
 * sysMenPci2VmeDma - performs a DMA.
 *
 * This routine performs a DMA from PCI (main memory at PCI starts at 0x00000000)
 * or SRAM to the VMEbus or vice versa.
 * The following modifier combinations are possible A24 or A32 and D16 or D32
 * and SWAPPED. The routine blocks until transfer was finished or a bus error
 * or timeout occures.
 * Due to a bug of the A500 the DMA works only from/to SRAM there.
 * The last 256kB of the 1MB SRAM will be used as temporarily buffer for
 * DMA via SRAM. If the buffer which should be transfered is inside the SRAM
 * space this one will be used directly of course.
 *
 * NOTE: The KEEP_ADDR feature of the bridge is not supported so source
 *       and destination address will be incremented at each DMA transfer.
 *
 * RETURNS: OK or ERROR
 */
STATUS sysMenPci2VmeDma( 
    int wrToVme,        /* 1 write to VME, 0 read from VME */
    u_int32 *src,       /* source address - 8 byte aligned */
    u_int32 *dest,      /* destination address - 8 byte aligned */
    u_int32 size,       /* 8 byte .. 256kB 4 byte aligned */
    int viaSram,        /* 0 for A15 - 1 for A500 */
    u_int32 addrSpace,   /* VME4VX_SPC_A24_D16 .. VME4VX_SPC_A32_D32_SWAPPED */
    int timeOutTicks )  /* timeout in system ticks min 1 tick */
                         
{
    DBGCMD( static const char *functionName = __FUNCTION__ ;)
   	int error = ERROR;
    u_int32     flags = 0;
    char*       bdAddr;
    int   		bd;
    u_int8 		status;
    u_int32 	doneSize;
    u_int32 	currentSize;
    u_int32     isSupervisor;

    /* first 512 kB will be used for shared memory at default
     * at the end of the SRAM are the DMA buffer descriptors
     * located
     */
    u_int8		*sramAddr = (u_int8*)G_pld.sram + 0x80000;

    /* parameter checking */
    if( (int)src & 0x7 || (int)dest & 0x7 )
	{
	    error = ERROR;
	    DBGWRT_ERR((DBH,"*** %s src and dst must be long long aligned\n", functionName ));
	    goto CLEANUP_NO_SEM_GIVE;
	}

    if( size < 8 || size & 0x3 )
	{
	    error = ERROR;
	    DBGWRT_ERR((DBH,"*** %s size to less or not long aligned\n", functionName ));
	    goto CLEANUP_NO_SEM_GIVE;
	}

    error = semTake( &G_dmaUserSem, timeOutTicks );
    if( error )
	{
	    DBGWRT_ERR((DBH,"*** %s semTake user\n", functionName ));
	    goto CLEANUP_NO_SEM_GIVE;
	}

    doneSize = 0;
    bdAddr = (char*)G_pld.sram + PLDZ002_RV11_SRAMREGS + PLDZ002_DMABD_OFF_RV11( 0 );
    
    /* check for non-priviledged or supervisory access */
    if( G_chamVariant == 'B' && G_chamRevision >= 27 )
    {
        isSupervisor = (G_pld.addrModifier & 0x02) >> 1;
        flags |= (isSupervisor << 8);
    }

    switch( addrSpace )
    {
        case VME4VX_SPC_A24_D16:
            flags |= PLDZ002_DMABD_AM_A24D16;
            flags |= PLDZ002_DMABD_SGL;
            break;
        case VME4VX_SPC_A24_D32:
            flags |= PLDZ002_DMABD_AM_A24D32;
            flags |= PLDZ002_DMABD_SGL;
            break;
        case VME4VX_SPC_A32_D32:
            flags |= PLDZ002_DMABD_AM_A32D32;
            flags |= PLDZ002_DMABD_SGL;
            break;
        case VME4VX_SPC_A24_D16_BLT:
            flags |= PLDZ002_DMABD_AM_A24D16;
            break;
        case VME4VX_SPC_A24_D32_BLT:
            flags |= PLDZ002_DMABD_AM_A24D32;
            break;
        case VME4VX_SPC_A32_D32_BLT:
            flags |= PLDZ002_DMABD_AM_A32D32;
            break;
        case VME4VX_SPC_A24_D64_BLT:
            flags |= PLDZ002_DMABD_AM_A24D64;
            break;
        case VME4VX_SPC_A32_D64_BLT:
            flags |= PLDZ002_DMABD_AM_A32D64;
            break;
        default:
            printf("Address space access error");
            error = ERROR;
            goto CLEANUP;
    }
    
    /* DMA descriptors filling loop */
    while( doneSize < size )
    {
        currentSize = ( size - doneSize );
        if( currentSize > PLDZ002_MAX_DMA_SIZE )
        {
            currentSize = PLDZ002_MAX_DMA_SIZE;
        }
        doneSize += currentSize;
        
        /* set the size to copy */
        PLDWRITE_D32( bdAddr,
                      PLDZ002_DMABD_LEN_OFF,
                      (currentSize >> 2) - 1 );
        
        if( doneSize >= size )
        {
             flags |= PLDZ002_DMABD_END;
        }

        if( viaSram )
        {
            if( wrToVme ) 
            {
                /* copy it into SRAM */
                memcpy( sramAddr, src, currentSize ); 
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_FLAGS_OFF,
                              PLDZ002_DMABD_SRC( PLDZ002_DMABD_DIR_SRAM )
                              | PLDZ002_DMABD_DST( PLDZ002_DMABD_DIR_VME ) | flags );
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_SRCADR_OFF,
                              (u_int32)sramAddr );
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_DESTADR_OFF,
                              (u_int32)dest );
            }
            else 
            {
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_FLAGS_OFF,
                              PLDZ002_DMABD_SRC( PLDZ002_DMABD_DIR_VME )
                              | PLDZ002_DMABD_DST( PLDZ002_DMABD_DIR_SRAM ) | flags );
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_DESTADR_OFF,
                              (u_int32)sramAddr );
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_SRCADR_OFF,
                              (u_int32)src );
            }
        }
        else
        {
            PLDWRITE_D32( bdAddr,
                          PLDZ002_DMABD_DESTADR_OFF,
                          (u_int32)dest );
            PLDWRITE_D32( bdAddr,
                          PLDZ002_DMABD_SRCADR_OFF,
                          (u_int32)src );
            
            if( wrToVme ) 
            {
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_FLAGS_OFF,
                              PLDZ002_DMABD_SRC( PLDZ002_DMABD_DIR_PCI )
                              | PLDZ002_DMABD_DST( PLDZ002_DMABD_DIR_VME ) | flags );
            }
            else 
            {
                PLDWRITE_D32( bdAddr,
                              PLDZ002_DMABD_FLAGS_OFF,
                              PLDZ002_DMABD_SRC( PLDZ002_DMABD_DIR_VME )
                              | PLDZ002_DMABD_DST( PLDZ002_DMABD_DIR_PCI ) | flags );
            }
        }
        /* increment to the next descriptor */
        bdAddr += 0x10;
    }
    
    error = DmaStart();
    if( error )
    {
        printf("DMA start Error !!!\n");
        DBGWRT_ERR((DBH,"*** %s DMA busy\n", functionName ));
        goto CLEANUP;
    }
    
    error = semTake( &G_dmaIrqSem, timeOutTicks );
    if( error )
    {
        printf("DMA Error taking the synchonisation semaphore !!!\n");
        DBGWRT_ERR((DBH,"*** %s semTake no IRQ\n", functionName ));
        goto CLEANUP;
    }
    
    /* copy the content of SRAM to local PCI address when needed */
    if( viaSram && !wrToVme )
    {
        memcpy( dest, sramAddr, doneSize );
    }

 CLEANUP:
    DmaStop();
    semGive( &G_dmaUserSem );
 CLEANUP_NO_SEM_GIVE:
    return( error );
}

void sysMenPci2VmeSpaceName( int spcNumber, char* spcName )
{
    switch( spcNumber )
    {
        case VME4VX_SPC_A16_D16:
            strcpy(spcName, "VME4VX_SPC_A16_D16" );
            break;
            
        case VME4VX_SPC_A24_D64_BLT:
            strcpy(spcName, "VME4VX_SPC_A24_D64_BLT" );
            break;
        
        case VME4VX_SPC_A16_D32:
            strcpy(spcName, "VME4VX_SPC_A16_D32" );
            break;
        
        case VME4VX_SPC_A24_D16:
            strcpy(spcName, "VME4VX_SPC_A24_D16" );
            break;
            
        case VME4VX_SPC_A24_D16_BLT:
            strcpy(spcName, "VME4VX_SPC_A24_D16_BLT" );
            break;
        
        case VME4VX_SPC_A24_D32:
            strcpy(spcName, "VME4VX_SPC_A24_D32" );
            break;
        
        case VME4VX_SPC_A24_D32_BLT:
            strcpy(spcName, "VME4VX_SPC_A24_D32_BLT" );
            break;
            
        case VME4VX_SPC_A32_D32:
            strcpy(spcName, "VME4VX_SPC_A32_D32" );
            break;
            
        case VME4VX_SPC_A32_D32_BLT:
            strcpy(spcName, "VME4VX_SPC_A32_D32_BLT" );
            break;
            
        case VME4VX_SPC_A32_D64_BLT:
            strcpy(spcName, "VME4VX_SPC_A32_D64_BLT" );
            break;
            
        default:
            strcpy(spcName, " /!\\ Unsuported /!\\ " );
    }
    return spcName;
}
