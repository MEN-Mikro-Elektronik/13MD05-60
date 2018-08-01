/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  ata2_chipset_common_ide.c
 *
 *      \author  klaus.popp@men.de/uf
 *        $Date: 2007/03/26 16:20:44 $
 *    $Revision: 1.3 $
 *
 *	   \project  many
 *  	 \brief  ATA2 chipset specific part for COMMON IDE unit
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 * $Log: ata2_chipset_common_ide.c,v $
 * Revision 1.3  2007/03/26 16:20:44  ufranke
 * added
 *  + define MEN_ATA2COM_COMPILE
 *
 * Revision 1.2  2006/12/22 15:06:26  cs
 * added:
 *    - chipset now knows funktions initLate, intPreProc, intPostProc
 *
 * Revision 1.1  2005/11/09 12:31:09  UFranke
 * Initial Revision
 *
 * cloned from ata2_chipset_16z023_ide.c Revision 1.1  2005/08/31 14:54:51  kp
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#define MEN_ATA2COM_COMPILE
#include "ata2_int.h"


/**********************************************************************/
/** Setup ATA2 handle for chipset
 *
 * The caller of ATA2_Create() must have filled maRegBlock[0] with the
 * base address of the FPGA IP core 16Z053_IDE i.e. 0x900021F0
 * or common IDE 0x000001F0
 *
 * \param ataH			\IN ATA handle
 *
 * \return RBDRV error code
 */
static int32 ChipsetCommonIdeInit(ATA2_HANDLE_T *ataH)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;

	h->chipsetData = NULL;

	/* h->desc.maRegBlock[0] */
	h->regOffs.data 	= 0x0;
	h->regOffs.error	= 0x1;
	h->regOffs.feature	= 0x1;
	h->regOffs.seccnt 	= 0x2;
	h->regOffs.sector	= 0x3;
	h->regOffs.cylLo 	= 0x4;
	h->regOffs.cylHi 	= 0x5;
	h->regOffs.sdh 		= 0x6;
	h->regOffs.command 	= 0x7;
	h->regOffs.status 	= 0x7;

	/* h->desc.maRegBlock[1] */
	h->regOffs.altStatus = 0x6;
	h->regOffs.devControl= 0x6;
	h->regOffs.drvAdr	 = 0x7;
	return 0;
}

/** The chipset class for  Common IDE */
const ATA2_CHIPSET_T ATA2_chipsetCommonIde = {
	ChipsetCommonIdeInit,		/* init */
	NULL,						/* initLate */
	NULL,						/* term */
	NULL,						/* setMode */
	NULL,						/* intPreProc */
	NULL						/* intPostProc */
};

