#ifndef __TASK_API_BESTCOMM_API_MEM_H
#define __TASK_API_BESTCOMM_API_MEM_H 1

/******************************************************************************
*
*       COPYRIGHT (c) 2001-2004 MOTOROLA INC.
*       ALL RIGHTS RESERVED
*
*       The code is the property of Motorola Semiconductor Products Sector.
*
*       The copyright notice above does not evidence any
*       actual or intended publication of such source code.
*
* Filename:     $Source: /Customizations/VxWorks-6.0/mpc5200/target/config/lite5200.pkr/bestComm/capi/task_api/bestcomm_api_mem.h,v $
* Author:       $Author: pkleiner $
* Locker:       $Locker:  $
* State:        $State: Exp $
* Revision:     $Revision: 1.1 $
*
******************************************************************************/

#include "mgt5200/mgt5200.h"

/*
 * An extern global variable is used here for the MBAR since it must 
 * be passed into the API for processes that use virtual memory.
 */
extern uint8 *MBarGlobal;

#define SDMA_TASK_BAR      (MBarGlobal+MBAR_SDMA+0x000)
#define SDMA_INT_PEND      (MBarGlobal+MBAR_SDMA+0x014)
#define SDMA_INT_MASK      (MBarGlobal+MBAR_SDMA+0x018)
#define SDMA_TCR           (MBarGlobal+MBAR_SDMA+0x01C)
#define SDMA_TASK_SIZE     (MBarGlobal+MBAR_SDMA+0x060)

#define PCI_TX_PKT_SIZE    (MBarGlobal+MBAR_SCPCI+0x000)
#define PCI_TX_NTBIT       (MBarGlobal+MBAR_SCPCI+0x01C)
#define PCI_TX_FIFO        (MBarGlobal+MBAR_SCPCI+0x040)
#define PCI_TX_FIFO_STAT   (MBarGlobal+MBAR_SCPCI+0x045)
#define PCI_TX_FIFO_GRAN   (MBarGlobal+MBAR_SCPCI+0x048)
#define PCI_TX_FIFO_ALARM  (MBarGlobal+MBAR_SCPCI+0x04E)

#define PCI_RX_PKT_SIZE    (MBarGlobal+MBAR_SCPCI+0x080)
#define PCI_RX_NTBIT       (MBarGlobal+MBAR_SCPCI+0x09C)
#define PCI_RX_FIFO        (MBarGlobal+MBAR_SCPCI+0x0C0)
#define PCI_RX_FIFO_STAT   (MBarGlobal+MBAR_SCPCI+0x0C5)
#define PCI_RX_FIFO_GRAN   (MBarGlobal+MBAR_SCPCI+0x0C8)
#define PCI_RX_FIFO_ALARM  (MBarGlobal+MBAR_SCPCI+0x0CE)


#define FEC_RX_FIFO        (MBarGlobal+MBAR_ETHERNET+0x184)
#define FEC_RX_FIFO_STAT   (MBarGlobal+MBAR_ETHERNET+0x188)
#define FEC_RX_FIFO_GRAN   (MBarGlobal+MBAR_ETHERNET+0x18C)
#define FEC_RX_FIFO_ALARM  (MBarGlobal+MBAR_ETHERNET+0x198)

#define FEC_TX_FIFO        (MBarGlobal+MBAR_ETHERNET+0x1A4)
#define FEC_TX_FIFO_STAT   (MBarGlobal+MBAR_ETHERNET+0x1A8)
#define FEC_TX_FIFO_GRAN   (MBarGlobal+MBAR_ETHERNET+0x1AC)
#define FEC_TX_FIFO_ALARM  (MBarGlobal+MBAR_ETHERNET+0x1B8)

#endif	/* __TASK_API_BESTCOMM_API_MEM_H */
