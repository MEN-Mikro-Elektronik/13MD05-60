/* udmen16z044.c - MEN 16Z044_DISP FPGA driver  */

/* Copyright 1999-2002 Wind River Systems, Inc. All Rights Reserved */
/* Copyright 2004      MEN GmbH All Rights Reserved */

/*
modification history
--------------------
$Log: udmen16z044.c,v $
Revision 1.9  2015/08/17 09:09:49  ts
R: device /graphics(23456789,98765432)/0 not instantiated under 6.8.3, 6.9
M: reverted menSysWindMLDevGet/Ctrl back to sysWindML... because MEN specific
   functions were not called by Wind ML

Revision 1.8  2011/09/07 19:31:27  ts
R: added support for 16Z044-01 (more resolutions, backward compatible)
M: added support for new resolutions 320x240 and 1280x800

Revision 1.7  2008/10/09 15:17:06  cs
R: called from RTP men16z044Deinitialize() would call wrong IOCTL
M: use correct IOCTL code (WINDML_MEN16Z044_DEINIT) for deinitialization

Revision 1.6  2006/10/15 16:43:31  cs
added define for clearing screen on startup (Z044_START_WITH_CLEAR_SCREEN)

added:
  - define Z044_START_WITH_CLEAR_SCREEN: disable clearing of screen on startup

Revision 1.5  2006/09/11 10:45:02  cs
fixed double buffering:
   - no video memory was assigned, uglPageCreate() failed

Revision 1.4  2006/05/19 16:34:33  cs
completion and bugfixes of changes from last checkin

Revision 1.3  2006/05/16 20:41:38  cs
added
    - support for Z044_DISP in common PCI device (P18)
    - support for enabling framebuffer data swap in HW
    - dynamically swapping register access

Revision 1.2  2006/01/26 09:43:33  cs
added
   + support for use from RTP
   + support for HW supporting - more than one resolution
                               - only one refresh rate

Revision 1.1  2005/12/19 19:28:22  cs
Initial Revision, split from WINDML3.x path

Revision 1.3  2005/08/30 10:18:58  CSchuster
changed to use new chameleon names for defines, functions and macros

Revision 1.2  2005/06/01 14:22:54  CSchuster
changed MENERIVAN_UGL_ID* to use MEN16Z044_DISP_UGL_ID*
added some debug prints on failure
cosmetics

renamed MENERIVAN_UGL_ID* to MEN16Z044_DISP_UGL_ID*
cosmetics

Revision 1.1  2004/12/21 11:51:56  CSchuster
Initial Revision

cloned from udct16.c
02c,18sep02,gav  SPR #81651 fix.
*/

static char *Z044_RCSid="$Id: udmen16z044.c,v 1.9 2015/08/17 09:09:49 ts Exp $";

/* DESCRIPTION

   MEN 16Z044_DISP 640x480 - 1280x1024 16bit FPGA driver

*/

/* includes */

#include <stdlib.h>
#include <string.h>

#include <ugl/ugl.h>
#include <ugl/ugltypes.h>
#include <ugl/driver/hal/udHal.h>
#include <ugl/driver/graphics/iodrv/udGraphicsDrv.h>
#include <ugl/driver/graphics/iodrv/udPci.h>
#include <ugl/driver/graphics/iodrv/udDevMemUtil.h>
#include <rtpLib.h>
#include <logLib.h>
#include "semLib.h"
#include "ffsLib.h"
#include "private/vmLibP.h"


#include <ugl/uglmem.h>
#include <ugl/ugllog.h>
#include <ugl/driver/graphics/common/udcHwAbsLayer.h>
#include <ugl/driver/graphics/iodrv/udGraphicsDrv.h>
#include <ugl/driver/graphics/generic/udgen.h>
#include <ugl/driver/graphics/generic/udgen16.h>
#include <ugl/ext/common/uglext.h>
#include <ugl/driver/ext/jpeg/udgenjpg.h>
#include <ugl/driver/graphics/men16z044/udmen16z044.h>

/* debug define, enable in case of problems */
/* #define Z044_DEBUG */

/* when defined, Screen is cleared on startup */
#define Z044_START_WITH_CLEAR_SCREEN

/* when defined, the Z044 is not switched off upon deinitialization (avoid floating TFT panel) */
#define Z044_ALWAYS_ENABLED

/* swap macros */
#if _BYTE_ORDER == _BIG_ENDIAN
#	define Z044_WSWAP16(word)  ( (((word)>>8) & 0xff) | (((word)<<8)&0xff00) )

#	define Z044_WSWAP32(dword) ( ((dword)>>24) | ((dword)<<24) | \
								  (((dword)>>8) & 0x0000ff00)   | \
								  (((dword)<<8) & 0x00ff0000)     )

#	define Z044_RSWAP16(word)  Z044_Swap16(word)
#	define Z044_RSWAP32(dword) Z044_Swap32(dword)
#else
#	define Z044_WSWAP16(word)	( word )
#	define Z044_WSWAP32(dword)	( dword )
#	define Z044_RSWAP16(word)	( word )
#	define Z044_RSWAP32(dword)	( dword )
#endif

/* defines for parameters in config files */
#define Z044_PARAM1_SWAP_GRAPHIC_DATA		0x00000001
						/* all framebuffer data is swapped (bytes in word) */


/* hardware related defines */
#define Z044_CTRL			0x00		/**< Control Register */
#define Z044_FOS			0x04		/**< Frame Offset Register */
#define Z044_LVDS_CTRL		0x08		/**< LVDS Control Register */
										/*   Controls the FPD link interface */
#define Z044_FP_CTRL		0x0C		/**< Flat Panel Control Register */
										/*   Depending on target design,
											 Outputs directly drive enable
											 signals for Flat Panel displays */

#define Z044_REG_IN(offs)														\
	Z044_RSWAP32(																\
		*(volatile UGL_UINT32*)(pGenDriver->pWmlDevice->pPhysBaseAdrs0+offs)\
		)

