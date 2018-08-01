/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  z25_mdis.h
 *
 *      \author  sv
 *        $Date: 2010/12/08 23:19:28 $
 *    $Revision: 2.3 $
 *
 *        \brief  13Z25 mdis interface public definitions
 *
 *            	  This header file consists of the public definitions of the
 *                mdis interface.
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: z25_mdis.h,v $
 * Revision 2.3  2010/12/08 23:19:28  cs
 * R: redundant definition of default values for descriptor keys in z25_driver.h
 * M: use defines from z25_driver.h, delete obsolete ones here
 *
 * Revision 2.2  2010/11/30 18:02:12  cs
 * R:1. driver used wrong default values for descriptor keys (not identical to .xml file)
 * M:1. define descriptor key defaults identically to ones in .xml file
 *
 * Revision 2.1  2005/06/23 08:59:56  SVogel
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	_Z25_MDIS_H
#define	_Z25_MDIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* default MDIS descriptor settings */
#define Z25_MDIS_DEFAULT_BAUD 			115200
#define Z25_MDIS_DEFAULT_PHYS_INT	 	     0	/* physical interface (RS232) */

#include <MEN/dbg.h>

extern STATUS Z25_MdisDriver(DESC_SPEC *boardDescP, DESC_SPEC *deviceDescP);

#ifdef __cplusplus
}
#endif

#endif	/* _Z25_MDIS_H */

