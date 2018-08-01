/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sys16z023ide.c
 *      Project: VxWorks for MEN EM3 family boards
 *
 *       Author: kp/ts
 *        $Date: 2010/01/22 09:14:31 $
 *    $Revision: 1.11 $
 *
 *  Description: Support routines for 16Z023_IDE controller in FPGA
 *				 This file should be included in sysLib.c
 *				 Uses MENs own menata2.c / ata2.c (replacement for WRS driver)
 *               taken from sysAta.c and adapted for menata2 usage
 *
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sys16z023ide.c,v $
 * Revision 1.11  2010/01/22 09:14:31  cs
 * R: Init could cause system corruption when called with
 *    index into ATA_RESOURCE bigger than ATA_MAX_CTRLS
 * M: check index into ATA_RESOURCE before execution
 *
 * Revision 1.10  2007/07/20 12:12:04  cs
 * changed:
 *   - use sysChameleonV2*() API (sysChameleon*() API obsolete)
 *
 * Revision 1.9  2007/04/19 09:30:31  cs
 * Reason:
 *    hot plug: power was enabled at driver init before CF was inserted
 * Measure:
 *    only enable power when CD pin is active
 *
 * Revision 1.8  2007/01/05 17:01:29  cs
 * fixed:
 *   - chipsetArg structure has to be allocated
 *
 * Revision 1.7  2006/12/22 15:08:11  cs
 * moved special interrupt handling to chipset driver (intPreProc, intPostProc)
 *
 * Revision 1.6  2006/08/18 14:11:45  RLange
 * Bugfix to access CTRL Register
 *
 * Revision 1.5  2006/03/20 16:09:31  cs
 * cosmetics in sys16Z023Irq header
 *
 * Revision 1.4  2006/03/16 18:24:49  cs
 * changed handling of Z023_IDE controller
 *     - don't go to secondary IDE if not asked to do so
 *     - call sysChameleonIntNumForUnitIrq() only for EM1
 *
 * Revision 1.3  2006/02/02 17:28:29  cs
 * added basic support for 16Z023_IDE controller (no hot swap functionality yet)
 *
 * Revision 1.2  2005/12/01 16:18:47  cschuster
 * cloned from em3fam/sys16z023ide.c
 * moved globals (sysAtaBase, ataResources) to sysAta.c
 * changed sys16Z023Init interface
 *     controller number (ataIdx) now directed from sysAta.c
 * changed sys16Z023Irq interface to take ataIdx as parameter
 *
 * Revision 1.1  2005/11/22 17:59:53  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifdef INCLUDE_ATA

#define CHAMELEONV2_16Z023_IDE     23
#define CHAMELEONV2_16Z023_IDENHS 123

/* includes */
#include <MEN/16z023_ide.h>
#include <logLib.h>

