/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: ata_int.h
 *
 *       Author: kp
 *        $Date: 2008/04/02 14:52:42 $
 *    $Revision: 1.3 $
 *
 *  Description: ATA driver internal include file
 *
 *     Switches: see ata.c
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: ata2_int.h,v $
 * Revision 1.3  2008/04/02 14:52:42  cs
 * R: header file msdos_fs.h not required
 * M: removed include statement for msdos_fs.h
 *
 * Revision 1.2  2006/12/22 15:07:20  cs
 * moved ATA_HANDLE_T to ata2_drv.h
 *
 * Revision 1.1  2005/08/31 14:54:52  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998-2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _ATA2_INT_H
#define _ATA2_INT_H

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef BIG_ENDIAN
#	ifdef LITTLE_ENDIAN
#		error "Do not define BIG_ENDIAN and LITTLE_ENDIAN together"
#	endif
#endif
#ifndef BIG_ENDIAN
#	ifndef LITTLE_ENDIAN
#		error "BIG_ENDIAN or LITTLE_ENDIAN must be defined"
#	endif
#endif

#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>
#include <MEN/dbg.h>
#include <MEN/maccess.h>
#include <MEN/ata_regs.h>
#include <MEN/ataioctl.h>
#include <MEN/rbdioctl.h>
#include <MEN/rbdrv.h>


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#define ATA_MAX_DRIVES			2
#define LOCK_ATA \
	{ while( OSS_SemWait(h->osh, h->lockSem, OSS_SEM_WAITFOREVER) != 0 );}
#define UNLOCK_ATA \
	{ OSS_SemSignal( h->osh, h->lockSem ); }

/*--- ATA_DRIVE.flags ---*/
#define DRV_PRESENT				0x0001
#define DRV_IS_FLASH			0x0002 /* compactFlash */
#define DRV_SUPPORT_UDMA		0x0004 /* UDMA mode */
#define DRV_SUPPORT_LBA			0x0008 /* LBA mode */
#define DRV_SUPPORT_LBA48		0x0010 /* LBA48 mode */

/*
 * Memory region pointed by prd table can't cross a 64K boundary in memory
 * # max of sectors = 256. 256 sectors = 128K. 128K = max 3 prd table
 * (start of the memory pointed by the PRDs table can be not align on 64K)
 *
 * The table is here twice in case the first 3 entries would cross a 64K
 * boundary. We use the second index 3..5 in this case
 */
#define ATA_PRD_TABLE_SIZE			3

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/

/*--- structure for drive (master/slave) specific parameters ---*/
typedef struct {
	u_int32			 flags;					/* see above */
	u_int32			 cylinders;				/* cylinders on drive */
	u_int32			 heads;					/* heads */
	u_int32			 sectorsTrack; 			/* sectors per track */
	u_int32			 nSectors;				/* raw capacity in sectors */
	u_int32			 xfrMode;				/* used xfer mode (see ataioctl.h)*/
	int 			 subDev;				/* 0=master 1=slave */
	u_int32			 state;					/* state of drive ATA2_DRIVE_ST_* */
} ATA_DRIVE;

/** Physical Region Descriptor */
typedef struct {
	MACCESS		addr;
	u_int16		count;
	u_int16		eot;
} ATA_PRD;

/** ATA transfer protocol */
typedef enum {
	ATA_PROTO_PIO_DATA_IN =	1,
	ATA_PROTO_PIO_DATA_OUT = 2,
	ATA_PROTO_NON_DATA = 3,
	ATA_PROTO_DMA_IN = 4,
	ATA_PROTO_DMA_OUT = 5
} ATA_PROTOCOL;

/** ATA direction */
typedef enum {
	ATA_DIR_IN = 0,
	ATA_DIR_OUT = 1
} ATA_DIRECTION;

typedef struct  {
	ATA_PROTOCOL protocol;
	u_int8 command;

	/* most recent written registers */
	struct {
		u_int8 feature;
		u_int8 secCnt;
		u_int8 lbaLow;			/* CHS: sector */
		u_int8 lbaMid;			/* CHS: cylLo */
		u_int8 lbaHigh;			/* CHS: cylHi */
		u_int8 sdh;
	} current;

	u_int8 useLba48;

	/* if useLba48==TRUE: previously written registers */
	struct {
		u_int8 feature;
		u_int8 secCnt;
		u_int8 lbaLow;
		u_int8 lbaMid;
		u_int8 lbaHigh;
	} previous;


} ATA_TASK_T;

/* needs some defines for ATA_HANDLE_T */
#include <MEN/ata2_drv.h>


/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/


#ifdef __cplusplus
	}
#endif

#endif	/* _ATA2_INT_H */
