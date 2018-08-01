/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: menata2xbd.c
 *      Project: MEN ATA driver for VxWorks
 *
 *       Author: cs/kp
 *        $Date: 2012/08/09 21:38:26 $
 *    $Revision: 1.12 $
 *
 *  Description: This module is the VxWorks specific layer on the top of
 *               the common ATA2 (UDMA32) low level driver.
 *               It provides a VxWorks extended block device (XBD) interface.
 *
 *     Required: -
 *     Switches: INCLUDE_SHOW_ROUTINES to include ataShow
 *               MEN_ATA2_USE_VXBUS to use vxBus Functions, e.g. vxbPciIntConnect
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: menata2xbd.c,v $
 * Revision 1.12  2012/08/09 21:38:26  ts
 * R: API for xbdDevCreate changed from 6.8.3 on
 * M: added new parameter bufferedIO depending on version
 *
 * Revision 1.11  2012/05/14 10:43:34  ts
 * R: misleading "invalid configType Parameter" occures on EM01 BSP
 * M: logMsg commented out because message results from WR code trying to init
 *    all 4 possible ATA controllers even if they dont exist -> can be ignored
 *
 * Revision 1.10  2010/07/14 18:03:12  cs
 * R: for VxWorks 6.8 BSPs where not buildable because of missing symbols
 *    comps\src\usrAta.c needs ataCtrl structure.
 * M: dirty workaround: create needed ataCtrl structure and do minimal init
 *
 * Revision 1.9  2009/11/26 18:17:31  cs
 * R: with VxWorks 6.6 ++ name space conflicts are generated
 * M: prefix some functions MEN specific
 *
 * Revision 1.8  2008/09/09 11:19:13  cs
 * R: cosmetics
 * M: add define for initial ATA2 debug level
 *
 * Revision 1.7  2008/05/05 10:17:37  cs
 * ataShow(): added support for UDMA 6
 *
 * Revision 1.6  2008/04/02 14:53:07  cs
 * R: a) ataDevCreate(), ataRawio() and ataShow() had incomplete error detection
 *    b) ataXbdRawio() had unused code (too early return call)
 * M: a) fixed error detection in this functions
 *    b) remove additional/wrong return call
 *
 * Revision 1.5  2008/01/29 12:10:36  cs
 * fixed:
 *   - removed dead lock in menataXbdIoctl(XBD_SOFT_EJECT/XBD_HARD_EJECT)
 *
 * Revision 1.4  2007/10/30 11:20:18  cs
 * added:
 *   + optionally set priority of XBD ATA task by external define (default = 50)
 * fixed:
 *   - only generate 0 size XBD device for hot pluggable devices,
 *     fail on device generation for not hot pluggable devices.
 *
 * Revision 1.3  2007/03/03 02:01:49  cs
 * disabled MENATA2_DEBUG
 *
 * Revision 1.2  2007/01/12 17:08:09  cschuster
 * fixed:
 *   - reparse drive geometry after insertion event (geometry might differ)
 *   - in case of drive not present at boot time create 0 size XBD (don't abort)
 *
 * Revision 1.1  2007/01/05 17:00:35  cs
 * Initial Revision
 *
 *
 * cloned from menata2.c Revision 1.4
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#define  ATA_DEBUG
#include "vxWorks.h"
#include "blkIo.h"
#include "ioLib.h"
#include "iosLib.h"
#include "memLib.h"
#include "errno.h"
#include "errnoLib.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "taskLib.h"
#include "dosFsLib.h"
#include "rawFsLib.h"

#include <drv/xbd/xbd.h>           	/* XBD library header */
#include <drv/erf/erfLib.h>        	/* event frame work library header */
#include <vxbus/vxbPciBus.h>		/* vxbPciIntConnect & friends */
#define ATA_PARAM ___ATA_PARAM	/* ataioctl.h also defines ATA_PARAM... */
#include "drv/hdisk/ataDrv.h"
	#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR >= 8 ) )
		/* For vxWorks 6.8, provide ataCtrl structure with minimal initialization */
		ATA_CTRL  ataCtrl[ATA_MAX_CTRLS];   /* Number of controllers is hardware dependent */
	#endif /* VxW Version == 6.8 */
#undef ATA_PARAM

#include "MEN/men_typs.h"
#include "MEN/maccess.h"
#include "MEN/dbg.h"
#include "MEN/dbg_vxworks.h"
#include "MEN/oss.h"
#include "MEN/rbdioctl.h"
#include "MEN/rbdrv.h"
#if ( ( _WRS_VXWORKS_MAJOR != 6 ) || ( _WRS_VXWORKS_MINOR < 2 ) )
#include "MEN/ata_regs.h"
#endif
#include "MEN/ataioctl.h"
#include "MEN/ata2_drv.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define MENATA2_DEBUG
#define MENATA2_ATA_DBGLEVEL 0xc0008001

#define DBG_MYLEVEL	pCtrl->atah->debugLevel
#define DBH			pCtrl->atah->dbh

#ifndef MENATA2XBD_TASK_PRIORITY
	#define MENATA2XBD_TASK_PRIORITY 50
#endif /* MENATA2XBD_TASK_PRIORITY */
#define LOCK_ATA \
	{ while( OSS_SemWait(OSS_VXWORKS_OS_HDL, pCtrl->atah->lockSem, OSS_SEM_WAITFOREVER) != 0 );}
#define UNLOCK_ATA \
	{ OSS_SemSignal( OSS_VXWORKS_OS_HDL, pCtrl->atah->lockSem ); }

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

/* define ATA_XBD structure/typedef so the xbd functions can find the
 * pointer to the device structure ataDev.  This is a locally defined
 * structure that is passed to the xbd routines.  The first entity is
 * XBD structure - and this must always be the first entity.  Other external
 * routines will modify this part.  The rest of the structure can
 * be ata driver specific.
 */
typedef struct
	{
	XBD        xbd;           /* must be first, add new items below here */
	SEMAPHORE  xbdSemId;      /* used for xbdBlkDevCreateSync() */
	devname_t  name;          /* name of device            */
	device_t   device;        /* device number of this instance */
	UINT32     xbdInserted;   /* device is inserted */
	BOOL       xbdInstantiated;  /* TRUE if stack init is complete */
	struct menAtaDev *ataDev;       /* necessary to access the device structure */
	} MEN_ATA_XBD;

/* blk device structure. created by ataDevCreate() */
typedef struct menAtaDev
{
	BLK_DEV          vxBlkDev;             /* generic block device structure */
	int 			 blkDevInit;		   /* block device initialized   */
	MEN_ATA_XBD    	 ataXbd;               /* actual xbd for this device */
	ATA_HANDLE_T    *atah;          	   /* MEN common ATA device handle   */
	u_int32 		 flags;		           /* for real raw disk access       */
	int              ctrl;
	int				 drive;
	int              blkOffset;
} MEN_ATA_DEV;

/* per controller structure (filled by ataDrv()) */
typedef struct
{
	ATA_HANDLE_T   *atah;          		/* MEN common ATA device handle   		*/
	SEMAPHORE    	ataBioReadySem;  	/* bio queue counting semaphore 		*/
	struct bio   	*bioQueueh;       	/* bio queue head for master device   	*/
	struct bio   	*bioQueuet;       	/* bio queue tail for master device   	*/
	int          	svcTaskId;        	/* id of service task for this ctrl   	*/
	VXB_DEVICE_ID 	vxbPciId;			/* for id of service task for this ctrl   	*/
	MEN_ATA_DEV	   	*ataDevs[ATA_MAX_DRIVES]; /* device structures */
	BOOL installed;
}MEN_ATA_CTRL;


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static STATUS ataIoctl
(
	MEN_ATA_DEV	*ataRbd,
	int             code,
	int             arg
);

static STATUS ataBlkRd
(
	MEN_ATA_DEV	*ataRbd,
	int             startBlk,
	int             numBlks,
	char            *pBuffer
);

static STATUS ataBlkWrt
(
	MEN_ATA_DEV	*ataRbd,
	int          	startBlk,
	int          	numBlks,
	char         	*pBuffer
);

/* globals */
MEN_ATA_CTRL MENATA_ctrl[ATA_MAX_CTRLS]; /* per controller vars */

#ifdef OSS_VXBUS_SUPPORT
VXB_DEVICE_ID G_menAtaXbdVxbID;
#endif

/* forward declarations */

LOCAL STATUS menataStatus (MEN_ATA_DEV * pDev);
LOCAL int32 MapErrorCodes( int32 rbdrvErr );

LOCAL struct bio * menataGetNextBio (MEN_ATA_CTRL *ataXbdCtrl);
LOCAL void menataExecBio (MEN_ATA_CTRL *ataXbdCtrl, struct bio *bio, int drive);
LOCAL void menataXbdService (MEN_ATA_CTRL *ataXbdCtrl);
LOCAL void menataXbdDevCreateSyncHandler (UINT16, UINT16, void *, void *);
LOCAL int menataXbdMediaTest ( MEN_ATA_XBD * );
LOCAL int menataXbdTest ( MEN_ATA_XBD * );
LOCAL int menataXbdEject ( MEN_ATA_XBD *);

LOCAL int menataXbdIoctl    (struct xbd *, int, void *);
LOCAL int menataXbdStrategy (struct xbd *, struct bio *);
LOCAL int menataXbdDump     (struct xbd *, sector_t, void *, size_t);


/*
 * functions to be exported to XBD interface
 */
LOCAL struct xbd_funcs menataXbdFuncs =
	{
			menataXbdIoctl,
			menataXbdStrategy,
			menataXbdDump,
	};



/******************************************************************************
*
* ataDrv - initialize the ATA driver
*
* This routine initializes the ATA/IDE driver, sets up interrupt vectors,
* and performs hardware initialization of the ATA/IDE chip.
*
* This routine must be called exactly once for every controller (e.g.
* primary/secondary), before any reads, writes, or calls to
* ataDevCreate().  Normally, it is called by usrRoot() in usrConfig.c.
*
* Like WRS driver, ataDrv() uses some info from imported ataResources array:
*
* - resource.ioStart[0] = will be copied to ATA_DESC_T.maRegBlock[0]
* - resource.ioStart[1] = will be copied to ATA_DESC_T.maRegBlock[1]
* - resource.memStart 	= will be copied to ATA_DESC_T.maRegBlockDma
*
* - resource.ioStop[0]  = will be copied to ATA_DESC_T.chipset
* - resource.ioStop[1]  = will be copied to ATA_DESC_T.chipsetArg
*
* - drives: number of max. drives on that controller
*
* - intLevel
*
* - configType (see below)
*
* Differences to WRS ata driver:
*
* - Most fields of ataResources are ignored
*
* - semTimeout, wdgTimeout parameter ignored
*
* - configType parameter must be either ATA_DMA_AUTO or ATA_PIO_AUTO.
*   If controller supports DMA, ATA driver will attempt to use DMA if
*	configType is ATA_DMA_AUTO.
*
* - this driver does not need and use ataTypes array.
*
* External requirements:
*
* - OSS_Init must have been called if this driver is used with DMA
*
* RETURNS: OK, or ERROR if initialization fails.
*
* SEE ALSO: ataDevCreate()
*/
STATUS ataDrv(
	int  ctrl,			/* controller no. IDE_LOCAL or 3*/
	int  drives,		/* number of drives */
	int  vector,		/* interrupt vector */
	int  level,			/* interrupt level */
	BOOL configType,	/* configuration type */
	int  semTimeout,	/* timeout seconds for sync semaphore */
	int  wdgTimeout		/* timeout seconds for watch dog */
	)
{
	ATA_RESOURCE *pAta			= &ataResources[ctrl];
	PCCARD_RESOURCE *pResource	= &pAta->resource;
	MEN_ATA_CTRL *pCtrl 		= &MENATA_ctrl[ctrl];
	u_int32 drvMask;
	int32 error;
	ATA2_DESC_T desc;
	ATA2_HANDLE_T *h;

	if ( (ctrl >= ATA_MAX_CTRLS) || (drives > ATA_MAX_DRIVES) || (drives>2) ){
		logMsg("*** ataDrv: invalid ctrl/drives param: c=%d/d=%d!\n", ctrl, drives, 0,0,0,0 );
		return (ERROR);
	}

	/* does this instance exist at all ? WindRiver tries instantiating them! */
	if( (configType != ATA_DMA_AUTO) && (configType != ATA_PIO_AUTO) ){
		logMsg("ataDrv: skipping a nonexisting controller instance.\n",0,0,0,0,0,0);
		return ERROR;
	}

	if( (pAta->ctrlType < IDE_LOCAL) || (pAta->ctrlType > 3)){
		logMsg("*** ataDrv: illegal controller type %d\n", pAta->ctrlType,0,0,0,0,0);
		return ERROR;
	}

	if( pCtrl->installed ){
		logMsg("*** ataDrv: controller already initialized!\n",0,0,0,0,0,0);
		return OK;			/* already initialized */
	}

	/* zero entire structure first */
	bzero((char *)pCtrl, sizeof (MEN_ATA_CTRL));

	memset( &desc, 0, sizeof(desc));

	desc.maRegBlock[0] = (MACCESS)pResource->ioStart[0];
	desc.maRegBlock[1] = (MACCESS)pResource->ioStart[1];
	desc.maRegBlockDma = (MACCESS)pResource->memStart;
	desc.chipset 	   = (const ATA2_CHIPSET_T *)pResource->ioStop[0];
	desc.chipsetArg    = (void *)pResource->ioStop[1];

	if( desc.maRegBlock[0] == 0 ){
		logMsg( "*** ataDrv: maRegBlock[0]==0 - controller doesnt exist? \n",0,0,0,0,0,0);
		return ERROR;			/* this controller does not exist */
	}

	drvMask = (1<<drives)-1;	/* mask for supported drives */

	if( configType == ATA_DMA_AUTO )
		drvMask |= (ATA2_DRVMASK_MASTER_UDMA | ATA2_DRVMASK_SLAVE_UDMA);

	desc.drvMask = drvMask;
	desc.allowSlaveCf = TRUE;
	desc.useIrq = TRUE;
	desc.toutReset 	= 30;
	desc.retry 	= 5;

#ifdef MENATA2_DEBUG
	printf("menata2xbd.c::ataDrv(): ctrl=%d drives=%d\nbase=%08x level=0x%x\n",
		   ctrl, drives, desc.maRegBlock[0], level);
#endif

	/*--- init ATA driver ---*/
	error = ATA2_Create(OSS_VXWORKS_OS_HDL, MENATA2_ATA_DBGLEVEL, &desc, &h );
	if( error ){
		logMsg("*** ataDrv: ATA2_Create returned with error 0x%x\n", error,0,0,0,0,0 );
		goto CLEANUP;
	}

	pCtrl->atah = (ATA_HANDLE_T*)h;

#ifdef MENATA2_DEBUG
	{
		DBG_SetMemSize(0x100000);
		DBG_SetDoCacheFlush(TRUE);
		logMsg("DBG_MemP/_WriteP = %x %x\n", (int)DBG_GetMemP, (int)DBG_GetWriteP,0,0,0,0 );

	}
#endif

	if (semCInit (&pCtrl->ataBioReadySem, SEM_Q_PRIORITY, SEM_EMPTY) != OK)
	{
		DBGWRT_ERR((DBH, "ataDrv: cannot initialize Sync semaphore, returning\n"));
		return(ERROR);
	}

	/* ts: dont connect devices with own IRQ handler, when passed level =-1.
	 * addendum 10/2015: use vxbPciIntConnect should be used! */
	if (level>=0){
#ifdef 	OSS_VXBUS_SUPPORT
		vxbPciIntConnect(G_vxbPcie2Hdl, INUM_TO_IVEC(level), ATA2_Irq, (int)h );
#else
		intConnect( INUM_TO_IVEC(level), ATA2_Irq, (int)h );
#endif
		intEnable( level );
	}

	pCtrl->installed = TRUE;

	if( pCtrl->atah->desc.chipset->initLate )
		pCtrl->atah->desc.chipset->initLate( h );

	/*--- startup drives ---*/
	ATA2_StartupDrives( h );
	DBGWRT_1((DBH, "ataDrv: return OK: handle=%p\n", h));

#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR >= 8 ) )
	/* workaround for vxWorks 6.8!! provide ataCtrl structure with minimal initialization */
	if ( pCtrl->installed )
	{
		/* zero entire structure first */
		bzero((char *)&ataCtrl[ctrl], sizeof (ATA_CTRL));
		ataCtrl[ctrl].ctrl = ctrl;   /* save the controller number for quick access */
		ataCtrl[ctrl].installed = TRUE;
		ataCtrl[ctrl].drive[0].type = ATA_TYPE_ATA;
		ataCtrl[ctrl].drive[1].type = ATA_TYPE_ATA;
	}
#endif /* VxW Version == 6.8 */

	return (OK);

CLEANUP:
	DBGWRT_1((DBH, "ataDrv: exit with error\n"));
	return( ERROR );
}


/*****************************************************************************
*
* ataDevCreate - create a device for a ATA/IDE disk
*
* This routine creates a device for a specified ATA/IDE disk.
*
* <drive> is a drive number for the hard drive; it must be 0 or 1.
*
* The <nBlocks> parameter specifies the size of the device in blocks.
* If <nBlocks> is zero, the whole disk is used.
*
* The <blkOffset> parameter specifies an offset, in blocks, from the start
* of the device to be used when writing or reading the hard disk.  This
* offset is added to the block numbers passed by the file system during
* disk accesses.  (VxWorks file systems always use block numbers beginning
* at zero for the start of a device.)
*
*
* RETURNS:
* A pointer to a block device structure (BLK_DEV) or NULL if memory cannot
* be allocated for the device structure.
*
* SEE ALSO: dosFsMkfs(), dosFsDevInit(), rt11FsDevInit(), rt11FsMkfs(),
* rawFsDevInit()
*/
BLK_DEV *ataDevCreate (
	int ctrl,
	int drive,
#if ( ( _WRS_VXWORKS_MAJOR != 6 ) || ( _WRS_VXWORKS_MINOR < 2 ) )
	int nBlocks,
	int blkOffset
#else
	UINT32 nBlocks,
	UINT32 blkOffset
#endif
	)
{
	MEN_ATA_DEV *pDev=NULL;
	MEN_ATA_CTRL *pCtrl = &MENATA_ctrl[ctrl];
	BLK_DEV *pBlkdev;
	RBDRV_IOCTL_PARMS ioctl;
	RBDRV_HANDLE *rbh;
	int32 error;

	DBGWRT_1((DBH, "ataDevCreate:: ctrl=%d drive=%d nBlocks=%d blkOffset=%d\n",
	  ctrl, drive, nBlocks, blkOffset ));

	if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES))
		return (NULL);

	if( pCtrl->atah == NULL ){
		DBGWRT_ERR((DBH, "ataDevCreate:: controller not initialized\n" ));
		return NULL;			/* controller not initialized */
	}

	if ((pDev = (MEN_ATA_DEV *)malloc(sizeof (MEN_ATA_DEV))) == NULL)
		return (NULL);

	/* zero entire structure first */
	bzero((char *)pDev, sizeof (MEN_ATA_DEV));

	pBlkdev = &pDev->vxBlkDev;
	rbh = &pCtrl->atah->ent;
	pCtrl->ataDevs[drive] = pDev;

	pBlkdev->bd_removable	= TRUE;	/* why???  */
	pBlkdev->bd_retry		= 1;
	pBlkdev->bd_mode		= O_RDWR;
	pBlkdev->bd_readyChanged = TRUE;
	pBlkdev->bd_blkRd		= ataBlkRd;
	pBlkdev->bd_blkWrt		= ataBlkWrt;
	pBlkdev->bd_ioctl		= ataIoctl;
	pBlkdev->bd_reset		= NULL;
	pBlkdev->bd_statusChk	= menataStatus;

	pDev->ctrl			= ctrl;
	pDev->drive			= drive;
	pDev->blkOffset		= blkOffset;
	pDev->atah   		= pCtrl->atah;

	/* check status, eventually card was just inserted */
	if( (error = menataStatus( pDev )) != OK){
		DBGWRT_ERR((DBH, "ataDevCreate:: no working device present\n" ));
		goto ERREXIT;
	}

	/*--------------------------------------+
	|  Query devices capacity/sector size   |
	+--------------------------------------*/
	ioctl.subdev = drive;
	ioctl.flags  = RBDRV_RAW_ACCESS;

	if( (error = rbh->Ioctl(rbh, RBDRV_IOCTL_INFO, &ioctl)))
		goto ERREXIT;

	DBGWRT_1((DBH, "  logSectors=%ld ssize=%ld any=%ld chs=%ld/%ld/%ld\n",
		   ioctl.p.info.logSectors,
		   ioctl.p.info.sectSize, ioctl.p.info.sectSizeAny,
		   ioctl.p.info.cylinders, ioctl.p.info.heads,
		   ioctl.p.info.sectorsTrack ));

	pBlkdev->bd_nBlocks      = ioctl.p.info.logSectors;  /* number of blocks */
	pBlkdev->bd_bytesPerBlk  = ioctl.p.info.sectSize;    /* bytes per block  */
	pBlkdev->bd_blksPerTrack = ioctl.p.info.sectorsTrack;/* blocks per track */

	pBlkdev->bd_nHeads       = ioctl.p.info.heads;       /* heads */

	DBGWRT_3((DBH, "ataDevCreate %d/%d: returning to caller\n",
			 ctrl, drive));

	pDev->blkDevInit = 1;
	return (&pDev->vxBlkDev);

 ERREXIT:
	DBGWRT_3((DBH, "ataDevCreate %d/%d: returning to caller after error\n",
			 ctrl, drive));
	return (&pDev->vxBlkDev);
}

