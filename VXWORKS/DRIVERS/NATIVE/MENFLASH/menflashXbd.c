/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: menflashXbd.c
 *      Project: PCMCIA
 *
 *       Author: uf/cs
 *        $Date: 2011/12/19 18:32:14 $
 *    $Revision: 1.5 $
 *
 *  Description: MEN block device driver for on board flashes
 *               The only user callable routines are MENXXFLASH_DevCreate()
 *               and MENFLASH_PartionSet().
 *               All other routines are performed by file system and
 *               io system calls like fopen()/open().. .
 *
 *     Required: -
 *     Switches: MEN_A9  set function name MENXXFLASH_DevCreate()
 *                       to  MENA9FLASH_DevCreate()
 *               MEN_A11, MEN_F006, MEN_A015
 *
 *         Note: File systems like MS-DOS and RT-11 compatible can be mounted
 *               for READ access ONLY!
 *               For write access mount a raw file system.
 *
 *               The lower level xxTry routine switches the flashes to the
 *               identify mode, therefore rom resident images should not be
 *               located in the affected flash chips.
 *
 *				 MENXXFLASH_DevCreate() should only called once. After this
 *               call up to 8 partitions can be created by calling
 *				 MENFLASH_PartionSet().
 *
 *               MENFLASH_PartionSet(): an device /menFlashXbd is created.
 *               for boards with more than one flash the MENFLASH_PartionSet()
 *               interface has to be extended to hold the name of this device!
 *
 *---------------------------------------------------------------------------
 *
 *  Module Structure of the MEN Flash Driver under VxWorks:
 *
 *
 *                           +-----------------+
 *       file system         | raw file system |
 *                           +-----------------+
 *
 *                           +-----------------+
 *       io system           |    io system    |
 *                           +-----------------+
 *
 *                           +-----------------+
 *       XBD driver          |   menflashXbd   |
 *                           +-----------------+
 *
 *       MTD -             +---------------------+
 *       memory            | i.e. A8 Super MTD   |
 *       technology        |                     |
 *       driver            | i.e. i28f008 MTD    |
 *                         +---------------------+
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: menflashXbd.c,v $
 * Revision 1.5  2011/12/19 18:32:14  ts
 * R: New Board EM10A as successor of EM01A was introduced
 * M: added support for EM10A
 *
 * Revision 1.4  2010/01/22 13:40:14  cs
 * R: VxW 6.7 compiler is more restringent and uncovered signed/unsigned issues
 * M: resolved signed/unsigned mismatches in comparisons and assignments
 *
 * Revision 1.3  2007/11/23 15:14:54  cs
 * fixed:
 *   - name for EM9 functions (MTD_EM9* to MTD_Em9*)
 *
 * Revision 1.2  2007/07/20 12:12:02  cs
 * added:
 *   + support for EM9
 *
 * Revision 1.1  2007/01/05 17:23:56  cs
 * Initial Revision
 *
 *
 * cloned from menflash.c Version 1.18
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static const char RCSid[]="MENFLASH - XBD driver: $Id: menflashXbd.c,v 1.5 2011/12/19 18:32:14 ts Exp $";

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
#include "vxWorks.h"

#include "semLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "drv/xbd/xbd.h"           /* XBD library header */
#include "drv/erf/erfLib.h"        /* event frame work library header */

#include <MEN/men_typs.h>
#include <MEN/maccess.h>
#include <MEN/dbg.h>
#include <MEN/oss.h>
#include <MEN/rbdioctl.h>
#include <MEN/rbdrv.h>
#include <MEN/mtdioctl.h>
#include <MEN/mtd.h>
#include <MEN/memioctl.h>
#include <MEN/mem_drv.h>
#include <MEN/menflash.h>

#include <drv/erf/erfLib.h>
#include <logLib.h>

/* typedefs */

typedef struct xbdFlashDev
{
	MENFLASH_DEV  fDev;		/* MEN Flash device handle */
	SEMAPHORE     mutex;    /* lock for this Wrapper */
	int           xbd_service;  /* Service task for this wrapper */
	SEMAPHORE     bio_ready;    /* Work available for service */
	BLK_DEV       blk_dev;      /* Block Device that we wrap */
	devname_t     xbd_name;     /* The name of this XBD */
	device_t      device;       /* device number of this instance */
	struct bio *  bio_queueh;   /* List of bio's for us to process */
	struct bio *  bio_queuet;   /* Tail of the bio list */
	BOOL          xbd_instantiated;  /* TRUE if stack init is complete */
    SEMAPHORE     xbd_semId;    /* used for xbdFlashDevCreateSync() */
} XBDFLASHDEV;

#ifdef MEN_PP01
	#include <MEN/pp01_cfg.h>
#endif
#ifdef MEN_KM01
	#include <MEN/km01_cfg.h>
#endif
#ifdef MEN_EM01
	#include <MEN/em01_cfg.h>
#endif
#ifdef MEN_EM03
	#include <MEN/em03_cfg.h>
#endif
#ifdef MEN_EM09
	#include <MEN/em09_cfg.h>
#endif
#ifdef MEN_EP01
	#include <MEN/ep01_cfg.h>
#endif
#ifdef MEN_EM10A
    #include <MEN/em10a_cfg.h>
#endif
#ifdef MEN_A021
    #include "men_a021.h"
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define DBG_MYLEVEL     devP->dbgLevel
#define DBH             devP->dbgHdl

#define A9_FLASH_BASE   	0x04000000
#define A9_FLASH_SIZE   	0x00800000
#define A11_FLASH_BASE_16MB     0xFF000000
#define A11_FLASH_SIZE_16MB     0x01000000
#define A11_FLASH_SIZE_8MB      0x00800000

#define F006_FLASH_BASE         0xfe000000
#define F006_FLASH_SIZE_8MB     0x00800000
#define F006_FLASH_SIZE_16MB    0x01000000
#define F006_FLASH_SIZE_32MB    0x02000000
#ifdef MEN_EM10A
# if defined(CPU_MODEL_EM10A00) || defined(CPU_MODEL_EM10A01)
# define EM10A_FLASH_BASE   	0xff800000
# endif
# ifdef CPU_MODEL_EM10A02
# define EM10A_FLASH_BASE   	0xff000000
# endif
/* sanity check against non-defined board model */
# ifndef EM10A_FLASH_BASE
# error "EM10A_FLASH_BASE not defined - which board model do you want me to support ? please specify -DCPU_MODEL_EM10A00, ..01 or ..02 !"
# endif
#endif

#define A015_FLASH_BASE         0x78000000
#define A015_FLASH_SIZE_32MB    0x02000000

