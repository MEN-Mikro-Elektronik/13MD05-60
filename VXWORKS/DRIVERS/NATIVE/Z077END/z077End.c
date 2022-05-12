/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: z077End.c
 *      Project: VxWorks END driver for MEN Z087_ETH
 *
 *       Author: ts
 *
 *  Description: VxWorks Enhanced Network Driver (END) for
 *               MEN 16Z087_ETH FPGA IP Core
 *
 *     Required:  -
 *     Switches:  DBG
 *     			  Z77_USE_VXBPCI_FUNCS  -  use vxBus interrupt
 *     			                           connect/enable default routines
 *                                         (NOT needed when vxbMenGirq driver is used!)
 *---------------------------------------------------------------------------
 * (c) Copyright by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
/*
  modification history
  --------------------
  01a,20feb06,ts   initially written.
  01b,07sep17,ts   major rework :-) .
*/

/*
  DESCRIPTION
  ===========

  1. Driver initialization
  ^^^^^^^^^^^^^^^^^^^^^^^^
  The Z077 END driver supports the 16Z087 MEN Ethernet MAC IP core.

  To integrate the driver into a BSP the following steps have to be taken:
  a) global initialization of z77 driver is done in z077Init() to be called within sysHwInit2().
  b) copy the code snippet below into a sysZ077END.c and adapt the configuration table
  c) provide the G_z077ConfSetTbl pointer to z077Init()
*/
#if 0
/* << configure all Z77/87 cores here: >> */
/*
 *  { <END unit> , <PCI path> , <chameleon instance> , <Addr. of conn. PHY> ,
 *     <MAC address callback> , <use PCI interr.> , <interrupt offset> , 0 }
 */
Z77_CONF_SET G_z077ConfSetTbl [] = {
	{0, "0x1e 0x05", 0, 0, NULL, TRUE, 0x20, 0},
	{1, "0x1e 0x05", 1, 1, NULL, TRUE, 0x20, 0},
	{Z077_EOL, NULL, 0, 0, NULL, 0} /* "end of list" entry */
};
#endif
/*
  Usage:

  <END unit>
  This is an index to specify the END unit # (men_sec0,1,...)
  <PCI path> [or direct location]
  PCI path that points to a chameleon device that contains the Z87 core.
  The last entry (here 0x05) is the device # of the chameleon device.

  The PCI location can also be passed as direct location in the easier to understand
  format PCI[domain]/bus/dev/function:
  The string must then be passed in the form of "PCIX:BBB.DD:FF".
  X=PCI domain (0 for CPUs with one PCI host controller).
  Bus/device/function are in DECIMAL numbers. Bus=[0:255] device=[0:31],
  function=0 (MEN Chameleon FPGA contains no multifunction PCI headers).

  <chameleon instance>
  Selects the chameleon instance within chameleon table
  <Addr. of conn. PHY>
  Address of PHY connected to the specific core
  <MAC address callback>
  Callback routine to get MAC address from BSP.
  <use PCI interr.>
  Set this TRUE to use PCI interrupts instead of chameleon interrupts
  <interrupt offset>
  This offset is added to PCI/chameleon ivec. (0x20 on x86)

  d) configure the men_sec instances in configNet.h

*/

/* includes */
#include <vxWorks.h>
#include <version.h>
#include <stdlib.h>
#include <cacheLib.h>
#include <intLib.h>
#include <end.h>            /* Common END structures. */
#include <endLib.h>
#include <lstLib.h>         /* Needed to maintain protocol list. */
#include <iv.h>
#include <semLib.h>
#include <logLib.h>
#include <netLib.h>
#include <stdio.h>
#include <sysLib.h>
#include <tickLib.h>
#include <errno.h>
#include <errnoLib.h>
#include <memLib.h>
#include <iosLib.h>
#include <miiLib.h>         /* PHY regs */
#if _WRS_VXWORKS_MAJOR == 7
#include <hwif/vxBus.h>
#include <subsys/int/vxbIntLib.h>
#include <hwif/buslib/pciDefines.h>
#include <hwif/buslib/vxbPciLib.h>
#else
#include <hwif/vxbus/vxBus.h>
#include <vxbus/vxbPciBus.h>
#include <drv/pci/pciIntLib.h>
#include <drv/pci/pciConfigLib.h>
#endif
#include <etherMultiLib.h>      /* multicast stuff. */
#include <MEN/dbg.h>
#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <net/mbuf.h>
/* #include <net/unixLib.h> */
/* #include <net/protosw.h> */
/* #include <net/systm.h> */
/* #include <net/route.h> */
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <types/vxCpu.h>
#include <MEN/chameleon.h>
#include <MEN/eeprod.h>
#include "z077End.h"

/* defines */
#define Z077_DRV_NAME       "men_sec"
#define DRV_NAME_LEN        (sizeof(Z077_DRV_NAME)+1)
#define UNIT_DRIVER_NAME    Z077_DRV_NAME" "
#define Z87_RST_TIMEOUT     5

#define Z077_MIN_PACKLEN    0x3C
#define Z077_MAX_PACKLEN    0x600

/* error handling for direct PCI dev. specification */
#define CHK_NUMBER(X)  if (((X) < '0') && ((X) > '9')) {	\
		errval = ERROR;										\
		goto errout;										\
	}

INT16 z077PhyAnOrderTbl [] = {
	-1 	/* Signal end of table 		*/
};

LOCAL CHAM_FUNCTBL  LocChamFctTable;   /* Chameleon function table */
DBG_HANDLE 			*LocDbgHdlP = NULL;        /* Men-DBG handle */
int					z077_DbgRxCount=0;


#ifdef MAC_IO_MAPPED
#define CHAM_INIT            (CHAM_InitIo)
#else
#define CHAM_INIT            (CHAM_InitMem)
#endif /* MAC_IO_MAPPED */

/* sanity checks */
#if (Z077_TBD_NUM < 1) || (Z077_RBD_NUM < 1)
# error Z077_TBD/RBD_NUM must be at least 1 !
#endif

/* Configuration items */
#define Z077END_BUFSIZ      1500            /* MTU seen at ifShow() command */
#define EH_SIZE             (14)            /* 2(type)+6(src)+6(dst)        */
#define END_SPEED           MII_10MBS

/* Definitions for the flags field */
#define Z077_POLLING        0x2
#define Z077_MAX_CL_LEN     0x700   /* MTU + overhead ( aligned )*/
#define MII_RXER_COUNTER    0x15 	/* Auto-Negotiation        */
#define TXRX_IDLE			0x60000 /* moder[17:18], TX/RX idle bits */
#define Z077_MOTSEC_BD      0x10

/* DEBUG MACROS */
#undef DEBUG_MBLK

/* central switch to en/disable debug functionalities. uncomment as needed. */

#undef Z077_DEBUG_DRIVER

#ifdef Z077_DEBUG_DRIVER
u_int32 G_z077DebugLevel = (DBG_NORM | DBG_LEVERR | 0xf);
#define DBG_MYLEVEL  (G_z077DebugLevel)
#define DBH  (LocDbgHdlP)

#define DRV_DEBUG_OFF          0
#define DRV_DEBUG_RX           DBG_LEV3
#define DRV_DEBUG_TX           DBG_LEV4
#define DRV_DEBUG_INT          DBG_LEV1
#define DRV_DEBUG_MII          DBG_LEV5
#define DRV_DEBUG_LOAD         DBG_LEV2
#define DRV_DEBUG_IOCTL        DBG_LEV6
#define DRV_DEBUG_ENTER        DBG_LEV1
#define DRV_DEBUG_PHINIT       DBG_LEV1
#define DRV_DEBUG_MEM          DBG_LEV7
#define DRV_DEBUG_POLL         DBG_LEV8
#define DRV_DEBUG_INIT         DBG_LEV2

#else /*Z077_DEBUG_DRIVER*/
u_int32 G_z077DebugLevel = (DBG_NORM_INTR | DBG_LEVERR | 0);
#define DBG_MYLEVEL  (G_z077DebugLevel)
#define DBH  (LocDbgHdlP)

#endif /*Z077_DEBUG_DRIVER*/

/* during Autonegotiation we need to wait rather long */
#define SECOND		sysClkRateGet()
#define TICKLEN_MS	(unsigned int) (1000 / (SECOND))

/*
 *  Macros to skip through the RX0/1 Registers. they
 *  shift from BD0 to BD31 resp. BD32 to BD63. when a nonempty bit is found
 *  process this Frames BD and jump to end of HandleRecv(). Taylored for
 *  use inside HandleRecv() function.
 */
#define CHECK_RX1_EMPTY()										\
	for (emptyBitPos=0; emptyBitPos < 32; emptyBitPos++ ) {		\
		if( (rx1 & ( 1 << emptyBitPos))) {						\
			z077Recv(pDrvCtrl, emptyBitPos + 32, NULL, FALSE );	\
			break;												\
		}														\
	}

#define CHECK_RX0_EMPTY()									\
	for (emptyBitPos=0; emptyBitPos < 32; emptyBitPos++ ) { \
		if( (rx0 & ( 1 << emptyBitPos))) {					\
			z077Recv(pDrvCtrl, emptyBitPos, NULL, FALSE );	\
			break;											\
		}													\
	}

#define CHECK_RX1_EMPTY_POLL()											\
	for (emptyBitPos=0; emptyBitPos < 32; emptyBitPos++ ) {				\
		if( (rx1 & ( 1 << emptyBitPos))) {								\
			retVal = z077Recv(pDrvCtrl, emptyBitPos + 32, pMblk, TRUE ); \
			break;														\
		}																\
	}

#define CHECK_RX0_EMPTY_POLL()										\
	for (emptyBitPos=0; emptyBitPos < 32; emptyBitPos++ ) {			\
		if( (rx0 & ( 1 << emptyBitPos))) {							\
			retVal = z077Recv(pDrvCtrl, emptyBitPos, pMblk, TRUE );	\
			break;													\
		}															\
	}

/* LOCALS */
/* keep Register descriptions in a comfortable struct */
const Z077_REG_INFO z77_reginfo[]={
    {"MODER     ", Z077_REG_MODER       },
    {"INT_SRC   ", Z077_REG_INT_SRC     },
    {"INT_MASK  ", Z077_REG_INT_MASK    },
    {"IPGT      ", Z077_REG_IPGT        },
    {"IPGR1     ", Z077_REG_IPGR1       },
    {"IPGR2     ", Z077_REG_IPGR2       },
    {"PACKLEN   ", Z077_REG_PACKLEN     },
    {"COLLCONF  ", Z077_REG_COLLCONF    },
    {"TX_BDNUM  ", Z077_REG_TX_BDNUM    },
    {"CTRLMODER ", Z077_REG_CTRLMODER   },
    {"MIIMODER  ", Z077_REG_MIIMODER    },
    {"MIICMD    ", Z077_REG_MIICMD      },
    {"MIIADR    ", Z077_REG_MIIADR      },
    {"MIITX_DATA", Z077_REG_MIITX_DATA  },
    {"MIIRX_DATA", Z077_REG_MIIRX_DATA  },
    {"MIISTATUS ", Z077_REG_MIISTATUS   },
    {"MAC_ADDR0 ", Z077_REG_MAC_ADDR0   },
    {"MAC_ADDR1 ", Z077_REG_MAC_ADDR1   },
    {"HASH_ADDR0", Z077_REG_HASH_ADDR0  },
    {"HASH_ADDR1", Z077_REG_HASH_ADDR1  },
    {"TXCTRL    ", Z077_REG_TXCTRL      },
    {"GLOBAL RST", Z077_REG_GLOBALRST   },
    {"BD START  ", Z077_REG_BDSTART     },
    {"RX EMPTY0 ", Z077_REG_RXEMPTY0    },
    {"RX EMPTY1 ", Z077_REG_RXEMPTY1    },
    {"TX EMPTY0 ", Z077_REG_TXEMPTY0    },
    {"TX EMPTY1 ", Z077_REG_TXEMPTY1    },
    {"RX BD STAT", Z087_REG_RX_BD_STATUS     },
    {"CORE REV  ", Z087_REG_CORE_REVISION    },
    {NULL, 0xffff} /* end mark */
};

/* global Z077 management structure */
typedef struct {/* Z077_MGNT*/
    u_int8      noChamDev;              /**< number of detected Cham. devs */
    u_int8      noEndUnits;             /**< number of Z077/Z087 in dev */
    Z077_INT_CONNECT_FNT fIntConnectP;  /**< function pointer to interrupt connect routine */
    Z077_INT_ENABLE_FNT fIntEnableP;    /**< function pointer to interrupt enable routine */
    Z077_END_DEVICE *endUnit[Z077_MAX_UNITS];  /**< pointers to the END instances */
} Z077_MGNT;

/* For driver initialization */
LOCAL STATUS z077GetBusBehindBridge(Z077_PCI_DESC *, int32 *);