/*****************************************************************************
*
* ataRawio - do raw I/O access
*
* This routine is called to perform raw I/O access.
*
* <drive> is a drive number for the hard drive: it must be 0 or 1.
*
* The <pAtaRaw> is a pointer to the structure ATA_RAW which is defined in
* ataDrv.h.
*
* RETURNS:
* OK, or ERROR if the parameters are not valid.
*
*/
STATUS ataRawio(
	int ctrl,
	int drive,
	ATA_RAW *pAtaRaw
	)
{
	MEN_ATA_CTRL *pCtrl = &MENATA_ctrl[ctrl];
	RBDRV_IOCTL_PARMS ioctl;
	RBDRV_HANDLE *rbh;
	STATUS status = ERROR;
	int32 retCode;
	u_int32 startBlk;
	DBGWRT_4((DBH, "ataRawIo: ctrl=% drv=%d cyl=%d hd=%d sec=%d nsect=%d dir=%d\n",
		   ctrl, drive, pAtaRaw->cylinder, pAtaRaw->head, pAtaRaw->sector,
		   pAtaRaw->nSecs, pAtaRaw->direction ));


	if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES))
		return( status );

	if( pCtrl->atah == NULL )
		return( status );			/* controller not initialized */

	rbh = &pCtrl->atah->ent;

	/*--------------------------------------+
	|  Query devices capacity/sector size   |
	+--------------------------------------*/
	ioctl.subdev = drive;
	ioctl.flags  = RBDRV_RAW_ACCESS;

	if( (retCode = rbh->Ioctl(rbh, RBDRV_IOCTL_INFO, &ioctl)))
		return ERROR;

	startBlk = pAtaRaw->cylinder *
		(ioctl.p.info.sectorsTrack * ioctl.p.info.heads) +
		pAtaRaw->head * ioctl.p.info.sectorsTrack + pAtaRaw->sector - 1;

	DBGWRT_4((DBH, "ataRawIo: startBlk=%ld\n", startBlk ));


	if( pAtaRaw->direction==0 ){
#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR > 6 ) )
		retCode = rbh->ReadBlocks( rbh, drive,
											startBlk, pAtaRaw->nSecs,
											(u_int8 *)pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
#else  /* VxW Version > 6.7 */
		retCode = rbh->ReadBlocks( rbh, drive,
											startBlk, pAtaRaw->nSecs,
											pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
#endif /* VxW Version > 6.7 */
	}
	else {
#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR > 6 ) )
		retCode = rbh->WriteBlocks( rbh, drive,
											startBlk, pAtaRaw->nSecs,
											(u_int8 *)pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
#else  /* VxW Version > 6.7 */
		retCode = rbh->WriteBlocks( rbh, drive,
											startBlk, pAtaRaw->nSecs,
											pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
#endif /* VxW Version > 6.7 */
	}


	if( retCode )
	{
		status = ERROR;
		errnoSet( MapErrorCodes(retCode) );
	}
	else
	{
		status = OK;
	}

	return status;
}


/******************************************************************************
*
* ataIoctl - do device specific control function
*
* This routine is called when the file system cannot handle an ioctl()
* function.
*
* There are no ioctl code supported right now!
*
* RETURNS:  OK or ERROR.
*/

LOCAL STATUS ataIoctl(
	MEN_ATA_DEV *pDev,
	int function,
	int arg
	)
{
	(void) errnoSet (S_ioLib_UNKNOWN_REQUEST);
#if ( ( _WRS_VXWORKS_MAJOR != 6 ) || ( _WRS_VXWORKS_MINOR < 2 ) )
	return ERROR;
#else
	return ENOTSUP;
#endif
}

/***************************************************************************
*
* menAtaBlkRW - read or write sectors to a ATA/IDE disk.
*
* Read or write sectors to a ATA/IDE disk. <startBlk> is the start Block,
* <nBlks> is the number of blocks, <pBuf> is data buffer pointer and
* <direction>  is the direction either to read or write. It should be O_WRONLY
* for data write to drive or O_RDONLY for read data from drive.
*
* RETURNS: OK, ERROR if the command didn't succeed.
*/
ATA_LOCAL STATUS menAtaBlkRW
	(
	MEN_ATA_DEV *pDev,
	UINT32 startBlk,
	UINT32 nBlks,
	char *pBuf,
	int  direction
	)
	{
	RBDRV_HANDLE *rbh = &pDev->atah->ent;
	int     bytesPerBlk;    /* number of bytes per block */
	int     status = OK;    /* the return code */
	int32   retCode;

	bytesPerBlk = pDev->vxBlkDev.bd_bytesPerBlk;
	startBlk += pDev->blkOffset;


	if( direction == O_WRONLY ){
		/*------------------+
		| write sectors     |
		+------------------*/
#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR > 6 ) )
		retCode = (rbh->WriteBlocks)( rbh, pDev->drive,
											startBlk, nBlks, (u_int8 *)pBuf,
											bytesPerBlk, RBDRV_RAW_ACCESS );
#else  /* VxW Version > 6.7 */
		retCode = (rbh->WriteBlocks)( rbh, pDev->drive,
											startBlk, nBlks, pBuf,
											bytesPerBlk, RBDRV_RAW_ACCESS );
#endif /* VxW Version > 6.7 */
	} else {
		/*------------------+
		| read sectors      |
		+------------------*/
#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR > 6 ) )
		retCode = (rbh->ReadBlocks)( rbh, pDev->drive,
										   startBlk, nBlks, (u_int8 *)pBuf,
										   bytesPerBlk, RBDRV_RAW_ACCESS );
#else  /* VxW Version > 6.7 */
		retCode = (rbh->ReadBlocks)( rbh, pDev->drive,
										   startBlk, nBlks, pBuf,
										   bytesPerBlk, RBDRV_RAW_ACCESS );
#endif /* VxW Version > 6.7 */
	}
	if( retCode )
	{
		status = ERROR;
		errnoSet( MapErrorCodes(retCode) );
	}

	return status;
	} /* menAtaBlkRW */

/******************************************************************************
*
* ataBlkRd - read one or more blocks from a ATA/IDE disk
*
* This routine reads one or more blocks from the specified device,
* starting with the specified block number.
*
* If any block offset was specified during ataDevCreate(), it is added
* to <startBlk> before the transfer takes place.
*
* RETURNS: OK, ERROR if the read command didn't succeed.
*/
static STATUS ataBlkRd(
	MEN_ATA_DEV		*pDev,
	int             startBlk,
	int             numBlks,
	char            *pBuffer
	)
{
	return (menAtaBlkRW (pDev, startBlk, numBlks, pBuffer,O_RDONLY));
}/*ataBlkRd*/


/******************************************************************************
*
* ataBlkWrt - write one or more blocks to a ATA/IDE disk
*
* This routine writes one or more blocks to the specified device,
* starting with the specified block number.
*
* If any block offset was specified during ataDevCreate(), it is added
* to <startBlk> before the transfer takes place.
*
* RETURNS: OK, ERROR if the write command didn't succeed.
*/
static STATUS ataBlkWrt(
	MEN_ATA_DEV		*pDev,
	int             startBlk,
	int             numBlks,
	char            *pBuffer
	)
{
	return (menAtaBlkRW (pDev, startBlk, numBlks, pBuffer,O_WRONLY));
}/*ataBlkWrt*/

int mapAta2DevStatus(
	u_int32 menataStat
	)
{
	switch( menataStat ){
	case (ATA2_DRIVE_ST_OK):
		return( ATA_DEV_OK );
	case (ATA2_DRIVE_ST_NONE):
		return( ATA_DEV_NONE );
	case (ATA2_DRIVE_ST_DIAG_F):
		return( ATA_DEV_DIAG_F );
	case (ATA2_DRIVE_ST_PREAD_F):
		return( ATA_DEV_PREAD_F );
	case (ATA2_DRIVE_ST_MED_CH):
		return( ATA_DEV_MED_CH );
	default:
		return( ATA_DEV_INIT );
	}
}
/**************************************************************************
*
* menataStatus - check status of a ATA/IDE disk controller
*
* This routine check status of a ATA/IDE disk controller.
*
* RETURNS: OK, ERROR .
*/

LOCAL STATUS menataStatus (
	MEN_ATA_DEV * pDev
	)
{
	MEN_ATA_CTRL  * pCtrl = &MENATA_ctrl[pDev->ctrl];
	RBDRV_HANDLE  *rbh    = &pDev->atah->ent;
	RBDRV_IOCTL_PARMS ioctl;
	int32 retCode = ATA_DEV_MED_CH;

	DBGWRT_2((DBH, "menataStatus: %d/%d: readyChanged=%#d\n",
				   pDev->ctrl, pDev->drive, pDev->vxBlkDev.bd_readyChanged));

	/*--- geometry/capacity info */
	ioctl.subdev = pDev->drive;
	ioctl.flags  = RBDRV_RAW_ACCESS;

	if( !pCtrl->installed )
	{
		DBGWRT_ERR((DBH, "menataStatus %d/%d NOT INSTALLED\n",
						pDev->ctrl, pDev->drive));
		return(ERROR);
	}

	while( retCode == ATA_DEV_MED_CH ) {
		if( (retCode = rbh->Ioctl(rbh, ATA_IOCTL_GET_DRV_STAT, &ioctl)) ){
			DBGWRT_ERR((DBH, "menataStatus %d/%d ATA_IOCTL_GET_DRV_STAT failed\n",
						pDev->ctrl, pDev->drive));
			return ERROR;
		}

		retCode = mapAta2DevStatus( *(u_int32*)ioctl.p.data );

		if( retCode == ATA_DEV_MED_CH ){
			DBGWRT_2((DBH, "menataStatus %d/%d ATA_DEV_MED_CH restart drives\n",
					  pDev->ctrl, pDev->drive));
			pDev->vxBlkDev.bd_readyChanged = TRUE;
			ATA2_StartupDrives( (ATA2_HANDLE_T*)pCtrl->atah );
			continue; /* recheck status */
		}
		if( retCode != ATA_DEV_OK )
		{
			DBGWRT_1((DBH, "menataStatus %d/%d NOT OKAY state=%d\n",
					  pDev->ctrl, pDev->drive, *(u_int32*)ioctl.p.data));
			return(ERROR);
		}
	}
	return(OK);
} /* menataStatus */


/**************************** MapErrorCodes *********************************
 *
 *  Description: Map the lower level error codes to VxWorks or driver
 *               specific error codes.
 *
 *---------------------------------------------------------------------------
 *  Input......: llErrorCode
 *
 *  Output.....: return mapped error code
 *
 *  Globals....: -
 ****************************************************************************/
static int32 MapErrorCodes( int32 llErrorCode )
{
	switch( llErrorCode ){
	case RBDRV_NOT_SUPPORTED:   return S_ioLib_UNKNOWN_REQUEST;
	case RBDRV_READ_ERROR:      return S_ioLib_DEVICE_ERROR;
	case RBDRV_WRITE_ERROR:     return S_ioLib_DEVICE_ERROR;
	case RBDRV_NOT_READY:       return S_ioLib_DISK_NOT_PRESENT;
	case RBDRV_WRITE_PROTECTED: return S_ioLib_WRITE_PROTECTED;
	case RBDRV_HW_DAMAGE:       return S_ioLib_DEVICE_TIMEOUT;
	case RBDRV_OUT_OF_RESOURCE: return S_ioLib_UNKNOWN_REQUEST;
	case RBDRV_BAD_SUBDEV:      return EINVAL;
	case RBDRV_ILLEGAL_SECTOR:  return EINVAL;
	case RBDRV_BAD_PARAMETER:   return EINVAL;
	case RBDRV_VERIFY_ERROR:    return S_ioLib_DEVICE_ERROR;
	default:                    return EINVAL;
	}
}/*MapErrorCodes*/


#ifdef INCLUDE_SHOW_ROUTINES

static void ataPrintSpc( char *str, int len )
{
	if( str[0]==0 && str[1]==0 ) return;
	while( len-- )
		printf("%c", *str++);
}

/******************************************************************************
*
* ataShowInit - replace ataShow routine
*
*/
#if ( ( _WRS_VXWORKS_MAJOR != 6 ) || ( _WRS_VXWORKS_MINOR < 2 ) )
void ataShowInit (void)
{
}
#else
STATUS ataShowInit (void)
{
	return OK;
}
#endif

/******************************************************************************
*
* ataShow - show the ATA/IDE disk parameters
*
* This routine shows the ATA/IDE disk parameters.  Its first argument is a
* controller number, 0 or 1; the second argument is a drive number, 0 or 1.
*
* RETURNS: OK, or ERROR if the parameters are invalid.
*/

STATUS ataShow(
	int ctrl,
	int drive
	)
{
	MEN_ATA_CTRL *pCtrl = &MENATA_ctrl[ctrl];
	RBDRV_IOCTL_PARMS ioctl;
	RBDRV_HANDLE *rbh;
	ATA_IOCTL_PARMS *ata = (ATA_IOCTL_PARMS *)ioctl.p.data;
	int32 retCode;
	const int w=25;

	if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES)){
		printf("ctrl/drive out of range\n");
		return ERROR;
	}

	if( pCtrl->atah == NULL ){
		printf("Controller not initialized\n");
		return ERROR;
	}

	rbh = &pCtrl->atah->ent;

	/*--- geometry/capacity info */
	ioctl.subdev = drive;
	ioctl.flags  = RBDRV_RAW_ACCESS;

	if( (retCode = rbh->Ioctl(rbh, RBDRV_IOCTL_INFO, &ioctl))){
		printf("RBDRV_IOCTL_INFO failed\n");
		return ERROR;
	}

	printf( "\n----------------------- General info "
			"--------------------------\n");
	printf("Controller %d, drive %d\n", ctrl, drive );
	printf("%-*s%ld/%ld/%ld\n", w, "Geometry C/H/S:",
		   ioctl.p.info.cylinders, ioctl.p.info.heads,
		   ioctl.p.info.sectorsTrack);


	/*--- adressing/transfer mode ---*/
	if( (retCode = rbh->Ioctl(rbh, ATA_IOCTL_GET_LBA, &ioctl)))
		return ERROR;

	printf("%-*s", w, "Addressing mode:");
	switch( ioctl.p.data[0] ){
	case 0:
		printf("CHS");
		break;
	case 1:
		printf("LBA28");
		break;
	case 2:
		printf("LBA48");
		break;
	};
	printf("\n");

	if( (retCode = rbh->Ioctl(rbh, ATA_IOCTL_GET_XFER, &ioctl)))
		return ERROR;

	printf("%-*s", w, "Transfer mode:");
	switch( ioctl.p.data[0] ){
	case ATA_PIO_DEF_W: 	printf("ATA_PIO_DEF_W"); break;
	case ATA_PIO_DEF_WO: 	printf("ATA_PIO_DEF_WO"); break;
	case ATA_PIO_W_0:		printf("ATA_PIO_W_0"); break;
	case ATA_PIO_W_1:		printf("ATA_PIO_W_1"); break;
	case ATA_PIO_W_2:		printf("ATA_PIO_W_2"); break;
	case ATA_PIO_W_3:		printf("ATA_PIO_W_3"); break;
	case ATA_PIO_W_4:		printf("ATA_PIO_W_4"); break;
	case ATA_DMA_ULTRA_0:	printf("ATA_DMA_ULTRA_0"); break;
	case ATA_DMA_ULTRA_1:	printf("ATA_DMA_ULTRA_1"); break;
	case ATA_DMA_ULTRA_2:	printf("ATA_DMA_ULTRA_2"); break;
	case ATA_DMA_ULTRA_3:	printf("ATA_DMA_ULTRA_3"); break;
	case ATA_DMA_ULTRA_4:	printf("ATA_DMA_ULTRA_4"); break;
	case ATA_DMA_ULTRA_5:	printf("ATA_DMA_ULTRA_5"); break;
	case ATA_DMA_ULTRA_6:	printf("ATA_DMA_ULTRA_6"); break;
	default:				printf("unknown (0x%02x)", ioctl.p.data[0] );
	}

	printf("\n");

	/*--- ATA identify info ---*/
	{
		/* query drive info */
		typedef struct
		{
			int wordNo;
			char explain[40];
		}ATA_PARAM_DESC;
		int i;
		 const ATA_PARAM_DESC dumpwords[] =
		 {
			{  0, "general cfg   " },
			{  1, "cylinders     " },
			{  3, "heads         " },
			{  6, "sectors/track " },
			{ 47, "multi sect    " },
			{ 49, "capabilities  " },
			{ 50, "capabilities  " },
			{ 51, "PIO mode      " },
			{ 53, "valid         " },
			{ 54, "cur cylinders " },
			{ 55, "cur heads     " },
			{ 56, "cur sect/track" },
			{ 57, "capacity0     " },
			{ 58, "capacity1     " },
			{ 59, "cur multi sect" },
			{ 60, "lba capacity0 " },
			{ 61, "lba capacity1 " },
			{ 63, "multi DMA mode" },
			{ 64, "advanced PIO  " },
			{ 65, "min cycl DMA  " },
			{ 66, "man cycl DMA  " },
			{ 67, "min cycl!IORDY" },
			{ 68, "min cycl IORDY" },
			{ 69, "resv          " },
			{ 70, "resv          " },
			{ 88, "UDMA mode     " }
		 };
		 u_int16 *words;

		if( (retCode = rbh->Ioctl(rbh, ATA_IOCTL_INFO, &ioctl)))
			return ERROR;


		printf( "\n----------------------- ATA Ident block "
				"-----------------------\n");

		printf("%-*s ", w, "Model:" );
		ataPrintSpc( ata->info.model, sizeof(ata->info.model));
		printf("\n%-*s ", w, "Firmware Revision:" );
		ataPrintSpc( ata->info.rev, sizeof(ata->info.rev));
		printf("\n%-*s ", w, "Serial Number:" );
		ataPrintSpc( ata->info.serial, sizeof(ata->info.serial));

		words = (u_int16 *)&ata->info;

		for( i=0; i<(int)(sizeof(dumpwords)/sizeof(ATA_PARAM_DESC)); i++ )
		{
			if( !(i%2))
				printf("\n");
			printf("  W[%02d] %s: %04x\t", dumpwords[i].wordNo, dumpwords[i].explain, words[dumpwords[i].wordNo]);
		}
		printf("\n");

}

 return OK;
}
#endif /* INCLUDE_SHOW_ROUTINES */

/*--------------------------------------------------------------------------
 *
 * This section of the file contains routines for handling the XBD nature of
 * the ATA driver.  A service task will be created for each controller.
 *
 * The service task will process bio's from the strategy call, and initiate
 * the appropriate disk operation.
 */

/*******************************************************************************
*
* ataXbdDevCreate - create an XBD device for a ATA/IDE disk
*
* Use the existing code to create a standard block dev device, then create an
* XBD device associated with the BLKDEV.
* RETURNS: a device identifier upon success, or NULLDEV otherwise
*
* ERRNO:
*/

device_t ataXbdDevCreate (
	int ctrl,         /* ATA controller number, 0 is the primary controller */
	int drive,        /* ATA drive number, 0 is the master drive */
	UINT32 nBlocks,   /* number of blocks on device, 0 = use entire disc */
	UINT32 blkOffset, /* offset BLK_DEV nBlocks from the start of the drive */
	const char * name /* name of xbd device to create */
#if (_WRS_VXWORKS_MAJOR == 6 )  && ( _WRS_VXWORKS_MINOR == 8 )
	, BOOL directModeFlag
#endif
	)
{
	BLK_DEV       * blkDev;
	MEN_ATA_CTRL  * pCtrl  = &MENATA_ctrl[ctrl];
	MEN_ATA_DEV   * pDev;
	device_t    retVal;
	int         error = 0;
	char        s[20];      /* used for building task name */

	DBGWRT_1((DBH,"ataXbdDevCreate %d/%d\n", ctrl, drive));

	if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES) || !pCtrl->installed )
	{
		DBGWRT_ERR((DBH, "ataXbdDevCreate: ERROR- Device %d on Controller %d not installed\n",
					   drive, ctrl));
		printErr("ataXbdDevCreate: ERROR- Device %d on Controller %d not installed\n",
					   drive, ctrl);
		return(NULLDEV);
	}

	if ((pCtrl->svcTaskId == ERROR) || (pCtrl->svcTaskId == 0))
	{
		sprintf(s, "tMenAtaSvc%d", ctrl);    /* build taskname with controller number */
		pCtrl->svcTaskId = taskSpawn (s, MENATA2XBD_TASK_PRIORITY, 0,
									  4096, /* Stack size. */
									  (FUNCPTR)menataXbdService, (int) pCtrl,
									   0, 0, 0, 0, 0, 0, 0, 0, 0);

		if (pCtrl->svcTaskId == ERROR)
			return (NULLDEV);
	}

	if( (blkDev = ataDevCreate (ctrl, drive, nBlocks, blkOffset)) == (BLK_DEV*)NULL){
		DBGWRT_ERR((DBH,"ataXbdDevCreate ERROR: %x  Device %d on Controller %d no BLK_DEV created\n",
				   errno, drive, ctrl ));
		printErr ("ataXbdDevCreate ERROR: %x  Device %d on Controller %d no BLK_DEV created\n",
				   errno, drive, ctrl);
		return (NULLDEV);
	}

	pDev = (MEN_ATA_DEV *)blkDev;
	if( pDev->blkDevInit == 0 )
	{
		DBGWRT_ERR((DBH,"ataXbdDevCreate ERROR: %x  Device %d on Controller %d\n",
				   errno, drive, ctrl ));
		printErr ("ataXbdDevCreate ERROR: %x  Device %d on Controller %d\n",
				   errno, drive, ctrl);
		error = ATA2_DRIVE_ST_NONE;
	}

	/* block dev device is now created.  Next instantiate the XBD for the
	 * the higher level app and generate an insertion event.
	 */
	if ( semBInit (&pDev->ataXbd.xbdSemId, SEM_Q_PRIORITY, SEM_EMPTY) != OK)
		goto ERR_EXIT;

	/* register new event handler for base device synchronization */
	if (erfHandlerRegister (xbdEventCategory, xbdEventInstantiated,
							menataXbdDevCreateSyncHandler, pDev, 0) != OK)
	{
		goto ERR_EXIT;
	}

	/* store name
	 * take care that name is always NULL terminated */
	strncpy((char *)&pDev->ataXbd.name, name, sizeof(devname_t));
	pDev->ataXbd.name[sizeof(devname_t) - 1] = 0;

	/* attach XBD */
	/* when hot pluggable and no card is inserted on boot: generate empty xbd */
	if( error == ATA2_DRIVE_ST_OK ) {
		DBGWRT_3((DBH,"xbd = 0x%x, funcs = 0x%x, %s, blksize=0x%x, blks=0x%x\n",
						 &pDev->ataXbd.xbd, &menataXbdFuncs, name,
						 pDev->vxBlkDev.bd_bytesPerBlk, pDev->vxBlkDev.bd_nBlocks));
		error = xbdAttach(&pDev->ataXbd.xbd, &menataXbdFuncs, name,
						   pDev->vxBlkDev.bd_bytesPerBlk,
						   pDev->vxBlkDev.bd_nBlocks, &retVal);
	}
	else if( pCtrl->atah->okRemovable )
	{
		/* We are making a 0-block length XBD */
		DBGWRT_3((DBH,"xbd = 0x%x, funcs = 0x%x: create %s 0 Size XBD!!!\n",
						 &pDev->ataXbd.xbd, &menataXbdFuncs, name,
						 pDev->vxBlkDev.bd_bytesPerBlk, pDev->vxBlkDev.bd_nBlocks));
		error = xbdAttach (&pDev->ataXbd.xbd, &menataXbdFuncs, name,
					512, 0, &retVal);
	} else {
		error = ATA2_DRIVE_ST_NONE;
	}

	if (error == 0)
	{
		pDev->ataXbd.ataDev = pDev;
		if (pDev->ataXbd.xbd.xbd_nblocks == 0)
			pDev->ataXbd.xbdInserted = 0;  /* no media inserted */
		else
			pDev->ataXbd.xbdInserted = 1;  /* media inserted */
		pDev->vxBlkDev.bd_readyChanged = 0;
		pDev->ataXbd.device = retVal;   /* save device instance in control block */
		erfEventRaise (xbdEventCategory, xbdEventPrimaryInsert,
					   ERF_ASYNC_PROC, (void *)retVal, NULL);
		DBGWRT_1((DBH,"xbdAttached and event raised. device_t = 0x%x xbdSize=0x%x "
						"xbdInserted=%d\n",
						retVal, pDev->ataXbd.xbd.xbd_nblocks,
						pDev->ataXbd.xbdInserted));
		semTake (&pDev->ataXbd.xbdSemId, WAIT_FOREVER);  /* wait for synchronization */
		return (retVal);
	}
	else
		goto ERR_EXIT;

