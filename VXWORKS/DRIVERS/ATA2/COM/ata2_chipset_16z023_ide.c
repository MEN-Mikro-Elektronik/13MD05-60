/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  ata2_chipset_16z023_ide.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2008/11/12 17:35:16 $
 *    $Revision: 1.4 $
 *
 *	   \project  many
 *  	 \brief  ATA2 chipset specific part for FPGA 16Z023_IDE unit
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 * $Log: ata2_chipset_16z023_ide.c,v $
 * Revision 1.4  2008/11/12 17:35:16  rt
 * R:1. Make compilable again.
 * M:1. New MCLRMASK_D32/MSETMASK_D32 implementations need brackets.
 *
 * Revision 1.3  2007/03/26 16:21:21  ufranke
 * added
 *  + define MEN_ATA2COM_COMPILE
 *
 * Revision 1.2  2006/12/22 15:06:19  cs
 * added:
 *    - chipset now knows funktions initLate, intPreProc, intPostProc
 *
 * Revision 1.1  2005/08/31 14:54:51  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/


#define MEN_ATA2COM_COMPILE
#include "ata2_int.h"
#include <MEN/16z023_ide.h>
typedef struct
{
	u_int8 irq; 		/* last read/handled state of Interrupt Request register */
	u_int32 hGotSize; 	/* memory size actually allocated for me */
} CHIPSET_16Z023_DATA;

/**********************************************************************/
/** Setup ATA2 handle for chipset
 *
 * The caller of ATA2_Create() must have filled maRegBlock[0] with the
 * base address of the FPGA IP core 16Z023_IDE / 16Z023_IDENHS
 *
 * \param ataH			\IN ATA handle
 *
 * \return RBDRV error code
 */
static int32 Chipset16Z023IdeInit(ATA2_HANDLE_T *ataH)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;
	u_int32 gotSize = 0;

	CHIPSET_16Z023_DATA *chipsetData;

	if( (chipsetData = (CHIPSET_16Z023_DATA *)
	   OSS_MemGet( h->osh, sizeof(CHIPSET_16Z023_DATA), &gotSize )) == NULL )
		return RBDRV_OUT_OF_RESOURCE;

	OSS_MemFill( h->osh, sizeof(*chipsetData), (char*)chipsetData, 0 );

	chipsetData->hGotSize = gotSize;

	h->chipsetData = chipsetData;
	h->desc.maRegBlock[1] = h->desc.maRegBlock[0];

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

	/* disable interrupts, just in case */
	MCLRMASK_D8( h->desc.maRegBlock[0], Z023_IDE_CONTROL,
				 Z023_IDE_IRQ_ATA | Z023_IDE_IRQ_CD_CHG );

	return 0;
}

/**********************************************************************/
/** Setup chipset after handles are initialized
 *
 * - enables interrupts in Z023_IDE_CONTROL
 *
 * \param ataH			\IN ATA handle
 *
 * \return RBDRV error code
 */
int32 Chipset16Z023IdeInitLate(ATA2_HANDLE_T *ataH)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)ataH;

	/* enable interrupts */
	MSETMASK_D8( h->desc.maRegBlock[0], Z023_IDE_CONTROL,
				 Z023_IDE_IRQ_ATA | Z023_IDE_IRQ_CD_CHG );
	return 0;

}

/********************************* Z023IdePowerControl **********************
 *
 *  Description: Turn on or off power of CF card
 *---------------------------------------------------------------------------
 *  Input......: h		  internal rbdrv handle
 *				 state	  FALSE (poweroff) or TRUE (poweron)
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void ChipsetZ023IdePowerControl( ATA_HANDLE_T *h, int state )
{
	MACCESS ma      = h->desc.maRegBlock[0];
	if( state ) {
		MSETMASK_D8( ma, Z023_IDE_CONTROL, Z023_IDE_CTRL_POW );
	}
	else {
		MCLRMASK_D8( ma, Z023_IDE_CONTROL, Z023_IDE_CTRL_POW );
	}
	return;
}

/******************************************************************************
*
* Chipset16Z023IntPrePoc - interrupt pre processor for 16Z023_IDE
*
*
*/
static int32 Chipset16Z023IntPreProc(
	ATA2_HANDLE_T *_h
)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	MACCESS ma      = h->desc.maRegBlock[0];
	u_int8 status, ctrl;
	CHIPSET_16Z023_DATA *chData = (CHIPSET_16Z023_DATA*)h->chipsetData;

	/*logMsg("Chipset16Z023IntPreProc\n",0,0,0,0,0,0);*/
	status = MREAD_D8( ma, Z023_IDE_IRQ_PENDING );

	if( status & Z023_IDE_IRQ_CD_CHG ){
		/* don't delete ATA IRQs
		 * when CD interrupt is handled ATA interrupts are ignored in driver
		 * after power off no handling possible */
		chData->irq = status & Z023_IDE_IRQ_CD_CHG;

		ctrl = MREAD_D8( ma, Z023_IDE_CONTROL );

		if( !(ctrl & Z023_IDE_CTRL_CD) ) { 			/* insert */
			ChipsetZ023IdePowerControl( h, TRUE );
			return( ATA2_CHIPS_ISTAT_CHANGE );
		} else if( ctrl & Z023_IDE_CTRL_CD ) { 		/* remove */
			ChipsetZ023IdePowerControl( h, FALSE );
			return( ATA2_CHIPS_ISTAT_REMOVE );
		}
	}

	chData->irq = status;
	if( status & Z023_IDE_IRQ_ATA )
		return( ATA2_CHIPS_ISTAT_OK );
	else
		return( ATA2_CHIPS_ISTAT_NOIRQ );
}

/******************************************************************************
*
* Chipset16Z023IntPostPoc - interrupt post processor for 16Z023_IDE
*
*
*/
static void Chipset16Z023IntPostProc(
	ATA2_HANDLE_T *_h
)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	MACCESS ma      = h->desc.maRegBlock[0];
	CHIPSET_16Z023_DATA *chData = (CHIPSET_16Z023_DATA*)h->chipsetData;

	/* logMsg("Chipset16Z023IntPostProc\n",0,0,0,0,0,0); */
	MWRITE_D8( ma, Z023_IDE_IRQ_PENDING, chData->irq );
	return;
}

/** The chipset class for 16Z023_IDE */
const ATA2_CHIPSET_T ATA2_chipset16Z023Ide = {
	Chipset16Z023IdeInit,		/* init */
	Chipset16Z023IdeInitLate,	/* initLate */
	NULL,						/* term */
	NULL,						/* setMode */
	Chipset16Z023IntPreProc,	/* interrupt pre processing */
	Chipset16Z023IntPostProc	/* interrupt post processing */
};
