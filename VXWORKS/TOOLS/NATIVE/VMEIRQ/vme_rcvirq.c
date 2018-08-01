/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: vme_rcvirq.c
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


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

static u_int32 G_bar0;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


static void vme_isr( int dummy )
{
	u_int8 vector=0, level=0, istat, i;

    istat = MREAD_D8( G_bar0 + PLDZ002_BRIDGE_CTRL, 	PLDZ002_ISTAT );
	istat &= MREAD_D8( G_bar0 + PLDZ002_BRIDGE_CTRL, 	PLDZ002_IMASK );

    if( istat != 0 ){
		/*--- decode *levP  ---*/
		if(      istat & 0x80 ) level = PLDZ002_IRQLEV_7; 
		else if( istat & 0x40 ) level = PLDZ002_IRQLEV_6;
		else if( istat & 0x20 ) level = PLDZ002_IRQLEV_5;
		else if( istat & 0x10 ) level = PLDZ002_IRQLEV_4;
		else if( istat & 0x08 ) level = PLDZ002_IRQLEV_3;
		else if( istat & 0x04 ) level = PLDZ002_IRQLEV_2;
		else if( istat & 0x02 ) level = PLDZ002_IRQLEV_1;
		else if( istat & 0x01 ){
			/* ACFAIL not active */
			;
		}

		}

	/* fetch vector (VME IACK cycle) */
	/*vector=*(volatile u_int8 *)((G_bar0 +PLDZ002_IACK_SPACE)+(level<<1)+1);*/
	vector = MREAD_D8( G_bar0 + PLDZ002_IACK_SPACE, ( level<<1 ) + 1 );

	logMsg("level 0x%02x vector 0x%02x\n", level, vector);

	for (i=0; i<8; i++){
		vector = MREAD_D8( G_bar0 + PLDZ002_IACK_SPACE, (i<<1 ) +1 );
		logMsg("v%02x vector 0x%02x\n", i, vector);
	}
	/* dummy access to CTRL space to clear IACK LED on VME tracer */
	/* istat = MREAD_D8( G_bar0 + PLDZ002_BRIDGE_CTRL,  PLDZ002_IMASK ); */
}



STATUS vme_rcvirq()
{
	int     	pciBus;
    int     	pciDev;
    int  		pciFunc;
	u_int8		irq=0;
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

	/*	Get BAR0 to access IMASK and other regs */
	pciConfigInLong ( pciBus, 
					  pciDev, 
					  pciFunc, 
					  PCI_CFG_BASE_ADDRESS_0, 
					  &G_bar0);

	/* get IRQ line .. should be 0x9 for INTB */
	pciConfigInByte( pciBus, 
					 pciDev, 
					 pciFunc, 
					 PCI_CFG_DEV_INT_LINE, 
					 &irq );

	/* enable reception of all VME irqs, no ACFAIL */
	MWRITE_D8(G_bar0, PLDZ002_BRIDGE_CTRL + PLDZ002_IMASK, 0xfe );
	
 	printf("connect PLDZ002 (BAR0=0x%08x) to irq 0x%02x\n", G_bar0, irq );

	/* connect IRQ and enable it */
	pciIntConnect((int)irq, vme_isr, G_bar0);
	intEnable( irq );

	return OK;		
}









