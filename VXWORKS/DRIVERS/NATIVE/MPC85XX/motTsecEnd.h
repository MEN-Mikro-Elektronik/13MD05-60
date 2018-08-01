
/* MotTsecEnd.h - Motorola TSEC Ethernet network interface.*/

/* Copyright 2003-2004 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,06mar06,dlk  Initialize the RXIC register.
01o,26jun05,dlk  Replace netJobAdd() with jobQueuePost().
01j,24mar05,dtr  Changes from code review.
01i,31jan05,dtr  Decouple from interrupt controller.
01h,23nov04,rcs  Fixed SPR 104568 - fails large pings when attached to a
                 gigabit switch
01g,22oct04,dtr  Added Interrupt coalescing defines. Changed some defaults
                 for increased performance.SPR 102536
01f,22jul04,rcs  Added PHY Access definitions
01e,21jun04,mil  Changed cacheArchXXX funcs to cacheXXX funcs.
01d,27apr04,mdo  Latest updates for B6 build
01c,13apr04,mil  Fixed location of header file.
01c,26mar04,rcs  Fixed SPR 95432
01b,12feb04,rcs  Adjusted default settings
01b,04feb04,mil  Fixed problem if compiled with GNU.
01a,10mar03,gjc  Motorola TSEC Ethernet.
*/


#ifndef __INCmotTsecEndh
#define __INCmotTsecEndh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* This include/define causes error on vxW5.5 build _WITHOUT_ PID.*/
#if defined(VX_VERSION_5_5_PID) || (_WRS_VXWORKS_MAJOR >= 6)
# include "net/ethernet.h"
# define ether_addr_octet octet
#endif
#include "etherLib.h"
#include "miiLib.h"

#define _WRS_VXWORKS_VNUM \
    ((_WRS_VXWORKS_MAJOR << 16)|(_WRS_VXWORKS_MINOR << 8)|(_WRS_VXWORKS_MAINT))

#if _WRS_VXWORKS_VNUM >= 0x060100

	#undef USE_NET_JOB_ADD

	#ifdef USE_NET_JOB_ADD
		#undef netJobAdd /* for the cases when we store it in a pointer */
	#endif

  #if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR >= 6 ) )
	#define NET_JOB_ADD(f, a1, a2, a3, a4, a5)			    \
  	  (jobQueueStdPost (netJobQueueId, NET_TASK_QJOB_PRI,		    \
			      (VOIDFUNCPTR)(f), (void *)(a1), (void *)(a2), \
			      (void *)(a3), (void *)(a4), (void *)(a5)))
  #else /* VxW >=6.6 */
	#define NET_JOB_ADD(f, a1, a2, a3, a4, a5)			    \
  	  (jobQueueStdPost (&netJobInfo, NET_TASK_QJOB_PRI,		    \
			      (VOIDFUNCPTR)(f), (void *)(a1), (void *)(a2), \
			      (void *)(a3), (void *)(a4), (void *)(a5)))
  #endif /* VxW >= 6.6 */
#else
	#define USE_NET_JOB_ADD
	#define NET_JOB_ADD(f, a1, a2, a3, a4, a5)			    \
    	(netJobAdd ((f), (a1), (a2), (a3), (a4), (a5)))
#endif


typedef struct ether_addr ENET_ADDR;

#define MOT_TSEC_DEV_NAME        "mottsec"
#define MOT_TSEC_DEV_NAME_LEN    8
#define MOT_TSEC_MAX_DEVS        4
#define MOT_TSEC_DEV_1           0
#define MOT_TSEC_DEV_2           1
#define MOT_TSEC_DEV_3           2 /* 8540: FEC ? */
#define MOT_TSEC_DEV_4           3
#define MOT_TSEC_ADRS_OFFSET_1  0x00024000
#define MOT_TSEC_ADRS_OFFSET_2  0x00025000
#define MOT_TSEC_ADRS_OFFSET_3  0x00026000 /* 8540: FEC ? */
#define MOT_TSEC_ADRS_OFFSET_4  0x00027000

/* IEVENT and IMASK Register definitions */
#define MOT_TSEC_IEVENT_BABR    0x80000000
#define MOT_TSEC_IEVENT_RXC     0x40000000
#define MOT_TSEC_IEVENT_BSY     0x20000000
#define MOT_TSEC_IEVENT_EBERR   0x10000000
#define MOT_TSEC_IEVENT_MSRO    0x04000000
#define MOT_TSEC_IEVENT_GTSC    0x02000000
#define MOT_TSEC_IEVENT_BABT    0x01000000
#define MOT_TSEC_IEVENT_TXC     0x00800000
#define MOT_TSEC_IEVENT_TXE     0x00400000
#define MOT_TSEC_IEVENT_TXB     0x00200000
#define MOT_TSEC_IEVENT_TXF     0x00100000
#define MOT_TSEC_IEVENT_LC      0x00040000
#define MOT_TSEC_IEVENT_CRL     0x00020000
#define MOT_TSEC_IEVENT_XFUN    0x00010000
#define MOT_TSEC_IEVENT_RXB0    0x00008000
#define MOT_TSEC_IEVENT_GRSC    0x00000100
#define MOT_TSEC_IEVENT_RXF0    0x00000080

/* Error Disable Registers */
#define MOT_TSEC_EDIS_BSYDIS    0x20000000
#define MOT_TSEC_EDIS_EBERRDIS  0x10000000
#define MOT_TSEC_EDIS_TXEDIS    0x00400000
#define MOT_TSEC_EDIS_LCDIS     0x00040000
#define MOT_TSEC_EDIS_CRLDIS    0x00020000
#define MOT_TSEC_EDIS_XFUNDIS   0x00010000

/* Ethernet Control Register */
#define MOT_TSEC_ECNTRL_CLRCNT  0x00004000
#define MOT_TSEC_ECNTRL_AUTOZ   0x00002000
#define MOT_TSEC_ECNTRL_STEN    0x00001000
#define MOT_TSEC_ECNTRL_TBIM    0x00000020
#define MOT_TSEC_ECNTRL_RPM     0x00000010
#define MOT_TSEC_ECNTRL_R100M   0x00000008

/* Minimum Frame Register Length */
#define MOT_TSEC_MINFLR(l)      (l&0x0000007f)

/* PTV Register Definition */
#define MOT_TSEC_PTV_PTE(t)     (t<<16)
#define MOT_TSEC_PTV_PT(t)      (t)

/* DMA Control Register */
#define MOT_TSEC_DMACTRL_TDSEN  0x00000080
#define MOT_TSEC_DMACTRL_TBDSEN 0x00000040
#define MOT_TSEC_DMACTRL_GRS    0x00000010
#define MOT_TSEC_DMACTRL_GTS    0x00000008
#define MOT_TSEC_DMACTRL_TOD    0x00000004
#define MOT_TSEC_DMACTRL_WWR    0x00000002
#define MOT_TSEC_DMACTRL_WOP    0x00000001

/* TBI Physical Address Registers */
#define MOT_TSEC_TBIPA(a)           (a&0x0000001f)

/* FIFO Transmit Threshold Registers */
#define MOT_TSEC_FIFO_TX_THR(a)     (a&0x000001ff)

/* FIFO Transmit Starve Registers */
#define MOT_TSEC_FIFO_TX_STARVE(a)  (a&0x000001ff)

/* FIFO Transmit Starve Shutoff Registers */
#define MOT_TSEC_FIFO_TX_STARVE_SHUTOFF(a) (a&0x000001ff)

/* Transmit Control Register */
#define MOT_TSEC_TCTRL_THDF         0x00000800
#define MOT_TSEC_TCTRL_RFC_PAUSE    0x00000010
#define MOT_TSEC_TCTRL_TFC_PAUSE    0x00000008

#define MOT_TSEC_TSTAT_THLT     0x80000000

/* Transmit Interrupt Coalescing */
#define MOT_TSEC_IC_ICEN	0x80000000
#define MOT_TSEC_IC_ICFCT(a)  ((a&0x000000ff)<<21)
#define MOT_TSEC_IC_ICTT(a)   (a&0x0000ffff)