ERR_EXIT:
	if( pDev )
		free( pDev );
	return (NULLDEV);
}

 /***************************************************************************
 *
 * menataXbdService - task level routine to handle read and write operation
 *
 * This routine waits on a semaphore from strategy routine. It calls
 * menataGetNextBio and menataExecBio to exercise all the bios currently
 * chained for this device.
 *
 * Note that this task is spawned when the device is attached to the system and
 * deleted after the device is detached and all the resources for this device
 * are freed.
 *
 * RETURN: N/A
 *
 * ERRNO: none
 *
 *\NOMANUAL
 */

LOCAL void menataXbdService(
	MEN_ATA_CTRL *pCtrl /* pointer to the ata XBD control structure */
	)
{
	struct bio *  bio;

	while (TRUE) {
		/* Wait semaphore from strategy routine */

		if (semTake(&pCtrl->ataBioReadySem, WAIT_FOREVER) == ERROR)
			return;
		/* get all the bios queued for this device and process them */

		DBGWRT_2((DBH,"menataXbdService: ctrl %d bio queue ready\n",
						pCtrl->ataDevs[0]->ctrl));
		while ((bio = menataGetNextBio(pCtrl)) != NULL)
		{
			/* bio->bio_error has drive number, from strategy call */
			DBGWRT_3((DBH,"processing bio for drive %d  bio=0x%x\n",
							 bio->bio_error, bio));
			menataExecBio (pCtrl, bio, bio->bio_error);
		}
	}
}

