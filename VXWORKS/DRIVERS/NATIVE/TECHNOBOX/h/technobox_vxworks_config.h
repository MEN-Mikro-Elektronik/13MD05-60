/* COMMON - Technobox VxWorks Common Code */
/* technobox_vxworks_config.h - Generic BSP MACRO definitions */

/******************************************************************************
 *
 * Filename: technobox_vxworks_config.h
 *
 * Copyright 2004 by Technobox, Inc.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted by other parties provided that the following 
 * conditions are met:
 * 1. This program may only be used on Technobox Inc. products.
 * 2. Any modified files must carry prominent notices stating that you 
 *    changed the file and the date of any changes.
 * 3. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer, without modification. 
 * 4. EXCEPT WHEN OTHERWISE STATED IN WRITING, TECHNOBOX INC. AND/OR OTHER PARTIES 
 *    PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
 *    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
 *    AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD
 *    THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY
 *    SERVICING, REPAIR OR CORRECTION.
 * 5. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
 *    WILL TECHNOBOX INC., OR ANY OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE
 *    THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING
 *    ANY GENERAL, DIRECT, INDIRECT, SPECIAL, INCIDENTAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE
 *    PROGRAM (INCLUDING, BUT NOT LIMITED TO, LOSS OF USE, DATA, OR PROFITS;
 *    DATA BEING RENDERED INACCURATE; PROCUREMENT OF SUBSTITUTE GOODS OR
 *    SERVICES; LOSSES SUSTAINED BY YOU OR THIRD PARTIES; BUSINESS INTERRUPTION;
 *    OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) HOWEVER
 *    CAUSED (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF TECHNOBOX INC. OR
 *    OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. 
 *
 * Functions: This file contains the function that specifies the PCI Address range.
 *
 * Revision:
 * REV   WHO   DATE       SWF#   COMMENTS
 * ----+-----+----------+------+-----------------------------------------------
 * 002   SGL   04/12/06   030    Moved PCI and CPU BSP addresses to config file
 * 001   SGL   03/15/04          Formating, grouping updates.
 * 000   SGL   03/10/04          Initial Release
 *
 *****************************************************************************/
#ifndef __technobox_vxworks_config_h__
#define __technobox_vxworks_config_h__

/*
DESCRIPTION
This file contains the macros for accessing the PCI Configuration space for
various BSPs.  The user needs to update the appropriate definitions for their
architecture and define which Adapter is being included.
*/

/* TODO: Replace the following macros with the appropriate BSP command.  Some BSPs
 * may share the same names
 * Set one of the following definitions to TRUE based on which Base Card and
 * BSP you are using.  This can be defined here or in inside the BSP configuration
 * file (we do in config file or Project to make supporting multiple BSPs easier)
 */
/* #define USING_MOTOROLA_BSP	TRUE */
/* #define USING_SBS_BSP		TRUE */
/* #define USING_FORCE_BSP		TRUE */
#define USING_OTHER_BSP		TRUE

#if ( !defined(USING_MOTOROLA_BSP) && !defined(USING_SBS_BSP) && !defined(USING_FORCE_BSP) && !defined(USING_OTHER_BSP))
/* Set one of the USING_xxx_BSP definitions above to true to
 * include the appropriate commands for the Config Macros
 */
#error No BSP Defined above so Macros are not specified (see comment)
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Replace the following addresses with their BSP values.
 * These are values which match cards used for testing purposes only and do not 
 * necessarily reflect your actual Base Card.
 * PCI_MASTER_xxx is the address from the PCI Bus Standpoint
 * CPU_MASTER_xxx is the address from the CPU Local Bus Standpoint
 */

#if defined( USING_MOTOROLA_BSP ) || defined( USING_SBS_BSP)
/* Address of PCI I/O Space */
#define PCI_MASTER_PCI_IO_BASE			0x00000000		/* From the PCI Bus standpoint */
#define CPU_MASTER_PCI_IO_BASE			0xFE000000		/* From the CPU standpoint */

/* Address of PCI Memory Space (non-prefetch) */
#define PCI_MASTER_PCI_MEM_BASE			0xFD000000		/* From the PCI Bus standpoint */
#define CPU_MASTER_PCI_MEM_BASE			0xFD000000		/* From the CPU standpoint */