#define Z044_REG_OUT(offs, val)													\
	(*(volatile UGL_UINT32*)(pGenDriver->pWmlDevice->pPhysBaseAdrs0 +		\
							 offs)) = Z044_WSWAP32(val)


#define Z044_CTL_IN  Z044_REG_IN(Z044_CTRL)
#define Z044_CTL_OUT(val) Z044_REG_OUT(Z044_CTRL, (val))

/* #defines for Z044_CTRL */
#define Z044_CTRL_CHANGE 	0x80000000
							/**< bit enabling change of this register */
							/**< 0: disable that changes of register settings
							 *      affect mode of display controller \n
							 *   1: changes to register also change mode of
							 *      display controller
							 */
#define Z044_CTRL_ONOFF  	0x40000000
							/**< bit setting power on/off status of display */
							/**< 0: display (and controller) enabled \n
							 *   1: display (and controller) disabled
							 */
#define Z044_CTRL_DEBUG  	0x20000000
							/**< bit setting debug mode of display controller */
							/**< 0: disable debug mode \n
							 *   1: enable debug mode
							 *      a colored rectangular the size of the actual
							 *      resolution is drawn
							 */
#define Z044_CTRL_BSWAP  	0x00000008
							/**< bit setting swap mode of display controller */
							/**< 0: disable frame buffer data swap \n
							 *   1: enable swapping
							 *      all frame buffer data is swapped
							 *      (the two bytes of each word)
							 */
#define Z044_CTRL_REFRESH_75 0x00000004
							/**< bit setting refresh frequency of display */
							/**< 0: 60 Hz \n
							 *   1: 75 Hz
							 */
#define Z044_CTRL_RES_MASK		0x00000033
							/**< mask for all bits setting the resolution */
#define Z044_CTRL_RES_640X480	0x00000000
							/**< setting VGA resolution (  640 x  480 ) */
#define Z044_CTRL_RES_800X600	0x00000001
							/**< setting VGA resolution (  800 x  600 ) */
#define Z044_CTRL_RES_1024X768	0x00000002
							/**< setting VGA resolution ( 1024 x  768 ) */
#define Z044_CTRL_RES_1280X1024	0x00000003
							/**< setting VGA resolution ( 1280 x 1024 ) */
#define Z044_CTRL_RES_320X240	0x00000010
							/**< setting VGA resolution (  320 x  240 ) */
#define Z044_CTRL_RES_1280X800	0x00000011
							/**< setting VGA resolution ( 1280 x  800 ) */


/* graphics defines */

#define Z044_WIDTH_320  	 320
#define Z044_WIDTH_640  	 640
#define Z044_WIDTH_800  	 800
#define Z044_WIDTH_1024 	1024
#define Z044_WIDTH_1280  	1280
#define Z044_HEIGHT_240   	 240
#define Z044_HEIGHT_480   	 480
#define Z044_HEIGHT_600   	 600
#define Z044_HEIGHT_768   	 768
#define Z044_HEIGHT_800   	 800
#define Z044_HEIGHT_1024  	1024
#define Z044_COLOR_DEPTH_16 	16
#define Z044_REFRESH_RATE_60	60
#define Z044_REFRESH_RATE_75	75


