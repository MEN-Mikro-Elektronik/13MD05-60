/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  fpga_load_os.h
 *
 *      \author  Christian.Schuster@men.de
 *        $Date: 2004/12/23 15:20:16 $
 *    $Revision: 1.1 $
 *
 *       \brief  Header file for Z100_os library
 *               containing OS specific functions for FPGA_LOAD tool
 *
 *    \switches  OS
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: fpga_load_os.h,v $
 * Revision 1.1  2004/12/23 15:20:16  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _FPGA_LOAD_OS_H
#define _FPGA_LOAD_OS_H

#ifdef __cplusplus
      extern "C" {
#endif

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
/**@}*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
/**@{*/
/** Prototypes of global functions */
extern void Z100_Os_init(DEV_HDL **h);
extern void Z100_Os_exit(DEV_HDL **h);
extern u_int32 Z100_Os_access_address( 	u_int32 bar,
										u_int32 offs,
										int size,
										int read,
										u_int32 value,
										int *be_flag );
extern int Z100_Os_findPciDevice( PCI_DEVS *dev,
								  PCI_DEVS *allPciDevs[],
								  u_int32 *numDevs,
								  int show_all );

/**@}*/

#ifdef __cplusplus
      }
#endif

#endif /* _FPGA_LOAD_OS_H */

