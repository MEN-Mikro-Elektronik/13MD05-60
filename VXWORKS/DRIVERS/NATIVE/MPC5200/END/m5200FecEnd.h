/* m5200FecEnd.h - Motorola MPC5200 Ethernet network interface header */

/* Copyright 1990-2005 Wind River Systems, Inc. */

/*
modification history
--------------------
$Log: m5200FecEnd.h,v $
Revision 1.16  2011/01/20 12:55:19  rlange
R: Customer TSR873640
M: number of BD can be controlled by define FEC_OVERWRITE_BD_NUM
   (if not defined, the default value will be unchanged)

Revision 1.15  2010/10/14 13:08:37  UFranke
R: SYS_FEC_INT_CONNECT/SYS_FEC_INT_DISCONNECT unsave
M: SYS_FEC_INT_CONNECT/SYS_FEC_INT_DISCONNECT do not overwrite
   a previous error in return code

Revision 1.14  2010/04/23 10:51:07  rlange
R: Removed defined needed to build m5200fecEnd.a
M: Added removed define

Revision 1.13  2010/04/23 10:37:34  rlange
R: Z077End driver BSP interface changed
M: Changed type of specific MII_REGS structure collided with define
   in miiLib.h
   Removed bit defines existing in miiLib.h

Revision 1.12  2010/03/31 14:12:48  RLange
R: Register offset iaddr1/2 and HASH_H_/L_ wrong (Customer reply)
M: Changed offeset addresses

Revision 1.11  2006/12/22 11:40:24  RLange
Added m5200FecMiiWrite prototype declaration

Revision 1.10  2006/09/11 10:39:36  cs
added declaration of external function m5200FecMiiRead()

Revision 1.9  2006/07/12 11:01:58  ufranke
fixed
 - sometimes netTask exception happens due to
   driver restart after RX FIFO ovwerflow
   merged WRS SPR #113348 with MEN fixes

Revision 1.8  2006/07/12 10:38:50  ufranke
changed
 - renamed sysEnetAddrGet() to sysFecEnetAddrGet()

Revision 1.7  2006/03/09 13:25:06  UFRANKE
cosmetics

cosmetics
Revision 1.6  2006/02/03 14:42:34  UFRANKE
changed
 - TASK_ETH_BD_ to TASK_BD_ for BestCOMM 2.2

Revision 1.5  2005/10/04 17:39:38  UFranke
changed
 - moved LOCAL definitions away from this header into the driver

01g,17feb05,j_b  add undocumented XFIFO Control Register bit to set with Rev.B
01f,10jan05,k_p  replaced sysEnetAddrGet() with sysFecEnetAddrGet() as a part
                 of 'M' command modification and cleanups.
01e,17jun04,bjn  RFIFO_ERROR - changes to fecStop/fecStart.
01d,25may04,bjn  resolve various issues, see change log details (SPR97198)
01c,15mar04,bjn  Support for Bestcomm 2.0
01b,18Jul03,pkr  adopted from motFecEnd
01a,09nov98,cn   written.
*/

/*
change log details
------------------
01e * Added stoppingTx for handling of RFIFO_ERROR/XFIFO_ERROR
01d * Increase FEC_END_BD_LOAN_NUM from 32 to 64. This creates more
      network cluster buffers for the system to use and is necessary
      to allow ping buffer sizes of 65500 bytes to be returned.
      This results in m5200FecInitMem() allocating much more memory,
      so this parameter is best trimmed to suit the application.
    * Added BUF_TYPE_LOCAL, txBuffAvailable and pTxBuffLocal, requied
      to ensure that TX packets get sent to the wire when NET_BUF_ALLOC()
      returns NULL.
*/

#ifndef __INCm5200FecEndh
#define __INCm5200FecEndh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

/* revision D.3 and greater processors require special FEC initialization */

#define REV_D_4 0x0502
#define REV_D_3 0x0501

/*
 * redefine the macro below in the bsp if you need to access the device
 * registers/descriptors in a more suitable way.
 */

#ifndef FEC_END_LONG_WR
#define FEC_END_LONG_WR(addr, value)                                        \
    (* (addr) = ((UINT32) (value)))
#endif /* FEC_END_LONG_WR */

#ifndef FEC_END_WORD_WR
#define FEC_END_WORD_WR(addr, value)                                        \
    (* (addr) = ((UINT16) (value)))
#endif /* FEC_END_WORD_WR */

#ifndef FEC_END_BYTE_WR
#define FEC_END_BYTE_WR(addr, value)                                        \
    (* (addr) = ((UINT8) (value)))
#endif /* FEC_END_BYTE_WR */

#ifndef FEC_END_LONG_RD
#define FEC_END_LONG_RD(addr, value)                                        \
    ((value) = (* (UINT32 *) (addr)))
#endif /* FEC_END_LONG_RD */

#ifndef FEC_END_WORD_RD
#define FEC_END_WORD_RD(addr, value)                                        \
    ((value) = (* (UINT16 *) (addr)))
#endif /* FEC_END_WORD_RD */

#ifndef FEC_END_BYTE_RD
#define FEC_END_BYTE_RD(addr, value)                                        \
    ((value) = (* (UINT8 *) (addr)))
#endif /* FEC_END_BYTE_RD */

/*
 * Default macro definitions for BSP interface.
 * These macros can be redefined in a wrapper file, to generate
 * a new module with an optimized interface.
 */