UGL_LOCAL UGL_MODE Z044_modes[] =
	{
		/* 320, 240, 16Bit Color, 60Hz */
		{Z044_WIDTH_320, Z044_HEIGHT_240, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 320, 240, 16Bit Color, 75Hz */
		{Z044_WIDTH_320, Z044_HEIGHT_240, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		/* 640, 480, 16Bit Color, 60Hz */
		{Z044_WIDTH_640, Z044_HEIGHT_480, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 640, 480, 16Bit Color, 75Hz */
		{Z044_WIDTH_640, Z044_HEIGHT_480, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		/* 800, 600, 16Bit Color, 60Hz */
		{Z044_WIDTH_800, Z044_HEIGHT_600, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 800, 600, 16Bit Color, 75Hz */
		{Z044_WIDTH_800, Z044_HEIGHT_600, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		/* 1024, 768, 16Bit Color, 60Hz */
		{Z044_WIDTH_1024, Z044_HEIGHT_768, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 1024, 768, 16Bit Color, 60Hz */
		{Z044_WIDTH_1024, Z044_HEIGHT_768, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		/* 1280, 1024, 16Bit Color, 60Hz */
		{Z044_WIDTH_1280, Z044_HEIGHT_1024, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 1280, 1024, 16Bit Color, 75Hz */
		{Z044_WIDTH_1280, Z044_HEIGHT_1024, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		/* 1280, 800, 16Bit Color, 60Hz */
		{Z044_WIDTH_1280, Z044_HEIGHT_800, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_60, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},
		/* 1280, 800, 16Bit Color, 60Hz */
		{Z044_WIDTH_1280, Z044_HEIGHT_800, Z044_COLOR_DEPTH_16, Z044_REFRESH_RATE_75, UGL_MODE_CRT, UGL_MODE_DIRECT_COLOR},

		};

UGL_LOCAL UGL_MODE * pDevModes = Z044_modes;

IMPORT STATUS sysWindMLDevCtrl( WINDML_DEVICE *,int,	int	*);

/* Forward Declarations */
UGL_STATUS men16z044Deinitialize( UGL_GENERIC_DRIVER* pGenDriver );
UGL_STATUS men16z044HwModeSet( UGL_UGI_DRIVER * pUgiDriver,
							   UGL_UINT32 swappedFb );
UGL_STATUS men16z044Initialize( UGL_GENERIC_DRIVER * pGenDriver );
UGL_STATUS men16z044ModeAvailable( UGL_UGI_DRIVER * pUgiDriver,
								   UGL_UINT32 * ctrlReg);
UGL_STATUS uglmen16z044DevDestroy( UGL_UGI_DRIVER * pDriver);
UGL_STATUS uglmen16z044Info( UGL_UGI_DRIVER * pDriver,
							 UGL_INFO_REQ infoRequest,
							 void *info);
UGL_STATUS uglmen16z044ModeAvailGet( UGL_UGI_DRIVER * pDriver,
									 UGL_UINT32  * pNumModes,
									 const UGL_MODE ** pModeArray);
UGL_STATUS uglmen16z044ModeSetSw( UGL_UGI_DRIVER * pUgiDriver, UGL_MODE * pMode);
UGL_STATUS uglmen16z044ModeSetNoSw( UGL_UGI_DRIVER * pUgiDriver, UGL_MODE * pMode);
UGL_LOCAL UGL_STATUS uglmen16z044ModeSet( UGL_UGI_DRIVER * pUgiDriver,
										  UGL_MODE * pMode,
										  UGL_UINT32 swappedFb);
UGL_STATUS uglmen16z044PageVisibleSet( UGL_UGI_DRIVER * pDriver,
									   UGL_PAGE * pPage);
UGL_STATUS uglmen16z044PageDrawSet( UGL_UGI_DRIVER * pDriver,
									UGL_PAGE * pPage);
/****************************** Z044_Swap16 **********************************
 *
 *  Description:  Swap bytes in word.
 *                cloned from oss_swap.c
 *
 *---------------------------------------------------------------------------
 *  Input......:  word      word to swap
 *  Output.....:  return    swapped word
 *  Globals....:  -
 ****************************************************************************/
UGL_UINT16 Z044_Swap16( UGL_UINT16 word )
{
	return( (word>>8) | (word<<8) );
}

/****************************** Z044_Swap32 **********************************
 *
 *  Description:  Swap bytes in double word.
 *                cloned from oss_swap.c
 *
 *---------------------------------------------------------------------------
 *  Input......:  dword     double word to swap
 *  Output.....:  return    swapped word
 *  Globals....:  -
 ****************************************************************************/
UGL_UINT32 Z044_Swap32( UGL_UINT32 dword )
{
	return(  (dword>>24) | (dword<<24) |
			 ((dword>>8) & 0x0000ff00) |
			 ((dword<<8) & 0x00ff0000)   );
}

/**************************************************************************
*
* menUglGraphicsDevOpen
*
* Open the pseudo PCI device by Name (pseudeo device/vendor ID as in devs )
*
*/

WINDML_DEVICE * menUglGraphicsDevOpen
(
	int vendorId,     /* vendor ID of device to open */
	int deviceId,     /* descriptor of device to open */
	int instance      /* instance of a device to open */
	)
{	
    char name[256];
    WINDML_DEVICE * pDevice;

    memset(name, 0x0, sizeof(name));

    /* Format device name and open device */
    sprintf(name, "%x,%x", vendorId, deviceId);
    pDevice = uglGraphicsDevOpenByName(name, instance);

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "-> menUglGraphicsDevOpen: name = '%s'  pDevice = %p\n",name,pDevice,0,0,0 );
#endif /* Z044_DEBUG */
    
	return (pDevice);

}

/**************************************************************************
*
* uglmen16z044DevCreate
*
* Initialize the graphics driver
*
*/
UGL_UGI_DRIVER * uglmen16z044DevCreate
(
	UGL_UINT32 instance, /* only 0 implemented */
	UGL_UINT32 param1,	 /* swap mode, ... */
	UGL_UINT32 param2	 /* not used */
)
{
	UGL_GENERIC_DRIVER *pGenDriver 	= UGL_NULL;
	UGL_UGI_DRIVER     *pDriver 	= UGL_NULL;

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "uglmen16z044DevCreate\n",0,0,0,0,0 );
#endif /* Z044_DEBUG */

	if (instance != 0) {
		uglLog( UGL_ERR_TYPE_FATAL,
				"uglmen16z044DevCreate(): only one device supported\n",
				0,0,0,0,0);
		goto CLEANUP;
	}

	/* Allocate device data structure from shared memory */
	pGenDriver = (UGL_GENERIC_DRIVER *)uglSharedMemAlloc(sizeof(UGL_GENERIC_DRIVER), 
														 UGL_MEM_CLEAR);

	if( UGL_NULL == pGenDriver ) {
		uglLog( UGL_ERR_TYPE_FATAL,	"uglmen16z044DevCreate(): not enough ressources\n",0,0,0,0,0);
		goto CLEANUP;
	}

	/* get device decriptor from BSP - i.e. framebuffer address */
	pGenDriver->pWmlDevice = menUglGraphicsDevOpen( MEN16Z044_DISP_UGL_ID1, MEN16Z044_DISP_UGL_ID2,	instance );

	if( !pGenDriver->pWmlDevice ) {
		uglLog( UGL_ERR_TYPE_FATAL,
				"uglmen16z044DevCreate(): uglGraphicsDevOpen(16Z044_DISP) failed\n",
				0,0,0,0,0);
		goto CLEANUP;
	}

	/* Initialize the driver (mandatory) */
	pDriver = (UGL_UGI_DRIVER *)pGenDriver;
	uglUgiDevInit(pDriver);

	pDriver->destroy			= uglmen16z044DevDestroy;
	pDriver->info				= uglmen16z044Info;
	pDriver->modeAvailGet		= uglmen16z044ModeAvailGet;
	if( param1 & Z044_PARAM1_SWAP_GRAPHIC_DATA ) {
		pDriver->modeSet		= uglmen16z044ModeSetSw;
	} else {
		pDriver->modeSet		= uglmen16z044ModeSetNoSw;
	}
	pGenDriver->fbAddress   	= (void *)0x00000000;

	/* Set Standard Driver API Functions */
	pDriver->bitmapBlt			= uglGeneric16BitBitmapBlt;
	pDriver->bitmapCreate		= uglGeneric16BitBitmapCreate;
	pDriver->bitmapDestroy		= uglGeneric16BitBitmapDestroy;
	pDriver->bitmapRead			= uglGeneric16BitBitmapRead;
	pDriver->bitmapWrite		= uglGeneric16BitBitmapWrite;
	pDriver->bitmapStretchBlt	= uglGeneric16BitBitmapStretchBlt;

	pDriver->monoBitmapBlt		= uglGeneric16BitMonoBitmapBlt;
	pDriver->monoBitmapCreate	= uglGeneric16BitMonoBitmapCreate;
	pDriver->monoBitmapDestroy	= uglGeneric16BitMonoBitmapDestroy;
	pDriver->monoBitmapRead		= uglGeneric16BitMonoBitmapRead;
	pDriver->monoBitmapWrite	= uglGeneric16BitMonoBitmapWrite;
	pDriver->monoBitmapStretchBlt	= uglGeneric16BitMonoBitmapStretchBlt;

	pDriver->transBitmapBlt		= uglGeneric16BitTransBitmapBlt;
	pDriver->transBitmapCreate	= uglGenericTransBitmapCreate;
	pDriver->transBitmapDestroy	= uglGenericTransBitmapDestroy;
	pDriver->transBitmapRead	= uglGenericTransBitmapRead;
	pDriver->transBitmapWrite	= uglGenericTransBitmapWrite;
	pDriver->transBitmapStretchBlt	= uglGeneric16BitTransBitmapStretchBlt;
	pDriver->transBitmapCreateFromDdb = uglGenericTransBitmapCreateFromDdb;

	pDriver->colorAlloc			= uglGenericColorAllocDirect;
	pDriver->colorFree			= UGL_NULL;
	pDriver->clutSet			= UGL_NULL;
	pDriver->clutGet			= UGL_NULL;
	pDriver->colorConvert	    = uglGeneric16BitColorConvert;
	pDriver->cursorBitmapCreate	= uglGenericCursorBitmapCreate;
	pDriver->cursorBitmapDestroy= uglGenericCursorBitmapDestroy;
	pDriver->cursorInit			= uglGenericCursorInit;
	pDriver->cursorDeinit		= uglGenericCursorDeinit;
	pDriver->cursorHide			= uglGenericCursorHide;
	pDriver->cursorImageGet		= uglGenericCursorImageGet;
	pDriver->cursorImageSet		= uglGenericCursorImageSet;
	pDriver->cursorMove			= uglGenericCursorMove;
	pDriver->cursorPositionGet	= uglGenericCursorPositionGet;
	pDriver->cursorOff			= uglGenericCursorOff;
	pDriver->cursorOn			= uglGenericCursorOn;
	pDriver->cursorShow			= uglGenericCursorShow;
	pDriver->ellipse			= uglGenericEllipse;
	pDriver->gcCopy				= uglGenericGcCopy;
	pDriver->gcCreate			= uglGenericGcCreate;
	pDriver->gcDestroy			= uglGenericGcDestroy;
	pDriver->gcSet				= uglGenericGcSet;
	pDriver->line				= uglGenericLine;
	pDriver->pixelGet			= uglGeneric16BitPixelGet;
	pDriver->pixelSet			= uglGeneric16BitPixelSet;
	pDriver->polygon			= uglGenericPolygon;
	pDriver->rectangle			= uglGenericRectangle;

	/* double buffering support */
	pDriver->pageCopy		= uglGenericPageCopy;
	pDriver->pageCreate		= uglGenericPageCreate;
	pDriver->pageDestroy	= uglGenericPageDestroy;
	pDriver->pageDrawGet	= uglGenericPageDrawGet;
	pDriver->pageDrawSet	= uglmen16z044PageDrawSet;
	pDriver->pageVisibleGet	= uglGenericPageVisibleGet;
	pDriver->pageVisibleSet	= uglmen16z044PageVisibleSet;
	/* double buffering support end */

	pDriver->memPoolCreate		= uglGenericMemPoolCreate;
	pDriver->memPoolDestroy		= uglGenericMemPoolDestroy;

	/* Set Generic Driver Functions */
	pGenDriver->bresenhamLine	= uglGeneric16BitBresenhamLine;

	pGenDriver->fbPixelGet		= uglGeneric16BitFbPixelGet;
	pGenDriver->fbPixelSet		= uglGeneric16BitFbPixelSet;

	pGenDriver->fill		= uglGeneric16BitFill;
	pGenDriver->hLine		= uglGeneric16BitHLine;
	pGenDriver->rectFill		= uglGeneric16BitRectFill;
	pGenDriver->vLine		= uglGeneric16BitVLine;

	pGenDriver->gpWait		= NULL /* no special driver routines */;

	return (pDriver);

CLEANUP:
	if( pGenDriver )
	{
		uglSharedMemFree ((char *)pGenDriver);
		pDriver = NULL;
	}

	uglLog( UGL_ERR_TYPE_FATAL,
			"uglmen16z044DevCreate(): failed\n",
			0,0,0,0,0);

	return (pDriver);
}

/***************************************************************************
*
* uglmen16z044Destroy - destroy an Z044 graphics device driver
*
*/
UGL_STATUS uglmen16z044DevDestroy
(
	UGL_UGI_DRIVER * pDriver
)
{
	UGL_GENERIC_DRIVER * pGenDriver = (UGL_GENERIC_DRIVER *)pDriver;

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "uglmen16z044DevDestroy()\n", 0,0,0,0,0);
#endif /* Z044_DEBUG */

	if (pDriver == UGL_NULL)
		return (UGL_STATUS_ERROR);

	men16z044Deinitialize( pGenDriver );

	/* Close the device at the BSP level */
	uglGraphicsDevClose (pGenDriver->pWmlDevice);

	if( pDriver->pPageZero )
	{
		if( pDriver->pPageZero->pDdb )
			UGL_FREE(pDriver->pPageZero->pDdb);

		UGL_FREE(pDriver->pPageZero);
	}


	UGL_FREE(pGenDriver);

	return (UGL_STATUS_OK);
}


/***************************************************************************
*
* uglmen16z044Info - get/set device information
*
*/
UGL_STATUS uglmen16z044Info
(
	UGL_UGI_DRIVER * pDriver,
	UGL_INFO_REQ infoRequest,
	void *info
)
{
	UGL_GENERIC_DRIVER *pGenDriver = (UGL_GENERIC_DRIVER *)pDriver;
	UGL_STATUS error = UGL_STATUS_ERROR;

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "uglmen16z044Info() infoRequest 0x%04x\n",
			infoRequest,0,0,0,0);
#endif /* Z044_DEBUG */

    if(pDriver->pMode == UGL_NULL)
        return(UGL_STATUS_ERROR);

	switch (infoRequest)
	{

		case UGL_FB_INFO_REQ:
		{
			UGL_FB_INFO *fbInfo = (UGL_FB_INFO *)info;
			fbInfo->width  = pDriver->pMode->width;
			fbInfo->height = pDriver->pMode->height;
			fbInfo->fbAddrs = pGenDriver->fbAddress;
			fbInfo->dsMemAmount = 0;
#ifdef INCLUDE_UGL_DOUBLE_BUFFERING
			fbInfo->flags = UGL_FB_PAGING_ENABLED;
#else
			fbInfo->flags = 0;
#endif
			error = UGL_STATUS_OK;
		}
		break;

		case UGL_COLOR_INFO_REQ:
		{
			UGL_COLOR_INFO *colorInfo = (UGL_COLOR_INFO *)info;
			colorInfo->cmodel = UGL_CMODEL_DIRECT;
			colorInfo->cspace = UGL_CSPACE_RGB;
			colorInfo->depth  = pDriver->pMode->colorDepth;;
			colorInfo->clutSize = 0;
			colorInfo->flags  = 0;
			error = UGL_STATUS_OK;
		}
		break;

		case UGL_MODE_INFO_REQ:
		{
			UGL_MODE_INFO * modeInfo = (UGL_MODE_INFO *)info;
			modeInfo->width 		= pDriver->pMode->width;
			modeInfo->height 		= pDriver->pMode->height;
			modeInfo->colorDepth 	= pDriver->pMode->colorDepth;
			modeInfo->fbAddress 	= pGenDriver->fbAddress;
			modeInfo->displayMemAvail = 0;

			modeInfo->colorModel = UGL_DIRECT;
			modeInfo->colorFormat = UGL_RGB565;
			modeInfo->clutSize = 0;
			modeInfo->flags = UGL_MODE_PAGING_ENABLED;
			error = UGL_STATUS_OK;
		}
		break;

#ifdef INCLUDE_UGL_EXT
		case UGL_EXT_INFO_REQ:
		{
#  ifdef INCLUDE_UGL_JPEG
			UGL_EXT_INFO * extInfo = (UGL_EXT_INFO *)info;

			int version;
			if (strcmp(extInfo->name, UGL_EXT_JPEG_NAME) == 0)
			{
				extInfo->pExt = (void *)uglGenJpegInit (
									   pDriver, &version);
				extInfo->version = version;
				error = UGL_STATUS_OK;
			}
#  endif /*INCLUDE_UGL_JPEG*/

		}
		break;
#endif /*INCLUDE_UGL_EXT*/

		default:
			uglLog( UGL_ERR_TYPE_WARN,
					"uglmen16z044Info() infoRequest 0x%08x\n",
					infoRequest,0,0,0,0);
			break;
	}

	return( error );
}

/**************************************************************************
*
* uglmen16z044ModeSetSw - set display mode, enable FB swapping in HW
*
*/
UGL_STATUS uglmen16z044ModeSetSw
(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_MODE * pMode
)
{
	return( uglmen16z044ModeSet( pUgiDriver, pMode, 1 ) );
}

/**************************************************************************
*
* uglmen16z044ModeSetNoSw - set display mode, disable FB swapping in HW
*
*/
UGL_STATUS uglmen16z044ModeSetNoSw
(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_MODE * pMode
)
{
	return( uglmen16z044ModeSet( pUgiDriver, pMode, 0 ) );
}

/**************************************************************************
*
* uglmen16z044ModeSet - set display mode
*
*/
UGL_STATUS uglmen16z044ModeSet
(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_MODE * pMode,
	UGL_UINT32 swappedFb
)
{
	UGL_BOOL allocate;
	UGL_GEN_DDB *pDdb;
	UGL_INT32 index;
	UGL_GENERIC_DRIVER *pGenDriver = (UGL_GENERIC_DRIVER *)pUgiDriver;
    UGL_UINT32 videoMemSize;
    void * videoMemAddress;

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "uglmen16z044ModeSet\n", 0,0,0,0,0);
#endif /* Z044_DEBUG */

	if( men16z044Initialize( pGenDriver ) != UGL_STATUS_OK ) {
		uglLog( UGL_ERR_TYPE_FATAL,
				"uglmen16z044ModeSet(): men16z044Initialize failed\n",
				0,0,0,0,0);
		return UGL_STATUS_ERROR;
	}

	/* First time through, pMode will be NULL */
	if (pUgiDriver->pMode == UGL_NULL)
	{
		allocate = UGL_TRUE;
	}
	else
	{
		allocate = UGL_FALSE;
	}

	/* Create pageZero (mandatory) */
	if(allocate == UGL_TRUE)
	{
		pUgiDriver->pPageZero = (UGL_PAGE *)UGL_CALLOC(1, sizeof(UGL_PAGE));
		pDdb = (UGL_GEN_DDB *)UGL_CALLOC(1, sizeof(UGL_GEN_DDB));
		pUgiDriver->pPageZero->pDdb = (UGL_DDB *)pDdb;
	}

	index = uglGenericModeFind(Z044_modes, pMode, NELEMENTS(Z044_modes));

	if (index > -1)
	{
		pUgiDriver->pMode = &Z044_modes[index];
	}
	else
	{
		return(UGL_STATUS_ERROR);
	}

	pUgiDriver->pPageZero->pDdb->height = pUgiDriver->pMode->height;
	pUgiDriver->pPageZero->pDdb->width = pUgiDriver->pMode->width;
	pUgiDriver->pPageZero->pDdb->type = UGL_DDB_TYPE;
	((UGL_GEN_DDB *)pUgiDriver->pPageZero->pDdb)->stride = 	pUgiDriver->pMode->width;
	((UGL_GEN_DDB *)pUgiDriver->pPageZero->pDdb)->colorDepth = 	pUgiDriver->pMode->colorDepth;
	((UGL_GEN_DDB *)pUgiDriver->pPageZero->pDdb)->image = 	pGenDriver->fbAddress;

	pGenDriver->pDrawPage = pUgiDriver->pPageZero;
	pGenDriver->pVisiblePage = pUgiDriver->pPageZero;


	/* Create a memory pool with the unused portion of video memory */
	videoMemAddress = (void *) ((UGL_UINT32)pGenDriver->fbAddress +
								pUgiDriver->pMode->width * pUgiDriver->pMode->height *
								(pUgiDriver->pMode->colorDepth / 8));

	/* FB size hard coded here. */
	/* TBD: use baseSize returned from WINDML_PCI_BASE_SIZE_GET? */
	videoMemSize = (UGL_UINT32)0x00800000 -((pUgiDriver->pMode->width *
											 pUgiDriver->pMode->height) *
											(pUgiDriver->pMode->colorDepth / 8));

	if(videoMemSize > ((pUgiDriver->pMode->width * pUgiDriver->pMode->height) * 2))
		{
		pGenDriver->videoMemPoolId = uglMemPoolCreate(videoMemAddress,
													  videoMemSize);
		}
	else
		{
		pGenDriver->videoMemPoolId = UGL_NULL;
		}

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO,
			"uglmen16z044ModeSet() w/h/bpp %d %d %d refresh %d  fb %08x\n",
			pUgiDriver->pMode->width,
			pUgiDriver->pMode->height,
			pUgiDriver->pMode->colorDepth,
			pUgiDriver->pMode->refreshRate,
			(int)pGenDriver->fbAddress);
	uglLog( UGL_ERR_TYPE_INFO,
			"                      videoMemPool: Id %d image: %p imgSize: %p\n",
			(int)pGenDriver->videoMemPoolId,
			(int)videoMemAddress,
			(int)videoMemSize,
			0, 0);
#endif /* Z044_DEBUG */

	/* Clear the screen */
#ifdef Z044_START_WITH_CLEAR_SCREEN
	memset(pGenDriver->fbAddress, 0x0, pUgiDriver->pMode->width *
			pUgiDriver->pMode->height * (pUgiDriver->pMode->colorDepth / 8));
#endif

	men16z044HwModeSet( pUgiDriver, swappedFb );

	return (UGL_STATUS_OK);
}

/**************************************************************************
*
* uglmen16z044ModeAvailGet - get available display modes
*
*/
UGL_STATUS uglmen16z044ModeAvailGet
	(
	UGL_UGI_DRIVER * pDriver,
	UGL_UINT32  * pNumModes,
	const UGL_MODE ** pModeArray
	)
	{

	/* return the mode list */
	*pModeArray = pDevModes;
	*pNumModes = NELEMENTS(Z044_modes);


	return(UGL_STATUS_OK);
	}

/***************************************************************************
* uglmen16z044PageDrawSet - Set active drawing page.
*
* RETURNS: UGL_STATUS_OK or UGL_STATUS_ERROR
*
*/

UGL_STATUS uglmen16z044PageDrawSet
	(
	UGL_UGI_DRIVER * pDriver,
	UGL_PAGE * pPage
	)
	{
	UGL_GENERIC_DRIVER *pGenDriver = (UGL_GENERIC_DRIVER *)pDriver;

	pGenDriver->pDrawPage = pPage;

	return(UGL_STATUS_OK);
	}


/***************************************************************************
* uglmen16z044PageVisibleSet - Set the active display page.
*
* RETURNS: UGL_STATUS_OK or UGL_STATUS_ERROR
*
*/

UGL_STATUS uglmen16z044PageVisibleSet
	(
	UGL_UGI_DRIVER * pDriver,
	UGL_PAGE * pPage
	)
	{
	UGL_GENERIC_DRIVER *pGenDriver = (UGL_GENERIC_DRIVER *)pDriver;

#ifdef _WRS_KERNEL
	UGL_UINT32 address;

	address = (UGL_UINT32)((UGL_GEN_DDB *)pPage->pDdb)->image -
	(UGL_UINT32)pGenDriver->fbAddress;

	/* Set address register to flip page */
	Z044_REG_OUT(Z044_FOS, address);

	pGenDriver->pVisiblePage = pPage;
#else  /* _WERS_KERNEL */

	MEN16Z044_PAGE men_page;

	men_page.pUgiDriver = pDriver;
	men_page.pPage = pPage;
	/* set visable page when execution in RTP */
	ioctl ( pGenDriver->pWmlDevice->fd,
			WINDML_MEN16Z044_PAGE_VISABLE_SET,
			(int)&men_page);

#endif /* _WRS_KERNEL */
	return(UGL_STATUS_OK);
	}


/*******************************************************************************
*
* men16z044Initialize - look for supported device and init hardware
*
* RETURNS: OK, if successful else ERROR
*
* NOTE:  This routine is present for both the kernel and RTP mode
*
*/
STATUS men16z044Initialize
(
	UGL_GENERIC_DRIVER * pGenDriver
)
{
#ifdef _WRS_KERNEL
	WINDML_BASE_SIZE baseSize = {1,0,0};
	STATUS error = ERROR;
	UINT32 virtAdrs;

	/* The address spaces of the chameleon FPGA devices are already mapped
	   by the BSP. Still a lot of stuff is done in uglGraphicsDevMap
	   if uglGraphicsDevMap is called with size 0 it trys to get the size
	   from the PCI config space. This will fail (no real PCI device).
	   Therefore we have to get the size of the framebuffer from the BSP.
	   We can only map here the framebuffer memory space. The registers space
	   is mapped by the BSP and can not be unmapped (required in
	   uglGraphicsDevMapWithType) since it does not cover a complete BAR and
	   usually is not at the beginning of a BAR space. */

	/* Get size of FB address space from BSP and map physical memory */

	error = sysWindMLDevCtrl (pGenDriver->pWmlDevice, WINDML_PCI_BASE_SIZE_GET, (void *)&baseSize);

	if (error != OK)
	{
		uglLog( UGL_ERR_TYPE_WARN,
				"men16z044Initialize(): get FB size failed, assuming 16MB\n",
				0,0,0,0,0);
		/* set default value: 16MB */
		baseSize.size = 0x1000000;
	}

	/* determined above already */
	pGenDriver->pWmlDevice->pUglDetermineSizeAndBase = 0;

	/* Map physical memory to virtual - evtl. probe base address and size     */
	/* If no size is provided the address has to be a Base address of the PCI */
	/* device and size is determined.   */
	/* If not so the size has to be provided here (e.g. for Chameleon devices)*/
#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO,
			"pGenDriver->pWmlDevice->pPhysBaseAdrs1=0x%08x\n",
			(UINT32)pGenDriver->pWmlDevice->pPhysBaseAdrs1,0,0,0,0 );
#endif

	if( (error = uglGraphicsDevMap( pGenDriver->pWmlDevice,
									(UINT32)pGenDriver->pWmlDevice->pPhysBaseAdrs1,
									baseSize.size,
									&virtAdrs )) != UGL_STATUS_OK )
	{
		uglLog( UGL_ERR_TYPE_FATAL,
				"men16z044Initialize(): uglGraphicsDevMap() failed\n",
				0,0,0,0,0);
		return error;
	}

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO,
			"men16z044Initialize(): physBase1=0x%08x, virtAddr=0x%08x\n",
			(UINT32)pGenDriver->pWmlDevice->pPhysBaseAdrs1,
			(int)virtAdrs,
			0,0,0);
#endif /* Z044_DEBUG */

	/* Set the base address of the frame buffer */
	pGenDriver->fbAddress = (void *)virtAdrs;
	if( pGenDriver->fbAddress == (void *)0x00000000 ||
		pGenDriver->fbAddress == (void *)0xFFFFFFFF )
	{
		return UGL_STATUS_ERROR;
	}
#else /* _WRS_KERNEL */

	/* Initialize when execution in RTP */
	ioctl (pGenDriver->pWmlDevice->fd,WINDML_MEN16Z044_INIT, (int)pGenDriver);

#endif /* _WRS_KERNEL */

	return UGL_STATUS_OK;
} /* men16z044Initialize */

/*******************************************************************************
*
* men16z044Deinitialize - deinit hardware
*
* RETURNS: OK, if successful else ERROR
*
* NOTE:  This routine is present for both the kernel and RTP mode
*
*/

UGL_STATUS men16z044Deinitialize
(
	UGL_GENERIC_DRIVER * pGenDriver
)
{
#ifdef _WRS_KERNEL
	/* Disable display output */
	UGL_UINT32 ldata = Z044_CTL_IN;

#ifndef Z044_ALWAYS_ENABLED
	ldata |=Z044_CTRL_ONOFF;
#endif

	Z044_CTL_OUT( ldata | Z044_CTRL_CHANGE );

#else /* _WRS_KERNEL */

	/* Initialize when execution in RTP */
	ioctl (pGenDriver->pWmlDevice->fd, WINDML_MEN16Z044_INIT, (int)pGenDriver);

#endif /* _WRS_KERNEL */

#ifdef Z044_DEBUG
	uglLog( UGL_ERR_TYPE_INFO, "men16z044Deinitialize()\n", 0,0,0,0,0);
#endif /* Z044_DEBUG */

	return UGL_STATUS_OK;
} /* men16z044Deinitialize */

/*******************************************************************************
*
* men16z044HwModeSet - set mode in HW
*
* RETURNS: OK, if successful else ERROR
*
* NOTE:  This routine is present for both the kernel and RTP mode
*
*/

UGL_STATUS men16z044HwModeSet
(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_UINT32       swappedFb
)
{
	UGL_GENERIC_DRIVER * pGenDriver = (UGL_GENERIC_DRIVER *)pUgiDriver;

#ifdef _WRS_KERNEL
	UGL_UINT32 ldata = 0;

	/* Disable display output */
	ldata = (Z044_CTL_IN) | Z044_CTRL_ONOFF;
	Z044_CTL_OUT( ldata | Z044_CTRL_CHANGE );
	if( men16z044ModeAvailable( pUgiDriver, &ldata ) != UGL_STATUS_OK )
	{
		uglLog( UGL_ERROR_BAD_MODE,
				"men16z044HwModeSet(): mode not supported (\n",
				(int)pGenDriver->pWmlDevice->pPhysBaseAdrs0, ldata, 0,0,0);
		return( UGL_STATUS_ERROR );
	}

	/* Set mode and enable display output */
	if( swappedFb )
	{
		ldata |= Z044_CTRL_BSWAP;
	} else
	{
		ldata &= ~Z044_CTRL_BSWAP;
	}

	/* turn display on */
	ldata &= ~Z044_CTRL_ONOFF;

#if 0 /* enable output of debug rectangle */
	ldata |= Z044_CTRL_DEBUG;
#endif

	/* printf("men16z044HwModeSet(): Z044_CTRL Reg: 0x%08x\n", ldata); */
	Z044_CTL_OUT( ldata | Z044_CTRL_CHANGE );

#ifdef Z044_DEBUG
	ldata = Z044_CTL_IN;
	uglLog( UGL_ERR_TYPE_INFO,
			"men16z044HwModeSet(): CTRL Reg: addr: 0x%08x   value: 0x%08x\n",
			(int)pGenDriver->pWmlDevice->pPhysBaseAdrs0, ldata, 0,0,0);
#endif /* Z044_DEBUG */

	return( UGL_STATUS_OK );

#else /* _WRS_KERNEL */

	/* set mode in HW when execution in RTP */
	if( swappedFb )
	{
		return( ioctl( pGenDriver->pWmlDevice->fd,
					   WINDML_MEN16Z044_HWMODE_SET_SW,
					   (int)pUgiDriver) );
	} else
	{
		return( ioctl( pGenDriver->pWmlDevice->fd,
					   WINDML_MEN16Z044_HWMODE_SET,
					   (int)pUgiDriver) );
	}

#endif /* _WRS_KERNEL */
}

#ifdef _WRS_KERNEL

/*******************************************************************************
*
* men16z044CheckMaskInCtrlReg - helper function for men16z044ModeAvailable()
*
* RETURNS:  UGL_STATUS_OK or UGL_STATUS_ERROR
*
* SEE ALSO:
*/
UGL_STATUS men16z044CheckMaskInCtrlReg(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_UINT32 bitSet,
	UGL_UINT32 bitMask
)
{
	UGL_GENERIC_DRIVER * pGenDriver = (UGL_GENERIC_DRIVER *)pUgiDriver;
	UGL_UINT32 ctrl_reg_bkup, ldata;

	/* save control register */
	ctrl_reg_bkup = (Z044_CTL_IN) & ~Z044_CTRL_CHANGE;

	Z044_CTL_OUT( (ctrl_reg_bkup & bitMask) | bitSet );
	ldata = (Z044_CTL_IN) & bitMask;

	/* restore control register */
	Z044_CTL_OUT( ctrl_reg_bkup );

	if( ldata != bitSet )
		return( UGL_STATUS_ERROR );

	return UGL_STATUS_OK;
}
/*******************************************************************************
*
* men16z044ModeAvailable - check if mode is available/settable in HW
*
* RETURNS:  UGL_STATUS_OK or UGL_STATUS_ERROR
*
* SEE ALSO:
*/
UGL_STATUS men16z044ModeAvailable(
	UGL_UGI_DRIVER * pUgiDriver,
	UGL_UINT32 *pCtrlReg
)
{
	UGL_MODE *pMode = pUgiDriver->pMode;
	/* check color depth */
	if( pMode->colorDepth != 16 )
	{
		goto ERR_EXIT;
	}

	/* check if resolution is supported by FPGA */
	if( pMode->height == Z044_HEIGHT_600 &&	pMode->width  == Z044_WIDTH_800 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_800X600,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_800X600;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->height == Z044_HEIGHT_768 && pMode->width  == Z044_WIDTH_1024 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_1024X768,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_1024X768;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->height == Z044_HEIGHT_1024 && pMode->width  == Z044_WIDTH_1280 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_1280X1024,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_1280X1024;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->height == Z044_HEIGHT_480 && pMode->width  == Z044_WIDTH_640 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_640X480,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_640X480;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->height == Z044_HEIGHT_240 &&	pMode->width  == Z044_WIDTH_320 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_320X240,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_320X240;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->height == Z044_HEIGHT_800 &&	pMode->width  == Z044_WIDTH_1280 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_RES_1280X800,
										 Z044_CTRL_RES_MASK ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg = (*pCtrlReg & !Z044_CTRL_RES_MASK ) |
						Z044_CTRL_RES_1280X800;
		} else
		{
			goto ERR_EXIT;
		}
	} else {
		goto ERR_EXIT;
	}

	/* check refresh frequency */
	if( pMode->refreshRate == 75 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 Z044_CTRL_REFRESH_75,
										 Z044_CTRL_REFRESH_75 ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg |= Z044_CTRL_REFRESH_75;
		} else
		{
			goto ERR_EXIT;
		}
	} else if( pMode->refreshRate == 60 )
	{
		if( men16z044CheckMaskInCtrlReg( pUgiDriver,
										 0x00,
										 Z044_CTRL_REFRESH_75 ) ==
			UGL_STATUS_OK )
		{
			*pCtrlReg &= ~Z044_CTRL_REFRESH_75;
		} else
		{
			goto ERR_EXIT;
		}
	} else
	{
		goto ERR_EXIT;
	}

	return( UGL_STATUS_OK );
ERR_EXIT:
 	uglLog( UGL_ERR_TYPE_FATAL,
			"men16z044ModeAvailable(): mode not supported: %dx%d refRate %d "
			"colDepth %d\n",
			pMode->width, pMode->height,
			pMode->refreshRate, pMode->colorDepth, 0);
	return( UGL_STATUS_ERROR );

}