/* These functions are called on instance basis */
END_OBJ *    z077Load(char* initString, void* datP);
LOCAL STATUS z077Start(Z077_END_DEVICE* pDrvCtrl);
LOCAL STATUS z077Stop(Z077_END_DEVICE* pDrvCtrl);
LOCAL int    z077Ioctl(Z077_END_DEVICE* pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS z077Unload(Z077_END_DEVICE* pDrvCtrl);
LOCAL STATUS z077Send(Z077_END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);

LOCAL STATUS z077MCastAdd(Z077_END_DEVICE* pDrvCtrl, char* pAddress);
LOCAL STATUS z077MCastDel(Z077_END_DEVICE* pDrvCtrl, char* pAddress);
LOCAL STATUS z077MCastGet(Z077_END_DEVICE* pDrvCtrl, MULTI_TABLE* pTable);
LOCAL STATUS z077PollStart(Z077_END_DEVICE* pDrvCtrl);
LOCAL STATUS z077PollStop(Z077_END_DEVICE* pDrvCtrl);
LOCAL STATUS z077PollSend(Z077_END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);
LOCAL STATUS z077PollRcv(Z077_END_DEVICE* pDrvCtrl, M_BLK_ID pBuf);
/* LOCAL void    z077AddrFilterSet(Z077_END_DEVICE *pDrvCtrl); */
LOCAL STATUS z077PciParse(int8 * pathStringP, int8 * devicePathP);
LOCAL STATUS z077InitParseUnit (char * , int *);
LOCAL STATUS z077InitParseRest (Z077_END_DEVICE * pDrvCtrl, char * initString);
LOCAL STATUS z077MemInit(       Z077_END_DEVICE * pDrvCtrl  /* device to be initialized */ );

LOCAL STATUS z077Reset( Z077_END_DEVICE *pDrvCtrl);
LOCAL void  z077Restart( Z077_END_DEVICE *pDrvCtrl);
LOCAL void  z077Int( Z077_END_DEVICE *pDrvCtrl);
LOCAL void  z077HandleRecv (Z077_END_DEVICE *pDrvCtrl, int irqFired );
LOCAL void  z077HandleRxErr(Z077_END_DEVICE *pDrvCtrl);
LOCAL void  z077HandleTxErr(Z077_END_DEVICE *pDrvCtrl);
/* LOCAL void z077HandleTxInt( Z077_END_DEVICE *pDrvCtrl); */
LOCAL void z077kickTransmission(Z077_END_DEVICE *pDrvCtrl, UINT32 idxTx);
LOCAL STATUS z077Recv(Z077_END_DEVICE *pDrvCtrl, UINT32 pktIndex,
                      M_BLK_ID pMblk, BOOL isPollMode);
LOCAL void   z077Config(Z077_END_DEVICE *pDrvCtrl);
LOCAL UINT   z077StatusRead (Z077_END_DEVICE *pDrvCtrl);
LOCAL STATUS z077BdSetup( Z077_END_DEVICE *pDrvCtrl );
LOCAL Z077_MGNT z077Mgnt;   /* main driver management structure */

/* functions to interact with the PHY */
LOCAL STATUS z077MiiBasicCheck(Z077_END_DEVICE *pDrvCtrl,UINT8 phyAddr);
LOCAL STATUS z077MiiAnRun(Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr);
LOCAL STATUS z077MiiAnStart(Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr);
LOCAL STATUS z077MiiAnCheck(Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr);
LOCAL STATUS z077MiiProbe(Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr);

LOCAL STATUS z077PhyInit ( Z077_END_DEVICE* pDrvCtrl );
LOCAL STATUS z077MiiRead( Z077_END_DEVICE * pDrvCtrl, UINT8 phyAddr,
                          UINT8 regAddr, UINT16 *retVal );
LOCAL STATUS z077MiiWrite ( Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr,
                            UINT8 regAddr, UINT16 data );
LOCAL STATUS z077PhyReset( Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr );
LOCAL STATUS z077MiiModeForce( Z077_END_DEVICE *pDrvCtrl, UINT8 phyAddr );

#if _WRS_VXWORKS_MAJOR == 7
IMPORT VXB_DEV_ID sysGetPciCtrlID(int Inst);
IMPORT VXB_DEV_ID sysGetFpgaPciCtrlID(void);
#else
IMPORT VXB_DEVICE_ID sysGetPciCtrlID(int Inst);
#endif

#define Z077_PHY_MEN_EMU_ID1			0x000D
#define Z077_PHY_MEN_EMU_ID2			0x7000
#define Z077_PHY_MICREL_ID1				0x0022
#define Z077_PHY_MICREL_ID2_MASK 		0xFFF0	/* bits 3-0 are revision bits and may vary */
#define Z077_PHY_MICREL_ID2_KS8721		0x1610
#define Z077_PHY_MICREL_ID2_KSZ8041NL	0x1510
#define Z077_PHY_MICREL_ID2_KS8995		0x1450

#define Z077_PHY_INTEL_ID1				0x0013
#define Z077_PHY_INTEL_ID2_MASK 		0xFFF0	/* bits 3-0 are revision bits and may vary */
#define Z077_PHY_INTEL_ID2_LXT971A 		0x78e0

/* table of connected PHYs. Add more possible Phys to come here        */
/* currently only ident#1 register is evaluated!                       */
/*	{ ident#1,             ident#2,                       PHY name } } */
LOCAL PHY_DEVICE_TBL z077PhyAttachTbl[] = {
    { Z077_PHY_MICREL_ID1, Z077_PHY_MICREL_ID2_KS8721,    	"Micrel KS8721BL"	},
    { Z077_PHY_MICREL_ID1, Z077_PHY_MICREL_ID2_KSZ8041NL, 	"Micrel KSZ8041NF"	},
    { Z077_PHY_MICREL_ID1, Z077_PHY_MICREL_ID2_KS8995,    	"Micrel KS8995MA/FQ"},
    { Z077_PHY_INTEL_ID1,  Z077_PHY_INTEL_ID2_LXT971A, 	  	"Intel LXT971A"		},
    { Z077_PHY_MEN_EMU_ID1,Z077_PHY_MEN_EMU_ID2, 			"MEN emulation PHY"	},
    { 0xffff, 0xffff, ""}
};

/* END Specific interfaces. */
LOCAL UINT16 z77AnLookupTbl [] = {
    MII_TECH_100BASE_TX_FD,
    MII_TECH_10BASE_FD,
    MII_TECH_100BASE_TX,
    MII_TECH_10BASE_T,
};

/*
 * Declare our function table.  This is static across all device
 * instances.
 */

LOCAL NET_FUNCS z077FuncTable =
{
    (FUNCPTR)z077Start,     /* Function to start the device.                */
    (FUNCPTR)z077Stop,      /* Function to stop the device.                 */
    (FUNCPTR)z077Unload,    /* Unloading function for the driver.           */
    (FUNCPTR)z077Ioctl,     /* Ioctl function for the driver.               */
    (FUNCPTR)z077Send,      /* Send function for the driver.                */
    (FUNCPTR)z077MCastAdd,  /* Multicast add function for the driver.       */
    (FUNCPTR)z077MCastDel,  /* Multicast delete function for the driver.    */
    (FUNCPTR)z077MCastGet,  /* Multicast retrieve function for the driver.  */
    (FUNCPTR)z077PollSend,  /* Polling send function                        */
    (FUNCPTR)z077PollRcv,   /* Polling receive function                     */

    endEtherAddressForm,    /* put address info into a NET_BUFFER           */
    endEtherPacketDataGet,  /* get pointer to data in NET_BUFFER            */
    endEtherPacketAddrGet   /* Get packet addresses.                        */
};

/**********************************************************************/
/** Routine to initialize local variables
 *
 *  This is called to initialize the local variables of this module. It
 *  was created for p511prog to use debugging and ChamFctTable
 *
 *  \param pModName   A pointer to a individual debug string
 *
 *  \return OK/ERROR
 */
UINT32 z077ModInit( char * pModName )
{
    /* Init Men-DBG */
    if(LocDbgHdlP == NULL)
    {
        DBGINIT(((pModName), (DBG_HANDLE **)&LocDbgHdlP));
	}

    /* Init ChamFctTable */
    if(CHAM_INIT(&LocChamFctTable) != CHAMELEON_OK)
    {
        DBGWRT_ERR( (DBH, "*** z077Init: Chameleon V2 initialization failed.\n") );
        return ERROR;
    }
    return OK;
}

/**********************************************************************/
/** Routine to parse a directly specified PCI location for a
 *  Z87 containing device
 *
 *  The string must be directly passed in the form of "PCIX:BBB.DD:FF"
 *  (bus/device/function in DECIMAL numbers. Bus=[0:255] device=[0:31],
 *  function is usually 0 (MEN Chameleon FPGA contains no multifunction
 *  PCI headers).
 **/
LOCAL STATUS DRV_ParseDirectPciLoc( char *pStr, int* dom, int* bus, int* dv, int* fn)
{
#define PCI_LOC_NR_ITEMS	4  /* we want domain,bus,dev,fct number to be parsed */

	STATUS errval = OK;

    if (sscanf(pStr, "PCI%d:%d.%d.%d", dom, bus, dv, fn ) < PCI_LOC_NR_ITEMS)
    {
    	DBGWRT_ERR( (DBH, "*** DRV_ParseDirectPciLoc: failed to assign all values, check location string '%s'!\n", pStr));
    	errval = ERROR;
    }

	return errval;
}

/**********************************************************************/
/** Routine to find and initialize the 16Z077/16Z087 instances
 *
 *  This routine follows the PCI paths given in confSetTbl and looks for 16Z077/16Z087 instances
 *  and creates a Z077_END_DEVICE for each instance.
 *
 *  \param confSetTbl   pointer to configiguration table
 *
 *  \return OK/ERROR
 */
UINT32 z077Init(Z77_CONF_SET * confSetTbl)
{
    UINT32 actConfSet = 0, j = 0, pathElement = 0;
    int pdom=0, pb=0, pd=0, pf=0;
    CHAMELEONV2_HANDLE * chamHdl = NULL;
    CHAMELEONV2_UNIT chamUnit;
    Z077_END_DEVICE* pEndDev = NULL;
    char *pPciLoc=NULL;
    Z077_PCI_DESC pciDesc={0};
    UINT8 tmpDevicePath[Z077_MAX_PCI_PATH];
    int32 actBus=0;
    INT32 temp32 = 0;

    if(z077ModInit("Z077Driver"))
        return ERROR;

#ifdef Z077_BIG_ENDIAN
	DBGWRT_1((DBH, "z077Init: BIG_ENDIAN\n"));
#else
	DBGWRT_1((DBH, "z077Init: LITTLE_ENDIAN\n"));
#endif

    /* confSetTbl NULL? */
    if(confSetTbl == NULL){
        DBGWRT_ERR( (DBH, "*** z077Init: confSetTbl NULL!\n") );
        return ERROR;
    }

    /* Init Mgnt structure */
    z077Mgnt.fIntConnectP 	= NULL;
    z077Mgnt.fIntEnableP 	= NULL;
    z077Mgnt.noEndUnits 	= 0;
    z077Mgnt.noChamDev 		= 0;
    for(temp32 = 0; temp32 < Z077_MAX_UNITS; temp32++)
    {
        z077Mgnt.endUnit[temp32] = NULL;
    }

    /* "found" must be 0 in all confSets */
    for(actConfSet = 0; confSetTbl[actConfSet].endUnit != Z077_EOL; actConfSet++)
    {
        confSetTbl[actConfSet].found = 0;
    }

    /* process configSetTbl,  create and init END devices for each entry in confSet */
    for( actConfSet = 0; confSetTbl[actConfSet].endUnit != Z077_EOL; actConfSet++ )
    {
    	pPciLoc = confSetTbl[actConfSet].pciPathStr;
        /* have we already found this cham. device? */
        if(confSetTbl[actConfSet].found == 0)
        {
            /* zero array */
            bzero((char*) tmpDevicePath, sizeof(UINT8)*Z077_MAX_PCI_PATH);

            /* parse PCI path string */
            if(confSetTbl[actConfSet].pciPathStr == NULL)
            {
                DBGWRT_ERR( (DBH, "*** z077Init: PCI path string NULL! (confSet %u)\n", actConfSet) );
                continue;
            }
            else
            {
                DBGWRT_1( (DBH, "Z077 - Checking PCI Path: \"%s\"\n",  confSetTbl[actConfSet].pciPathStr) );
            }

            /* -------- process direct specified domain/bus/dev/func --- */
			if (pPciLoc[0] == 'P' && pPciLoc[1] == 'C' && pPciLoc[2] == 'I')
			{	/* direct PCI device specified, omit PCI path scanning. Interpret the numbers behind "PCI" */
				DRV_ParseDirectPciLoc(pPciLoc, &pdom,&pb,&pd,&pf);
				pciDesc.domainNo 	= pdom;
				pciDesc.busNo 		= OSS_MERGE_BUS_DOMAIN(pb, pdom);
				pciDesc.deviceNo 	= pd;
				pciDesc.funcNo 		= pf;

				DBGWRT_1((DBH, "Z077 - found direct PCI location: d/b/d/f = %x/%x/%x/%x\n",
						  pciDesc.domainNo, pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo ));
			}
			else /* -------- process standard PCI device ID path list --- */
			{
				if( z077PciParse( (int8*)pPciLoc, (int8*)tmpDevicePath) )
				{
					DBGWRT_ERR( (DBH, "*** z077Init: Bad PCI string END unit %u!\n",actConfSet) );
					continue;
				}
				else
				{
					DBGWRT_1( (DBH, "Z077 - PCI path: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
							   tmpDevicePath[0], tmpDevicePath[1], tmpDevicePath[2],
							   tmpDevicePath[3], tmpDevicePath[4], tmpDevicePath[5]) );
				}

				/*
				 * Does this device exist?
				 * Search through busses - but only till "last element - 1" (last element is device nr.)
				 */
				pathElement = 0;
				actBus = 0;
				bzero((char *) &pciDesc,sizeof(Z077_PCI_DESC));

				/* search through PCI2PCI bridges */
				while( pathElement < (Z077_MAX_PCI_PATH-1))
				{
					/* get bus behind bridge and get device header */
					pciDesc.busNo = actBus;
					pciDesc.deviceNo = tmpDevicePath[pathElement];
					pciDesc.funcNo = 0;
					pathElement++;
					DBGWRT_1( (DBH, "Z077 - Getting sec. busNo from %u/%u/%u\n",
							   actBus, pciDesc.deviceNo, pciDesc.funcNo) );
					if((temp32 = z077GetBusBehindBridge(&pciDesc, &actBus)) == ERROR)
					{
						break;
					}
				}

				/* use detected secondary bus number if last bus check worked */
				if(temp32 != ERROR)
				{
					pciDesc.busNo = actBus;
					pciDesc.deviceNo = tmpDevicePath[pathElement];
				}
			} /* end of direct / path tracing PCI device search, now device is known */

			DBGWRT_1( (DBH, "Z077 - confSet %d points to d/b/d/f %x/%x/%x/%x \n",
					   actConfSet, pciDesc.domainNo, pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo) );

            /* Check if this is a chameleon device */
            if(z077CheckChameleonDevice(&pciDesc) == ERROR)
            {
                DBGWRT_1( (DBH, "*** z077Init: Device in path %u (%u/%u/%u/%u) is not a chameleon device!\n",
						   actConfSet, pciDesc.domainNo, pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo) );
                continue;
            }
            /* we look trough rest of list if we have other entries with the same path and set the found-flag */
            if(confSetTbl[actConfSet+1].endUnit != Z077_EOL) /* make this is not the last entry */
            {
                for(j = actConfSet+1; confSetTbl[j].endUnit != Z077_EOL; j++)
                {
                    if(confSetTbl[j].pciPathStr == NULL)
                        continue;
                    if(strcmp(confSetTbl[actConfSet].pciPathStr, confSetTbl[j].pciPathStr) == 0)
                        confSetTbl[j].found = 1;
                }
            }
        }
        else
        {
            DBGWRT_1( (DBH, "Z077 - Device in confSet %u b/d/f %u/%u/%u already found\n",
					   actConfSet, pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo) );
        }

        /* --- here we should know the PCI bus dev fct num --- */

        /* Find Z77/Z78 core in cham. table */
        bzero((char *) &chamUnit, sizeof(CHAMELEONV2_UNIT));
        chamHdl = NULL;
        /* Init cham. dev. */
		if(z077InitCham(
			   &pciDesc,
			   &chamHdl) != OK)
        {
            DBGWRT_ERR( (DBH, "*** z077Init: Chameleon V2 PCI initialization failed.\n") );
            continue;
        }
        /* Find Z87 core */
        if(z077FindZxCore( (INT8*)&confSetTbl[actConfSet].chamInstance, Z077_CHAMID_16Z087_ETH, &chamHdl, &chamUnit) != OK)
        {
            /* Find Z77 core */
            if(z077FindZxCore(
				   (INT8*)&confSetTbl[actConfSet].chamInstance,
				   Z077_CHAMID_16Z077_ETH,
				   &chamHdl,
				   &chamUnit) != OK)
            {
	            /* nothing found */
	            DBGWRT_ERR( (DBH, "*** z077Init: Unit %d not found in Cameleon device %u/%u/%u\n",
							 confSetTbl[actConfSet].endUnit, pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo) );
	            continue;
            }
        }
        DBGWRT_1( (DBH, "Z077 - Found 16Z%03u rev %02x instance %d in Cameleon device %u/%u/%u\n",
				   chamUnit.devId, chamUnit.revision, confSetTbl[actConfSet].chamInstance,
				   pciDesc.busNo, pciDesc.deviceNo, pciDesc.funcNo) );

        /* finally allocate END structure*/
        pEndDev = (Z077_END_DEVICE *)calloc( sizeof(Z077_END_DEVICE), 1);
        if(!pEndDev) {
            DBGWRT_ERR( (DBH, "*** z077Init: calloc for END unit %u failed!\n",
						 confSetTbl[actConfSet].endUnit) );
            return ERROR;
        }

        /* allocate PHY  structure*/
        pEndDev->phyInfo = (PHY_INFO *)calloc(sizeof(PHY_INFO), 1);
        if (!pEndDev->phyInfo) {
            DBGWRT_ERR( (DBH, "*** z077Init: calloc for PHY of END unit %u failed!\n",
						 confSetTbl[actConfSet].endUnit) );
			if (pEndDev) {
				free( pEndDev );
			}
            return ERROR;
        }

        /* initialize END */
        pEndDev->baseAddr 			= (UINT32) chamUnit.addr;
        pEndDev->chamUnitRevision	= chamUnit.revision;
        pEndDev->unit 				= confSetTbl[actConfSet].endUnit;
        pEndDev->phyInfo->phyAddr 	= confSetTbl[actConfSet].phyAddr;
        pEndDev->usePciIrq 			= confSetTbl[actConfSet].usePciIrq;
        pEndDev->iOffset 			= confSetTbl[actConfSet].interruptOffset;
        pEndDev->getMACFunc	 		= confSetTbl[actConfSet].getMACaddrFunc;

        pEndDev->pciDesc.domainNo 	= pciDesc.domainNo;
        pEndDev->pciDesc.busNo 		= pciDesc.busNo;
        pEndDev->pciDesc.deviceNo 	= pciDesc.deviceNo;
        pEndDev->pciDesc.funcNo 	= pciDesc.funcNo;

        /* set Interrupt */
        if( pEndDev->usePciIrq == TRUE )
		{
            int32 vec;
            OSS_PciGetConfig(NULL,
							 OSS_MERGE_BUS_DOMAIN(pciDesc.busNo, pciDesc.domainNo),
				             pciDesc.deviceNo,
				             pciDesc.funcNo,
				             OSS_PCI_INTERRUPT_LINE,
				             &vec);
            pEndDev->ivec = vec;
        }
        else 
		{
            pEndDev->ivec = chamUnit.interrupt;
        }
        pEndDev->ivec += pEndDev->iOffset;
        DBGWRT_1( (DBH, "Z077 - Unit %d uses %s interrupt %u\n",
				   confSetTbl[actConfSet].endUnit,
				   confSetTbl[actConfSet].usePciIrq ? "PCI" : "Chameleon",
				   pEndDev->ivec ) );

        /* everything ok - we can store infos in z077Mgnt */
        z077Mgnt.endUnit[actConfSet] = pEndDev;
        z077Mgnt.noEndUnits++;
    } /* end of for confSet...*/

    DBGWRT_1( (DBH, "Z077 - Successfully initialized %d instances\n", z077Mgnt.noEndUnits) );
    return OK;
}

/**********************************************************************/
/** Routine to check a given PCI device if it is a chameleon device
 *
 *  This routine looks at a PCI device if the vendor ID is Altera (0x1172) or MEN (0x1A88)
 *  and if the device ID is Chameleon (0x4d45).
 *
 *  \param pDev     pointer to PCI descriptor (bus/device/function)
 *
 *  \return             STATUS - OK if chameleon device, else ERROR
 */
STATUS z077CheckChameleonDevice(Z077_PCI_DESC * pDev)
{
    u_int32 device = 0;  /* device ID */
    u_int32 vendor = 0;  /* vendor ID */

	OSS_PciGetConfig(NULL, OSS_MERGE_BUS_DOMAIN(pDev->busNo, pDev->domainNo),
					 pDev->deviceNo, pDev->funcNo, OSS_PCI_DEVICE_ID, (int32*)&device);

    /* return error if access failed, no device present */
    if( device == 0xFFFF || device == 0x0000 ){
    	goto errout;
    }

	OSS_PciGetConfig(NULL, OSS_MERGE_BUS_DOMAIN(pDev->busNo, pDev->domainNo),
					 pDev->deviceNo, pDev->funcNo, OSS_PCI_VENDOR_ID, (int32*)&vendor);

	/* chameleon device && men/altera vendor ID? */
    if((device == CHAMELEON_PCI_DEV_ID) &&
	   ((vendor == CHAMELEON_PCI_VENID_ALTERA) ||
        (vendor == CHAMELEON_PCI_VENID_MEN)))
    {
        DBGWRT_1((DBH, "z077CheckChameleonDevice OK: dom/b/d/f %d/%d/%d/%d, vendor=%04x, device=%04x\n",
				  pDev->domainNo,	pDev->busNo, pDev->deviceNo, pDev->funcNo, vendor, device ) );
        return OK;
    }
errout:
	DBGWRT_1((DBH, "z077CheckChameleonDevice ERROR: dom/b/d/f %d/%d/%d/%d, vendor=%04x, device=%04x\n",
			  pDev->domainNo,	pDev->busNo, pDev->deviceNo, pDev->funcNo, vendor, device ) );
	return ERROR;
}

/**********************************************************************/
/** Routine to get the secondary bus number of a bridge device
 *
 *  This routine checks if a given device is a bridge device and then reads the secondary bus
 *  number.
 *
 *  \param pPCIDesc         pointer to the PCI descriptor (bus/device/function)
 *  \param pSecBus          pointer to the destination variable
 *
 *  \return STATUS          ERROR if not PCI bridge or read error, else OK
 */
LOCAL STATUS z077GetBusBehindBridge(Z077_PCI_DESC * pPCIDesc, int32 * pSecBus)
{
    int32 header = 0;

    if(pPCIDesc == NULL)
        return ERROR;

    if( OSS_PciGetConfig(NULL,
						 OSS_MERGE_BUS_DOMAIN( pPCIDesc->busNo, pPCIDesc->domainNo),
						 pPCIDesc->deviceNo, pPCIDesc->funcNo, OSS_PCI_HEADER_TYPE, &header) != ERROR)
    {
        if ((header & PCI_HEADER_TYPE_MASK) == PCI_HEADER_PCI_PCI)
        {
            /* PCI to PCI bridge header found - read out secondary bus number */
            if(	OSS_PciGetConfig(NULL,
								 OSS_MERGE_BUS_DOMAIN( pPCIDesc->busNo, pPCIDesc->domainNo),
								 pPCIDesc->deviceNo,
								 pPCIDesc->funcNo,
								 OSS_PCI_SECONDARY_BUS, pSecBus)!= ERROR)
            {
                return OK;
            }
        }
    }
    return ERROR;

}

/**********************************************************************/
/** Routine to Initialize a chameleon V2 device
 *
 *  This routine initializes a chameleon V2 device
 *
 *  \param      pPCIDesc -  pointer to PCI descriptor
 *              pChamHdl -  pointer to chameleon v2 handle
 *
 *  \return STATUS          ERROR if not PCI bridge or read error, else OK
 */
STATUS z077InitCham(
	Z077_PCI_DESC * pPCIDesc,
	CHAMELEONV2_HANDLE ** pChamHdl)
{

    if( pPCIDesc == NULL )
    {
        return ERROR;
    }

    if( pChamHdl == NULL )
    {
        return ERROR;
    }

    if(LocChamFctTable.InitPci( NULL, pPCIDesc->busNo, pPCIDesc->deviceNo, pPCIDesc->funcNo, pChamHdl) != CHAMELEON_OK )
    {
        return ERROR;
    }

    return OK;
}

/**********************************************************************/
/** Routine to find a specific 16Zxx FPGA core
 *
 *  This routine reads the chameleon table from a given device and looks for a
 *  16Zxx core.
 *
 *  \param pInstance - instance to find
 *         devID    -  DeviceID of core
 *         pChamHdl -  pointer to chameleon v2 handle
 *         pChamInfo   pointer to the destination Chameleon descriptor
 *
 *  \return STATUS          OK if we found the instance, else ERROR
 */
STATUS z077FindZxCore(
	INT8 *pInstance,
	int16 devID,
	CHAMELEONV2_HANDLE ** pChamHdl,
	CHAMELEONV2_UNIT * pChamInfo)
{
    CHAMELEONV2_FIND	chamFind;       /* Chameleon find */

    if( pInstance == NULL )
    {
        return ERROR;
    }

    if( pChamInfo == NULL )
    {
        return ERROR;
    }

    /* find core in Chameleon table */
    chamFind.variant  = -1; 
    chamFind.instance = *pInstance;
    chamFind.busId    = -1; 
	chamFind.group    = -1;
    chamFind.bootAddr = -1;

    chamFind.devId    = devID;

    if(LocChamFctTable.InstanceFind( *pChamHdl, 0, chamFind, pChamInfo, NULL, NULL) == CHAMELEONV2_UNIT_FOUND)
    {
        return OK;
    }
    return ERROR; /* dev. not found */
}

/**********************************************************************/
/** Routine to set the interrupt connect and enable functions
 *
 *  This routine sets the function pointers for interrupt connect and
 *  enable functions.
 *
 *  \param intConnectAddr   interrupt connect function pointer
 *  \param intEnableAddr    interrupt enable function pointer
 *
 *  \return OK
 */
STATUS z077SetIntFunctions(FUNCPTR intConnectAddr, FUNCPTR intEnableAddr){

	/* check which interrupt connect function shall be used */
    if( intConnectAddr == NULL )    {
#ifndef Z77_USE_VXBPCI_FUNCS
        z077Mgnt.fIntConnectP = (FUNCPTR)intConnect;
#else
        z077Mgnt.fIntConnectP = (FUNCPTR)vxbIntConnect;
#endif
    }
    else
    { /* take user interrupt connect function */
    	z077Mgnt.fIntConnectP = (FUNCPTR)intConnectAddr;
    }

	/* check which interrupt enable function shall be used */

    if( intEnableAddr == NULL ) {/* take VxWorks intEnable */
#ifndef Z77_USE_VXBPCI_FUNCS
        z077Mgnt.fIntEnableP = (FUNCPTR)intEnable;
#else
        z077Mgnt.fIntEnableP = (FUNCPTR)vxbIntEnable;
#endif
    }
    else{/* take user interrupt enable function */
        z077Mgnt.fIntEnableP = (FUNCPTR)intEnableAddr;
    }

    return OK;
}/* Z077SetIntFunctions */

/*****************************************************************************
 *
 * copied from netBufLib.c::netMblkToBufCopy for optimization:use always
 * bcopy
 *
 * RETURNS:
 * The length of data copied or zero.
 *
 * ERRNO:
 * none
 */
LOCAL int MyNetMblkToBufCopy
(
    M_BLK_ID        pMblk,  /* pointer to an mBlk                       */
    char *      pBuf,       /* pointer to the buffer to copy            */
    FUNCPTR     pCopyRtn    /* function pointer for copy routine        */
    )
{
    char *      pChar;
    int i = 0;

    if (pMblk == NULL || pBuf == NULL)
        return 0;

    pCopyRtn = (FUNCPTR)bcopy;  /* always default copy routine */
    pChar = pBuf;

    while (pMblk != NULL){
#if defined(MEN_IMX6)
    	cacheClear(DATA_CACHE, pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen);
#endif /* #ifdef MEN_IMX6 */
        (*pCopyRtn) ((char *)pMblk->mBlkHdr.mData, pBuf, pMblk->mBlkHdr.mLen);
#if defined(MEN_IMX6) || defined(MEN_MPC5121)
        cacheFlush(DATA_CACHE, pBuf, pMblk->mBlkHdr.mLen);
#endif /* #ifdef MEN_IMX6 */
        pBuf    += pMblk->mBlkHdr.mLen;
        pMblk   = pMblk->mBlkHdr.mNext;
        i++;
    }

    return ((int) pBuf - (int) pChar);  /* return length copied */
}


/**********************************************************************/
/** Routine to convert a string to hexadecimal values
 *
 *  This routine converts a byte coded string to its hexdecimal values.
 *
 *  \param pathStringP      device path string
 *  \param devicePathP      device path hexadecimal
 *
 *  \return no return value
 */
LOCAL STATUS z077PciParse( int8 *pathStringP, int8 *devicePathP )
{
    u_int8 i = 0;           /* loop index */
    u_int8 hlpIndex = 0;    /* index of devicePathP */

    for( i=0; i<strlen( (char*)pathStringP ); i++ ){
        if( pathStringP[i] == 'x' ){
            int8 tmpString[3];
            u_int32 tmp = 0;

            strncpy( (char*)&tmpString[0], (char*)&pathStringP[i+1], 2);
            tmpString[2] = '\0';

            if(!sscanf( (char*) tmpString, "%x", (unsigned int *)(void*)&tmp))
                return ERROR; /* bad element in path */

            devicePathP[hlpIndex] = (int8)(tmp&0xff);

            ++hlpIndex;
        }
    }
    return OK;
}/* LocBuildPciPath */

/**********************************************************************/
/** Routine to extract the END unit number from a given init string
 *
 *  This routine scans a given init string for a END unit number.
 *  The muxLib.o module automatically prepends the unit number to the user's
 *  initialization string from the BSP (configNet.h).
 *
 *  \param initString       string pointer
 *  \param pUnit            pointer to destination int
 *
 *  \return STATUS          OK, ERROR
 */
LOCAL STATUS z077InitParseUnit (
    char * initString,       /* information string */
    int * pUnit
    )
{
    if(sscanf(initString, "%u:", (unsigned int*) pUnit) == 1)
        return OK;
    else
        return ERROR;
}

/******************************************************************************
 *
 * z077InitParseRest - parse the init string
 *
 * Parse the input string. For Z077 there are just the Autonegotiation or
 * fixed Phy Link settings to be considered:
 *
 * The muxLib.o module automatically prepends the unit number to the user's
 * initialization string from the BSP (configNet.h).
 *
 * RETURNS: OK or ERROR for invalid arguments.
 */
LOCAL STATUS z077InitParseRest (
    Z077_END_DEVICE * pDrvCtrl, /* device pointer */
    char * initString           /* information string */
    )
{

    char *  tok;            /* an initString token */
    char *  holder = NULL;  /* points to initString fragment */

    pDrvCtrl->phyInfo->phyFlags         = 0;

	/* find unit */
    tok = strtok_r (initString, ":", &holder);
    if (tok == NULL)
        return ERROR;

	/* find mode */
    Z077_PHY_FLAGS_CLEAR(MII_PHY_AUTO);

    if (!(strncmp(holder,"10HD", 4))) {
        pDrvCtrl->phyInfo->phySpeed         = MII_10MBS;
        pDrvCtrl->phyInfo->phyDefMode          = MII_PHY_HD;
        Z077_PHY_FLAGS_CLEAR(MII_PHY_FD);
        Z077_PHY_FLAGS_SET( MII_PHY_10 | MII_PHY_HD );

    } else if (!(strncmp(holder,"10FD", 4))) {
        pDrvCtrl->phyInfo->phySpeed         = MII_10MBS;
        pDrvCtrl->phyInfo->phyDefMode          = MII_PHY_FD;
        Z077_PHY_FLAGS_SET( MII_PHY_10 | MII_PHY_FD );

    } else if (!(strncmp(holder,"100HD", 5))) {
        pDrvCtrl->phyInfo->phySpeed         = MII_100MBS;
        pDrvCtrl->phyInfo->phyDefMode          = MII_PHY_HD;
        Z077_PHY_FLAGS_CLEAR(MII_PHY_FD);
        Z077_PHY_FLAGS_SET( MII_PHY_100 | MII_PHY_HD );

    } else if (!(strncmp(holder,"100FD", 5))) {
        pDrvCtrl->phyInfo->phySpeed         = MII_100MBS;
        pDrvCtrl->phyInfo->phyDefMode          = MII_PHY_FD;
        Z077_PHY_FLAGS_SET( MII_PHY_100 | MII_PHY_FD );

    } else if (!(strncmp(holder,"AUTO", 4))) {
        Z077_PHY_FLAGS_SET(MII_PHY_AUTO);
        pDrvCtrl->phyInfo->phyDefMode          = MII_PHY_FD;
        pDrvCtrl->phyInfo->phySpeed         = MII_100MBS;
        Z077_PHY_FLAGS_SET(MII_PHY_AUTO);
        Z077_PHY_FLAGS_SET(MII_PHY_10 | MII_PHY_100 );
        Z077_PHY_FLAGS_SET(MII_PHY_HD | MII_PHY_FD );
    }

    /* parse pci path */
    DBGWRT_1( (DBH, UNIT_DRIVER_NAME "Unit %u set to %s\n", pDrvCtrl->unit, holder) );

    return OK;
}


/******************************************************************************
 * z077BdSetup - Setup Rx & TX BDs
 *
 * Behaviour for old OpenCore:
 * This routine sets up the Rx and Tx Descriptors between base+0x400 and
 * base+0x800 to a default state.
 * We definitely need linear buffers of up to MTU bytes so call netTupleGet
 * with noSmaller = TRUE and M_WAIT.
 *
 * For new Core, the BDs are located in Main Memory
 *
 * RETURNS: OK or ERROR
 */
LOCAL STATUS z077BdSetup(
    Z077_END_DEVICE *pDrvCtrl   /* device to be initialized */
    )
{

    UINT32 i=0, value=0;
    M_BLK_ID    pMblk   = NULL;

    /* clean out whole BD Area (Base + 0x400..0x800) */
    memset((char*)(Z077_BDBASE + Z077_BD_OFFS), 0x00, Z077_BD_SPACE );

    /*******************
     * Setup Tx BDs
     */
    for (i = 0; i < Z077_TBD_NUM; i++ ) {

        /* free a probably still used M_BLK, init this tBufList Element*/
        if (pDrvCtrl->tBufList[i] != NULL)
            netMblkClChainFree(pDrvCtrl->tBufList[i]);

        /* Get fix Tuple for this BD */
        if ((pDrvCtrl->tBufList[i] = \
             netTupleGet(pDrvCtrl->endObj.pNetPool,
                         Z077_MAX_CL_LEN,        /* size of buffer to get   */
                         M_WAIT,                 /* wait or don't wait      */
                         MT_DATA,                /* type of data            */
                         TRUE)) == NULL)    {    /* TRUE: no smaller buffer */
            DBGWRT_ERR( (DBH, "*** z077BdSetup: netTupleGet for Rx BD %d failed!\n") );
            return (ERROR);
        }

        /* terminate this M_BLK with NULL, its a single buffer, no chain*/
        pDrvCtrl->tBufList[i]->mBlkHdr.mNext = NULL;
        value = (UINT32)(pDrvCtrl->tBufList[i]->mBlkHdr.mData);

#if _WRS_VXWORKS_MAJOR == 7
        Z077_SET_TBD_ADDR( i, END_CACHE_VIRT_TO_PHYS(value));
#else
        Z077_SET_TBD_ADDR( i, value);
#endif

    }

    /**************************
     * Setup Receive BDs
     */
    for (i = 0; i < Z077_RBD_NUM; i++ ) {

        Z077_SET_RBD_FLAG( i, Z077_RBD_IRQ | Z077_RBD_EMP );

        pDrvCtrl->rxBd[i].Done = FALSE;

        /* free a probably still used M_BLK */
        if (pDrvCtrl->pMblkList[i] != NULL)
            netMblkClChainFree(pDrvCtrl->pMblkList[i]);

        /* Get fixed Tuple for this BD to receive data into*/
        if ((pDrvCtrl->pMblkList[i] = \
             netTupleGet(pDrvCtrl->endObj.pNetPool,
                         Z077_MAX_CL_LEN,        /* size of buffer to get   */
                         M_WAIT,                 /* wait till we get buffer */
                         MT_DATA,                /* type of data            */
                         TRUE)) == NULL)    {    /* TRUE: no smaller buffer */
            DBGWRT_ERR( (DBH, "*** z077BdSetup: netTupleGet for Rx BD %d failed!\n",i) );
            return (ERROR);
        }

        pMblk = pDrvCtrl->pMblkList[i];

        /* terminate this M_BLK with NULL, its a single buffer, no chain*/
        pMblk->mBlkHdr.mNext = NULL;

        /* associate the Tuple with this BD */
#if _WRS_VXWORKS_MAJOR == 7
        Z077_SET_RBD_ADDR( i,END_CACHE_VIRT_TO_PHYS((UINT32)(pMblk->mBlkHdr.mData)) );
#else
        Z077_SET_RBD_ADDR( i,(UINT32)(pMblk->mBlkHdr.mData) );
#endif

        /* and store the buffers address for passing to upper layers */
        pDrvCtrl->rxBd[i].BdAddr=(UINT32)(pMblk->mBlkHdr.mData);

        DBGWRT_7( (DBH, "netTuple RxBD%d: len 0x%04x adr 0x%08x\n",
				   i, pMblk->mBlkHdr.mLen, (UINT32)(pMblk->mBlkHdr.mData)) );

        /* cleanout the memory */
        memset((char*)(pDrvCtrl->rxBd[i].BdAddr), 0x0 , 0x700);
    }

    /* close the Rx/Tx Rings with Wrap bit in each last BD */
    Z077_SET_TBD_FLAG( Z077_TBD_NUM - 1 , Z077_TBD_WRAP );
    Z077_SET_RBD_FLAG( Z077_RBD_NUM - 1 , Z077_RBD_WRAP );

    return(OK);
}

/******************************************************************************
 * z077MemInit - initialize memory
 *
 * This routine initializes all the memory needed by the driver whose control
 * structure is passed in <pDrvCtrl>. Attention: must be executed before
 * Tx/Rx BD setup!
 *
 * RETURNS: OK or ERROR
 */
LOCAL STATUS z077MemInit(
	Z077_END_DEVICE * pDrvCtrl  /* device to be initialized */
    )
{
    UINT32  bdMemSize;   /* total BD area including Alignment   */
    UINT32  rbdMemSize;  /* Receive BD area size                */
    UINT32  tbdMemSize;  /* Transmit BD area size               */
    UINT16  clNum;       /* a buffer number holder              */

    pDrvCtrl->initType  	= END_NET_POOL_CREATE;  /* new API          				*/
    pDrvCtrl->pBufAlloc 	= NULL; 				/* driver must allocate clusters    */
    pDrvCtrl->bufSize   	= 0;    				/* none allocated                   */
    pDrvCtrl->pBdAlloc  	= NULL; 				/* driver must allocate descriptor ring */
	pDrvCtrl->pNetBufCfg 	= NULL;
	pDrvCtrl->tSendList 	= NULL;
    pDrvCtrl->pMblkList		= NULL;
    pDrvCtrl->tBufList 		= NULL;
    pDrvCtrl->bdSize    	= 0;
    pDrvCtrl->tbdNum    	= Z077_TBD_NUM;
    pDrvCtrl->rbdNum    	= Z077_RBD_NUM;
#if 0
    /* initialize the netPool */
    if ((pDrvCtrl->endObj.pNetPool = memalign(4, sizeof(NET_POOL))) == NULL) {
        return ERROR;
    }
#endif

	if ( !CACHE_DMA_IS_WRITE_COHERENT ()) {
		DBGWRT_7( (DBH, "Cache DMA not write Coherent!\n") );
		return ERROR;
	}
	rbdMemSize = Z077_BDSIZE * pDrvCtrl->rbdNum;
	tbdMemSize = Z077_BDSIZE * pDrvCtrl->tbdNum;
	bdMemSize  = ROUND_UP ((rbdMemSize + tbdMemSize),tbdMemSize);

	pDrvCtrl->pBdAlloc = memalign( bdMemSize, bdMemSize );
	if (pDrvCtrl->pBdAlloc == NULL) {
        goto LBL_ERR_RETURN;
	}

	pDrvCtrl->bdSize = bdMemSize;
	pDrvCtrl->flags |= Z077_MOTSEC_BD /* MOT_TSEC_OWN_BD_MEM */;
	
	DBGWRT_7( (DBH, "bdMemSize=0x%08x pBdAlloc: 0x%08x\n", (UINT32)pDrvCtrl->pBdAlloc, (UINT32)bdMemSize) );
	
    /* zero the shared memory */
    memset (pDrvCtrl->pBdAlloc, 0, (int) pDrvCtrl->bdSize);

    /* align the shared memory */
    pDrvCtrl->pBdBase = pDrvCtrl->pBdAlloc;

    DBGWRT_7( (DBH,
			   "\n\tBD:Alloc 0x%08x Base 0x%08x Total 0x%08x RxBD %d TxBD %d\n",
			   (int) pDrvCtrl->pBdAlloc, (int) pDrvCtrl->pBdBase,
			   pDrvCtrl->bdSize, pDrvCtrl->rbdNum, pDrvCtrl->tbdNum) );

    /*
     * number of clusters, including loaning buffers, a min number
     * of transmit clusters for copy-mode transmit, and one transmit
     * cluster for polling operation.
     */
    clNum = 1024;

	DBGWRT_7( (DBH, "pDrvCtrl->initType: END_NET_POOL_CREATE\n") );
	/*
	 * Allocate enough space to also hold 16 additional bytes for pDrvCtrl->pNetBufCfg->pName field
	 * ts: these are the steps described in vxWorks Drivers developer guide. (END_NAME_MAX+8) = 16
	 */

	if ((pDrvCtrl->pNetBufCfg = (NETBUF_CFG *)memalign(sizeof(long), (sizeof(NETBUF_CFG)+ 16))) == NULL) {
        goto LBL_ERR_RETURN;
	}

	pDrvCtrl->pNetBufCfg->pClDescTbl = NULL;

	bzero ((void*) pDrvCtrl->pNetBufCfg, (sizeof(NETBUF_CFG)+16));

	/* Initialize the pName field */
	pDrvCtrl->pNetBufCfg->pName = (char *)((int)pDrvCtrl->pNetBufCfg + sizeof(NETBUF_CFG));

	sprintf(pDrvCtrl->pNetBufCfg->pName, "%s%d%s", Z077_DRV_NAME, pDrvCtrl->unit, "Pool");

	/* Set the attributes to be Cached, Cache aligned, sharable, ISR safe */
	pDrvCtrl->pNetBufCfg->attributes    = ATTR_AC_SH_ISR;

	/* Set pDomain to kernel, use NULL. */
	pDrvCtrl->pNetBufCfg->pDomain       = NULL;

	/* Set ratio of mBlks to clusters */
	pDrvCtrl->pNetBufCfg->ctrlNumber    = clNum;

	/* Set memory partition of mBlks to kernel, use NULL */
	pDrvCtrl->pNetBufCfg->ctrlPartId    = NULL;

	/* Set extra memory size to zero for now */
	pDrvCtrl->pNetBufCfg->bMemExtraSize = 0;

	/* Set cluster's memory partition to kernel, use NULL */
	pDrvCtrl->pNetBufCfg->bMemPartId    = NULL;

	/* Allocate memory for network cluster descriptor */
	pDrvCtrl->pNetBufCfg->pClDescTbl    = (NETBUF_CL_DESC *)memalign(sizeof(long), sizeof(NETBUF_CL_DESC));
	if ( pDrvCtrl->pNetBufCfg->pClDescTbl == NULL ) {
        goto LBL_ERR_RETURN;
	}
		
	/* Initialize the Cluster Descriptor */
	pDrvCtrl->pNetBufCfg->pClDescTbl->clSize = Z077_MAX_CL_LEN;
	pDrvCtrl->pNetBufCfg->pClDescTbl->clNum  = clNum;
	pDrvCtrl->pNetBufCfg->clDescTblNumEnt    = 1;

	/* Call netPoolCreate() with the Link Pool Function Table */
	pDrvCtrl->endObj.pNetPool = netPoolCreate((NETBUF_CFG *)pDrvCtrl->pNetBufCfg, _pLinkPoolFuncTbl);
	if ( pDrvCtrl->endObj.pNetPool == NULL ) {
        goto LBL_ERR_RETURN;
	}
	DBGWRT_7( (DBH, "call to netPoolCreate = OK\n") );

    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool, Z077_MAX_CL_LEN,FALSE))==NULL ) {
        goto LBL_ERR_RETURN;
	}

    /*
     * allocate receive buffer list
     */
    pDrvCtrl->pMblkList=(M_BLK_ID *)memalign(8, (pDrvCtrl->rbdNum * sizeof(M_BLK_ID)));
    if (pDrvCtrl->pMblkList == NULL){
        DBGWRT_ERR( (DBH, "*** calloc of pDrvCtrl->tMblkList failed!\n") );
        goto LBL_ERR_RETURN;
    }
    memset (pDrvCtrl->pMblkList, 0, (pDrvCtrl->rbdNum * sizeof(M_BLK_ID)));

    /*
     * allocate the M_BLK buffer list, <tbdNum> Elements of sizeof(M_BLK *)
     */
    pDrvCtrl->tBufList =(M_BLK_ID *)memalign(8, (pDrvCtrl->tbdNum * sizeof(M_BLK_ID)));
    if (pDrvCtrl->tBufList == NULL){
        DBGWRT_ERR( (DBH, "*** calloc of pDrvCtrl->tBufList failed!\n") );
        goto LBL_ERR_RETURN;
    }
    memset (pDrvCtrl->tBufList, 0, (pDrvCtrl->tbdNum * sizeof(M_BLK *)));

    /* allocate the M_BLK send buffer list for freeing them after sending */
    pDrvCtrl->tSendList =(M_BLK_ID *)memalign(8, (pDrvCtrl->tbdNum * sizeof(M_BLK_ID)));
    if (pDrvCtrl->tSendList == NULL){
        DBGWRT_ERR( (DBH, "*** calloc of pDrvCtrl->tSendList failed!\n") );
        goto LBL_ERR_RETURN;
    }
    memset (pDrvCtrl->tSendList, 0, (pDrvCtrl->tbdNum * sizeof(M_BLK *)));

    if (pDrvCtrl->maxRxFrames == 0)
        pDrvCtrl->maxRxFrames = (pDrvCtrl->rbdNum * 3) >> 2;

    return OK;

