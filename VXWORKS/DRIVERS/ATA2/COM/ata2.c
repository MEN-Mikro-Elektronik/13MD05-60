/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  ata2.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2008/11/17 13:17:10 $
 *    $Revision: 1.14 $
 *
 *	   \project  many
 *  	 \brief  Generic simple ATA disk driver
 *
 *    \switches  none or MEN_EM01 or MENMON
 */
/*-------------------------------[ History ]---------------------------------
 * $Log: ata2.c,v $
 * Revision 1.14  2008/11/17 13:17:10  cs
 * R: avoid warning about unused function RegDump() for non DBG compilation
 * M: include function RegDump in "#ifdef DBG"
 *
 * Revision 1.13  2008/09/05 11:18:59  cs
 * R: a) bmIdeCommand register was always completely overwritten
 *       this caused some bits (SiI3114/Interrupt Steering) to be cleared
 * M: a) only set/clear bits we need to change in bmIdeCommand register
 *
 * Revision 1.12  2008/05/05 10:18:40  cs
 * DiveMaxUdmaMode(): added support for UDMA 6
 *
 * Revision 1.11  2007/01/12 17:01:44  cschuster
 * fixed:
 *   - set drive state to ATA2_DRIVE_ST_NONE when Identify fails
 *   - claim irqSem in StartupDrives (some CFs generate stray IRQs on power up)
 * changed:
 *   - put all RegDump in DBGCMD() (may avoid useless function calls)
 *   - RegDump now puts line from where it was called
 *
 * Revision 1.10  2006/12/22 15:05:18  cs
 * fixed:
 *    - enhanced handling for hot swap
 *    - rearranged (level) of debug messages
 *    - don't read status reg when not really needed, use alternate status reg
 *      in WaitBusyLow() and RegDump()
 *
 * Revision 1.9  2006/09/09 17:27:09  cs
 * removed workarounds for EM1
 *
 * Revision 1.8  2006/08/30 15:04:18  ufranke
 * cosmetics
 *
 * Revision 1.7  2006/02/02 17:03:29  cs
 * fixed
 *   - extended Ulf's quick hack for different EM1 FPGA designs
 *
 * Revision 1.6  2006/01/25 10:34:38  UFRANKE
 * fixed
 *  - registerblock address in quick hack
 *
 * Revision 1.5  2006/01/24 10:43:57  UFRANKE
 * fixed
 * + quick hack for EM01 - IDE IRQ is called only
 *   if IRQ is from it's disk
 *   remove this hack if we have an clean interrupt wrappper
 *
 * Revision 1.4  2005/11/14 11:40:33  UFranke
 * changed
 *  - short delay in WaitBusyLow()
 *    try to detect BusyHigh before if switch MENMON was set
 *
 * Revision 1.3  2005/11/11 11:07:17  cs
 * now set SDH to master in DoReset and at end of StartupDrives
 * fixed DBG output in ResetWait (function name)
 *
 * Revision 1.2  2005/09/01 08:44:44  kp
 * fixed compiler warnings
 *
 * Revision 1.1  2005/08/31 14:54:50  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/**
 Reimplementation of the MEN_PCMCIA ATA (ata.c Rev 2.11)
 driver with these main differences:

 - ATA_DESC specifies per instantiation options rather compiler time switches.
   Allows multiple instances with different ATA register layout and features.

		 ATA_USE_xxx_ACCESS		-> ATA2_DESC_T.regOffs
		 ATA_USE_IRQ			-> ATA2_DESC_T.useIrq
		 ATA_TOUT_RESET			-> ATA2_DESC_T.toutReset
		 ATA_SUPPORT_DMA		-> function pointers to actual DMA routines
		                           ATA2_DESC_T.chipsetHandleInit and
								   ATA2_DESC_T.chipsetSetMode

		 ATA_RETRY				-> ATA2_DESC_T.retry (1=no retries)
		 ATA_ALLOW_SLAVE_CF		-> ATA2_DESC_T.allowSlaveCf
		 ATA_NO_IRQ_INSTALL		-> removed. IRQ must be installed always
		                           by caller.

 - LBA48 addressing (drives >128GB), even in this mode, driver uses
   a maximum of 32 bit (2 TByte) and a max. transfercount of 256 sectors
   per command.

 - PCMCIA socket access no longer supported
 - Code for partition handling removed. Must be done by caller


 This code supports ATA drives (rotating and flash media) complying to
 ATA2..6 standard.

 Both Master and Slave drives can be used. The presence of master and slave
 are determined automatically by ATA2_StartupDrives().

 Instantiation:

   The ATA2_Create() function should be called exactly once for each
   instance.  In contrast to the original ATA driver, it creates the
   handle only. Actual drive initialisation must be explicitely requested
   by calling ATA2_StartupDrives() afterwards.


 Chipset support:

   Caller must pass a chipset class structure to ATA2_Create(). This
   structure contains functions pointers to the chipset specific
   functions.

   Several chipset drivers are already present in this directory,
   which may be referenced by the caller of ATA2_Create().

   \em init is called during ATA2_Create(). It must initialize
   register offsets in the handle (regOffs members). The base
   addresses of these register blocks (descriptor members
   maRegBlock[0/1] and maRegBlockDma) have to be determined by the
   caller.

   \em setMode is called during ATA2_StartupDrives() to
   determine the maximum PIO and DMA mode. If it is passed as NULL,
   the driver assumes PIO mode 0 and no DMA mode.

   If DMA mode is possible, this driver assumes a standard programming
   interface for busmaster DMA, specified by the handle members
   bmIdeCommand, bmIdeStatus, bmIdePrdTable. This code is
   not chipset specific as long as the IDE controller confirms to
   standard IDE busmaster controller regs.

   \em term is called from Exit() function

 Interrupt handling

   This driver does not register the interrupt routine to the operating
   system, the upper (OS specific) layer must call ATA2_Irq() when an
   interrupt occurs.

 Master/Slave Support User Options:

   ATA2_DESC_T.drvMask contains 4 bits that allows the caller to specify
   - if master and/or slave drive is supported
   - if DMA shall be allowed on master and/or slave

   See ATA2_DRVMASK_xxx defines in ata2_drv.h

 Hardware Access

   is done via MREAD_xx and MWRITE_xxx macros. Wether regs are
   accessed memory mapped or I/O mapped must be decided at compile
   time.  It is currently not possible to have memory and I/O accessed
   driver version in the same namespace (unless all global symbols are
   redefined)

 TODO:
  - Check IDENTIFY checksum
  - Support 32 Bit PIO transfers (e.g. NAND IDE)
  - Fill DriveMaxPioMode with life
  - Support ATAPI
  - Support SMART

*/

static const char RCSid[]="$Id: ata2.c,v 1.14 2008/11/17 13:17:10 cs Exp $";

#define MEN_ATA2COM_COMPILE
#include "ata2_int.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifdef MENMON
extern u_int32 MMBIOS_dbgLevel;
# define DBG_MYLEVEL	MMBIOS_dbgLevel
#else
# define DBG_MYLEVEL	h->debugLevel
#endif
#define DBH			h->dbh

/** Shortcuts to members of ATA_DESC */
#define r		regOffs

# define ATA_READ_D8(h,io,reg)	\
  MREAD_D8(h->desc.maRegBlock[io], reg )
# define ATA_WRITE_D8(h,io,reg,v)	\
  MWRITE_D8(h->desc.maRegBlock[io], reg, v )
# define ATA_WRITE_D32(h,io,reg,v)	\
  MWRITE_D32(h->desc.maRegBlock[io], reg, v )
# define ATA_READ_STRING_D16(h,io,reg,buf,len) \
  MFIFO_READ_D16(h->desc.maRegBlock[io],reg,len,buf)
# define ATA_WRITE_STRING_D16(h,io,reg,buf,len) \
  MFIFO_WRITE_D16(h->desc.maRegBlock[io],reg,len,buf)

/* Macros to access chipset bus master regs */
# define ATA_BMIDE_READ_D8(h,reg)	\
  MREAD_D8(h->desc.maRegBlockDma, reg )
# define ATA_BMIDE_WRITE_D8(h,reg,v)	\
  MWRITE_D8(h->desc.maRegBlockDma, reg, v )
# define ATA_BMIDE_WRITE_D32(h,reg,v)	\
  MWRITE_D32(h->desc.maRegBlockDma, reg, v )


#ifdef BIG_ENDIAN
#define SWAP_LE16(w) OSS_SWAP16(w)
#define SWAP_LE32(l) OSS_SWAP32(l)
#else
#define SWAP_LE16(w) (w)
#define SWAP_LE32(l) (l)
#endif

#define TICKDIFF(h,basetick)  OSS_TickGet((h)->osh) - basetick

#define ATA_BLOCK_SIZE	    512

#define TOUT_READY			(1*1000) 	/* (ms) until ready for command */
#define TOUT_CMD			(5*1000) 	/* (ms) for command completion */

#define _strncmp(s,t,n) OSS_StrNcmp( h->osh, s, t, n )

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int32 Diagnose( ATA_HANDLE_T *h, u_int8 *diagStatusP );
static int32 DoReset( ATA_HANDLE_T *h );
static int32 Identify( ATA_HANDLE_T *h, int subdev, ATA_PARAM *parm );
DBGCMD( static void RegDump( ATA_HANDLE_T *h, int line ); )
static int32 RegTest( ATA_HANDLE_T *h );

