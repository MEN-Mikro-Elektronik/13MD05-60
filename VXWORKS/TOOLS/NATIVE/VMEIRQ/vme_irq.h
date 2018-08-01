/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: 
 *      Project: A14 VME Interrupter test
 *
 *       Author: ts
 *        $Date: $
 *    $Revision: $
 *
 *  Description: tool to issue vme interrupts. call with 
 *			     vme_issirq( <level>, <vector>)
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
#ifndef _MCRW_H
#  define _MCRW_H

#  ifdef __cplusplus
      extern "C" {
#  endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

/* VME IRQ generating side */

/* offset relative to BAR0 */
#define PLDZ002_BRIDGE_CTRL		0x1800000
#define PLDZ002_IACK_SPACE		0x1c00000

#define PLDZ002_INTR 			0x0000
#define PLDZ002_INTID			0x0004
#define PLDZ002_ISTAT			0x0008
#define PLDZ002_IMASK			0x000c
#define PLDZ002_MSTR			0x0010

#define PLDZ002_INTR_INTEN 		8	/* bit3 */

#define PLDZ002_PCI_DEV_ID		0x5056
#define PLDZ002_PCI_VEN_ID		0x1172


#define PLDZ002_IRQLEV_UNKNOWN		0 /**< IRQ level unknown */
#define PLDZ002_IRQLEV_1			1 /**< VME IRQ level 1   */
#define PLDZ002_IRQLEV_2			2 /**< VME IRQ level 2   */
#define PLDZ002_IRQLEV_3			3 /**< VME IRQ level 3   */
#define PLDZ002_IRQLEV_4			4 /**< VME IRQ level 4   */
#define PLDZ002_IRQLEV_5			5 /**< VME IRQ level 5   */
#define PLDZ002_IRQLEV_6			6 /**< VME IRQ level 6   */
#define PLDZ002_IRQLEV_7			7 /**< VME IRQ level 7   */


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


#  ifdef __cplusplus
      }
#  endif

#endif/*_MCRW_H*/

