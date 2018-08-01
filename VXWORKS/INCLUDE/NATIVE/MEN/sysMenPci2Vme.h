/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysMenPci2Vme.h
 *
 *       Author: kp
 *        $Date: 2012/09/10 15:11:33 $
 *    $Revision: 1.8 $
 *
 *  Description: VME interrupts
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysMenPci2Vme.h,v $
 * Revision 1.8  2012/09/10 15:11:33  sy
 * Add support to Vxbus Driver of VME ip core
 *
 * Revision 1.7  2009/02/06 21:16:03  cs
 * R: no API was provided for setting bus release and request modi
 * M: add defines and prototypes for bus release and request modi
 *    (sysVmeReleaseMode(), sysVmeRequesterMode())
 *
 * Revision 1.6  2005/11/25 14:58:55  UFranke
 * added
 *  + sysMenPci2VmeSetA32MasterPrefix
 *  + sysVmeA32SlaveAddressSet
 *  + sysMenPci2VmeDma
 *
 * Revision 1.5  2005/10/24 13:19:37  UFranke
 * added
 *  + SYS_VME_A32_D64(_SWAPPED)
 *
 * Revision 1.4  2005/10/24 09:49:30  UFranke
 * MEN_PLD_Z002 used on CPU boards A15/A13:
 * added
 *  + access directly to main memory via slave window
 * changed
 *  - interface changed sysVme-A24/A32-SlaveAddressSet()
 *
 * Revision 1.3  2004/02/05 10:38:44  UFranke
 * added
 *  + sysVmeA32SlaveAddressSet(), sysVmeA24SlaveAddressSet() and
 *    sysVmeA16SlaveAddressSet
 *
 * Revision 1.2  2003/12/19 12:30:42  UFranke
 * added
 *  + A15/A13 PLDZ002 support
 *
 * Revision 1.1  2002/04/30 14:35:00  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef	__INCsysMenPci2Vme
#define	__INCsysMenPci2Vme

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEN_PLD_Z002
	#define SYS_VME_WIN_SRAM		0
	#define SYS_VME_WIN_MAIN_MEM	1
#endif /*MEN_PLD_Z002*/

#define SYS_VME_A24_D16				0x01
#define SYS_VME_A24_D32				0x02
#define SYS_VME_A24_D64             0x03
#define SYS_VME_A32_D16				0x04
#define SYS_VME_A32_D32				0x05
#define SYS_VME_A32_D64				0x06
#define SYS_VME_CR_CSR              0x07
#define SYS_VME_AXX_DXX_SWAPPED		0x10  /* swapped flag */
#define SYS_VME_A24_D16_SWAPPED		0x11
#define SYS_VME_A24_D32_SWAPPED		0x12
#define SYS_VME_A24_D64_SWAPPED     0x13
#define SYS_VME_A32_D16_SWAPPED		0x14
#define SYS_VME_A32_D32_SWAPPED		0x15
#define SYS_VME_A32_D64_SWAPPED		0x16

#define SYS_VME_LM_ADDR_MODE_A16	0
#define SYS_VME_LM_ADDR_MODE_A24	1
#define SYS_VME_LM_ADDR_MODE_A32	2

typedef enum {
	SYS_VME_REL_MODE_GET	=	0x00,	/* just return current bus release mode */
	SYS_VME_REL_MODE_ROR	=	0x01,	/* set bus release mode to Release-On-Request*/
	SYS_VME_REL_MODE_RWD	=	0x02	/* set bus release mode to Release-When-Done */
} SYS_VME_REL_MODE;

typedef enum {
	SYS_VME_REQ_MODE_GET	=	0x00,	/* just return current bus requester mode */
	SYS_VME_REQ_MODE_DEMD	=	0x01,	/* set bus requester mode to demand */
	SYS_VME_REQ_MODE_FAIR	=	0x02,	/* set bus requester mode to fair */
} SYS_VME_REQ_MODE;