#ifndef SYS_FEC_INT_CONNECT
#define SYS_FEC_INT_CONNECT(pDrvCtrl, pFuncFEC, pFuncRDMA, pFuncWDMA, arg, ret)  \
        { \
         IMPORT STATUS intConnect (VOIDFUNCPTR *, VOIDFUNCPTR, int); \
         ret = OK; \
         if (!pDrvCtrl->intrConnect) \
            { \
         if (FEC_END_FEC_VECTOR (pDrvCtrl)) \
                { \
                      ret |= (intConnect) ((VOIDFUNCPTR*) \
                         INUM_TO_IVEC (FEC_END_FEC_VECTOR (pDrvCtrl)), \
                                    (pFuncFEC), (int) (arg));  \
             } \
               \
           if (FEC_END_RDMA_VECTOR (pDrvCtrl))  \
            {  \
                   ret |= (intConnect) ((VOIDFUNCPTR*) \
                           INUM_TO_IVEC (FEC_END_RDMA_VECTOR (pDrvCtrl)),\
                                    (pFuncRDMA), (int) (arg)); \
             } \
               \
    if (FEC_END_WDMA_VECTOR (pDrvCtrl))  \
           { \
            ret |= (intConnect) ((VOIDFUNCPTR*)  \
                               INUM_TO_IVEC (FEC_END_WDMA_VECTOR (pDrvCtrl)), \
                                (pFuncWDMA), (int) (arg)); \
           } \
           pDrvCtrl->intrConnect = TRUE; \
    }  \
 }
#endif /* SYS_FEC_INT_CONNECT */



#ifndef SYS_FEC_INT_DISCONNECT

#define SYS_FEC_INT_DISCONNECT(pDrvCtrl, pFuncFEC, pFuncRDMA, pFuncWDMA, arg, ret) \
        {  \
        ret = OK;  \
        if ( m5200FecIntDisc != NULL) \
           { \
        if (FEC_END_FEC_VECTOR (pDrvCtrl)) \
               {  \
                ret |= (*m5200FecIntDisc) ((VOIDFUNCPTR*) \
                         INUM_TO_IVEC (FEC_END_FEC_VECTOR (pDrvCtrl)), \
                                  (pFuncFEC)); \
                }  \
              \
        if (FEC_END_RDMA_VECTOR (pDrvCtrl)) \
                {  \
                 ret |= (*m5200FecIntDisc) ((VOIDFUNCPTR*) \
                       INUM_TO_IVEC (FEC_END_RDMA_VECTOR (pDrvCtrl)), \
                           (pFuncRDMA)); \
                } \
         \
        if (FEC_END_WDMA_VECTOR (pDrvCtrl)) \
            { \
                ret |= (*m5200FecIntDisc) ((VOIDFUNCPTR*) \
                   INUM_TO_IVEC (FEC_END_WDMA_VECTOR (pDrvCtrl)),    \
                                  (pFuncWDMA)); \
               } \
          pDrvCtrl->intrConnect = FALSE; \
          } \
       }
#endif /* SYS_FEC_INT_DISCONNECT */


#ifndef SYS_FEC_INT_ENABLE
#define SYS_FEC_INT_ENABLE(pDrvCtrl, ret)  \
        {                                                               \
        IMPORT int intEnable (int);       \
        ret = OK;                                 \
        if (FEC_END_FEC_VECTOR (pDrvCtrl))    \
            ret = intEnable ((int) (FEC_END_FEC_VECTOR (pDrvCtrl)));    \
        if (FEC_END_RDMA_VECTOR (pDrvCtrl))                 \
           ret = intEnable ((int) (FEC_END_RDMA_VECTOR (pDrvCtrl)));    \
        if (FEC_END_WDMA_VECTOR (pDrvCtrl))                     \
           ret = intEnable ((int) (FEC_END_WDMA_VECTOR (pDrvCtrl)));    \
       }
#endif /* SYS_FEC_INT_ENABLE */


#ifndef SYS_FEC_INT_DISABLE

#define SYS_FEC_INT_DISABLE(pDrvCtrl, ret)   \
        {                                                               \
        IMPORT int intDisable (int);         \
        ret = OK;  \
        if (FEC_END_FEC_VECTOR (pDrvCtrl))   \
          ret = intDisable ((int) (FEC_END_FEC_VECTOR (pDrvCtrl))); \
        if (FEC_END_RDMA_VECTOR (pDrvCtrl))                     \
          ret = intDisable ((int) (FEC_END_RDMA_VECTOR (pDrvCtrl)));    \
        if (FEC_END_WDMA_VECTOR (pDrvCtrl))                     \
           ret = intDisable ((int) (FEC_END_WDMA_VECTOR (pDrvCtrl)));   \
        }

#endif /* SYS_FEC_INT_DISABLE */

#define SYS_FEC_ENET_ADDR_GET(address)                                  \
if (sysFecEnetAddrGet != NULL)                                          \
    if (sysFecEnetAddrGet (pDrvCtrl->unit, (address)) == ERROR)         \
        {                                                               \
        errnoSet (S_iosLib_INVALID_ETHERNET_ADDRESS);                   \
        return (NULL);                                                  \
        }

#define SYS_FEC_ENET_ENABLE                                             \
if (sysFecEnetEnable != NULL)                                           \
    if (sysFecEnetEnable (pDrvCtrl->fecBaseAddr) == ERROR)              \
        return (ERROR);