LBL_ERR_RETURN:

	/* check each dynamically allocated member and free it if necessary */
    if (pDrvCtrl->pBdAlloc != NULL )
		free ( pDrvCtrl->pBdAlloc );

	if ( pDrvCtrl->pNetBufCfg != NULL ) {
		if ( pDrvCtrl->pNetBufCfg->pClDescTbl != NULL )
			free ( pDrvCtrl->pNetBufCfg->pClDescTbl );
	}

    if ( pDrvCtrl->pMblkList != NULL )
		free ( pDrvCtrl->pMblkList );

    if ( pDrvCtrl->tBufList != NULL )
		free ( pDrvCtrl->tBufList );

	if ( pDrvCtrl->tSendList != NULL ) 
		free ( pDrvCtrl->tSendList );

    return ERROR;
}

#ifdef INCLUDE_SHOW_ROUTINES
/******************************************************************************
 *
 * z077dumpTbd -
 *
 * RETURNS: N/A
 */
LOCAL void z077dumpTbd( Z077_END_DEVICE *pDrvCtrl )
{
    UINT32 adr  = 0;
    UINT32 i    = 0;

    for (i = 0; i < Z077_TBD_NUM; i++ ) {
        adr = Z077_BD_OFFS + (i * Z077_BDSIZE);

        printf("%02x STAT: 0x%04x LEN: 0x%04x  ADR 0x%08x\n", i, 
               Z077_GET_TBD_FLAG(i, 0xffff),
               Z077_GET_TBD_LEN(i),
#if _WRS_VXWORKS_MAJOR == 7
               mtod(pDrvCtrl->tBufList[i], UINT32));
#else
               Z077_GET_TBD_ADDR(i) );
#endif
    }
}

