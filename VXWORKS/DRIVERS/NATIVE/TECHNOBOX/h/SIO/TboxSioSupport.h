/* TboxSioSupport.h - configuration file for Technobox Serial Port Installation */

/******************************************************************************
 *
 * Copyright 2007 by Technobox
 *
 * Functions: This file contains the register and macro definitions.
 *
 * Revision:
 * REV   WHO   DATE       SWF#   COMMENTS
 * ----+-----+----------+------+-----------------------------------------------
 * 006   SGL   07/06/07   026    Moved Dev & Ven ID outside ifdef
 * 005   SGL   06/15/07   026    Separated BAR and Port programming
 *                               Update Part Numbers to Lead Free Ones
 * 004   SGL   04/12/06   030    Moved PCI and CPU BSP addresses to config file
 *                               Removed 4382 support.
 * 003   SGL   06/20/05   025    Updated constants
 * 002   SGL   03/24/04          Added Serial ATA Card Serial Ports.
 * 001   SGL   03/11/04          Updated to Support two of each Adapter
 * 000   SGL   03/10/04          Initial Release
 *
 *****************************************************************************/
#ifndef __TboxSioSupport_h__
#define __TboxSioSupport_h__

/* Include the MACRO configuration file */
#include "..\..\h\technobox_vxworks_config.h"

/*
DESCRIPTION
This file contains the definitions for installation of the Technobox Serial Port
Adapters.  The user needs to update the appropriate definitions for their
architecture and define which Adapter is being included.

In addition the user may want to define INCLUDE_TBOX_SIO to gates includes or defines

*/

