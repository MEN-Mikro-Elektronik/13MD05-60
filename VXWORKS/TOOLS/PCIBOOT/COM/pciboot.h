/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  pciboot.h
 *
 *      \author  rt
 *        $Date: 2011/05/04 11:42:34 $
 *    $Revision: 1.5 $
 *
 *       \brief  Header file for PCIBOOT tool containing
 *               specific function prototypes
 *               specific type definitions
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *				 ONE_NAMESPACE_PER_TOOL
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: pciboot.h,v $
 * Revision 1.5  2011/05/04 11:42:34  rt
 * R: New HW revision model -00 with new PCI/PCIe bridge (new PCI device ID).
 * M: PCIB_BRIDGE_DEVICE_ID_8112 added.
 *
 * Revision 1.4  2011/01/19 14:29:21  rt
 * R: New HW model -03 with new PCI/PCIe bridge (new PCI vendor ID).
 * M: PCIB_BRIDGE_VENDOR_ID_2 added.
 *
 * Revision 1.3  2011/01/19 14:15:02  rt
 * R: New HW model -03 with new PCI/PCIe bridge (new PCI vendor ID).
 * M: PCIB_BRIDGE_VENDOR_ID_2 added.
 *
 * Revision 1.2  2007/10/01 18:00:07  rtrübenbach
 * added:
 * -support for VxWorks
 * -option -e
 * -ep4 now hot pluggable
 * -binaries are verified before starting MPC
 *
 * Revision 1.1  2007/02/28 13:22:41  rtrübenbach
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _PCIBOOT_H
#define _PCIBOOT_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/
/**@{*/
/** OS Specific includes */
#if OS9
#	undef NULL
#	include <setjmp.h>
#	include <setsys.h>
#	include <stdio.h>
#	include <ctype.h>
#endif /* OS9 */

#if LINUX
#	undef NULL
#	include <stdio.h>
#	include <stdlib.h>
#	include <stdarg.h>
#	include <ctype.h>
#endif /* LINUX */

#if VXWORKS
#	include <stdio.h>
#   include <stdlib.h>
#   include <stdarg.h>
#   include <ctype.h>
#endif /* VXWORKS */

#if WINNT
#   define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#   define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
#	include <windows.h>
#	include <stdio.h>
#	include <stdlib.h>
#	include <winioctl.h>

#	include "MEN/pci_genacc.h"
#endif /* WINNT */

/**@}*/

#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <maccess_mem.h>
#include <MEN/oss.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/usr_err.h>


/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/** \name  Defines */

#ifdef DBG
#define DBGOUT(_x_) printf _x_
#else
#define DBGOUT(_x_)
#endif /* DBG */

#define PCIB_MAX_DEVICES 	32		 /* maximum number of matching PCI devices */
#define PCIB_MAX_FILE_SIZE 	0x200000 /* currently set to 2MB */
#define PCIB_WAIT_DELAY		5000	 /* wait for target reset in ms */
#define PCIB_RETRY_RESTORE_HEADER	50 /* no. of retries to restore pci header */
#define PCIB_RETRY_COPY_FILE		5  /* no. of retries to copy bootloader binary */

#define PCIB_OS_ACCESS_READ  1
#define PCIB_OS_ACCESS_WRITE 0

#define PCIB_VENDOR_ID			0x1057		/**< vendor id */
#define PCIB_DEVICE_ID_8540		0x000D		/**< device id MPC8540 */
#define PCIB_DEVICE_ID_8541		0x0008		/**< device id MPC8540 */
#define PCIB_SUBVENDOR_ID		0x0000		/**< sub vendor id */
#define PCIB_CCSR_BAR			0			/**< bar for MPCs registers  */
#define PCIB_CCSR_SIZE			0x100000	/**< size of MPCs register area  */
#define PCIB_DDR_AREA_BAR		1			/**< bar for download area in DDR */
#define PCIB_DDR_AREA_SIZE		0x2000000	/**< size of whole download area */
#define PCIB_OS_OFFSET			0x100000	/**< offset of os download area */