/******************************************************************************
 *
 * z077dumpRbd -
 *
 * RETURNS: N/A
 */
LOCAL void z077dumpRbd( Z077_END_DEVICE *pDrvCtrl )
{
    UINT32 adr  = 0;
    UINT32 i    = 0;

    for (i = 0; i < Z077_RBD_NUM ; i++ ) {
        adr = Z077_BD_OFFS + (i + Z077_TBD_NUM ) * Z077_BDSIZE;
        printf("%02x STAT: 0x%04x LEN: 0x%04x  ADR 0x%08x\n", i,
               Z077_GET_RBD_FLAG(i, 0xffff),
               Z077_GET_RBD_LEN(i),
#if _WRS_VXWORKS_MAJOR == 7
               mtod(pDrvCtrl->pMblkList[i], UINT32));
#else
               Z077_GET_RBD_ADDR(i) );
#endif
    }
}
#endif

/******************************************************************************
 *
 * z077Load - initialize the driver and device
 *
 * This routine initializes the driver and the device to the operational state.
 * All of the device specific parameters are passed in the initString.
 *
 * The string contains the target specific parameters like this:
 * This routine can be called in two modes.  If it is called with an empty but
 * allocated string, it places the name of this device into the <initString>
 * and returns 0.
 *
 * If the string is allocated and not empty, the routine attempts to load
 * the driver using the values specified in the string.
 *
 * RETURNS: An END object pointer, or NULL on error, or 0 and the name of the
 * device if the <initString> was empty.
 */

END_OBJ* z077Load ( char* initString, void* datP )
{

    UINT32 mac0 = 0, mac1 = 0, moder = 0, ENDunitNo = 0;
    Z077_END_DEVICE * pDrvCtrl = NULL;
	UINT8 *pMac=NULL;

    DBGWRT_2( (DBH, "Loading z087\n") );

    if (initString == NULL) {
        DBGWRT_ERR( (DBH, "*** z077Load: initStr=NULL\n") );
        return NULL;
    }

    if (initString[0] == EOS) {
        bcopy ((char *)Z077_DRV_NAME, initString, DRV_NAME_LEN);
        return NULL;
    }

    if(z077InitParseUnit(initString, (int*)&ENDunitNo) == ERROR)
    {
        DBGWRT_ERR( (DBH, "*** z077Load: Error parsing unit!\n") );
        goto LBL_ERROR_EXIT;
    }

	/* KlocWork #9898 - limit unit nr. range to [0..7] */
	ENDunitNo &= 0x7;

    if(ENDunitNo == 0)
    { /* introduce driver just once ... */
#if _WRS_VXWORKS_MAJOR == 7
        printf( UNIT_DRIVER_NAME "END Driver for VxWorks %s built %s %s.\n", RELEASE_VERSION, __DATE__, __TIME__ );
#else
        printf( UNIT_DRIVER_NAME "END Driver for VxWorks %s built %s %s.\n", RUNTIME_VERSION, __DATE__, __TIME__ );
#endif
    }

    /* KlocWork finding */
    if ( ENDunitNo >= Z077_MAX_UNITS ) {
	    DBGWRT_ERR( (DBH, "*** z077Load: invalid value for ENDunitNo (%d)\n", (int)ENDunitNo ) );
	    goto LBL_ERROR_EXIT;
    }

    /* now we know the END unit no., we can access the initialized structure from z077Init() */
    if(z077Mgnt.endUnit[ENDunitNo] == NULL)
    {
        DBGWRT_ERR( (DBH, "*** z077Load: can't load " Z077_DRV_NAME "%u: not initialized/found\n", ENDunitNo) );
        goto LBL_ERROR_EXIT;
    } else
        pDrvCtrl = z077Mgnt.endUnit[ENDunitNo];

    /* parse init string for  Phy settings, fill the phy structure */
    if (z077InitParseRest( pDrvCtrl, initString ) == ERROR)
        goto LBL_ERROR_EXIT;

    /* allocate Space for BDs which reside in RAM there */   
#if defined(MEN_IMX6) || defined(MEN_MPC5121)
       pDrvCtrl->bdBase = (UINT32)cacheDmaMalloc(Z077_BD_SPACE);
#if _WRS_VXWORKS_MAJOR == 7
       pDrvCtrl->pCacheFuncs = &cacheDmaFuncs;
#endif
#else
	pDrvCtrl->bdBase = (UINT32)memalign( Z077_BDALIGN, Z077_BD_SPACE );
#endif /* #ifdef MEN_IMX6 || MEN_MPC5121 */

	if (!pDrvCtrl->bdBase) {
		DBGWRT_ERR( (DBH, "*** z077Load: memalign() bdBase failed!\n") );
		goto LBL_ERROR_EXIT;
	}

    printf( UNIT_DRIVER_NAME "END Driver: ETH core 16Z087 Base 0x%08x Irq %u Rev %u\n", pDrvCtrl->baseAddr, pDrvCtrl->ivec, pDrvCtrl->chamUnitRevision );

    /* Try to get ethernet address. from BSP */
	if( pDrvCtrl->getMACFunc == NULL )
    {
        DBGWRT_ERR( (DBH, "*** z077Load: No MAC load function provided, check confSetTbl[]!\n" ) );
        goto LBL_ERROR_EXIT;
    }
	if ( pDrvCtrl->getMACFunc( pDrvCtrl->unit, pDrvCtrl->enetAddr) == ERROR )
    {
        DBGWRT_ERR( (DBH, "*** z077Load: Failed to load MAC address for " UNIT_DRIVER_NAME "%u!\n", pDrvCtrl->unit) );
        goto LBL_ERROR_EXIT;
    }

    /* check for invalid MACs 00:00:00:00:00:00 / ff:ff:ff:ff:ff:ff */
	pMac = pDrvCtrl->enetAddr;
	if( ((pMac[0] == 0) && (pMac[1] == 0) && (pMac[2] == 0) && (pMac[3] == 0) && (pMac[4] == 0) && (pMac[5] == 0)) || \
		((pMac[0] == 0xff) && (pMac[1] == 0xff) && (pMac[2] == 0xff) && (pMac[3] == 0xff) && (pMac[4] == 0xff) && (pMac[5] == 0xff)) )
    {
        DBGWRT_ERR( (DBH, "*** z077Load: No valid MAC address for " UNIT_DRIVER_NAME "%u!\n", pDrvCtrl->unit) );
        goto LBL_ERROR_EXIT;
    }

    DBGWRT_1( (DBH, "z077Load: MAC addr. %02x:%02x:%02x:%02x:%02x:%02x\n", pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]) );
			  /* -------------- Z077_BASE is valid NOW ---------------- */
#if _WRS_VXWORKS_MAJOR == 7
        Z77WRITE_D32( Z077_BASE, Z077_REG_BDSTART, END_CACHE_VIRT_TO_PHYS((UINT32)pDrvCtrl->bdBase));
#else
	Z77WRITE_D32( Z077_BASE, Z077_REG_BDSTART, (UINT32)pDrvCtrl->bdBase);
#endif

	/* reset Register values */
	z077Reset(pDrvCtrl);
	
	/* setup MAC Address in MAC_ADDR0, MAC_ADDR1 */
	mac0 = pDrvCtrl->enetAddr[2] << 24 | pDrvCtrl->enetAddr[3] << 16 | \
		pDrvCtrl->enetAddr[4] << 8  | pDrvCtrl->enetAddr[5];
	Z77WRITE_D32( Z077_BASE, Z077_REG_MAC_ADDR0, mac0 );

	mac1 = pDrvCtrl->enetAddr[0] << 8 | pDrvCtrl->enetAddr[1];
	Z77WRITE_D32( Z077_BASE, Z077_REG_MAC_ADDR1, mac1 );

	/* set Min/Max. packet length */
	Z77WRITE_D32( Z077_BASE, Z077_REG_PACKLEN,
				  ((Z077_MIN_PACKLEN)<<16) | (Z077_MAX_PACKLEN) );

	/* setup Rx/Tx buffer defaults, packet lengths */
	pDrvCtrl->tbdNum = Z077_TBD_NUM;
	pDrvCtrl->rbdNum = Z077_RBD_NUM;

	/* start with RX0 as first Rx BD empty bit register to check */
	pDrvCtrl->lruRxBD 	= 	Z077_REG_RXEMPTY0;
	pDrvCtrl->tbdOff  	=   Z077_TBD_NUM;
	pDrvCtrl->rbdOff    =   0;
	pDrvCtrl->bdOff     =   0;

	/*
	 * set MODER Register to meaningful values. Attention: Half/Full Duplex
	 * Mode is set after Autonegotiation.
	 */
	moder = OETH_MODER_IFG  | OETH_MODER_EXDFREN | OETH_MODER_CRCEN | OETH_MODER_PAD  | OETH_MODER_BRO | OETH_MODER_RECSMALL;

	Z77WRITE_D32( Z077_BASE, Z077_REG_MODER, moder);

	/* initialize the END and MIB2 parts of the structure */

	/*
	 * The M2 element must come from m2Lib.h
	 * This template is set up for a DIX type ethernet device.
	 */
	if (END_OBJ_INIT(&pDrvCtrl->endObj, NULL, Z077_DRV_NAME, pDrvCtrl->unit, &z077FuncTable, "END Z077 Driver.") == ERROR || \
		END_MIB_INIT(&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd, &pDrvCtrl->enetAddr[0], 6, Z077END_BUFSIZ, END_SPEED) == ERROR) 
	{
		DBGWRT_ERR( (DBH, "*** END_OBJ_INIT / END_MIB_INIT failed !\n") );
		goto LBL_ERROR_EXIT;
	}

	/* Perform memory allocation/distribution */
	if (z077MemInit(pDrvCtrl) == ERROR) {
		DBGWRT_ERR( (DBH, "*** z077MemInit failed!\n") );
		goto LBL_ERROR_EXIT;
	}

	/* initialize Tx/Rx Buffer Descriptors and their M_BLK Tuples */
	if ( z077BdSetup( pDrvCtrl ) == ERROR ) {
		DBGWRT_ERR( (DBH, "*** Error during z077BdSetup!\n") );
		goto LBL_ERROR_EXIT;
	}

	DBGWRT_1( (DBH, "Done loading Z077...\n") );
	return (&pDrvCtrl->endObj);

LBL_ERROR_EXIT:
	DBGWRT_ERR( (DBH, "*** Error on z077Load! exiting.\n") );
	/* Free allocated spaces */

	if(pDrvCtrl != NULL)
	{
		if (pDrvCtrl->phyInfo)
			free ((char *)pDrvCtrl->phyInfo);

		/* free malloc'ed/memaligned members of pDrvCtrl first */
		if (pDrvCtrl->bdBase) {
#ifdef MEN_IMX6
			cacheDmaFree((void*)pDrvCtrl->bdBase);
#else
			free ((char *)pDrvCtrl->bdBase);
#endif
		}

		if ( pDrvCtrl->pMblkList) {
			free( pDrvCtrl->pMblkList );
		}

		if ( pDrvCtrl->tBufList ) {
			free( pDrvCtrl->tBufList );
		}

		if ( pDrvCtrl->pNetBufCfg->pClDescTbl ) {
			free ( pDrvCtrl->pNetBufCfg->pClDescTbl );
		}

		if ( pDrvCtrl->pNetBufCfg ) {
			free ( pDrvCtrl->pNetBufCfg );
		}

		if ( pDrvCtrl->tSendList ) {
			free( pDrvCtrl->tSendList );
		}
		
		free ((char *)pDrvCtrl);

		pDrvCtrl = NULL;
	}
	return NULL;
}

/******************************************************************************
 *
 * z077Start - start the device
 *
 * This function calls BSP functions to connect interrupts and start the
 * device running in interrupt mode.
 *
 * RETURNS: OK or ERROR
 *
 */
