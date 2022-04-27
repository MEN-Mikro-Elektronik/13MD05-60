/***********************  I n c l u d e  -  F i l e  ***********************/
/**
 *         Name: z077End.h
 *      Project: VxWorks END driver for MEN Z087_ETH
 *
 *       Author: ts
 *        $Date: 2012/05/03 09:43:27 $
 *    $Revision: 1.19 $
 *
 *  Description: Definitions for VxWorks Enhanced Network Driver (END) for
 *               OpenCore Network FPGA IP Core (MEN 16Z077_ETH) and
 *               MEN 16Z087_ETH FPGA IP Core
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: z077End.h,v $
 * Revision 1.19  2012/05/03 09:43:27  channoyer
 * R: No support of polling mode
 * M:1. Add new registers for MODER
 *   2. Add z077IntMask in struct Z077_END_DEVICE
 *   3. Add define for Z077_TIMEOUT
 *
 * Revision 1.18  2011/03/04 11:05:57  cschuster
 * R: 1. add support to read the Z87 attached MAC EEPROM
 * M: 1. add defines for Z87 I2C register
 *
 * Revision 1.17  2010/10/15 08:52:39  UFranke
 * R: avoid net job queue overload
 * M: added net job running counter for Rx, RxErr, TxErr and Restart
 * R: possibility of loss of RX irq
 * M: added Z087_REG_CORE_REVISION_NEW_RX_IRQ_ENABLE
 *
 * Revision 1.16  2010/01/28 14:14:38  cs
 * R: distinguish single PHYs from same Manufacturer
 * M: modified PHY_DEVICE_TBL to hold ident#1 and #2 values
 *
 * Revision 1.15  2009/06/03 14:31:18  ufranke
 * R: more than one Z77 in onboard FPGA
 * M: added unit number to BSP MAC address get function
 *
 * Revision 1.14  2009/04/08 10:38:46  MKolpak
 * R:1. Modifications for little endian CPU support, multi instance ability
 *      and P511 EEPROM support (MAC address)
 * M:1. - added new defines and typedefs
 *      - endianness now derived from CPU_FAMILY
 *
 * Revision 1.13  2008/05/29 10:42:24  RLange
 * Bugfix for Z077_WAIT_FOR_BUSY Macro, now really waiting
 *
 * Revision 1.12  2008/05/06 10:45:26  ts
 * made comment next to each define and member doxygen-compliant
 * added timeout functionality to MII queries
 *
 * Revision 1.11  2007/08/31 15:34:58  ts
 * unnecessary defines removed
 *
 * Revision 1.10  2007/06/01 15:24:03  ts
 * additional defines for IRQ en/disabling
 *
 * Revision 1.9  2007/02/12 20:14:14  ts
 * Cosmetics
 *
 * Revision 1.8  2007/02/09 20:14:00  ts
 * 1st Release for use with new Core
 *
 * Revision 1.7  2007/02/08 10:46:51  ts
 * First intermediate Version for new Core
 *
 * Revision 1.6  2006/05/05 17:47:26  ts
 * beta2 checkin, cosmetics
 *
 * Revision 1.5  2006/05/02 19:47:05  ts
 * beta release checkin
 *
 * Revision 1.4  2006/04/07 18:26:27  ts
 * updated / corrected defines
 *
 * Revision 1.3  2006/03/28 16:59:53  ts
 * intermediate checkin
 *
 * Revision 1.2  2006/03/27 16:03:39  ts
 * 2nd intermediate checkin
 *
 * Revision 1.1  2006/03/20 18:40:45  ts
 * Initial Revision
 *
 */

#ifndef _Z077END_H_
#define _Z077END_H_

/*
 * Z077 configuraion
 */
#define Z077_MAX_UNITS          12  /* max. 16z077/16z087 units and thus end devices */
#define Z077_MAX_PCI_PATH       10  /* maximal length of pci path */
#define Z077_MAX_PCI_BUS_SEARCH 0x40 /* max. pci busses */
#define Z077_MAX_PCI_DEV_NO     32  /* max. pci devs */
#define	Z077_REV_00             0   /* 16Z077,  all newer ones are > 0*/
#define Z077_MAC_ADDR_SIZE      6
#define Z077_MAC_ADDR_MAGIC     0xB0 /* magic for identifying MAC address in EEPROM */
#define Z077_CHAMID_16Z077_ETH  77
#define Z077_CHAMID_16Z087_ETH  87
#define Z077_CHAMID_16Z001_SMB  1
#define Z077_EOL                -1 /* end of list */
#define Z077_PHY_ACC_TIMEOUT     5
#define Z077_PKT_LEN_GET(pPkt) (((PKT *)pPkt)->len)
#define Z077_PKT_VIRT_GET(pPkt) (((PKT *)pPkt)->pData)
#define Z077_TIMEOUT            10000

/* MEN OUI */
#define		MEN_OUI_BYTE_0	0x00
#define		MEN_OUI_BYTE_1	0xc0
#define		MEN_OUI_BYTE_2	0x3a

/* Z077 uses CPU_FAMILY to decide whether we swap or not */
#ifndef CPU
#error "Please define CPU (also see vxCpu.h)"
#endif
#if defined(CPU_FAMILY)
    #if (CPU_FAMILY == I80X86 || CPU_FAMILY == ARM)
    #undef Z077_BIG_ENDIAN
    #else
    #define Z077_BIG_ENDIAN
    #endif
#else
#error "Please define CPU_FAMILY (also see vxCpu.h)"
#endif

typedef UCHAR Z077_MAC_ADDR[Z077_MAC_ADDR_SIZE];

/* type for the loadfunction of MAC address for the different END units */
typedef STATUS (* Z077_MACLOADFUNC)( UINT32 unitNum, Z077_MAC_ADDR addr );

#ifndef Z77_USE_VXBPCI_FUNCS
 typedef STATUS (* Z077_INT_CONNECT_FNT)(VOIDFUNCPTR *, VOIDFUNCPTR , int );
 typedef STATUS (* Z077_INT_ENABLE_FNT)(int);
