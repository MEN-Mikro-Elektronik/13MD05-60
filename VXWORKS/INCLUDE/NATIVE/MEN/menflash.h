/***********************  I n c l u d e  -      F i l e  ************************
 *
 *        Name: menflash.h
 *
 *      Author: uf
 *       $Date: 2007/07/20 12:11:57 $
 *   $Revision: 1.13 $
 *
 * Description: men raw flash driver interface
 *
 *    Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: menflash.h,v $
 * Revision 1.13  2007/07/20 12:11:57  cs
 * added:
 *   + support for EM9
 *
 * Revision 1.12  2007/05/29 14:20:21  cs
 * added EP04
 *
 * Revision 1.11  2006/12/22 15:14:16  cs
 * added:
 *    + support for XBD driver (vxW >= 6.2)
 *
 * Revision 1.10  2006/07/12 15:33:24  RLange
 * Added EP01
 *
 * Revision 1.9  2006/01/24 12:16:42  ts
 * + EM03
 *
 * Revision 1.8  2005/09/09 19:59:37  CSchuster
 * added
 *     + EM01
 *
 * Revision 1.7  2005/03/14 14:30:10  ufranke
 * added
 *  + KM01
 *
 * Revision 1.6  2003/05/26 14:49:48  UFranke
 * added
 *  - PP01 support
 *
 * Revision 1.5  2003/03/12 14:49:52  UFranke
 * added A015
 *
 * Revision 1.4  2002/09/13 10:16:35  UFranke
 * added F6
 *
 * Revision 1.3  2000/04/27 13:29:44  Franke
 * added A11 support
 *
 * Revision 1.2  1998/08/19 16:31:22  Franke
 * added support of device partitions
 *
 * Revision 1.1  1998/07/23 14:02:14  Franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _INC_MEN_FLASH
#   define _INC_MEN_FLASH

#ifdef __cplusplus
      extern "C" {
#endif

#include "blkIo.h"
#ifdef USE_MENFLASHXBD
	#include "drv/xbd/xbd.h"
#endif /* USE_MENFLASHXBD */


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
#ifdef COMPILE_MENFLASH
typedef struct              /* MENFLASH_DEV - flash disk device descriptor */
{
#ifdef USE_MENFLASHXBD
    XBD           xbd;     	/* XBD or device */
#else
    BLK_DEV     blkdev;     /* generic block device structure */
#endif
	u_int32     partNbr;    /* partion number                 */
    u_int32     dbgLevel;   /* current debug level            */
    DBG_HANDLE* dbgHdl;     /* debug handle                   */
    MEM_HANDLE* memHdl;     /* mem driver handle              */
}MENFLASH_DEV;
#else
   typedef void* MENFLASH_DEV;
#endif /*COMPILE_MENFLASH*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*--- Ioctl function codes ---*/
#define MENFLASH_IOCTL_OFFSET               0x1000
#define MENFLASH_IOCTL_IDENT                MENFLASH_IOCTL_OFFSET
#define MENFLASH_IOCTL_FLASH_ERASE          MENFLASH_IOCTL_OFFSET + 1
#define MENFLASH_IOCTL_FLASH_SECTOR_SIZE    MENFLASH_IOCTL_OFFSET + 2
#define MENFLASH_IOCTL_INFO                 MENFLASH_IOCTL_OFFSET + 3

/*--- error codes ---*/
#define MENFLASH_E_OFFSET                   0x1000
#define MENFLASH_E_UNKSVC                   0x01 + MENFLASH_E_OFFSET
#define MENFLASH_E_READ                     0x02 + MENFLASH_E_OFFSET
#define MENFLASH_E_WRITE                    0x03 + MENFLASH_E_OFFSET
#define MENFLASH_E_NOTRDY                   0x04 + MENFLASH_E_OFFSET
#define MENFLASH_E_WP                       0x05 + MENFLASH_E_OFFSET
#define MENFLASH_E_HARDWARE                 0x06 + MENFLASH_E_OFFSET
#define MENFLASH_E_NORAM                    0x07 + MENFLASH_E_OFFSET
#define MENFLASH_E_UNIT                     0x08 + MENFLASH_E_OFFSET
#define MENFLASH_E_SECT                     0x09 + MENFLASH_E_OFFSET
#define MENFLASH_E_CRC                      0x0A + MENFLASH_E_OFFSET

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
	extern MENFLASH_DEV* MENA9FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENA11FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENF006FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENA015FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENPP01FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENKM01FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENEM01FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENEP01FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENEP04FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENEM03FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

	extern MENFLASH_DEV* MENEM09FLASH_DevCreate
	(
	    char     *addr,
	    u_int32  size,
	    u_int32  dbgLevel
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENFLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENFLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENF006FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENF006FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENA015FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENA015FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENPP01FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENPP01FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENKM01FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENKM01FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENEM01FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENEM01FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENEM03FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENEM03FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENEM09FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENEM09FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
(
    MENFLASH_DEV    *devP,
	u_int32         part,
	u_int32         partStart,
	u_int32         partSize
);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENEP01FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENEP01FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifndef USE_MENFLASHXBD
	extern BLK_DEV* MENEP04FLASH_PartionSet
#else /* USE_MENFLASHXBD */
	extern device_t MENEP04FLASH_PartionSet
#endif /* USE_MENFLASHXBD */
	(
	    MENFLASH_DEV    *devP,
		u_int32         part,
		u_int32         partStart,
		u_int32         partSize
	);

#ifdef USE_MENFLASHXBD
	extern STATUS   MENFLASH_DevDelete      (device_t d);
#endif/* USE_MENFLASHXBD */

#ifdef __cplusplus
    }
#endif

#endif /*_INC_MEN_FLASH */