#define MOT_TSEC_OSTBD_R        0x80000000
#define MOT_TSEC_OSTBD_PAD      0x40000000
#define MOT_TSEC_OSTBD_W        0x20000000
#define MOT_TSEC_OSTBD_I        0x10000000
#define MOT_TSEC_OSTBD_L        0x08000000
#define MOT_TSEC_OSTBD_TC       0x04000000
#define MOT_TSEC_OSTBD_DEF      0x02000000
#define MOT_TSEC_OSTBD_LC       0x00800000
#define MOT_TSEC_OSTBD_RL       0x00400000
#define MOT_TSEC_OSTBD_RC(r)    ((r&0x0000003c)>>18)
#define MOT_TSEC_OSTBD_UN       0x00020000
#define MOT_TSEC_OSTBD_LEN(l)   (l&0x0000ffff)

#define MOT_TSEC_RCTRL_BC_REJ   0x00000010
#define MOT_TSEC_RCTRL_PROM     0x00000008
#define MOT_TSEC_RCTRL_RSF      0x00000004

#define MOT_TSEC_RSTAT_QHLT     0x00800000
#define MOT_TSEC_TDLEN(l)       (l&0x0000ffff)
#define MOT_TSEC_RBDLEN(l)      (l&0x0000ffff)
#define MOT_TSEC_CRBPTR(p)      (p&0xfffffff8)
#define MOT_TSEC_MRBLR(l)       (l&0x0000ffc0)
#define MOT_TSEC_RBASE(p)       (p&0xfffffff8)

#define MOT_TSEC_MACCFG1_SOFT_RESET     0x80000000
#define MOT_TSEC_MACCFG1_RESET_RX_MC    0x00080000
#define MOT_TSEC_MACCFG1_RESET_TX_MC    0x00040000
#define MOT_TSEC_MACCFG1_RESET_RX_FUN   0x00020000
#define MOT_TSEC_MACCFG1_RESET_TX_FUN   0x00010000
#define MOT_TSEC_MACCFG1_LOOPBACK       0x00000100
#define MOT_TSEC_MACCFG1_RX_FLOW        0x00000020
#define MOT_TSEC_MACCFG1_TX_FLOW        0x00000010
#define MOT_TSEC_MACCFG1_SYNCD_RX_EN    0x00000008
#define MOT_TSEC_MACCFG1_RX_EN          0x00000004
#define MOT_TSEC_MACCFG1_SYNCD_TX_EN    0x00000002
#define MOT_TSEC_MACCFG1_TX_EN          0x00000001

#define MOT_TSEC_MACCFG2_PRE_LEN(l)         ((l<<12) & 0xf000)
#define MOT_TSEC_MACCFG2_PRE_LEN_GET(r)     ((r&0xf000)>>12)
#define MOT_TSEC_MACCFG2_IF_MODE(m)         ((m<<8) & 0x0300)
#define MOT_TSEC_MACCFG2_IF_MODE_GET(r)     ((r&0x0300)>>8)

#define MOT_TSEC_MACCFG2_IF_MODE_MASK       0x00000003
#define MOT_TSEC_MACCFG2_IF_MODE_MII        0x00000001
#define MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI   0x00000002
#define MOT_TSEC_MACCFG2_HUGE_FRAME         0x00000020
#define MOT_TSEC_MACCFG2_LENGTH_CHECK       0x00000010
#define MOT_TSEC_MACCFG2_PADCRC             0x00000004
#define MOT_TSEC_MACCFG2_CRC_EN             0x00000002
#define MOT_TSEC_MACCFG2_FULL_DUPLEX        0x00000001

#define MOT_TSEC_IPGIFG_NBBIPG1(l)      ((l&0x0000007f)<<24)
#define MOT_TSEC_IPGIFG_NBBIPG2(l)      ( (l&0x0000007f)<<16)
#define MOT_TSEC_IPGIFG_MIFGE(l)        ((l&0x000000ff)<<8)
#define MOT_TSEC_IPGIFG_BBIPG(l)        (l&0x0000007f)

#define MOT_TSEC_HALDUP_ALTBEB_TRUNC(l) ((l&0x0000000f)<<20)
#define MOT_TSEC_HALFDUP_BEB            0x00080000
#define MOT_TSEC_HALFDUP_BPNBO          0x00040000
#define MOT_TSEC_HALFDUP_NBO            0x00020000
#define MOT_TSEC_HALFDUP_EXCESS_DEF     0x00010000
#define MOT_TSEC_HALDUP_RETRY(v)        ((v&0x0000000F)<<12)
#define MOT_TSEC_HALDUP_COL_WINDOW(w)   (w&0x003f)

#define MOT_TSEC_MAXFRM(l)              (l&0x0000ffff)
#define MOT_TSEC_MIIMCFG_RESET          0x80000000
#define MOT_TSEC_MIIMCFG_NO_PRE         0x00000010
#define MOT_TSEC_MIIMCFG_MCS(l)         (l&0x00000007)
#define MOT_TSEC_MIIMCFG_MCS_2          0x00000000
#define MOT_TSEC_MIIMCFG_MCS_4          0x00000001
#define MOT_TSEC_MIIMCFG_MCS_6          0x00000002
#define MOT_TSEC_MIIMCFG_MCS_8          0x00000003
#define MOT_TSEC_MIIMCFG_MCS_10         0x00000004
#define MOT_TSEC_MIIMCFG_MCS_14         0x00000005
#define MOT_TSEC_MIIMCFG_MCS_20         0x00000006
#define MOT_TSEC_MIIMCFG_MCS_28         0x00000007

#define MOT_TSEC_MIIMCOM_SCAN_CYCLE     0x00000002
#define MOT_TSEC_MIIMCOM_READ_CYCLE     0x00000001

#define MOT_TSEC_MIIMADD_PHYADRS(a)     ((a&0x0000001f)<<8)
#define MOT_TSEC_MIIMADD_REGADRS(a)     (a&0x0000001f)

#define MOT_TSEC_MIIMCON_PHY_CTRL(a)    (a&0x0000ffff)

#define MOT_TSEC_MIIMSTAT_PHY(a)        (a&0x0000ffff)

#define MOT_TSEC_MIIMIND_NOT_VALID      0x00000004
#define MOT_TSEC_MIIMIND_SCAN           0x00000002
#define MOT_TSEC_MIIMIND_BUSY           0x00000001

#define MOT_TSEC_IFSTAT_EXCESS_DEF      0x00000200

#define MOT_TSEC_MACSTNADDR1_SA_1       0xff000000
#define MOT_TSEC_MACSTNADDR1_SA_2       0x00ff0000
#define MOT_TSEC_MACSTNADDR1_SA_3       0x0000ff00
#define MOT_TSEC_MACSTNADDR1_SA_4       0x000000ff
#define MOT_TSEC_MACSTNADDR2_SA_5       0xff000000
#define MOT_TSEC_MACSTNADDR2_SA_6       0x00ff0000

/* Transmit Buffer Descriptor bit definitions */
#define MOT_TSEC_TBD_R          0x8000
#define MOT_TSEC_TBD_PADCRC     0x4000
#define MOT_TSEC_TBD_W          0x2000
#define MOT_TSEC_TBD_I          0x1000
#define MOT_TSEC_TBD_L          0x0800
#define MOT_TSEC_TBD_TC         0x0400
#define MOT_TSEC_TBD_DEF        0x0200
#define MOT_TSEC_TBD_HFE_LC     0x0080
#define MOT_TSEC_TBD_HFE_RL     0x0040
#define MOT_TSEC_TBD_HFE_RC     0x003c
#define MOT_TSEC_TBD_HFE_UN     0x0002