#else
#if _WRS_VXWORKS_MAJOR == 7
 typedef STATUS (* Z077_INT_CONNECT_FNT)(VXB_DEV_ID, VXB_RESOURCE *, VOIDFUNCPTR , int );
 typedef STATUS (* Z077_INT_ENABLE_FNT)(VXB_DEV_ID, VXB_RESOURCE *);
#else
 typedef STATUS (* Z077_INT_CONNECT_FNT)(VXB_DEVICE_ID, VOIDFUNCPTR *, VOIDFUNCPTR , int );
 typedef STATUS (* Z077_INT_ENABLE_FNT)(VXB_DEVICE_ID, int, VOIDFUNCPTR , void *);
#endif
#endif

/* this descriptor is used to configure each Z077 END unit */
typedef struct z77_conf_set
    {
    INT8        endUnit;            /* End unit # */
    char*       pciPathStr;         /* The PCI path  to the chameleon device as string */
    UINT8       chamInstance;       /* chameleon instance */
    UINT8       phyAddr;            /* address of PHY connected to instance */
    Z077_MACLOADFUNC getMACaddrFunc;/* callback to get MAC address */
    UINT8       usePciIrq;          /* use PCI interrupts or Chameleon interrupts */
    UINT8       interruptOffset;    /* interrupt offset */
    UINT8       found;              /* for scanning - must always be 0 */
    } Z77_CONF_SET;


/**
 * Registers with offsets to base Address. All Regs are 32bit wide.
 * Base Address is retrieved from FPGA unit Z077 in cham table.
 */
#define Z077_BASE			(pDrvCtrl->baseAddr)
#define Z077_BDBASE			(pDrvCtrl->bdBase)
#define Z077_REG_BASE		(0)

#define Z077_REG_MODER		(Z077_REG_BASE+0x00) /**< Mode Reg.	*/
#define Z077_REG_INT_SRC	(Z077_REG_BASE+0x04) /**< IRQ source  */
#define Z077_REG_INT_MASK	(Z077_REG_BASE+0x08) /**< IRQ Mask 	*/

#define Z077_REG_IPGT		(Z077_REG_BASE+0x0c) /**< InterPacket Gap 	*/
#define Z077_REG_IPGR1		(Z077_REG_BASE+0x10) /**< No InterPacketGap	 */
#define Z077_REG_IPGR2		(Z077_REG_BASE+0x14) /**< No InterPacketGap 2 */

#define Z077_REG_PACKLEN	(Z077_REG_BASE+0x18) /**< Packet Len (min/max) 	*/
#define Z077_REG_COLLCONF	(Z077_REG_BASE+0x1c) /**< Collision/Retry Conf	*/
#define Z077_REG_TX_BDNUM	(Z077_REG_BASE+0x20) /**< Number of TX BDs	*/
#define Z077_REG_CTRLMODER	(Z077_REG_BASE+0x24) /**< Control Module Mode */
#define Z077_REG_MIIMODER	(Z077_REG_BASE+0x28) /**< MII Mode Register	*/
#define Z077_REG_MIICMD		(Z077_REG_BASE+0x2c) /**< MII command	*/
#define Z077_REG_MIIADR		(Z077_REG_BASE+0x30) /**< MII Address */
#define Z077_REG_MIITX_DATA (Z077_REG_BASE+0x34) /**< MII Transmit Data */
#define Z077_REG_MIIRX_DATA (Z077_REG_BASE+0x38) /**< MII Receive Data	*/
#define Z077_REG_MIISTATUS  (Z077_REG_BASE+0x3c) /**< MII Status */
#define Z077_REG_MAC_ADDR0 	(Z077_REG_BASE+0x40) /**< MAC Addr[0-3]	*/
#define Z077_REG_MAC_ADDR1  (Z077_REG_BASE+0x44) /**< MAC Addr[4-5]	*/
#define Z077_REG_HASH_ADDR0 (Z077_REG_BASE+0x48) /**< Hash Reg 0	*/
#define Z077_REG_HASH_ADDR1 (Z077_REG_BASE+0x4c) /**< Hash Reg 1	*/
#define Z077_REG_TXCTRL    	(Z077_REG_BASE+0x50) /**< TX control Reg.	*/

/* Registers added by MEN - this is removed in new Core */
#define Z077_REG_GLOBALRST 	(Z077_REG_BASE+0x54) /**< global MAC core reset */
#define Z077_REG_BDSTART 	(Z077_REG_BASE+0x5c) /**< Start of TxRx BDs	*/
#define Z077_REG_RXEMPTY0 	(Z077_REG_BASE+0x60) /**< RxBD[31:0]  */
#define Z077_REG_RXEMPTY1 	(Z077_REG_BASE+0x64) /**< RxBD[63:32] */
#define Z077_REG_TXEMPTY0 	(Z077_REG_BASE+0x68) /**< TxBD[31:0] */
#define Z077_REG_TXEMPTY1 	(Z077_REG_BASE+0x6c) /**< TxBD[63:32] */


#define Z087_REG_RX_BD_STATUS  (Z077_REG_BASE+0x70) /**< where the IP core would like to place the next data */
#define Z087_REG_I2C           (Z077_REG_BASE+0x74) /**< I2C BitBang interface to e.g. MAC EEPROM */
#define Z087_REG_CORE_REVISION (Z077_REG_BASE+0x78) /**< revision, new RX IRQ behaviour enable */


#define Z087_REG_RX_BD_STATUS_BD_EMPTY   /**< BD empty */
#define Z087_REG_RX_BD_STATUS_FEATURE    /**< this feature implemented */
#define Z087_REG_RX_BD_STATUS_RX_B_POS   /**< currently monitored BD */

#define Z087_REG_CORE_REVISION_NEW_RX_IRQ_ENABLE	0x00000100  /**< [7] new RX IRQ behaviour enable, bit can be set only if feature implemented
																     always RX IRQ if at least one rx buffer descriptor is not handled by SW
															    */

/* The BDs are accessed by the MAC from BASE+0x400 to BASE+0x7ff */
#define OETH_TOTAL_BD       128
#define OETH_MAXBUF_LEN     0x610