static int32 PrepareCmd( ATA_HANDLE_T *h, u_int8 sdh, u_int32 ms);
static int32 WaitBusyLow( ATA_HANDLE_T *h, u_int32 ms);
static int32 WaitInterrupt( ATA_HANDLE_T *h, u_int32 ms);
static u_int32 Ms2Ticks( ATA_HANDLE_T *h, u_int32 ms );
static int32 TransferSecs(
    ATA_HANDLE_T *h,
    int subdev,
    u_int32 lsn,
    u_int32 nBlocks,
    u_int8 *buffer,
    ATA_DIRECTION direction);
static int32 Command(
	ATA_HANDLE_T *h,
	const ATA_TASK_T *task,
	u_int32 nBlocks,
	u_int8 *buffer,
    u_int32 setupTo,
    u_int32 cmdTo);

/**********************************************************************/
/** Get driver revision info string
 *
 * \param _h		\IN ATA handle
 * \param buf		\IN ptr to buffer where ident string will be stored
 * \param len		\IN max space in buf
 *
 * \return >0: number of bytes copied, -1: user buffer too small
 */
static int32 Ident( RBDRV_HANDLE *_h, char *buf, u_int32 len )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;

	u_int32 copylen = OSS_StrLen( h->osh, (char *)RCSid ) + 1;

	if( copylen > len )
		return -1;

	OSS_StrCpy( h->osh, (char *)RCSid, buf );

	return copylen;
}

/**********************************************************************/
/** Set driver debug level
 *
 * \param _h		\IN ATA handle
 * \param dbgLevel	\IN debug level to set
 */
static void DbgLevelSet( RBDRV_HANDLE *_h, u_int32 dbgLevel )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	h->debugLevel = dbgLevel;
}

/**********************************************************************/
/** Read blocks from ATA drive
 *
 * \param h			\IN ATA handle
 * \param subdev	\IN	sub device (0=master 1=slave IDE drive)
 * \param lsn		\IN logical sector number to access
 * \param nblks		\IN number of blocks to read
 * \param buf		\OUT receives data read
 * \param blksz		\IN size of each sector (must be 512)
 * \param flags		\IN ignored
 *
 * \return RBDRV error code
 */
static int32 ReadBlocks(
	RBDRV_HANDLE *_h,
	u_int32 subdev,
	u_int32 lsn,
    u_int32 nblks,
    u_int8  *buf,
    u_int32 blksz,
    u_int32 flags)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	int32 error;

	LOCK_ATA;
	DBGWRT_1((DBH, "ATA::ReadBlocks: lsn=0x%x n=0x%x bs=%d\n",
			  lsn, nblks, blksz));

	/*--- validity checks ---*/
	if( h->incompatibleHw ){
		error = RBDRV_NOT_SUPPORTED;
		goto END;
	}

	if( blksz != ATA_BLOCK_SIZE ){
		error = RBDRV_BAD_PARAMETER;
		goto END;
	}
	if( subdev >= ATA_MAX_DRIVES ){
		error = RBDRV_BAD_SUBDEV;
		goto END;
	}

	error = TransferSecs( h, subdev, lsn, nblks, buf, ATA_DIR_IN );

 END:
	DBGWRT_1((DBH, "ATA::ReadBlocks: error=%d\n",error));
	UNLOCK_ATA;
	return error;
}

/**********************************************************************/
/** Write blocks to ATA drive
 *
 * \param _h		\IN ATA handle
 * \param subdev	\IN	sub device (0=master 1=slave IDE drive)
 * \param lsn		\IN logical sector number to access
 * \param nblks		\IN number of blocks to read
 * \param buf		\IN data buffer to write
 * \param blksz		\IN size of each sector (must be 512)
 * \param flags		\IN RBDRV_WRITE_VERIFY:	Reread each sector and verify data
 *
 * \return RBDRV error code
 */
static int32 WriteBlocks(
	RBDRV_HANDLE *_h,
	u_int32 subdev,
	u_int32 lsn,
    u_int32 nblks,
    u_int8  *buf,
    u_int32 blksz,
    u_int32 flags)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	int32 error;

	LOCK_ATA;
	DBGWRT_1((DBH, "ATA::WriteBlocks: lsn=0x%x n=0x%x bs=%d\n",
			  lsn, nblks, blksz));

	/*--- validity checks ---*/
	if( h->incompatibleHw ){
		error = RBDRV_NOT_SUPPORTED;
		goto END;
	}

	if( blksz != ATA_BLOCK_SIZE ){
		error = RBDRV_BAD_PARAMETER;
		goto END;
	}
	if( subdev >= ATA_MAX_DRIVES ){
		error = RBDRV_BAD_SUBDEV;
		goto END;
	}

	/*----------------+
    |  Write Sectors  |
    +----------------*/
	if( (error = TransferSecs( h, subdev, lsn, nblks, buf, ATA_DIR_OUT )) )
		goto END;

	/*----------------+
    |  Verify Sectors |
    +----------------*/

	/*
	 * Note: verify is done sector-by-sector. This may cause a big
	 * performance penalty on rotational disks.
	 */
	if( flags & RBDRV_WRITE_VERIFY ){
		u_int32 vlsn;
		u_int32 *p, *rp;
		u_int32 i;

		for( p=(u_int32 *)buf, vlsn=lsn; vlsn < lsn+nblks; vlsn++ ){

			if( (error = TransferSecs( h, subdev, vlsn, 1, h->buf,
									   ATA_DIR_IN )) )
				goto END;

			for(i=0, rp=(u_int32 *)h->buf; i<(blksz>>2); i++){
				if( *p++ != *rp++ ){
					DBGWRT_ERR((DBH,"*** ATA::WriteBlocks verify err:"
								"lsn=%d off=0x%x is=%08x sb=%08x\n",
								vlsn, i<<2, rp[-1], p[-1] ));
					error = RBDRV_VERIFY_ERROR;
					goto END;
				}
			}
		}
	}

 END:
	DBGWRT_1((DBH, "ATA::WriteBlocks: error=%d\n",error));
	UNLOCK_ATA;
	return error;
}

/**********************************************************************/
/** ATA driver I/O control
 *
 * Support ioctls:
 *	RBDRV_IOCTL_INFO: 	return standard RBDRV info block
 *
 *	ATA_IOCTL_INFO:   	return Identify block from ATA drive
 *
 *	ATA_IOCTL_GET_XFER	get PIO/DMA transfer mode
 * 		parms.p.data[0] (OUT): ata_regs.h: ATA_PIO_DEF_W ... ATA_DMA_ULTRA_2
 *
 *	ATA_IOCTL_GET_LBA	see if LBA mode is used on drive:
 *		parms.p.data[0] (OUT): 0=use CHS, 1=use LBA, 2=use LBA48
 *
 *  ATA_IOCTL_GET_DRV_STAT  get status of drive:
 *      parms           (OUT): ata2_drv.h: ATA2_DRIVE_ST_*
 *
 * \param _h			\IN ATA handle
 * \param function	\IN	IOCTL function code
 * \param parms		\INOUT parameters to IOCTL
 *
 * \return RBDRV error code
 */
static int32 Ioctl(
    RBDRV_HANDLE *_h,
	u_int32 function,
	RBDRV_IOCTL_PARMS *parms)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	int32 error=0;
	ATA_DRIVE *drv;
	ATA_IOCTL_PARMS *ataIoctl = (ATA_IOCTL_PARMS *)parms->p.data;

	DBGWRT_3((DBH, "ATA::Ioctl: subdev=0x%x  func=0x%x\n", parms->subdev, function));
	LOCK_ATA;

	if( parms->subdev >= ATA_MAX_DRIVES ){
		error = RBDRV_BAD_SUBDEV;
		goto END;
	}

	if( h->incompatibleHw &&
		(function != ATA_IOCTL_REGTEST) &&
		(function != ATA_IOCTL_GET_DRV_STAT) )
	{
		error = RBDRV_NOT_SUPPORTED;
		goto END;
	}

	drv = &h->drive[parms->subdev];

	if( !(drv->flags & DRV_PRESENT ) &&
		(function != ATA_IOCTL_GET_DRV_STAT) )
	{
		error = RBDRV_NOT_READY;
		goto END;
	}

	switch( function ){
	case RBDRV_IOCTL_INFO:
		parms->p.info.type			= RBDRV_IOCTL_TYPE_ATA;
		parms->p.info.phySectors 	= drv->nSectors;
		parms->p.info.logSectors 	= drv->nSectors;
		parms->p.info.sectSize 		= ATA_BLOCK_SIZE;
		parms->p.info.sectSizeAny 	= 0;
		parms->p.info.logOffset		= 0;
		parms->p.info.cylinders 	= drv->cylinders;
		parms->p.info.heads			= drv->heads;
		parms->p.info.sectorsTrack	= drv->sectorsTrack;
		break;

	case ATA_IOCTL_INFO:
		error = Identify( h, parms->subdev, &ataIoctl->info );
		break;

	case ATA_IOCTL_GET_XFER:
		parms->p.data[0] = drv->xfrMode;
		break;

	case ATA_IOCTL_GET_LBA:
		if( drv->flags & DRV_SUPPORT_LBA ){
			if( drv->flags & DRV_SUPPORT_LBA48 )
				parms->p.data[0] = 2;
			else
				parms->p.data[0] = 1;
		}
		else
			parms->p.data[0] = 0;
		break;

	case ATA_IOCTL_REGTEST:
		error = RegTest( h );
		break;

	case ATA_IOCTL_GET_DRV_STAT:
		*(u_int32*)parms->p.data = drv->state;
		break;

	default:
		error = RBDRV_NOT_SUPPORTED;
		break;
	}
END:
	DBGWRT_3((DBH, "ATA::Ioctl: finished error=%d\n", error));
	UNLOCK_ATA;
	return error;
}