/* Address of prefetchable PCI Memory Space (prefetch) */
#define PCI_MASTER_PREFETCH_PCI_MEM_BASE	0xFD000000	/* From the PCI Bus standpoint */
#define CPU_MASTER_PREFETCH_PCI_MEM_BASE	0xFD000000	/* From the CPU standpoint */    

/* Address of Processor Memory */
#define PCI_MASTER_MAIN_MEMORY_BASE		0x00000000	/* From the PCI Bus standpoint */
#define CPU_MASTER_MAIN_MEMORY_BASE		0x00000000	/* From the CPU standpoint */    

#endif


#if defined (USING_FORCE_BSP)
/* Address of PCI I/O Space */
#define PCI_MASTER_PCI_IO_BASE			0x00800000		/* From the PCI Bus standpoint */
#define CPU_MASTER_PCI_IO_BASE			0xFE800000		/* From the CPU standpoint */    

/* Address of PCI Memory Space (non-prefetch) */
#define PCI_MASTER_PCI_MEM_BASE			0xD0000000		/* From the PCI Bus standpoint */
#define CPU_MASTER_PCI_MEM_BASE			0xD0000000		/* From the CPU standpoint */    

/* Address of prefetchable PCI Memory Space (prefetch) */
#define PCI_MASTER_PREFETCH_PCI_MEM_BASE	0xD0000000	/* From the PCI Bus standpoint */
#define CPU_MASTER_PREFETCH_PCI_MEM_BASE	0xD0000000	/* From the CPU standpoint */    

/* Address of Processor Memory */
#define PCI_MASTER_MAIN_MEMORY_BASE		0x00000000	/* From the PCI Bus standpoint */
#define CPU_MASTER_MAIN_MEMORY_BASE		0x00000000	/* From the CPU standpoint */    

#endif


#if defined( USING_OTHER_BSP)

/* Address of PCI I/O Space */
#define PCI_MASTER_PCI_IO_BASE			0x00000000		/* From the PCI Bus standpoint */
#define CPU_MASTER_PCI_IO_BASE			0xFBFF8000		/* From the CPU standpoint */    

/* Address of PCI Memory Space (non-prefetch) */
/*#define PCI_MASTER_PCI_MEM_ADDR			0x12345678		From the PCI Bus standpoint */
/*#define CPU_MASTER_PCI_MEM_ADDR			0x12345678		From the CPU standpoint */    

/* Address of prefetchable PCI Memory Space (prefetch) */
/*#define PCI_MASTER_PREFETCH_PCI_MEM_ADDR	0x12345678		From the PCI Bus standpoint */
/*#define CPU_MASTER_PREFETCH_PCI_MEM_ADDR	0x12345678		From the CPU standpoint */    

/* Address of Processor Memory */
/*#define PCI_MASTER_MAIN_MEMORY_BASE		0x12345678	 From the PCI Bus standpoint */
/*#define CPU_MASTER_MAIN_MEMORY_BASE		0x12345678	 From the CPU standpoint */    

#endif



/* These definitions are valid for the Motorola MVME2603 and SBS K2 BSP
 * and should probably be OK for all of the Motorola and SBS BSP's
 * Set the USING_MOTOROLA_BSP or USING_SBS_BSP above definitions to TRUE
 */
#if ( defined(USING_MOTOROLA_BSP) || defined(USING_SBS_BSP))
#include "drv/pci/pciConfigLib.h"
#define PciConfigInByte_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInByte( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigInWord_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInWord( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigInLong_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInLong( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigOutByte_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutByte( (Bus), (Dev), (Func), (Reg), (Data))
#define PciConfigOutWord_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutWord( (Bus), (Dev), (Func), (Reg), (Data))
#define PciConfigOutLong_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutLong( (Bus), (Dev), (Func), (Reg), (Data))
#define PciConfigModifyLong_M(Bus, Dev, Func, Reg, Mask, Data) \
			pciConfigModifyLong( (Bus), (Dev), (Func), (Reg), (Mask), (Data))
#define PciFindDevice_M(Vendor, Device, Inst, Bus, Slot, Func) \
			pciFindDevice((int)(Vendor),(int)(Device),(int)(Inst),\
							(int *)(Bus),(int *)(Slot),(int *)(Func))
#endif

/* These definitions are valid for the Force PowerCore 6750
 * and should probably be OK for all of the Force BSP's
 * Set the USING_FORCE_BSP above definition to TRUE
 */
