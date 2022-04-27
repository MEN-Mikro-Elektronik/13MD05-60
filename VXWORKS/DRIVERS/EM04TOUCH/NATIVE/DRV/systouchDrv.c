/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systouchDrv.c
 *
 *      \author  rla
 *        $Date: 2012/09/22 12:57:17 $
 *    $Revision: 1.16 $
 *
 *        \brief  device driver for touch panel
 *
 *
 *     Switches: SYSTOUCH_IRQ_FROM_PCI
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systouchDrv.c,v $
 * Revision 1.16  2012/09/22 12:57:17  ts
 * R: diab compiler warned about adding an int to a void*
 * M: added cast
 *
 * Revision 1.15  2012/02/27 19:06:39  ts
 * R: new calib parameters added for a customerspecific 4point calibration
 * M: compare maximum parameter number to new define MAXPARAM
 *
 * Revision 1.14  2011/10/10 17:22:00  ts
 * R: touchIrq crash because intConnect instead pciIntConnect used on EM10A
 * M: use also INCLUDE_PCI_BUS, INCLUDE_PCI define changed in vxWorks6.8
 *
 * Revision 1.13  2011/09/10 10:40:00  ts
 * R: new Variant of 16Z044 with 1280x800 pixel resolution introduced
 * M: added handling of this bit masks value in driver
 *
 * Revision 1.12  2007/08/31 18:35:46  cs
 * changed:
 *   - use sysChameleonV2*() API
 * fixed:
 *   - error handling when sysTouchDevInit() failed
 *     reason: did not return with error when e.g. Z31_SPI unit not present
 *
 * Revision 1.11  2007/04/18 15:38:19  RLange
 * added routine to get touch handle
 *
 * Revision 1.10  2007/03/03 02:02:28  cs
 * added: ioctl TOUCH_CALIBRATE
 *
 * Revision 1.9  2006/09/09 17:29:12  cs
 * fixed: only use pciIntConnect() when SYSTOUCH_IRQ_FROM_PCI is set
 *        define DBG_MYLEVEL here
 *
 * Revision 1.8  2006/03/22 15:07:39  cs
 * added:
 *     + IOCTL calls for reading temperature and AUX input (VBATT)
 *
 * Revision 1.7  2005/12/19 21:13:11  cs
 * fixed DBG print in  sysTouchDevCreate (just cosmetics)
 *
 * Revision 1.6  2005/12/01 19:10:21  cschuster
 * fixed to use correct macro for Z044_DISP control register
 *
 * Revision 1.5  2005/07/19 15:06:04  CSchuster
 * changed Chameleon names to new style
 * added 16Z044_DISP module to supported graphic modules
 * optionally use IRQ from PCI instead from Chameleon table
 *
 * Revision 1.4  2005/04/12 16:56:15  kp
 * cosmetic
 *
 * Revision 1.3  2005/03/03 18:41:13  CSchuster
 * updated to use the newly rewritten EM04 touch driver and sysparam2
 *
 * Revision 1.2  2004/09/01 12:16:31  ufranke
 * cosmetics
 *
 * Revision 1.1  2003/04/10 17:21:31  Rlange
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char IdentString[]="touch - device-driver: $Id: systouchDrv.c,v 1.16 2012/09/22 12:57:17 ts Exp $";

/*--------------------------------------------------------------------------+
 |	INCLUDES											              		|
 +--------------------------------------------------------------------------*/
#include "systouchDrv.h"

/*--------------------------------------------------------------------------+
 |	DEFINES														   		    |
 +--------------------------------------------------------------------------*/
#undef DBH
#define DBH				G_dbh
#define DBG_MYLEVEL				0xC0008003

#define CHAMELEONV2_16Z031_SPI		31
#define CHAMELEONV2_16Z032_DISP		32
#define CHAMELEONV2_16Z033_DISP		33
#define CHAMELEONV2_16Z044_DISP		44
/*--------------------------------------------------------------------------+
 |	GLOBALS														   		    |
 +--------------------------------------------------------------------------*/

