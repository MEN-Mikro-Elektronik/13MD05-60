/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: menflash.c
 *      Project: PCMCIA
 *
 *       Author: uf
 *        $Date: 2011/12/19 18:32:11 $
 *    $Revision: 1.22 $
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
 *               MEN_A11, MEN_F006, MEN_A015, MEN_SC15A
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
 *       block device        +-----------------+
 *       driver              |    menflash     |
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
 * $Log: menflash.c,v $
 * Revision 1.22  2011/12/19 18:32:11  ts
 * R: New Board EM10A as successor of EM01A was introduced
 * M: added support for EM10A
 *
 * Revision 1.21  2007/05/29 15:17:08  cs
 * added:
 *    - EP4
 *
 * Revision 1.20  2007/03/02 10:14:04  SYao
 * debugged some "debug information" :)
 *
 * Revision 1.19  2007/02/28 18:27:22  SYao
 * Add support for SC15A
 *
 * Revision 1.18  2006/07/17 12:13:42  ufranke
 * cosmetics
 *
 * Revision 1.17  2006/07/13 10:17:18  RLange
 * Cosmetics
 *
 * Revision 1.16  2006/07/12 15:24:28  RLange
 * Added EP01
 *
 * Revision 1.15  2006/06/07 10:48:46  cs
 * bugfix for A15: DevCreate Routine was named for EM3 by mistake
 *
 * Revision 1.14  2006/02/28 17:08:20  UFRANKE
 * cosmetics
 *
 * Revision 1.13  2006/02/22 11:50:42  ts
 * corrected EM03 Partion Set
 *
 * Revision 1.12  2006/02/14 13:13:29  ts
 * + EM03 Flash
 *
 * Revision 1.11  2005/09/07 19:00:36  CSchuster
 * added
 *     + EM01
 *
 * Revision 1.10  2005/04/12 16:56:19  kp
 * + EM01 (untested)
 *
 * Revision 1.9  2005/03/01 15:33:50  ufranke
 * added
 *  + KM01 support
 * fixed
 *  - ren MENP01FLASH_DevRemove to MENPP01FLASH_DevRemove
 *
 * Revision 1.8  2003/05/26 14:49:46  UFranke
 * added
 *  - PP01 support
 *
 * Revision 1.7  2003/03/12 15:14:24  UFranke
 * added A015 support
 *
 * Revision 1.6  2002/09/12 15:10:16  UFranke
 * added F006 support
 *
 * Revision 1.5  2000/04/27 10:51:35  Franke
 * bugfix read/write return now with ERROR on error
 *
 * Revision 1.4  2000/04/25 18:21:32  Franke
 * added A11 support
 *
 * Revision 1.3  1998/08/20 10:13:14  Franke
 * documentation upgrade
 *
 * Revision 1.2  1998/08/19 16:32:37  Franke
 * added support of device partitions
 *
 * Revision 1.1  1998/07/23 13:57:40  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998..2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char RCSid[]="MENFLASH - block device driver: $Id: menflash.c,v 1.22 2011/12/19 18:32:11 ts Exp $";

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

#include "MEN/men_typs.h"
#include "MEN/maccess.h"
#include "MEN/dbg.h"
#include "MEN/oss.h"
#include "MEN/rbdioctl.h"
#include "MEN/rbdrv.h"
#include "MEN/mtdioctl.h"
#include "MEN/mtd.h"
#include "MEN/memioctl.h"
#include "MEN/mem_drv.h"
#include "MEN/menflash.h"

#ifdef MEN_EM03
	#include <MEN/em03_cfg.h>
#endif
#ifdef MEN_PP01
	#include <MEN/pp01_cfg.h>
#endif
#ifdef MEN_KM01
	#include <MEN/km01_cfg.h>
#endif
#ifdef MEN_EM01
	#include <MEN/em01_cfg.h>
#endif 
#ifdef MEN_EP01
	#include <MEN/ep01_cfg.h>
#endif
#ifdef MEN_EP04
	#include <MEN/ep04_cfg.h>
#endif
#ifdef MEN_EM10A
    #include <MEN/em10a_cfg.h>
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define DBG_MYLEVEL     devP->dbgLevel
#define DBH             devP->dbgHdl