/* Tx BD */
#define OETH_TX_BD_READY        0x8000 	/**< Tx BD Ready	*/
#define OETH_TX_BD_IRQ          0x4000 	/**< Tx BD IRQ Enable	*/
#define OETH_TX_BD_WRAP         0x2000 	/**< Tx BD Wrap (last BD) 	*/
#define OETH_TX_BD_PAD          0x1000 	/**< Tx BD Pad Enable	*/
#define OETH_TX_BD_CRC          0x0800 	/**< Tx BD CRC Enable	*/
#define OETH_TX_BD_UNDERRUN     0x0100 	/**< Tx BD Underrun Status */
#define OETH_TX_BD_RETRY        0x00F0 	/**< Tx BD Retry Status */
#define OETH_TX_BD_RETLIM       0x0008 	/**< Tx BD Retransm Limit Stat */
#define OETH_TX_BD_LATECOL      0x0004 	/**< Tx BD Late Collision Stat */
#define OETH_TX_BD_DEFER        0x0002 	/**< Tx BD Defer Status */
#define OETH_TX_BD_CARRIER      0x0001 	/**< Tx BD Carrier Sense Lost Status */
#define OETH_TX_BD_STATS   (OETH_TX_BD_UNDERRUN | OETH_TX_BD_RETRY |\
							OETH_TX_BD_RETLIM | OETH_TX_BD_LATECOL | \
							OETH_TX_BD_DEFER | OETH_TX_BD_CARRIER)

/* Rx BD */
#define OETH_RX_BD_EMPTY        0x8000 	/**< Rx BD Empty */
#define OETH_RX_BD_IRQ          0x4000 	/**< Rx BD IRQ Enable */
#define OETH_RX_BD_WRAP         0x2000 	/**< Rx BD Wrap (last BD) */

#define OETH_RX_BD_MISS         0x0080 	/**< Rx BD Miss Status 	*/
#define OETH_RX_BD_OVERRUN      0x0040 	/**< Rx BD Overrun Status	*/
#define OETH_RX_BD_INVSYMB      0x0020 	/**< Rx BD Invalid Symbol Status */
#define OETH_RX_BD_DRIBBLE      0x0010 	/**< Rx BD Dribble Nibble Status */
#define OETH_RX_BD_TOOLONG      0x0008 	/**< Rx BD Too Long Status */
#define OETH_RX_BD_SHORT        0x0004 	/**< Rx BD Too Short Frame Status */
#define OETH_RX_BD_CRCERR       0x0002 	/**< Rx BD CRC Error Status */
#define OETH_RX_BD_LATECOL      0x0001 	/**< Rx BD Late Collision Status */

#define OETH_ERRFLAGS			OETH_RX_BD_OVERRUN  |\
	                            OETH_RX_BD_INVSYMB  |\
	                            OETH_RX_BD_DRIBBLE  |\
                                OETH_RX_BD_TOOLONG  |\
	                            OETH_RX_BD_SHORT    |\
	                            OETH_RX_BD_CRCERR   |\
	                            OETH_RX_BD_LATECOL
/*
 * MODER Register
 */
#define OETH_MODER_RXEN     (1<<0) 	/**< Receive Enable  	*/
#define OETH_MODER_TXEN     (1<<1) 	/**< Transmit Enable 					*/
#define OETH_MODER_NOPRE    (1<<2) 	/**< No Preamb.   		  	REV01:NA	*/
#define OETH_MODER_BRO      (1<<3) 	/**< Reject Broadcast 	*/
#define OETH_MODER_IAM      (1<<4) 	/**< Use Individual Hash*/
#define OETH_MODER_PRO      (1<<5) 	/**< Promisc Mode */
#define OETH_MODER_IFG      (1<<6) 	/**< Min. IFG not required  REV01:NA 	*/
#define OETH_MODER_LOOPBCK  (1<<7) 	/**< Loop Back 		  	  	REV01:NA	*/
#define OETH_MODER_NOBCKOF  (1<<8) 	/**< No Backoff 		  	REV01:NA	*/
#define OETH_MODER_EXDFREN  (1<<9) 	/**< Excess Defer 		  	REV01:NA	*/
#define OETH_MODER_FULLD    (1<<10) /**< Full Duplex */
#define OETH_MODER_RST      (1<<11) /**< Reset MAC 				REV01:NA 	*/
#define OETH_MODER_DLYCRCEN (1<<12) /**< Delayed CRC Enable 	REV01:NA */
#define OETH_MODER_CRCEN    (1<<13) /**< CRC Enable	*/
#define OETH_MODER_HUGEN    (1<<14) /**< Huge Enable  			REV01:NA	*/
#define OETH_MODER_PAD      (1<<15) /**< Pad Enable 	*/
#define OETH_MODER_RECSMALL (1<<16) /**< Receive Small 	  	  	REV01:NA	*/
#define OETH_MODER_RX_IDLE  (1<<17) /**< RX idle State */
#define OETH_MODER_TX_IDLE  (1<<18) /**< TX idle State */
#define OETH_MODER_GEN_HD   (1<<19) /**< Halfduplex generic status */
#define OETH_MODER_GEN_FC   (1<<20) /**< Flow_Control generic status */
#define OETH_MODER_BD_MAX   (0x3F<<21) /**< Maximum number of available buffer descriptors */
#define OETH_MODER_COMPL    (0x07<<29) /**< Compliance Mode Activation */


/*
 * Interrupt Source Register
 */
#define OETH_INT_TXB    0x01 /**< Transmit Buffer IRQ */
#define OETH_INT_TXE    0x02 /**< Transmit Error IRQ */
#define OETH_INT_RXF    0x04 /**< Receive Frame IRQ */
#define OETH_INT_RXE    0x08 /**< Receive Error IRQ */
#define OETH_INT_BUSY   0x10 /**< Busy IRQ 	*/
#define OETH_INT_TXC    0x20 /**< Tx Control Frame IRQ	*/
#define OETH_INT_RXC    0x40 /**< Rx Control Frame IRQ	*/