#if _WRS_VXWORKS_MAJOR == 7
IMPORT STATUS intConnectGirq (UINT32, VOIDFUNCPTR , void *);
IMPORT STATUS intEnableGirq  (UINT32);
IMPORT STATUS intDisableGirq (UINT32);
#endif

LOCAL TOUCH_DEV G_touchDev;
LOCAL int G_touchDrvNum = 0;		/* driver number of this driver */
LOCAL int G_irq;

LOCAL int  sysTouchWrite(TOUCH_DEV *ptouchDev, u_char *pbuf, int length);
LOCAL int  sysTouchRead(TOUCH_DEV *ptouchDev, u_char *pbuf, int length);
LOCAL void sysTouchInt (TOUCH_DEV *ptouchDev);
LOCAL int  sysTouchDevInit(void);
LOCAL int  sysTouchIoctl(TOUCH_DEV *ptouchDev, int funcCode, int arg);
LOCAL int  sysTouchOpen(TOUCH_DEV *ptouchDev, char *name, int mode);
LOCAL int  sysTouchClose(TOUCH_DEV *ptouchDev);
LOCAL void touchevent(EM04TOUCH_EVENT *tevth, void *dlp );
EM04_TOUCH_HANDLE* sysTouchGetTouchDev(void);

/**********************************************************************/
/** Create a device for touch
 *
 *  \param name		name to use for this device
 *
 *  \return 0=ok, or negative error number
 */
STATUS sysTouchDevCreate
(
	char *name	/* name to use for this device */
)
{
	TOUCH_DEV *ptouchDev = &G_touchDev;

	DBGINIT((NULL, &DBH));
	DBGWRT_1((DBH,"sysTouchDevCreate: enter\n"));

	G_ti.dbh = DBH;
	G_ti.debugLevel = DBG_MYLEVEL;

    /* if there already is a device on this channel, don't do it */
	if (ptouchDev->created)
		return (OK);

    /* check if driver already installed */
    if (G_touchDrvNum != 0)
		return (ERROR);

	selWakeupListInit(&G_touchDev.selWakeupList);

    /* install driver, i.e. make an entry in the driver table */
    G_touchDrvNum = iosDrvInstall (	sysTouchOpen,
    								(FUNCPTR)NULL,
    								sysTouchOpen,
									sysTouchClose,
									sysTouchRead,
									sysTouchWrite,
									sysTouchIoctl );

	if( G_touchDrvNum == ERROR ){
		errno = S_ioLib_NO_DRIVER;
		return (ERROR);
	}

    /* mark the device as created, and add the device to the I/O system */
    ptouchDev->created = TRUE;

	/* create device, fill device-descriptor structure */
    if( iosDevAdd (&ptouchDev->devHdr, name, G_touchDrvNum) == ERROR ){
    	G_touchDrvNum = ERROR;
    	return ERROR;
    }

    /* mark the device as created, and add the device to the I/O system */
    ptouchDev->created = TRUE;

	/* this routine configures the touch hardware settings */
	if( sysTouchDevInit() != OK )
		sysTouchRemove();

	if( G_touchDrvNum == ERROR || G_touchDrvNum == 0 ) {
		return( ERROR );
	} else
		return( G_touchDrvNum );
}


/**********************************************************************/
/** Remove touch driver and delete device
 *
 *  \return 0=ok or negative error code
 */