/* Receive Buffer Descriptors bit definitions */
#define MOT_TSEC_RBD_E          0x8000
#define MOT_TSEC_RBD_RO1        0x4000
#define MOT_TSEC_RBD_W          0x2000
#define MOT_TSEC_RBD_I          0x1000
#define MOT_TSEC_RBD_L          0x0800
#define MOT_TSEC_RBD_F          0x0400
#define MOT_TSEC_RBD_M          0x0200
#define MOT_TSEC_RBD_BC         0x0080
#define MOT_TSEC_RBD_MC         0x0040
#define MOT_TSEC_RBD_LG         0x0020
#define MOT_TSEC_RBD_NO         0x0010
#define MOT_TSEC_RBD_SH         0x0008
#define MOT_TSEC_RBD_CR         0x0004
#define MOT_TSEC_RBD_OV         0x0002
#define MOT_TSEC_RBD_TR         0x0001

#define MOT_TSEC_RBD_ERR  (MOT_TSEC_RBD_TR | MOT_TSEC_RBD_OV |  \
                           MOT_TSEC_RBD_CR | MOT_TSEC_RBD_SH |  \
                           MOT_TSEC_RBD_NO | MOT_TSEC_RBD_LG)


#define MOT_TSEC_ATTR_ELCWT_NA       0x0
#define MOT_TSEC_ATTR_ELCWT_L2       0x4
#define MOT_TSEC_ATTR_ELCWT_L2_LOCK  0x6
#define MOT_TSEC_ATTR_ELCWT(v)       ((v&0x00000003)<<13)

#define MOT_TSEC_ATTR_BDLWT_NA       0x0
#define MOT_TSEC_ATTR_BDLWT_L2       0x2
#define MOT_TSEC_ATTR_BDLWT_L2_LOCK  0x3
#define MOT_TSEC_ATTR_BDLWT(v)       ((v&0x00000003)<<10)

#define MOT_TSEC_ATTR_RDSEN          0x00000080
#define MOT_TSEC_ATTR_RBDSEN         0x00000040

#define MOT_TSEC_ATTRELI_EL(v)       ((v&0x00003fff)<<16)
#define MOT_TSEC_ATTRELI_EI(v)       (v&0x00003fff)

/* TSEC init string parameters */

/* "MMBASE:TSEC_PORT:MAC_ADRS:PHY_DEF_MODES:USER_FLAGS:FUNC_TABLE:EXT_PARMS"  */
/*        MMBASE - 85xx local base address. Used as base for driver memory space.
 *      MAC_ADRS - Mac address in 12 digit format eg. 00-A0-1E-11-22-33
 *  PHY_DEF_MODE - Default Attributes passed to the MII driver.
 * USER_DEF_MODE - Mandatory initialization user parameters.
 *    FUNC_TABLE - Table of BSP and Driver callbacks
 *         PARMS - Address of a structure that contains required initialization
 *                 parameters and driver specific tuning parameters.
 *     EXT_PARMS - Address of a structure that contains optional initialization
 *                 parameters and driver specific tuning parameters.
 */

/* MII/PHY PHY_DEF_MODE flags to init the phy driver */
#define MOT_TSEC_USR_MODE_DEFAULT         0
#define MOT_TSEC_USR_PHY_NO_AN   0x00000001	/* do not auto-negotiate */
#define MOT_TSEC_USR_PHY_TBL     0x00000002	/* use negotiation table */
#define MOT_TSEC_USR_PHY_NO_FD   0x00000004	/* do not use full duplex */
#define MOT_TSEC_USR_PHY_NO_100  0x00000008	/* do not use 100Mbit speed */
#define MOT_TSEC_USR_PHY_NO_HD   0x00000010	/* do not use half duplex */
#define MOT_TSEC_USR_PHY_NO_10   0x00000020	/* do not use 10Mbit speed */
#define MOT_TSEC_USR_PHY_MON     0x00000040	/* use PHY Monitor */
#define MOT_TSEC_USR_PHY_ISO     0x00000080	/* isolate a PHY */


/* ECNTRL Ethernet Control */
#define MOT_TSEC_USR_STAT_CLEAR     0x00000100	/* init + runtime clear mstats */
#define MOT_TSEC_USR_STAT_AUTOZ     0x00000200	/* init */
#define MOT_TSEC_USR_STAT_ENABLE    0x00000400	/* init */

/* PHY bus configuration selections */
#define MOT_TSEC_USR_MODE_MASK      0x0003f800
#define MOT_TSEC_USR_MODE_TBI       0x00000800
#define MOT_TSEC_USR_MODE_RTBI      0x00001000
#define MOT_TSEC_USR_MODE_MII       0x00002000
#define MOT_TSEC_USR_MODE_GMII      0x00004000
#define MOT_TSEC_USR_MODE_RGMII     0x00008000
#define MOT_TSEC_USR_MODE_RGMII_10  0x00010000
#define MOT_TSEC_USR_MODE_RGMII_100 0x00020000

/* TSEC extended initialization parameters */

/* Bit flags */

/* DMACTRL - Configure the DMA block */
#define MOT_TSEC_TX_SNOOP_EN          0x00000001	/* snoop Tx Clusters */
#define MOT_TSEC_TX_BD_SNOOP_EN       0x00000002	/* snoop txbds */
#define MOT_TSEC_TX_WWR               0x00000004	/* init */
#define MOT_TSEC_TXBD_WOP             0x00000008	/* init */

/* RCTRL - Receive Control flags */
#define MOT_TSEC_BROADCAST_REJECT     0x00000010	/* Broadcast Reject */
#define MOT_TSEC_PROMISCUOUS_MODE     0x00000020	/* Promiscuous Mode */
#define MOT_TSEC_RX_SHORT_FRAME       0x00000040	/* Rx Shorter Frames */
	/* if (frame<MINFLR) */

/* MACCFG1 - Mac Configuration */
#define MOT_TSEC_MAC_LOOPBACK         0x00000080	/* init + runtime */
#define MOT_TSEC_MAC_RX_FLOW          0x00000100	/* enable Rx Flow */
#define MOT_TSEC_MAC_TX_FLOW          0x00000200	/* enable Tx Flow */

/* MACCFG2 Mac Configuration */
#define MOT_TSEC_MAC_HUGE_FRAME       0x00000400	/* enable huge frame support */
#define MOT_TSEC_MAC_PADCRC           0x00000800	/* MAC pads short frames */
	/* and appends CRC */
#define MOT_TSEC_MAC_CRC_ENABLE       0x00001000	/* MAC appends CRC */
#define MOT_TSEC_MAC_DUPLEX           0x00002000	/* MAC duplex mode */

/* ATTR */
#define MOT_TSEC_RX_SNOOP_ENABLE      0x00004000	/* snoop Rx cluster */
#define MOT_TSEC_RXBD_SNOOP_ENABLE    0x00008000	/* snoop rxbds */

/* MII flags */
#define MOT_TSEC_MII_NOPRE            0x00010000	/* suppress preamble */
#define MOT_TSEC_MII_RESET            0x00020000	/* runtime reset MII MGMT */
#define MOT_TSEC_MII_SCAN_CYCLE       0x00040000	/* Continuous read */
#define MOT_TSEC_MII_READ_CYCLE       0x00080000	/* Preform single read */

/* HALDUP */
#define MOT_TSEC_HALDUP_ALTBEB        0x00100000	/* use alternate backoff */
	/* algorithm */
#define MOT_TSEC_HALDUP_BACK_PRESSURE 0x00200000	/* immediate retrans back */
	/* pressure */
#define MOT_TSEC_HALDUP_EX_DEFERENCE  0x00400000
#define MOT_TSEC_HALDUP_NOBACKOFF     0x00800000

/* Driver Runtime Attributes */
#define MOT_TSEC_POLLING_MODE         0x01000000	/* polling mode */
#define MOT_TSEC_MULTICAST_MODE       0x02000000	/* multicast addressing */

/* TSEC Attribute Default Values */
#define MOT_TSEC_IMASK_DEFAULT (MOT_TSEC_IEVENT_RXC  | MOT_TSEC_IEVENT_TXC   | \
                                MOT_TSEC_IEVENT_MSRO | MOT_TSEC_IEVENT_GTSC  | \
                                MOT_TSEC_IEVENT_TXB  | MOT_TSEC_IEVENT_RXB0  | \
                                MOT_TSEC_IEVENT_TXF  | MOT_TSEC_IEVENT_GRSC  | \
                                MOT_TSEC_IEVENT_RXF0 )

