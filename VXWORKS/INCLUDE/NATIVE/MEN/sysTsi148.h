/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysTsi148.h
 *
 *       Author: cs
 *        $Date: 2010/07/01 15:13:06 $
 *    $Revision: 2.6 $
 *
 *  Description: VME interrupts
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysTsi148.h,v $
 * Revision 2.6  2010/07/01 15:13:06  UFranke
 * R: make man sysLib.i:25478: ERROR: Check file 'logLib.h':
 *    routine-like comment or else badly formatted routine header
 * M: cosmetics
 *
 * Revision 2.5  2010/06/30 10:27:02  UFranke
 * R: missing prototype
 * M: added sysTsi148ShowInitStatus()
 *
 * Revision 2.4  2008/11/17 12:01:41  cs
 * R: avoid warning because of  unnecessary "," in enum
 * M: avoid unnecessary ","
 *
 * Revision 2.3  2008/08/18 18:13:06  cs
 * R: some sysTsi148 specific prototypes where not explicitely defined extern
 * M: added IMPORT key word for prototypes where missing
 *
 * Revision 2.2  2008/08/01 13:08:17  cs
 * R: 1. most enums where not possible to be used as bit masks
 *    2. SYS_VME_INB_ATTR is now also used for GCSR/.. window setup
 *    3. mailboxing and location monitor support was only very basic
 * M: 1. changed enums to be xor exclusive so they can be used as bit masks
 *    2. instead of using single BOOL variables for every setting
 *       use bit masks for transfer and access cycle mode in SYS_VME_INB_ATTR
 *    3a. add sysVmeMailboxIrqSetup(), sysTsi148InboundGcsrWinSet()
 *     b. changed API of sysVmeLocMonSetup(), added SYS_VME_LMX typedef
 *
 * Revision 2.1  2008/07/29 22:23:10  cs
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008..2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	__SYSTSI148_H
	#define	__SYSTSI148_H