/*
 * Control Module Mode Register
 */
#define OETH_CTRLMODER_PASSALL  0x0001 /**< Pass Control Frames 		*/
#define OETH_CTRLMODER_RXFLOW   0x0002 /**< Rx Control Flow Enable 	*/
#define OETH_CTRLMODER_TXFLOW   0x0004 /**< Tx Control Flow Enable 	*/


/*  MII Mode Register */
#define OETH_MIIMODER_CLKDIV    0x00FF /**< Clock Divider 	*/
#define OETH_MIIMODER_NOPRE     0x0100 /**< No Preamble */
#define OETH_MIIMODER_RST       0x0200 /**< MIIM Reset */

/*
 * MII Command Register
 */
#define OETH_MIICMD_SCANSTAT  	0x0001 /**< Scan Status */
#define OETH_MIICMD_RSTAT     	0x0002 /**< Read Status */
#define OETH_MIICMD_WCTRLDATA 	0x0004 /**< Write Control Data	*/

/*
 * MII Address Register
 */
#define OETH_MIIADDRESS_FIAD    0x001F /**< PHY Address */
#define OETH_MIIADDRESS_RGAD    0x1F00 /**< RGAD Address */

/*
 * MII Status Register
 */
#define OETH_MIISTATUS_LINKFAIL 0x0001 /**< Link Fail */
#define OETH_MIISTATUS_BUSY     0x0002 /**< MII Busy */
#define OETH_MIISTATUS_NVALID   0x0004 /**< MIIStatusReg data invalid	*/

/*
 * I2C Interface Register
 */
#define Z087_I2C_AVAIL_REV     (0x7)                /**< revision where I2C interface is available */
#define Z087_I2C_SCL            0x0001 /**< bitmask for SCL line */
#define Z087_I2C_SDA            0x0002 /**< bitmask for SDA line */

/* The current default KS8721 PHY address (on EM01) */
#define Z077_PHY_ADDRESS        1

/* Autoneg timeout in ms */
#define Z077_PHY_MAX_WAIT       5000

#define MII_100_BASE_TX_PHY     0x1f
/* power save bit */
#define MII_100BASE_PWR			(1<<10)

/*
 * Buffer Descriptor defines. ETH MAC can maintain up to 128 BDs in total.
 * (0x80) BD location = base + 0x400 - base + 0x7ff ( =0x400 )
 */

#define Z077_BDSIZE		0x8  	/**< size in byte of one BD	*/
#define Z077_TBD_NUM	0x40 	/**< default number of TBDs */
#define Z077_RBD_NUM	0x40	/**< Z077_TBD_NUM make both equal */
#define Z077_BDALIGN	(1<<10) /**< 10bit adress alignment in RAM */

/* resulting size of whole BD Area */
#define Z077_BD_SPACE	(Z077_BDSIZE*(Z077_TBD_NUM+Z077_RBD_NUM))


#define Z077_BD_OFFS_STAT	0x0	 /**< offset of status within  BD */
#define Z077_BD_OFFS_LEN	0x2	 /**< offset of length within  BD */
#define Z077_BD_OFFS_ADR	0x4	 /**< offset of Tx/Rx frame adr. within  BD */

/* Transmit Buffer Descriptor bit definitions */
#define Z077_TBD_RDY     0x8000	/**< Tx Ready    1: buffer is ready for Tx */
#define Z077_TBD_IRQ	 0x4000	/**< IRQreq enable  1: raise IRQ after Tx */
#define Z077_TBD_WRAP    0x2000	/**< Wrap indicate  1: Wrap bit, last buffer */
#define Z077_TBD_PAD     0x1000	/**< pad enable 1: pads short frames */
#define Z077_TBD_CRC     0x0800 /**< CRC enable 1: add CRC at packet end */

/**< bits 10..9: reserved  */
#define Z077_TBD_UR      0x0100 /**< Underrun: 	1: underrun occured */
#define Z077_TBD_RETRY   0x00f0	/**< [7:4]: # of retries till frame was sent */
#define Z077_TBD_RTRANS	 0x0008	/**< 1: retransm.limit reached */
#define Z077_TBD_LCOLL	 0x0004 /**< 1: late collision occured */
#define Z077_TBD_DEFER	 0x0002 /**< Deferred indication */
#define Z077_TBD_CSLOST	 0x0001 /**< Carrier sense lost while transmission	*/

/**
 *	Receive Buffer Descriptors bit definitions
 */
#define Z077_RBD_EMP     0x8000	/**< Rx buffer empty */
#define Z077_RBD_IRQ	 0x4000	/**< generate IRQ after recv */
#define Z077_RBD_WRAP    Z077_TBD_WRAP
/**< bits 12..9 : reserved */
#define Z077_RBD_CFRM   0x0100 /**< 1: got Control Frame 0: normal frame */
#define Z077_RBD_MISS   0x0080 /**< 1: frame accepted due to promisc mode */
#define Z077_RBD_OV		0x0040 /**< overrun occured */
#define Z077_RBD_IS		0x0020 /**< invalid symbol (5b4b) */
#define Z077_RBD_DN	 	0x0010 /**< dribble nibble (extra nibble received) 	*/
#define Z077_RBD_TL	 	0x0008 /**< too longm Frame longer then PACKETLEN */
#define Z077_RBD_SF	 	0x0004 /**< too short, Frame shorter then MINFL	*/
#define Z077_RBD_CERR 	0x0002 /**< CRC error occured	*/
#define Z077_RBD_LC 	0x0001 /**< late collision 	*/

/**< Cache macros */
#define END_CACHE_INVALIDATE(address, len) \
        CACHE_DRV_INVALIDATE (pDrvCtrl->pCacheFuncs, (address), (len))

#define END_CACHE_VIRT_TO_PHYS(address) \
        CACHE_DRV_VIRT_TO_PHYS (pDrvCtrl->pCacheFuncs, (address))

/**<
 * Default macro definitions for BSP interface.
 * These macros can be redefined in a wrapper file, to generate
 * a new module with an optimized interface.
 */