#define MOT_TSEC_IEVENT_ERROR  (MOT_TSEC_IEVENT_BSY  | MOT_TSEC_IEVENT_EBERR | \
                                MOT_TSEC_IEVENT_TXE  | MOT_TSEC_IEVENT_LC    | \
                                MOT_TSEC_IEVENT_CRL  | MOT_TSEC_IEVENT_XFUN  | \
                                MOT_TSEC_IEVENT_BABT | MOT_TSEC_IEVENT_BABR )

#define MOT_TSEC_MACCFG1_DEFAULT (0x0)


/* IF_MODE = 1 ; MII mode 10/100 only */
#define MOT_TSEC_MACCFG2_DEFAULT  \
                (MOT_TSEC_MACCFG2_PRE_LEN(7)  | \
                 MOT_TSEC_MACCFG2_FULL_DUPLEX | \
                 MOT_TSEC_MACCFG2_PADCRC      | \
                 MOT_TSEC_MACCFG2_CRC_EN      | \
                 MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI))

/* CLRCNT = YES,  = NO,  STEN = YES,  TBIM = NO, RPM = NO, R100M = 10 */

#define MOT_TSEC_ECNTRL_DEFAULT  (MOT_TSEC_ECNTRL_STEN | MOT_TSEC_ECNTRL_AUTOZ)

#define MOT_TSEC_TBIPA_DEFAULT  (30)

#define MOT_TSEC_EDIS_DEFAULT (MOT_TSEC_EDIS_EBERRDIS 	| 	\
                               MOT_TSEC_EDIS_TXEDIS 	| 	\
			       MOT_TSEC_EDIS_LCDIS    	| 	\
                               MOT_TSEC_EDIS_CRLDIS 	| 	\
			       MOT_TSEC_EDIS_XFUNDIS )

/* Minimum Frame Receive Length */
#define MOT_TSEC_MINFLR_DEFAULT (64)

/* Tx Flow Control PAUSE Time Default  */
#define MOT_TSEC_PVT_DEFAULT  (MOT_TSEC_PTV_PTE(0)+ MOT_TSEC_PTV_PTE(0))

/* DMA Control Register Default */
#define MOT_TSEC_DMACTRL_DEFAULT  (MOT_TSEC_DMACTRL_TDSEN  | \
                                   MOT_TSEC_DMACTRL_TBDSEN | \
                                   MOT_TSEC_DMACTRL_WWR)

#define MOT_TSEC_FIFO_TX_THR_DEFAULT           (256)
#define MOT_TSEC_FIFO_TX_STARVE_DEFAULT        (128)
#define MOT_TSEC_FIFO_TX_STARVE_OFF_DEFAULT    (256)

/* Transmit Interrupt Coalescing */
#define MOT_TSEC_TXIC_DEFAULT MOT_TSEC_IC_ICFCT(18) | \
                              MOT_TSEC_IC_ICTT(0xffff)

/* Receive Interrupt Coalescing */
#define MOT_TSEC_RXIC_DEFAULT ( MOT_TSEC_IC_ICFCT(32) |	\
								MOT_TSEC_IC_ICTT(400))


/* BACK PRESSURE = NO, RFC_PAUSE = NO, TFC_PAUSE = NO */
#define MOT_TSEC_TCTRL_DEFAULT      (0)
#define MOT_TSEC_TSTAT_DEFAULT      (MOT_TSEC_TSTAT_THLT)

/* PROMISCUOUS = NO, BROAD_REJECT = NO, RX SHORT FRAMES = NO */
#define MOT_TSEC_RCTRL_DEFAULT      (0)
#define MOT_TSEC_RSTAT_DEFAULT      (0)
#define MOT_TSEC_MRBLR_DEFAULT      MOT_TSEC_MRBLR(0x600)

#define MOT_TSEC_IPGIFG_DEFAULT     (MOT_TSEC_IPGIFG_NBBIPG1(0x40) | \
                                     MOT_TSEC_IPGIFG_NBBIPG2(0x60) | \
                                     MOT_TSEC_IPGIFG_MIFGE(0x50)   | \
                                     MOT_TSEC_IPGIFG_BBIPG(0x60))

#define MOT_TSEC_HAFDUP_DEFAULT     (MOT_TSEC_HALDUP_ALTBEB_TRUNC(0x0a) | \
                                     MOT_TSEC_HALDUP_RETRY(0x0f)        | \
                                     MOT_TSEC_HALDUP_COL_WINDOW(0x37)     )

#define MOT_TSEC_MAXFRM_DEFAULT      MOT_TSEC_MAXFRM(0x0600)
#define MOT_TSEC_MIICFG_DEFAULT     (MOT_TSEC_MIIMCFG_MCS(MOT_TSEC_MIIMCFG_MCS_14))
#define MOT_TSEC_MIICOM_DEFAULT      0
#define MOT_TSEC_IFSTAT_DEFAULT      0

#define MOT_TSEC_ATTR_DEFAULT_NO_L2  ((MOT_TSEC_ATTR_RDSEN | MOT_TSEC_ATTR_RBDSEN)     | \
                                (MOT_TSEC_ATTR_BDLWT(MOT_TSEC_ATTR_BDLWT_NA)))
#define MOT_TSEC_ATTR_DEFAULT_L2     ((MOT_TSEC_ATTR_RDSEN | MOT_TSEC_ATTR_RBDSEN)     | \
                                (MOT_TSEC_ATTR_BDLWT(MOT_TSEC_ATTR_BDLWT_L2)))

/* Set extraction length to be 2 * max frame size */
#define MOT_TSEC_ATTRELI_EL_DEFAULT (MOT_TSEC_ATTRELI_EL(0xc00) | \
                                     MOT_TSEC_ATTRELI_EI(0))