/*******************************************************************************
*
* menataGetNextBio - get the next bio in the bio chain
*
* This routine is called to get the next bio in the bio chain for an XBD device.
*
* The drive number for the operation is stored in the bio->bio_error field
*
* RETURNS: pointer to the next bio
*
* ERRNO: none
*
*\NOMANUAL
*/

LOCAL struct bio * menataGetNextBio(
	MEN_ATA_CTRL *pCtrl  /* pointer to XBD block device wrapper */
)
{
	struct bio *  retVal;

	/* take the mutex semaphore for this XBD interface */
	LOCK_ATA;

	retVal = pCtrl->bioQueueh;

	if (retVal != NULL)
	{
		pCtrl->bioQueueh = retVal->bio_chain;
		retVal->bio_chain = NULL;

		if (pCtrl->bioQueueh == NULL)
			pCtrl->bioQueuet = NULL;
	}

	UNLOCK_ATA;
	return (retVal);
}

/***************************************************************************
*
* menataExecBio - execute a bio
*
* This routine processes the bio for read from or write to the XBD block device.
*
* RETURNS: N/A
*
* ERRNO: none
*
*\NOMANUAL
*/

LOCAL void menataExecBio(
	MEN_ATA_CTRL   *pCtrl,  /* pointer to XBD block device wrapper */
	struct bio *bio,         /* pointer to bio */
	int        drive         /* drive number */
	)
{
	MEN_ATA_DEV *  pDev = pCtrl->ataDevs[drive];
	BLK_DEV *  bd = &pDev->vxBlkDev;
	int        status = ERROR;
	unsigned   bd_bsize = pDev->ataXbd.xbd.xbd_blocksize;
	sector_t   bd_blocks = pDev->ataXbd.xbd.xbd_nblocks;
	sector_t   nblocks;
	unsigned   size;

	DBGWRT_2((DBH,"menataExecBio: Entering menataExecBio %d/%d biodrive=%d\n",
					 pDev->ctrl, drive, bio->bio_error));

	/* Check that all of this transaction fits in the disk */
	size = bio->bio_bcount;
	nblocks = size / bd_bsize;

	/* If our starting block number is bad, done with error */
	if (bd_blocks <= bio->bio_blkno)
	{
		bio->bio_bcount = 0;
		bio_done (bio, ENOSPC);
		return;
	}

	/* If we overrun the end of the disk, truncate the block number */
	if (bd_blocks < bio->bio_blkno + nblocks)
	{
		nblocks = bd_blocks - bio->bio_blkno;
	}

	/* calculate the real size and residual */
	size = nblocks * bd_bsize;
	bio->bio_bcount = size;

	/* If we have less than 1 block, set the resid and done it */
	if (nblocks == 0)
	{
		bio->bio_bcount = 0;
		bio_done (bio, 0);
		return;
	}

	if (bio->bio_flags & BIO_READ)
	{
		DBGWRT_3((DBH,"menataExecBio: Calling ata block read\n"));
		status = bd->bd_blkRd (bd, (int) bio->bio_blkno,
				   (int) nblocks, bio->bio_data);
	}
	else if (bio->bio_flags & BIO_WRITE)
	{
		DBGWRT_3((DBH,"menataExecBio: Calling ata block write\n"));
		status = bd->bd_blkWrt (bd, (int)bio->bio_blkno,
				   (int) nblocks, bio->bio_data);
	}
	if (status == OK)
		bio_done (bio, 0);
	else
	{
		bio->bio_bcount = 0;
		/* S_ioLib_DISK_NOT_PRESENT is *really* ENXIO */
		if (errno == S_ioLib_DISK_NOT_PRESENT)
		{
			bio_done (bio, ENXIO);
			/* The underlying media has gone away - remove this device */
		}
		else
		{
			bio_done (bio, errno);
		}
	}
}