#define PCIB_BRIDGE_VENDOR_ID			0x10B5		/**< vendor id */
#define PCIB_BRIDGE_VENDOR_ID_2			0x10E3		/**< 2nd vendor id */
#define PCIB_BRIDGE_DEVICE_ID_8111		0x8111		/**< device id 1x bridge */
#define PCIB_BRIDGE_DEVICE_ID_8112		0x8112		/**< device id new 1x bridge */
#define PCIB_BRIDGE_DEVICE_ID_8114		0x8114		/**< device id 4x bridge */

/* some macros */
#define MS2TICKS(ms)        (UOS_MsecTimerResolution() * ms)
#define MSECDIFF(basemsec)  (UOS_MsecTimerGet() - basemsec)

/*-----------------------------------------------------------------------
 * Macros to convert from big endian format 16/32 bit values
 *----------------------------------------------------------------------*/
#if defined(_LITTLE_ENDIAN_)
# define PCIB_SWAP_BE32(dword)	OSS_SWAP32(dword)
# define PCIB_SWAP_BE16(word)	OSS_SWAP16(word)
#elif defined(_BIG_ENDIAN_)
# define PCIB_SWAP_BE32(dword) (dword)
# define PCIB_SWAP_BE16(word) (word)
#else
# error "Define _BIG_ENDIAN_ or _LITTLE_ENDIAN_"
#endif /* _BIG/_LITTLE_ENDIAN_ */

/* error numbers */
#define ERR_TRIED_TO_DELETE_BOOTSECTOR		(ERR_UOS+0x14)
#define ERR_PCIB_ILL_CONFIG_FILE			(ERR_UOS+0x15)
#define ERR_PCIB_CORRUPT_CONFIG_FILE		(ERR_UOS+0x16)


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/**@{*/

#if 0 /*why it does not work???*/
/** Structure for holding pci device info */
typedef struct _PCI_DEVS {
	u_int32 venId;			/**< vendor Id */
	u_int32 devId;			/**< device Id */
	u_int32 subSysVenId;	/**< sub system vendor Id */
	u_int32 bar[6];			/**< BAR0 - BAR5 */
	u_int16 comReg;			/**< command register */
	u_int8  bus;			/**< bus number */
	u_int8  dev;			/**< number of device on bus */
	u_int8  fun;			/**< number of function in device */
} PCI_DEVS;
#else
typedef struct _PCI_DEVS {
	u_int8  bus;			/**< bus number */
	u_int8  dev;			/**< number of device on bus */
	u_int8  fun;			/**< number of function in device */
	u_int32 venId;			/**< vendor Id */
	u_int32 devId;			/**< device Id */
	u_int32 subSysVenId;	/**< sub system vendor Id */
	u_int32 bar[6];			/**< BAR0 - BAR5 */
	u_int16 comReg;			/**< flag: command reg of device was changed */
} PCI_DEVS;
#endif

/** Structure for holding all device info */
typedef struct _DEV_HDL {
	void 				*physAddrCcsr;	/**< CCSR physical address */
	void 				*physAddrDdr;	/**< DDR physical address */
	u_int8				*mappedAddrCcsr;/**< CCSR address mapped to user space */
	u_int8				*mappedAddrDdr;	/**< DDR address mapped to user space */
	char				*bootFileName;	/**< bootloader file name */
	char				*osFileName;	/**< operating system file name */
	PCI_DEVS			pciDev[6];		/**< struct with PCI device infos */
	PCI_DEVS			pciBridge;		/**< struct with PCI bridge infos */
	u_int8				dbgLevel;		/**< verbosity level */
} DEV_HDL;

/**@}*/
/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef __cplusplus
      }
#endif

#endif /* _PCIBOOT_H */