#define SYS_FEC_ENET_DISABLE                                            \
if (sysFecEnetDisable != NULL)                                          \
    if (sysFecEnetDisable (pDrvCtrl->fecBaseAddr) == ERROR)             \
        return (ERROR);

#define FEC_END_DEV_NAME        "fec"
#define FEC_END_DEV_NAME_LEN    4
#define FEC_END_TBD_DEF_NUM     64      /* default number of TBDs */
#define FEC_END_RBD_DEF_NUM     48      /* default number of RBDs */
#define FEC_END_TX_CL_NUM       6       /* number of tx clusters */
#ifndef FEC_OVERWRITE_BD_NUM
#define FEC_END_BD_LOAN_NUM     64      /* loaned BDs */
#else
#define FEC_END_BD_LOAN_NUM     FEC_OVERWRITE_BD_NUM      /* loaned BDs */
#endif
#define FEC_END_TBD_MAX         128         /* max number of TBDs */
#define FEC_END_RBD_MAX         128         /* max number of TBDs */
#define FEC_END_WAIT_MAX    0xf0000000  /* max delay after sending */

#define FEC_END_ADDR_LEN        6               /* ethernet address length */

/* Control/Status Registers (CSR) definitions */

#define FEC_END_EVENT_OFF       0x0004  /* interrupt event register */
#define FEC_END_MASK_OFF        0x0008  /* interrupt mask register */
#define FEC_END_iaddr1_OFF      0x0118
#define FEC_END_iaddr2_OFF  	0x011C
#define FEC_END_CTRL_OFF        0x0024  /* FEC control register */
#define FEC_END_MII_DATA_OFF    0x0040 /* MII data register */
#define FEC_END_MII_SPEED_OFF   0x0044  /* MII speed register */
#define FEC_END_RX_CTRL_OFF     0x0084  /* rx control register */
#define FEC_END_TX_CTRL_OFF     0x00c4  /* tx control register */
#define FEC_END_ADDR_L_OFF      0x00e4  /* lower 32-bits of MAC address */
#define FEC_END_ADDR_H_OFF      0x00e8  /* upper 16-bits of MAC address */
#define FEC_END_OP_PAUSE_OFF    0x00EC
#define FEC_END_HASH_H_OFF      0x0120  /* upper 32-bits of hash table */
#define FEC_END_HASH_L_OFF      0x0124  /* lower 32-bits of hash table */

#define FEC_END_fifo_id_OFF         0x140
#define FEC_END_x_wmrk_OFF          0x144
#define FEC_END_fcntrl_OFF          0x148
#define FEC_END_r_bound_OFF         0x14C
#define FEC_END_r_fstart_OFF        0x150
#define FEC_END_r_count_OFF         0x154
#define FEC_END_r_lag_OFF           0x158
#define FEC_END_r_read_OFF          0x15C
#define FEC_END_r_write_OFF         0x160
#define FEC_END_x_count_OFF         0x164
#define FEC_END_x_lag_OFF           0x168
#define FEC_END_x_retry_OFF         0x16C
#define FEC_END_x_write_OFF         0x170
#define FEC_END_x_read_OFF          0x174
#define FEC_END_fm_cntrl_OFF        0x180
#define FEC_END_rfifo_data_OFF      0x184
#define FEC_END_rfifo_status_OFF    0x188
#define FEC_END_rfifo_cntrl_OFF     0x18C
#define FEC_END_rfifo_lrf_ptr_OFF   0x190
#define FEC_END_rfifo_lwf_ptr_OFF   0x194
#define FEC_END_rfifo_alarm_OFF     0x198
#define FEC_END_rfifo_rdptr_OFF     0x19C
#define FEC_END_rfifo_wrptr_OFF     0x1A0
#define FEC_END_tfifo_data_OFF      0x1A4
#define FEC_END_tfifo_status_OFF    0x1A8
#define FEC_END_tfifo_cntrl_OFF     0x1AC
#define FEC_END_tfifo_lrf_ptr_OFF   0x1B0
#define FEC_END_tfifo_lwf_ptr_OFF   0x1B4
#define FEC_END_tfifo_alarm_OFF     0x1B8
#define FEC_END_tfifo_rdptr_OFF     0x1BC
#define FEC_END_tfifo_wrptr_OFF     0x1C0
#define FEC_END_reset_cntrl_OFF     0x1C4
#define FEC_END_xmit_fsm_OFF        0x1C8

#define FEC_FIFO_STAT_ERROR         0x400000
#define FEC_FIFO_STAT_UF            0x200000
#define FEC_FIFO_STAT_OF            0x100000
#define FEC_FIFO_STAT_ALARM         0x020000

/* Control/Status Registers (CSR) bit definitions */

#define FEC_END_RX_START_MSK    0xfffffffc      /* quad-word alignment */
                                                /* required for rx BDs */

#define FEC_END_TX_START_MSK    0xfffffffc      /* quad-word alignment */
                                                /* required for tx BDs */
/* Ethernet CSR bit definitions */

#define FEC_END_ETH_EN          0x00000002      /* enable Ethernet operation */
#define FEC_END_ETH_DIS         0x00000000      /* disable Ethernet operation*/
#define FEC_END_ETH_RES         0x00000001      /* reset the FEC */
#define FEC_END_CTRL_MASK       0x00000003      /* FEC control register mask */

/*
 * interrupt bits definitions: these are common to both the
 * mask and the event register.
 */