#define A9_FLASH_BASE   0x04000000
#define A9_FLASH_SIZE   0x00800000
#define A11_FLASH_BASE_16MB 0xFF000000
#define A11_FLASH_SIZE_16MB 0x01000000
#define A11_FLASH_SIZE_8MB  0x00800000

#define F006_FLASH_BASE       0xfe000000
#define F006_FLASH_SIZE_8MB   0x00800000
#define F006_FLASH_SIZE_16MB  0x01000000
#define F006_FLASH_SIZE_32MB  0x02000000

#define A015_FLASH_BASE       0x78000000
#define A015_FLASH_SIZE_32MB  0x02000000


#define SC15A_FLASH_BASE		(0xF0000000UL)
#define SC15A_FLASH_SIZE_256MB	(0x10000000UL)

#define IDENT_BUF_SIZE  0x1000
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
#elif defined (MEN_EM03)
	MTD_Em03Flash_Try
#elif defined (MEN_EP01)
	MTD_EP01Flash_Try
#elif defined (MEN_EP04)
	MTD_Ep04Flash_Try
#elif defined (MEN_SC15A)
	MTD_SC15A_Flash_Try
#elif defined (MEN_EM10A)
	MTD_EM10A_Flash_Try
#else
#	error "*** MEN_A9, MEN_A11, MEN_A015, MEN_PP01, MEN_KM01, MEN_F006, MEN_EM01, MEN_EM03, MEN_EP01, MEN_EP04 or MEN_SC15A must be defined"
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
#elif defined (MEN_EP01)
    printf("           for EP01\n");
#elif defined (MEN_EP04)
    printf("           for EP04\n");
#elif defined (MEN_EM03)
    printf("           for EM03\n");
#elif defined (MEN_SC15A)
	printf("           for SC15A\n");
#elif defined (MEN_EM10A)
	printf("           for EM10A\n");
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
 *                        PP01  - 0xFF000000 - 16MB, 0xFF800000 - 8MB
 *                        KM01  - 0xFF800000
 *                        EM01  - 0xFF800000
 *                        EM03  - 0xFF800000 - 8 MB
 *						  SC15A	- 0xF0000000
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
 *                        PP01
 *                        	0x00E80000      - get 14.5MB - 16MB
 *                        	0x00680000      - get  6.5MB -  8MB
 *                        KM01
 *                        	0x00700000      - get  7MB
 *						  SC15A
 *							0x10000000      -get 256MB
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
#elif defined (MEN_EM03)
   MENEM03FLASH_DevCreate
#elif defined (MEN_EP01)
   MENEP01FLASH_DevCreate
#elif defined (MEN_EP04)
   MENEP04FLASH_DevCreate
#elif defined (MEN_SC15A)
   MENSC15AFLASH_DevCreate