#if ( defined(USING_FORCE_BSP) )
#include "../config/force/h/pci/frcPciLibInclude.h"

#define PciConfigInByte_M(Bus, Dev, Func, Reg, pData) \
			frcPciConfigReadByte( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (pData))
#define PciConfigInWord_M(Bus, Dev, Func, Reg, pData) \
			frcPciConfigReadWord( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (pData))
#define PciConfigInLong_M(Bus, Dev, Func, Reg, pData) \
			frcPciConfigReadDword( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (pData))
#define PciConfigOutByte_M(Bus, Dev, Func, Reg, Data) \
			frcPciConfigWriteByte( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (Data))
#define PciConfigOutWord_M(Bus, Dev, Func, Reg, Data) \
			frcPciConfigWriteWord( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (Data))
#define PciConfigOutLong_M(Bus, Dev, Func, Reg, Data) \
			frcPciConfigWriteDword( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (Data))
#define PciConfigModifyLong_M(Bus, Dev, Func, Reg, Mask, Data)	\
			{													\
			UINT32	RdData;										\
			frcPciConfigReadDword( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), (&RdData));	\
			frcPciConfigWriteDword( (UINT8)(Bus), (UINT8)(Dev), (UINT8)(Func), (Reg), ((RdData & ~(Mask)) | ( (Data) & (Mask))));	\
			}
#define PciFindDevice_M(Vendor, Device, Inst, Bus, Slot, Func) \
			frcPciFindDevice( (UINT32) (((Device) << 16) | (Vendor)), (UINT16) (Inst),\
							 (UINT8 *) ( (int)(Bus)+3), (UINT8 *) ( (int)(Slot) + 3), (UINT8 *) ( (int)(Func) + 3))

#endif

/* These definitions are provided as a template for the user's BSP.
 * The include file and routine names must be specified.
 * Set the USING_OTHER_BSP above definition to TRUE
 */
#if ( defined(USING_OTHER_BSP) )
/*#include "<include file with PCI Config routine prototype>.h"*/

#define PciConfigInByte_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInByte( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigInWord_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInWord( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigInLong_M(Bus, Dev, Func, Reg, pData) \
			pciConfigInLong( (Bus), (Dev), (Func), (Reg), (pData))
#define PciConfigOutByte_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutByte( (Bus), (Dev), (Func), (Reg), (Data))
#define PciConfigOutWord_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutWord( (Bus), (Dev), (Func), (Reg), (Data))
#define PciConfigOutLong_M(Bus, Dev, Func, Reg, Data) \
			pciConfigOutLong( (Bus), (Dev), (Func), (Reg), (Data))
/*#define PciConfigModifyLong_M(Bus, Dev, Func, Reg, Mask, Data) \
			<YourConfigModifyLongName>( (Bus), (Dev), (Func), (Reg), (Mask), (Data))*/
#define PciFindDevice_M(Vendor, Device, Inst, Bus, Slot, Func) \
			pciFindDevice( (int)(Vendor), (int)(Device), (int)(Inst),\
							 (int *)(Bus), (int *)(Slot), (int *)(Func) )
#endif

/* Macros to convert from PCI to Local Address */
#define PCI_IO_TO_CPU_ADDR(x)		\
	((UINT32)(x) + (CPU_MASTER_PCI_IO_BASE - PCI_MASTER_PCI_IO_BASE))
#define PCI_MEM_TO_CPU_ADDR(x)		\
	((UINT32)(x) + (CPU_MASTER_PCI_MEM_BASE - PCI_MASTER_PCI_MEM_BASE))
#define PCI_PREFETCH_MEM_TO_CPU_ADDR(x)	\
	((UINT32)(x) + (CPU_MASTER_PREFETCH_PCI_MEM_BASE - PCI_MASTER_PREFETCH_PCI_MEM_BASE))

/* TODO
 * If these are not defined in the BSP package make sure they are OK
 */
#ifndef PCI_CACHE_LINE_SIZE
#define PCI_CACHE_LINE_SIZE				(32/4)
#endif

#ifndef PCI_LATENCY_TIMER
#define PCI_LATENCY_TIMER				0xff
#endif

#ifdef __cplusplus
}
#endif

#endif /* __technobox_vxworks_config_h__ */