#define FEC_END_EVENT_HB        0x80000000      /* heartbeat error */
#define FEC_END_EVENT_BABR      0x40000000      /* babbling rx error */
#define FEC_END_EVENT_BABT      0x20000000      /* babbling tx error */
#define FEC_END_EVENT_GRA       0x10000000      /* graceful stop complete */
#define FEC_END_EVENT_TXF       0x08000000      /* tx frame */
#define FEC_END_EVENT_MII       0x00800000      /* MII transfer */
#define FEC_END_EVENT_BERR      0x00400000      /* U-bus access error */
#define FEC_END_EVENT_LCOL      0x00200000      /* Late collision */
#define FEC_END_EVENT_COL_RL    0x00100000      /* Collision retry limit */
#define FEC_END_EVENT_XFIFO_UN  0x00080000      /* Transmit fifo underrun */
#define FEC_END_EVENT_FIFO_ERR  0x00040000      /* Transmit fifo error */
#define FEC_END_EVENT_RFIFO_ERR 0x00020000      /* Receive  fifo error */
#define FEC_END_EVENT_MSK       0xfffe0000      /* clear all interrupts */
#define FEC_END_MASK_ALL        FEC_END_EVENT_MSK    /* mask all interrupts */


/* bit masks for the interrupt level/vector CSR */

#define FEC_END_LVL_MSK         0xe0000000      /* intr level */
#define FEC_END_TYPE_MSK        0x0000000c      /* highest pending intr */
#define FEC_END_VEC_MSK         0xe000000c      /* this register mask */
#define FEC_END_RES_MSK         0x1ffffff3      /* reserved bits */
#define FEC_END_LVL_SHIFT       0x1d            /* intr level bits location */

/* transmit and receive active registers definitions */

#define FEC_END_TX_ACT          0x01000000      /* tx active bit */
#define FEC_END_RX_ACT          0x01000000      /* rx active bit */

/* MII management frame CSRs */

#define FEC_END_MII_ST          0x40000000      /* start of frame delimiter */
#define FEC_END_MII_OP_RD       0x20000000      /* perform a read operation */
#define FEC_END_MII_OP_WR       0x10000000      /* perform a write operation */
#define FEC_END_MII_ADDR_MSK    0x0f800000      /* PHY address field mask */
#define FEC_END_MII_REG_MSK     0x007c0000      /* PHY register field mask */
#define FEC_END_MII_TA          0x00020000      /* turnaround */
#define FEC_END_MII_DATA_MSK    0x0000ffff      /* PHY data field */
#define FEC_END_MII_RA_SHIFT    0x12            /* mii reg address bits */
#define FEC_END_MII_PA_SHIFT    0x17            /* mii PHY address bits */

#define FEC_END_MII_PRE_DIS     0x00000080      /* desable preamble */
#define FEC_END_MII_SPEED_25    0x00000005      /* recommended for 25Mhz CPU */
#define FEC_END_MII_SPEED_33    0x00000007      /* recommended for 33Mhz CPU */
#define FEC_END_MII_SPEED_40    0x00000008      /* recommended for 40Mhz CPU */
#define FEC_END_MII_SPEED_50    0x0000000a      /* recommended for 50Mhz CPU */
#define FEC_END_MII_SPEED_SHIFT 1       /* MII_SPEED bits location */
#define FEC_END_MII_CLOCK_MAX   2500000     /* max freq of MII clock (Hz)*/
#define FEC_END_MII_MAN_DIS     0x00000000      /* disable the MII management*/
                                                /* interface */
#define FEC_END_MII_SPEED_MSK   0xffffff81      /* speed field mask */

/* FIFO transmit and receive CSRs definitions */

#define FEC_END_FIFO_MSK        0x000003ff      /* FIFO rx/tx/bound mask */

/* SDMA function code CSR */

#define FEC_END_SDMA_DATA_BE    0x60000000      /* big-endian byte-ordering */
                                                /* for SDMA data transfer */

#define FEC_END_SDMA_DATA_PPC   0x20000000      /* PPC byte-ordering */
                                                /* for SDMA data transfer */

#define FEC_END_SDMA_DATA_RES   0x00000000      /* reserved value */

#define FEC_END_SDMA_BD_BE      0x18000000      /* big-endian byte-ordering */
                                                /* for SDMA BDs transfer */

#define FEC_END_SDMA_BD_PPC     0x08000000      /* PPC byte-ordering */
                                                /* for SDMA BDs transfer */


#define FEC_END_SDMA_BD_RES     0x00000000      /* reserved value */
#define FEC_END_SDMA_FUNC_0     0x00000000      /* no function code */

/* receive control/hash registers bit definitions */

#define FEC_END_RX_CTRL_PROM    0x00000008      /* promiscous mode */
#define FEC_END_RX_CTRL_MII     0x00000004      /* select MII interface */
#define FEC_END_RX_CTRL_DRT     0x00000002      /* disable rx on transmit */
#define FEC_END_RX_CTRL_LOOP    0x00000001      /* loopback mode */
#define FEC_END_RX_FR_MSK       0x000007ff      /* rx frame length mask */


/* transmit control register bit definitions */

#define FEC_END_TX_CTRL_FD      0x00000004      /* enable full duplex mode */
#define FEC_END_TX_CTRL_HBC     0x00000002      /* HB check is performed */
#define FEC_END_TX_CTRL_GRA     0x00000001      /* issue a graceful tx stop */