/******************************************************************************
*
* sys16Z023Init - find Ata and fill ataResources table
*
* Usually called by SysHwInit2(). Must be called after sysChameleonInit()!
*
* RETURNS: OK, or ERROR
*
*/
STATUS sys16Z023Init(
	unsigned int ataIdx,
	unsigned int sec
#ifdef MEN_Z023_USE_PCI_IRQ
	, unsigned int pciIrq
#endif
#ifdef MEN_Z023_USE_INSTANCE
	, unsigned int instance
#endif
)
{
	CHAMELEONV2_UNIT unit;
	CHAMELEONV2_FIND chamFind;
	ATA2_CHIPSET_ARG_T *chipsetArg;
	u_int32 gotSize = 0;
	int irq;
	u_int8 ctrl_reg;
	u_int8 devFound = 0;

	chipsetArg = (ATA2_CHIPSET_ARG_T *)OSS_MemGet( OSS_VXWORKS_OS_HDL, sizeof(ATA2_CHIPSET_ARG_T), &gotSize );
	if( chipsetArg == NULL )
		return ERROR;

	OSS_MemFill( OSS_VXWORKS_OS_HDL, sizeof(ATA2_CHIPSET_ARG_T), (char*)chipsetArg, 0 );
	chipsetArg->hGotSize = gotSize;

	chamFind.variant  = -1;
	chamFind.instance =  instance;
	chamFind.busId    = -1;
	chamFind.group    = -1;
	chamFind.bootAddr = -1;

	if(ataIdx >= (ATA_MAX_CTRLS)){
		logMsg("sys16Z023Init: requested ATA_RESOURCE out of range (%d) ATA_MAX_CTRLS=%d\n",
					ataIdx, (ATA_MAX_CTRLS),3,4,5,6 );
		return ERROR;
	}

	/*------------------------+
	|  16Z023_IDE controller  |
	+------------------------*/
	/* TODO: make sysChameleonV2UnitFind working for n Chameleon tables! */
	chamFind.devId  = CHAMELEONV2_16Z023_IDENHS;
	if( sysChameleonV2UnitFind( chamFind, 0, &unit ) == CHAMELEON_OK )
	{
		devFound = 1;
        SYS_ATA_LOG( "found CHAMELEON_16Z023_IDENHS (addr:0x%08x;irq:%d)\n",
        			 (int)unit.addr,unit.interrupt,3,4,5,6);
	} else {
		chamFind.devId    = CHAMELEONV2_16Z023_IDE; /* CHAMELEON_16Z023_IDE */
		if( sysChameleonV2UnitFind( chamFind, 0, &unit )
			== CHAMELEON_OK )
		{
 			devFound = 1;
			SYS_ATA_LOG( "found CHAMELEON_16Z023_IDE (addr:0x%08x;irq:%d;)\n",
        			 (int)unit.addr,unit.interrupt,3,4,5,6);
        }
	}

	if ( !devFound ){
		SYS_ATA_LOG( "*** no CHAMELEON_16Z023_IDE(NHS) controller found\n",
        			 1,2,3,4,5,6);
		return ERROR;
	}

	if( unit.devId == CHAMELEONV2_16Z023_IDE ) /* hot swappable */
		chipsetArg->isRemovable = TRUE;
	else
		chipsetArg->isRemovable = FALSE;

	/* check if IDE is used */
	ctrl_reg =
	  MREAD_D8( (MACCESS)unit.addr, (sec*Z023_IDE_BLOCK) + Z023_IDE_CONTROL );

	if( ctrl_reg & Z023_IDE_CTRL_NC )
	{
        SYS_ATA_LOG( "sys16Z023Init: AtaIdx %d, IDE %d not used!\n",
        			 ataIdx, sec, 3,4,5,6);
		return ERROR;
	}
#ifdef MEN_Z023_USE_PCI_IRQ
	irq = pciIrq;
#else
	irq = unit.interrupt;
#endif

	SYS_ATA_LOG( "sys16Z023Init: ataIdx=%d irq=%d, sec=%d\n", ataIdx, irq, sec, 4,5,6);

	/* register the 16Z023_IDE(NHS) controller */
	sysAtaBase[ataIdx]=(MACCESS)((u_int32)unit.addr+(sec * Z023_IDE_BLOCK));

	if(!(MREAD_D8( sysAtaBase[ataIdx],Z023_IDE_CONTROL)&Z023_IDE_CTRL_NC))
	{
		/* port connected in FPGA */
		ATA_RESOURCE *r = &ataResources[ataIdx];

		SYS_ATA_LOG("sys16Z023Init: 0x%p irq %d\n", (u_int32)sysAtaBase[ataIdx], irq, 3,4,5,6);

		r->resource.ioStart[0] = 	(int)sysAtaBase[ataIdx];
		r->resource.ioStart[1] = 	0; /*taken care of by ata2_chipset_16z023_ide*/

		/* ts: necessary for usage with ATA2! */
		r->resource.ioStop[0]  = 	(int)&ATA2_chipset16Z023Ide;
		r->resource.ioStop[1]  = 	(int)chipsetArg;	/* chipset arg */

		r->semTimeout = ATA_TOUT_DEFAULT;
		r->wdgTimeout = ATA_TOUT_DEFAULT;

		r->resource.memStart   = 0; /* ioStart are full addresses 		*/
		r->ctrlType			   = 0; /* not PC compliant pri/sec IDE 	*/
		r->drives 			   = 2; /* Master/Slave	*/
		r->intLevel  		   = irq;
		r->configType  		   = ATA_PIO_AUTO;
	}

	/* enable power if CD_STATUS is active, interrupts still disabled */
	/* disable power if CD_STATUS is inactive (Hot Plug) */
	if( sysAtaBase[ataIdx] ) {
		if( !(ctrl_reg & Z023_IDE_CTRL_CD) ) {
			MWRITE_D8( sysAtaBase[ataIdx], Z023_IDE_CONTROL, Z023_IDE_CTRL_POW );
		} else {
			MWRITE_D8( sysAtaBase[ataIdx], Z023_IDE_CONTROL, 0x00 );
		}
	}

	return OK;
}

#endif /* INCLUDE_ATA */