/* TSEC Register flags */
#define MOT_TSEC_FLAG_MINFLR      0x00000001
#define MOT_TSEC_FLAG_MAXFRM      0x00000002
#define MOT_TSEC_FLAG_PVT         0x00000004
#define MOT_TSEC_FLAG_TBIPA       0x00000008
#define MOT_TSEC_FLAG_FIFO_TX     0x00000010
#define MOT_TSEC_FLAG_IADDR       0x00000020
#define MOT_TSEC_FLAG_GADDR       0x00000040
#define MOT_TSEC_FLAG_MACCFG2     0x00000080
#define MOT_TSEC_FLAG_IPGIFGI     0x00000100
#define MOT_TSEC_FLAG_HAFDUP      0x00000200
#define MOT_TSEC_FLAG_MIICFG      0x00000400
#define MOT_TSEC_FLAG_ATTR        0x00000800

	typedef struct {
		/* function pointers for BSP and Driver call backs */
		FUNCPTR miiPhyInit;		/* BSP Phy driver init callback */
		FUNCPTR miiPhyInt;		/* BSP call back to process PHY status change interrupt */
		FUNCPTR miiPhyStatusGet;	/* Driver call back to get duplex status  */
		FUNCPTR miiPhyRead;		/* BSP call back to read MII/PHY registers */
		FUNCPTR miiPhyWrite;	/* BSP call back to write MII/PHY registers */
		FUNCPTR enetAddrGet;	/* Driver call back to get the Ethernet address */
		FUNCPTR enetAddrSet;	/* Driver call back to set the Ethernet address */
		FUNCPTR enetEnable;		/* Driver call back to enable the ENET interface */
		FUNCPTR enetDisable;	/* Driver call back to disable the ENET interface */
		FUNCPTR extWriteL2AllocFunc;	/* Driver call back to put tx bd in L2 */
	} MOT_TSEC_FUNC_TABLE;

	typedef struct {
		UINT32 phyAddr;			/* phy physical address */
		UINT32 phyDefMode;		/* default mode */
		UINT32 phyMaxDelay;		/* max delay */
		UINT32 phyDelayParm;	/* poll interval if in poll mode */
		MII_AN_ORDER_TBL *phyAnOrderTbl;	/* autonegotiation table */
	} MOT_TSEC_PHY_PARAMS;

	typedef struct {
		VUINT16 bdStat;
		VUINT16 bdLen;
		VUINT32 bdAddr;
	} TSEC_BD;


	typedef struct {
		UINT8 *memBufPtr;		/* Buffer pointer for allocated buffer space */
		UINT32 memBufSize;		/* Buffer pool size */
		TSEC_BD *bdBasePtr;		/* Descriptor Base Address */
		UINT32 bdSize;			/* Descriptor Size */
		UINT32 rbdNum;			/* Number of Receive Buffer Descriptors  */
		UINT32 tbdNum;			/* Number of Transmit Buffer Descriptors */
		UINT32 unit0TxVec;
		UINT32 unit0RxVec;
		UINT32 unit0ErrVec;
		UINT32 unit1TxVec;
		UINT32 unit1RxVec;
		UINT32 unit1ErrVec;
	} MOT_TSEC_PARAMS;

	typedef struct {
		UINT32 usrBitFlags;		/* TSEC specific user bit flags */
		UINT32 usrRegFlags;		/* TSEC specific user reg flags */
		UINT32 minFrameLength;	/* Ethernet Minimum Frame Length */
		UINT32 maxFrameLength;	/* Ethernet Maximum Frame Length */

		/* TSEC Specific Device Parameters */
		UINT32 pauseTimeValue;	/* ext + pause time value */
		UINT32 tbiPhyAds;		/* Ten Bit Interface physical address */

		/* Tx FIFO Manipulation */
		UINT32 fifoTxTheshold;
		UINT32 fifoTxStarve;
		UINT32 fifoTxStarveShutOff;

		/* MAC specific Parameters */
		UINT32 macIndividualHash[8];	/* initial individual addresses [8] */
		UINT32 macGroupHash[8];	/* initial group addresses [8] */

		UINT32 macPreambleLength;
		UINT32 macIfMode;

		UINT32 macIpgifgNbbipg1;
		UINT32 macIpgifgNbbipg2;
		UINT32 macIpgifgMifge;
		UINT32 macIpgifgBbipg;

		/* MAC half duplex specific parameters */
		UINT32 macHalfDuplexAltBebTruncation;
		UINT32 macHalfDuplexRetryMaximum;
		UINT32 macHalfDuplexCollisionWindow;

		UINT32 miiMgmtClockSelect;
		UINT32 phyAddress;

		/* Misc */
		UINT32 extL2Cache;
		UINT32 bdL2Cache;
		UINT32 dmaExtLength;
		UINT32 dmaExtIndex;

	} MOT_TSEC_EXT_PARAMS;

typedef struct
    {
    UINT32   inum_tsecTx;    /* Transmit Interrupt */
    UINT32   inum_tsecRx;    /* Receive Interrupt */
    UINT32   inum_tsecErr;    /* Error Interrupt */
    FUNCPTR  inumToIvec;     /* function to convert INUM to IVEC */
    FUNCPTR  ivecToInum;     /* function to convert IVEC to INUM */
    } MOT_TSEC_INT_CTRL;


/*-----------------------------------------------------------------*/

/*                   TSEC registers                                */

/*-----------------------------------------------------------------*/
	typedef struct {

/* TSEC General Control and Status Registers */
		VUINT32 pad_1[4];
		VUINT32 ievent;
		VUINT32 imask;
		VUINT32 edis;
		VUINT32 pad_2;
		VUINT32 ecntrl;
		VUINT32 minflr;
		VUINT32 ptv;
		VUINT32 dmactrl;
		VUINT32 tbipa;
		VUINT32 pad_3[3];
		VUINT32 pad_4[19];

/* TSEC FIFO control and Status */
		VUINT32 fifoTxTheshold;
		VUINT32 pad_5[2];
		VUINT32 fifoTxStarve;
		VUINT32 fifoTxStarveShutoff;
		VUINT32 pad_6[24];

/* TSEC Transmit Control and Status Registers */
		VUINT32 tctrl;
		VUINT32 tstat;
		VUINT32 pad_7;
		VUINT32 tbdlen;
		VUINT32 txic;
		VUINT32 pad_8[4];
		VUINT32 ctbptr;
		VUINT32 pad_9[23];
		VUINT32 tbptr;
		VUINT32 pad_10[31];
		VUINT32 tbase;
		VUINT32 pad_11[42];
		VUINT32 ostbd;
		VUINT32 ostbdp;
		VUINT32 pad_12[18];

/* TSEC Receive Control and Status */
		VUINT32 rctrl;
		VUINT32 rstat;
		VUINT32 pad_13[1];
		VUINT32 rbdlen;
    VUINT32 rxic;
    VUINT32 pad_14[3];
		VUINT32 pad_15[1];
		VUINT32 crbptr;
		VUINT32 pad_16[6];
		VUINT32 mrblr;
		VUINT32 pad_17[16];
		VUINT32 rbptr;
		VUINT32 pad_18[31];
		VUINT32 rbase;
		VUINT32 pad_19[62];

/* TSEC MAC Registers */
		VUINT32 maccfg1;
		VUINT32 maccfg2;
		VUINT32 ipgifgi;
		VUINT32 hafdup;
		VUINT32 maxfrm;
		VUINT32 pad_20[1];
		VUINT32 pad_21[1];
		VUINT32 pad_22[1];

		VUINT32 miicfg;
		VUINT32 miicom;
		VUINT32 miimadd;
		VUINT32 miimcon;
		VUINT32 miimstat;
		VUINT32 miimind;
		VUINT32 pad_23[1];

		VUINT32 ifstat;
		VUINT32 macstnaddr1;
		VUINT32 macstnaddr2;
		VUINT32 pad_24[78];

/* TSEC Receive Counters */
		VUINT32 tr64;
		VUINT32 tr127;
		VUINT32 tr255;
		VUINT32 tr511;
		VUINT32 tr1k;
		VUINT32 trmax;
		VUINT32 trmgv;
		VUINT32 rbyt;
		VUINT32 rpkt;
		VUINT32 rfcs;
		VUINT32 rmca;
		VUINT32 rbca;
		VUINT32 rxcf;
		VUINT32 rxpf;
		VUINT32 rxuo;
		VUINT32 raln;
		VUINT32 rflr;
		VUINT32 rcde;
		VUINT32 rcse;
		VUINT32 rund;
		VUINT32 rovr;
		VUINT32 rfgr;
		VUINT32 rjbr;
		VUINT32 rdrp;

/* Transmit Counters */
		VUINT32 tbyt;
		VUINT32 tpkt;
		VUINT32 tmca;
		VUINT32 tbca;
		VUINT32 txpf;
		VUINT32 tdfr;
		VUINT32 tedf;
		VUINT32 tscl;
		VUINT32 tmcl;
		VUINT32 tlcl;
		VUINT32 txcl;
		VUINT32 tncl;
		VUINT32 pad_25[1];
		VUINT32 tdrp;
		VUINT32 tjbr;
		VUINT32 tfcs;
		VUINT32 tfcf;
		VUINT32 tovr;
		VUINT32 tund;
		VUINT32 tfrg;

		/* General Registers */
		VUINT32 car1;
		VUINT32 car2;
		VUINT32 cam1;
		VUINT32 cam2;
		VUINT32 pad_26[48];

/* Individual Hash function Registers */
		VUINT32 iaddr[8];
		VUINT32 pad_27[24];

/* Group Hash function Registers */
		VUINT32 gaddr[8];
		VUINT32 pad_28[214];

		VUINT32 attr;
		VUINT32 attreli;

/* TSEC Future Expansion Space */
		VUINT32 resv[256];
	} TSEC_REG_T;


	typedef struct {
		UINT32 numInts;
		UINT32 numZcopySends;
		UINT32 numNonZcopySends;
		UINT32 numTXBInts;
		UINT32 numBSYInts;
		UINT32 numRXFInts;
		UINT32 numRXBInts;
		UINT32 numGRAInts;
		UINT32 numRXCInts;
		UINT32 numTXCInts;
		UINT32 numTXEInts;
		UINT32 numOTHERInts;

		UINT32 numRXFHandlerEntries;
		UINT32 numRXFHandlerErrQuits;
		UINT32 numRXFHandlerFramesProcessed;
		UINT32 numRXFHandlerFramesRejected;
		UINT32 numRXFHandlerNetBufAllocErrors;
		UINT32 numRXFHandlerNetCblkAllocErrors;
		UINT32 numRXFHandlerNetMblkAllocErrors;
		UINT32 numRXFHandlerFramesCollisions;
		UINT32 numRXFHandlerFramesCrcErrors;
		UINT32 numRXFHandlerFramesLong;
		UINT32 numRXFExceedBurstLimit;

		UINT32 numNetJobAddErrors;

		UINT32 numRxStallsEntered;
		UINT32 numRxStallsCleared;

		UINT32 numTxStallsEntered;
		UINT32 numTxStallsCleared;
		UINT32 numTxStallErrors;

		UINT32 numLSCHandlerEntries;
		UINT32 numLSCHandlerExits;

		UINT32 txErr;
		UINT32 HbFailErr;
		UINT32 txLcErr;
		UINT32 txUrErr;
		UINT32 txCslErr;
		UINT32 txRlErr;
		UINT32 txDefErr;

		UINT32 rxBsyErr;
		UINT32 rxLgErr;
		UINT32 rxNoErr;
		UINT32 rxCrcErr;
		UINT32 rxOvErr;
		UINT32 rxShErr;
		UINT32 rxLcErr;
		UINT32 rxMemErr;
	} TSEC_DRIVER_STATS;