/**********************************************************************/
/** Destroy ATA driver instance
 *
 * \param _h		\IN ATA handle
 * \return RBDRV error code
 */
static int32 Exit( RBDRV_HANDLE *_h )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	DBGWRT_1((DBH, "ATA::Exit\n"));

	if( h->desc.chipset->term )
		h->desc.chipset->term( _h ); /* terminate chipset */

	if( h->lockSem ) OSS_SemRemove( h->osh, &h->lockSem );
	if( h->irqSem )  OSS_SemRemove( h->osh, &h->irqSem );
	DBGEXIT((&DBH));

	/*--- free handle ---*/
	OSS_MemFree( h->osh, (char *)h, h->hGotSize);
	h = NULL;
	return 0;
}


static int32 NotImplemented()
{
	return RBDRV_NOT_SUPPORTED;
}

/**********************************************************************/
/** Check PRD table for correct alignment / set prdStartIndex
 *
 * \return return RBDRV error code
 */
static int32 PrdAlign( ATA_HANDLE_T *h )
{
	int32 error = 0;

	/*-------------------------------------+
    |  Check if prd table is 4-byte align  |
	+------------------------------------*/
	if((u_int32)(&(h->prdTable)) & 0x3) {
		DBGWRT_ERR((DBH,"*** ATA::Init: prd table (%08x) not 4-byte "
					"aligned ***\n",&(h->prdTable)));
		error = RBDRV_OUT_OF_RESOURCE;
		goto END;
	}
	/*
	 * Check if first 3 PRD entries would cross a 64K boundary;
	 * if so, use the index 3..5
	 */
	if( ((u_int32)&h->prdTable[0] & 0xffff0000) !=
		(((u_int32)&h->prdTable[ATA_PRD_TABLE_SIZE]-1) & 0xffff0000)){
		DBGWRT_2((DBH,"  PRD crosses 64K boundary. Use index 3..5\n"));
		h->prdStartIndex = ATA_PRD_TABLE_SIZE;
	}
	else
		h->prdStartIndex = 0;
 END:
	return error;
}


/**********************************************************************/
/** Create an instance of the ATA core driver
 *
 * This function must be called exactly once for each ATA controller
 * (i.e. for each master/slave combination). It creates, initializes
 * the handle and allocates the required resources.
 *
 * This function does NOT perform any HW access, especially does not
 * perform any ATA commands nor does it perform ATA register accesses.
 * However, the chipset init function is called that may access
 * chipset regs to detect the chipset.
 *
 * Actual drive startup must be performed later by calling ATA2_Startup()
 *
 * \param osh		  	\IN OSS handle
 * \param initDbgLevel	\IN initial debug level
 * \param desc			\IN ATA initialisation parameters
 *							(caller may destroyed this structure later)
 * \param handleP		\OUT receives created handle
 *
 * \return RBDRV error code
 */
int32 ATA2_Create(
    OSS_HANDLE *osh,
	u_int32    initDbgLevel,
	const ATA2_DESC_T *desc,
    ATA2_HANDLE_T **handleP)
{
	ATA_HANDLE_T *h;
	u_int32 hGotSize;
	int32 error;
	int32 subdev;

	ATA2_CHIPSET_ARG_T *chipsetArg;

	/*------------------+
    |  Allocate handle  |
    +------------------*/
	if( (h = (ATA_HANDLE_T *)
	   OSS_MemGet( osh, sizeof(ATA_HANDLE_T), &hGotSize )) == NULL )
		return RBDRV_OUT_OF_RESOURCE;

	OSS_MemFill( osh, sizeof(*h), (char*)h, 0 );

	h->osh = osh;
	h->hGotSize = hGotSize;

	*handleP = h;

	DBGINIT((NULL,&DBH));

	DBG_MYLEVEL = initDbgLevel;

	h->desc = *desc;				/* copy descriptor to handle */

	/* initialize ATA_DRIVE struct */
	for( subdev= 0; subdev < ATA_MAX_DRIVES; subdev++){
		h->drive[subdev].state = ATA2_DRIVE_ST_INIT;
	}

	DBGWRT_1((DBH,"ATA2_Create: base=%x/%x dmaBase %x drvMask=%x\n",
			  h->desc.maRegBlock[0], h->desc.maRegBlock[1], h->desc.maRegBlockDma,
			  h->desc.drvMask));

	/* Setup chipset */
	if( (error = h->desc.chipset->init( (ATA2_HANDLE_T *)h ))){
		DBGWRT_ERR((DBH,"*** ATA2_Create: failed to init chipset\n"));
		goto END;
	}


	/*----------------------+
    |  Prepare Dma PRDs     |
    +----------------------*/
	if( (error = PrdAlign( h )))
		goto END;

	/*--------------------+
    |  Create Semaphores  |
    +--------------------*/
	if( (error = OSS_SemCreate( osh, OSS_SEM_BIN, 1, &h->lockSem )) ){
		DBGWRT_ERR((DBH,"*** ATA2_Create: can't create lock sem err=%d ***\n",
					error ));
		error = RBDRV_OUT_OF_RESOURCE;
		goto END;
	}

	if( h->desc.useIrq ){
		if( (error = OSS_SemCreate( osh, OSS_SEM_BIN, 0, &h->irqSem )) ){
			DBGWRT_ERR((DBH,"*** ATA2_Create: can't create IRQ sem "
						"err=%d ***\n",	error ));
			error = RBDRV_OUT_OF_RESOURCE;
			goto END;
		}
	}


	/*--------------------------+
    |  Setup RB driver entries  |
    +--------------------------*/
    h->ent.Exit 		= Exit;
	h->ent.Ident		= Ident;
	h->ent.DbgLevelSet	= DbgLevelSet;
	h->ent.ReadBlocks	= ReadBlocks;
	h->ent.WriteBlocks	= WriteBlocks;
	h->ent.Ioctl		= Ioctl;
	h->ent.reserved1	= NotImplemented;
	h->ent.reserved2	= NotImplemented;
	h->ent.reserved3	= NotImplemented;
	h->ent.reserved4	= NotImplemented;


	/* check chipset arguments */
	chipsetArg = (ATA2_CHIPSET_ARG_T*)h->desc.chipsetArg;
	if( chipsetArg && chipsetArg->isRemovable ) {
		h->okRemovable = TRUE;
	}

 END:
	DBGWRT_1((DBH,"ATA2_Create: error=%d\n", error));

	if( error ) {
		Exit(&h->ent);
		*handleP = NULL;
	}

	return error;
}


/**********************************************************************/
/** Find out which addressing mode to use (CHS/LBA/LBA48)
 *
 * Based on the addressing mode, compute number of total sectors.
 * Compute also number of cylinders, heads and sectors/track
 *
 * set the h->drive[subdev] members:
 *  flags: (none) if CHS mode
 *		   DRV_SUPPORT_LBA if LBA mode
 *		   DRV_SUPPORT_LBA | DRV_SUPPORT_LBA48 if LBA48 mode
 *  heads:
 *  sectorsTrack:
 *  cylinders: computed based either from CHS geometry, from words 61:60 or
 *             words 103:100
 */
static int32 DetermineGeometryAddressingMode(
	ATA_HANDLE_T *h,
	int subdev,
	ATA_PARAM *parm )
{
	ATA_DRIVE *drv = &h->drive[subdev];
	u_int32 sectors6160	= parm->sectors0 + (parm->sectors1<<16);
	u_int32 chsSectors;
	const u_int32 thold = 16 * 63 * 16383;

	drv->sectorsTrack 	= parm->sectors;
	drv->heads			= parm->heads;
	drv->cylinders		= parm->cylinders;

	chsSectors 			= drv->sectorsTrack * drv->heads * drv->cylinders;
	drv->nSectors		= chsSectors;

	if( parm->capabilities & ATA_PARAM49_SUP_LBA ){
		if( sectors6160 >= thold ){
			drv->nSectors = sectors6160;
			drv->cylinders = drv->nSectors / (drv->sectorsTrack * drv->heads);
		}

		drv->flags |= DRV_SUPPORT_LBA;
		DBGWRT_2((DBH," ATA::Reset: LBA mode supported\n"));


		if( (parm->cmdSet1 & ATA_PARAM83_SUP_LBA48) &&
			(parm->cmdSetEnabled1 & ATA_PARAM86_SUP_LBA48)){
			/* drive supports LBA48 */
			if( (parm->maxLba48[2] != 0) || (parm->maxLba48[3] != 0)){

				/* use max. 32 bits for addressing */
				DBGWRT_ERR((DBH,"*** ATA2:: Huge drive limited to 2TByte!\n"));
				drv->nSectors = 0xFFFFFFFF;
			}
			else {
				drv->nSectors = parm->maxLba48[0] + (parm->maxLba48[1]<<16);
			}
			drv->flags |= DRV_SUPPORT_LBA48;
			drv->cylinders = drv->nSectors / (drv->sectorsTrack * drv->heads);
			DBGWRT_2((DBH," ATA::Reset: LBA48 mode supported\n"));
		}
	}
	DBGWRT_2((DBH," sd=%d chs %d/%d/%d nSect=%d, (60:%d)\n",
			  subdev, drv->cylinders, drv->heads, drv->sectorsTrack,
			  drv->nSectors, sectors6160 ));

	return 0;
}

/**********************************************************************/
/** Determine if drive is a CompactFlash
 *
 * \return TRUE if CF
 */
