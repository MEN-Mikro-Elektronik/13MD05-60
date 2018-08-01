/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: menata2.c
 *      Project: MEN ATA driver for VxWorks
 *
 *       Author: ch/kp
 *        $Date: 2006/12/22 15:09:04 $
 *    $Revision: 1.5 $
 *
 *  Description: This module is the VxWorks specific layer on the top of
 *               the common ATA2 (UDMA32) low level driver provides a VxWorks
 *               block device interface.
 *
 *     Required: -
 *     Switches: INCLUDE_SHOW_ROUTINES to include ataShow
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: menata2.c,v $
 * Revision 1.5  2006/12/22 15:09:04  cs
 * added:
 *    + chipset.initLate() handling (eg. additional interrupt enables, ...)
 *
 * Revision 1.4  2006/03/16 17:48:53  cs
 * cosmetics
 *
 * Revision 1.3  2006/02/14 22:08:12  cs
 * changed interface of ataDevCreate and ataShowInit for VxWorks 6.2
 *
 * Revision 1.2  2005/11/21 16:42:54  ts
 * cosmetic: more verbose error messages when ataDrv fails
 * ataDrv::if level=-1, no IRQ fctn is connected (for 16z023 special handler)
 *
 * Revision 1.1  2005/09/01 08:44:47  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005..2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

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

#define ATA_PARAM ___ATA_PARAM	/* ataioctl.h also defines ATA_PARAM... */
#include "drv/hdisk/ataDrv.h"
#undef ATA_PARAM

#include "MEN/men_typs.h"
#include "MEN/maccess.h"
#include "MEN/dbg.h"
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
/*#define MENATA2_DEBUG*/

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

/* per controller structure (filled by ataDrv()) */
typedef RBDRV_HANDLE *MEN_ATA_CTRL;


/* blk device structure. created by ataDevCreate() */
typedef struct
{
    BLK_DEV          vxBlkDev;             /* generic block device structure */
    RBDRV_HANDLE     *rbh;          	   /* MEN common ATA device handle   */
    u_int32 		 flags;		           /* for real raw disk access       */
    int              ctrl;
	int				 drive;
    int              blkOffset;
} MEN_ATA_DEV;


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


static int32 MapErrorCodes( int32 rbdrvErr );


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
    ATA_RESOURCE *pAta		= &ataResources[ctrl];
    PCCARD_RESOURCE *pResource	= &pAta->resource;
	u_int32 drvMask;
	int32 error;
	ATA2_DESC_T desc;
	ATA2_HANDLE_T *h;

#ifdef MENATA2_DEBUG
	printf("ataDrv: irq level=%d vector=%d wdgTimeout=%d semTimeout=%d\n",
		   level, vector, wdgTimeout, semTimeout);
#endif

    if ( (ctrl >= ATA_MAX_CTRLS) || (drives > ATA_MAX_DRIVES) || (drives>2) ){
		printf("*** ataDrv: invalid ctrl/drives param: c=%d/d=%d!\n",
			   ctrl, drives );
		return (ERROR);
	}

	if( (configType != ATA_DMA_AUTO) && (configType != ATA_PIO_AUTO) ){
		printf("*** ataDrv: invalid configType parameter!\n");
		return ERROR;
	}

	if( (pAta->ctrlType < IDE_LOCAL) || (pAta->ctrlType > 3)){
	    printf("*** ataDrv: illegal controller type %d\n", pAta->ctrlType);
		return ERROR;
	}

	if( MENATA_ctrl[ctrl] != NULL ){
	    printf("*** ataDrv: controller already initialized!\n");
		return ERROR;			/* already initialized */
	}

	memset( &desc, 0, sizeof(desc));

	desc.maRegBlock[0] = (MACCESS)pResource->ioStart[0];
	desc.maRegBlock[1] = (MACCESS)pResource->ioStart[1];
	desc.maRegBlockDma = (MACCESS)pResource->memStart;
	desc.chipset 	   = (const ATA2_CHIPSET_T *)pResource->ioStop[0];
	desc.chipsetArg    = (void *)pResource->ioStop[1];

	if( desc.maRegBlock[0] == 0 ){
	    printf("*** ataDrv: maRegBlock[0]==0 - controller doesnt exist? \n");
		return ERROR;			/* this controller does not exist */
	}

	drvMask = (1<<drives)-1;	/* mask for supported drives */

	if( configType == ATA_DMA_AUTO )
		drvMask |= (ATA2_DRVMASK_MASTER_UDMA | ATA2_DRVMASK_SLAVE_UDMA);

	desc.drvMask = drvMask;
	desc.allowSlaveCf = TRUE;
	desc.useIrq = TRUE;
	desc.toutReset = 30;
	desc.retry = 5;