#define MOT_TSEC_FRAME_SET(p)        TSEC_REG_T * tsecReg = (p)->tsecRegsPtr;
#define MOT_TSEC_MII_FRAME_SET(p)    TSEC_REG_T * tsecMiiReg = (p)->tsecMiiPtr;
#define MOT_TSEC_IEVENT_REG          tsecReg->ievent
#define MOT_TSEC_IMASK_REG           tsecReg->imask
#define MOT_TSEC_EDIS_REG            tsecReg->edis
#define MOT_TSEC_ECNTRL_REG          tsecReg->ecntrl
#define MOT_TSEC_MINFLR_REG          tsecReg->minflr
#define MOT_TSEC_PTV_REG             tsecReg->ptv
#define MOT_TSEC_DMACTRL_REG         tsecReg->dmactrl
#define MOT_TSEC_TBIPA_REG           tsecReg->tbipa
#define MOT_TSEC_FIFO_TX_THR_REG     tsecReg->fifoTxTheshold
#define MOT_TSEC_FIFO_TX_STARVE_REG  tsecReg->fifoTxStarve
#define MOT_TSEC_FIFO_TX_STARVE_SHUTOFF_REG tsecReg->fifoTxStarveShutoff
#define MOT_TSEC_TCTRL_REG           tsecReg->tctrl
#define MOT_TSEC_TSTAT_REG           tsecReg->tstat
#define MOT_TSEC_TBDLEN_REG          tsecReg->tbdlen
#define MOT_TSEC_TXIC_REG            tsecReg->txic
#define MOT_TSEC_CTBPTR_REG          tsecReg->ctbptr
#define MOT_TSEC_TBPTR_REG           tsecReg->tbptr
#define MOT_TSEC_TBASE_REG           tsecReg->tbase
#define MOT_TSEC_OSTBD_REG           tsecReg->ostbd
#define MOT_TSEC_OSTBDP_REG          tsecReg->ostbdp
#define MOT_TSEC_RCTRL_REG           tsecReg->rctrl
#define MOT_TSEC_RSTAT_REG           tsecReg->rstat
#define MOT_TSEC_RBDLEN_REG          tsecReg->rbdlen
#define MOT_TSEC_RXIC_REG            tsecReg->rxic
#define MOT_TSEC_CRBPTR_REG          tsecReg->crbptr
#define MOT_TSEC_MRBLR_REG           tsecReg->mrblr
#define MOT_TSEC_RBPTR_REG           tsecReg->rbptr
#define MOT_TSEC_RBASE_REG           tsecReg->rbase
#define MOT_TSEC_MACCFG1_REG         tsecReg->maccfg1
#define MOT_TSEC_MACCFG2_REG         tsecReg->maccfg2
#define MOT_TSEC_IPGIFG_REG          tsecReg->ipgifgi
#define MOT_TSEC_HAFDUP_REG          tsecReg->hafdup
#define MOT_TSEC_MAXFRM_REG          tsecReg->maxfrm
#define MOT_TSEC_MIIMCFG_REG         tsecMiiReg->miicfg
#define MOT_TSEC_MIIMCOM_REG         tsecMiiReg->miicom
#define MOT_TSEC_MIIMADD_REG         tsecMiiReg->miimadd
#define MOT_TSEC_MIIMCON_REG         tsecMiiReg->miimcon
#define MOT_TSEC_MIIMSTAT_REG        tsecMiiReg->miimstat
#define MOT_TSEC_MIIMIND_REG         tsecMiiReg->miimind
#define MOT_TSEC_IFSTAT_REG          tsecReg->ifstat
#define MOT_TSEC_MACSTNADDR1_REG     tsecReg->macstnaddr1
#define MOT_TSEC_MACSTNADDR2_REG     tsecReg->macstnaddr2
#define MOT_TSEC_TR64_REG            tsecReg->tr64
#define MOT_TSEC_TR127_REG           tsecReg->tr127
#define MOT_TSEC_TR255_REG           tsecReg->tr255
#define MOT_TSEC_TR511_REG           tsecReg->tr511
#define MOT_TSEC_TR1K_REG            tsecReg->tr1k
#define MOT_TSEC_TRMAX_REG           tsecReg->trmax
#define MOT_TSEC_TRMGV_REG           tsecReg->trmgv
#define MOT_TSEC_RBYT_REG            tsecReg->rbyt
#define MOT_TSEC_RPKT_REG            tsecReg->rpkt
#define MOT_TSEC_RFCS_REG            tsecReg->rfcs
#define MOT_TSEC_RMCA_REG            tsecReg->rmca
#define MOT_TSEC_RBCA_REG            tsecReg->rbca
#define MOT_TSEC_RXCF_REG            tsecReg->rxcf
#define MOT_TSEC_RXPF_REG            tsecReg->rxpf
#define MOT_TSEC_RXUO_REG            tsecReg->rxuo
#define MOT_TSEC_RALN_REG            tsecReg->raln
#define MOT_TSEC_RFLR_REG            tsecReg->rflr
#define MOT_TSEC_RCDE_REG            tsecReg->rcde
#define MOT_TSEC_RCSE_REG            tsecReg->rcse
#define MOT_TSEC_RUND_REG            tsecReg->rund
#define MOT_TSEC_ROVR_REG            tsecReg->rovr
#define MOT_TSEC_RFGR_REG            tsecReg->rfgr
#define MOT_TSEC_RJBR_REG            tsecReg->rjbr
#define MOT_TSEC_RDRP_REG            tsecReg->rdrp

#define MOT_TSEC_TBYT_REG            tsecReg->tbyt
#define MOT_TSEC_TPKT_REG            tsecReg->tpkt
#define MOT_TSEC_TMCA_REG            tsecReg->tmca
#define MOT_TSEC_TBCA_REG            tsecReg->tbca
#define MOT_TSEC_TXPF_REG            tsecReg->txpf
#define MOT_TSEC_TDFR_REG            tsecReg->tdfr
#define MOT_TSEC_TEDF_REG            tsecReg->tedf
#define MOT_TSEC_TSCL_REG            tsecReg->tscl
#define MOT_TSEC_TMCL_REG            tsecReg->tmcl
#define MOT_TSEC_TLCL_REG            tsecReg->tlcl
#define MOT_TSEC_TXCL_REG            tsecReg->txcl
#define MOT_TSEC_TNCL_REG            tsecReg->tncl