static int IsCompactFlash( ATA_HANDLE_T *h, ATA_PARAM *parm )
{
	/*
	 * Check for ATA flash cards. They will never have a slave
	 * and some cards even do not respond as a master once
	 * they have been addressed as a slave
	 * (Taken from Linux ide-probe.c/ide.c)
	 */
	if( (parm->config == 0x848a)/* CompactFlash */
		|| !_strncmp(parm->model, "KODAK ATA_FLASH", 15)	/* Kodak */
		|| !_strncmp(parm->model, "Hitachi CV", 10)		/* Hitachi */
		|| !_strncmp(parm->model, "SunDisk SDCFB", 13)	/* SunDisk */
		|| !_strncmp(parm->model, "SanDisk SDCFB", 13)	/* SunDisk */
		|| !_strncmp(parm->model, "HAGIWARA HPC", 12)	/* Hagiwara */
		|| !_strncmp(parm->model, "LEXAR ATA_FLASH", 15)	/* Lexar */
		|| !_strncmp(parm->model, "ATA_FLASH", 9))		/*Simple Tech*/
	{
		return TRUE;
	}
	return FALSE;
}

/**********************************************************************/
/** Determine maximum UDMA mode supported by drive
 *
 * \return ATA_DMA_ULTRA_xxx or 0 if no UDMA supported
 */
static int DriveMaxUdmaMode( ATA_HANDLE_T *h, ATA_PARAM *parm )
{
	int udmaMode = 0;

	if( parm->capabilities & ATA_PARAM49_SUP_UDMA ){
		if( parm->ultradma & 0x40 )
			udmaMode = ATA_DMA_ULTRA_6;
		else if( parm->ultradma & 0x20 )
			udmaMode = ATA_DMA_ULTRA_5;
		else if( parm->ultradma & 0x10 )
			udmaMode = ATA_DMA_ULTRA_4;
		else if( parm->ultradma & 0x8 )
			udmaMode = ATA_DMA_ULTRA_3;
		else if( parm->ultradma & 0x4 )
			udmaMode = ATA_DMA_ULTRA_2;
		else if( parm->ultradma & 0x2 )
			udmaMode = ATA_DMA_ULTRA_1;
		else if( parm->ultradma & 0x1 )
			udmaMode = ATA_DMA_ULTRA_0;
	}
	return udmaMode;
}

/**********************************************************************/
/** Determine maximum PIO mode supported by drive
 *
 * \return ATA_PIO_xxx
 */
static int DriveMaxPioMode( ATA_HANDLE_T *h, ATA_PARAM *parm )
{
	return ATA_PIO_DEF_W; 		/* ??? fill with life */
}


/**********************************************************************/
/** Set PIO or UDMA mode in drive using SET_FEATURES/SET_RW_MODE command
 *
 * \param xferMode	\IN one of ATA_PIO_xxx or ATA_DMA_ULTRA_x
 *
 * \return RBDRV error code
 */
static int32 TellDriveXferMode(
	ATA_HANDLE_T *h,
	int subdev,
	u_int8 xferMode)
{
	int32 error;
	ATA_TASK_T task;

	DBGWRT_2((DBH,"  drive %d: trying to set XFER mode 0x%02x\n", subdev,
			  xferMode ));

	OSS_MemFill( h->osh, sizeof(task), (char *)&task, 0 );

	task.protocol 			= ATA_PROTO_NON_DATA;
	task.command  			= ATA_CMD_SET_FEATURE;
	task.current.feature 	= ATA_SUB_SET_RWMODE;
	task.current.secCnt 	= xferMode;
	task.current.sdh		= ATA_SDH_IBM | (subdev<<4);

	error = Command( h, &task, 0, NULL, TOUT_READY, TOUT_CMD );

	if(error) {
		DBGWRT_ERR((DBH, "*** ATA2::failed to set XFER mode err=%d\n", error));
	}
	return error;
}

/**********************************************************************/
/** Setup chipset/drive/driver for optimum transfer modes
 *
 * \return RBDRV error code
 */
static int32 SetupTransferMode(
	ATA_HANDLE_T *h,
	int subdev,
	ATA_PARAM *parm )
{
	ATA_DRIVE *drv = &h->drive[subdev];
	int udmaMode, pioMode;
	int32 error = 0;

	/* determine max. modes supported by drive */
	pioMode  = DriveMaxPioMode( h, parm );
	udmaMode = DriveMaxUdmaMode( h, parm );

	DBGWRT_2((DBH, " SetupTransferMode: drive %d: PIO=0x%02x DMA=0x%02x\n",
			  subdev, pioMode, udmaMode ));

	/* don't use UDMA if user doesn't want it */
	if( ! (h->desc.drvMask &
		   (subdev ? ATA2_DRVMASK_SLAVE_UDMA : ATA2_DRVMASK_MASTER_UDMA))){
		udmaMode = 0;
		DBGWRT_2((DBH, " SetupTransferMode: UDMA disabled by user\n"));
	}

	/* limit maximum modes supported by chipset, setup chipset */
	if( h->desc.chipset->setMode != NULL ){
		error = h->desc.chipset->setMode( h, subdev, &pioMode, &udmaMode );
	}
	else {
		/* no chipset support */
		pioMode = ATA_PIO_DEF_W;
		udmaMode = 0;
	}
	DBGWRT_2((DBH, " SetupTransferMode: chipset/drive %d: "
			  "PIO=0x%02x DMA=0x%02x error=%d\n",
			  subdev, pioMode, udmaMode, error ));

	if( error )
		goto END;

	/* setup PIO mode */
	if( (error = TellDriveXferMode( h, subdev, pioMode )))
		goto END;
	drv->xfrMode = pioMode;

	if( udmaMode != 0 ){
		/* setup UDMA mode */
		if( (error = TellDriveXferMode( h, subdev, udmaMode ))){
			DBGWRT_ERR((DBH, "*** ATA2: Drive doesn't accept UDMA mode!\n"));
			error = 0;
			goto END;
		}
		drv->flags |= DRV_SUPPORT_UDMA;
		drv->xfrMode = udmaMode;
	}

 END:
	return error;
}

/**********************************************************************/
/** Initialize Drive for operation
 *
 * Based on parameters from IDENTIFY command,
 * - Determine drive geometry
 * - Determine addressing mode
 * - Setup drive structure with the geometry parameters
 * - Set max. possible PIO mode supported by chipset and drive
 * - Set max. possible UDMA mode supported by chipset and drive
 *
 * \return return RBDRV error code
 */
static int32 DriveInit( ATA_HANDLE_T *h, int subdev )
{
	ATA_DRIVE *drv = &h->drive[subdev];
	ATA_PARAM *parms = (ATA_PARAM *)h->buf;
	int32 error;

	DBGWRT_1((DBH, "ATA2: DriveInit for drive %d\n", subdev ));

	/*--- Perform IDENTIFY command ---*/
	if( (error = Identify( h, subdev, parms )) != 0 ){
		DBGWRT_ERR((DBH, "*** ATA2: Identify failed for drive %d\n", subdev ));
		goto END;
	}

	/*--- Identification ok, read parameters ---*/
	drv->flags = DRV_PRESENT;

	/*--- Determine geometry and addressing mode ---*/
	DetermineGeometryAddressingMode( h, subdev, parms );

	/*--- test if drive is a CompactFlash ---*/
	if( IsCompactFlash( h, parms ) )
		drv->flags |= DRV_IS_FLASH;

	/* for some CFs the SetupTransferMode fails even for PIO mode 0
	 * drives still work. So for now, set ok earlier */
	drv->state = ATA2_DRIVE_ST_OK;

	/*--- setup optimum transfer modes ---*/
	if( (error = SetupTransferMode( h, subdev, parms ))){
		DBGWRT_ERR((DBH, "*** ATA2: SetupTransferMode failed for drive %d\n",
					subdev ));
		goto END;
	}

	drv->state = ATA2_DRIVE_ST_OK;

 END:
	DBGWRT_1((DBH, "ATA2: DriveInit exit for drive %d\n", subdev ));
	return error;
}

/**********************************************************************/
/** Startup both drives (master and slave)
 *
 * Must be called after ATA2_Create() before other functions such
 * as ReadBlocks()/WriteBlocks() and Ioctl() are called.
 *
 * This function can be called more than once.
 *
 * - reset controller
 * - Perform ATA register test
 * - Perform Device Diagnostics
 * - for both drives
 *   - Init drive
 *
 * \return return RBDRV error code
 *
 * No error is returned when drives are not present
 */
int32 ATA2_StartupDrives( ATA2_HANDLE_T *_h )
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	int32 error;
	u_int8 diagStatus;
	int subdev;

	DBGWRT_1((DBH,"ATA_StartupDrives drvMask=0x%x\n", h->desc.drvMask));

	/* in case of insertion, some CFs generate IRQs on power on!!
	 * clear IRQ semaphore after stray interrupts */
	OSS_SemWait(h->osh, h->irqSem, OSS_SEM_NOWAIT );

	LOCK_ATA;

	/* set drives to not present */
	for( subdev=0; subdev<ATA_MAX_DRIVES; subdev++ ){
		ATA_DRIVE *drv = &h->drive[subdev];
		drv->state = ATA2_DRIVE_ST_NONE;
	}


	/*------------------------+
    |  Reset ATA controller   |
    +------------------------*/
	if( (error = DoReset(h)) ){
		DBGWRT_ERR((DBH,"ATA_StartupDrives DoReset() failed\n"));
		goto END;
	}

	/*----------------------------+
    |  Perform ATA register test  |
    +----------------------------*/
    h->incompatibleHw = FALSE; /* in case of retry give it a chance */
	if( (error = RegTest(h))){
		h->incompatibleHw = TRUE;
		DBGWRT_ERR((DBH,"ATA_StartupDrives RegTest() failed\n"));
		goto END;
	}
	/*-----------------------------+
    |  Execute Device Diagnostics  |
    +-----------------------------*/
	if( (error = Diagnose( h, &diagStatus )) ){
		DBGWRT_ERR((DBH,"ATA_StartupDrives Diagnose() failed\n"));
		goto END;
	}

	/*-------------------------------------+
    |  Init drive                          |
    +-------------------------------------*/
	for( subdev=0; subdev<ATA_MAX_DRIVES; subdev++ ){
		ATA_DRIVE *drv = &h->drive[subdev];

		drv->subDev = subdev;

		if( !(h->desc.drvMask &
			  (subdev ? ATA2_DRVMASK_SLAVE : ATA2_DRVMASK_MASTER )))
			continue;

		DBGWRT_3((DBH,"ATA_StartupDrives Init drive %d\n", subdev));

		if( (error = DriveInit( h, subdev )))
			/* ignore error */
			error = 0;

		if( !h->desc.allowSlaveCf && (drv->flags & DRV_IS_FLASH) ){
			DBGWRT_2((DBH," CompactFlash detected. Don't look for "
					  "slave\n"));
			break;				/* don't look for slave on compactFlash */
		}
	}

	if( (error = WaitBusyLow( h, TOUT_READY )) )
		goto END;

	ATA_WRITE_D8( h, 0, h->r.sdh, ATA_SDH_IBM );

 END:
	UNLOCK_ATA;
	return error;
}