#define MENFLASH_SECTOR_SIZE  512

#define IDENT_BUF_SIZE  0x1000

/* the base name for XBD devices must not have more than 8 characters
 * (including leading forward slash) */
#if defined (MEN_A9)   || defined (MEN_A11)   || defined (MEN_A015) || \
	defined (MEN_EM01) || defined (MEN_EM03)  || defined (MEN_EM09) || \
	defined (MEN_EP01) || defined (MEN_EM10A) || defined (MEN_A021) || \
	defined (MEN_F006) || defined (MEN_KM01)  || defined (MEN_PP01)
#	define MENFLASH_XBD_BASENAME "/flash"
#else
#	error "*** MEN_A9, MEN_A11, MEN_A015, MEN_PP01, MEN_KM01, MEN_F006, MEN_EM01, MEN_EM10A, MEN_A021, MEN_EM03, MEN_EM09 or MEN_EP01 must be defined"
#endif /*MEN_A9 ... */

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
extern int32
#ifdef MEN_A9
	MTD_A8Flash_Try
#elif defined (MEN_A11)
	MTD_A11Flash_Try
#elif defined (MEN_F006)
	MTD_F006_Flash_Try
#elif defined (MEN_A015)
	MTD_A015_Flash_Try
#elif defined (MEN_PP01)
	MTD_PP01Flash_Try
#elif defined (MEN_KM01)
	MTD_KM01Flash_Try
#elif defined (MEN_EM01)
	MTD_EM01Flash_Try
#elif defined (MEN_EM10A)
	MTD_EM10AFlash_Try
#elif defined (MEN_EM03)
	MTD_Em03Flash_Try
#elif defined (MEN_EM09)
	MTD_Em09Flash_Try
#elif defined (MEN_EP01)
	MTD_EP01Flash_Try
#elif defined (MEN_A021)
	MTD_A021Flash_Try
#else
#   error ("board type not supported")
#endif
(   OSS_HANDLE *osh,
	DBG_HANDLE *dbh,
	u_int32 dbgLevel,
	MTD_ACCDESC *mtdAccDesc,
	JEDEC_INFO *jedInfo,
	MTD_INIT *mtdInitP
);

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static STATUS MENFLASH_Ioctl
(
	MENFLASH_DEV    *devP,
	int             code,
	int             arg
);

static STATUS MENFLASH_BlkRd
(
	MENFLASH_DEV    *devP,
	int             startBlk,
	int             numBlks,
	char            *pBuffer
);

static STATUS MENFLASH_BlkWrt
(
	MENFLASH_DEV *devP,
	int          startBlk,
	int          numBlks,
	char         *pBuffer
);

static int32 MapErrorCodes( int32 rbdrvErr );

/* foward declarations */

LOCAL int menFlashXbdIoctl        (struct xbd *, int, void *);
LOCAL int menFlashXbdStrategy     (struct xbd *, struct bio *);
LOCAL int menFlashXbdDump         (struct xbd *, sector_t, void *, size_t);
LOCAL void menFlashXbdService      (struct xbdFlashDev *);
LOCAL void menFlashXbdCreateSyncHandler (UINT16, UINT16, void *, void *);

LOCAL struct xbd_funcs menFlashXbd_funcs =
	{
	menFlashXbdIoctl,
	menFlashXbdStrategy,
	menFlashXbdDump,
	};


/*************************** MENXXFLASH_Ident ********************************
 *
 *  Description: Print the ident string to stdout.
 *
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: RCSid
 ****************************************************************************/
static void MENXXFLASH_Ident
(
	void
)
{
	printf("%s\n", RCSid );
#ifdef MEN_A9
	printf("           for A8910\n");
#elif defined (MEN_A11)
	printf("           for A11\n");
#elif defined (MEN_F006)
	printf("           for F006\n");
#elif defined (MEN_A015)
	printf("           for A015\n");
#elif defined (MEN_PP01)
	printf("           for PP01\n");
#elif defined (MEN_KM01)
	printf("           for KM01\n");
#elif defined (MEN_EM01)
	printf("           for EM01\n");
#elif defined (MEN_EM10A)
	printf("           for EM10A\n");
#elif defined (MEN_EP01)
	printf("           for EP01\n");
#elif defined (MEN_EM03)
	printf("           for EM03\n");
#elif defined (MEN_EM09)
	printf("           for EM09\n");
#elif defined (MEN_A021)
	printf("           for A021\n");
#else
#   error ("board type not supported")
#endif /*MEN_A9*/
}/*MENFLASH_Ident*/
/************************ MENXXFLASH_DevCreate ********************************
 *
 *  Description:  Creates a flash device.
 *
 *---------------------------------------------------------------------------
 *  Input......: addr     physical start address of the flash disk
 *                        A9/10 - 0x04000000
 *                        A11   - 0xFF000000 - 16MB, 0xFF800000 - 8MB
 *                        F006  - 0xFE100000
 *                        A015  - 0x78000000
 *                        A021  - 0xEE000000 -  32MB
 *                        PP01  - 0xFF000000 - 16MB, 0xFF800000 - 8MB
 *                        KM01  - 0xFF800000
 *                        EM01  - 0xFF800000
 *                        EM10A - 0xFF800000
 *                        EM03  - 0xFF800000 - 8 MB
 *                        EM09  - 0xFF000000 - 16 MB
 *
 *
 *               size     size of the flash disk
 *                        A9/10:
 *                        	0               - get the whole flash
 *                        	0x00400000      - get 4MB
 *                        	must be flash sector aligned
 *                        ( e.g. sector size A9/10 with 4 flashes per bank
 *                          is 0x40000, with one flash is 0x10000 )
 *                        A11:
 *                        	0x00800000      - get  8MB
 *                          0x01000000      - get 16MB
 *                        F006:
 *                        	0x00800000      - get  8MB
 *                         A015:
 *                        	0x02000000      - get 32MB
 *                         A021:
 *                        	0x02000000      - get 32MB
 *                        PP01
 *                        	0x00E80000      - get 14.5MB - 16MB
 *                        	0x00680000      - get  6.5MB -  8MB
 *                        KM01
 *                        	0x00700000      - get  7MB
 *
 *               dbgLevel e.g. 0xc008000 show errors at interrupt and
 *                        task level see -> dbg.h
 *                        debug messages are only available with the debug
 *                        driver
 *
 *  Output.....: return  pointer to menflash device structure | NULL
 *
 *  Globals....: -
 *
 ****************************************************************************/
MENFLASH_DEV*
#ifdef MEN_A9
   MENA9FLASH_DevCreate
#elif defined (MEN_A11)
   MENA11FLASH_DevCreate