#ifdef __cplusplus
extern "C" {
#endif

/* TODO.  Define the appropriate INCLUDE below to include support for that Adapter.
 * The generic code supplied will only work with a single Adapter installed in the system.
 * Refer to the documentation for changes needed to support more than one adapter or
 * more than one type of adapter.
 * Fill in the Number of Adapters installed in the system
 *
 * If your BSP does not program the PCI BAR registers then define INCLUDE_PCI_BAR_PROG
 * to enable programming of the PCI BAR register.
 */
#undef		INCLUDE_TBOX_4960		/* Change undef to define to include Technobox 2012, 4960 */
#define		TBOX_4960_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#define		INCLUDE_TBOX_5284		/* Change undef to define to include Technobox 2229, 5284 */
#define		TBOX_5284_NUM_CARDS	1	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_TBOX_5436		/* Change undef to define to include Technobox 2238, 5436 */
#define		TBOX_5436_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_TBOX_5288		/* Change undef to define to include Technobox 2316, 5288 */
#define		TBOX_5288_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_TBOX_4966		/* Change undef to define to include Technobox 2901, 4966 */
#define		TBOX_4966_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_TBOX_5382		/* Change undef to define to include Technobox 3101, 5382 */
#define		TBOX_5382_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_TBOX_3607		/* Change undef to define to include Technobox 3607 */
#define		TBOX_3607_NUM_CARDS	0	/* Number of Cards in the system (0, 1, or 2) */

#undef		INCLUDE_PCI_BAR_PROG	/* Change undef to define to include defines to program */
									/* .. the Technobox Adapters PCI BAR registers */


/* TODO.
 * Change the default baud rate used for all ports
 */
#define		TBOX_SIO_BAUD_RATE	9600

#ifdef INCLUDE_PCI_BAR_PROG

/* TODO
 * Define the TBOXPMC1IRQ and TBOXPMC2IRQ here or in configuration file or replace
 * with the actual IRQ definition.  
 * These are values which match cards used for testing purposes only and do not 
 * necessarily reflect your actual Base Card.
 */
/* #define TBOXPMC1IRQ	0xE */
/* #define TBOXPMC2IRQ	0x5 */


/* TODO: Replace the PCI_MASTER_xxx and CPU_MASTER_xxx addresses in the
 * technobox_vxworks_config.h file with the correct addresses for your BSP.
 */

/******************************************************************************
 ******************************************************************************
 *
 * NOTE:
 *
 * Make changes in the following section if you are using the TboxSioHwInit
 * routine to setup the PCI BAR Registers.  Otherwise you should not need
 * to make changes below this line.
 *
 ******************************************************************************
 ******************************************************************************/

/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_4960

#define TBOX_4960A_BASE		0x32000		/* Technobox 4960 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_4960A_INTVECT	TBOXPMC1IRQ	/* Technobox 4960 Card A PCI Interrupt Vector*/

#define TBOX_4960B_BASE		0x32200		/* Technobox 4960 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_4960B_INTVECT	TBOXPMC2IRQ	/* Technobox 4960 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 4960 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card. 
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_5284

#define TBOX_5284A_BASE		0x32400		/* Technobox 5284 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_5284A_INTVECT	TBOXPMC1IRQ	/* Technobox 5284 Card A PCI Interrupt Vector*/

#define TBOX_5284B_BASE		0x32600		/* Technobox 5284 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_5284B_INTVECT	TBOXPMC2IRQ	/* Technobox 5284 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 5284 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_5436

#define TBOX_5436A_BASE		0x32800		/* Technobox 5436 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_5436A_INTVECT	TBOXPMC1IRQ	/* Technobox 5436 Card A PCI Interrupt Vector*/

#define TBOX_5436B_BASE		0x32A00		/* Technobox 5436 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_5436B_INTVECT	TBOXPMC2IRQ	/* Technobox 5436 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 5436 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_5288

#define TBOX_5288A_BASE		0x32C00		/* Technobox 5288 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_5288A_INTVECT	TBOXPMC1IRQ	/* Technobox 5288 Card A PCI Interrupt Vector*/

#define TBOX_5288B_BASE		0x32E00		/* Technobox 5288 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_5288B_INTVECT	TBOXPMC2IRQ	/* Technobox 5288 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 5288 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_4966

#define TBOX_4966A_BASE		0x33000		/* Technobox 4966 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_4966A_INTVECT	TBOXPMC1IRQ	/* Technobox 4966 Card A PCI Interrupt Vector*/

#define TBOX_4966B_BASE		0x33200		/* Technobox 4966 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_4966B_INTVECT	TBOXPMC2IRQ	/* Technobox 4966 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 4966 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_5382

#define TBOX_5382A_BASE		0x33400		/* Technobox 5382 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_5382A_INTVECT	TBOXPMC1IRQ	/* Technobox 5382 Card A PCI Interrupt Vector*/

#define TBOX_5382B_BASE		0x33600		/* Technobox 5382 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_5382B_INTVECT	TBOXPMC2IRQ	/* Technobox 5382 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 5382 */

															   
/* TODO.
 * Fill in the PCI I/O Offset Address of the Adapter being installed in your system.
 *    This is the offset from the start of PCI I/O Memory to use for this card.
 *    The actual BAR registers will be calculated as an offset from this address
 * Fill in the Interrupt Vector of the Adapter being installed in your system.
 *    The interrupt routines will be attached to this interrupt vector.
 *    This is the level that is used in the int enable routine.
 */
#ifdef INCLUDE_TBOX_3607

#define TBOX_3607A_BASE		0x33800		/* Technobox 3607 Card A I/O Space Offset Location (Size 0x200) */
#define TBOX_3607A_INTVECT	TBOXPMC1IRQ	/* Technobox 3607 Card A PCI Interrupt Vector*/

#define TBOX_3607B_BASE		0x33A00		/* Technobox 3607 Card B I/O Space Offset Location (Size 0x200) */
#define TBOX_3607B_INTVECT	TBOXPMC2IRQ	/* Technobox 3607 Card B PCI Interrupt Vector*/

#endif /* INCLUDE TBOX 3607 */


/******************************************************************************
 ******************************************************************************
 *
 *
 * NOTE:
 *
 * The user should not need to make changes below this line
 *
 ******************************************************************************
 ******************************************************************************/

#ifdef INCLUDE_TBOX_4960
/* Definitions for the 4960 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_4960_UART_CHANNELS		4				/* ports per card */

#define TBOX_4960_OFFSET_BAR1	(0x0000)			/* 4960 BAR 1 */
#define TBOX_4960_OFFSET_BAR2	(0x0100)			/* 4960 BAR 2 */
#define TBOX_4960_OFFSET_BAR3	(0x0110)			/* 4960 BAR 3 */
#define TBOX_4960_OFFSET_BAR4	(0x0120)			/* 4960 BAR 4 */
#define TBOX_4960_OFFSET_BAR5	(0x0130)			/* 4960 BAR 5 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_4960A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_4960A_BASE + TBOX_4960_OFFSET_BAR1)
#define TBOX_4960A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_4960A_BASE + TBOX_4960_OFFSET_BAR2)
#define TBOX_4960A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_4960A_BASE + TBOX_4960_OFFSET_BAR3)
#define TBOX_4960A_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_4960A_BASE + TBOX_4960_OFFSET_BAR4)
#define TBOX_4960A_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_4960A_BASE + TBOX_4960_OFFSET_BAR5)

#if (TBOX_4960_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_4960B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_4960B_BASE + TBOX_4960_OFFSET_BAR1)
#define TBOX_4960B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_4960B_BASE + TBOX_4960_OFFSET_BAR2)
#define TBOX_4960B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_4960B_BASE + TBOX_4960_OFFSET_BAR3)
#define TBOX_4960B_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_4960B_BASE + TBOX_4960_OFFSET_BAR4)
#define TBOX_4960B_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_4960B_BASE + TBOX_4960_OFFSET_BAR5)

#endif /* (TBOX_4960_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 4960 */

#ifdef INCLUDE_TBOX_5284
/* Definitions for the 5284 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_5284_UART_CHANNELS	8				/* ports per card */

#define TBOX_5284_OFFSET_BAR1	(0x0000)			/* 5284 BAR 1 */
#define TBOX_5284_OFFSET_BAR2	(0x0100)			/* 5284 BAR 2 */
#define TBOX_5284_OFFSET_BAR3	(0x0140)			/* 5284 BAR 3 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_5284A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5284A_BASE + TBOX_5284_OFFSET_BAR1)
#define TBOX_5284A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5284A_BASE + TBOX_5284_OFFSET_BAR2)
#define TBOX_5284A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5284A_BASE + TBOX_5284_OFFSET_BAR3)

#if (TBOX_5284_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_5284B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5284B_BASE + TBOX_5284_OFFSET_BAR1)
#define TBOX_5284B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5284B_BASE + TBOX_5284_OFFSET_BAR2)
#define TBOX_5284B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5284B_BASE + TBOX_5284_OFFSET_BAR3)

#endif /* (TBOX_5284_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 5284 */



#ifdef INCLUDE_TBOX_5436
/* Definitions for the 5436 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_5436_UART_CHANNELS		16				/* ports per card */

#define TBOX_5436_OFFSET_BAR1	(0x0000)			/* 5436 BAR 1 */
#define TBOX_5436_OFFSET_BAR2	(0x0100)			/* 5436 BAR 2 */
#define TBOX_5436_OFFSET_BAR3	(0x0140)			/* 5436 BAR 3 */
#define TBOX_5436_OFFSET_BAR4	(0x0180)			/* 5436 BAR 4 */
#define TBOX_5436_OFFSET_BAR5	(0x01c0)			/* 5436 BAR 5 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_5436A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5436A_BASE + TBOX_5436_OFFSET_BAR1)
#define TBOX_5436A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5436A_BASE + TBOX_5436_OFFSET_BAR2)
#define TBOX_5436A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5436A_BASE + TBOX_5436_OFFSET_BAR3)
#define TBOX_5436A_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_5436A_BASE + TBOX_5436_OFFSET_BAR4)
#define TBOX_5436A_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_5436A_BASE + TBOX_5436_OFFSET_BAR5)

#if (TBOX_5436_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_5436B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5436B_BASE + TBOX_5436_OFFSET_BAR1)
#define TBOX_5436B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5436B_BASE + TBOX_5436_OFFSET_BAR2)
#define TBOX_5436B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5436B_BASE + TBOX_5436_OFFSET_BAR3)
#define TBOX_5436B_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_5436B_BASE + TBOX_5436_OFFSET_BAR4)
#define TBOX_5436B_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_5436B_BASE + TBOX_5436_OFFSET_BAR5)

#endif /* (TBOX_5436_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 5436 */



#ifdef INCLUDE_TBOX_5288
/* Definitions for the 5288 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_5288_UART_CHANNELS		8				/* ports per cards */

#define TBOX_5288_OFFSET_BAR1	(0x0000)			/* 5288 BAR 1 */
#define TBOX_5288_OFFSET_BAR2	(0x0100)			/* 5288 BAR 2 */
#define TBOX_5288_OFFSET_BAR3	(0x0140)			/* 5288 BAR 3 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_5288A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5288A_BASE + TBOX_5288_OFFSET_BAR1)
#define TBOX_5288A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5288A_BASE + TBOX_5288_OFFSET_BAR2)
#define TBOX_5288A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5288A_BASE + TBOX_5288_OFFSET_BAR3)

#if (TBOX_5288_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_5288B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5288B_BASE + TBOX_5288_OFFSET_BAR1)
#define TBOX_5288B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5288B_BASE + TBOX_5288_OFFSET_BAR2)
#define TBOX_5288B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5288B_BASE + TBOX_5288_OFFSET_BAR3)

#endif /* (TBOX_5288_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 5288 */



#ifdef INCLUDE_TBOX_4966
/* Definitions for the 4966 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_4966_UART_CHANNELS		2				/* ports per cards */

#define TBOX_4966_OFFSET_BAR1	(0x0000)			/* 4966 BAR 1 */
#define TBOX_4966_OFFSET_BAR2	(0x0100)			/* 4966 BAR 2 */
#define TBOX_4966_OFFSET_BAR3	(0x0110)			/* 4966 BAR 3 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_4966A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_4966A_BASE + TBOX_4966_OFFSET_BAR1)
#define TBOX_4966A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_4966A_BASE + TBOX_4966_OFFSET_BAR2)
#define TBOX_4966A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_4966A_BASE + TBOX_4966_OFFSET_BAR3)

#if (TBOX_4966_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_4966B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_4966B_BASE + TBOX_4966_OFFSET_BAR1)
#define TBOX_4966B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_4966B_BASE + TBOX_4966_OFFSET_BAR2)
#define TBOX_4966B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_4966B_BASE + TBOX_4966_OFFSET_BAR3)

#endif /* (TBOX_4966_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 4966 */



#ifdef INCLUDE_TBOX_5382
/* Definitions for the 5382 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_5382_UART_CHANNELS		8				/* ports per card */

#define TBOX_5382_OFFSET_BAR1	(0x0000)			/* 5382 BAR 1 */
#define TBOX_5382_OFFSET_BAR2	(0x0100)			/* 5382 BAR 2 */
#define TBOX_5382_OFFSET_BAR3	(0x0140)			/* 5382 BAR 3 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_5382A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5382A_BASE + TBOX_5382_OFFSET_BAR1)
#define TBOX_5382A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5382A_BASE + TBOX_5382_OFFSET_BAR2)
#define TBOX_5382A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5382A_BASE + TBOX_5382_OFFSET_BAR3)

#if (TBOX_5382_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_5382B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_5382B_BASE + TBOX_5382_OFFSET_BAR1)
#define TBOX_5382B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_5382B_BASE + TBOX_5382_OFFSET_BAR2)
#define TBOX_5382B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_5382B_BASE + TBOX_5382_OFFSET_BAR3)

#endif /* (TBOX_5382_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 5382 */



#ifdef INCLUDE_TBOX_3607
/* Definitions for the 3607 card.  This allocates the appropriate space on the registers.
 * It also generates the define for PCI I/O and CPU addressing
 */
#define	TBOX_3607_UART_CHANNELS		16				/* ports per card */

#define TBOX_3607_OFFSET_BAR1	(0x0000)			/* 3607 BAR 1 */
#define TBOX_3607_OFFSET_BAR2	(0x0100)			/* 3607 BAR 2 */
#define TBOX_3607_OFFSET_BAR3	(0x0140)			/* 3607 BAR 3 */
#define TBOX_3607_OFFSET_BAR4	(0x0180)			/* 3607 BAR 4 */
#define TBOX_3607_OFFSET_BAR5	(0x01c0)			/* 3607 BAR 5 */

/* PCI Address Space BAR Register Defines for Card A */
#define TBOX_3607A_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_3607A_BASE + TBOX_3607_OFFSET_BAR1)			/* 3607 BAR 1 PCI I/O Address */
#define TBOX_3607A_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_3607A_BASE + TBOX_3607_OFFSET_BAR2)			/* 3607 BAR 2 PCI I/O Address */
#define TBOX_3607A_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_3607A_BASE + TBOX_3607_OFFSET_BAR3)			/* 3607 BAR 3 PCI I/O Address */
#define TBOX_3607A_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_3607A_BASE + TBOX_3607_OFFSET_BAR4)			/* 3607 BAR 4 PCI I/O Address */
#define TBOX_3607A_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_3607A_BASE + TBOX_3607_OFFSET_BAR5)			/* 3607 BAR 5 PCI I/O Address */

#if (TBOX_3607_NUM_CARDS > 1)

/* PCI Address Space BAR Register Defines for Card B */
#define TBOX_3607B_PCI_BAR1		(PCI_MASTER_PCI_IO_BASE + TBOX_3607B_BASE + TBOX_3607_OFFSET_BAR1)			/* 3607 BAR 1 PCI I/O Address */
#define TBOX_3607B_PCI_BAR2		(PCI_MASTER_PCI_IO_BASE + TBOX_3607B_BASE + TBOX_3607_OFFSET_BAR2)			/* 3607 BAR 2 PCI I/O Address */
#define TBOX_3607B_PCI_BAR3		(PCI_MASTER_PCI_IO_BASE + TBOX_3607B_BASE + TBOX_3607_OFFSET_BAR3)			/* 3607 BAR 3 PCI I/O Address */
#define TBOX_3607B_PCI_BAR4		(PCI_MASTER_PCI_IO_BASE + TBOX_3607B_BASE + TBOX_3607_OFFSET_BAR4)			/* 3607 BAR 4 PCI I/O Address */
#define TBOX_3607B_PCI_BAR5		(PCI_MASTER_PCI_IO_BASE + TBOX_3607B_BASE + TBOX_3607_OFFSET_BAR5)			/* 3607 BAR 5 PCI I/O Address */

#endif /* (TBOX_3607_NUM_CARDS > 1) */

#endif /* INCLUDE TBOX 3607 */


#endif /* INCLUDE PCI BAR PROGRAMMING */

/* List the Boards Device and Vendor ID's */
#define PCI_ID_TBOX_4960			0x102310b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_5284			0x104510b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_5436			0x104610b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_5288			0x108410b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_4966			0x213310b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_5382			0x217510b5      /* Device and Vendor ID */
#define PCI_ID_TBOX_3607			0x241710b5      /* Device and Vendor ID */

#define	TBOX_ADJ_REGS		1		/* Register Offset for Adjacent Registers */
#define	TBOX_SIO_BAUD_XTAL	1843200	/* Crystal frequency */

/* Prototypes */
void TboxSioHwInit (void);
void TboxSioSerialInit (UINT8 LevelOverride);
void TboxSioSerialInit2 (int VectorOverride);
void TboxSioSerialHwReset (void);

#ifdef __cplusplus
}
#endif

#endif /* __TboxSioSupport_h__ */
