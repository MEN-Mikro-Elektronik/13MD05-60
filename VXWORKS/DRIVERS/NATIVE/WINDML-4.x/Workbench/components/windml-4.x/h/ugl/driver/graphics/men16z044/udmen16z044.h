/* udmen16z044.h - MEN 16Z044_DISP FPGA  graphics device header file */
/* Copyright 1999-2002 Wind River Systems, Inc. All Rights Reserved */
/* Copyright 2004      MEN GmbH All Rights Reserved */

/*
modification history
--------------------
$Log: udmen16z044.h,v $
Revision 1.3  2006/05/19 16:30:07  cs
added IOCTL code for WINDML_MEN16Z044_HWMODE_SET_SW

Revision 1.2  2006/01/26 09:38:54  cs
added support for RTP
   + typedef MEN16Z044_PAGE
   + IO_EXT IOCTL code defines

Revision 1.1  2005/12/19 19:28:21  cs
Initial Revision

Revision 1.3  2005/08/30 10:18:17  CSchuster
changed to use new chameleon names for defines, functions and macros

replaced udmenerivan.h

Revision 1.2  2005/06/01 14:22:52  CSchuster
added defines MEN16Z044_DISP_UGL_ID*
cosmetics

added defines MEN16Z044_DISP_UGL_ID*
cosmetics

Revision 1.1  2004/12/21 11:51:55  CSchuster
Initial Revision


cloned from udmensaruman.h
01n,01aug02,wdf  Fixed params in create functions.
*/

#ifndef INCLUDE_udmen16z044_h
#define INCLUDE_udmen16z044_h

#if __cplusplus
extern "C" {
#endif

/*
 * DESCRIPTION - This file defines the hardware specific functions and
 * addresses used by the FPGA drivers.
 */

/* Get the Device Driver Configuration Definitions */
#ifndef INCLUDE_MEN16Z044_GRAPHICS
#define INCLUDE_MEN16Z044_GRAPHICS
#define BUILD_DRIVER
#include <uglInit.h>
#endif /* INCLUDE_MEN16Z044_GRAPHICS */

/* General 16 bit frame buffer definitions */
#ifdef INCLUDE_UGL_RGB565
/* Define 16 bit driver */
#define UGL_GRAPHICS_NAME       "MEN 16Z044_DISP"

#define UGL_GRAPHICS_CREATE     uglmen16z044DevCreate
#define UGL_COLOR_DEPTH         16
#define UGL_MODE_FLAGS          UGL_MODE_DIRECT_COLOR
#else
	#error "udmen16z044.h configuration not RGB565"
#endif /* INCLUDE_UGL_RGB565 */

#define MEN16Z044_DISP_UGL_ID1	0x23456789 /* ID1 shared with sysWindML.c */
#define MEN16Z044_DISP_UGL_ID2	0x98765432 /* ID2 shared with sysWindML.c */

/* includes */
#include <ugl/uglugi.h>
#include <ugl/driver/graphics/generic/udgen.h>

/* typedefs */
typedef struct _MEN16Z044_PAGE {
	UGL_UGI_DRIVER * pUgiDriver;
	UGL_PAGE       * pPage;
} MEN16Z044_PAGE;

/* IOCTL codes to access kernel portions of driver */
#define WINDML_MEN16Z044_INIT				WINDML_EXT_DRIVER_IOCTL + 1
#define WINDML_MEN16Z044_DEINIT				WINDML_EXT_DRIVER_IOCTL + 2
#define WINDML_MEN16Z044_PAGE_VISABLE_SET	WINDML_EXT_DRIVER_IOCTL + 3
#define WINDML_MEN16Z044_HWMODE_SET			WINDML_EXT_DRIVER_IOCTL + 4
#define WINDML_MEN16Z044_HWMODE_SET_SW		WINDML_EXT_DRIVER_IOCTL + 5

/* Establish chip access mechanism */
UGL_UGI_DRIVER * uglmen16z044DevCreate(UGL_UINT32 notUsed0, UGL_UINT32 notUsed1,
				 UGL_UINT32 notUsed2);


#if __cplusplus
} /* extern "C" */
#endif

#endif