#ifdef __cplusplus
extern "C" {
#endif


/* Transfer Modes to be used for DMA and In-/Oubound Window setup */

/* Data Read Prefetch size */
typedef enum {
	SYS_VME_RDPFDIS		=	0x01,	/* disable PF for all memeory read commands */
	SYS_VME_RDPF2CL		=	0x02,	/* 2 Cache Lines */
	SYS_VME_RDPF4CL		=	0x04,	/* 4 Cache Lines */
	SYS_VME_RDPF8CL		=	0x08,	/* 8 Cache Lines */
	SYS_VME_RDPF16CL	=	0x10	/* 16 Cache Lines */
} SYS_VME_RDPFSIZE;

/* VMEbus Transfer Mode */
typedef enum {
	SYS_VME_TM_SCT		=	0x01,	/* SCT */
	SYS_VME_TM_BLT		=	0x02,	/* BLT */
	SYS_VME_TM_MBLT		=	0x04,	/* MBLT */
	SYS_VME_TM_2EVME	=	0x08,	/* 2EVME */
	SYS_VME_TM_2ESST160	=	0x10,	/* 160 MB/s */
	SYS_VME_TM_2ESST267	=	0x20,	/* 267 MB/s */
	SYS_VME_TM_2ESST320	=	0x40,	/* 267 MB/s */
	SYS_VME_TM_2ESSTB	=	0x80	/* 2ESST Broadcast */
} SYS_VME_TM;

/* VMEbus Access Cyle Type */
/* may be used as bit mask when more than one are to be enabed */
typedef enum {
	SYS_VME_AC_DATA		=	0x01,	/* Data access cycle */
	SYS_VME_AC_PGM		=	0x02,	/* Program access cycle */
	SYS_VME_AC_NPRIV	=	0x04,	/* Non-priviledged access cycle */
	SYS_VME_AC_SUPR		=	0x08	/* Supervisor access cycle  */
} SYS_VME_AC;

/* VMEbus Address Mode */
/* for inbound windows _A16 - _A64 may be used as bitmask */
typedef enum {
	SYS_VME_A16			=	0x01,
	SYS_VME_A24			=	0x02,
	SYS_VME_A32			=	0x04,
	SYS_VME_A64			=	0x08,
	SYS_VME_CRCSR		=	0x10,
	SYS_VME_UM1			=	0x21,
	SYS_VME_UM2			=	0x22,
	SYS_VME_UM3			=	0x24,
	SYS_VME_UM4			=	0x28
} SYS_VME_ADDRMODE;

/* VMEbus max Data Width
 * applies to SCT and BLT transfers only, MBLT, 2eVME and 2eSST transfers are always 64-bit */
typedef enum {
	SYS_VME_D16			=	0x01,
	SYS_VME_D32			=	0x02
} SYS_VME_DATAMODE;

/* Outbound Window Attribute settings */
typedef struct _SYS_VME_OUTB_ATTR {
	SYS_VME_TM					tm;			/* transfer mode */
	SYS_VME_ADDRMODE			am;			/* address mode */
	SYS_VME_DATAMODE			dm;			/* data bus width */
	SYS_VME_RDPFSIZE			rdpf;		/* data read prefetch size */
	BOOL						sup;		/* set for supervisory mode access, else non-priviledged */
	BOOL						pgm;		/* set for programm mode access, else data mode */
} SYS_VME_OUTB_ATTR;

/* Inbound Window Attribute settings */
typedef struct _SYS_VME_INB_ATTR {
	u_int32						tm;			/* bitmask of transfer modes (SYS_VME_TM_*) to respomd to */
	SYS_VME_ADDRMODE			am;			/* address mode */
	u_int32						vfs;		/* virtual FIFO size (64/128/256/512) */
	BOOL						threshold;	/* threshold enable */
	u_int32						ac;			/* bitmask of access cycle modes (SYS_VME_AC_*) to respond to */
} SYS_VME_INB_ATTR;

/* Location monitor enables */
/* is used as bit mask when more than one are to be enabed */
typedef enum {
	SYS_VME_LM_0	=	0x01,	/* LM0, triggers at LMBA + offsets 0x00 - 0x07 */
	SYS_VME_LM_1	=	0x02,	/* LM1, triggers at LMBA + offsets 0x08 - 0x0f */
	SYS_VME_LM_2	=	0x04,	/* LM2, triggers at LMBA + offsets 0x10 - 0x17 */
	SYS_VME_LM_3	=	0x08	/* LM3, triggers at LMBA + offsets 0x18 - 0x1f */
} SYS_VME_LMX;


/*=================================================
*          
*    D M A 
*
===================================================*/
/* DMA type,
 *  *_PATTERN_* only applies to src descriptors */
typedef enum {
	SYS_VME_DMA_PCIX			=	0,	/* PCI/X bus */
	SYS_VME_DMA_VME 			=	1,	/* VME bus */
	SYS_VME_DMA_PATTERN_SZ32_I	=	2,	/* PATTERN, pattern size = 32bit, incremented */
	SYS_VME_DMA_PATTERN_SZ32_NI	=	3,	/* PATTERN, pattern size = 32bit, not incremented */
	SYS_VME_DMA_PATTERN_SZ8_I	=	4,	/* PATTERN, pattern size = 8bit, incremented */
	SYS_VME_DMA_PATTERN_SZ8_NI	=	5	/* PATTERN, pattern size = 8bit, not incremented */
} SYS_VME_DMATYPE;

/* DMA status flag */
typedef enum {
	SYS_VME_DMA_STATE_ERROR 	=	-1,	/* finished command with error */
	SYS_VME_DMA_STATE_DONE		=	0,	/* successfully finished command */
	SYS_VME_DMA_STATE_BUSY		=	1,	/* still busy finishing transfer */
	SYS_VME_DMA_STATE_ABT		=	2,	/* successfully completed commanded abort */
	SYS_VME_DMA_STATE_PAU		=	3	/* successfully completed commanded pause */
} SYS_VME_DMA_STATE;

/* DMA Setup/Control structures */
/* DMA transfer control */
typedef struct _SYS_VME_DMA_CTRL {
	BOOL 		vfar;		/* VME   Flush on Aborted Read	*/
	BOOL 		pfar;		/* PCI/X Flush on Aborted Read	*/
	u_int16		vBlkSize;	/* VMEbus Block Size 			*/
	u_int16		pBlkSize;	/* PCI/X Block Size 			*/
	u_int8		vbot;		/* VMEbus Back-off Timer 		*/
	u_int8		pbot;		/* PCI/X Back-off Timer 		*/
} SYS_VME_DMA_CTRL;

/* function pointer for user functions to be called on DMA transfer finish */
/* This function will be called on interrupt level.
 * So all rules for interrupt service routines apply to functions used here:
 * not calling waits, prints, ... */
typedef void ( * SYS_VME_DMA_INTHANDLER ) ( u_int32 chan );

/* DMA Source and Destination Attribute settings */
typedef struct _SYS_VME_DMA_ATTR {
	SYS_VME_DMATYPE 			dmaType;		/* PCI/VME/PATTERN */
	SYS_VME_TM					dmaTm;			/* transfer mode */
	SYS_VME_DATAMODE			dmaDm;			/* data bus width */
	SYS_VME_ADDRMODE			dmaAm;			/* address mode */
	u_int32						dmaAc;			/* access cycle (bitmask of SYS_VME_AC_xxx) */
} SYS_VME_DMA_ATTR;

/* DMA Status */
typedef struct _SYS_VME_DMA_STATUS {
	SYS_VME_DMA_STATE status;	/* current status of DMA controller */
	u_int32		dctl;		/* DMA Control  */
	u_int32		dsta;		/* DMA Status */
	u_int32		dcsau;		/* DMA Current Source Address Upper */
	u_int32		dcsal;		/* DMA Current Source Address Lower */
	u_int32		dcdau;		/* DMA Current Destination Address Upper */
	u_int32		dcdal;		/* DMA Current Destination Address Lower */
	u_int32		dclau;		/* DMA Current Link Address Upper */
	u_int32		dclal;		/* DMA Current Link Address Lower */
	TSI148_DMA_LL_DESC	dmaLlDesc; /* image of DMA registers */
} SYS_VME_DMA_STATUS;

/* DMA Descriptor */
typedef struct _SYS_VME_DMA_DESC {
	u_int64						srcAddr;	/* DMA source address */
	u_int64						dstAddr;	/* DMA destination address */
	u_int32						dmaSize;	/* DMA transfer size (bytes) */
	u_int32						dmaDbs2esst;/* DMA Destination Broadcast Select (2ESST) */
	SYS_VME_DMA_ATTR            srcAttr;	/* DMA source attributes */
	SYS_VME_DMA_ATTR            dstAttr;	/* DMA destination attributes */
	struct _SYS_VME_DMA_DESC	*next;		/* pointer to next descriptor for linked transfers or NULL */

	/* Descriptor for DMA transfers
	 * This will be computed by the driver before starting the transfer.
	 * For linked lists the DMA controller requires this descriptor to be 8byte aligned.
	 * Therefore size descriptor to allow 4 byte alignments (done by driver) */
	u_int32						dmaLlDesc[(sizeof(TSI148_DMA_LL_DESC) + 4)/4 ];
} SYS_VME_DMA_DESC;

/* prototypes */
IMPORT char* 	sysTsi148Ident( void );
IMPORT int 		sysTsi148Init( void );
IMPORT int 		sysTsi148Init2( void );
IMPORT void		sysTsi148ShowInitStatus( void );

IMPORT STATUS sysVmeLocalToBusAdrs
(
	int 	adrsSpace,	/* bus address space where busAdrs resides */
	char *	localAdrs,	/* local address to convert */
	char **	pBusAdrs	/* where to return bus address */
);
IMPORT STATUS sysVmeBusToLocalAdrs
(
	int  	adrsSpace, 	/* bus address space where busAdrs resides */
	char *	busAdrs,   	/* bus address to convert */
	char **	pLocalAdrs 	/* where to return local address */
);
IMPORT STATUS sysVmeIntEnable
(
	int intLevel        /* VME interrupt level to enable (1-7) */
);
IMPORT STATUS sysVmeIntDisable
(
	int intLevel        /* interrupt level to disable (1-7) */
);
IMPORT STATUS sysVmeBusIntGen
(
	int  level,        /* VMEbus interrupt level to generate (1-7) */
	int  vector        /* interrupt vector to generate (0-255)     */
);
IMPORT int sysVmeBusIntAck
(
	int intLevel       /* interrupt level to acknowledge */
);





IMPORT STATUS sysTsi148OutboundWinSet
(
	int winNo,					/* window 0 - 7 */
	u_int64 srcAdrs,			/* source (PCI) address */
	u_int64 dstAdrs,			/* destination (VME) address */
	u_int64 size,				/* size of Window */
	SYS_VME_OUTB_ATTR *outbAttr,/* outbound window attributes */
	u_int32 bs,					/* 2eSST broadcast select */
	BOOL enable					/* enable window */
);
IMPORT STATUS sysTsi148InboundWinSet
(
	int winNo,					/* window 0 - 7 */
	u_int64 srcAdrs,			/* source (VME) address */
	u_int64 dstAdrs,			/* destination (PCI) address */
	u_int64 size,				/* size of Window */
	SYS_VME_INB_ATTR *inbAttr,  /* inbound window attributes */
	BOOL enable					/* enable window */
);
IMPORT STATUS sysTsi148InboundGcsrWinSet
(
	u_int64 srcAdrs,			/* source (VME) address */
	SYS_VME_INB_ATTR *inbAttr,  /* inbound window attributes */
	BOOL enable					/* enable window */
);
IMPORT STATUS sysTsi148OutboundWinGetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_OUTBOUND *win	/* window image structure */
);
IMPORT STATUS sysTsi148InboundWinGetImg
(
	int winNo,				/* window 0 - 7 */
	TSI148_INBOUND *win		/* window image structure */
);

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
/* IMPORT STATUS	sysVmeReset(void); */