#elif defined (MEN_EM10A)
   MENEM10AFLASH_DevCreate
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
    devP = (MENFLASH_DEV *) malloc( sizeof(MENFLASH_DEV) );
    if( devP == NULL )
        return( NULL );                 /* no memory */

    /* fill turkey with 0 */
    OSS_MemFill( OSS_VXWORKS_OS_HDL, sizeof(MENFLASH_DEV), (char*) devP, 0 );


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
        mtdTry = MTD_EM10A_Flash_Try;
        if( (int)addr < EM10CFG_FLASH_8MB ) {
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
	#elif defined (MEN_EP04)
        mtdTry = MTD_Ep04Flash_Try;
        if( (int)addr < EP04CFG_FLASH_ADDR
            || (EP04CFG_SMM_BASE) < (unsigned int)addr
            || (EP04CFG_SMM_BASE) < (unsigned int)addr + size )
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
        } else {
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
    #elif defined (MEN_SC15A)

    	mtdTry = MTD_SC15A_Flash_Try;
      	if( ((int)addr < SC15A_FLASH_BASE) ||
			((SC15A_FLASH_BASE + (SC15A_FLASH_SIZE_256MB-1)) < (int)addr) )
        {
            DBGWRT_ERR((DBH,"*** ERROR %s(): addr 0x%08x out of range\n",
						functionName, addr ));
            goto CLEANUP;
        }

        jedInfo.manfId   = 0;
        jedInfo.deviceId = 0;
        knownSize        = size;
        if( size < 1 ){

            DBGWRT_ERR((DBH,"*** ERROR %s(): size 0x%08x out of range\n",
						functionName, size ));
            goto CLEANUP;
        }
        else{
            maxPhySearch = size;
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
#elif defined (MEN_EM03)
   MENEM03FLASH_DevRemove
#elif defined (MEN_EP01)
   MENEP01FLASH_DevRemove
#elif defined (MEN_EP04)
   MENEP04FLASH_DevRemove
#elif defined (MEN_SC15A)
   MENSC15AFLASH_DevRemove
#elif defined (MEN_EM10A)
   MENEM10AFLASH_DevRemove
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
BLK_DEV*
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
#elif defined (MEN_EM03)
   MENEM03FLASH_PartionSet
#elif defined (MEN_EP01)
   MENEP01FLASH_PartionSet
#elif defined (MEN_EP04)
   MENEP04FLASH_PartionSet
#elif defined (MEN_SC15A)
   MENSC15AFLASH_PartionSet
#elif defined (MEN_EM10A)
   MENEM10AFLASH_PartionSet
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

    DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, partNbr ));

    if( partNbr > 7 )
	{
        DBGWRT_ERR((DBH,"*** ERROR - %s():parameter partNbr %d out of range\n",
					functionName, partNbr ));
		return( NULL );
    }/*if*/

    if( devP->partNbr != 0 || devP == NULL )
	{
        DBGWRT_ERR((DBH,"*** ERROR - %s(): illegal device pointer\n",
					functionName ));
		return( NULL );
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
        devPartitionP = (MENFLASH_DEV *) malloc( sizeof(MENFLASH_DEV) );
        if( devPartitionP == NULL )
		{
            DBGWRT_ERR((DBH,"*** ERROR - %s(): no memory\n", functionName ));
            return( NULL );                 /* no memory */
		}/*if*/

        /* copy the MENFLASH_DEV structure */
		*devPartitionP = *devP;

        /* set the partion number */
        devPartitionP->partNbr = partNbr;

        pBlkDev = &devPartitionP->blkdev;
    }
	else
	{
        pBlkDev = &devP->blkdev;
	}/*if*/

    /* Initialize BLK_DEV structure (in MENFLASH_DEV) */
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

    return( pBlkDev );

CLEANUP:
    return( NULL );
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

    DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, devP->partNbr ));

    if( code < MENFLASH_IOCTL_FLASH_ERASE || MENFLASH_IOCTL_INFO < code )
    {
        switch( code )
        {
            case MENFLASH_IOCTL_IDENT:
                 MENXXFLASH_Ident();
                 buf = malloc( IDENT_BUF_SIZE );
                 if( buf )
                 {
                     (*devP->memHdl->Ident)(devP->memHdl, buf, IDENT_BUF_SIZE);
                     printf( buf );
                     free( buf );
                 }/*if*/
               break;

            default:
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
                code = MEM_IOCTL_FLASH_ERASE;
               break;
            case MENFLASH_IOCTL_FLASH_SECTOR_SIZE:
                code = MEM_IOCTL_FLASH_SECTOR_SIZE;
               break;
            case MENFLASH_IOCTL_INFO:
                code = MTD_IOCTL_INFO;
               break;
            default:
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
    int     bytesPerBlk;    /* number of bytes per block */
    int     status;         /* the return code */
    int32   retCode;
    u_int32 flags=0;
	u_int32 subDev;

    status = OK;
    bytesPerBlk = devP->blkdev.bd_bytesPerBlk;

    DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, devP->partNbr ));

    /*------------------+
    | read sectors      |
    +------------------*/
	subDev  = devP->partNbr << 4;
    retCode = (devP->memHdl->ReadBlocks)( devP->memHdl, subDev,
										  startBlk, numBlks, pBuffer,
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
    int     bytesPerBlk;    /* number of bytes per block */
    int     status;         /* the return code */
    int32   retCode;
    u_int32 flags=0;
	u_int32 subDev;

    status = OK;
    bytesPerBlk = devP->blkdev.bd_bytesPerBlk;

    DBGWRT_1((DBH,"%s  partNbr %d\n", functionName, devP->partNbr ));

    /*------------------+
    | read sectors      |
    +------------------*/
	subDev  = devP->partNbr << 4;
    retCode = (devP->memHdl->WriteBlocks)( devP->memHdl, subDev, startBlk,
										   numBlks, pBuffer, bytesPerBlk,
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