#elif defined (MEN_F006)
   MENF006FLASH_DevCreate
#elif defined (MEN_A015)
   MENA015FLASH_DevCreate
#elif defined (MEN_PP01)
   MENPP01FLASH_DevCreate
#elif defined (MEN_KM01)
   MENKM01FLASH_DevCreate
#elif defined (MEN_EM01)
   MENEM01FLASH_DevCreate
#elif defined (MEN_EM10A)
   MENEM10AFLASH_DevCreate
#elif defined (MEN_EM03)
   MENEM03FLASH_DevCreate
#elif defined (MEN_EM09)
   MENEM09FLASH_DevCreate
#elif defined (MEN_EP01)
   MENEP01FLASH_DevCreate
#elif defined (MEN_A021)
   MENA021FLASH_DevCreate
#else
#   error ("board type not supported")
#endif /*MEN_A9*/
(
	char     *addr,
	u_int32  size,
	u_int32  dbgLevel
)
{
	DBGCMD( const char *functionName = __FUNCTION__; )
	MENFLASH_DEV    *devP;
	u_int32         retCode;
	MTD_TRY         mtdTry;
	MTD_INIT        mtdInit;
	MTD_ACCDESC     mtdAccDesc;
	JEDEC_INFO      jedInfo;
	u_int32         knownSize;
	u_int32         maxPhySearch;

	OSS_Init();

	/* Allocate a MENFLASH_DEV structure for device */
	devP = (MENFLASH_DEV *) malloc( sizeof(struct xbdFlashDev) );
	if( devP == NULL )
		return( NULL );                 /* no memory */

	/* fill turkey with 0 */
	OSS_MemFill( OSS_VXWORKS_OS_HDL, sizeof(struct xbdFlashDev), (char*) devP, 0 );

	/* set the partion number */
	devP->partNbr = 0;

	/*----------------------+
	|  Create debug handle  |
	+----------------------*/
	DBGINIT((NULL,&DBH));

	DBG_MYLEVEL  = dbgLevel;
	DBGWRT_1((DBH,"%s()\n", functionName ));

	/*----------------------+
	|  Parameter setup      |
	+----------------------*/
	#ifdef MEN_A9
		mtdTry = MTD_A8Flash_Try;
		if( (int)addr < A9_FLASH_BASE ||
			(A9_FLASH_BASE+A9_FLASH_SIZE-1) < (int)addr )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = 0;
		if( size == 0 )
		{
			maxPhySearch     = A9_FLASH_SIZE - (A9_FLASH_BASE - (u_int32)addr);
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_A11)
		mtdTry = MTD_A11Flash_Try;
		if( (int)addr < A11_FLASH_BASE_16MB ||
			(A11_FLASH_BASE_16MB+A11_FLASH_SIZE_16MB-1) < (int)addr )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = 0;
		if( size < 1 || A11_FLASH_SIZE_16MB < size )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_F006)
		mtdTry = MTD_F006_Flash_Try;
		if( (int)addr < F006_FLASH_BASE
			|| (F006_FLASH_BASE+F006_FLASH_SIZE_8MB -1) < (int)addr )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = 0;
		if( size < 1 || F006_FLASH_SIZE_8MB < size )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_A015)
		mtdTry = MTD_A015_Flash_Try;
		if( (int)addr < 0x78000000
			|| (0x78000000+0x2000000 -1) < (int)addr )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = 0;
		if( size < 1 || 0x2000000 < size )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_PP01)
		mtdTry = MTD_PP01Flash_Try;
		if( (int)addr < PP01CFG_FLASH_16MB
			|| (PP01CFG_KD12_MSCAN_FPGA_CODE_BASE) < (int)addr
			|| (PP01CFG_KD12_MSCAN_FPGA_CODE_BASE) < (int)addr + size
		  )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_A021)
		mtdTry = MTD_A021Flash_Try;
		if( ((int)addr < A021_FLASH_BASE ) || ((int)addr >= A021_FLASH_BASE + A021_FLASH_SIZE) ){
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 ) {
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		} else {
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_KM01)
		mtdTry = MTD_KM01Flash_Try;
		if( (int)addr < KM01CFG_FLASH_8MB
			|| (KM01CFG_PMM_BASE) < (int)addr
			|| (KM01CFG_PMM_BASE) < (int)addr + size
		  )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_EM01)
		mtdTry = MTD_EM01Flash_Try;
		if( (int)addr < EM01CFG_FLASH_8MB
			|| (EM01CFG_PMM_BASE) < (int)addr
			|| (EM01CFG_PMM_BASE) < (int)addr + size
		  )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_EM10A)
		mtdTry = MTD_EM10AFlash_Try;
		if( (int)addr < EM10A_FLASH_BASE ) {
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 ) {
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		} else {
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_EP01)
		mtdTry = MTD_EP01Flash_Try;
		if( (int)addr < EP01CFG_FLASH_8MB
			|| (EP01CFG_PMM_BASE) < (int)addr
			|| (EP01CFG_PMM_BASE) < (int)addr + size
		  )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/
	#elif defined (MEN_EM03)
		mtdTry = MTD_Em03Flash_Try;
		if( (int)addr < EM03CFG_FLASH_ADDR )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/

	#elif defined (MEN_EM09)
		mtdTry = MTD_Em09Flash_Try;
		if( (int)addr < EM09CFG_FLASH_ADDR )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
			goto CLEANUP;
		}
		jedInfo.manfId   = 0;
		jedInfo.deviceId = 0;
		knownSize        = size;
		if( size < 1 )
		{
			DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
			goto CLEANUP;
		}
		else
		{
			maxPhySearch     = size;
		}/*if*/

	#else
	#   error ("board type not supported")
	#endif /*MEN_A9*/

	/*----------+
	|  Try MTD  |
	+----------*/
	mtdAccDesc.ma = (MACCESS)addr;
	if( (retCode = (*mtdTry)( OSS_VXWORKS_OS_HDL, DBH, DBG_MYLEVEL,
							  &mtdAccDesc, &jedInfo, &mtdInit )) )
	{
		DBGWRT_ERR((DBH,"*** ERROR %s(): error 0x%08x in MTD_Try\n",
					functionName, retCode ));
		goto CLEANUP;
	}/*if*/

	/*-----------+
	|  Init MEM  |
	+-----------*/
	retCode = MEM_Init( OSS_VXWORKS_OS_HDL,
						mtdInit,
						&mtdAccDesc,
						&jedInfo,
						knownSize,
						maxPhySearch,
						dbgLevel,
						&devP->memHdl );
	if( retCode )
	{
		DBGWRT_ERR((DBH,"*** ERROR - %s(): error 0x%08x in MEM_Init()\n",
					functionName, retCode ));
		goto CLEANUP;
	}/*if*/

	return( devP );