#define MOT_TSEC_TDRP_REG            tsecReg->tdrp
#define MOT_TSEC_TJBR_REG            tsecReg->tjbr
#define MOT_TSEC_TFCS_REG            tsecReg->tfcs
#define MOT_TSEC_TFCF_REG            tsecReg->tfcf
#define MOT_TSEC_TOVR_REG            tsecReg->tovr
#define MOT_TSEC_TUND_REG            tsecReg->tund
#define MOT_TSEC_TFGR_REG            tsecReg->tfrg

#define MOT_TSEC_CAR1_REG            tsecReg->car1
#define MOT_TSEC_CAR2_REG            tsecReg->car2
#define MOT_TSEC_CAM1_REG            tsecReg->cam1
#define MOT_TSEC_CAM2_REG            tsecReg->cam2

#define MOT_TSEC_IADDR_REG           tsecReg->iaddr
#define MOT_TSEC_IADDR0_REG          tsecReg->iaddr[0]
#define MOT_TSEC_IADDR1_REG          tsecReg->iaddr[1]
#define MOT_TSEC_IADDR2_REG          tsecReg->iaddr[2]
#define MOT_TSEC_IADDR3_REG          tsecReg->iaddr[3]
#define MOT_TSEC_IADDR4_REG          tsecReg->iaddr[4]
#define MOT_TSEC_IADDR5_REG          tsecReg->iaddr[5]
#define MOT_TSEC_IADDR6_REG          tsecReg->iaddr[6]
#define MOT_TSEC_IADDR7_REG          tsecReg->iaddr[7]

#define MOT_TSEC_GADDR_REG           tsecReg->gaddr
#define MOT_TSEC_GADDR0_REG          tsecReg->gaddr[0]
#define MOT_TSEC_GADDR1_REG          tsecReg->gaddr[1]
#define MOT_TSEC_GADDR2_REG          tsecReg->gaddr[2]
#define MOT_TSEC_GADDR3_REG          tsecReg->gaddr[3]
#define MOT_TSEC_GADDR4_REG          tsecReg->gaddr[4]
#define MOT_TSEC_GADDR5_REG          tsecReg->gaddr[5]
#define MOT_TSEC_GADDR6_REG          tsecReg->gaddr[6]
#define MOT_TSEC_GADDR7_REG          tsecReg->gaddr[7]

#define MOT_TSEC_ATTR_REG            tsecReg->attr
#define MOT_TSEC_ATTRELI_REG         tsecReg->attreli

/* TBI registers */
#define MOT_TSEC_TBI_SR_REG          MOT_TSEC_TBIPA_REG + 1


/* Driver State Variables */

#define MOT_TSEC_STATE_INIT          0x00
#define MOT_TSEC_STATE_NOT_LOADED    0x00
#define MOT_TSEC_STATE_LOADED        0x01
#define MOT_TSEC_STATE_NOT_RUNNING   0x00
#define MOT_TSEC_STATE_RUNNING       0x02

/* Internal driver flags */

#define MOT_TSEC_OWN_BUF_MEM    0x01	/* internally provided memory for data */
#define MOT_TSEC_INV_TBD_NUM    0x02	/* invalid tbdNum provided */
#define MOT_TSEC_INV_RBD_NUM    0x04	/* invalid rbdNum provided */
#define MOT_TSEC_POLLING        0x08	/* polling mode */
#define MOT_TSEC_PROM           0x20	/* promiscuous mode */
#define MOT_TSEC_MCAST          0x40	/* multicast addressing mode */
#define MOT_TSEC_FD             0x80	/* full duplex mode */
#define MOT_TSEC_OWN_BD_MEM     0x10	/* internally provided memory for BDs */
#define MOT_TSEC_MIN_TX_PKT_SZ   100	/* the smallest packet we send */

#define MOT_TSEC_CL_NUM_DEFAULT   128	/* number of tx clusters */
#define MOT_TSEC_CL_MULTIPLE       11	/* ratio of clusters to RBDs */
#define MOT_TSEC_TBD_NUM_DEFAULT  32	/* default number of TBDs */
#define MOT_TSEC_RBD_NUM_DEFAULT  32	/* default number of RBDs */
#define MOT_TSEC_TX_POLL_NUM      1	/* one TBD for poll operation */

#define MOT_TSEC_CL_OVERHEAD      4	/* prepended cluster overhead */
#define MOT_TSEC_CL_ALIGNMENT     64	/* cluster required alignment */
#define MOT_TSEC_CL_SIZE          1536	/* cluster size */
#define MOT_TSEC_MBLK_ALIGNMENT   64	/* mBlks required alignment */
#define MOT_TSEC_BD_SIZE          0x8	/* size of TSEC BD */
#define MOT_TSEC_BD_ALIGN         64	/* required alignment for BDs */
#define MOT_TSEC_BUF_ALIGN        64	/* required alignment for data buffer */

/*
 * the total is 0x630 and it accounts for the required alignment
 * of receive data buffers, and the cluster overhead.
 */
#define XXX_TSEC_MAX_CL_LEN ((MII_ETH_MAX_PCK_SZ             \
                            + (MOT_TSEC_BUF_ALIGN - 1)       \
                            + MOT_TSEC_BUF_ALIGN             \
                            + (MOT_TSEC_CL_OVERHEAD - 1))    \
                            & (~ (MOT_TSEC_CL_OVERHEAD - 1)))

#define MOT_TSEC_MAX_CL_LEN     ROUND_UP(XXX_TSEC_MAX_CL_LEN,MOT_TSEC_BUF_ALIGN)

#define MOT_TSEC_RX_CL_SZ       (MOT_TSEC_MAX_CL_LEN)
#define MOT_TSEC_TX_CL_SZ       (MOT_TSEC_MAX_CL_LEN)

/* BIT mask defines for hardware specific PHY events. */
#define MOT_TSEC_PHY_EVENT_AUTONEG_ERROR    0x0001
#define MOT_TSEC_PHY_EVENT_SPEED            0x0002
#define MOT_TSEC_PHY_EVENT_DUPLEX           0x0004
#define MOT_TSEC_PHY_EVENT_AUTONEG_COMPLETE 0x0008
#define MOT_TSEC_PHY_EVENT_LINK             0x0010
#define MOT_TSEC_PHY_EVENT_SYMBOL_ERROR     0x0020
#define MOT_TSEC_PHY_EVENT_FALSE_CARRIER    0x0040
#define MOT_TSEC_PHY_EVENT_FIFO_ERROR       0x0080
#define MOT_TSEC_PHY_EVENT_XOVER            0x0100
#define MOT_TSEC_PHY_EVENT_DOWNSHIFT        0x0200
#define MOT_TSEC_PHY_EVENT_POLARITY         0x0400
#define MOT_TSEC_PHY_EVENT_JABBER           0x0800

/* PHY Access definitions */
#define MOT_TSEC_PHY_GIG_STATUS_REG	0xa
#define MOT_TSEC_PHY_1000_M_LINK_FD     0x0800
#define MOT_TSEC_PHY_1000_M_LINK_OK     0x1000

#define MOT_TSEC_PHY_LINK_STATUS        0x5
#define MOT_TSEC_PHY_10_M_LINK_FD       0x0040
#define MOT_TSEC_PHY_100_M_LINK_FD      0x0100

	typedef struct {
		UINT8 autonegError;		/* 0-N/A, 0 - none,   1- error */
		UINT8 duplex;			/* 1 - half,   2- full */
#define MOT_TSEC_PHY_DUPLEX_HALF    (1)
#define MOT_TSEC_PHY_DUPLEX_FULL    (2)
		UINT8 speed;			/* 0-N/A, 1 - 10, 2- 100, 3 -1G */
#define MOT_TSEC_PHY_SPEED_10       (1)
#define MOT_TSEC_PHY_SPEED_100      (2)
#define MOT_TSEC_PHY_SPEED_1000     (3)
		UINT8 link;				/* 0-N/A, 1 - down, 2- up */
		UINT8 symbolError;		/* 0-N/A, 1 - none, 2- error */
		UINT8 autoNegComplete;	/* 0-N/A, 1 - no, 2- completed */
		UINT8 energyDetect;		/* 0-N/A, 1 - no, 2- detected */
		UINT8 falseCarrier;		/* 0-N/A, 1 - no, 2- detected */
		UINT8 downShift;		/* 0-N/A, 1 - no, 2- detected */
		UINT8 fifoError;		/* 0-N/A, 1 - none, 2- error */
		UINT8 xover;			/* 0-N/A, 1 - MDI, 2- MDIX */
		UINT8 polarity;			/* 0-N/A, 1 - normal, 2- reversed */
		UINT8 jabber;			/* 0-N/A, 1 - no, 2- detected */
		UINT8 pageReceived;		/* 0-N/A, 1 - no, 2- detected */
		UINT8 cableLength;		/* 0-N/A */
		UINT8 txPause;			/* 0-N/A, 1 - no, 2- detected */
		UINT8 rxPause;			/* 0-N/A, 1 - no, 2- detected */
		UINT8 farEndFault;		/* 0-N/A, 1 - no, 2- detected */
		UINT32 rxErrorCntr;		/* 0-N/A, number of rx errors */
		UINT32 reserved[4];		/* future use */
	} MOT_TSEC_PHY_STATUS;