/* rx/tx buffer descriptors definitions */

#define FEC_END_RBD_SZ          8       /* RBD size in byte */
#define FEC_END_TBD_SZ          8       /* TBD size in byte */
#define FEC_END_TBD_MIN         6       /* min number of TBDs */
#define FEC_END_RBD_MIN         4       /* min number of RBDs */
#define FEC_END_TBD_POLL_NUM    1       /* one TBD for poll operation */
#define CL_OVERHEAD             4       /* prepended cluster overhead */
#define CL_ALIGNMENT            4       /* cluster required alignment */
#define MBLK_ALIGNMENT          4       /* mBlks required alignment */
#define FEC_END_BD_ALIGN        0x20    /* required alignment for RBDs */
#define FEC_END_MAX_PCK_SZ      (ETHERMTU + SIZEOF_ETHERHEADER          \
                                 + ETHER_CRC_LEN)

#define FEC_END_BD_STAT_OFF     0       /* offset of the status word */
#define FEC_END_BD_LEN_OFF      2       /* offset of the data length word */
#define FEC_END_BD_ADDR_OFF     4       /* offset of the data pointer word */

#define BESTCOMM_API  11

#ifndef BESTCOMM_API
/* TBD bits definitions */

#define FEC_END_TBD_RDY         0x8000          /* ready for transmission */
#define FEC_END_TBD_WRAP        0x2000          /* look at CSR5 for next bd */
#define FEC_END_TBD_INT         0x1000          /* Interrupt */
#define FEC_END_TBD_LAST        0x0800          /* last bd in this frame */
#define FEC_END_TBD_CRC         0x0400          /* transmit the CRC sequence */
#define FEC_END_TBD_DEF         0x0200          /* Append bad CRC  */
#define FEC_END_TBD_HB          0x0100          /* heartbeat error */
#define FEC_END_TBD_LC          0x0080          /* late collision */
#define FEC_END_TBD_RL          0x0040          /* retransmission limit */
#define FEC_END_TBD_UN          0x0002          /* underrun error */
#define FEC_END_TBD_CSL         0x0001          /* carrier sense lost */
#define FEC_END_TBD_RC_MASK     0x003c          /* retransmission count mask */

/* RBD bits definitions */

#define FEC_END_RBD_EMPTY       0x8000          /* ready for reception */
#define FEC_END_RBD_WRAP        0x2000          /* look at CSR4 for next bd */
#define FEC_END_RBD_INT         0x1000          /* BD int bit in status */
#define FEC_END_RBD_LAST        0x0800          /* last bd in this frame */
#define FEC_END_RBD_MISS        0x0100          /* address recognition miss */
#define FEC_END_RBD_BC          0x0080          /* broadcast frame */
#define FEC_END_RBD_MC          0x0040          /* multicast frame */
#define FEC_END_RBD_LG          0x0020          /* frame length violation */
#define FEC_END_RBD_NO          0x0010          /* nonoctet aligned frame */
#define FEC_END_RBD_SH          0x0008          /* short frame error */
                                                /* not supported by the 860T */
#define FEC_END_RBD_CRC         0x0004          /* CRC error */
#define FEC_END_RBD_OV          0x0002          /* overrun error */
#define FEC_END_RBD_TR          0x0001          /* truncated frame (>2KB) */

#define FEC_END_RBD_ERR         (FEC_END_RBD_LG  |                      \
                                 FEC_END_RBD_NO  |                      \
                                 FEC_END_RBD_CRC |                      \
                                 FEC_END_RBD_OV  |                      \
                                 FEC_END_RBD_TR)
#else
#define FEC_END_RBD_EMPTY        (SDMA_BD_MASK_READY>>16)  /* ready for RX */
#define FEC_END_TBD_RDY          (SDMA_BD_MASK_READY>>16)  /* ready for TX */
#define FEC_END_TBD_LAST         (TASK_BD_TFD>>16)        /* ready for TX */
#define FEC_END_TBD_INT          (TASK_BD_INT>>16)   /* BD int bit in status */
#define FEC_END_TBD_CRC          0

/* if RBD contains RxFIO Status at the end */
#define FEC_END_RBD_FEMPTY       0x0001          /* FIFO run empty */
#define FEC_END_RBD_ALARM        0x0002          /* FIFO alarm */
#define FEC_END_RBD_FULL         0x0004          /* FIFO is full */
#define FEC_END_RBD_FR           0x0008          /* frame data ready */
#define FEC_END_RBD_OV           0x0010          /* FIFO overflow */
#define FEC_END_RBD_UF           0x0020          /* FIFO underflow */

#define FEC_END_RBD_ERR         (FEC_END_RBD_FEMPTY  |       \
                                 FEC_END_RBD_ALARM  |        \
                                 FEC_END_RBD_FULL |          \
                                 FEC_END_RBD_OV  |           \
                                 FEC_END_RBD_UF)
#endif

#define FEC_END_CRC_POLY    0x04c11db7  /* CRC polynomium: */
                        /* x^32 + x^26 + x^23 + */
                        /* x^22 + x^16 + x^12 + */
                        /* x^11 + x^10 + x^8  + */
                        /* x^7  + x^5  + x^4  + */
                        /* x^2  + x^1  + x^0  + */

#define FEC_END_HASH_MASK   0x7c000000  /* bits 27-31 */
#define FEC_END_HASH_SHIFT  0x1a        /* to get the index */