#ifdef DBG
/**********************************************************************/
/** Dumps ATA register in debug mode
 */
static void RegDump( ATA_HANDLE_T *h, int line )
{
	DBGWRT_3((DBH,"\nATA REG DUMP: from line %d\n", line));
	DBGWRT_3((DBH,"  error: %02x sector: %02x seccnt: %02x cylHi: %02x\n",
			  ATA_READ_D8(h, 0, h->r.error),
			  ATA_READ_D8(h, 0, h->r.sector),
			  ATA_READ_D8(h, 0, h->r.seccnt),
			  ATA_READ_D8(h, 0, h->r.cylHi)));
	DBGWRT_3((DBH," cylLo: %02x sdh: %02x altStat: %02x addr: "
			  "%02x\n",
			  ATA_READ_D8(h, 0, h->r.cylLo),
			  ATA_READ_D8(h, 0, h->r.sdh),
			  ATA_READ_D8(h, 1, h->r.altStatus),
			  ATA_READ_D8(h, 1, h->r.drvAdr)
			  ));
}
#endif /* DBG */
#ifndef ATA_UNITTEST_SIMULATED_COMMAND_FUNC

/**********************************************************************/
/** Check if ATA registers can be correctly accessed
 *
 * Perform a write/read test to the ATA registers that can be written
 * without side effects.
 *
 * \return 0 or RBDRV_HW_DAMAGE
 */
static int32 RegTest( ATA_HANDLE_T *h )
{
	int loop;
	u_int8 rv, sb;
	int errorCounter = 0;
	DBGCMD( const char errstr[] = "***ATA::Regtest: "; );

	DBGWRT_1((DBH,"ATA::RegTest!\n"));

	for(loop=0; loop<50; loop++ ){
		ATA_WRITE_D8(h, 0, h->r.sector, loop + 0xb8);
		ATA_WRITE_D8(h, 0, h->r.cylLo, 	loop + 0x2f);
		ATA_WRITE_D8(h, 0, h->r.cylHi, 	loop + 0x3c);
		ATA_WRITE_D8(h, 0, h->r.seccnt, loop + 0x0a);

		rv = ATA_READ_D8(h, 0, h->r.sector);
		sb = loop + 0xb8;
		if( rv != sb ){
			DBGWRT_ERR((DBH,"%sERR SECTOR loop=%d rv=%02x sb=%02x\n",
						errstr, loop, rv, sb ));
			errorCounter++;
		}
		rv = ATA_READ_D8(h, 0, h->r.cylLo);
		sb = loop + 0x2f;
		if( rv != sb ){
			DBGWRT_ERR((DBH,"%sERR CYLLO loop=%d rv=%02x sb=%02x\n",
						errstr, loop, rv, sb ));
			errorCounter++;
		}
		rv = ATA_READ_D8(h, 0, h->r.cylHi);
		sb = loop + 0x3c;
		if( rv != sb ){
			DBGWRT_ERR((DBH,"%sERR CYLHI loop=%d rv=%02x sb=%02x\n",
						errstr, loop, rv, sb ));
			errorCounter++;
		}
		rv = ATA_READ_D8(h, 0, h->r.seccnt);
		sb = loop + 0x0a;
		if( rv != sb ){
			DBGWRT_ERR((DBH,"%sERR SECCNT loop=%d rv=%02x sb=%02x\n",
						errstr, loop, rv, sb ));
			errorCounter++;
		}

        if( errorCounter > 10 )
		{
			DBGWRT_ERR((DBH," too many errors - break\n" ));
			break;
        }

	}
	DBGWRT_1((DBH,"ATA::RegTest: finished %d errors\n", errorCounter));

	return errorCounter ? RBDRV_HW_DAMAGE : 0;
}

/**********************************************************************/
/** Wait for drive not busy or bogus status
 *
 * \return 0 or RBDRV_HW_DAMAGE
 */
static int32 ResetWait( ATA_HANDLE_T *h, u_int32 seconds )
{
	u_int32 i;
	int32	error = 0;
	u_int8  status;

	i = seconds * 2;

    while( (status = (ATA_READ_D8(h, 0, h->r.status))) & ATA_STAT_BUSY){
		if( status & ATA_STAT_DRQ ){
			/* bogus status, exit (no drive attached...) */
			DBGWRT_ERR((DBH,"*** ATA::ResetWait bogus status 0x%02x\n", status));
			error = RBDRV_HW_DAMAGE;
			goto EXIT;
		}
		OSS_Delay(h->osh, 500 );
		if(--i == 0 ){
			DBGWRT_ERR((DBH,"*** ATA::ResetWait Pre-Timeout\n"));
			error = RBDRV_HW_DAMAGE;
			goto EXIT;
		}
		DBGWRT_3((DBH,"Status=%02x\n", status ));
	}
 EXIT:
	return error;
}

/**********************************************************************/
/** Perform ATA soft reset
 *
 * Wait until drive ready, perform ATA reset and wait until again ready
 *
 * \return 0 or RBDRV_HW_DAMAGE
 */
static int32 DoReset( ATA_HANDLE_T *h )
{
	int32	error = 0;

	DBGWRT_1((DBH,"ATA::DoReset\n"));

	if( (error = WaitBusyLow( h, TOUT_READY )) )
		goto EXIT;

	ATA_WRITE_D8( h, 0, h->r.sdh, 0 );

	/*--- device must release busy after 31 secs ---*/
	if( (error = ResetWait( h, h->desc.toutReset)) != 0 )
		goto EXIT;

	/* reset device */
	ATA_WRITE_D8( h, 1, h->r.devControl,
				  ATA_CTL_4BIT | ATA_CTL_RST | ATA_CTL_IDS);
	OSS_Delay(h->osh, 100);

	ATA_WRITE_D8( h, 1, h->r.devControl, ATA_CTL_4BIT | ATA_CTL_IDS);
	OSS_Delay(h->osh, 100);

	/*--- device must release busy after 31 secs ---*/
	if( (error = ResetWait( h, h->desc.toutReset)) != 0 )
		goto EXIT;

	if( h->desc.useIrq )
		/*--- enable interrupts ---*/
		ATA_WRITE_D8( h, 1, h->r.devControl, ATA_CTL_4BIT);

 EXIT:
	DBGWRT_1((DBH,"ATA::DoReset: error=%d\n",error));

	return error;
}
#endif /*  ATA_UNITTEST_SIMULATED_COMMAND_FUNC */

/**********************************************************************/
/** Setup task file regs to to Disk r/w
 *
 * \param h			\IN	ATA handle
 * \param drv		\IN drive object
 * \param lsn		\IN logical sector number to setup
 * \param nSectorsP	\IN	max. number of sectors to transfer
 *					\IN actual number of sectors to transfers
 * \param task		\IN register task file to setup. All members are setup
 *						except command, protocol
 *
 * \return RBDRV error code
 */