#if 0
/**< Macro to connect interrupt handler to vector */
#ifndef SYS_INT_CONNECT
FUNCPTR	z077IntConnectRtn = intConnect;
# define SYS_INT_CONNECT(pDrvCtrl,rtn,arg,pResult) \
	do { \
	*pResult = (*z077IntConnectRtn)((VOIDFUNCPTR *) \
			INUM_TO_IVEC (pDrvCtrl->ivec), rtn, (int)arg); \
	} while ((0))
#endif

/**< Macro to disconnect interrupt handler from vector */
#ifndef SYS_INT_DISCONNECT
STATUS	(*z077IntDisconnectRtn) \
(int level, FUNCPTR rtn, int arg) = NULL;
# define SYS_INT_DISCONNECT(pDrvCtrl,rtn,arg,pResult) \
	do { \
	if (z077IntDisconnectRtn != NULL) \
	    *pResult = (*z077IntDisconnectRtn)(pDrvCtrl->ivec, \
					rtn, (int)arg); \
	else \
	    *pResult = ERROR; \
	} while ((0))
#endif


/**< Macro to enable the appropriate interrupt level */
#ifndef SYS_INT_ENABLE
STATUS	(*z077IntEnableRtn)(int level) = NULL;
#   define SYS_INT_ENABLE(pDrvCtrl) \
	do { \
	    if (z077IntEnableRtn != NULL) \
		(*z077IntEnableRtn) (pDrvCtrl->ilevel); \
	} while ((0))
#endif

/**< Macro to get the ethernet address from the BSP */
#ifndef SYS_ENET_ADDR_GET
STATUS (*z077EnetAddrGetRtn)(int unit, char * pResult) = NULL;
#   define SYS_Z077_ADDR_GET(pDevice) \
	do { \
	if (z077EnetAddrGetRtn == NULL) \
	    bzero ((char *)&pDevice->enetAddr, 6); \
	else \
	    (*z077EnetAddrGetRtn)(pDevice->unit, \
					(char *)&pDevice->enetAddr); \
	} while ((0))
#endif

/*
 * Macros to do a short (UINT16) access to the chip. Default
 * assumes a normal memory mapped device.
 */

#ifndef Z077_OUT_SHORT
STATUS (*z077OutShortRtn)(UINT addr, UINT value) = NULL;
#   define Z077_OUT_SHORT(pDrvCtrl,addr,value) \
	do { \
	    if (z077OutShortRtn == NULL) \
		(*(USHORT *)addr = value) \
	    else \
		(*z077OutShortRtn)((UINT)addr, (UINT)value) \
	} while ((0))
#endif

#ifndef Z077_IN_SHORT
STATUS (*z077InShortRtn)(UINT addr, USHORT *pData) = NULL;
#   define Z077_IN_SHORT(pDrvCtrl,addr,pData) \
	do { \
	    if (z077InShortRtn == NULL) \
		(*pData = *addr) \
	    else \
		(*z077InShortRtn)((UINT)addr, pData) \
	} while ((0))
#endif
#endif


/* swap register definitions */

#ifdef Z077_BIG_ENDIAN
#define Z077_MEM_HANDLE     VXB_HANDLE_SWAP(VXB_HANDLE_ORDERED)
#else
#define Z077_MEM_HANDLE     VXB_HANDLE_ORDERED
#endif

#define Z77WRITE_D32(ma, offs,val)  \
    z077WriteD32 ((_WRS_IOLONG)(ma), (size_t)(offs), (UINT32)(val))

_WRS_INLINE void z077WriteD32 (_WRS_IOLONG addr, size_t offset, UINT32 val)
    {
    void * handle = (void *)Z077_MEM_HANDLE;

    vxbWrite32 (handle, (UINT32 *)(addr + offset), val);
    }

#define Z77WRITE_D16(ma,offs,val)   \
        z077WriteD16 ((_WRS_IOLONG)(ma), (size_t)(offs), (UINT16)(val))

_WRS_INLINE void z077WriteD16 (_WRS_IOLONG addr, size_t offset, UINT16 val)
    {
    void * handle = (void *)Z077_MEM_HANDLE;

    vxbWrite16 (handle, (UINT16 *)(addr + offset), val);
    }


#define Z77READ_D32(ma,offs)        \
    z077ReadD32 ((_WRS_IOLONG)(ma), (size_t)(offs))

_WRS_INLINE u_int32 z077ReadD32 (_WRS_IOLONG addr, size_t offset)
    {
    void * handle = (void *)Z077_MEM_HANDLE;

    return vxbRead32 (handle, (UINT32 *)(addr + offset));
    }

#define Z77READ_D16(ma,offs)        \
    z077ReadD16 ((_WRS_IOLONG)(ma), (size_t)(offs))

_WRS_INLINE u_int16 z077ReadD16 (_WRS_IOLONG addr, size_t offset)
    {
    void * handle = (void *)Z077_MEM_HANDLE;

    return vxbRead16 (handle, (UINT16 *)(addr + offset));
    }

#define Z87_MSETMASK_D32(ma,offs,mask)  \
    z087MsetMaskD32((_WRS_IOLONG)(ma), (size_t)(offs), (UINT32)(mask))

_WRS_INLINE void z087MsetMaskD32 (_WRS_IOLONG addr, size_t offset, UINT32 mask)
    {
    void * handle = (void *)Z077_MEM_HANDLE;
    UINT32 val;

    val = vxbRead32 (handle, (UINT32 *)(addr + offset));

    val |= mask;

    vxbWrite32 (handle, (UINT32 *)(addr + offset), val);
    }

#define Z87_MCLRMASK_D32(ma,offs,mask)  \
    z087MclrMaskD32((_WRS_IOLONG)(ma), (size_t)(offs), (UINT32)(mask))

_WRS_INLINE void z087MclrMaskD32 (_WRS_IOLONG addr, size_t offset, UINT32 mask)
    {
    void * handle = (void *)Z077_MEM_HANDLE;
    u_int32 val;

    val = vxbRead32 (handle, (UINT32 *)(addr + offset));

    val &= ~mask;

    vxbWrite32 (handle, (UINT32 *)(addr + offset), val);
    }