/***************************************************************************
*
* menataXbdDump - XBD block device dump routine
*
* /NOMANUAL
*/

LOCAL int menataXbdDump(
	struct xbd *  self,
	sector_t      blkno,
	void *        data,
	size_t        size
	)
{
	return (EINVAL);
}


/***************************************************************************
*
* menataXbdStrategy - XBD block device strategy routine
*
* /NOMANUAL
*/

LOCAL int menataXbdStrategy(
	struct xbd *  self,       /* pointer to XBD */
	struct bio *  bio         /* pointer to bio */
	)
{
	MEN_ATA_XBD    * ataXbd = (MEN_ATA_XBD *) self;
	int          status;
	struct bio * next_bio;
	MEN_ATA_CTRL   * pCtrl  = &MENATA_ctrl[ataXbd->ataDev->ctrl];

	DBGWRT_1((DBH,"menataXbdStrategy called: xbd = 0x%x, bio = 0x%x\n",
			 self, bio));

	if (pCtrl->atah->okRemovable)
	{
		DBGWRT_2((DBH,"               is okRemovable\n"));
		/* Test to see if we have been removed.
		 * If status is not OK, then the XBD has been ejected and outstanding
		 * bio's have been purged from the bio queue.
		 * return status should be either S_ioLib_MEDIA_CHANGED or  ENXIO
		 */
		status = menataXbdTest(ataXbd);
		if (status != OK)
		{
			/* bio queue has been purged, kill this one */
			bio_done(bio, status);
			return (status);
		}
	}

	/* we have a non-zero XBD and there is media inserted */
	LOCK_ATA;

	/* Iterate through the chain, running each bio as we get it */
	for ( ; bio != NULL; bio = next_bio)
	{
		DBGWRT_2((DBH,"               bio = 0x%x\n",bio));
		/* store drive number in bio error field so we know which
		 * drive to read or write from when pulling bio's off queue.
		*/
		bio->bio_error = ataXbd->ataDev->drive;

		/* Make sure that we can get the next one */
		next_bio = bio->bio_chain;

		/* Add this bio as the tail of the queue */
		bio->bio_chain = NULL;

		if (pCtrl->bioQueuet)
		{
			pCtrl->bioQueuet->bio_chain = bio;
		}
		else
		{
			pCtrl->bioQueueh = bio;
		}
		pCtrl->bioQueuet = bio;

		/* Tap our service task */
		semGive (&pCtrl->ataBioReadySem);
	}

	UNLOCK_ATA;
	return (OK);
}

