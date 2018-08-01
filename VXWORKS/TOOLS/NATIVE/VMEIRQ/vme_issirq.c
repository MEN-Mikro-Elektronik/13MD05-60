/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: vme_issirq.c
 *      Project: A14 VME Interrupter test
 *
 *       Author: ts
 *        $Date: $
 *    $Revision: $
 *
 *  Description: issues vme interrupts. call with vme_issirq(<lev>,<vector>)
 *				 to be used with vme_rcvirq on opponent VME board
 *
 *     Required: -
 *     Switches: -
 *
 *		   Note: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: $
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#include "vxWorks.h"
/*#include "config.h"
*/
#include <MEN/men_typs.h>
#include <MEN/dbg.h>
#include <MEN/oss.h>
#include <MEN/maccess.h>

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "memLib.h"
#include "sysLib.h"
#include "tickLib.h"
#include "taskLib.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"
#include "vme_irq.h"
#include "MEN/men_typs.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/


#ifndef CPU_PCI_IO_ADRS
	#if ((CPU == I80486) | (CPU == I80386) | (CPU == PENTIUM))
		#define CPU_PCI_IO_ADRS		0
	#else	
		#define CPU_PCI_IO_ADRS     0x80000000   /* base of PCI I/O addr */
	#endif
#endif /* CPU_PCI_IO_ADRS */


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

static void vme_issueIrq( UINT32 base, u_int8 v, u_int8 l)
{

	/* actually issue the Interrupt */
	MWRITE_D8(base, PLDZ002_BRIDGE_CTRL + PLDZ002_INTID, v );
	MWRITE_D8(base, PLDZ002_BRIDGE_CTRL + PLDZ002_INTR,  l );
	MWRITE_D8(base, PLDZ002_BRIDGE_CTRL + PLDZ002_INTR,
			  ( l | PLDZ002_INTR_INTEN) );

}


STATUS vme_issirq( u_int8 vector, u_int8 level )
{
	int     	pciBus;
    int     	pciDev;
    int  		pciFunc;
    UINT32		baseAddr;	/* BAR0 */

	printf("\nA14 VME IRQ test. Use: vme_issirq <vector> <level>\n");

	/*
	 *	detect Z002 PLD
	 */
	if(	pciFindDevice( PLDZ002_PCI_VEN_ID, PLDZ002_PCI_DEV_ID, 0,
					   &pciBus, &pciDev, &pciFunc))
	{
		printf("*** could not Find VME PCI device 0x%04x / 0x%04x\n",
			   PLDZ002_PCI_VEN_ID, PLDZ002_PCI_DEV_ID );
		return -1;
	}

	printf("PLD Z002 detected. issue vector %02x level %02x\n",
		   vector, level);

	/*
	 *	Get BAR0 to issue interrupter Register values
	 */
	pciConfigInLong ( pciBus, pciDev, pciFunc, PCI_CFG_BASE_ADDRESS_0,
					  &baseAddr);
	printf("BAR0 base address: 0x%08x\n", baseAddr );

	/*
	 *  do the Interrupter access
	 */		
	vme_issueIrq( baseAddr, vector, level);

	return OK;		
}