IMPORT char*	sysMenPci2VmeIdent( void );
#ifdef MEN_VME_VXB_SUPPORT
IMPORT int   	sysMenPci2VmeInit( VXB_DEVICE_ID );
#else
IMPORT int   	sysMenPci2VmeInit( void );
#endif
IMPORT void  	sysMenPci2VmeIntr (void);
IMPORT int   	sysAddrInVmeRange( char* adrs );
IMPORT STATUS 	sysVmeProbe
    (
    char   * adrs,	 /* address to be probed */
    int      mode,	 /* VX_READ or VX_WRITE */
    int      length, /* 1, 2 or 4 byte probe */
    char   * pVal	 /* address of value to write OR */
			         /* address of location to place value read */
    );
IMPORT STATUS	sysVmeAcFailIntEnable(void);
IMPORT STATUS	sysVmeAcFailIntDisable(void);
IMPORT STATUS	sysVmeIsSystemController(void);
IMPORT STATUS	sysVmeReset(void);
IMPORT STATUS sysVmeLocMonSetup
(
	int  	mon,			/* monitor 0 or 1 */
	u_int8  storeAddrBits,	/* not used */
	int  	wr,				/* monitor write access */
	int  	rd,				/* monitor read access */
	int  	addrMode,		/* SYS_VME_LM_ADDR_MODE_Axx xx 16/24/32 */
	u_int32 addr,			/* compare addr */
	int  	enable 			/* 1 - enable irq */
);
IMPORT STATUS sysVmeMailboxIrqSetup
(
	int mbox,   /* mailbox 0..3 */
	int wr,		/* at write */
	int rd,		/* at read */
	int enable
);

IMPORT STATUS sysVmeA16SlaveAddressSet
(
	int     enable,
	u_int16 slaveAddr
);

IMPORT SYS_VME_REL_MODE sysVmeBusReleaseMode
(
	SYS_VME_REL_MODE relMode
);

IMPORT SYS_VME_REQ_MODE sysVmeBusRequesterMode
(
	SYS_VME_REQ_MODE reqMode
);

#ifndef MEN_PLD_Z002
	IMPORT STATUS sysVmeA24SlaveAddressSet
	(
		int     enable,
		u_int32 slaveAddr
	);
#else
#ifdef MEN_VME_VXB_SUPPORT
	IMPORT void sysVmeShow( VXB_DEVICE_ID pciCtrl, int unit  );
#else
	IMPORT void sysVmeShow( int unit  );
#endif
	IMPORT void sysVmePLDZ002setEndianMode( int littleMode );

	IMPORT u_int32 sysMenPci2VmeSetA32MasterPrefix(	u_int32 a32AddressPrefix );

	IMPORT STATUS sysVmeA24SlaveAddressSet
	(
		u_int8  whichWindow,
		int     enable,
		u_int32 vmeAddr,
		u_int32 cpuAddr,
		u_int32 windowSize
	);
	IMPORT STATUS sysVmeA32SlaveAddressSet
	(
		u_int8  whichWindow,
		int     enable,
		u_int32 vmeAddr,
		u_int32 cpuAddr,
		u_int32 windowSize
	);

	IMPORT STATUS sysMenPci2VmeDma
    (
    	int		wrToVme,   /* 1 write to VME, 0 read from VME */
    	u_int32 *src,	   /* source address - 8 byte aligned */
    	u_int32 *dest,	   /* destination address - 8 byte aligned */
    	u_int32 size,	   /* 8 byte .. 256kB 4 byte aligned */
    	int     viaSram,   /* 0 for A15 - 1 for A500 */
    	u_int32 modifier,  /* SYS_VME_A24_D16 .. SYS_VME_A32_D32_SWAPPED */
    	int     timeOutTicks /* timeout in system ticks min 1 tick */
    );
	
	void sysMenPci2VmeSpaceName( int spcNumber, char* spcName );

#endif /*MEN_PLD_Z002*/

#ifdef __cplusplus
}
#endif

#endif	/* __INCsysMenPci2Vme */







