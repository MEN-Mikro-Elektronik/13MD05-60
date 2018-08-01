/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  ata2_chipset_16z016_ide.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2008/04/30 16:19:13 $
 *    $Revision: 1.5 $
 *
 *	   \project  many
 *  	 \brief  ATA2 chipset specific part for FPGA 16Z016_IDE unit
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 * $Log: ata2_chipset_16z016_ide.c,v $
 * Revision 1.5  2008/04/30 16:19:13  cs
 * added support for 16Z016-01 (UDMA mode selectable)
 *
 * Revision 1.4  2007/03/26 16:21:06  ufranke
 * added
 *  + define MEN_ATA2COM_COMPILE
 *
 * Revision 1.3  2007/02/27 20:19:20  rt
 * make file compilable again
 *
 * Revision 1.2  2006/12/22 15:06:13  cs
 * added:
 *    - chipset now knows funktions initLate, intPreProc, intPostProc
 *
 * Revision 1.1  2005/09/01 08:44:46  kp
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/


#define MEN_ATA2COM_COMPILE
#include "ata2_int.h"

/* defines */
#define Z16_IDE_CAP_PIO_OFF		0xC0		/**< PIO capability bitmask */
#define Z16_IDE_CAP_UDMA_OFF	0xC1		/**< UDMA capability bitmask */
#define Z16_IDE_MODE_SET_OFF	0xC2		/**< Mode adjustment register */
#define Z16_IDE_API_VER_OFF		0xC3		/**< API Version register */

#define Z16_IDE_MODE_SET_MASK_PIO	0x07	/**< PIO mode */
#define Z16_IDE_MODE_SET_MASK_UDMA	0x70	/**< UDMA mode */

/* debug support */
#if defined(DBG) && !defined(DBG_MYLEVEL)
	#define DBG_MYLEVEL	h->debugLevel
	#define DBH			h->dbh
#endif

/**********************************************************************/
/** Setup ATA2 handle for chipset
 *
 * The caller of ATA2_Create() must have filled maRegBlock[0] with the
 * base address of the FPGA IP core 16Z016_IDE
 *
 * \param ataH			\IN ATA handle
 *
 * \return RBDRV error code
 */
static int32 Chipset16Z016IdeInit(ATA2_HANDLE_T *ataH)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;

	h->chipsetData = NULL;
	h->desc.maRegBlockDma = h->desc.maRegBlock[1] = h->desc.maRegBlock[0];

	h->regOffs.data 	= 0x0;
	h->regOffs.error	= 0x4;
	h->regOffs.feature	= 0x4;
	h->regOffs.seccnt 	= 0x8;
	h->regOffs.sector	= 0xc;
	h->regOffs.cylLo 	= 0x10;
	h->regOffs.cylHi 	= 0x14;
	h->regOffs.sdh 		= 0x18;
	h->regOffs.command 	= 0x1c;
	h->regOffs.status 	= 0x1c;
	h->regOffs.altStatus= 0x38;

	h->regOffs.devControl= 0x38;
	h->regOffs.drvAdr	= 0x3c;

	h->regOffs.bmIdeCommand = 0x40;
	h->regOffs.bmIdeStatus = 0x42;
	h->regOffs.bmIdePrdTable = 0x44;

	return 0;
}
/**********************************************************************/
/** get next lower UDMA capability
 *
 * \return RBDRV error code
 */
static int32 Z016IdeModeGetNextDmaMode(
	ATA2_HANDLE_T *ataH,
	int *udmaReqP,
	int *udmaCapP  )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;
	MACCESS ma      = h->desc.maRegBlock[0];
	u_int8 udmaCap = MREAD_D8(ma,Z16_IDE_CAP_UDMA_OFF);
	int8 i;
	int selMode = *udmaReqP - (ATA_DMA_ULTRA_0);

	/* sanity check: old API / invalid mode requested */
	if( !udmaCap ||
		((0 > selMode) || (selMode > 6   )) )
	{
		return RBDRV_NOT_SUPPORTED;
	}

	for( i = selMode; i >= 0; i-- ) {
		if( (udmaCap & (0x01 << i)) ) { /* capability given? */
			selMode = i;
			break;
		}
		if( i == 0 ) { /* should not get here, no supported mode found */
			return RBDRV_NOT_SUPPORTED;
		}
	}
	*udmaCapP = (ATA_DMA_ULTRA_0) + selMode;
	DBGWRT_3((DBH, "ATA::Z016IdeModeGetNextDmaMode: udmaMode=0x%x\n", *udmaCapP));
	return 0;
}
/**********************************************************************/
/** setup max. possible PIO and UDMA mode
 * If this function pointer is NULL, PIO default mode and no UDMA is used.
 *
 * \param subdev	\IN 0=master, 1=slave
 * \param pioModeP	\IN requested PIO mode \OUT actualPIO  mode
 *					ATA_PIO_xxx
 * \param dmaModeP	\IN requested UDMA mode \OUT actual UDMA mode
 *					ATA_DMA_ULTRA_xxx or 0 for no DMA
 */
static int32 Chipset16Z016IdeSetMode(
	ATA2_HANDLE_T *ataH,
	int subdev,
	int *pioModeP,
	int *dmaModeP )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;
	MACCESS ma      = h->desc.maRegBlock[0];

	if( subdev<0 || subdev>1 )
		return RBDRV_BAD_PARAMETER;

	*pioModeP = ATA_PIO_DEF_W;

	/* 16Z016-01 knows multiple UDMA modi (MODE register)
	 *       -00 knows UDMA 5 only                   */
	/* get API Version, for 16Z016-00 this Registers default to 0! */
	if( MREAD_D8(ma,Z16_IDE_API_VER_OFF) ) { /* API Version > 0 */
		/* adjust UDMA mode if possible
		 * when requested mode available: set
		 * else:                          set next slower one available */
		int dmaSel = 0;
		if( Z016IdeModeGetNextDmaMode( ataH, dmaModeP, &dmaSel  ) != RBDRV_NOT_SUPPORTED ) {
			MWRITE_D8( ma, Z16_IDE_MODE_SET_OFF, (dmaSel - (ATA_DMA_ULTRA_0)) << 4 );
			*dmaModeP = dmaSel;
		} else {
			*dmaModeP = 0;
		}

	} else {
		if( *dmaModeP >= ATA_DMA_ULTRA_5 )
			*dmaModeP = ATA_DMA_ULTRA_5;
		else
			*dmaModeP = 0;
	}

	if( subdev == 1 )
		*dmaModeP = 0;			/* don't know if slave will work with DMA */

	DBGWRT_1((DBH, "ATA::Chipset16Z016IdeSetMode: pioMode=0x%x dmaMode=0x%x\n", *pioModeP, *dmaModeP));

	return 0;
}

/** The chipset class for 16Z016_IDE */
const ATA2_CHIPSET_T ATA2_chipset16Z016Ide = {
	Chipset16Z016IdeInit,		/* init */
	NULL,						/* initLate */
	NULL,						/* term */
	Chipset16Z016IdeSetMode,	/* setMode */
	NULL,						/* intPreProc */
	NULL						/* intPostProc */
};