#ifdef MENATA2_DEBUG
    printf("ata2Drv: ctrl=%d drives=%d\n     base=%08x level=0x%x\n",
	       ctrl, drives,
	       desc.maRegBlock[0], level);
	{
		extern unsigned char *DBG_MemP;
		extern unsigned char *DBG_WriteP;
		extern unsigned int  DBG_MemSize;
		extern unsigned int  DBG_doCacheFlush;
		DBG_HANDLE *dbh;

		DBG_MemSize = 0x100000;
		DBG_doCacheFlush = TRUE;
		DBG_Init( NULL, &dbh );
		printf("DBG_MemP = %x %x\n", DBG_MemP, &DBG_WriteP );

	}
#endif
	/*--- init ATA driver ---*/
	error = ATA2_Create(OSS_VXWORKS_OS_HDL, 0x40008003/*DBG_ALL*/, &desc, &h );
	if( error )
		goto CLEANUP;

	MENATA_ctrl[ctrl] = h;

	/* ts: dont connect devices with own IRQ handler, when passed level =-1. */
	if (level>=0){
		intConnect( INUM_TO_IVEC(level), ATA2_Irq, (int)h );
		intEnable( level );
	}

	if( desc.chipset->initLate )
		desc.chipset->initLate( h );

	/*--- startup drives ---*/
	ATA2_StartupDrives( h );

    return (OK);