int sysTouchRemove(void)
{
	STATUS status;
	DBGWRT_1((DBH,"sysTouchRemove\n"));

	/* delete device */
	iosDevDelete(&G_touchDev.devHdr);

	/* remove driver and delete driver table entry */
	status = iosDrvRemove(G_touchDrvNum, TRUE);

	/* uninstall interrupts */
#if _WRS_VXWORKS_MAJOR == 7
	(void) intConnectGirq(G_irq, (VOIDFUNCPTR) NULL, &G_touchDev);
	intDisableGirq(G_irq);
#else
	(void) intConnect(INUM_TO_IVEC(G_irq), (VOIDFUNCPTR) NULL, (int)&G_touchDev);
	intDisable(G_irq);
#endif

	/* mark the driver as not created */
	G_touchDev.created = FALSE;

	/* set driver number to 0 */
	G_touchDrvNum = 0;

	return status;
}


/**********************************************************************/
/** Open touch device
 *
 *  \param ptouchDev	pointer of device data structure
 *  \param name			pointer to device name
 *  \param mode			open mode options
 *  \return ptouchDev	pointer of device data structure
 */
 LOCAL int sysTouchOpen
(
	TOUCH_DEV *ptouchDev, 	/* pointer of device data structure */
	char *name, 				/* pointer of device name */
	int mode					/* open mode (read-only, write-only... */
)
{
	return ( (int)ptouchDev );
}


/**********************************************************************/
/** Close touch device.
 *  \param ptouchDev	pointer of device data structure
 *
 *  \return 0
 */
LOCAL int sysTouchClose
(
	TOUCH_DEV *ptouchDev
)
{
	return ( (int)ptouchDev );
}


/**********************************************************************/
/** Initialize touch device structure.
 *
 */