LOCAL STATUS z077Start
(
	Z077_END_DEVICE * pDrvCtrl  /* Z077 device struct */
	)
{
	UINT32 moder = 0;
#ifdef Z77_USE_VXBPCI_FUNCS
#if _WRS_VXWORKS_MAJOR == 7
    VXB_RESOURCE * pIntRes = NULL; 
    VXB_DEV_ID pciId = sysGetFpgaPciCtrlID();
#else
    VXB_DEVICE_ID pciId = sysGetPciCtrlID(pDrvCtrl->pciDesc.domainNo);
#endif
#endif

	DBGWRT_1( (DBH, "--> z077Start\n") );

	pDrvCtrl->nCurrRbd = 0;
	pDrvCtrl->nCurrTbd = 0;

	if (z077PhyInit( pDrvCtrl ) == ERROR){
		DBGWRT_ERR( (DBH, "*** end Phy Initialization failed!\n") );
	}

	/* Set Full/Half Duplex Bit According to results of Phy Init */
	moder = Z77READ_D32(Z077_BASE, Z077_REG_MODER);

	if (pDrvCtrl->phyInfo->phyDefMode == MII_PHY_FD)
		moder |= OETH_MODER_FULLD;
	else
		moder &= ~OETH_MODER_FULLD;

	Z77WRITE_D32( Z077_BASE, Z077_REG_MODER, moder);

	/* finally connect and enable our IRQ handler */
	printf("z077Start: connect IRQ vector %u\n", pDrvCtrl->ivec);

#ifndef Z77_USE_VXBPCI_FUNCS
	z077Mgnt.fIntConnectP((VOIDFUNCPTR*) INUM_TO_IVEC(pDrvCtrl->ivec),
						  (VOIDFUNCPTR)z077Int,
						  (int) pDrvCtrl);
	z077Mgnt.fIntEnableP(pDrvCtrl->ivec);
#else
#if _WRS_VXWORKS_MAJOR == 7
    pIntRes = vxbResourceAlloc (pciId, VXB_RES_IRQ, 0);
    if (pIntRes == NULL)
    {
    	kprintf("pIntRes is NULL!!!!\n");
    	return ERROR;
    }
    z077Mgnt.fIntConnectP(pciId, pIntRes, (VOIDFUNCPTR)z077Int, (Z077_END_DEVICE *)pDrvCtrl);
    z077Mgnt.fIntEnableP(pciId, pIntRes);
    
#else

	z077Mgnt.fIntConnectP(pciId,
						  (VOIDFUNCPTR*)pDrvCtrl->ivec,
						  (VOIDFUNCPTR)z077Int,
						  (int) pDrvCtrl);
	z077Mgnt.fIntEnableP( pciId,
						  INUM_TO_IVEC(pDrvCtrl->ivec),
						  (VOIDFUNCPTR)z077Int,
						  (void*)pDrvCtrl);
#endif
#endif

	/*
	 *  enable the IRQs in the ETH MAC which we currently use:
	 *  Rx[-ERR], Tx[-ERR], Busy ( no more BDs available )
	 */
	Z77WRITE_D32(Z077_BASE, Z077_REG_INT_MASK,
				 OETH_INT_TXE |OETH_INT_RXF |OETH_INT_RXE | OETH_INT_BUSY );

	/* clear any possible pending IRQs ( clear-on-write) */
	Z77WRITE_D32( Z077_BASE, Z077_REG_INT_SRC, 0x7f );

	/* try to enable new RX IRQ behaviour and test the bit */
	Z77WRITE_D32( Z077_BASE, Z087_REG_CORE_REVISION, Z087_REG_CORE_REVISION_NEW_RX_IRQ_ENABLE );
	{
		UINT32 val;
		val = Z77READ_D32( Z077_BASE, Z087_REG_CORE_REVISION );
		if( val & Z087_REG_CORE_REVISION_NEW_RX_IRQ_ENABLE )
			pDrvCtrl->newRxIrqBehaviour = 1;
		else
			pDrvCtrl->newRxIrqBehaviour = 0;
	}

	/* set the flags to indicate readiness and
	 * finally enable Rx/Tx in the MAC. Lets get ready to rumble...
	 */
	Z077_SET_MODE_FLAG(OETH_MODER_RXEN | OETH_MODER_TXEN );

	pDrvCtrl->txBlocked  = FALSE;
	END_OBJ_READY( &pDrvCtrl->endObj,
				   IFF_NOTRAILERS | IFF_BROADCAST | IFF_MULTICAST );
	END_FLAGS_SET (&pDrvCtrl->endObj, IFF_UP | IFF_RUNNING);

	DBGWRT_1( (DBH, "<-- z077Start\n") );

	return (OK);
}

/******************************************************************************
 *
 * z077Send - the driver send routine
 *
 * This routine takes a M_BLK_ID sends off the data in the M_BLK_ID.
 * The buffer must already have the addressing information properly installed
 * in it.  This is done by a higher layer.
 *
 * RETURNS: OK, ERROR, or END_ERR_BLOCK.
 */
LOCAL STATUS z077Send
(
	Z077_END_DEVICE * pDrvCtrl, /* device ptr                           */
	M_BLK_ID     pMblk          /* data to send, [M_BLK_ID]= *M_BLK     */
	)
{
	UINT32  idxTx = 0, len = 0, adr = 0, txbEmpty = 0, regval = 0;

#ifdef DEBUG_MBLK
	UINT32      i=0;
	M_BLK_ID    pMblkHead = NULL;
	M_BLK_ID    pMblkTst = NULL;
#endif

	END_TX_SEM_TAKE ( &pDrvCtrl->endObj, WAIT_FOREVER );

	/* mblk sanity check */
	if (pMblk == NULL) {
		DBGWRT_ERR( (DBH, "*** z077Send: Zero Mblk!\n") );
		END_TX_SEM_GIVE(&pDrvCtrl->endObj);
		errno = EINVAL;
		return (ERROR);
	}

	/* return if in poll mode */
	if (pDrvCtrl->flags & Z077_POLLING) {
		DBGWRT_ERR( (DBH, "*** z077Send: Poll Mode set!\n") );
		netMblkClChainFree (pMblk);
		errno = EINVAL;
		return (ERROR);
	}

	/* place Tx request in the recent Tx BD */
	idxTx   = pDrvCtrl->nCurrTbd;

	/* Check if the Tx BD is empty. If not, a stall condition occurs. */
	if (idxTx < 32) {
		regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY0);
		txbEmpty= regval & (1<<idxTx);		
	}
	else
	{
		regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY1);
		txbEmpty=regval & (1 << ( idxTx-32 ));		
	}
    
	if (!txbEmpty) {
		/* Stall Condition */
		pDrvCtrl->txBlocked = TRUE;
		END_TX_SEM_GIVE( &pDrvCtrl->endObj );
		return (END_ERR_BLOCK);
	}

	/* get the .mData address that was stored in Tx BD at BdSetup */
#if _WRS_VXWORKS_MAJOR == 7
    adr = mtod(pDrvCtrl->tBufList[idxTx],UINT32);
#else
	adr = Z077_GET_TBD_ADDR(idxTx);
#endif

#ifdef DEBUG_MBLK
	/* dump whats in the chain... */
	for (i=0, pMblkHead= pMblk; pMblk != NULL;i++, pMblk=pMblk->mBlkHdr.mNext){
		if (pMblk->mBlkHdr.mLen != 0) {
			printf("%d 0x%04x \n", i, pMblk->mBlkHdr.mLen);
		}
	}
	/* restore pMblk */
	pMblk = pMblkHead;
#endif

	/* copy scattered data to the continous single buffer of this Tx BD  */
	len = MyNetMblkToBufCopy(pMblk, (char*)adr, NULL);

	/* put this MBLK back to the pool after data is copied */
	netMblkClChainFree (pMblk);

	/* set accumulated len to the Tx BD which is kicked off now*/
	Z077_SET_TBD_LEN( idxTx ,len );

	DBGWRT_4( (DBH, "z077Send %d len 0x%04x adr %08x\n", idxTx, len, adr) );

	/* finally kick off transmission */
	z077kickTransmission( pDrvCtrl, idxTx);

	/* Bump the statistics counters. */
	pDrvCtrl->nrTxbIrq++;
	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

	/* go to next Tx BD */
	pDrvCtrl->nCurrTbd = (++pDrvCtrl->nCurrTbd % Z077_TBD_NUM);

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	return (OK);
}

/******************************************************************************
 *
 * z077Ioctl - the driver I/O control routine
 *
 * Process an ioctl request.
 *
 * RETURNS: A command specific response, usually OK or ERROR.
 */

LOCAL int z077Ioctl (
    Z077_END_DEVICE * pDrvCtrl, /* device receiving command */
    int cmd,            /* ioctl command code */
    caddr_t data        /* command argument */
    )
{
    int error = 0;
    long value;

    switch ((unsigned)cmd) {
    case EIOCSADDR:     /* set MAC address      */
        if (data == NULL)
            {
            error = EINVAL;
            }
        else
            {
            bcopy ((char *)data, (char *)END_HADDR(&pDrvCtrl->endObj),
                   END_HADDR_LEN(&pDrvCtrl->endObj));
            }
        break;

    case EIOCGADDR:     /* get MAC address      */
        if (data == NULL)
            {
            error = EINVAL;
            }
        else
            {
            bcopy ((char *)END_HADDR(&pDrvCtrl->endObj), (char *)data,
                   END_HADDR_LEN(&pDrvCtrl->endObj));
            }
        break;

    case EIOCSFLAGS:    /* set (or clear) flags */
        value = (long)data;
        if (value < 0){
            value = -value;
            value--;
            END_FLAGS_CLR (&pDrvCtrl->endObj, value);
        } else {
            END_FLAGS_SET (&pDrvCtrl->endObj, value);
        }
        z077Config (pDrvCtrl);
        break;

    case EIOCGFLAGS:    /* get flags */
        *(int *)data = END_FLAGS_GET(&pDrvCtrl->endObj);
        break;

    case EIOCPOLLSTART: /* Begin polled operation */
        z077PollStart(pDrvCtrl);
        break;

    case EIOCPOLLSTOP:  /* End polled operation */
        z077PollStop(pDrvCtrl);
        break;

    case EIOCGMIB2233:
    case EIOCGMIB2:     /* return MIB information */
        if (data == NULL)
            {
            error = EINVAL;
            }
        else
            {
            error = endM2Ioctl(&pDrvCtrl->endObj, cmd, data);
            }
        break;

    case EIOCGFBUF:     /* return minimum First Buffer for chaining */
        if (data == NULL)
            {
            error = EINVAL;
            }
        else
            {
            *(int *)data = Z077END_BUFSIZ;
            }
        break;

    case EIOCGHDRLEN:
        if (data == NULL)
            {
            error = EINVAL;
            }
        else
            {
            *(int *)data = EH_SIZE;
            }
        break;

    default:        /* unknown request */
        error = EINVAL;
    }

    return (error);
}

/******************************************************************************
 *
 * z077PollRcv - routine to receive a packet in polled mode.
 *
 * Polled mode operation takes place without any kernel or other OS
 * services available.  Use extreme care to insure that this code does not
 * call any kernel services.  Polled mode is only for WDB system mode use.
 * Kernel services, semaphores, tasks, etc, are not available during WDB
 * system mode.
 *
 * The WDB agent polls the device constantly looking for new data.  Typically
 * the device has a ring of RFDs to receive incoming packets.  This routine
 * examines the ring for any new data and copies it to the provided mblk.
 * The concern here is to keep the device supplied with empty buffers at all
 * time.
 *
 * RETURNS: OK upon success.  EAGAIN is returned when no packet is available.
 * A return of ERROR indicates a hardware fault or no support for polled mode
 * at all.
 */

LOCAL STATUS z077PollRcv (
	Z077_END_DEVICE * pDrvCtrl, /* device to be polled */
	M_BLK_ID      pMblk     /* ptr to buffer */
	)
{

	unsigned int emptyBitPos 	= 0;
	unsigned int rx0 = 0;
	unsigned int rx1 = 0;
	STATUS retVal = EAGAIN;

	if (!(pMblk->mBlkHdr.mFlags & M_EXT))  {
		return (retVal);
	}

	/* TODO - Invalidate any inbound RFD DMA buffers */
	/* TODO - clear any status bits that may be set. */
	/* TODO - Check packet and device for errors */

	/*
	 * determine which BD we pass to Stack next. we calculate with
	 * inverted Values (bit=1 =>Data available instead 0, easier for below
	 * calculations
	 */
	rx0 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY0);/* RxBDs[31:0]*/
	rx1 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY1);/* RxBDs[63:32]*/
	DBGWRT_3(( DBH, "DbgCount %d RXjobs %d E0 0x%08x E1 0x%08x\n", z077_DbgRxCount, pDrvCtrl->RunningRxJobs, rx0, rx1 ));

	/* pass packets in order they arrived & care of BD63->BD0 Wrap */
	if ( !rx0  && !rx1 )
	{ /* If no packet is available return immediately */
		return (retVal);
	}
	else if ( rx0  && !rx1 )
	{
		CHECK_RX0_EMPTY_POLL();
	}
	else if ( rx1 && !rx0 )
	{
		CHECK_RX1_EMPTY_POLL();
	}
	else if ( rx1 && rx0 )
	{
		if( pDrvCtrl->lastRxPosition < 31 || pDrvCtrl->lastRxPosition == 63 )
		{
			CHECK_RX0_EMPTY_POLL();
		}
		else
		{
			CHECK_RX1_EMPTY_POLL();
		}
	}

	/*
	 * TODO - Done with this packet, clean up device. If needed
	 * setup a new RFD/buffer for incoming packets
	 */
	return (retVal);
}

/******************************************************************************
 *
 * z077PollSend - routine to send a packet in polled mode.
 *
 * RETURNS: OK upon success.  EAGAIN if device is busy or no resources.
 * A return of ERROR indicates a hardware fault or no support for polled mode
 * at all.
 */

LOCAL STATUS z077PollSend (
	Z077_END_DEVICE*    pDrvCtrl,   /* device to be polled */
	M_BLK_ID    pMblk   /* packet to send */
	)
{
	int         i         = 0;
	UINT32      idxTx = 0,
		len = 0,
		adr = 0,
		txbEmpty = 0,
		regval = 0;

	/* mblk sanity check */
	if (pMblk == NULL) {
		DBGWRT_ERR( (DBH, "*** z077PollSend: Zero Mblk!\n") );
		errno = EINVAL;
		return (ERROR);
	}

	/* place Tx request in the recent Tx BD */
	idxTx   = pDrvCtrl->nCurrTbd;

	/* Check if the Tx BD is empty. If not, a stall condition occurs. */
	if (idxTx < 32) {
		regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY0);
		txbEmpty= regval & (1<<idxTx);
	}
	else
	{
		regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY1);
		txbEmpty=regval & (1 << ( idxTx-32 ));
	}

	if (!txbEmpty) {
		/* Stall Condition */
		pDrvCtrl->txBlocked = TRUE;
		return (END_ERR_BLOCK);
	}

	/* get the .mData address that was stored in Tx BD at BdSetup */
#if _WRS_VXWORKS_MAJOR == 7
    adr = mtod(pDrvCtrl->tBufList[idxTx], UINT32);
#else
	adr = Z077_GET_TBD_ADDR(idxTx);
#endif

	/* copy scattered data to the continous single buffer of this Tx BD  */
	len = MyNetMblkToBufCopy(pMblk, (char*)adr, NULL);

	/* set accumulated len to the Tx BD which is kicked off now*/
	Z077_SET_TBD_LEN( idxTx ,len );

	DBGWRT_4( (DBH, "z077PollSend %d len 0x%04x adr %08x\n", idxTx, len, adr) );

	/* finally kick off transmission */
	z077kickTransmission( pDrvCtrl, idxTx);

	/* Bump the statistics counters. */
	pDrvCtrl->nrTxbIrq++;
	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

	/* go to next Tx BD */
	pDrvCtrl->nCurrTbd = (++pDrvCtrl->nCurrTbd % Z077_TBD_NUM);

	/* wait for completion. */
	for (i = 0; i < Z077_TIMEOUT; i++)
	{
		if (idxTx < 32) {
			regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY0);
			txbEmpty= regval & (1<<idxTx);
		}
		else
		{
			regval = Z77READ_D32(Z077_BASE, Z077_REG_TXEMPTY1);
			txbEmpty=regval & (1 << ( idxTx-32 ));
		}

		if (txbEmpty) {
			break;
		}
	}	

	if (i == Z077_TIMEOUT) {
		return (ERROR);
	}

	return (OK);
}

/*****************************************************************************
 *
 * z077MCastAdd - add a multicast address for the device
 *
 * This routine adds a multicast address to whatever the driver
 * is already listening for.  It then resets the address filter.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077MCastAdd (
	Z077_END_DEVICE *pDrvCtrl,      /* device pointer */
	char* pAddress  /* new address to add */
	)
{
	int error;

	if ((error = etherMultiAdd (&pDrvCtrl->endObj.multiList,
								pAddress)) == ENETRESET)
		z077Config (pDrvCtrl);

	return (OK);
}

/*****************************************************************************
 *
 * z077MCastDel - delete a multicast address for the device
 *
 * This routine removes a multicast address from whatever the driver
 * is listening for.  It then resets the address filter.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077MCastDel (
	Z077_END_DEVICE *pDrvCtrl,      /* device pointer */
	char* pAddress      /* address to be deleted */
	)
{
	int error;

	if ((error = etherMultiDel (&pDrvCtrl->endObj.multiList,
								(char *)pAddress)) == ENETRESET)
		z077Config (pDrvCtrl);

	return (OK);
}

/*****************************************************************************
 *
 * z077MCastGet - get the multicast address list for the device
 *
 * This routine gets the multicast list of whatever the driver
 * is already listening for.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077MCastGet (
	Z077_END_DEVICE *pDrvCtrl,          /* device pointer */
	MULTI_TABLE* pTable     /* address table to be filled in */
	)
{
	return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
}

/******************************************************************************
 *
 * z077Stop - stop the device
 *
 * This function calls BSP functions to disconnect interrupts and stop
 * the device from operating in interrupt mode.
 *
 * RETURNS: OK or ERROR.
 */
LOCAL STATUS z077Stop (
	Z077_END_DEVICE *pDrvCtrl   /* device to be stopped */
	)
{
	STATUS result = OK;

	END_FLAGS_CLR (&pDrvCtrl->endObj, IFF_UP | IFF_RUNNING);

	/* TODO - stop/disable the device. */
	/*SYS_INT_DISCONNECT (pDrvCtrl, (FUNCPTR)z077Int, (int)pDrvCtrl, &result);*/
	if (result == ERROR)
	{
		DBGWRT_ERR( (DBH, "*** Could not disconnect interrupt!\n") );
	}

	return (result);
}

/******************************************************************************
 *
 * z077Unload - unload a driver from the system
 *
 * This function first brings down the device, and then frees any
 * stuff that was allocated by the driver in the load function.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077Unload (
	Z077_END_DEVICE* pDrvCtrl   /* device to be unloaded */
	)
{
	END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

	/* TODO - Free any special allocated memory */

	/* New: free the END_OBJ structure allocated during z077Load() */
	cfree ((char *)pDrvCtrl);

	return (OK);
}

/*  MUX->END entry points - end */


/******************************************************************************
 *
 * z077Int - ISR called by PCI int dispatcher
 *
 * This routine is called at interrupt level in response to an interrupt from
 * the controller. If Rx/Tx Errors occured,
 *
 * RETURNS: N/A.
 */