static int32 DiskRwTaskFileSetup(
	ATA_HANDLE_T *h,
	ATA_DRIVE *drv,
	u_int32 lsn,
	u_int32 *nSectorsP,
	ATA_TASK_T *task )
{
	u_int32 nSectors = *nSectorsP;
	int32 rv = 0;

	/* clear task */
	OSS_MemFill( h->osh, sizeof(*task), (char*)task, 0 );

	if (!(drv->flags & DRV_SUPPORT_LBA))
	{
		/* CHS mode */
		u_int32 cylinder, track, head, sector;

		cylinder = lsn / (drv->sectorsTrack * drv->heads);
		task->current.lbaMid 	= cylinder & 0xff; 			/* cylLo */
		task->current.lbaHigh 	= (cylinder & 0xff00) >> 8;	/* cylHi */

		track    = lsn % (drv->sectorsTrack * drv->heads);
		head     = track / drv->sectorsTrack;
		sector 	 = track % drv->sectorsTrack + 1;

		task->current.lbaLow  = sector;	/* sector */

		/* transfer at max. to end of track */
		if( nSectors > drv->sectorsTrack - sector + 1 )
			nSectors = drv->sectorsTrack - sector + 1;

		task->current.sdh = ATA_SDH_IBM | (drv->subDev<<4) | head;
		task->current.secCnt = nSectors;

		if( (cylinder > 0xffff) || (head > 0xf ) || (nSectors > 0xff)){
			rv = RBDRV_BAD_PARAMETER;
			goto END;
		}

		DBGWRT_2((DBH,"  c/h/s = %d/%d/%d blks=%d\n",
				  cylinder, head, sector, nSectors ));
	}
	else /* LBA mode */
	{
		if (!(drv->flags & DRV_SUPPORT_LBA48)){
			/* LBA28 */

			task->current.lbaLow = (lsn & 0x000000FF);
			task->current.lbaMid = (lsn & 0x0000FF00)>>8;
			task->current.lbaHigh = (lsn & 0x00FF0000)>>16;
			task->current.sdh = ATA_SDH_LBA | (drv->subDev<<4) |
				((lsn & 0x0F000000)>>24);

			if( nSectors > 0xFF )
				nSectors = 0x100; /* 0 means 0x100 */

			task->current.secCnt = nSectors;

			if( lsn > 0x0FFFFFFF ){
				rv = RBDRV_BAD_PARAMETER;
				goto END;
			}
			DBGWRT_2((DBH,"  (LBA)%d blks=%d\n", lsn, nSectors ));
		}
		else {
			/* LBA48 (we use only 32 of them) */
			task->useLba48 = TRUE;
			task->current.lbaLow 	= (lsn & 0x000000FF);
			task->current.lbaMid 	= (lsn & 0x0000FF00)>>8;
			task->current.lbaHigh 	= (lsn & 0x00FF0000)>>16;
			task->previous.lbaLow 	= (lsn & 0xFF000000)>>24;
			task->current.sdh = ATA_SDH_LBA | (drv->subDev<<4);

			/*
			 * this driver uses a max. transfer count of 256 sectors
			 * due to limited number of PRDs
			 */
			if( nSectors > 0xFF )
				nSectors = 0x100;

			task->current.secCnt 	= (nSectors & 0x00FF);
			task->previous.secCnt 	= (nSectors & 0xFF00)>>8;

			DBGWRT_2((DBH,"  (LBA48)%d blks=%d\n", lsn, nSectors ));
		}
	}
 END:
	if( rv ){
		DBGWRT_ERR((DBH,"*** ATA::DiskRwTaskFileSetup: LSN %d out of range\n",
					lsn ));
	}
	*nSectorsP = nSectors;
	return rv;
}

/**********************************************************************/
/** Determine ATA command and protocol to use for Disk r/w operation
 *
 * \param h			\IN	ATA handle
 * \param drv		\IN drive object
 * \param buffer	\IN RAM buffer address. Used to check if buffer is 16-bit
 *						aligned.
 * \param direction	\IN 0=read 1=write
 * \param protocolP \OUT transfer procotol to use
 *
 * \return command to use
 */
static u_int8 DiskRwCommandGet(
	ATA_HANDLE_T *h,
	ATA_DRIVE *drv,
	u_int8 *buffer,
	ATA_DIRECTION direction,
	ATA_PROTOCOL *protocolP)
{
	int useDma = !!(drv->flags & DRV_SUPPORT_UDMA);
	u_int8 command = 0;
	ATA_PROTOCOL protocol = ATA_PROTO_PIO_DATA_IN;

	if( (u_int32)buffer & 0x1)
		useDma = FALSE;			/* odd address, use PIO mode */

	if( useDma ){

		if( direction == ATA_DIR_OUT ){
			protocol = ATA_PROTO_DMA_OUT;

			if( drv->flags & DRV_SUPPORT_LBA48)
				command = ATA_CMD_WRITE_DMA_EXT;
			else
				command = ATA_CMD_WRITE_DMA;

		}
		else {
			protocol = ATA_PROTO_DMA_IN;

			if( drv->flags & DRV_SUPPORT_LBA48)
				command = ATA_CMD_READ_DMA_EXT;
			else
				command = ATA_CMD_READ_DMA;
		}
	}
	else {
		/* non-DMA */
		if( direction == ATA_DIR_OUT ){
			protocol = ATA_PROTO_PIO_DATA_OUT;

			if( drv->flags & DRV_SUPPORT_LBA48)
				command = ATA_CMD_WRITE_EXT;
			else
				command = ATA_CMD_WRITE;
		}
		else {
			protocol = ATA_PROTO_PIO_DATA_IN;

			if( drv->flags & DRV_SUPPORT_LBA48)
				command = ATA_CMD_READ_EXT;
			else
				command = ATA_CMD_READ;
		}
	}
	*protocolP = protocol;
	return command;
}

/**********************************************************************/
/** Setup PRD table for DMA
 *
 * \param h			\IN	ATA handle
 * \param buffer	\IN RAM buffer start address
 * \param nSectors 	\IN number of sectors to transfer
 *
 * \return RBDRV error code (if buffer exceeds max entries in PRD table)
 */
static int32 PrdSetup(
	ATA_HANDLE_T *h,
	u_int8 *buffer,
	u_int32 nSectors)
{
	u_int32 currentBuffer, nextBuffer;
	u_int32 count;
	int prdIndex = h->prdStartIndex, prdEndIndex;
	int32 error = 0;

	prdEndIndex = prdIndex + ATA_PRD_TABLE_SIZE;

	count = ATA_BLOCK_SIZE * nSectors;
	nextBuffer = (u_int32)buffer;
	DBGWRT_2((DBH,"  prdTable: %08x, buffer: %x, count: %x\n",
			  &(h->prdTable), buffer, count));

	/* split the buffer into 64k segments for PRD */
	do {
		if( prdIndex >= prdEndIndex ) {
			DBGWRT_ERR((DBH,"ATA::TransferSecs: not enough space "
					  "in prd table\n"));
			error = RBDRV_OUT_OF_RESOURCE;
			goto END;
		}
		currentBuffer = nextBuffer;
		nextBuffer = (currentBuffer & 0xFFFF0000) + 0x10000;
		h->prdTable[prdIndex].addr = (MACCESS)SWAP_LE32(currentBuffer);
		if((currentBuffer & 0xffff0000) == ((currentBuffer + count)
											& 0xffff0000)) {
			h->prdTable[prdIndex].count = SWAP_LE16(count);
			count = 0;
		} else {
			h->prdTable[prdIndex].count =
				SWAP_LE16((u_int16)(nextBuffer - currentBuffer));

			count -= (nextBuffer - currentBuffer);
		}
		h->prdTable[prdIndex].eot = (count==0) ?
			SWAP_LE16(0x8000) : SWAP_LE16(0x0000);

		DBGWRT_2((DBH,"  count: %04x, addr: %08x, eot: %04x\n",
				  h->prdTable[prdIndex].count,
				  h->prdTable[prdIndex].addr, h->prdTable[prdIndex].eot));
		prdIndex++;
	} while (count > 0);
 END:
	return error;
}

/**********************************************************************/
/** Read or Write Sectors on ATA drive
 *
 * \param h			\IN	ATA handle
 * \param subdev	\IN 0=master, 1=slave
 * \param lsn		\IN logical sector number on drive
 * \param nBlocks	\IN number of sectors to transfer
 * \param buffer	\INOUT buffer to read/write data
 * \param direction	\IN	ATA_DIR_IN or ATA_DIR_OUT
 *
 * \return RBDRV error code
 */
static int32 TransferSecs(
    ATA_HANDLE_T *h,
    int subdev,
    u_int32 lsn,
    u_int32 nBlocks,
    u_int8 *buffer,
    ATA_DIRECTION direction)
{
	u_int32 curBlocks;
	int32 retry, error = 0;
	ATA_DRIVE *drv = &h->drive[subdev];
	ATA_PROTOCOL usedProtocol;
	u_int8 usedCommand;

	DBGWRT_2((DBH,"ATA::TransferSecs: sd=%d lsn=%d nBlks=%d dir=%s\n",
			  subdev, lsn, nBlocks, direction == ATA_DIR_OUT ? "W" : "R" ));

	drv = &h->drive[subdev];
	if( !(drv->flags & DRV_PRESENT) ){
		error = RBDRV_NOT_READY;
		goto END;
	}

	/*--- check if sector number in range ---*/
	if( lsn+nBlocks > drv->nSectors ){
		error = RBDRV_ILLEGAL_SECTOR;
		goto END;
	}

	/* determine command/protocol */
	usedCommand = DiskRwCommandGet( h, drv, buffer, direction,
									&usedProtocol );

	/*-------------------+
    |  Transfer Blocks   |
    +-------------------*/
	while( nBlocks > 0 ){
		ATA_TASK_T task;

		curBlocks = nBlocks;

		/* compute ATA regs (CHS or LBA) for transfer */
		if( (error = DiskRwTaskFileSetup( h, drv, lsn, &curBlocks, &task )))
			goto END;

		task.command = usedCommand;
		task.protocol = usedProtocol;

		/* setup DMA physical region descriptors */
		if ( (usedProtocol == ATA_PROTO_DMA_IN) ||
			 (usedProtocol == ATA_PROTO_DMA_OUT)) {

			if( (error = PrdSetup( h, buffer, curBlocks )))
				goto END;
		}

		retry = h->desc.retry;
		error = 1;

		while( retry && error ){

			/*--- issue READ/WRITE sectors command (maybe with DMA) ---*/
			error = Command( h, &task, curBlocks, buffer,
							 TOUT_READY, TOUT_CMD );

			/*
			 * Note: for rotating disks, we should possibly better
			 * perform recalibrate/seek commands here
			 */
			if( error == RBDRV_HW_DAMAGE )
				DoReset(h);

			retry--;
		}
		if( error )
			goto END;

		if( direction == ATA_DIR_IN ){
			DBGDMP_4((DBH,"Read Data", buffer, 512, 2 ));
		}

		buffer += curBlocks * ATA_BLOCK_SIZE;
		lsn += curBlocks;
		nBlocks -= curBlocks;
	}
 END:
	DBGWRT_2((DBH,"ATA::TransferSecs: finished error=%d\n", error ));
	return error;
}