/*******************************************************************************
*
* uglmen16z044Ioctl - graphics driver kernel level ioctl
*
* This routine handles the ioctl calls made by the upper level driver.  It
* provides the processing for a user level graphics driver that requires the
* services to be performed at the kernel level.
*
* RETURNS:  OK
*
* SEE ALSO:
*/
int uglmen16z044Ioctl
(
	WINDML_GRAPHICS_DEV_DESC *dev,          /* Device descriptor */
	int                      request,       /* ioctl function */
	int                      arg            /* function arg */
)
{

	STATUS  result = OK;

	switch (request)
	{
		case WINDML_MEN16Z044_INIT:
		{
			result = men16z044Initialize( (UGL_GENERIC_DRIVER *)arg );
			break;
		}
		case WINDML_MEN16Z044_DEINIT:
		{
			result = men16z044Deinitialize( (UGL_GENERIC_DRIVER *)arg );
			break;
		}
		case WINDML_MEN16Z044_PAGE_VISABLE_SET:
		{
			MEN16Z044_PAGE *pMenPage = (MEN16Z044_PAGE *)arg;
			result = uglmen16z044PageVisibleSet( pMenPage->pUgiDriver,
												 pMenPage->pPage );
			break;
		}
		case WINDML_MEN16Z044_HWMODE_SET:
		{
			men16z044HwModeSet( (UGL_UGI_DRIVER *)arg, 0 );
			break;
		}
		case WINDML_MEN16Z044_HWMODE_SET_SW:
		{
			men16z044HwModeSet( (UGL_UGI_DRIVER *)arg, 1 );
			break;
		}
		default:
		{
			/* Unknown driver extension ioctl code */
			errno = S_ioLib_UNKNOWN_REQUEST;
			result = ERROR;
		}
	}

	return (result);

}

#endif