CLEANUP:
	if( devP->memHdl )
		(*devP->memHdl->Exit)(devP->memHdl );
	/* cleanup debug */
	DBGEXIT((&DBH));
	OSS_Exit();
	free( devP );
	return( NULL );
}/*MENXXFLASH_DevCreate*/


/************************* MENXXFLASH_DevRemove *******************************
 *
 *  Description: Deinitialize device and free memory.
 *
 *         Note: for debug purposes only
 *               Don't call after having mounted a file system.
 *
 *---------------------------------------------------------------------------
 *  Input......: devP   pointer to block device structure
 *
 *  Output.....: return 0 | error code
 *
 *  Globals....: -
 *
 ****************************************************************************/
int
#ifdef MEN_A9
   MENA9FLASH_DevRemove
#elif defined (MEN_A11)
   MENA11FLASH_DevRemove
#elif defined (MEN_F006)
   MENF006FLASH_DevRemove
#elif defined (MEN_A015)
   MENA015FLASH_DevRemove
#elif defined (MEN_PP01)
   MENPP01FLASH_DevRemove
#elif defined (MEN_KM01)
   MENKM01FLASH_DevRemove
#elif defined (MEN_EM01)
   MENEM01FLASH_DevRemove
#elif defined (MEN_EM10A)
   MENEM10AFLASH_DevRemove
#elif defined (MEN_EM03)
   MENEM03FLASH_DevRemove
#elif defined (MEN_EM09)
   MENEM09FLASH_DevRemove
#elif defined (MEN_EP01)
   MENEP01FLASH_DevRemove
#elif defined (MEN_A021)
   MENA021FLASH_DevRemove
#else
#   error ("board type not supported")
#endif /*MEN_A9*/
(
	MENFLASH_DEV    *devP
)
{
	int retCode;

	retCode = (*devP->memHdl->Exit)(devP->memHdl );

	DBGEXIT((&DBH));
	OSS_Exit();
	free( devP );
	return( retCode );
}/*MENXXFLASH_DevRemove*/

/************************* MENFLASH_PartionSet ********************************
 *
 *  Description: Set a memory partion and return with a pointer to the block.
 *               device structure.
 *   Once the device has been created, it must be associated with a name and a
 *   file system (dosFs, rt11Fs, or rawFs).  This is accomplished using the
 *   file system's device initialization routine or make-file-system routine,
 *   e.g., dosFsDevInit() or dosFsMkfs().
 *   The MENFLASH_PartionSet() call returns a pointer to a block device
 *   structure (BLK_DEV).  This structure contains fields that describe the
 *   physical properties of a disk device and specify the addresses
 *   of routines within the menFlashDrv driver.  The BLK_DEV structure
 *   address must be passed to the desired file system (dosFs, rt11Fs or rawFs)
 *   via the file system's device initialization or make-file-system routine.
 *   Only then is a name and file system associated with the device, making it
 *   available for use.
 *
 *         Note:  Only the rawFs can be used for write access!
 *
 *---------------------------------------------------------------------------
 *  Input......: devP       pointer to block device structure of whole dev.
 *               partNbr    partition number   ( 0..7 )
 *               partStart	partion start addr ( byte 0..x )
 *               partSize	partion size ( multiple of sector size )
 *
 *  Output.....: return  pointer to block device structure for the
 *                       selected partition | NULL
 *
 *  Globals....: -
 *
 ****************************************************************************/
device_t
#ifdef MEN_A9
   MENFLASH_PartionSet
#elif defined (MEN_A11)
   MENFLASH_PartionSet
#elif defined (MEN_F006)
   MENF006FLASH_PartionSet
#elif defined (MEN_A015)
   MENA015FLASH_PartionSet
#elif defined (MEN_PP01)
   MENPP01FLASH_PartionSet
#elif defined (MEN_KM01)
   MENKM01FLASH_PartionSet
#elif defined (MEN_EM01)
   MENEM01FLASH_PartionSet
#elif defined (MEN_EM10A)
   MENEM10AFLASH_PartionSet
#elif defined (MEN_EM03)
   MENEM03FLASH_PartionSet
#elif defined (MEN_EM09)
   MENEM09FLASH_PartionSet
#elif defined (MEN_EP01)
   MENEP01FLASH_PartionSet
#elif defined (MEN_A021)
   MENA021FLASH_PartionSet