/***************************************************************************
*
* menataXbdIoctl - XBD block device ioctl routine
*
* /NOMANUAL
*/

LOCAL int menataXbdIoctl(
	struct xbd *  xbd,
	int           command,
	void    *     arg
	)
{
	MEN_ATA_XBD *  ataXbd = (MEN_ATA_XBD *)xbd;
	device_t            dev;
	INT16               reType;
	BLK_DEV *           bd = &ataXbd->ataDev->vxBlkDev;
	XBD_GEOMETRY        *geo;
	int                 status;
	MEN_ATA_CTRL   * pCtrl = &MENATA_ctrl[ataXbd->ataDev->ctrl];
	MEN_ATA_DEV    * pDev = pCtrl->ataDevs[ataXbd->ataDev->drive];
	struct bio * bio;

	DBGWRT_2((DBH,"menataXbdIoctl called:  xbd = 0x%x, ioctl = 0x%x\n",
			ataXbd, command));

	if (pCtrl->atah->okRemovable)
	{
		/* Test to see if we have been removed.
		 * If status is not OK, then the XBD has been ejected and outstanding
		 * bio's have been purged from the bio queue.
		 * return status should be either S_ioLib_MEDIA_CHANGED or  ENXIO
		 */
		status = menataXbdTest(ataXbd);
		if (status == S_ioLib_MEDIA_CHANGED)
			goto IOCTL_EXIT;
	}

	switch (command)
	{
	case XBD_SOFT_EJECT:
	case XBD_HARD_EJECT:
		/* we are the base XBD so if we get these it is for us */
		/*
		 * first raise a removal event
		 * then purge the bio queue
		 * last raise an insertion event
		 *
		 */
		dev = ataXbd->xbd.xbd_dev.dv_dev;
		erfEventRaise(xbdEventCategory, xbdEventRemove, ERF_ASYNC_PROC,
					 (void *)dev, NULL);
		while ((bio = menataGetNextBio(pCtrl)))
			bio_done(bio, ENXIO);
		reType = (command == XBD_HARD_EJECT) ?
				  xbdEventPrimaryInsert : xbdEventSoftInsert;
		erfEventRaise(xbdEventCategory, reType, ERF_ASYNC_PROC,
							(void *)dev, NULL);

		status = OK;
		break;

	case XBD_GETGEOMETRY:
		geo = (XBD_GEOMETRY *) arg;
		geo->heads = bd->bd_nHeads;
		geo->secs_per_track = bd->bd_blksPerTrack;
		geo->total_blocks = bd->bd_nBlocks;
		geo->blocksize = bd->bd_bytesPerBlk;
		geo->cylinders = geo->blocksize / geo->secs_per_track / geo->heads;
		status = OK;
		break;

	case XBD_GETBASENAME:
		dev = ataXbd->xbd.xbd_dev.dv_dev;
		if (devName(dev, arg) == ERROR)
			status = errno;
		else
			status = OK;
		break;

	case XBD_SYNC:
		bd->bd_ioctl(pDev, FIOSYNC, 0);
		status = OK;
		break;

	case XBD_TEST:
		DBGWRT_3((DBH,"XBD_TEST just return OK\n"));
		status = OK;
		break;

	case XBD_STACK_COMPLETE:
		if (!ataXbd->xbdInstantiated)
		{
			LOCK_ATA;
			erfEventRaise (xbdEventCategory, xbdEventInstantiated,
						   ERF_ASYNC_PROC,
						   ataXbd->xbd.xbd_dev.dv_xname, NULL);
			ataXbd->xbdInstantiated = TRUE;
			UNLOCK_ATA;
			DBGWRT_3((DBH,"xbd %d/%d instantiated\n",
						  ataXbd->ataDev->ctrl, ataXbd->ataDev->drive));
		}
		status = OK;
		break;

	default:
		DBGWRT_3((DBH,"menataXbdIoctl unknown ioctl = 0x%x: forward to block device\n",
					 command));
		status = bd->bd_ioctl(pDev, command, arg);
		break;
	}  /* end switch */

IOCTL_EXIT:
	DBGWRT_2((DBH,"menataXbdIoctl exit xbd = 0x%x: status=%x\n",
				  ataXbd, status));

	return (status);
}