/*
 *  Shortcut for polling BUSY until its deasserted after MII accesses. Time-
 *  out occurs after Z077_PHY_ACC_TIMEOUT seconds
 */
#define Z077_WAIT_FOR_BUSY \
	{ \
		/* fails if we loaded  some 2^32 tick after boot... */ \
		u_int32 timestart=tickGet()/OSS_TickRateGet(NULL);	\
		while(Z77READ_D32(Z077_BASE,Z077_REG_MIISTATUS) & OETH_MIISTATUS_BUSY)\
        { \
			if (((tickGet()/OSS_TickRateGet(NULL))-timestart)> Z077_PHY_ACC_TIMEOUT){ \
				logMsg("*** men_sec: PHY access timeout!\n",0,0,0,0,0,0); \
		    	break; \
			} \
	    } \
    }

/* A shortcut for getting the hardware address from the MIB II stuff. */
#define END_HADDR(pEnd)	((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)
#define END_HADDR_LEN(pEnd) ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

#define Z077_FLAG_CLEAR(clearBits)	  (pDrvCtrl->flags &= ~(clearBits))
#define Z077_FLAG_SET(setBits)		  (pDrvCtrl->flags |= (setBits))
#define Z077_FLAG_GET()				  (pDrvCtrl->flags)
#define Z077_FLAG_ISSET(setBits)	  (pDrvCtrl->flags & (setBits))
#define Z077_PHY_FLAGS_SET(setBits)	 (pDrvCtrl->phyInfo->phyFlags |= (setBits))
#define Z077_PHY_FLAGS_ISSET(setBits) (pDrvCtrl->phyInfo->phyFlags & (setBits))
#define Z077_PHY_FLAGS_GET(setBits)	  (pDrvCtrl->phyInfo->phyFlags)
#define Z077_PHY_FLAGS_CLEAR(clearBits)	\
                 (pDrvCtrl->phyInfo->phyFlags &= ~(clearBits))

#define Z077_USR_FLAG_ISSET(setBits)  (pDrvCtrl->userFlags & (setBits))
#define END_FLAGS_ISSET(setBits)  	  ((&pDrvCtrl->endObj)->flags & (setBits))
#define Z077_ADDR_GET(pEnd)        ((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)
#define Z077_ADDR_LEN_GET(pEnd)    ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)


/*!
 * Macros to manipulate Rx- and Tx BD Entries
 *
 * These provide HW-Independent ways to set/get Address entries and Flag
 * settings. Detailled Explanation for Macro construction below:
 *
 * original ETH OpenCore:  BDs start at Z077_BD_OFFS (0x400) after ChamBase.
 *
 * Z077_BDBASE  = ChamBase + 0x400
 * Z077_BD_OFFS = 0x400
 * +--------------+
 * |			  |
 * |--------------|- Rx BD Start
 * |   	   	   	  |
 * |--------------|- TX Bd  Start (= Z077_BDBASE + Z077_BD_OFFS )
 * |XXXXXXXXXXXXXX|  Ctrl Regs
 * +--------------+- Chameleon Base Address
 *
 *
 * New Core: BDs reside in RAM, Address must be 10bit-aligned
 *
 * Z077_BDBASE  = pDrvCtrl->bdBase;
 * Z077_BD_OFFS = 0
 *
 * +--------------+
 * |			  |
 * |--------------|- Tx BD Start
 * |   	   	   	  |
 * +--------------+- Rx Bd  Start (= Z077_BDBASE + Z077_BD_OFFS )
 *
 */
#define TBD_OFF			(pDrvCtrl->tbdOff )
#define RBD_OFF  		(pDrvCtrl->rbdOff )
#define Z077_BD_OFFS	(pDrvCtrl->bdOff  )

/**< Set Adress <adr> for ETH Frame in Tx BD <n> */

#define Z077_SET_TBD_ADDR(n,adr) \
		Z77WRITE_D32(Z077_BDBASE, \
        Z077_BD_OFFS+(((n)+TBD_OFF) * Z077_BDSIZE) + Z077_BD_OFFS_ADR, (adr))

/**< Set Adress <adr> for ETH Frames in Rx BD <n> */
#define Z077_SET_RBD_ADDR(n,adr) \
		Z77WRITE_D32(Z077_BDBASE, \
        Z077_BD_OFFS+( ((n)+RBD_OFF) * Z077_BDSIZE)+ Z077_BD_OFFS_ADR, adr)

/**< Get ETH Frame storage Adresses in Tx BD <n> */
#define Z077_GET_TBD_ADDR(n) \
		Z77READ_D32(Z077_BDBASE, Z077_BD_OFFS + \
        ( ((n)+TBD_OFF) * Z077_BDSIZE)+Z077_BD_OFFS_ADR)

/**< Get ETH Frame storage Adresses in Rx BD <n> */
#define Z077_GET_RBD_ADDR(n) \
		Z77READ_D32(Z077_BDBASE, Z077_BD_OFFS + \
        ( ((n)+RBD_OFF) * Z077_BDSIZE) + Z077_BD_OFFS_ADR)

/**< Get a Flag <f> from Tx BD <n> */
#define Z077_GET_TBD_FLAG(n,f) \
       (Z77READ_D16(Z077_BDBASE, Z077_BD_OFFS+( ((n)+TBD_OFF)*Z077_BDSIZE)) & \
       (f) )

/**< Get a Flag <f> from Rx BD <n> */
#define Z077_GET_RBD_FLAG(n,f) \
       (Z77READ_D16(Z077_BDBASE, Z077_BD_OFFS+( ((n)+RBD_OFF)*Z077_BDSIZE)) & \
       (f) )

/**< Set a Flag <f> in Tx BD <n> */
#define Z077_SET_TBD_FLAG(n,f) \
		Z77WRITE_D16(Z077_BDBASE, Z077_BD_OFFS+( ((n)+TBD_OFF)*Z077_BDSIZE), \
        Z77READ_D16( Z077_BDBASE, Z077_BD_OFFS+( ((n)+TBD_OFF)*Z077_BDSIZE))| \
        (f) )