#else
#   error ("board type not supported")
#endif /*MEN_A9*/
(
	MENFLASH_DEV    *devP,
	u_int32         partNbr,
	u_int32         partStart,
	u_int32         partSize

)
{
	DBGCMD( static const char functionName[] = "MENFLASH_PartionSet()"; )
	BLK_DEV             *pBlkDev;
	RBDRV_IOCTL_PARMS	ioctl;
	MEM_IOCTL_PARMS *memParms = (MEM_IOCTL_PARMS *)&ioctl.p.data;
	u_int32  retCode;
	MENFLASH_DEV    *devPartitionP;
	struct xbdFlashDev  * pXbdDev   = (struct xbdFlashDev *)devP,
						* pXbdPart = pXbdDev;
	char*				fBaseName = MENFLASH_XBD_BASENAME;
	device_t            xbd;
	int                 error;

	DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, partNbr ));

	if( partNbr > 7 )
	{
		DBGWRT_ERR((DBH,"*** ERROR - %s():parameter partNbr %d out of range\n",
					functionName, partNbr ));
		return( NULLDEV );
	}/*if*/

	if( devP == NULL )
	{
		DBGWRT_ERR((DBH,"*** ERROR - %s(): device pointer = NULL!\n",
					functionName ));
		goto CLEANUP;
	}/*if*/

	if( devP->partNbr != 0 )
	{
		DBGWRT_ERR((DBH,"*** ERROR - %s(): illegal device pointer\n",
					functionName ));
		goto CLEANUP;
	}/*if*/

	ioctl.subdev = partNbr<<4;
	ioctl.flags	 = 0;

	if( partNbr > 0 )
	{
		/*--- only the real partitions need to be configured ---*/
		memParms->configPartition.startAddr = partStart;
		memParms->configPartition.size		= partSize;

		/*--- Now call MEM driver to configure partition ---*/
		retCode = (*devP->memHdl->Ioctl)(devP->memHdl,
										 MEM_IOCTL_CONFIG_PARTITION, &ioctl );
		if( retCode )
		{
			DBGWRT_ERR((DBH,"*** ERROR - %s(): error 0x%08x in MEM_Ioctl() "
						"config partNbr %d\n",
						functionName, retCode, partNbr ));
			errnoSet( MapErrorCodes(retCode) );
			goto CLEANUP;
		}/*if*/
	}/*if*/

	/*-----------------+
	|  Query capacity  |
	+-----------------*/
	ioctl.subdev = 0;
	if( (retCode = (*devP->memHdl->Ioctl)(devP->memHdl, RBDRV_IOCTL_INFO,
										  &ioctl)) )
	{
		DBGWRT_ERR((DBH,"*** ERROR - %s(): error 0x%08x in MEM_Ioctl("
					"RBDRV_IOCTL_INFO )\n", functionName, retCode ));
		goto CLEANUP;
	}/*if*/

	DBGWRT_2((DBH,"  logSectors=%d ssize=%d any=%d chs=%d/%d/%d\n",
					  ioctl.p.info.logSectors,
					  ioctl.p.info.sectSize, ioctl.p.info.sectSizeAny,
					  ioctl.p.info.cylinders, ioctl.p.info.heads,
					  ioctl.p.info.sectorsTrack ));

	if( partNbr > 0 )
	{
		/* Allocate a MENFLASH_DEV structure for device */
		pXbdPart = (struct xbdFlashDev *) malloc( sizeof(struct xbdFlashDev) );
		if( pXbdPart == NULL )
		{
			DBGWRT_ERR((DBH,"*** ERROR - %s(): no memory\n", functionName ));
			goto CLEANUP;                 /* no memory */
		}/*if*/
		devPartitionP = (MENFLASH_DEV*)pXbdPart;

		/* copy the complete structure */
		*pXbdPart = *pXbdDev;

		/* set the partion number */
		devPartitionP->partNbr = partNbr;

	 }/* if */

	 pBlkDev = &pXbdPart->blk_dev;

	/* Initialize BLK_DEV structure (in xbdFlashDev) */
	pBlkDev->bd_nBlocks      = ioctl.p.info.logSectors; /* number of blocks */
	pBlkDev->bd_bytesPerBlk  = ioctl.p.info.sectSize;   /* bytes per block */
	pBlkDev->bd_blksPerTrack = ioctl.p.info.logSectors; /* blocks per track */

	pBlkDev->bd_nHeads       = 1;           /* one "head" */
	pBlkDev->bd_removable    = FALSE;       /* not removable */
	pBlkDev->bd_retry        = 1;           /* retry count */
	pBlkDev->bd_mode         = O_RDWR;      /* initial mode for device */
	pBlkDev->bd_readyChanged = TRUE;        /* new ready status */

	pBlkDev->bd_blkRd        = MENFLASH_BlkRd;  /* read block function */
	pBlkDev->bd_blkWrt       = MENFLASH_BlkWrt; /* write block function */
	pBlkDev->bd_ioctl        = MENFLASH_Ioctl;  /* ioctl function */
	pBlkDev->bd_reset        = NULL;    /* no reset function */
	pBlkDev->bd_statusChk    = NULL;    /* no check-status function */

	if( partNbr > 0 )
	{ /* if its one of the partitions assign the correct # of blocks, not whole size! */
		pBlkDev->bd_nBlocks = partSize / MENFLASH_SECTOR_SIZE;
	}

	if (semMInit (&pXbdPart->mutex, SEM_Q_PRIORITY | SEM_INVERSION_SAFE) != 0 )
		goto CLEANUP;

	if (semCInit(&pXbdPart->bio_ready, SEM_Q_PRIORITY, 0) != 0)
		goto CLEANUP;

	if (semBInit (&pXbdPart->xbd_semId, SEM_Q_PRIORITY, SEM_EMPTY) != 0)
		goto CLEANUP;

	/* store the name
	 * take care that name is always NULL terminated */
	strncpy(pXbdPart->xbd_name, fBaseName, sizeof(devname_t) - 1);
	pXbdPart->xbd_name[sizeof(devname_t) - 1] = 0;

	pXbdPart->bio_queueh = NULL;
	pXbdPart->bio_queuet = NULL;
	pXbdPart->xbd_instantiated    = FALSE;

	if (erfHandlerRegister (xbdEventCategory, xbdEventInstantiated,
							menFlashXbdCreateSyncHandler, pXbdPart,
				(int) NULL) != OK)
	{
		goto CLEANUP;
	}

	pXbdPart->xbd_service = taskSpawn ( "tMenFlashService",
										50, 0, 4096, /* Stack size. */
										(FUNCPTR)menFlashXbdService,
										(int) pXbdPart,
										0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (pXbdPart->xbd_service == ERROR)
		goto CLEANUP;

	error = xbdAttach ( &pXbdPart->fDev.xbd, &menFlashXbd_funcs, pXbdPart->xbd_name,
						pBlkDev->bd_bytesPerBlk, pBlkDev->bd_nBlocks, &xbd);

	if (error == 0)
	{
		pXbdPart->device = xbd;   /* save device instance in control block */
		pBlkDev->bd_readyChanged = 0;

		erfEventRaise (xbdEventCategory, xbdEventPrimaryInsert,
			ERF_ASYNC_PROC, (void*)xbd, NULL);
		DBGWRT_1((DBH,"%s: xbdAttached and event raised. device_t = 0x%x xbdSize=0x%x\n",
						functionName, xbd, pXbdPart->fDev.xbd.xbd_nblocks));
	} else {
		if( partNbr > 0 )
		{
			free( pXbdPart );
		}
		goto CLEANUP;
	}

    semTake (&pXbdPart->xbd_semId, WAIT_FOREVER);

	return (xbd);

CLEANUP:
	return NULLDEV;
 }/*MENFLASH_PartionSet*/


/***************************** MENFLASH_Ioctl ********************************
 *
 *  Description: Block device driver ioctl routine.
 *
 *  supported codes                  arg                 meaning
 *  ----------------------           ------   ------------------------------
 *  MENFLASH_IOCTL_IDENT             -        print ident strings to stdout
 *
 *  MENFLASH_IOCTL_FLASH_ERASE       RBDRV_IOCTL_PARMS
 *                                   *MTD_IOCTL_PARMS  address and size
 *                                                     see MENFLASH/TEST/test.c
 *                                                     for example
 *
 *  MENFLASH_IOCTL_FLASH_SECTOR_SIZE RBDRV_IOCTL_PARMS subdev
 *                                   *MTD_IOCTL_PARMS  address of sector
 *                                                     see MENFLASH/TEST/test.c
 *                                                     for example
 *
 *  MENFLASH_IOCTL_INFO              RBDRV_IOCTL_PARMS subdev
 *                                                     see MENFLASH/TEST/test.c
 *                                                     for example
 *
 *---------------------------------------------------------------------------
 *  Input......: devP   pointer to block device structure
 *               code   io control code
 *               arg    argument
 *
 *  Output.....: return OK | ERROR
 *
 *  Globals....: errno
 ****************************************************************************/