LOCAL void z077Int
(
	Z077_END_DEVICE  *pDrvCtrl  /* interrupting device */
	)
{

	UCHAR stat = (UCHAR)Z77READ_D32(Z077_BASE, Z077_REG_INT_SRC);

	if (!stat)  /* not my IRQ */
		return;

	/*
	 * The following condition can occured in case of shared interrupt.
	 * Then we check it and exit if we are in polling mode.
	 */
	if (pDrvCtrl->flags & Z077_POLLING) {
		Z77WRITE_D32( Z077_BASE, Z077_REG_INT_SRC, stat);
		return;
	}

	/* OETH_INT_RXE - Rx Error IRQ occured ? handle before */
	if( stat & OETH_INT_RXE && pDrvCtrl->RunningRxErrJobs < 5 )
	{
		if(netJobAdd((FUNCPTR)z077HandleRxErr,(int)pDrvCtrl,0,0,0,0) != OK)
		{
			IDBGWRT_ERR( (DBH, "*** RxErr: netJobRing is full!\n") );
		}
		else
		{
			pDrvCtrl->RunningRxErrJobs++;
		}
	}

	/* Normal Frame received ? */
	if (stat & OETH_INT_RXF)
	{
		IDBGWRT_1(( DBH, "INT_RXF %d\n", pDrvCtrl->nrRxbIrq ));
		pDrvCtrl->nrRxbIrq++;

		if( pDrvCtrl->RunningRxJobs < 1 )
		{
			if( netJobAdd((FUNCPTR)z077HandleRecv, (int)pDrvCtrl, 1/* IRQ fired */,0,0,0) == OK )
			{
				pDrvCtrl->RunningRxJobs++;
			}
			else
			{
				IDBGWRT_ERR( (DBH, "*** RxInt: netJobRing is full!\n") );
			}
		}
		if( pDrvCtrl->RunningRxJobs > 0 )
		{
			if( pDrvCtrl->newRxIrqBehaviour )
			{  /* disable RX IRQ if at least 1 RunningRxJobs are on the way */
				Z077_DISABLE_IRQ( OETH_INT_RXF );
			}
		}
	}

	/*
	 * OETH_INT_BUSY - Busy IRQ. No free BDs or FIFO full/underrun.
	 * needs to be cared of at TCP/IP level. We Restart the core.
	 */
	if ( stat & OETH_INT_BUSY )
	{
		if( netJobAdd((FUNCPTR)z077Restart,(int)pDrvCtrl,0,0,0,0) != OK )
		{
			IDBGWRT_ERR( (DBH, "*** Busy: netJobRing is full!\n") );
		}
		else
		{
			pDrvCtrl->RunningRestartJobs++;
		}
	}

	/* OETH_INT_TXE - Tx Error IRQ occured */
	if ( stat & OETH_INT_TXE && pDrvCtrl->RunningTxErrJobs < 5 )
	{
		if(netJobAdd((FUNCPTR)z077HandleTxErr,(int)pDrvCtrl,0,0,0,0) != OK)
		{
			IDBGWRT_ERR( (DBH, "*** TxErr: netJobRing is full!\n") );
		}
		else
		{
			pDrvCtrl->RunningTxErrJobs++;
		}
	}

	/* clear a Tx Stall, enable Network Stack to Send Again */
	if (pDrvCtrl->txBlocked) {
		pDrvCtrl->txBlocked = FALSE;
		netJobAdd ((FUNCPTR)muxTxRestart,(int)&pDrvCtrl->endObj, 0, 0, 0, 0);
	}

	/* yes, it was me - clear IRQ by writing back ISR status bits */
	Z77WRITE_D32( Z077_BASE, Z077_REG_INT_SRC, stat);

	return;
}

/******************************************************************************
 *
 * z077Recv - process the next incoming packet
 *
 * Handle one packet from the Rx BD Ring.
 *
 * RETURNS: OK or ERROR if no net Tuple could be obtained.
 */
LOCAL STATUS z077Recv
(
	Z077_END_DEVICE *pDrvCtrl,  /* device structure                    */
	UINT32 pktIndex,            /* Rx packet index (BD#) to process    */
	M_BLK_ID pMblkRbd,          /* pointer to an mBlk for polling mode */
	BOOL isPollMode             /* TRUE if we are in polling mode      */
	)
{

	int         len      = 0;
	M_BLK_ID    pMblkNew = NULL;
	STATUS		error	 = ERROR;

	pDrvCtrl->lastRxPosition = pktIndex;
	/* If not in polling mode use the mBlk from the list */
	if (!isPollMode) {
		pMblkRbd = pDrvCtrl->pMblkList[pktIndex];
	}

	/*
	 * Assert that this packet isnt erroneous, check RxBD_stat[6:1]
	 * if its somehow damaged, just set Empty bit again.
	 */
	if ( Z077_GET_RBD_FLAG( pktIndex, OETH_ERRFLAGS ))
	{
		goto clean;
	}

	if (!isPollMode) {
		/* Get a Tuple to replace the tuple just beeing used */
		pMblkNew = netTupleGet(pDrvCtrl->endObj.pNetPool, Z077_MAX_CL_LEN, M_WAIT, MT_DATA, TRUE);
			if ( pMblkNew == NULL )
			{
				pDrvCtrl->errRxNoMem++;
				END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
				DBGWRT_ERR( (DBH, "*** z077Recv: netTupleGet() lost BD#%d\n", pktIndex) );
				goto clean;
			}
		pDrvCtrl->pMblkList[pktIndex] = pMblkNew;
	}

	error = OK;

	/* put the information from our receive buffer to tuple to be passed to stack */
	len = Z077_GET_RBD_LEN(pktIndex);
	pMblkRbd->mBlkHdr.mLen      = len - ETHER_CRC_LEN;
	pMblkRbd->mBlkPktHdr.len    = pMblkRbd->mBlkHdr.mLen;

	/*
	 * In some circumstance (not reproducible) the lenght can be
	 * less of the minimum expected packet lenght and this can generate
	 * a pagefault. To avoid this we discard this packet.
	 */
	if (len < Z077_MIN_PACKLEN) {
		logMsg("*** read len 0x%04x RxBD %d Flags:%04x\n",
			   len, pktIndex,
			   Z077_GET_RBD_FLAG( pktIndex , 0xff),0,0,0);
		error = ERROR;
		goto clean;
	}

	if (isPollMode) {
		if(pMblkRbd->m_len < len) {
#if _WRS_VXWORKS_MAJOR == 7
            bcopy (mtod(pDrvCtrl->pMblkList[pktIndex], UINT32), pMblkRbd->mBlkHdr.mData, len);
#else
			bcopy ((char *)Z077_GET_RBD_ADDR(pktIndex), pMblkRbd->mBlkHdr.mData, len);
#endif
		} else {
			error = ERROR;
		}
	}

	/* set the packet header flag */
	pMblkRbd->mBlkHdr.mFlags |= M_PKTHDR;

	if (!isPollMode) {
		/* associate the new Tuple with this BD */
#if _WRS_VXWORKS_MAJOR == 7
        cacheInvalidate (DATA_CACHE, pMblkNew->m_data, (size_t)pMblkNew->m_len);

        Z077_SET_RBD_ADDR( pktIndex,END_CACHE_VIRT_TO_PHYS((UINT32)(pMblkNew->mBlkHdr.mData)) );
#else
		Z077_SET_RBD_ADDR( pktIndex,(UINT32)(pMblkNew->mBlkHdr.mData) );
#endif

		/* and get it finally to the Stack..., the tuple is returned to the pool here! */
#ifdef MEN_IMX6
		cacheClear (DATA_CACHE, pMblkRbd->mBlkHdr.mData, len);
#endif
#if defined(MEN_MPC5121)
        cacheInvalidate (DATA_CACHE, pMblkRbd->mBlkHdr.mData, len);
#endif
		END_RCV_RTN_CALL( &pDrvCtrl->endObj, pMblkRbd );
	}

clean:
	/* clean processed Rx BD */
	if ( pktIndex < 32 ) {
		Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY0, 1 << pktIndex );
	}
	else {
		Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY1, 1 << (pktIndex-32));
	}
    
	/* update statistics of good packets */
	if( !error )
		END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

	return( error );
}

/******************************************************************************
 *
 * z077HandleRxErr - task level handler to report Rx Errors
 *
 * This Routine is queued as a netJob when a Rx Err IRQ occured. It checks
 * for error bits in the Tx BD.
 *
 * RETURNS: N/A.
 */

LOCAL void z077HandleRxErr
(
	Z077_END_DEVICE *pDrvCtrl
	)
{
	int key;
	UINT32 i = 0;
	UINT32 rxflags = 0;

	if( pDrvCtrl->RunningRxErrJobs > 0 )
	{
		key = intCpuLock();
		pDrvCtrl->RunningRxErrJobs--;
		intCpuUnlock( key );
	}

	/* simply skip Rx BD Ring backwards, report errornous frame and
	   reset this frames RX empty bit! */
	for ( i = Z077_RBD_NUM - 1; i > 0; i--) {

		rxflags = Z077_GET_RBD_FLAG( i , OETH_ERRFLAGS) ;

		if (!Z077_RBD_EMPTY(i) && rxflags )
		{ /* process only nonempty errornous ones */

			/* always report overruns, may indicate severe FPGA problem */
			if ( rxflags & OETH_RX_BD_OVERRUN ) {
				DBGWRT_ERR( (DBH, "*** RX err: overrun[%d]\n", i)  );
				pDrvCtrl->errOverrun++;
			}

			if (rxflags & OETH_RX_BD_INVSYMB){
				DBGWRT_ERR( (DBH, "*** RX err: inv symbol[%d]\n", i) );
				pDrvCtrl->errInvsymb++;
			}

			if (rxflags & OETH_RX_BD_DRIBBLE){
				DBGWRT_ERR( (DBH, "*** RX err: dribble[%d]\n", i)  );
				pDrvCtrl->errDribble++;
			}

			if (rxflags & OETH_RX_BD_TOOLONG){
				DBGWRT_ERR( (DBH, "*** RX err: too long[%d]\n", i)  );
				pDrvCtrl->errToolong++;
			}

			if (rxflags & OETH_RX_BD_SHORT){
				DBGWRT_ERR( (DBH, "*** RX err: too short[%d]\n", i)  );
				pDrvCtrl->errTooshort++;
			}

			if (rxflags & OETH_RX_BD_CRCERR)
			{
				UINT32 len;
				UINT32 adr;

				len = Z077_GET_RBD_LEN(  i );
#if _WRS_VXWORKS_MAJOR == 7
                adr = mtod(pDrvCtrl->pMblkList[i], UINT32);
#else

				adr = Z077_GET_RBD_ADDR( i );
#endif
				DBGWRT_ERR(( DBH, "*** RX err: CRC err[%d] @%08x size %d\n", i, adr, len ));
				pDrvCtrl->errCrcerr++;
			}

			if (rxflags & OETH_RX_BD_LATECOL){
				DBGWRT_ERR( (DBH, "*** RX err: late coll[%d]\n", i)  );
				pDrvCtrl->errLatecoll++;
			}

			/* update statistics */
			pDrvCtrl->nrRxeIrq++; /* increment count for sysMenSecShow() */
			END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);

			/* reset empty bit */
			if ( i < 32 )
				Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY0, 1 << i );
			else
				Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY1, 1 << (i-32));			
		}
	}
}

/******************************************************************************
 **
 * z077Restart - process context routine to recover from a BUSY Irq after
 *               system load too high etc.
 *
 * 				 This Routine is queued as a netJob when a BUSY IRQ occured.
 *
 * RETURNS: N/A.
 */
LOCAL void z077Restart
(
	Z077_END_DEVICE *pDrvCtrl
	)
{
	int error ;
	int key;

	if( pDrvCtrl->RunningRestartJobs > 0 )
	{
		key = intCpuLock();
		pDrvCtrl->RunningRestartJobs--;
		intCpuUnlock( key );
	}

	error = z077Reset( pDrvCtrl );
	if( !error )
	{
		Z077_SET_MODE_FLAG(OETH_MODER_RXEN | OETH_MODER_TXEN );
	}
	else
	{
		DBGWRT_ERR(( DBH, "*** %s: z077Reset()\n", __FUNCTION__ ));
	}
}

/******************************************************************************
 *
 * z077HandleTxErr - task level handler to report Errors
 *
 * This Routine is queued as a netJob when a Tx Err IRQ occured. It checks
 * for error bits in the Tx BD.
 *
 * RETURNS: N/A.
 */
LOCAL void z077HandleTxErr
(
	Z077_END_DEVICE *pDrvCtrl
	)
{
	int i, key;

	if( pDrvCtrl->RunningTxErrJobs > 0 )
	{
		key = intCpuLock();
		pDrvCtrl->RunningTxErrJobs--;
		intCpuUnlock( key );
	}

	/* simply skip Tx BD Ring backwards */
	for (i = Z077_TBD_NUM-1; i >= 0; i--) {

		if (Z077_GET_TBD_FLAG( i, OETH_TX_BD_DEFER)){
			DBGWRT_ERR( (DBH, "*** TX err: defered frame[%d]\n", i)  );
			Z077_CLR_TBD_FLAG( i, OETH_TX_BD_DEFER );
		}

		if (Z077_GET_TBD_FLAG( i, OETH_TX_BD_CARRIER)){
			DBGWRT_ERR( (DBH, "*** TX err: Carrier lost[%d]\n", i)  );
			Z077_CLR_TBD_FLAG( i, OETH_TX_BD_CARRIER);
		}

		if (Z077_GET_TBD_FLAG( i, OETH_TX_BD_RETLIM)){
			DBGWRT_ERR( (DBH, "*** TX err: retrans limit[%d]\n", i)  );
			Z077_CLR_TBD_FLAG( i, OETH_TX_BD_RETLIM);
		}

		if (Z077_GET_TBD_FLAG( i, OETH_TX_BD_LATECOL)){
			DBGWRT_ERR( (DBH, "*** TX err: late coll[%d]\n", i)  );
			Z077_CLR_TBD_FLAG( i, OETH_TX_BD_LATECOL);
		}

		if (Z077_GET_TBD_FLAG( i, OETH_TX_BD_UNDERRUN)){
			DBGWRT_ERR( (DBH, "*** TX err: underrun[%d]\n", i)  );
			Z077_CLR_TBD_FLAG( i, OETH_TX_BD_UNDERRUN);
		}

		pDrvCtrl->nrTxeIrq++;
		Z077_ENABLE_IRQ(OETH_INT_TXE);
		END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1);

	}
}

/******************************************************************************
 *
 * z077HandleRecv - task level interrupt service for input packets
 *
 * This routine is called at task level indirectly by the interrupt
 * service routine to do any message received processing.
 *
 *  Pass packets to the network stack in the order they
 *  arrived, so here the Rx BD Ring is processed from the earliest
 *  unempty packet up to the current Rx BD.
 *
 *  usually we process a received packet at the time but IRQs might
 *  had been disabled by other processes some time. So Skip through
 *  Rx BD Ring until we look at first nonempty packet. This assumes the
 *  BD Ring is not fragmented
 *
 * RETURNS: N/A.
 */
LOCAL void z077HandleRecv
(
	Z077_END_DEVICE *pDrvCtrl,   /* interrupting Z077 device */
	int             irqFired
	)
{
	unsigned int rx0 = 0;
	unsigned int rx1 = 0;
	unsigned int emptyBitPos 	= 0;
	int 		 key;

	z077_DbgRxCount++;

	if( pDrvCtrl->RunningRxJobs > 0 )
	{
		key = intCpuLock();
		pDrvCtrl->RunningRxJobs--;
		intCpuUnlock( key );
	}

	/*
	 * determine which BD we pass to Stack next. we calculate with
	 * inverted Values (bit=1 =>Data available instead 0, easier for below
	 * calculations
	 */
	rx0 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY0);/* RxBDs[31:0]*/
	rx1 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY1);/* RxBDs[63:32]*/
	DBGWRT_3(( DBH, "DbgCount %d fired %d RXjobs %d E0 0x%08x E1 0x%08x\n", z077_DbgRxCount, irqFired, pDrvCtrl->RunningRxJobs, rx0, rx1 ));

	/* pass packets in order they arrived & care of BD63->BD0 Wrap */
	if ( !rx0  && !rx1 )
	{
		goto done;
	}
	else if ( rx0  && !rx1 )
	{
		CHECK_RX0_EMPTY();
		goto done;
	}
	else if ( rx1 && !rx0 )
	{
		CHECK_RX1_EMPTY();
		goto done;
	}
	else if ( rx1 && rx0 )
	{
		if( pDrvCtrl->lastRxPosition < 31 || pDrvCtrl->lastRxPosition == 63 )
		{
			CHECK_RX0_EMPTY();
			goto done;
		}
		else
		{
			CHECK_RX1_EMPTY();
			goto done;
		}
	}
done:
	/* reschedule ourself if another packet arrived meanwhile */
	rx0 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY0);/* RxBDs[31:0]*/
	rx1 = ~Z77READ_D32(Z077_BASE, Z077_REG_RXEMPTY1);/* RxBDs[63:32]*/
	if( ( rx1 || rx0 ) && pDrvCtrl->RunningRxJobs < 1 )
	{	/* not done yet .. schedule myself once more */
		if (netJobAdd((FUNCPTR)z077HandleRecv,(int)pDrvCtrl,0,0,0,0)!=OK)
		{
			DBGWRT_ERR( (DBH, "*** z077HandleRecv: netJobRing full!\n") );
		}
		else
		{
			key = intCpuLock();
			pDrvCtrl->RunningRxJobs++;
			intCpuUnlock( key );
		}
	}

	if( pDrvCtrl->newRxIrqBehaviour && pDrvCtrl->RunningRxJobs < 1 )
	{ /* reenable RX IRQ */
		Z077_ENABLE_IRQ(OETH_INT_RXF);
	}

} /* end z077HandleRecv */

/******************************************************************************
 * z077kickTransmission
 *
 * physically start sending a Frame
 */
LOCAL void z077kickTransmission(Z077_END_DEVICE *pDrvCtrl, UINT32 index)
{

	/* MEN cores maintain separate TX Empty Registers */
	if (index < 32)
		Z77WRITE_D32(Z077_BASE, Z077_REG_TXEMPTY0, (u_int32)(1 << index) );
	else
		Z77WRITE_D32(Z077_BASE, Z077_REG_TXEMPTY1, (u_int32)(1 << (index-32)) );    
}

