/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  pciboot_doc.c
 *
 *      \author  rt
 *        $Date: 2007/10/01 18:00:28 $
 *    $Revision: 1.2 $
 *
 *      \brief   User documentation for PCIBOOT tool
 *
 *     Required: -
 *
 *     \switches -
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: pciboot_doc.c,v $
 * Revision 1.2  2007/10/01 18:00:28  rtrübenbach
 * cosmetics
 *
 * Revision 1.1  2007/02/28 13:22:40  rtrübenbach
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

/*! \mainpage
    This is the documentation for the tool to boot a EP04 PCIe endpoint device
    via PCI.

    \n
    \section Variants Variants
    no

    \verbatim
    Tool                Variant Description
    --------            --------------------------------
    Standard            non-swapped hw access
    _sw                 swapped hw access
    \endverbatim

    \n \section FuncDesc Functional Description

    \n \subsection General General
	The tool downloads bootloader and OS binaries to the EP04 PCIe endpoint
	device and starts it.

    \n \subsection Build Build
	The tool is made to be built with the MEN MDIS build environment.
	Up to now only native NT build with VC++ 2005 is tested\n

	The tool needs the following libraries to be able to access
	the hardware: OSS library (for user space)

	\linux For Linux the pciutils library are needed by the OSS_USR library.

	In order to use the tool without installing the MDIS environment
	on your target, you can built the tool using static linking of the user
	mode libraries (MDIS Makefile: LIB_MODE = static).

*/

/*! \page dummy
  \menimages
 */