static STATUS MENFLASH_Ioctl
(
	MENFLASH_DEV    *devP,
	int             code,
	int             arg
)
{
	DBGCMD( static const char functionName[] = "MENFLASH_Ioctl()"; )
	int      status = OK;         /* returned status value */
	u_int32  retCode;
	char*    buf;
	RBDRV_IOCTL_PARMS	*ioCtrlParamsP;

    DBGWRT_1((DBH,"%s  devP %p partNbr %d code= %x\n", functionName, devP, devP->partNbr, code ));

	if( code < MENFLASH_IOCTL_FLASH_ERASE || MENFLASH_IOCTL_INFO < code )
	{
		switch( code )
		{
			case MENFLASH_IOCTL_IDENT:
				DBGWRT_1((DBH,"MEN IOCTL:: IDENT\n"));
				 MENXXFLASH_Ident();
				 buf = calloc( IDENT_BUF_SIZE, 1 );
				 if( buf )
				 {
					 (*devP->memHdl->Ident)(devP->memHdl, buf, IDENT_BUF_SIZE);
					 buf[IDENT_BUF_SIZE-1]='\0'; /* KlocWork finding: terminate last element of buf always with 0 */
					 printf( stdout, buf );
					 free( buf );
				 }/*if*/
			   break;
			case FIOSYNC:
				DBGWRT_1((DBH,"MEN IOCTL:: FIOSYNC\n"));

				break;
			default:
				DBGWRT_1((DBH,"MEN IOCTL:: UNKNOWN1 !!!!\n"));
			   errnoSet (S_ioLib_UNKNOWN_REQUEST);
			   status = ERROR;
		}/*switch*/
	}
	else
	{
		ioCtrlParamsP = (RBDRV_IOCTL_PARMS*)arg;
		ioCtrlParamsP->subdev = devP->partNbr << 4;;
		switch( code )
		{
			case MENFLASH_IOCTL_FLASH_ERASE:
			{
#ifdef DBG
				RBDRV_IOCTL_PARMS* argptr = (RBDRV_IOCTL_PARMS*)arg;
				MEM_IOCTL_PARMS * ptr= (MEM_IOCTL_PARMS *)(argptr->p.data);
#endif
				DBGWRT_1((DBH,"MEN IOCTL:: erase request start Adr %x  size %x\n", ptr->erase.startAddr, ptr->erase.nBytes) );
			}
				code = MEM_IOCTL_FLASH_ERASE;
			   break;
			case MENFLASH_IOCTL_FLASH_SECTOR_SIZE:
				DBGWRT_1((DBH,"MEN IOCTL:: SECTOR SIZE\n"));
				code = MEM_IOCTL_FLASH_SECTOR_SIZE;
			   break;
			case MENFLASH_IOCTL_INFO:
				DBGWRT_1((DBH,"MEN IOCTL:: IOCTL INFO\n"));
				code = MTD_IOCTL_INFO;
			   break;
			default:
			DBGWRT_1((DBH,"MEN IOCTL:: UNKNOWN IOCTL !!!!!!!!!!1\n"));
			   errnoSet (S_ioLib_UNKNOWN_REQUEST);
			   status = ERROR;
			   goto CLEANUP;
		}/*switch*/

		retCode = (*devP->memHdl->Ioctl)(devP->memHdl, code,
										 (RBDRV_IOCTL_PARMS*) arg );
		if( retCode )
		{
			status = ERROR;
			DBGWRT_ERR((DBH,"*** ERROR - %s(): error 0x%08x in MEM_Ioctl()\n",
						functionName, retCode ));
			errnoSet( MapErrorCodes(retCode) );
		}/*if*/
	}/*if*/

CLEANUP:
	return (status);
}/*MENFLASH_Ioctl*/

/***************************** MENFLASH_BlkRd ********************************
 *
 *  Description: Block device driver read routine.
 *
 *---------------------------------------------------------------------------
 *  Input......: devP      pointer to block device structure
 *               startBlk  number of the first block to read
 *               numBlks   number of blocks to read
 *               pBuffer   pointer to the read buffer
 *
 *  Output.....: return OK | ERROR
 *
 *  Globals....: errno
 ****************************************************************************/
static STATUS MENFLASH_BlkRd
(
	MENFLASH_DEV    *devP,
	int             startBlk,
	int             numBlks,
	char            *pBuffer
)
{
	DBGCMD( static const char functionName[] = "MENFLASH_BlkRd()"; )
	struct xbdFlashDev *pXbdDev = (struct xbdFlashDev *)devP;
	int     bytesPerBlk;    /* number of bytes per block */
	int     status;         /* the return code */
	int32   retCode;
	u_int32 flags=0;
	u_int32 subDev;

	status = OK;
	bytesPerBlk = pXbdDev->blk_dev.bd_bytesPerBlk;

	/* DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, devP->partNbr )); */

	/*------------------+
	| read sectors      |
	+------------------*/
	subDev  = devP->partNbr << 4;
	retCode = (devP->memHdl->ReadBlocks)( devP->memHdl, subDev,
										  startBlk, numBlks, (u_int8*)pBuffer,
										  bytesPerBlk, flags );
	if( retCode )
	{
		status = ERROR;
		DBGWRT_ERR((DBH,"*** ERROR - %s(): error 0x%08x in MEM_ReadBlocks()\n",
					functionName, retCode ));
		errnoSet( MapErrorCodes(retCode) );
	}/*if*/

	return (status);
}/*MENFLASH_BlkRd*/

/****************************** MENFLASH_BlkWrt *****************************
 *
 *  Description: Block device driver write routine.
 *
 *---------------------------------------------------------------------------
 *  Input......: devP      pointer to block device structure
 *               startBlk  number of the first block to write
 *               numBlks   number of blocks to write
 *               pBuffer   pointer to the write buffer
 *
 *  Output.....: return OK | ERROR
 *
 *  Globals....: errno
 ****************************************************************************/