/**< Set a Flag <f> in Rx BD <n> */
#define Z077_SET_RBD_FLAG(n,f) \
	Z77WRITE_D16(Z077_BDBASE, Z077_BD_OFFS + ((n)+RBD_OFF) * Z077_BDSIZE, \
    Z77READ_D16( Z077_BDBASE, Z077_BD_OFFS + ((n)+RBD_OFF) * Z077_BDSIZE)| \
    (f) )

/**< Clear a Flag <f> in Rx BD <n> */
#define Z077_CLR_RBD_FLAG(n,f) \
	Z77WRITE_D16(Z077_BDBASE, Z077_BD_OFFS + ((n)+RBD_OFF) * Z077_BDSIZE, \
    Z77READ_D16( Z077_BDBASE, Z077_BD_OFFS + ((n)+RBD_OFF) * Z077_BDSIZE) & \
    ~(f) )

/**< Clear a Flag <f> in Tx BD <n> */
#define Z077_CLR_TBD_FLAG(n,f) \
	Z77WRITE_D16(Z077_BDBASE, Z077_BD_OFFS+ (((n)+TBD_OFF) * Z077_BDSIZE), \
    Z77READ_D16( Z077_BDBASE, Z077_BD_OFFS+ (((n)+TBD_OFF) * Z077_BDSIZE)) & \
		 ~(f) )

/**< set ETH Frame length for Tx BD nr. <n> */
#define Z077_SET_TBD_LEN(n,l) Z77WRITE_D16(Z077_BDBASE, \
        Z077_BD_OFFS + Z077_BD_OFFS_LEN+( ((n)+TBD_OFF) * Z077_BDSIZE), (l));

/**< Get ETH Frame length for Tx BD nr. <n> */
#define Z077_GET_TBD_LEN(n) Z77READ_D16(Z077_BDBASE, \
        Z077_BD_OFFS + Z077_BD_OFFS_LEN+( ((n)+TBD_OFF) * Z077_BDSIZE))


/**< Get the ETH Frame length stored in Rx BD nr. <n> */
#define Z077_GET_RBD_LEN(n)  Z77READ_D16(Z077_BDBASE, \
	    Z077_BD_OFFS+( ((n)+RBD_OFF) * Z077_BDSIZE) + Z077_BD_OFFS_LEN )

/**< clear a Rx BDs len entry after packet is processed */
#define Z077_CLR_RBD_LEN(n) \
	    Z77WRITE_D16(Z077_BDBASE, \
		Z077_BD_OFFS+( ((n)+RBD_OFF) * Z077_BDSIZE)+Z077_BD_OFFS_LEN, 0 );


#define Z077_DISABLE_IRQ(n)	\
        Z77WRITE_D32(Z077_BASE,		Z077_REG_INT_MASK, \
         (Z77READ_D32(Z077_BASE,	Z077_REG_INT_MASK)) & ~(n) )

#define Z077_ENABLE_IRQ(n) \
        Z77WRITE_D32(Z077_BASE, 	Z077_REG_INT_MASK, \
            Z77READ_D32(Z077_BASE, 	Z077_REG_INT_MASK)|(n))

/**< set/clear bits in MODE Register */
#define Z077_CLR_MODE_FLAG(f) \
		Z77WRITE_D32(Z077_BASE, Z077_REG_MODER, \
        Z77READ_D32(Z077_BASE, 	Z077_REG_MODER) &~(f))

#define Z077_SET_MODE_FLAG(f) \
		Z77WRITE_D32(Z077_BASE, Z077_REG_MODER, \
        Z77READ_D32(Z077_BASE, 	Z077_REG_MODER)|(f))

/**< return non-zero if the Rx BD <n> is empty */
#define Z077_RBD_EMPTY(n) Z077_GET_RBD_FLAG((n), OETH_RX_BD_EMPTY)

typedef struct
{
    int len;
    char * pData;	/**< data virtual address */
    char * pPhys;	/**< dma physical address */
} PKT;	/**< A dummy DMA data packet */

typedef struct rfd
{
    PKT *  pPkt;
    struct rfd * next;
} RFD;	/**< dummy rx frame descriptor */

typedef struct free_args
{
    void* arg1;
    void* arg2;
} FREE_ARGS;

/**< more comfortable dumping of Registers */
typedef struct z077_reg_info
{
	const char *name;
	UINT32		addr;
} Z077_REG_INFO;

/**< struct to Hold Phy names and identifiers */
typedef struct phy_device_tbl
{
	UINT32		ident1;
	UINT32		ident2;
	const char *name;
} PHY_DEVICE_TBL;

/**
 *	Buffer Descriptors, located between z77_base+0x400 and z77_base+0x800
 */
typedef struct z077_bd {
    UINT16 	BdLen;		/**< # of bytes [to transmit,received] 	*/
    UINT16 	BdStat;		/**< the 16bit Statusword					*/
    UINT32 	BdAddr;		/**< pointer to buffer 					*/
	BOOL	Done;		/**< TRUE: this BD was passed to stack 	*/
} Z077_BD;


/** This structure describes PCI bus scan interface.
 */
typedef struct {/* Z25_PCI_SCAN_TS */
    u_int32     vendorId;           /**< vendor ID */
    u_int32     deviceId;           /**< device ID */
    u_int32     domainNo;           /**< PCI ctrl. number (for CPUs with > 1 PCI controller) */
    u_int32     busNo;              /**< PCI bus number */
    u_int32     deviceNo;           /**< PCI device number */
    u_int32     funcNo;             /**< PCI function number */
} Z077_PCI_DESC;

/**
 * The definition of the Main driver control structure
 */

/**< int z077ClDescTblNumEnt = (NELEMENTS(z077ClDescTbl)); */
#define Z077_CLDESC_TBL_NUM      8