/**********************************************************************/
/** Perform Diagnostic command on ATA drive
 *
 * \param h				\IN	ATA handle
 * \param diagStatusP 	\OUT receives diag status
 * \return RBDRV error code
 */
static int32 Diagnose( ATA_HANDLE_T *h, u_int8 *diagStatusP )
{
	int32 retry=1, rv=0;
	ATA_TASK_T task;

	DBGWRT_1((DBH,"ATA::Diagnose\n"));
	/*DBGCMD( RegDump(h, __LINE__); )*/

	OSS_MemFill( h->osh, sizeof(task), (char *)&task, 0 );
	task.protocol 			= ATA_PROTO_NON_DATA;
	task.command  			= ATA_CMD_DIAGNOSE;
	task.current.sdh		= ATA_SDH_IBM;

	while( retry-- ){
		if( (rv = Command( h, &task, 0, NULL, TOUT_READY, TOUT_CMD )))
			continue;

		*diagStatusP = ATA_READ_D8( h, 0, h->r.error );
		DBGWRT_1((DBH,"Diag Status = %02x\n", *diagStatusP ));
		break;
	}
	DBGWRT_1((DBH,"ATA::Diagnose: rv=%d\n", rv));

	return rv;
}

/**********************************************************************/
/** Perform Identify command on ATA drive
 *
 * \param h				\IN	ATA handle
 * \param subdev		\IN 0=master, 1=slave
 * \param parm			\OUT receives identification data
 *
 * \return RBDRV error code
 */
static int32 Identify( ATA_HANDLE_T *h, int subdev, ATA_PARAM *parm )
{
	int32 rv;
	ATA_TASK_T task;
#ifdef BIG_ENDIAN
	int32 i;
	u_int16 *p;
#endif /*BIG_ENDIAN*/

	OSS_MemFill( h->osh, sizeof(task), (char *)&task, 0 );

	DBGWRT_1((DBH,"ATA::Identify: sd=%d\n", subdev));

	task.protocol 			= ATA_PROTO_PIO_DATA_IN;
	task.command  			= ATA_CMD_IDENTIFY;
	task.current.sdh		= ATA_SDH_IBM | (subdev<<4);

	if( (rv = Command( h, &task, 1, (u_int8 *)parm, 1000, 1000 )))
		goto END;

#ifdef BIG_ENDIAN
	/*--- swap all words in parameter block ---*/
	for(i=0, p=(u_int16 *)parm; i<256; i++, p++ )
		*p = SWAP_LE16( *p );
#endif

	DBGDMP_4((DBH,"Identify",(void *)parm, sizeof(*parm), 2 ));
	DBGWRT_1((DBH,"ATA::Identify: Model=%s\n", parm->model ));

END:
	return rv;
}

/**********************************************************************/
/** Write command task file registers to HW, except command and sdh reg
 *
 * \param h			\IN	ATA handle
 * \param task		\IN register task file to use for command.
 *
 */
void TaskToHw( ATA_HANDLE_T *h, const ATA_TASK_T *task )
{
	if( task->useLba48 ){
		ATA_WRITE_D8( h, 0, h->r.feature, task->previous.feature );
		ATA_WRITE_D8( h, 0, h->r.seccnt,  task->previous.secCnt);
		ATA_WRITE_D8( h, 0, h->r.sector,  task->previous.lbaLow	);
		ATA_WRITE_D8( h, 0, h->r.cylLo,   task->previous.lbaMid );
		ATA_WRITE_D8( h, 0, h->r.cylHi,   task->previous.lbaHigh);
	}
	ATA_WRITE_D8( h, 0, h->r.feature, task->current.feature );
	ATA_WRITE_D8( h, 0, h->r.seccnt,  task->current.secCnt);
	ATA_WRITE_D8( h, 0, h->r.sector,  task->current.lbaLow	);
	ATA_WRITE_D8( h, 0, h->r.cylLo,   task->current.lbaMid );
	ATA_WRITE_D8( h, 0, h->r.cylHi,   task->current.lbaHigh);
}

#ifndef ATA_UNITTEST_SIMULATED_COMMAND_FUNC
/**********************************************************************/
/** Execute ATA command, wait until completed
 *
 * \param h			\IN	ATA handle
 * \param task		\IN register task file to use for command. Includes
 *						protocol type, command and all other ATA registers
 * \param nBlocks	\IN number of 512 Byte blocks to transfer.
 *						Ignored if protocol is ATA_PROTO_NON_DATA
 * \param buffer    \IN buffer to read/write data. Ignored in
 *						PROTO_NON_DATA
 * \param setupTo	\IN time in ms to wait until ready for command
 * \param cmdTo		\IN time in ms to wait until command finished
 *
 * \return RBDRV error code
 *
 */
static int32 Command(
	ATA_HANDLE_T *h,
	const ATA_TASK_T *task,
	u_int32 nBlocks,
	u_int8 *buffer,
    u_int32 setupTo,
    u_int32 cmdTo)
{
	int32 error;
	u_int32 i;
	u_int8 bmIdeCommand;

	DBGWRT_2((DBH,"ATA::Command: cmd=0x%02x proto=%d nBlocks=%d\n",
			  task->command, task->protocol, nBlocks));
	DBGWRT_2((DBH,"f=%02x sc=%02x sec=%02x cyl=%02x/%02x sdh=%02x\n",
			  task->current.feature, task->current.secCnt,
			  task->current.lbaLow, task->current.lbaMid,
			  task->current.lbaHigh, task->current.sdh ));

	/*--- prepare for command, select device ---*/
	if( (error = PrepareCmd( h, task->current.sdh, setupTo )) ){
		DBGCMD( RegDump(h, __LINE__); )
		goto END;
	}

	/*--- transfer task file regs to ATA regs ---*/
	TaskToHw( h, task );

	DBGCMD( RegDump(h, __LINE__); )
	/*--------------------------+
    |  Transfer block by block  |
    +--------------------------*/
	error = -1;
	switch( task->protocol ){

	case ATA_PROTO_DMA_IN:
	case ATA_PROTO_DMA_OUT:
		ATA_BMIDE_WRITE_D32( h, h->r.bmIdePrdTable,
							 SWAP_LE32((u_int32)(
										   &h->prdTable[h->prdStartIndex])));

		bmIdeCommand = ATA_BMIDE_READ_D8( h, h->r.bmIdeCommand ) & ~0x01; /* dont care for PBM enable bit yet */
		if( task->protocol == ATA_PROTO_DMA_IN )
			bmIdeCommand |= 0x08;	/* ATA dev to system memory */
		else
			bmIdeCommand &= ~0x08;	/* system memory to ATA dev */

		ATA_BMIDE_WRITE_D8( h, h->r.bmIdeCommand, bmIdeCommand );

		ATA_BMIDE_WRITE_D8( h, h->r.bmIdeStatus,
							ATA_BMIDE_READ_D8( h, h->r.bmIdeStatus ));

		/*--- writing command register starts command ---*/
		ATA_WRITE_D8( h, 0, h->r.command, task->command );

		/* start DMA */
		ATA_BMIDE_WRITE_D8( h, h->r.bmIdeCommand, bmIdeCommand | 0x01 );

		OSS_MikroDelay(h->osh, 1);
		/*--- wait for command completion ---*/
		error = WaitInterrupt( h, cmdTo );

		ATA_BMIDE_WRITE_D8( h, h->r.bmIdeCommand, bmIdeCommand );/* be sure to stop DMA */

		DBGWRT_3((DBH,"finished DMA reg: stat=%02x cmd=%02x\n",
				  ATA_BMIDE_READ_D8( h, h->r.bmIdeStatus ),
				  ATA_BMIDE_READ_D8( h, h->r.bmIdeCommand )));
		DBGCMD( RegDump(h, __LINE__); )

		if( error )
			break;

		if( h->intStatus & ATA_STAT_ERR ){
			DBGWRT_ERR((DBH,"*** ATA::Command: cmd=%02x error st=%02x "
						"err=%02x\n", task->command, h->intStatus,
						ATA_READ_D8( h, 0, h->r.error)));
			error = RBDRV_READ_ERROR;
		}
		break;

	case ATA_PROTO_PIO_DATA_IN:
		/*--- writing command register starts command ---*/
		ATA_WRITE_D8( h, 0, h->r.command, task->command );

		for( i=0; i<nBlocks; i++ ){
			/*--- wait for command completion ---*/
			if( (error = WaitInterrupt( h, cmdTo )) )
				break;

			if( h->intStatus & ATA_STAT_DRQ ){
				/*--- read data from ATA ---*/
				ATA_READ_STRING_D16(h, 0, h->r.data, (u_int16 *)buffer,
									ATA_BLOCK_SIZE );
			}
			else {
				DBGWRT_ERR((DBH,"*** ATA::Command: pioIn cmd=%02x no DRQ\n",
							task->command));
				error = RBDRV_HW_DAMAGE;
				break;
			}
			buffer += ATA_BLOCK_SIZE;

			if( h->intStatus & ATA_STAT_ERR ){
				DBGWRT_ERR((DBH,"*** ATA::Command: cmd=%02x error st=%02x "
							"err=%02x\n", task->command, h->intStatus,
							ATA_READ_D8( h, 0, h->r.error)));
				error = RBDRV_READ_ERROR;
				break;
			}
		}
		break;

	case ATA_PROTO_PIO_DATA_OUT:
		/*--- writing command register starts command ---*/
		ATA_WRITE_D8( h, 0, h->r.command, task->command );

		for( i=0; i<nBlocks; i++ ){

			/*--- wait until busy low ---*/
			if( (error = WaitBusyLow( h, cmdTo )) )
				break;

			if( ATA_READ_D8( h, 0, h->r.status) & ATA_STAT_DRQ ){
				/*--- write data to ATA ---*/
				ATA_WRITE_STRING_D16(h, 0, h->r.data, (u_int16 *)buffer,
									 ATA_BLOCK_SIZE );
			}
			else {
				DBGWRT_ERR((DBH,"*** ATA::Command: pioOut cmd=%02x no DRQ\n",
							task->command));
				error = RBDRV_HW_DAMAGE;
				break;
			}
			buffer += ATA_BLOCK_SIZE;

			/*--- wait for command completion ---*/
			if( (error = WaitInterrupt( h, cmdTo )) )
				break;

			if( h->intStatus & ATA_STAT_ERR ){
				DBGWRT_ERR((DBH,"*** ATA::Command: cmd=%02x error st=%02x "
							"err=%02x\n", task->command, h->intStatus,
							ATA_READ_D8( h, 0, h->r.error)));
				error = RBDRV_WRITE_ERROR;
				break;
			}
		}
		break;

	case ATA_PROTO_NON_DATA:
		/*--- writing command register starts command ---*/
		ATA_WRITE_D8( h, 0, h->r.command, task->command );

		/*--- wait for command completion ---*/
		if( (error = WaitInterrupt( h, cmdTo )) )
			break;

		if( h->intStatus & ATA_STAT_ERR ){
			DBGWRT_ERR((DBH,"*** ATA::Command: cmd=%02x error st=%02x "
						"err=%02x\n", task->command, h->intStatus,
						ATA_READ_D8( h, 0, h->r.error)));
			error = RBDRV_READ_ERROR;
		}
		break;
	}

END:
	DBGWRT_2((DBH,"ATA::Command: finished error=%d\n", error ));
	return error;
}