static STATUS MENFLASH_BlkWrt
(
	MENFLASH_DEV *devP,
	int          startBlk,
	int          numBlks,
	char         *pBuffer
)
{
	DBGCMD( static const char functionName[] = "MENFLASH_BlkWrt()"; )
	struct xbdFlashDev *pXbdDev = (struct xbdFlashDev *)devP;
	int     bytesPerBlk;    /* number of bytes per block */
	int     status;         /* the return code */
	int32   retCode;
	u_int32 flags=0;
	u_int32 subDev;

	status = OK;
	bytesPerBlk = pXbdDev->blk_dev.bd_bytesPerBlk;

    DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, devP->partNbr ));

    /*------------------+
	| read sectors      |
	+------------------*/
	subDev  = devP->partNbr << 4;
	retCode = (devP->memHdl->WriteBlocks)( devP->memHdl, subDev, startBlk,
										   numBlks, (u_int8*)pBuffer, bytesPerBlk,
										   flags );
	if( retCode )
	{
		status = ERROR;
		DBGWRT_ERR((DBH,"*** ERROR - %s():error 0x%08x in MEN_WriteBlocks()\n",
					functionName, retCode ));
		errnoSet( MapErrorCodes(retCode) );
	}/*if*/

	return (status);
}/*MENFLASH_BlkWrt*/


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
	case RBDRV_NOT_SUPPORTED:   return MENFLASH_E_UNKSVC;
	case RBDRV_READ_ERROR:      return MENFLASH_E_READ;
	case RBDRV_WRITE_ERROR:     return MENFLASH_E_WRITE;
	case RBDRV_NOT_READY:       return MENFLASH_E_NOTRDY;
	case RBDRV_WRITE_PROTECTED: return MENFLASH_E_WP;
	case RBDRV_HW_DAMAGE:       return MENFLASH_E_HARDWARE;
	case RBDRV_OUT_OF_RESOURCE: return MENFLASH_E_NORAM;
	case RBDRV_BAD_SUBDEV:      return MENFLASH_E_UNIT;
	case RBDRV_ILLEGAL_SECTOR:  return MENFLASH_E_SECT;
	case RBDRV_BAD_PARAMETER:   return EINVAL;
	case RBDRV_VERIFY_ERROR:    return MENFLASH_E_CRC;
	default:                    return EINVAL;
	}
}/*MapErrorCodes*/

/*--------------------------------------------------------------------------
 *
 * This section of the file contains routines for handling the XBD nature of
 * the ATA driver.  A service task will be created for each controller.
 *
 * The service task will process bio's from the strategy call, and initiate
 * the appropriate disk operation.
 */


/***************************************************************************
*
* getNextBio - get the next bio
*
* /NOMANUAL
*/

LOCAL struct bio * getNextBio
(
	struct xbdFlashDev *  pXbdDev   /* pointer to XBD flash device */
)
{
	struct bio *  retVal;

	semTake (&pXbdDev->mutex, WAIT_FOREVER);
	retVal = pXbdDev->bio_queueh;
	if (retVal != NULL)
	{
		pXbdDev->bio_queueh = retVal->bio_chain;
		retVal->bio_chain = NULL;

		if (pXbdDev->bio_queueh == NULL)
			pXbdDev->bio_queuet = NULL;
	}

	semGive (&pXbdDev->mutex);
	return (retVal);
}

/***************************************************************************
*
* execBio - the execBio routine
*
* /NOMANUAL
*/

LOCAL void execBio
(
	struct xbdFlashDev *  pXbdDev,   /* pointer to XBD flash device */
	struct bio *        bio       /* pointer to bio */
)
{
	int        status = ERROR;
	unsigned   bd_bsize = pXbdDev->fDev.xbd.xbd_blocksize;
	sector_t   bd_blocks = pXbdDev->fDev.xbd.xbd_nblocks;
	sector_t   nblocks;
	BLK_DEV *  bd = &pXbdDev->blk_dev;
	unsigned   size;

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


	if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK) {
		bio_done (bio, errno);
		return;
	}

	if (bio->bio_flags & BIO_READ)
	{
		status = bd->bd_blkRd (pXbdDev, (int) bio->bio_blkno,
							   (int) nblocks, bio->bio_data);
	}
	else if (bio->bio_flags & BIO_WRITE)
	{

		status = bd->bd_blkWrt (pXbdDev, (int)bio->bio_blkno,
								(int) nblocks, bio->bio_data);
	}

	semGive (&pXbdDev->mutex);

	if (status == OK)
	{

		bio_done (bio, 0);
	}
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
* menFlashXbdService - XBD block device service routine
*
* /NOMANUAL
*/

LOCAL void menFlashXbdService
(
	struct xbdFlashDev *  pXbdDev   /* pointer to XBD flash device */
)
{
	struct bio *  bio;

	while( TRUE )
	{
		/* Await our semaphore */
		if( semTake (&pXbdDev->bio_ready, WAIT_FOREVER) == ERROR)
			 return;

		while ((bio = getNextBio (pXbdDev)) != NULL )
		{
			execBio (pXbdDev, bio);
		}
	}
}


/***************************************************************************
*
* menFlashXbdIoctl - XBD block device ioctl routine
*
* /NOMANUAL
*/