#ifdef INCLUDE_SHOW_ROUTINES
/******************************************************************************
 *
 * sysMenSecShow - Dump the contents of the OPENETH Registers
 *
 * This helper function displays the User all internal ETH Register plus
 * the MII Standard Registers.
 *
 * RETURNS: OK always
 */
STATUS sysMenSecShow( UINT8 unit_no )
{

	UINT32 tmp = 0;
	UINT16 dat = 0;
	UINT32 i = 0;

	if((unit_no+1) > z077Mgnt.noEndUnits)
	{
		printf("men_sec %u not available!\n", unit_no);
		return ERROR;
	}

	printf("\nZ87 Rev %d Irq %d Base %08x Registers:\n\n",	z077Mgnt.endUnit[unit_no]->chamUnitRevision,
		   z077Mgnt.endUnit[unit_no]->ivec,
		   z077Mgnt.endUnit[unit_no]->baseAddr);
	while (z77_reginfo[i].addr != 0xffff){
		tmp = Z77READ_D32( z077Mgnt.endUnit[unit_no]->baseAddr, z77_reginfo[i].addr);
		printf("%s   0x%08x\n", z77_reginfo[i].name, tmp);
		i++;
	}

	if( 0xff != z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr ){
		printf("\n-------------- MII Registers -----------------\n");
		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr, MII_CTRL_REG, &dat );
		printf("MII_CTRL_REG\t\t0x%04x\n", dat );

		/* it seems that some Phys need 2 accesses until they update link state */
		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_STAT_REG, &dat   );
		taskDelay(1);
		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_STAT_REG, &dat   );
		printf("MII_STAT_REG\t\t0x%04x\tLink: %s\n",
			   dat, (dat & MII_SR_LINK_STATUS) ? "up" : "down" );

		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_AN_ADS_REG, &dat );
		printf("MII_AN_ADS_REG\t\t0x%04x\n", dat );

		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_AN_PRTN_REG, &dat);
		printf("MII_AN_PRTN_REG\t\t0x%04x\n", dat );

		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_RXER_COUNTER, &dat);
		printf("RXER Counter Reg\t0x%04x\n", dat );

		z077MiiRead(z077Mgnt.endUnit[unit_no], z077Mgnt.endUnit[unit_no]->phyInfo->phyAddr , MII_100_BASE_TX_PHY, &dat);
		printf("MII_100_BASE_TX_PHY\t0x%04x (", dat );
		if( dat != 0xFFFF ) {
			/* dump PHY Op Mode Indication Reg. Bits [4..2] */
			switch((dat & 0x1c) >> 2) {
			case 0x0: printf("in Autoneg)\n");
				break;
			case 0x1:printf("10MB HD)\n");
				break;
			case 0x2:printf("100MB HD)\n");
				break;
			case 0x3:printf("Default)\n");
				break;
			case 0x5:printf("10MB FD)\n");
				break;
			case 0x6:printf("100MB FD)\n");
				break;
			case 0x7:printf("PHY Isolated)\n");
				break;
			}
		}
	}
	printf("----------------------------------------------\n\n");
	printf("Interrupts occured on this device so far:\n");
	printf("Rx ctrl            %d\n", z077Mgnt.endUnit[unit_no]->nrRxcIrq  );
	printf("Tx ctrl            %d\n", z077Mgnt.endUnit[unit_no]->nrTxcIrq  );
	printf("Busy (MAC resets)  %d\n", z077Mgnt.endUnit[unit_no]->nrBusyIrq );
	printf("Rx errors          %d\n", z077Mgnt.endUnit[unit_no]->nrRxeIrq  );
	printf(" |-> overruns:     %d\n", z077Mgnt.endUnit[unit_no]->errOverrun);
	printf(" |-> invalid syms: %d\n", z077Mgnt.endUnit[unit_no]->errInvsymb);
	printf(" |-> dribbles:     %d\n", z077Mgnt.endUnit[unit_no]->errDribble);
	printf(" |-> too short:    %d\n", z077Mgnt.endUnit[unit_no]->errTooshort);
	printf(" |-> too long:     %d\n", z077Mgnt.endUnit[unit_no]->errToolong);
	printf(" |-> CRC err:      %d\n", z077Mgnt.endUnit[unit_no]->errCrcerr );
	printf(" |-> late Coll.:   %d\n", z077Mgnt.endUnit[unit_no]->errLatecoll );
	printf(" `-> no mem    :   %d\n", z077Mgnt.endUnit[unit_no]->errRxNoMem  );
	printf("Rx Frames  %d\n", z077Mgnt.endUnit[unit_no]->nrRxbIrq  );
	printf("Tx errors  %d\n", z077Mgnt.endUnit[unit_no]->nrTxeIrq  );
	printf("Tx frames  %d\n", z077Mgnt.endUnit[unit_no]->nrTxbIrq  );
	return (OK);
}

/******************************************************************************
 *
 * sysMenSecRbdShow - Dump the contents of all Buffer Descriptors. Globally visible
 *          helper function
 * RETURNS: OK always
 */
void sysMenSecRbdShow( UINT8 unit_no  )
{
	if(unit_no >= z077Mgnt.noEndUnits)
	{
		printf("men_sec %u not available! (max. %u)\n", unit_no, z077Mgnt.noEndUnits-1);
		return;
	}

	Z077_END_DEVICE *pDrvCtrl = z077Mgnt.endUnit[unit_no];
	printf("RX Bd Ring Status:\n");
	z077dumpRbd( pDrvCtrl);
}

/******************************************************************************
 *
 * sysMenSecTbdShow - Dump the contents of all Buffer Descriptors. Global helper fct.
 *
 * RETURNS: OK always
 */
void sysMenSecTbdShow( UINT8 unit_no  )
{
	if(unit_no >= z077Mgnt.noEndUnits)
	{
		printf("men_sec %u not available! (max. %u)\n", unit_no, z077Mgnt.noEndUnits-1);
		return;
	}

	Z077_END_DEVICE *pDrvCtrl = z077Mgnt.endUnit[unit_no];
	printf("TX Bd Ring Status:\n");
	z077dumpTbd(pDrvCtrl);

}
#endif /* INCLUDE_SHOW_ROUTINES */

#ifdef Z077_DEBUG_DRIVER
/******************************************************************************
 *
 * z077DumpPool - Dump the netPool Information for this END device
 *
 * This helper function displays the User all internal ETH Register plus
 * the MII Standard Registers.
 *
 * RETURNS: OK always
 */
STATUS z077DumpPool( int unit )
{

	END_OBJ * pEnd = NULL;

	if ((pEnd = endFindByName ( "men_sec" /* dev */, unit )) != NULL)
		netPoolShow (pEnd->pNetPool);
	else
		printf ("Could not find device end0/* %s%d */\n"/* , devName, unit */);

	return (OK);
}
#endif

/******************************************************************************
 *
 * z077Config - reconfigure the interface under us.
 *
 * Reconfigure the interface setting promiscuous mode, and changing the
 * multicast interface list.
 *
 * RETURNS: N/A.
 */
LOCAL void z077Config (
	Z077_END_DEVICE *pDrvCtrl   /* device to be re-configured */
	)
{
	/* Set promiscuous mode if it's asked for. */
	if (END_FLAGS_GET(&pDrvCtrl->endObj) & IFF_PROMISC) {
		DBGWRT_6( (DBH, "Set promisc mode on!\n") );
		Z077_SET_MODE_FLAG(OETH_MODER_PRO);

	} else {
		DBGWRT_6( (DBH, "Set promisc mode off!\n") );
		Z077_CLR_MODE_FLAG(OETH_MODER_PRO);
	}

	return;
}

/******************************************************************************
 *
 * z077PollStart - start polled mode operations
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077PollStart (
	Z077_END_DEVICE * pDrvCtrl  /* device to be polled */
	)
{
	int         oldLevel;

	oldLevel = intCpuLock ();          /* disable ints during update */

	/* turn off interrupts */
	pDrvCtrl->z077IntMask = Z77READ_D32(Z077_BASE, Z077_REG_INT_MASK);

	pDrvCtrl->flags |= Z077_POLLING;

	intCpuUnlock (oldLevel);   /* now z077Int won't get confused */
	DBGWRT_8( (DBH, "STARTED\n") );

	z077Config (pDrvCtrl);  /* reconfigure device */

	return (OK);
}

/******************************************************************************
 *
 * z077PollStop - stop polled mode operations
 *
 * This function terminates polled mode operation.  The device returns to
 * interrupt mode.
 *
 * The device interrupts are enabled, the current mode flag is switched
 * to indicate interrupt mode and the device is then reconfigured for
 * interrupt operation.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077PollStop (
	Z077_END_DEVICE * pDrvCtrl  /* device to be polled */
	)
{
	int         oldLevel;

	oldLevel = intCpuLock ();  /* disable ints during register updates */

	/* re-enable interrupts */
	Z77WRITE_D32(Z077_BASE, Z077_REG_INT_MASK, pDrvCtrl->z077IntMask);

	pDrvCtrl->flags &= ~Z077_POLLING;

	intCpuUnlock(oldLevel);

	z077Config (pDrvCtrl);

	DBGWRT_8( (DBH, "STOPPED\n") );

	return (OK);
}

/******************************************************************************
 *
 * z077Reset - reset device
 *
 * RETURNS: N/A.
 */
LOCAL STATUS z077Reset ( Z077_END_DEVICE *pDrvCtrl )
{

	u_int32 i;
	u_int32 timeout = Z87_RST_TIMEOUT * sysClkRateGet();
	int error = OK;

	DBGWRT_1(( DBH, "%s: pDrvCtrl %08x base %08x timeout ticks %d\n", __FUNCTION__, pDrvCtrl, pDrvCtrl->baseAddr, timeout ));

	/* disable RX/TX... */
	Z077_CLR_MODE_FLAG(OETH_MODER_RXEN | OETH_MODER_TXEN );

	/* wait for MODER[17:18] ( until both Rx/Tx idle reaches 1 again) */
	i = 0;
	while( ((Z77READ_D32(Z077_BASE, Z077_REG_MODER) & TXRX_IDLE)!= TXRX_IDLE))
	{
		taskDelay( 1 );
		i++;
		if( i >= timeout )
		{
			DBGWRT_ERR(( DBH, "*** %s: base %08x IP core reset timed out!\n", __FUNCTION__, pDrvCtrl->baseAddr ));
			logMsg( "*** %s: base %08x IP core reset timed out!\n", (int)__FUNCTION__, (int)pDrvCtrl->baseAddr,3,4,5,6 );
			taskDelay( 1 ); /* time for log message */

			error =  ERROR;
			goto CLEANUP;
		}
	}

	/* ok, go on */
	pDrvCtrl->nCurrRbd = 0;
	pDrvCtrl->nCurrTbd = 0;
	pDrvCtrl->RunningRxJobs     = 0;
	pDrvCtrl->RunningRxErrJobs	= 0;
	pDrvCtrl->RunningTxErrJobs	= 0;
	pDrvCtrl->RunningRestartJobs= 0;
	pDrvCtrl->lastRxPosition	= 0;

	Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY0, 0xffffffff);
	Z77WRITE_D32(Z077_BASE, Z077_REG_RXEMPTY1, 0xffffffff);
    
CLEANUP:
	return( error );
}

/**************************************************************************
 *
 * z077PhyInit - initialize and configure the PHY devices
 *
 * RETURNS: OK or ERROR if the PHY could not be initialised.
 *
 */

LOCAL STATUS z077PhyInit (
	Z077_END_DEVICE *  pDrvCtrl
	)
{
	UINT16  dat = 0;
	UINT8   phyAddr;        /* address of a PHY                     */
	int     retVal;         /* convenient holder for return value   */
	BOOL    found = FALSE;  /* no PHY has been found                */

	DBGWRT_1( (DBH, "--> z077PhyInit\n") );

	/* init Autonegotiation on the PHY */
	phyAddr = pDrvCtrl->phyInfo->phyAddr;

	if( 0xFF == phyAddr ){
		printf("men_sec END Driver: operation without PHY requested.\n");
		DBGWRT_1( (DBH, "z077PhyInit: Z077 operation without PHY requested.\n"));
		return (OK);
	}

	/* reset the PHY */
	z077PhyReset( pDrvCtrl, phyAddr );

	DBGWRT_1( (DBH, "Init PHY on addr 0x%02x\n", phyAddr) );

	/* check the PHY is there */
	retVal = z077MiiProbe(pDrvCtrl, phyAddr);
	if (retVal != OK)
		return (ERROR);

	found = TRUE;

	if( pDrvCtrl->phyInfo->miiRegs.phyId1 != Z077_PHY_MICREL_ID1 ||
		pDrvCtrl->phyInfo->miiRegs.phyId2 != Z077_PHY_MICREL_ID2_KS8995)
	{
		/* disable powerdown mode */
		z077MiiRead(pDrvCtrl, phyAddr , MII_100_BASE_TX_PHY, &dat);
		dat &=~(MII_100BASE_PWR);
		z077MiiWrite(pDrvCtrl, phyAddr, MII_100_BASE_TX_PHY, dat);
	}

	/*
	 *  start auto-negotiation process unless user dictated the contrary.
	 */
	if ( pDrvCtrl->phyInfo->phyFlags & MII_PHY_AUTO ){
		DBGWRT_1( (DBH, "Running Autonegotiation\n") );
		if (z077MiiAnRun (pDrvCtrl, phyAddr) == OK)
			return (OK);
	} else {
		DBGWRT_1( (DBH, "Init PHY with Forced settings\n") );
		if (z077MiiModeForce (pDrvCtrl, phyAddr) == OK) {
			return (OK);
		}
	}

	if (!found)
		return (ERROR);

	/* if we're here, none of the PHYs could be initialized */
	logMsg ("\n*** men_sec: check cable connection!\n", 0,0,0,0,0,0);
	return (ERROR);
}

/******************************************************************************
 *
 * z077PhyReset
 *
 * This routine send a Reset command to the PHY specified in
 * the parameter <phyAddr>.
 *
 * RETURNS: OK or ERROR.
 */
LOCAL STATUS z077PhyReset(
	Z077_END_DEVICE *  pDrvCtrl,    /* pointer to DRV_CTRL structure    */
	UINT8   phyAddr                 /* address of the PHY               */
	)
{

	UINT16 dat = 0;
	UINT32 tmp = 0;

	DBGWRT_5( (DBH, "Phy Reset: Z077_BASE = 0x%08x\n", Z077_BASE) );

	z077MiiRead( pDrvCtrl, phyAddr, MII_CTRL_REG, &dat );

	tmp = Z77READ_D32( Z077_BASE , Z077_REG_MIISTATUS );
	DBGWRT_5( (DBH, "1. MIISTATUS = 0x%08x dat = 0x%04x\n", tmp, dat) );

	z077MiiWrite(pDrvCtrl, phyAddr, MII_CTRL_REG, MII_CR_RESET );

	z077MiiRead(pDrvCtrl, phyAddr, MII_CTRL_REG, &dat );

	DBGWRT_5( (DBH, "2. dat = 0x%04x\n", dat) );

	return(OK);
}

/**************************************************************************
 *
 * z077MiiProbe - probe the PHY device
 *
 * This routine probes the PHY device by reading its PHY Identifier Register
 *
 * RETURNS: OK, ERROR in case of fatal errors, or FEC_END_PHY_NULL, if
 * the device was not found.
 */

LOCAL STATUS z077MiiProbe (
	Z077_END_DEVICE *  pDrvCtrl,   /* pointer to DRV_CTRL structure     */
	UINT8   phyAddr                /* the PHY being read                */
	)
{

	UINT32  i=0;
	UINT16  id1, id2;

	DBGWRT_1( (DBH, "--> z077MiiProbe\n") );

	if (z077MiiRead (pDrvCtrl, phyAddr, MII_PHY_ID1_REG, &id1) != OK)
		return (ERROR);
	if (z077MiiRead (pDrvCtrl, phyAddr, MII_PHY_ID2_REG, &id2) != OK)
		return (ERROR);

	if( id1 == Z077_PHY_MICREL_ID1 || id1 == Z077_PHY_INTEL_ID1 )
	{
		id2 &= Z077_PHY_MICREL_ID2_MASK; /* suppress the last 3 bits of the silicon rev. */
	}

	while (z077PhyAttachTbl[i].ident1 != 0xffff){
		if (id1 == z077PhyAttachTbl[i].ident1 &&
			id2 == z077PhyAttachTbl[i].ident2 )
		{
			DBGWRT_1( (DBH, "Phy %s found.\n", z077PhyAttachTbl[i].name) );
			pDrvCtrl->phyInfo->miiRegs.phyId1 = id1;
			pDrvCtrl->phyInfo->miiRegs.phyId2 = id2;
			return (OK);
		}
		i++;
	}
	DBGWRT_ERR( (DBH, "!!!No matching PHY found with read IDs 0x%04x/0x%04x!!!\n", id1, id2) );
	printf( "men_sec: !!!No matching PHY found with read IDs 0x%04x/0x%04x!!!\n", id1, id2 );

	return (MII_PHY_NULL);

}

/******************************************************************************
 *
 * z077MiiRead - read the MII register
 *
 * This routine reads the register specified by <phyReg> in the PHY device
 * whose address is <phyAddr>. The value read is returned in the location
 * pointed to by <retVal>.
 *
 * RETURNS: OK (since errorneous HW access can't be detected)
 *
 */
STATUS z077MiiRead(
	Z077_END_DEVICE * pDrvCtrl, /* pointer to DRV_CTRL structure */
	UINT8   phyAddr,            /* the PHY being read            */
	UINT8   regAddr,            /* the PHY's register being read */
	UINT16  *retVal             /* the value read                */
	)
{
	/* wait until a previous BUSY disappears */
	Z077_WAIT_FOR_BUSY;
	/* set up combined PHY and Register within Phy */

	DBGWRT_5( (DBH, "z077MiiRead phyAdr %d reg. %d: ", phyAddr, regAddr) );

	Z77WRITE_D32( Z077_BASE, Z077_REG_MIIADR, regAddr << 8 | phyAddr );

	/* kickoff Read Command, this asserts BUSY*/
	Z77WRITE_D32( Z077_BASE, Z077_REG_MIICMD, OETH_MIICMD_RSTAT);

	/* wait for busy to disappear */
	Z077_WAIT_FOR_BUSY;

	/* fetch read Value from MIIRX_DATA*/
	*retVal = Z77READ_D32( Z077_BASE, Z077_REG_MIIRX_DATA );

	DBGWRT_5( (DBH, "read Value = 0x%04x\n", *retVal) );

	return (OK);
}

/******************************************************************************
 *
 * z077MiiWrite - write to the MII register
 *
 * This routine writes the register specified by <phyReg> in the PHY device,
 * whose address is <phyAddr>, with the 16-bit value included in <data>.
 *
 * RETURNS: OK
 */