/* defines related to the PHY device */

#define FEC_END_PHY_PRE_INIT    0x0001      /* PHY info initialized */
#define FEC_END_PHY_AUTO    0x0010      /* enable auto-negotiation */
#define FEC_END_PHY_TBL     0x0020      /* use negotiation table */
#define FEC_END_PHY_100     0x0040      /* PHY may use 100Mbit speed */
#define FEC_END_PHY_10      0x0080      /* PHY may use 10Mbit speed */
#define FEC_END_PHY_FD      0x0100      /* PHY may use full duplex */
#define FEC_END_PHY_HD      0x0200      /* PHY may use half duplex */
#define FEC_END_PHY_MAX_WAIT    0x100       /* max delay before */
#define FEC_END_PHY_NULL    0xff        /* PHY is not present */
#define FEC_END_PHY_DEF     0x0     /* PHY's logical address */

/* allowed PHY's speeds */

#define FEC_END_100MBS      100000000       /* bits per sec */
#define FEC_END_10MBS       10000000        /* bits per sec */

/*
 * user flags: full duplex mode, loopback mode, serial interface etc.
 * the user may configure some of this options according to his needs
 * by setting the related bits in the <userFlags> field of the load string.
 */

#define FEC_END_USR_PHY_NO_AN   0x00000001  /* do not auto-negotiate */
#define FEC_END_USR_PHY_TBL 0x00000002  /* use negotiation table */
#define FEC_END_USR_PHY_NO_FD   0x00000004  /* do not use full duplex */
#define FEC_END_USR_PHY_NO_100  0x00000008  /* do not use 100Mbit speed */
#define FEC_END_USR_PHY_NO_HD   0x00000010  /* do not use half duplex */
#define FEC_END_USR_PHY_NO_10   0x00000020  /* do not use 10Mbit speed */
#define FEC_END_USR_PHY_ISO 0x00000100  /* isolate a PHY */
#define FEC_END_USR_SER     0x00000200  /* 7-wire serial interface */
#define FEC_END_USR_LOOP    0x00000400  /* loopback mode */
                        /* only use it for testing */
#define FEC_END_USR_HBC     0x00000080  /* perform heartbeat control */

#define FEC_END_TBD_OK      0x1     /* the TBD is a good one */
#define FEC_END_TBD_BUSY    0x2     /* the TBD has not been used */
#define FEC_END_TBD_ERROR   0x4     /* the TBD is errored */

#define PKT_TYPE_MULTI      0x1 /* packet with a multicast address */
#define PKT_TYPE_UNI        0x2 /* packet with a unicast address */
#define PKT_TYPE_NONE       0x4 /* address type is not meaningful */

#define BUF_TYPE_CL     0x1 /* this's a cluster pointer */
#define BUF_TYPE_MBLK       0x2 /* this's a mblk pointer */
#define BUF_TYPE_LOCAL      0x4 /* local TX buffer (not from
                                           netBufLib) */

/* frame descriptors definitions */

typedef char *          FEC_END_BD_ID;
typedef FEC_END_BD_ID       FEC_END_TBD_ID;
typedef FEC_END_BD_ID       FEC_END_RBD_ID;

/* MII definitions */

#define MII_MAX_PHY_NUM     0x20    /* max number of attached PHYs */

#define MII_CTRL_REG        0x0 /* Control Register */
#define MII_STAT_REG        0x1 /* Status Register */
#define MII_PHY_ID1_REG     0x2 /* PHY identifier 1 Register */
#define MII_PHY_ID2_REG     0x3 /* PHY identifier 2 Register */
#define MII_AN_ADS_REG      0x4 /* Auto-Negotiation       */
                    /* Advertisement Register */
#define MII_AN_PRTN_REG     0x5 /* Auto-Negotiation         */
                    /* partner ability Register */
#define MII_AN_EXP_REG      0x6 /* Auto-Negotiation   */
                    /* Expansion Register */
#define MII_AN_NEXT_REG     0x7 /* Auto-Negotiation            */
                    /* next-page transmit Register */

/* MII control register bit  */

#define MII_CR_COLL_TEST    0x0080      /* collision test */
#define MII_CR_FDX      0x0100      /* FDX =1, half duplex =0 */
#define MII_CR_RESTART      0x0200      /* restart auto negotiation */
#define MII_CR_ISOLATE      0x0400      /* isolate PHY from MII */
#define MII_CR_POWER_DOWN   0x0800      /* power down */
#define MII_CR_AUTO_EN      0x1000      /* auto-negotiation enable */
#define MII_CR_100      0x2000      /* 0 = 10mb, 1 = 100mb */
#define MII_CR_LOOPBACK     0x4000      /* 0 = normal, 1 = loopback */
#define MII_CR_RESET        0x8000      /* 0 = normal, 1 = PHY reset */
#define MII_CR_NORM_EN      0x0000      /* just enable the PHY */

/* MII Status register bit definitions */

#define MII_SR_LINK_STATUS  0x0004          /* link Status -- 1 = link */
#define MII_SR_AUTO_SEL     0x0008          /* auto speed select capable */
#define MII_SR_REMOTE_FAULT     0x0010          /* Remote fault detect */
#define MII_SR_AUTO_NEG         0x0020          /* auto negotiation complete */
#define MII_SR_10T_HALF_DPX     0x0800      /* 10BaseT HD capable */
#define MII_SR_10T_FULL_DPX     0x1000      /* 10BaseT FD capable */
#define MII_SR_TX_HALF_DPX      0x2000      /* TX HD capable */
#define MII_SR_TX_FULL_DPX      0x4000      /* TX FD capable */
#define MII_SR_T4               0x8000      /* T4 capable */