/* The definition of the driver control structure */

	typedef struct tsec_drv_ctrl {
		END_OBJ endObj;			/* base class */
		TSEC_REG_T *tsecRegsPtr;	/* pointer to TSEC registers */
		UINT32 unit;			/* unit number 0 or 1 */
		ENET_ADDR enetAddr;		/* current Ethernet Address */
		UINT32    inum_tsecTx;  /* TSEC Tx interrupt num */
		UINT32    inum_tsecRx;  /* TSEC Rx interrupt num */
		UINT32    inum_tsecErr; /*TSEC Err interrupt num */
		FUNCPTR   inumToIvec;   /* Conversion utility */
		FUNCPTR   ivecToInum;   /* Conversion utility */
		UINT32 initFlags;		/* user init flags */
		UINT32 userFlags;
		BOOL pollDone;			/* Flag indicating outer loop exit */
		UINT32 pollCnt;			/* polling counter */
		UINT32 pollLoops;		/* polling limit */
		UINT maxRxFrames;		/* max frames to Receive in one job */
		int initType;			/* netPoolInit() or netPoolCreate() */
    NETBUF_CFG * pNetBufCfg;
#ifdef USE_NET_JOB_ADD
		volatile BOOL rxJobQued;
		volatile BOOL tbdFreeQued;
		volatile UINT32 txHandlerQued;
#endif
		UINT32 retries;
		UINT32 maxRetries;

		/* phy init parameter table */
		MOT_TSEC_PHY_PARAMS *phyInit;

		/* Bsp specific functions and call backs */
		MOT_TSEC_FUNC_TABLE *initFuncs;

		/* Driver specific init parameters */
		MOT_TSEC_PARAMS *initParms;

		/* TSEC hardware specific init parameters */
		MOT_TSEC_EXT_PARAMS *initParmsExt;

		/* Interrupt Controller Specific info */
		MOT_TSEC_INT_CTRL *intCtrl;

		TSEC_REG_T *tsecMiiPtr;	/* pointer to TSEC Mii registers */

		UINT32 tsecNum;			/* physical TSEC 0 or 1 */

		UINT32 fifoTxBase;		/* address of Tx FIFO in internal RAM */
		UINT32 fifoRxBase;		/* address of Rx FIFO in internal RAM */

		char *pBufAlloc;		/* Allocated TSEC memory pool base */
		char *pBufBase;			/* Rounded TSEC memory pool base */
		UINT32 bufSize;			/* TSEC memory pool size */

		TSEC_BD *pBdAlloc;		/* TSEC BDs Alloc pointer */
		TSEC_BD *pBdBase;		/* TSEC BDs base */
		UINT32 bdSize;			/* TSEC BDs size */

		/* receive buffer descriptor management */
		UINT32 rbdNum;			/* number of RX bd's */
		TSEC_BD *pRbdNext;		/* RBD next to rx */
		TSEC_BD *pRbdLast;		/* RBD last to replenish */
		UINT16 rbdCnt;			/* number of rbds full */
#ifndef USE_NET_JOB_ADD
		JOB_QUEUE_ID jobQueueId;	/* Job queue to post to */
		QJOB	rxJob;
#endif
		TSEC_BD *pRbdBase;		/* RBD ring */
		UINT32 rbdIndex;
		UINT32 rbdMask;
		M_BLK_ID *pMblkList;	/* allocated clusters */

		UINT16 rxUnStallThresh;	/* rx reclaim threshold */
		UINT16 rxStallThresh;	/* rx low water threshold */

		volatile BOOL rxStall;	/* rx handler stalled - no Tbd */

		/* transmit buffer descriptor management */

#ifndef USE_NET_JOB_ADD
		QJOB	txJob;
#endif
		UINT16 tbdNum;			/* number of TX bd's */
		volatile TSEC_BD *pTbdNext;	/* TBD index */
		TSEC_BD *pTbdLast;		/* TBD index */
		UINT16 tbdFree;
		TSEC_BD *pTbdBase;		/* TBD ring base */
		M_BLK **tBufList;		/* allocated clusters */
		UINT32 tbdMask;
		UINT32 tbdIndex;
		UINT32 tbdCleanIndex;
		UINT16 txUnStallThresh;	/* tx reclaim threshold */
		UINT16 txStallThresh;	/* tx low water threshold */

		UINT16 tbiAdr;			/* tbi interface address */
		UINT32 phyFlags;		/* Phy flags */
		UINT32 flags;			/* driver flags */
		UINT32 state;			/* driver state including load flag */
		UINT32 intMask;			/* interrupt mask register */
		UINT32 intErrorMask;	/* interrupt error mask register */
		UCHAR *pTxPollBuf;		/* cluster pointer for poll mode */
		M_BLK_ID pTxPollMblk;
		/* transmit command */
		char *pClBlkArea;		/* cluster block pointer */
		UINT32 clBlkSize;		/* clusters block memory size */
		char *pMBlkArea;		/* mBlock area pointer */
		UINT32 mBlkSize;		/* mBlocks area memory size */
		CACHE_FUNCS bufCacheFuncs;	/* cache descriptor */
		CL_POOL_ID pClPoolId;	/* cluster pool identifier */
		PHY_INFO *phyInfo;		/* info on a MII-compliant PHY */

		BOOL lscHandling;
		BOOL txStall;			/* tx handler stalled - no Tbd */

		/* function pointers to support unit testing */

#ifdef USE_NET_JOB_ADD
		FUNCPTR netJobAdd;
#endif
		FUNCPTR muxTxRestart;
		FUNCPTR muxError;

		/* Bsp specific functions and call backs */

		FUNCPTR phyInitFunc;	/* BSP Phy Init */
		FUNCPTR phyStatusFunc;	/* Status Get function */
		FUNCPTR miiPhyRead;		/* mii Read */
		FUNCPTR miiPhyWrite;	/* mii Write */
		FUNCPTR enetEnable;		/* enable ethernet */
		FUNCPTR enetDisable;	/* disable ethernet */
		FUNCPTR enetAddrGet;	/* get ethernet address */
		FUNCPTR enetAddrSet;	/* set ethernet Address */
		FUNCPTR extWriteL2AllocFunc;	/* Use ext write alloc L2 for Tx BD */
#ifdef MOT_TSEC_DBG
		TSEC_DRIVER_STATS *stats;
#endif
		UINT32 missedCnt;
		UINT32 passCnt;
		UINT32 packetCnt;
		UINT32 starveCnt;
		UINT32 busyMissed;
		BOOL busyState;
		END_IFDRVCONF endStatsConf;
		END_IFCOUNTERS endStatsCounters;
	} TSEC_DRV_CTRL;


IMPORT STATUS cacheInvalidate (CACHE_TYPE, void *, size_t);
IMPORT STATUS cacheFlush (CACHE_TYPE, void *, size_t);
IMPORT int    intEnable (int);
IMPORT int    intDisable (int);

#ifdef __cplusplus
}
#endif

#endif							/* __INCmotTsecEndh */