/***************************************************************************
*
* menataXbdDevCreateSyncHandler -
*/

LOCAL void menataXbdDevCreateSyncHandler(
	UINT16 Category,
	UINT16 Type,
	void * eventData,
	void * userData
	)
{
	MEN_ATA_DEV   *  pDev;
	devname_t *  devname;

	if ((Category == xbdEventCategory) && (Type == xbdEventInstantiated))
	{
		devname = (devname_t *) eventData;
		pDev = (MEN_ATA_DEV *) userData;

		if (strncmp ((char *) devname,
					 (char *) pDev->ataXbd.name,
					 sizeof (devname_t)) != 0)
		{
			return;
		}

		erfHandlerUnregister (xbdEventCategory, xbdEventInstantiated,
							  menataXbdDevCreateSyncHandler, userData);

		semGive (&pDev->ataXbd.xbdSemId);
	}

	return;
}


/***************************************************************************
* menataXbdMediaTest - Test a block wrapper for media existence
*
* /NOMANUAL
*/

LOCAL int menataXbdMediaTest(
	MEN_ATA_XBD *  ataXbd
	)
{
	BLK_DEV *  bd = &ataXbd->ataDev->vxBlkDev;
	int error;
	BOOL rc_bit;
	DBGCMD( MEN_ATA_CTRL * pCtrl = &MENATA_ctrl[ataXbd->ataDev->ctrl]; );

	/* call the driver's status routine, if one exists */

	if (bd->bd_statusChk)
		error = bd->bd_statusChk( bd );
	else
		error = OK;

	DBGWRT_1((DBH,"menataXbdMediaTest: %d/%d  error=0x%x  readyChanged = %d\n",
					 ataXbd->ataDev->ctrl, ataXbd->ataDev->drive,
					 error, bd->bd_readyChanged));

	rc_bit = bd->bd_readyChanged;
	bd->bd_readyChanged = 0;

	if (error != OK)
		return (ERROR);

	if (rc_bit)
		return (ERROR);

	return (OK);
}