/* MII Link Code word  bit definitions */

#define MII_BP_FAULT    0x2000          /* remote fault */
#define MII_BP_ACK  0x4000          /* acknowledge */
#define MII_BP_NP   0x8000          /* nexp page is supported */

/* MII Next Page bit definitions */

#define MII_NP_TOGGLE   0x0800          /* toggle bit */
#define MII_NP_ACK2 0x1000          /* acknowledge two */
#define MII_NP_MSG  0x2000          /* message page */
#define MII_NP_ACK1 0x4000          /* acknowledge one */
#define MII_NP_NP   0x8000          /* nexp page will follow */

/* MII Expansion Register bit definitions */

#define MII_EXP_FAULT   0x0010          /* parallel detection fault */
#define MII_EXP_PRTN_NP 0x0008          /* link partner next-page able */
#define MII_EXP_LOC_NP  0x0004          /* local PHY next-page able */
#define MII_EXP_PR  0x0002          /* full page received */
#define MII_EXP_PRT_AN  0x0001          /* link partner auto negotiation able*/

/* technology ability field bit definitions */

#define MII_TECH_10BASE_T   0x0020  /* 10Base-T */
#define MII_TECH_10BASE_FD  0x0040  /* 10Base-T Full Duplex */
#define MII_TECH_100BASE_TX 0x0080  /* 100Base-TX */
#define MII_TECH_100BASE_TX_FD  0x0100  /* 100Base-TX Full Duplex */
#define MII_TECH_100BASE_T4 0x0200  /* 100Base-T4 */
#define MII_TECH_MASK		0x1fe0

#define MII_AN_FAIL     0x10    /* auto-negotiation fail */
#define MII_STAT_FAIL       0x20    /* errors in the status register */
#define FEC_END_PHY_NO_ABLE 0x40    /* the PHY lacks some abilities */

/* FEC_Lite FIFO Transmit Watermark Register(X_WMRK) definitions */

#define FEC_XFIFO_WMRK_64        0x00000000  /* TX FIFO watermark(64 bytes) */
#define FEC_XFIFO_WMRK_128       0x00000001  /* TX FIFO watermark(128 bytes) */
#define FEC_XFIFO_WMRK_192       0x00000002  /* TX FIFO watermark(192 bytes) */
#define FEC_XFIFO_WMRK_256       0x00000003  /* TX FIFO watermark(256 bytes) */

/* FEC_Lite Opcode/Pause Duration Register(OP_PAUSE) definitions */

#define FEC_OP_PAUSE_OPCODE      0x00010000  /* Opcode field used in PAUSE
                                                frames */

/* settings */

/* FEC-Lite RFIFO Control Register (RFIFO_CNTRL) definitions */

#define FEC_RFIFO_CNTRL_FRAME    0x08000000  /* Frame mode enable */

#ifdef BESTCOMM_API
#define FEC_RFIFO_CNTRL_GR       0x07000000  /* Default Granularity value = 7*/
#else
#define FEC_RFIFO_CNTRL_GR       0x04000000  /* Default Granularity value = 4*/
#endif

/* FEC-Lite RFIFO Alarm Register (RFIFO_ALARM) definitions */
#define FEC_RFIFO_ALARM          0x0000030C  /* Default value is 520 bytes */

/* FEC-Lite XFIFO Control Register(XFIFO_CNTRL) definitions */

#define FEC_XFIFO_CNTRL_FRAME    0x08000000  /* Frame mode enable */

#ifdef BESTCOMM_API
#define FEC_XFIFO_CNTRL_GR       0x07000000  /* Default Granularity value = 7*/
#else
#define FEC_XFIFO_CNTRL_GR       0x04000000  /* Default Granularity value = 4*/
#endif

#define FEC_XFIFO_CNTRL_TXW_MASK 0x00040000  /* Undocumented bit to set w/Rev.B */

#define FEC_PAUSE_DURATION       0x0020  /* Pause transmission for the
                                            duration */

/*
 * this table may be customized by the user in configNet.h
 */

IMPORT INT16 m5200FecPhyAnOrderTbl [];


typedef struct m5200fec_mii_regs
    {
    UINT16      phyStatus;  /* PHY's status register */
    UINT16      phyCtrl;    /* PHY's control register */
    UINT16      phyId1;     /* PHY's identifier field 1 */
    UINT16      phyId2;     /* PHY's identifier field 2 */
    UINT16      phyAds;     /* PHY's advertisement register */
    UINT16      phyPrtn;    /* PHY's partner register */
    UINT16      phyExp;     /* PHY's expansion register */
    UINT16      phyNext;    /* PHY's next paget transmit register */
    } M5200FEC_MII_REGS;

typedef struct m5200fec_phy_info
    {
    M5200FEC_MII_REGS        miiRegs;    /* PHY registers */
    UINT8       phyAddr;    /* address of the PHY to be used */
    UINT8       isoPhyAddr; /* address of a PHY to isolate */
    UINT32      phyFlags;   /* some flags */
    UINT32              phySpeed;       /* 10/100 Mbit/sec */
    UINT32              phyMode;        /* half/full duplex mode */
    UINT32              phyDefMode;     /* default operating mode */
    } M5200FEC_PHY_INFO;