LOCAL int menFlashXbdIoctl
(
	struct xbd *  self,
	int           command,
	void *        arg
)
{
	struct xbdFlashDev *  pXbdDev = (struct xbdFlashDev *) self;
	MENFLASH_DEV *        devP = (MENFLASH_DEV *) self;
	device_t              dev = devP->xbd.xbd_dev.dv_dev;
	INT16                 reType;
	BLK_DEV *             bd = &pXbdDev->blk_dev;
	XBD_GEOMETRY *        geo;
	int                   status = 0;
	struct bio *          bio;

	DBGWRT_1((DBH,"menFlashXbdIoctl %x, %x\n",command,arg));

	switch (command)
	{
	case XBD_SOFT_EJECT:
	case XBD_HARD_EJECT:
		/* we are the base XBD so if we get these it is for us */
	    /*
	     * first raise a removal event
	     * then purge the bio queue
	     * last raise an insertion event
	     */
		DBGWRT_2((DBH,"XbdIoCtl::   EJECT\n"));
		if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK) {
			status = ERROR;
			break;
		}

		erfEventRaise (xbdEventCategory, xbdEventRemove, ERF_ASYNC_PROC,
					   (void *)dev, NULL);

		while( (bio = getNextBio(pXbdDev)) )
			bio_done(bio, ENXIO);

		reType = (command == XBD_HARD_EJECT) ?
				 xbdEventPrimaryInsert : xbdEventSoftInsert;
		erfEventRaise (xbdEventCategory, reType, ERF_ASYNC_PROC,
					   (void *)dev, NULL);
		semGive (&pXbdDev->mutex);
		status = OK;
		break;

	case XBD_GETGEOMETRY:
		DBGWRT_2((DBH,"XbdIoCtl::   GETGEOMETRY\n"));
		geo = (XBD_GEOMETRY *) arg;
		geo->heads = bd->bd_nHeads;
		geo->secs_per_track = bd->bd_blksPerTrack;
		geo->total_blocks = bd->bd_nBlocks;
		geo->blocksize = bd->bd_bytesPerBlk;
		geo->cylinders = geo->blocksize / geo->secs_per_track / geo->heads;
		status = OK;
		break;

	case XBD_GETBASENAME:
		DBGWRT_2((DBH,"XbdIoCtl::   GETBASENAME\n"));
		if (devName(dev, arg) == ERROR)
			status = errno;
		else
			status = OK;
		break;

	case XBD_SYNC:
		DBGWRT_2((DBH,"XbdIoCtl::   SYNC\n"));
		if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK) {
			status = ERROR;
			break;
		}
		bd->bd_ioctl(devP, FIOSYNC, 0);
		status = OK;
		semGive (&pXbdDev->mutex);
		break;

	case XBD_TEST:
		DBGWRT_2((DBH,"XBD_TEST just return OK\n"));
		status = OK;
		break;
	case XBD_STACK_COMPLETE:
		DBGWRT_2((DBH,"XbdIoCtl::   STACK COMPLETE\n"));
		if (!pXbdDev->xbd_instantiated)
		{
			if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK) {
				status = ERROR;
				break;
			}
			erfEventRaise (xbdEventCategory, xbdEventInstantiated,
						   ERF_ASYNC_PROC,
						   devP->xbd.xbd_dev.dv_xname, NULL);
			pXbdDev->xbd_instantiated = TRUE;
			semGive (&pXbdDev->mutex);
		}
		status = OK;
		break;

	default:
		DBGWRT_2((DBH,"XbdIoCtl::   MEN IOCTL CODE??\n"));
		if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK) {
			status = ERROR;
			break;
		}
		status = (int)bd->bd_ioctl(devP, command, arg);
		semGive (&pXbdDev->mutex);
		break;
	}

	return(status);
}

/***************************************************************************
*
* menFlashXbdCreateSyncHandler -
*/

LOCAL void menFlashXbdCreateSyncHandler
(
	UINT16 Category,
	UINT16 Type,
	void * eventData,
	void * userData
)
{
	struct xbdFlashDev *  pXbdDev;
	devname_t *         devname;

	if ((Category == xbdEventCategory) && (Type == xbdEventInstantiated))
	{
		devname = (devname_t *) eventData;
		pXbdDev = (struct xbdFlashDev *) userData;

		if (strncmp ((char*) devname,
					 (char *) pXbdDev->fDev.xbd.xbd_dev.dv_xname,
					 sizeof (devname_t)) != 0)
		{
			return;
		}

		erfHandlerUnregister (xbdEventCategory, xbdEventInstantiated,
							  menFlashXbdCreateSyncHandler, userData);

		semGive (&pXbdDev->xbd_semId);
	}

	return;
}

/***************************************************************************
*
* menFlashXbdStrategy - XBD flash device strategy routine
*
* /NOMANUAL
*/

LOCAL int menFlashXbdStrategy
(
	struct xbd *  self,       /* pointer to XBD */
	struct bio *  bio         /* pointer to bio */
)
{
	struct xbdFlashDev * pXbdDev = (struct xbdFlashDev *) self;
	struct bio *       next_bio;

	if (semTake (&pXbdDev->mutex, WAIT_FOREVER) != OK)
		return errno;

	/* Iterate through the chain, running each bio as we get it */
	for ( ; bio != NULL; bio = next_bio )
	{
		/* Make sure that we can get the next one */
		next_bio = bio->bio_chain;


		/* Add this bio as the tail of the queue */
		bio->bio_chain = NULL;

		if (pXbdDev->bio_queuet)
			pXbdDev->bio_queuet->bio_chain = bio;
		else
			pXbdDev->bio_queueh = bio;

		pXbdDev->bio_queuet = bio;

		/* Tap our service task */
		semGive (&pXbdDev->bio_ready);
	}

	semGive (&pXbdDev->mutex);
	return (OK);
}

/***************************************************************************
*
* menFlashXbdDump - XBD flash device dump routine
*
* /NOMANUAL
*/

LOCAL int menFlashXbdDump
(
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
* MENFLASH_XbdDevDelete - deletes an XBD flash device
*
* This routine deletes or destroys an XBD flash device .
*
* The <d> parameter specifies the XBD flash device  to delete. This should be
* the same value that was returned from MENFLASH_PartionSet()
*
* RETURNS: a device identifier upon success, or NULLDEV otherwise
*
* ERRNO:
*/

STATUS MENFLASH_XbdDevDelete
(
	device_t     d   /* device_t returned from MENFLASH_PartionSet */
)
{
	struct xbdFlashDev *  pXbdDev;
	struct bio * bio;

	pXbdDev = (struct xbdFlashDev *) devMap (d);

	if (pXbdDev == NULL)
		return (ERROR);

	/* Grab the semaphore */
	semTake (&pXbdDev->mutex, WAIT_FOREVER);

	/* Detach the XBD so no one else can access it */
	xbdDetach (&pXbdDev->fDev.xbd);
	devUnmap ((struct device *)pXbdDev);

	/* Drain the bios */
	while( (bio = getNextBio (pXbdDev)) )
		bio_done (bio, ENXIO);

	/* Free all resources */
#	ifdef MEN_A9
	    MENA9FLASH_DevRemove
#	elif defined (MEN_A11)
	   MENA11FLASH_DevRemove
#	elif defined (MEN_F006)
	   MENF006FLASH_DevRemove
#	elif defined (MEN_A015)
	   MENA015FLASH_DevRemove
#	elif defined (MEN_PP01)
	   MENPP01FLASH_DevRemove
#	elif defined (MEN_KM01)
	   MENKM01FLASH_DevRemove
#	elif defined (MEN_EM01)
	   MENEM01FLASH_DevRemove
#	elif defined (MEN_EM10A)
	   MENEM10AFLASH_DevRemove
#	elif defined (MEN_EM03)
	   MENEM03FLASH_DevRemove
#	elif defined (MEN_EM09)
	   MENEM09FLASH_DevRemove
#	elif defined (MEN_EP01)
	   MENEP01FLASH_DevRemove
#	elif defined (MEN_A021)
	   MENA021FLASH_DevRemove
#	else
#	   error ("board type not supported")
#	endif /*MEN_A9*/
	( (MENFLASH_DEV *)pXbdDev );

	/* Finally send a removal event to whatever maybe on top of us */
	erfEventRaise(xbdEventCategory, xbdEventRemove,
		ERF_SYNC_PROC, (void *)d, NULL);

	return (OK);
}