CLEANUP:
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
	MEN_ATA_CTRL pCtrl = MENATA_ctrl[ctrl];
    BLK_DEV *pBlkdev;
    RBDRV_IOCTL_PARMS ioctl;
    int32 error;

	printf("ataDevCreate:: ctrl=%d drive=%d nBlocks=%d blkOffset=%d\n",
	  ctrl, drive, nBlocks, blkOffset );

    if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES))
		return (NULL);

	if( pCtrl == NULL ){
		printf("ataDevCreate:: controller not initialized\n" );
		return NULL;			/* controller not initialized */
	}

    if ((pDev = (MEN_ATA_DEV *)malloc(sizeof (MEN_ATA_DEV))) == NULL)
		return (NULL);

    pBlkdev = &pDev->vxBlkDev;

    /*--------------------------------------+
    |  Query devices capacity/sector size   |
    +--------------------------------------*/
    ioctl.subdev = drive;
    ioctl.flags  = RBDRV_RAW_ACCESS;

    if( (error = pCtrl->Ioctl(pCtrl, RBDRV_IOCTL_INFO, &ioctl)))
		goto ERREXIT;

    printf("  logSectors=%ld ssize=%ld any=%ld chs=%ld/%ld/%ld\n",
		   ioctl.p.info.logSectors,
		   ioctl.p.info.sectSize, ioctl.p.info.sectSizeAny,
		   ioctl.p.info.cylinders, ioctl.p.info.heads,
		   ioctl.p.info.sectorsTrack );

    pBlkdev->bd_nBlocks      = ioctl.p.info.logSectors;  /* number of blocks */
    pBlkdev->bd_bytesPerBlk  = ioctl.p.info.sectSize;    /* bytes per block  */
    pBlkdev->bd_blksPerTrack = ioctl.p.info.sectorsTrack;/* blocks per track */

    pBlkdev->bd_nHeads       = ioctl.p.info.heads;       /* heads */
    pBlkdev->bd_removable	= TRUE;	/* why???  */
    pBlkdev->bd_retry		= 1;
    pBlkdev->bd_mode		= O_RDWR;
    pBlkdev->bd_readyChanged = TRUE;
    pBlkdev->bd_blkRd		= ataBlkRd;
    pBlkdev->bd_blkWrt		= ataBlkWrt;
    pBlkdev->bd_ioctl		= ataIoctl;
    pBlkdev->bd_reset		= NULL;
    pBlkdev->bd_statusChk	= NULL;

    pDev->ctrl			= ctrl;
    pDev->drive			= drive;
		pDev->blkOffset		= blkOffset;
    pDev->rbh          	= pCtrl;

    return (&pDev->vxBlkDev);

 ERREXIT:
	if( pDev )
		free( pDev );
	return NULL;
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
	MEN_ATA_CTRL pCtrl = MENATA_ctrl[ctrl];
    RBDRV_IOCTL_PARMS ioctl;
	STATUS status = ERROR;
	int32 retCode;
	u_int32 startBlk;

	/*printf("ataRawIo: ctrl=% drv=%d cyl=%d hd=%d sec=%d nsect=%d dir=%d\n",
		   ctrl, drive, pAtaRaw->cylinder, pAtaRaw->head, pAtaRaw->sector,
		   pAtaRaw->nSecs, pAtaRaw->direction );*/


    if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES))
		return( status );

	if( pCtrl == NULL )
		return( status );			/* controller not initialized */

    /*--------------------------------------+
    |  Query devices capacity/sector size   |
    +--------------------------------------*/
    ioctl.subdev = drive;
    ioctl.flags  = RBDRV_RAW_ACCESS;

    if( (retCode = pCtrl->Ioctl(pCtrl, RBDRV_IOCTL_INFO, &ioctl)))
		return ERROR;

    startBlk = pAtaRaw->cylinder *
		(ioctl.p.info.sectorsTrack * ioctl.p.info.heads) +
		pAtaRaw->head * ioctl.p.info.sectorsTrack + pAtaRaw->sector - 1;

	/*printf("ataRawIo: startBlk=%ld\n", startBlk );*/


	if( pAtaRaw->direction==0 ){
		retCode = pCtrl->ReadBlocks( pCtrl, drive,
											startBlk, pAtaRaw->nSecs,
											pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
	}
	else {
		retCode = pCtrl->WriteBlocks( pCtrl, drive,
											startBlk, pAtaRaw->nSecs,
											pAtaRaw->pBuf,
											ioctl.p.info.sectSize,
											RBDRV_RAW_ACCESS );
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
	return ERROR;
}


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
    int     bytesPerBlk;    /* number of bytes per block */
    int     status = OK;    /* the return code */
    int32   retCode;

    bytesPerBlk = pDev->vxBlkDev.bd_bytesPerBlk;
	startBlk += pDev->blkOffset;

    /*------------------+
    | read sectors      |
    +------------------*/
    retCode = (pDev->rbh->ReadBlocks)( pDev->rbh, pDev->drive,
									   startBlk, numBlks, pBuffer,
									   bytesPerBlk, RBDRV_RAW_ACCESS );
    if( retCode )
    {
        status = ERROR;
        errnoSet( MapErrorCodes(retCode) );
    }

    return status;
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
    int     bytesPerBlk;    /* number of bytes per block */
    int     status = OK;    /* the return code */
    int32   retCode;

    bytesPerBlk = pDev->vxBlkDev.bd_bytesPerBlk;
	startBlk += pDev->blkOffset;

    /*------------------+
    | read sectors      |
    +------------------*/
    retCode = (pDev->rbh->WriteBlocks)( pDev->rbh, pDev->drive,
										startBlk, numBlks, pBuffer,
										bytesPerBlk, RBDRV_RAW_ACCESS );
    if( retCode )
    {
        status = ERROR;
        errnoSet( MapErrorCodes(retCode) );
    }

    return status;
}/*ataBlkWrt*/



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
	MEN_ATA_CTRL pCtrl = MENATA_ctrl[ctrl];
    RBDRV_IOCTL_PARMS ioctl;
	ATA_IOCTL_PARMS *ata = (ATA_IOCTL_PARMS *)ioctl.p.data;
	int32 retCode;
	const int w=25;

    if ((ctrl >= ATA_MAX_CTRLS) || (drive >= ATA_MAX_DRIVES)){
		printf("ctrl/drive out of range\n");
		return ERROR;
	}

	if( pCtrl == NULL ){
		printf("Controller not initialized\n");
		return ERROR;
	}

	/*--- geometry/capacity info */
    ioctl.subdev = drive;
    ioctl.flags  = RBDRV_RAW_ACCESS;

    if( (retCode = pCtrl->Ioctl(pCtrl, RBDRV_IOCTL_INFO, &ioctl))){
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
    if( (retCode = pCtrl->Ioctl(pCtrl, ATA_IOCTL_GET_LBA, &ioctl)))
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

    if( (retCode = pCtrl->Ioctl(pCtrl, ATA_IOCTL_GET_XFER, &ioctl)))
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
	default:				printf("unknown (0x%02x)", ioctl.p.data[0] );
	}

	printf("\n");

	/*--- ATA identify info ---*/
	{
	    /* query drive info */
		int i;
		const int dumpwords[] = {
			0, 1, 3, 6, 47, 49, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 63,
			64, 65, 66, 67, 68, 88
		};
		u_int16 *words;

		if( (retCode = pCtrl->Ioctl(pCtrl, ATA_IOCTL_INFO, &ioctl)))
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

		for( i=0; i<(int)(sizeof(dumpwords)/sizeof(int)); i++ ){
			if( !(i%4)) printf("\n");
			printf("  Word %02d: %04x", dumpwords[i], words[dumpwords[i]]);
		}
		printf("\n");
	}

	return OK;
}
#endif /* INCLUDE_SHOW_ROUTINES */