/***************************************************************************
*
* menataXbdTest - Test to see if media is present or not.  If a change
*              of media has occurred, test XBD's for 0 size and eject
*              the device and instantiate the next device accordingly.
*
* /NOMANUAL
*/
LOCAL int menataXbdTest(
	MEN_ATA_XBD    *ataXbd     /* pointer to XBD */
	)
{
	int status;
	DBGCMD( MEN_ATA_CTRL * pCtrl = &MENATA_ctrl[ataXbd->ataDev->ctrl]; );

	status = menataXbdMediaTest(ataXbd);
	if (ataXbd->xbdInserted)
	{
		/* We are a full-size XBD - if there is no media, then eject */
		if (status == ERROR)
		{
			/* Removal happened */
			DBGWRT_1((DBH,"menataXbdTest: Calling Eject %d/%d, full size and no media\n",
						 ataXbd->ataDev->ctrl, ataXbd->ataDev->drive));
			menataXbdEject(ataXbd);
			return (S_ioLib_MEDIA_CHANGED);
		}
	}
	else
	{
		/* We are a 0 size XBD */
		if (status == OK)
		{
			/* if there IS media then eject */
			DBGWRT_1((DBH,"menataXbdTest: Calling Eject %d/%d, zero size and media\n",
						 ataXbd->ataDev->ctrl, ataXbd->ataDev->drive));
			menataXbdEject(ataXbd);
			return (S_ioLib_MEDIA_CHANGED);
		}
		else
		{
			/* 0 size and no media, check for media */
			status = menataXbdMediaTest(ataXbd);
			if (status == OK)
			{
				menataXbdEject(ataXbd);
				return (S_ioLib_MEDIA_CHANGED);
			}
			else
			{
				DBGWRT_ERR((DBH,"menataXbdTest: Returning Error %d/%d, zero size and no media\n",
							 ataXbd->ataDev->ctrl, ataXbd->ataDev->drive));
				return (ENOSPC);
			}
		}
	}
	return (OK);
}


/***************************************************************************
*
* menataXbdEject - Eject the device and instantiate the next step
*
* /NOMANUAL
*/

LOCAL int menataXbdEject(
	MEN_ATA_XBD   *ataXbd
	)
{
	int error;
	device_t device;
	BLK_DEV *  bd = &ataXbd->ataDev->vxBlkDev;
	struct bio *bio;
	MEN_ATA_CTRL * pCtrl = &MENATA_ctrl[ataXbd->ataDev->ctrl];
	RBDRV_HANDLE *rbh = &pCtrl->atah->ent;
	RBDRV_IOCTL_PARMS ioctl;

	DBGWRT_1((DBH,"entered menataXbdEject %d/%d\n",
					 ataXbd->ataDev->ctrl, ataXbd->ataDev->drive));

	/* Announce the ejection */
	erfEventRaise(xbdEventCategory, xbdEventMediaChanged, ERF_ASYNC_PROC,
				  (void *)ataXbd->xbd.xbd_dev.dv_dev, NULL);

	/* Detach the XBD handle */
	xbdDetach(&ataXbd->xbd);

	/* Done any pending bio's */
	while ((bio = menataGetNextBio(pCtrl)))
		bio_done(bio, ENXIO);

	/*
	* Now we instantiate the next XBD - if we have media, then its a regular XBD
	* but if we don't, its an xbd of size 0 which fills in for the XBD until media
	* is actually inserted
	*/

	if (menataXbdMediaTest(ataXbd) == OK)
	{
		/* We have new media to mount */
		/*--------------------------------------+
		|  Query devices capacity/sector size   |
		+--------------------------------------*/
		ioctl.subdev = ataXbd->ataDev->drive;
		ioctl.flags  = RBDRV_RAW_ACCESS;

		if( (error = rbh->Ioctl(rbh, RBDRV_IOCTL_INFO, &ioctl)))
			return ERROR;

		DBGWRT_1((DBH, "  logSectors=%ld ssize=%ld any=%ld chs=%ld/%ld/%ld\n",
			   ioctl.p.info.logSectors,
			   ioctl.p.info.sectSize, ioctl.p.info.sectSizeAny,
			   ioctl.p.info.cylinders, ioctl.p.info.heads,
			   ioctl.p.info.sectorsTrack ));

		bd->bd_nBlocks      = ioctl.p.info.logSectors;  /* number of blocks */
		bd->bd_bytesPerBlk  = ioctl.p.info.sectSize;    /* bytes per block  */
		bd->bd_blksPerTrack = ioctl.p.info.sectorsTrack;/* blocks per track */

		bd->bd_nHeads       = ioctl.p.info.heads;       /* heads */

		error = xbdAttach ((struct xbd *) ataXbd, &menataXbdFuncs, ataXbd->name,
							 bd->bd_bytesPerBlk, bd->bd_nBlocks, &device);

		ataXbd->xbdInserted = 1;
	}
	else
	{
		/* We are making a 0-block length XBD */
		error = xbdAttach ((struct xbd *) ataXbd, &menataXbdFuncs, ataXbd->name,
					512, 0, &device);
		ataXbd->xbdInserted = 0;
	}


	/* Announce our new XBD */
	if (error == OK)
	{
		erfEventRaise(xbdEventCategory, xbdEventPrimaryInsert,
						ERF_ASYNC_PROC, (void *)device, NULL);
		DBGWRT_1((DBH,"xbdAttached and event raised. device_t = 0x%x xbdSize=0x%x\n",
			 device, bd->bd_nBlocks));
	}

	return (error);
}


/*******************************************************************************
*
* ataXbdRawio - do raw I/O access
*
* This routine is called to perform raw I/O access.
*
* <device>   is the XBD device identifier for the drive
* <sector>   starting sector for I/O operation
* <numSecs>  number of sectors to read/write
* <data>     pointer to data buffer
* <dir>	     read or write
*
* The <pAtaRaw> is a pointer to the structure ATA_RAW which is defined in
* ataDrv.h.
*
* RETURNS:
* OK, or ERROR if the parameters are not valid.
*
*/
STATUS ataXbdRawio(
	device_t	device,
	sector_t	sector,
	UINT32	numSecs,
	char	*data,
	int		direction
	)
{
	STATUS	status = OK;    /* the return code */
	XBD		*xbd;
	MEN_ATA_XBD	*ataXbd;

	xbd = (XBD *)devMap(device);
	if (xbd == NULL)
	return (ERROR);

	ataXbd = (MEN_ATA_XBD *)xbd;

	status = menAtaBlkRW (ataXbd->ataDev, (UINT32)sector, numSecs, data,
			direction);

	devUnmap ((struct device *) xbd);
	return (status);
}