STATUS z077MiiWrite (
	Z077_END_DEVICE *   pDrvCtrl,   /* pointer to DRV_CTRL structure        */
	UINT8   phyAddr,                /* the PHY being written                */
	UINT8   regAddr,                /* the PHY's register being written     */
	UINT16  data                    /* the value written to the PHY register */
	)
{
	/* wait until a previous BUSY disappears */
	Z077_WAIT_FOR_BUSY;

	DBGWRT_5( (DBH, "z077MiiWrite: phyAdr %d reg. %d Val 0x%04x\n", phyAddr, regAddr, data) );

	/* set up combined PHY address and Register within Phy */
	Z77WRITE_D32( Z077_BASE, Z077_REG_MIIADR, regAddr << 8 | phyAddr );

	/* setup TX data */
	Z77WRITE_D32( Z077_BASE, Z077_REG_MIITX_DATA, data );

	/* kickoff write Command, this asserts BUSY*/
	Z77WRITE_D32( Z077_BASE, Z077_REG_MIICMD, OETH_MIICMD_WCTRLDATA );

	/* wait until BUSY disappears */
	Z077_WAIT_FOR_BUSY;

	return (OK);
}

/******************************************************************************
 *
 * z077MiiModeForce - force an operating mode for the PHY
 *
 * This routine forces an operating mode for the PHY whose address is
 * specified in the parameter <phyAddr>. It also calls z077MiiBasicCheck()
 * to ensure a valid link has been established.
 *
 * RETURNS: OK or ERROR.
 */
LOCAL STATUS z077MiiModeForce(
	Z077_END_DEVICE *  pDrvCtrl,
	UINT8   phyAddr     /* address of the PHY */
	)
{
	UINT16  data;       /* data to be written to the control reg */
	UINT8   regAddr;    /* PHY register */
	UINT16  phyStatus;  /* holder for the PHY status */

	DBGWRT_5( (DBH, "--> z077MiiModeForce. Phy adr = %d\n", phyAddr) );

	/* find out what abilities the PHY features */
	regAddr = MII_STAT_REG;
	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK){
		DBGWRT_ERR( (DBH, "*** z077MiiModeForce Error\n") );
		return (ERROR);
	}

	/* force Mode given by pDrvCtrl->phyInfo->phyFlags */
	data = MII_CR_NORM_EN;

	if ((phyStatus & (MII_SR_TX_FULL_DPX | MII_SR_TX_HALF_DPX)) &&
		(Z077_PHY_FLAGS_ISSET (MII_PHY_100))) {
		data |= MII_CR_100;
		pDrvCtrl->phyInfo->phySpeed = MII_100MBS;
	}

	if ((phyStatus & (MII_SR_TX_FULL_DPX | MII_SR_10T_FULL_DPX | MII_SR_T4)) &&
		(Z077_PHY_FLAGS_ISSET (MII_PHY_FD))) {
		pDrvCtrl->phyInfo->phyDefMode = MII_PHY_FD;
		data |= MII_CR_FDX;
	}

	pDrvCtrl->phyInfo->miiRegs.phyCtrl = data;

	regAddr = MII_CTRL_REG;

	if (z077MiiWrite (pDrvCtrl, phyAddr, regAddr, data) != OK)
		return (ERROR);

	/* handle some flags */
	if (pDrvCtrl->phyInfo->phySpeed == MII_100MBS)
		Z077_PHY_FLAGS_SET (MII_PHY_100);
	else
		Z077_PHY_FLAGS_CLEAR (MII_PHY_100);

	if (pDrvCtrl->phyInfo->phyDefMode == MII_PHY_FD)
		Z077_PHY_FLAGS_SET (MII_PHY_FD);
	else
		Z077_PHY_FLAGS_CLEAR (MII_PHY_FD);

	return (OK);
}

/******************************************************************************
 *
 * z077MiiAnRun - run the auto-negotiation process
 *
 * This routine runs the auto-negotiation process for the PHY whose
 * address is specified in the parameter <phyAddr>, without enabling the
 * next page function. It also calls z077MiiAnCheck() to ensure
 * a valid link has been established.
 *
 * RETURNS: OK or ERROR.
 */
LOCAL STATUS z077MiiAnRun (
	Z077_END_DEVICE *  pDrvCtrl,    /* pointer to DRV_CTRL structure */
	UINT8   phyAddr                 /* address of a PHY */
	)
{
	UINT8   regAddr;    /* PHY register */
	UINT16  phyAds;     /* holder for the PHY ads register value */
	UINT16  status;     /* PHY auto-negotiation status */
	UINT16  ix;         /* a counter */
	int     retVal;     /* holder for return value */

	regAddr = MII_AN_ADS_REG;

	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyAds) != OK)
		return (ERROR);

	DBGWRT_1( (DBH, "--> z077MiiAnRun phyAds=0x%x\n", phyAds) );

	/* disable the next page function */
	phyAds &= (~MII_NP_NP);

	if (z077MiiWrite (pDrvCtrl, phyAddr, regAddr, phyAds) != OK)
		return (ERROR);

	/* Read ANER (where available) to clear status from previous operations */
	if( pDrvCtrl->phyInfo->miiRegs.phyId1 != Z077_PHY_MICREL_ID1 ||
		pDrvCtrl->phyInfo->miiRegs.phyId2 != Z077_PHY_MICREL_ID2_KS8995)
	{
		regAddr = MII_AN_EXP_REG;
		if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &status) != OK)
			return (ERROR);

		DBGWRT_1( (DBH, "z077MiiAnRun expStat=0x%x\n", status) );
	}

	/* Read ANAR */
	regAddr = MII_AN_ADS_REG;
	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyAds) != OK)
		return (ERROR);

	/* store the current registers values */
	pDrvCtrl->phyInfo->miiRegs.phyAds = phyAds;

	/*
	 * Start Autionegotiation, possibly following the order the user dictated.
	 */
	for (ix = 0; ; ix++) {
		DBGWRT_5( (DBH, "z077MiiAnRun loop %d\n",ix) );
		if (Z077_PHY_FLAGS_ISSET (MII_PHY_TBL)) {
			/* check we're not at the end of the user table */
			if ((z077PhyAnOrderTbl [ix]) == -1)
				return (ERROR);

			/* just negotiate one ability at a time */
			phyAds &= ~MII_TECH_MASK;

			/* translate user settings */
			phyAds |= z77AnLookupTbl[(z077PhyAnOrderTbl[ix])];

			/* check the PHY has the desidered ability */
			if (!(phyAds & pDrvCtrl->phyInfo->miiRegs.phyAds))
				continue;

			/* set the ANAR accordingly */
			regAddr = MII_AN_ADS_REG;
			if (z077MiiWrite (pDrvCtrl, phyAddr, regAddr, phyAds)
				!= OK)
				return (ERROR);
		} else {
			/*
			 * check the PHY flags and possibly mask some abilities off
			 */

			/* supress Full duplex negotiation ? */
			if ( !(pDrvCtrl->phyInfo->phyFlags & MII_PHY_FD) )  /* *** */
				/* Z077_PHY_DONT_NEGOTIATE_FD */
				phyAds &= ~(MII_TECH_10BASE_FD | MII_TECH_100BASE_TX_FD
							| MII_TECH_100BASE_T4);

			/* supress half duplex negotiation ? */
			if (!(pDrvCtrl->phyInfo->phyFlags & MII_PHY_HD))
				/* Z077_PHY_DONT_NEGOTIATE_HD */
				phyAds &= ~(MII_TECH_10BASE_T | MII_TECH_100BASE_TX);

			/* supress negotiation of 100 Mbit ? */
			if ( !(pDrvCtrl->phyInfo->phyFlags & MII_PHY_100)) /* *** */
				phyAds &= ~(MII_TECH_100BASE_TX | MII_TECH_100BASE_TX_FD
							| MII_TECH_100BASE_T4);

			/* supress negotiation of 10 Mbit ? */
			if (!(pDrvCtrl->phyInfo->phyFlags & MII_PHY_10 ))
				phyAds &= ~(MII_TECH_10BASE_T | MII_TECH_10BASE_FD);

			/* Write ANAR */
			regAddr = MII_AN_ADS_REG;
			if (z077MiiWrite (pDrvCtrl, phyAddr, regAddr, phyAds) != OK)
				return (ERROR);

			/* store the current registers values */
			pDrvCtrl->phyInfo->miiRegs.phyAds = phyAds;

			DBGWRT_5( (DBH, "z077MiiAnRun phyAds=0x%x\n", phyAds) );
		}

		/* start the auto-negotiation process: return only if fatal error. */
		retVal = z077MiiAnStart (pDrvCtrl, phyAddr);
		if (retVal == ERROR)
			return (ERROR);

		/*
		 * in case of failure we return only if we use the standard
		 * auto-negotiation process.
		 */
		if (retVal == MII_AN_FAIL) {
			if ( !(Z077_PHY_FLAGS_ISSET(MII_PHY_TBL)) )
				return (retVal);
			else
				continue;
		}

		/* check the negotiation was successful */
		if (z077MiiAnCheck (pDrvCtrl, phyAddr) == OK)
			return (OK);

		/*
		 * we are here if the negotiation went wong: if the user did not force
		 * any priority order, we return ERROR, as all the PHY abilities
		 * were negotiated at once.
		 */

		if (!(Z077_PHY_FLAGS_ISSET (MII_PHY_TBL)))
			return (ERROR);
	}
	return (OK);
}

/******************************************************************************
 *
 * z077MiiAnStart - start the auto-negotiation process
 *
 * This routine starts the auto-negotiation process for the PHY whose
 * address is specified in the parameter <phyAddr>.
 *
 * RETURNS: OK, ERROR in case of fatal errors or MII_AN_FAIL, if the
 * auto-negotiation did not complete within a reasonable amount of time.
 */

LOCAL STATUS z077MiiAnStart (
	Z077_END_DEVICE *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
	UINT8   phyAddr     /* address of a PHY */
	)
{
	UINT16  data;       /* data to be written to the control reg */
	UINT8   regAddr;    /* PHY register */
	UINT16  phyStatus;  /* holder for the PHY status */
	UINT16  ix = 0;     /* a counter */

	regAddr = MII_STAT_REG;

	if ( z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
		return (ERROR);

	/* check the PHY has this ability */
	if ((phyStatus & MII_SR_AUTO_SEL) != MII_SR_AUTO_SEL) {
		DBGWRT_1( (DBH, "z077MiiAnStart phy not auto neg capable\n") );
		return (ERROR);
	}

	/* restart the auto-negotiation process */
	regAddr = MII_CTRL_REG;
	data = (MII_CR_RESTART | MII_CR_AUTO_EN);

	if (z077MiiWrite (pDrvCtrl, phyAddr, regAddr, data) != OK)
		return (ERROR);

	/* save status info */
	pDrvCtrl->phyInfo->miiRegs.phyCtrl = data;

	/* lets check the PHY status */
	regAddr = MII_STAT_REG;

	/* spin until it is done */
	taskDelay(1);
	ix += ((UINT16) TICKLEN_MS);
	do {
		/* PHY ok? */
		if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
			return (ERROR);

		/* Link? */
		if ((phyStatus & MII_SR_LINK_STATUS) != MII_SR_LINK_STATUS) {
			/* wait for half second */
			DBGWRT_5( (DBH, "no link @ %u\n", tickGet()) );
		} else {
			DBGWRT_5( (DBH, "link is up!\n") );
		}

		/* Autoneg complete? */
		if((phyStatus & MII_SR_AUTO_NEG) == MII_SR_AUTO_NEG)
			break;

		/* Have we waited long enough? */
		if (ix >= Z077_PHY_MAX_WAIT)
			break;

		/* wait more */
		ix += 500;
		taskDelay ( SECOND >> 1 );
	} while (1);

	if (ix >= Z077_PHY_MAX_WAIT) {
		DBGWRT_ERR( (DBH, "*** z077MiiAnStart auto neg fail\n") );
		return (MII_AN_FAIL);
	} else {
		DBGWRT_5( (DBH, "z077MiiAnStart auto neg done phyStat=0x%x\n", phyStatus) );
	}

	return (OK);
}

/******************************************************************************
 *
 * z077MiiBasicCheck - run a basic check on the PHY status register
 *
 * This routine runs a basic check on the PHY status register to
 * ensure a valid link has been established and no faults have been
 * detected.
 *
 * RETURNS: OK, MII_STAT_FAIL, if an error was reported in the PHY's
 * status register, or ERROR, in case of unrecoverable errors.
 */

LOCAL STATUS z077MiiBasicCheck(
	Z077_END_DEVICE *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
	UINT8   phyAddr     /* address of a PHY */
	)
{
	UINT8   regAddr;    /* PHY register */
	UINT16  phyStatus;  /* holder for the PHY status */

	/* let's check the PHY status */
	regAddr = MII_STAT_REG;
	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
		return (ERROR);

	if ((phyStatus & MII_SR_LINK_STATUS) != MII_SR_LINK_STATUS) {
		/* wait for a while */

		taskDelay (1);

		if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
			return (ERROR);

		if ((phyStatus & MII_SR_LINK_STATUS) != MII_SR_LINK_STATUS) {
			DBGWRT_5( (DBH, "z077MiiBasicCheck: phy stat=0x%x\n", phyStatus) );
			return (MII_STAT_FAIL);
		}
	}

	DBGWRT_5( (DBH,  "z077MiiBasicCheck Link up! status=0x%x\n", phyStatus) );

	/* check for remote fault condition */
	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
		return (ERROR);

	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyStatus) != OK)
		return (ERROR);

	if ((phyStatus & MII_SR_REMOTE_FAULT) == MII_SR_REMOTE_FAULT) {
		DBGWRT_5( (DBH,  "z077MiiBasicCheck remote fault\n") );
		return (MII_STAT_FAIL);
	}

	/* store the current registers values */
	pDrvCtrl->phyInfo->miiRegs.phyStatus = phyStatus;

	return (OK);
}

/******************************************************************************
 *
 * z077MiiAnCheck - check the auto-negotiation process result
 *
 * This routine checks the auto-negotiation process has completed
 * successfully and no faults have been detected by any of the PHYs
 * engaged in the process.
 *
 * RETURNS: OK or ERROR.
 */

LOCAL STATUS z077MiiAnCheck
(
	Z077_END_DEVICE *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
	UINT8   phyAddr     /* address of a PHY */
	)
{
	UINT8   regAddr;    /* PHY register */
	UINT16  phyAds;     /* PHY advertisement register value */
	UINT16  phyPrtn;    /* PHY partner ability register value */
	UINT16  phyExp;     /* PHY expansion register value */
	UINT16  negAbility; /* abilities after negotiation */
	int     retVal;     /* convenient holder for return value */
	char*   result;

	/* run a check on the status bits of basic registers only */
	retVal = z077MiiBasicCheck (pDrvCtrl, phyAddr);
	if (retVal != OK)
		return (retVal);

	if( pDrvCtrl->phyInfo->miiRegs.phyId1 != Z077_PHY_MICREL_ID1 ||
		pDrvCtrl->phyInfo->miiRegs.phyId2 != Z077_PHY_MICREL_ID2_KS8995)
	{
		/* we know the auto-negotiation process has terminated */
		regAddr = MII_AN_EXP_REG;

		if ( z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyExp) != OK )
			return (ERROR);

		/* check for faults detected by the parallel function */
		if ((phyExp & MII_EXP_FAULT) == MII_EXP_FAULT) {
			DBGWRT_5( (DBH, "z077MiiAnCheck: fault expStat=0x%x\n", phyExp) );

			return (ERROR);
		}
	}
	/* check for remote faults */
	regAddr = MII_AN_PRTN_REG;

	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyPrtn) != OK)
		return (ERROR);

	if ((phyPrtn & MII_BP_FAULT) == MII_BP_FAULT) {
		DBGWRT_5( (DBH,  "z077MiiAnCheck partner stat=0x%x\n", phyPrtn) );
		return (ERROR);
	}

	regAddr = MII_AN_ADS_REG;
	if (z077MiiRead (pDrvCtrl, phyAddr, regAddr, &phyAds) != OK)
		return (ERROR);

	/* store the current registers values */
	pDrvCtrl->phyInfo->miiRegs.phyAds   = phyAds;
	pDrvCtrl->phyInfo->miiRegs.phyPrtn  = phyPrtn;
	pDrvCtrl->phyInfo->miiRegs.phyExp   = phyExp;

	/* find out the max common abilities */
	negAbility = phyPrtn & phyAds & MII_TECH_MASK;

	DBGWRT_5( (DBH, "z77MiiAnChk: phyAds %x phyPrtn 0x%x common 0x%x phyExp=0x%x\n",
			   phyAds, phyPrtn, negAbility, phyExp) );

	if (negAbility & MII_TECH_100BASE_TX_FD) {
		pDrvCtrl->phyInfo->phySpeed = MII_100MBS;
		pDrvCtrl->phyInfo->phyDefMode  = MII_PHY_FD;
		result = "FD";

	} else if ((negAbility & MII_TECH_100BASE_T4) ||
			   (negAbility & MII_TECH_100BASE_TX)) {
		pDrvCtrl->phyInfo->phySpeed = MII_100MBS;
		pDrvCtrl->phyInfo->phyDefMode = MII_PHY_HD;
		result = "HD";

	} else if (negAbility & MII_TECH_10BASE_FD) {
		pDrvCtrl->phyInfo->phySpeed = MII_10MBS;
		pDrvCtrl->phyInfo->phyDefMode = MII_PHY_FD;
		result = "FD";

	} else if (negAbility & MII_TECH_10BASE_T) {
		pDrvCtrl->phyInfo->phySpeed = MII_10MBS;
		pDrvCtrl->phyInfo->phyDefMode = MII_PHY_HD;
		result = "HD";

	} else {
		DBGWRT_5( (DBH, "z077MiiAnCheck fail!\n") );
		return (ERROR);
	}

	printf( UNIT_DRIVER_NAME "Autonegotiation done. Speed %dMbit %s\n",
			(pDrvCtrl->phyInfo->phySpeed/1000000), result);

	/* handle some flags */
	if (pDrvCtrl->phyInfo->phySpeed == MII_100MBS)
		Z077_PHY_FLAGS_SET (MII_PHY_100);
	else
		Z077_PHY_FLAGS_CLEAR (MII_PHY_100);

	if (pDrvCtrl->phyInfo->phyDefMode == MII_PHY_FD)
		Z077_PHY_FLAGS_SET (MII_PHY_FD);
	else
		Z077_PHY_FLAGS_CLEAR (MII_PHY_FD);

	DBGWRT_5( (DBH, "z077MiiAnCheck OK\n") );

	return (OK);
}