LOCAL int sysTouchDevInit(void)
{
	CHAMELEONV2_UNIT unit;
	CHAMELEONV2_FIND chamFind;
	int error = 0;
	int instance = 0;

	DBGWRT_1((DBH,"sysTouchDevInit: enter\n"));

	chamFind.variant  = -1;
	chamFind.instance =  instance;
	chamFind.busId    = -1;
	chamFind.flags    = -1;
	chamFind.group    = -1;
	chamFind.bootAddr = -1;

	/* find chameleon unit CHAMELEON_16Z031_SPI */
	chamFind.devId    = CHAMELEONV2_16Z031_SPI;
	if ( (error = sysChameleonV2UnitFind(chamFind,0,&unit)) != OK ){
		DBGWRT_ERR((DBH,"sysTouchDevInit: 16Z031_SPI chameleon unit not found\n"));
		return ERROR;
	}

	DBGWRT_1((DBH,"sysTouchDevInit: bar=0x%x offset=0x%x irq=0x%x\n",unit.bar,
		unit.offset,unit.interrupt));

	G_ti.base = (MACCESS)unit.addr;

#ifdef SYSTOUCH_IRQ_FROM_PCI
	/* get PCI interrupt */
	G_irq = sysChameleonIntNumForUnitIrq( unit.interrupt );
#else
	G_irq = unit.interrupt;
#endif

    /* find CHAMELEON_16Z033_DISP module in FPGA */
	chamFind.devId    = CHAMELEONV2_16Z033_DISP;
	if ( (error = sysChameleonV2UnitFind( chamFind, 0, &unit )) == OK ){
		DBGWRT_2((DBH,"sysTouchDevInit: playing with 16Z033_DISP\n"));
		G_ti.ds_xres = 640;
		G_ti.ds_yres = 480;
		goto SYSTOUCH_DISP_FOUND;
	}

	/* if not found try to find CHAMELEON_16Z032_DISP module in FPGA */
	chamFind.devId    = CHAMELEONV2_16Z032_DISP;
	if ( (error = sysChameleonV2UnitFind( chamFind, 0, &unit )) == OK ){
		DBGWRT_2((DBH,"sysTouchDevInit: playing with 16Z032_DISP\n"));
		G_ti.ds_xres = 800;
		G_ti.ds_yres = 600;
		goto SYSTOUCH_DISP_FOUND;
	}

	/* if not found try to find CHAMELEON_16Z044_DISP module in FPGA */
	chamFind.devId    = CHAMELEONV2_16Z044_DISP;
	if ( (error = sysChameleonV2UnitFind(chamFind, 0, &unit)) == OK ){
		unsigned long ctrlReg = 0;

		DBGWRT_2((DBH,"sysTouchDevInit: playing with 16Z044_DISP\n"));
		ctrlReg = OSS_Swap32(*(volatile unsigned long*)( (unsigned long)unit.addr + Z044_DISP_CTRL));

		if( (ctrlReg & Z044_DISP_CTRL_RES_MASK) == Z044_DISP_CTRL_RES_640X480 ){
			G_ti.ds_xres = 640;
			G_ti.ds_yres = 480;
		} else if( (ctrlReg & Z044_DISP_CTRL_RES_MASK) == Z044_DISP_CTRL_RES_800X600 ){
			G_ti.ds_xres = 800;
			G_ti.ds_yres = 600;
		} else if( (ctrlReg & Z044_DISP_CTRL_RES_MASK) == Z044_DISP_CTRL_RES_1024X768 ){
			G_ti.ds_xres = 1024;
			G_ti.ds_yres = 768;
		} else if( (ctrlReg & Z044_DISP_CTRL_RES_MASK) == Z044_DISP_CTRL_RES_1280X1024 ){
			G_ti.ds_xres = 1280;
			G_ti.ds_yres = 1024;
		} else if( (ctrlReg & Z044_DISP_CTRL_RES_MASK) == Z044_DISP_CTRL_RES_1280X800 ){
			G_ti.ds_xres = 1280;
			G_ti.ds_yres = 800;
		} else {
			DBGWRT_ERR((DBH,"unable to read resolution of MEN 16Z044_DISP FPGA module\n"));
			return ERROR;
		}
		DBGWRT_2((DBH,"Resolution of 16Z044_DISP = %dX%d\n", G_ti.ds_xres, G_ti.ds_yres ));
		goto SYSTOUCH_DISP_FOUND;


	} else {
		/* set resolution of touch, get from header */
		G_ti.ds_xres = TOUCH_DEFAULT_XRES;
		G_ti.ds_yres = TOUCH_DEFAULT_YRES;
		goto SYSTOUCH_DISP_FOUND;
	}

SYSTOUCH_DISP_FOUND:
	/* set touchevent function pointer */
	G_ti.data = (void*)&G_touchDev.devHdr;
	G_ti.tefunctp = touchevent;

	/* set pixel threshold parameters */
	G_ti.ds_thresh = TOUCH_TRESHOLD_DEFAULT;
	G_ti.ds_flags = TOUCH_MOVE_DEFAULT;

	/* init touch controller and handle */
	if( (error = EM04TOUCH_Init(&G_ti, &G_th)) != OK )
		return ERROR;
	DBGWRT_2((DBH,"sysTouchDevInit: connecting G_Irq = %d\n", G_irq));

	/* connect function sysTouchInt with interrupt */
#if _WRS_VXWORKS_MAJOR == 7
#ifdef SYSTOUCH_IRQ_FROM_PCI
#error SYSTOUCH_IRQ_FROM_PCI not supported for VxWorks 7
#else
	intConnectGirq(G_irq, (VOIDFUNCPTR) sysTouchInt, &G_touchDev);
#endif /* SYSTOUCH_IRQ_FROM_PCI */

#elif ( defined( INCLUDE_PCI ) || defined( INCLUDE_PCI_BUS ))  && defined( SYSTOUCH_IRQ_FROM_PCI )
	pciIntConnect (INUM_TO_IVEC(G_irq), sysTouchInt, (int)&G_touchDev);
#else
	intConnect(INUM_TO_IVEC(G_irq), (VOIDFUNCPTR) sysTouchInt, (int)&G_touchDev);
#endif /* INCLUDE_PCI */

	/* enable interrupt */
#if _WRS_VXWORKS_MAJOR == 7
	intEnableGirq(G_irq);
#else
	intEnable(G_irq);
#endif

	return OK;
}