typedef struct end_device
{
    END_OBJ   	endObj;			/**< The class we inherit from. 			*/
    int		  	unit;			/**< unit number 							*/
    Z077_MACLOADFUNC getMACFunc;/**< callback to get MAC address 			*/
    UINT8       usePciIrq;      /**< use PCI interrupts or Chameleon interrupts */
    UINT8       iOffset;        /**< interrupt offset 						*/
    int         ivec;           /**< interrupt vector (from cham tbl)		*/

    long	  	flags;			/**< Our local flags. 					*/
    UCHAR	  	enetAddr[6+2];	/**< MAC addr (padded to 8 byte)		*/
	UINT32	  	baseAddr;		/**< base addr from cham. unit Z077/87	*/

	/* maintain Buffer Descriptors in RAM */
    int         initType;    	/**< netPoolInit() or netPoolCreate() 	*/
    char       	*pBufAlloc;   	/**< Allocated TSEC memory pool base 	*/
    char      	*pBufBase;      /**< Rounded TSEC memory pool base 		*/
    UINT32      bufSize;        /**< TSEC memory pool size 				*/
    UINT32      tbdNum;         /**< number of TX bd's 					*/
    UINT32      rbdNum;         /**< number of TX bd's 					*/
    Z077_BD     *pBdAlloc;      /**< TSEC BDs Alloc pointer 			*/
    Z077_BD     *pBdBase;       /**< TSEC BDs base 						*/
    UINT32      bdSize;         /**< TSEC BDs size 						*/
	UINT32 		txBlocked;		/**< Tx Stall (busy) occured ? 			*/

	UINT32		bdBase;			/**< Base Address of all BDs in RAM 	*/
	UINT32 		tbdOff;
	UINT32 		rbdOff;
	UINT32		bdOff;
	Z077_PCI_DESC pciDesc;
	UINT32 		lruRxBD;		/**< last recently used Rx empty register */
	UINT32		nCurrTbd;		/**< currently used Tx BD 				*/
	UINT32		nCurrRbd;		/**< currently used Rx BD	 			*/

	M_CL_CONFIG z077MclBlkConfig;/**< network mbuf config table			*/

	/* elements needed for the new netPoolCreate() API call 			*/
    CL_POOL_ID   pClPoolId;      /**< cluster pool identifier 			*/
	M_BLK_ID 	*pMblkList;		 /**< allocated clusters 				*/
    M_BLK_ID   	*tBufList;       /**< allocated clusters 				*/
	M_BLK_ID   	*tSendList; 	 /**< stores pMblks from send()			*/
	NETBUF_CFG	*pNetBufCfg;
    UINT32       maxRxFrames; 	 /**< max frames to Receive in one job	*/

	/* store assigned buffers & lengths */
	Z077_BD		txBd[Z077_TBD_NUM];
	Z077_BD		rxBd[Z077_RBD_NUM];
	UINT32		lastRxPosition;

    UINT32      nrRxcIrq;	/**< received control Frame 			*/
    UINT32      nrTxcIrq;	/**< transmitted control frame  		*/
    UINT32      nrBusyIrq;	/**< Busy interrupts			 		*/
    UINT32      nrRxeIrq;	/**< Receive error IRQs					*/
    UINT32      nrRxbIrq;	/**< Receive Frame IRQs (main IRQ) 		*/
    UINT32      nrTxeIrq;	/**< transmit error occured IRQ 		*/
    UINT32      nrTxbIrq;	/**< transmitted frame IRQ (main IRQ)	*/

    UINT32     	errOverrun;	/**< nr. of overruns occured 			*/
    UINT32     	errInvsymb;	/**< nr. of invalid symbols occured 	*/
    UINT32     	errDribble;
    UINT32     	errToolong;
    UINT32     	errTooshort;
    UINT32     	errCrcerr;
    UINT32     	errLatecoll;

#ifdef USE_NET_JOB_ADD
    FUNCPTR     netJobAdd;
#endif

    /* network cluster pool config table */
    CL_DESC         z077ClDescTbl[Z077_CLDESC_TBL_NUM];
    PHY_INFO        *phyInfo;       /**< info of PHY KS8721 		*/
    CACHE_FUNCS*    pCacheFuncs;    /**< cache function pointers 	*/
    FUNCPTR         freeRtn[128];   /**< Array of free routines. 	*/
    struct free_args freeData[128]; /**< Array of free arguments 	*/

    UINT32     	errRxNoMem;			 /**< nr. of no OS memory buffer */

    int RunningRxJobs;				 /**< RxJob counter to avoid queue overflow	        */
    int RunningRxErrJobs;			 /**< RxErrJob counter to avoid queue overflow  	*/
    int RunningTxErrJobs;			 /**< TxErrJob counter to avoid queue overflow	    */
    int RunningRestartJobs;			 /**< RestartJob counter to avoid queue overflow	*/

    int newRxIrqBehaviour;			/**< flag which IRQ behaviour to be used */
    UINT32 chamUnitRevision;       	/**< IP core revision */
	UINT32        z077IntMask;		/**< Interrupt mask for polling mode */
} Z077_END_DEVICE;

UINT32  z077ModInit( char * );
UINT32  z077Init(Z77_CONF_SET *);
STATUS  z077SetIntFunctions(FUNCPTR , FUNCPTR );
STATUS  z077FindZxCore(INT8 *, INT16, CHAMELEONV2_HANDLE ** , CHAMELEONV2_UNIT * );
STATUS  z077CheckChameleonDevice(Z077_PCI_DESC *);
STATUS  z077InitCham(Z077_PCI_DESC * , CHAMELEONV2_HANDLE **);

IMPORT STATUS sysZ077MACAddrGet ( UINT32 unitNum, Z077_MAC_ADDR addr );
/* IMPORT STATUS sysZ077MACAddrSet ( UINT32 unitNum, Z077_MAC_ADDR addr ); */
IMPORT STATUS sysZ077ChameleonInfoGet(int , Z077_END_DEVICE* );
IMPORT STATUS sysZ077EnetDisable(UINT32 );

/**
 * this table may be customized by the user in configNet.h
 */
IMPORT INT16 z077PhyAnOrderTbl[];

#endif /* #ifndef _Z077END_H_ */