typedef struct mot_fec_tbd_list
    {
    UINT16          fragNum;
    UINT16          pktType;
    UCHAR *         pBuf;
    UINT16          bufType;
    struct mot_fec_tbd_list *   pNext;
    FEC_END_TBD_ID      pTbd;
    } FEC_END_TBD_LIST;

typedef FEC_END_TBD_LIST *  FEC_END_TBD_LIST_ID;

/* The definition of the driver control structure */

typedef struct drv_ctrl
    {
    END_OBJ             endObj;         /* base class */
    int                 unit;           /* unit number */
    UINT32              fecBaseAddr;    /* internal RAM base address */
    VOIDFUNCPTR *       ivecFEC;        /* interrupt vector number FEC */
    VOIDFUNCPTR *       ivecRDMA;       /* interrupt vector number RDMA */
    VOIDFUNCPTR *       ivecWDMA;       /* interrupt vector number WDMA */
    UINT32              fifoTxBase;     /* address of Tx FIFO in internal RAM*/
    UINT32              fifoRxBase;     /* address of Rx FIFO in internal RAM*/
    char *              pBufBase;       /* FEC memory pool base */
    ULONG               bufSize;        /* FEC memory pool size */
    UINT16              rbdNum;         /* number of RBDs */
#ifndef BESTCOMM_API
    FEC_END_RBD_ID  rbdBase;        /* RBD ring */
#else
    int         recvTaskNo;   /* Bestcomm Task for Receive */
    int         rxPollTID;
 #endif
    UINT16              rbdIndex;       /* RBD index */
    UINT16              tbdNum;         /* number of TBDs */
    FEC_END_TBD_ID  tbdBase;        /* TBD ring */
 #ifdef BESTCOMM_API
    int         xmitTaskNo; /* Bestcomm Task for Transmit */
 #endif
    UINT16              tbdIndex;       /* TBD index */
    UINT16              usedTbdIndex;   /* used TBD index */
    volatile UINT16     cleanTbdNum;    /* number of clean TBDs */
    BOOL                txStall;        /* tx handler stalled - no Tbd */
	BOOL                txHandlerScheduled;  /* kp: tx handler scheduled */
    FEC_END_TBD_LIST *  pTbdList [FEC_END_TBD_MAX];
                    /* list of TBDs */
    ULONG               userFlags;      /* some user flags */
    INT8                flags;          /* driver state */
    BOOL                loaded;         /* interface has been loaded */
    BOOL                intrConnect;    /* interrupt has been connected */
    UINT32      intMask;    /* interrupt mask register */
    UCHAR *     pTxPollBuf; /* cluster pointer for poll mode */
    UCHAR *             rxBuf[FEC_END_RBD_MAX];
                                        /* array of pointers to clusters */
    SEM_ID      miiSem;     /* synch semaphore for mii frames */
    SEM_ID      graSem;     /* synch semaphore for graceful */
                    /* transmit command */
    char *              pClBlkArea;     /* cluster block pointer */
    UINT32              clBlkSize;      /* clusters block memory size */
    char *              pMBlkArea;      /* mBlock area pointer */
    UINT32              mBlkSize;       /* mBlocks area memory size */
    CACHE_FUNCS         bdCacheFuncs;   /* cache descriptor */
    CACHE_FUNCS         bufCacheFuncs;  /* cache descriptor */
    CL_POOL_ID          pClPoolId;      /* cluster pool identifier */
    M5200FEC_PHY_INFO        *phyInfo;   /* info on a MII-compliant PHY */
    UINT32      clockSpeed; /* clock speed (Hz) for MII_SPEED */
    UINT8               txBuffAvailable; /* Flag for use with pTxBuffLocal */
    char *              pTxBuffLocal;    /* Local buffer for sending data */
    UINT32              stoppingTx;     /* Flag to stop sending frames after */
                                        /* fecStop has been scheduled */
    } DRV_CTRL;

/*
 * this cache functions descriptors is used to flush/invalidate
 * the FEC's data buffers. They are set to the system's cache
 * flush and invalidate routine. This will allow proper operation
 * of the driver if data cache are turned on.
 */

IMPORT STATUS   cacheArchInvalidate (CACHE_TYPE, void *, size_t);

IMPORT STATUS   cacheArchFlush (CACHE_TYPE, void *, size_t);

IMPORT STATUS   sysFecEnetAddrGet (UINT32 motCmpAddr, UCHAR * address);
IMPORT STATUS   sysEnetAddrSet (UINT32 motCmpAddr, UCHAR * address);
IMPORT STATUS   sysFecEnetEnable (UINT32 motCmpAddr);
IMPORT STATUS   sysFecEnetDisable (UINT32 motCmpAddr);
IMPORT FUNCPTR  _func_m5200FecPhyInit;
IMPORT FUNCPTR  _func_m5200FecHbFail;


/* export function defined in m5200FecEnd.c */
extern STATUS  m5200FecMiiRead (DRV_CTRL *pDrvCtrl, UINT8 phyAddr,
         	                    UINT8 regAddr, UINT16 *retVal);

extern STATUS  m5200FecMiiWrite (DRV_CTRL *pDrvCtrl, UINT8 phyAddr,
                				 UINT8 regAddr, UINT16 writeData);

#ifdef __cplusplus
}
#endif

#endif /* __INCm5200FecEndh */

