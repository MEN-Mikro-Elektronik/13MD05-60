/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sys16z053ide.c
 *      Project: VxWorks for EM3 family
 *
 *       Author: ts
 *        $Date: 2011/09/10 10:37:58 $
 *    $Revision: 1.5 $
 *
 *  Description: Stub driver to support any standard ATA conform IDE
 *               Interface (here NAND IDE 16Z053) which is represented thru
 *               a FPGA unit.
 *				 This file should be included in sysLib.c
 *				 Uses MENs own menata2.c / ata2.c (replacement for WRS driver)
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sys16z053ide.c,v $
 * Revision 1.5  2011/09/10 10:37:58  ts
 * R: BSP needed for EM01A09 on vxWorks 6.8
 * M: ported driver to 6.8
 *
 * Revision 1.4  2010/01/22 09:14:33  cs
 * R: Init could cause system corruption when called with
 *    index into ATA_RESOURCE bigger than ATA_MAX_CTRLS
 * M: check index into ATA_RESOURCE before execution
 *
 * Revision 1.3  2007/07/20 12:12:06  cs
 * changed:
 *   - use sysChameleonV2*() API (sysChameleon*() API obsolete)
 *
 * Revision 1.2  2005/12/01 16:20:27  cschuster
 * cloned from em3fam/sysCommonIde.c
 * changed sys16Z053Init interface
 *     controller number (ataIdx) now controlled from sysAta.c
 * fixed intLevel assignment
 *
 * Revision 1.1  2005/11/22 17:57:17  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/* defines */
#define CHAMELEONV2_16Z053_IDE		 53

#define ATA_REG_OFFS_0    0x1f0
#define ATA_REG_OFFS_1    0x3f0

/* globals */

/* forward declarations */

/******************************************************************************
*
* sys16z053ideInit - find 16z016ide and fill ataResources table
*
* Usually called by SysHwInit2(). Must be called after sysChameleonInit()!
*
* RETURNS: OK, or ERROR
*
*/

int mysys16Z053Init = 0;
STATUS sys16Z053Init(
	unsigned int ataIdx,
	unsigned int sec
)
{
	CHAMELEONV2_UNIT unit;
	CHAMELEONV2_FIND chamFind;
	int irq=0;
	ATA_RESOURCE *r = &ataResources[ataIdx];

	if(ataIdx >= (ATA_MAX_CTRLS)){
		logMsg("sys16Z053Init: requested ATA_RESOURCE out of range (%d) ATA_MAX_CTRLS=%d\n", ataIdx, (ATA_MAX_CTRLS),3,4,5,6 );
		mysys16Z053Init =1;
		return ERROR;
	}

	chamFind.variant  = -1;
	chamFind.instance =  0;
	chamFind.busId    = -1;
	chamFind.group    = -1;
	chamFind.bootAddr = -1;
	chamFind.devId    = CHAMELEONV2_16Z053_IDE;

	if( sysChameleonV2UnitFind( chamFind, 0, &unit )
		!= CHAMELEON_OK )
		{
			mysys16Z053Init = 2;
			return ERROR;
		}

	irq = sysChameleonIntNumForUnitIrq( unit.interrupt );
	SYS_ATA_LOG("sys16Z053Init: CHAMELEON_16Z053_IDE found, irq=%d\n", irq, 2,3,4,5,6 );

	/* initialisation for menata2.c:ataDrv() */
	r->resource.ioStart[0] =((u_int32)unit.addr)+ATA_REG_OFFS_0;
	r->resource.ioStart[1] =((u_int32)unit.addr)+ATA_REG_OFFS_1;

	r->resource.ioStop[0]  = (int)&ATA2_chipsetCommonIde;
	r->resource.ioStop[1]  = 0;	/* chipset arg */

	r->semTimeout          = ATA_TOUT_DEFAULT;
	r->wdgTimeout          = ATA_TOUT_DEFAULT;

	r->drives 		       = 1; /*just one "drive" on NAND!*/
	r->intLevel  	       = irq;
	r->configType  	       = ATA_PIO_AUTO;

	mysys16Z053Init = 3;
	return OK;
}