/**********************************************************************/
/** Setup ATA interface to send a command
 *
 *	Waits until BSY low, then writes the <sdh> value to the device/head
 *  register. Now waits until BSY low and DRDY high
 *
 * \param h			\IN	ATA handle
 * \param sdh		\IN value to write to device/head reg.
 * \param ms		\IN time to wait until ready for command
 *
 * \return  RBDRV error code
 *
 */
static int32 PrepareCmd( ATA_HANDLE_T *h, u_int8 sdh, u_int32 ms)
{
	u_int32 toutTicks = Ms2Ticks(h,ms);
	u_int32 baseTick;
	volatile u_int32 tout = 100000;
	int rv;

	DBGWRT_2((DBH,"ATA::PrepareCmd:\n"));

	if( (rv = WaitBusyLow( h, ms )) )
		return rv;

	ATA_WRITE_D8( h, 0, h->r.sdh, sdh );

	baseTick = OSS_TickGet(h->osh);

	/*--- wait for BUSY low, READY high ---*/
    while( (ATA_READ_D8(h, 0, h->r.status) & (ATA_STAT_BUSY | ATA_STAT_READY))
		  != ATA_STAT_READY){

		if( toutTicks != 0 ) {
			if( TICKDIFF(h,baseTick) > toutTicks ){
				DBGWRT_ERR((DBH,"*** ATA PrepareCmd: timeout status=%02x\n",
							ATA_READ_D8(h, 0, h->r.status)));
				return RBDRV_HW_DAMAGE;
			}
		}
		else {
			/*--- clock not running ---*/
			if( --tout == 0 ){
				DBGWRT_ERR((DBH,"*** ATA PrepareCmd: timeout(p)status=%02x\n",
							ATA_READ_D8(h, 0, h->r.status)));
				return RBDRV_HW_DAMAGE;
			}
		}
	}
	return 0;
}

#endif /* ATA_UNITTEST_SIMULATED_COMMAND_FUNC */

/**********************************************************************/
/** Wait until BSY goes or is low
 *
 * \param h			\IN	ATA handle
 * \param ms		\IN time to wait for BSY low in ms
 *
 * \return RBDRV error code
 *
 */
static int32 WaitBusyLow( ATA_HANDLE_T *h, u_int32 ms)
{
	u_int32 toutTicks = Ms2Ticks(h,ms);
	u_int32 baseTick = OSS_TickGet(h->osh);

	DBGWRT_2((DBH,"ATA::WaitBusyLow:\n"));

#ifdef MENMON
	{
	int 	i;
	/* wait for BUSY high
	 * may be the high pulse is very short
	 * so we try to detect it only 100 times
	 */
	for( i=0; i<10000; i++ )
	{
    	if( !(i%100) &&
    		(ATA_READ_D8(h, 0, h->r.altStatus) & ATA_STAT_BUSY)
    	  )
    		break;
	}
	}
#endif

	/* wait for BUSY low */
    while( (ATA_READ_D8(h, 1, h->r.altStatus) & ATA_STAT_BUSY)){
		if( TICKDIFF(h,baseTick) > toutTicks ){
			DBGWRT_ERR((DBH,"*** ATA WaitBusyLow: timeout after %d ms\n",
						ms ));
			return RBDRV_HW_DAMAGE;
		}
	}
	return 0;
}

/**********************************************************************/
/** Wait for ATA interrupt
 *
 * Polls status or waits for interrupt semaphore depending on useIrq flag.
 *
 * h->intStatus is set to the value of the ATA STATUS register during irq.
 *
 * \param h			\IN	ATA handle
 * \param ms		\IN time to wait for interrupt low in ms
 *
 * \return RBDRV error code
 *
 */
static int32 WaitInterrupt( ATA_HANDLE_T *h, u_int32 ms)
{
	int32 rv;

	if( !h->desc.useIrq ) {

		rv =  WaitBusyLow( h, ms );
		h->intStatus = ATA_READ_D8(h, 0, h->r.status);
		return rv;
	}
	else {
		/*--- wait for interrupt ---*/
		while( (rv = OSS_SemWait(h->osh, h->irqSem, ms)) ==
			   ERR_OSS_SIG_OCCURED )
			;
		if( rv == ERR_OSS_NO_SYSCLOCK ){
			/*--- if no sysclock, wait forever ---*/
			while( (rv = OSS_SemWait(h->osh, h->irqSem, OSS_SEM_WAITFOREVER))
				   == ERR_OSS_SIG_OCCURED )
				;
		}

		DBGCMD( RegDump(h, __LINE__); )

		if( rv==ERR_OSS_TIMEOUT ){
			DBGWRT_ERR((DBH,"*** ATA: WaitInterrupt timeout!\n"));
		}
		return rv==ERR_OSS_TIMEOUT ? RBDRV_HW_DAMAGE : 0;
	}
}

/**********************************************************************/
/** Convert time to number of OSS ticks
 *
 * \param h			\IN	ATA handle
 * \param ms		\IN time in ms
 *
 * \return number of ticks (at least 1)
 *
 */
static u_int32 Ms2Ticks( ATA_HANDLE_T *h, u_int32 ms )
{
	u_int32 rv, tickRate = OSS_TickRateGet(h->osh), f;

	f = tickRate * ms;

	if( tickRate == 0 )
		rv = 1;

	else if( (ms != f/tickRate) ){
		/* computation would overflow, use seconds basis */
		rv = tickRate * (ms/1000);
	}
	else {
		rv = f / 1000;

	}
	if( rv == 0 )
		rv = 1;					/* at least one tick */

	return rv;
}

/**********************************************************************/
/** ATA interrupt service routine
 *
 * Must be called by OS specific interrupt handler
 *
 * \param h			\IN	ATA handle
 *
 */
void ATA2_Irq( ATA2_HANDLE_T *_h)
{
	ATA_HANDLE_T *h = (ATA_HANDLE_T *)_h;
	int drive;
    DBGCMD( static const char *functionName = __FUNCTION__; )

	IDBGWRT_1((DBH,">>> %s: handle=0x%x\n", functionName, _h));

	/* do chipset specific pre handling if required */
	if( h->desc.chipset->intPreProc != NULL ){
		int32 retVal = h->desc.chipset->intPreProc( _h );
		switch(retVal){
		case( ATA2_CHIPS_ISTAT_NOIRQ ):
			goto EXIT_POST_PROC;
		case( ATA2_CHIPS_ISTAT_REMOVE ):
			IDBGWRT_2((DBH,">>> %s: preproc: remove\n", functionName));
			for( drive = 0; drive < ATA_MAX_DRIVES; drive++ ){
				h->drive[drive].flags = 0;
				h->drive[drive].state = ATA2_DRIVE_ST_NONE;
			}
			goto EXIT_POST_PROC;
		case( ATA2_CHIPS_ISTAT_CHANGE ):
			IDBGWRT_2((DBH,">>> %s: preproc: insert\n", functionName));
			for( drive = 0; drive < ATA_MAX_DRIVES; drive++ ){
				h->drive[drive].flags = 0;
				h->drive[drive].state = ATA2_DRIVE_ST_MED_CH;
			}
			goto EXIT_POST_PROC;
		default:
			break;
		}

		/* when got here it is an ATA interrupt from this controller */
	}


	/*--- reading of status reg resets irq ---*/
	/*??? check if interrupt actually pending */
	h->intStatus = ATA_READ_D8(h, 0, h->r.status);

	OSS_SemSignal( h->osh, h->irqSem );

EXIT_POST_PROC:
	if( h->desc.chipset->intPostProc != NULL ) {
		h->desc.chipset->intPostProc( _h );
	}

	IDBGWRT_1((DBH,"<<< %s\n", functionName));
}