EM04_TOUCH_HANDLE* sysTouchGetTouchDev(void)
{
	return G_touchDev.touch;
}

/**********************************************************************/
/** Routine to write to touch panel device.
 *  \param ptouchDev		pointer of device data structure
 *  \param pbuf				pointer of data structure
 *  \param usrlength		number of bytes that will be sent
 *
 *  \return number of data or error code
 */
LOCAL int sysTouchWrite
(
	TOUCH_DEV *ptouchDev,
	u_char *pbuf,
	int usrlength
)
{
	return ERROR;
}


/**********************************************************************/
/** Routine to read from touch device
 *  \param ptouchDev 	pointer of device data structure
 *  \param pbuf 		pointer to read data
 *  \param usrlength	number of requested data
 *
 *  \return number of read data packets or error code
 */
 LOCAL int sysTouchRead
(
	TOUCH_DEV *ptouchDev,
	u_char *pbuf,
	int usrlength
)
{
	TOUCH_PACKET *ptouchPacket = (TOUCH_PACKET*)pbuf;

	/* check if data available */
	if( ptouchDev->dataAvailable ){
		ptouchPacket->x = ptouchDev->x;
		ptouchPacket->y = ptouchDev->y;
		ptouchPacket->penStatus = ptouchDev->event;
		ptouchPacket->sync = TOUCH_SYNC;
		ptouchDev->dataAvailable = FALSE;
		return (sizeof(TOUCH_PACKET));
	}

	return 0;
}


/**********************************************************************/
/** Interrupt level processing for touch device.
 *  \param ptouchDev	pointer of device data structure
 *
 */
LOCAL void sysTouchInt
(
	TOUCH_DEV *ptouchDev
)
{

	IDBGWRT_2((DBH,"sysTouchInt: calling EM04TOUCH_Irq thp= 0x%x\n",G_th));

	/* call touch specific interrupt service routine */
	EM04TOUCH_Irq(G_th);
}


/*****************************************************************************
*
* sysTouchIoctl - Set special options of driver
*
*
* RETURNS: OK | ERROR
*/
/**********************************************************************/
/** Set special options of touch device driver.
 *  \param ptouchDev	pointer of device data structure
 *  \param funcCode		function code
 *  \param arg			argument
 *
 *  \return 0=ok, or negative error number
 */
LOCAL int sysTouchIoctl
(
	TOUCH_DEV *ptouchDev,
	int funcCode,
	int arg
)
{
	int intState;

	switch (funcCode){
		case FIOSELECT:
		    if(selWakeupType((SEL_WAKEUP_NODE *) arg) == SELREAD){

				selNodeAdd(&G_touchDev.selWakeupList,
					(SEL_WAKEUP_NODE *)arg);

				if(G_touchDev.dataAvailable == TRUE){
				    /* data already available, no need to pend */
				    selWakeup((SEL_WAKEUP_NODE *) arg);
				}
			}
		    else
				return(ERROR);

			break;

		case FIOUNSELECT:
		    if(selWakeupType((SEL_WAKEUP_NODE *) arg) == SELREAD){
				selNodeDelete(&G_touchDev.selWakeupList,
					(SEL_WAKEUP_NODE *)arg);
			}
		    else
				return(ERROR);

			break;

		case TOUCH_TRESHOLD_SET:
			if( arg > TOUCH_TRESHOLD_MIN && arg < TOUCH_TRESHOLD_MAX ){
				intState = intCpuLock();
				G_ti.ds_thresh = arg;
				intCpuUnlock(intState);
			}
			else
				return ERROR;
			break;

		case TOUCH_CALIBRATE:
		{
			EM04TOUCH_ERRCODE err;
			u_int32 calibMode = *(u_int32*)arg;

			if( calibMode >= EM04TOUCH_CALIB_UL && calibMode <= EM04TOUCH_CALIB_MAXPARAM ){
				if( (err = EM04TOUCH_Calibrate( G_th, calibMode )) != EM04TOUCH_SUCCESS )
					return ERROR;
			} else
				return ERROR;
			break;
		}

		case TOUCH_GS_TEMP:
		{	int *dataP = (int *)arg;

			*dataP = EM04TOUCH_MeasureAuxInp( G_th, EM04TOUCH_DEV_TEMP );
			if( *dataP == EM04TOUCH_ERROR )
				return ERROR;
			break;
		}
		case TOUCH_GS_TEMP_RAW:
		{	int *dataP = (int *)arg;

			*dataP = EM04TOUCH_MeasureAuxInp( G_th,
											  (*dataP == 0 ?
											   EM04TOUCH_DEV_TEMP_RAW_LO :
											   EM04TOUCH_DEV_TEMP_RAW_HI) );
			if( *dataP == EM04TOUCH_ERROR )
				return ERROR;
			break;
		}
		case TOUCH_GS_VBAT:
		{	int *dataP = (int *)arg;

			*dataP = EM04TOUCH_MeasureAuxInp( G_th, EM04TOUCH_DEV_BATT );
			if( *dataP == EM04TOUCH_ERROR )
				return ERROR;
			break;
		}
		default:
			return (ERROR);
	}
	return OK;
}

