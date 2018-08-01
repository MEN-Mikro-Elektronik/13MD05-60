/***********************  I n c l u d e  -      F i l e  ************************
 *
 *        Name: menpcmcia.h
 *
 *      Author: uf
 *       $Date: 1999/06/18 16:25:47 $
 *   $Revision: 1.3 $
 *
 * Description: men PCMCIA driver interface
 *
 *    Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: menpcmcia.h,v $
 * Revision 1.3  1999/06/18 16:25:47  Franke
 * added   MENPCMCIA_MountRawFs() - for test purposes only !!!
 *         ioctl codes MENPCMCIA_IOCTL_NBLOCKS, MENPCMCIA_IOCTL_NHEADS
 * 			 MENPCMCIA_IOCTL_NBLK_PER_TRACK and MENPCMCIA_IOCTL_NBYTE_PER_BLK
 * renamed MENPCMCIA_UnmountDosFs to MENPCMCIA_UnmountFs
 *
 * Revision 1.2  1999/05/06 10:41:13  Franke
 * added M20 support
 * renamed MENA9PCMCIA_DevRemove to MENPCMCIA_DevRemove
 *
 * Revision 1.1  1998/07/28 14:50:33  Franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _INC_MEN_PCMCIA
#   define _INC_MEN_PCMCIA

#ifdef __cplusplus
      extern "C" {
#endif

#include "blkIo.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef struct
{
    void*            pcmciaRbd;	   /* pointer to raw block device */
    int              csTaskId;     /* csc polling task */
	int32            (*setup)(int sock, RBDRV_HANDLE *, int createFlag, void *osCscData );
    int              reserved;
}MENPCMCIA_OS_CSC_DATA;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define MENPCMCIA_MAX_SOCKETS                1

#define MENPCMCIA_CSC_ONE_SHOT               0
#define MENPCMCIA_CSC_POLL					 1
#define MENPCMCIA_CSC_IRQ					 2

/*--- Ioctl function codes ---*/
#define MENPCMCIA_IOCTL_OFFSET               0x1000
#define MENPCMCIA_IOCTL_IDENT                (MENPCMCIA_IOCTL_OFFSET + 0)
#define MENPCMCIA_IOCTL_NBLOCKS              (MENPCMCIA_IOCTL_OFFSET + 1)
#define MENPCMCIA_IOCTL_NHEADS               (MENPCMCIA_IOCTL_OFFSET + 2)
#define MENPCMCIA_IOCTL_NBLK_PER_TRACK       (MENPCMCIA_IOCTL_OFFSET + 3)
#define MENPCMCIA_IOCTL_NBYTE_PER_BLK        (MENPCMCIA_IOCTL_OFFSET + 4)


/*--- error codes ---*/
#define MENPCMCIA_E_OFFSET                   0x1000
#define MENPCMCIA_E_UNKSVC                   0x01 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_READ                     0x02 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_WRITE                    0x03 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_NOTRDY                   0x04 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_WP                       0x05 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_HARDWARE                 0x06 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_NORAM                    0x07 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_UNIT                     0x08 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_SECT                     0x09 + MENPCMCIA_E_OFFSET
#define MENPCMCIA_E_CRC                      0x0A + MENPCMCIA_E_OFFSET


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern void	*PCMCIA_M20_BaseAddr;
extern int	 PCMCIA_M20_Vector;
extern int	 PCMCIA_M20_Level;

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
extern int32 MENA9PCMCIA_DevCreate
(
    u_int32  sock,
    u_int32  cscMode,					 
    u_int32  dbgLevel,
	void**	 menPcmciaP
);

extern int32 MENM20PCMCIA_DevCreate
(
    u_int32  sock,
    u_int32  cscMode,					 
    u_int32  dbgLevel,
	void**	 menPcmciaP
);

extern int32 MENPCMCIA_DevRemove
(
	void**	 menPcmciaP
);

extern int32 MENPCMCIA_MountDosFs
(
    void*   menPcmcia,
    int	    sock,
    char*   pName
);
extern int32 MENPCMCIA_InitAndMountDosFs
(
    void*   menPcmcia,
    int	    sock,
    char*   pName
);
extern int32 MENPCMCIA_UnmountFs
(
    void*   menPcmcia,
    int	    sock
);
extern int32 MENPCMCIA_MountRawFs  /* only for test purposes */
(
    void*   menPcmcia,
    int     sock,
    char*   pName,
    u_int32 flags
);



#ifdef __cplusplus
    }
#endif

#endif /*_INC_MEN_PCMCIA */