IMPORT STATUS sysVmeLocMonSetup
(
	SYS_VME_ADDRMODE  	am,		/* SYS_VME_Axx where xx=16/24/32/64 */
	u_int32				ac,		/* bitmask of SYS_VME_AC_xx where xx=DATA/PGM/NPRIV/SUPR  */
	u_int64 			adrs,	/* compare addr */
	u_int32				enable	/* bitmask of irqs to enable SYS_VME_LM_0-3 */
);
IMPORT STATUS sysVmeMailboxIrqSetup
(
	int mbox,   /* mailbox 0..3 */
	BOOL enable
);

IMPORT void sysVmeShow
(
	int verbose 			/* when != 0 detailed information is dumped (registers) */
);

IMPORT STATUS sysTsi148Dma
(
	int chan,						/* select DMA channel (0/1) */
	SYS_VME_DMA_CTRL    *dmaCtrl,	/* control structure for DMA */
	SYS_VME_DMA_DESC *dmaDesc,	/* descriptor for DMA (src/dest/..) */
	SYS_VME_DMA_INTHANDLER userHdl,	/* userHandler */
	int toutTicks					/* timeout when channel is busy */
);

IMPORT STATUS sysTsi148DmaStatus
(
	int ctrlNum,					 /* select DMA channel (0/1) */
	SYS_VME_DMA_STATUS *dmaStatus /* structure to be filled with current status of controller */
);

#ifdef __cplusplus
}
#endif

#endif	/* __SYSTSI148_H */