/**********************************************************************/
/** This routine creates a IRQ Handle
 *
 *  \param osh		OSS handle
 *  \param irq		irq number of device
 *  \param irqP		pointer to IRQ handle
 *
 *  \return 0=ok, or negative error number
 */
STATUS sysTouchIrqInit(
	OSS_HANDLE *osh,
	int irq,
	OSS_IRQ_HANDLE **irqP
)
{
	int32 retcode = 0;

	DBGWRT_1((DBH,"sysTouchIrqInit: irq=%d\n",irq));
	osh = OSS_VXWORKS_OS_HDL;
	if( (retcode = OSS_IrqHdlCreate(irq, TOUCH_INTR_LEVEL, irqP)) != OK )
		return (retcode);

	return OK;
}

/**********************************************************************/
/** This routine destroyes the IRQ handle
 *
 *  \param osh		OSS handle
 *  \param irqP		pointer to IRQ handle
 *
 *  \return 0=ok, or negative error number
 */
STATUS sysTouchIrqExit(
	OSS_HANDLE *osh,
	OSS_IRQ_HANDLE **irqP
)
{
	int32 retcode = 0;

	DBGWRT_1((DBH,"sysTouchIrqExit: irqP=0x%x\n",irqP));
	osh = OSS_VXWORKS_OS_HDL;
	if( (retcode = OSS_IrqHdlRemove(irqP)) != OK )
		return (retcode);

	return OK;

}


/**********************************************************************/
/** touchevent routine.
 *  The routine transfers the measured touchevent data to VxWorks specific
 *  structure called form the OS interrupt service routine of touchdriver
 *  \param tevth		pointer to TOUCH_EVENT structure
 *  \param dlp			pointer to user specific data
 */

LOCAL void touchevent( EM04TOUCH_EVENT *tevth, void *dlp )
{
	u_int16 xpos, ypos;

	/* deliver new data, not until old data was read */
	if( !G_touchDev.dataAvailable ){
		G_touchDev.x = xpos = (tevth->highx << 8) + tevth->lowx;
		G_touchDev.y = ypos = (tevth->highy << 8) + tevth->lowy;
		if( tevth->event == TOUCH_PRESSED){
			G_touchDev.event = TOUCH_PEN_DOWN;
		}
		else{
			G_touchDev.event = TOUCH_PEN_UP;
		}

		G_touchDev.dataAvailable = TRUE;
	}

    selWakeupAll(&G_touchDev.selWakeupList, SELREAD);
}




