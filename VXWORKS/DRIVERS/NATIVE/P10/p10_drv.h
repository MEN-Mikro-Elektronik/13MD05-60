/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: p10_drv.h
 *      Project: P10 VxWorks driver
 *
 *       Author: rl
 *        $Date: 2006/05/12 10:17:47 $
 *    $Revision: 1.4 $
 *
 *  Description: P10 VxWorks driver definitions
 *
 *     Required:
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: p10_drv.h,v $
 * Revision 1.4  2006/05/12 10:17:47  cs
 * added define P10_IER again (dependant on CPU)
 *
 * Revision 1.3  2006/02/23 12:23:13  ts
 * + P10_IER
 *
 * Revision 1.2  2002/04/22 15:25:18  Franke
 * added
 * - console support
 * removed
 * - PCI2DRAM_BASE_ADRS
 *
 * Revision 1.1  2000/03/14 14:30:33  loesel
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef	__INCP10_DRVh
#define	__INCP10_DRVh

#ifdef __cplusplus
extern "C" {
#endif



#include "tyLib.h"


/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
typedef struct
{
	/* always goes first */
    SIO_DRV_FUNCS	*pDrvFuncs;	/* driver functions */

    /* callbacks */
    STATUS	(*getTxChar)();	/* pointer to a xmitr function */
    STATUS	(*putRcvChar)();/* pointer to a recvr function */
    void *	getTxArg;
    void *	putRcvArg;
} P10_SIO_STRUCTURE;

typedef struct
{
	TY_DEV			tyDev;		/* tty Device header structure */
} P10_TTY_DEV_STRUCT;

typedef union
{
	P10_SIO_STRUCTURE	sioT;
	P10_TTY_DEV_STRUCT	ttyT;
} HEADER_UNION;

typedef struct			/* P10_TY_CO_DEV */
{
	HEADER_UNION 	u;
    BOOL 		created;	/* true if this device has really been created */

#if (CPU_FAMILY == I80X86)	/* Registers I/O mapped, max of 64k of I/O */
    USHORT		ioAddr;		/* base address IO */
    USHORT		ioAddr2;	/* base address of OX16C950 configuration registers */
#else						/* Registers are Memory mapped */
    ULONG		ioAddr;		/* base address memory */
    ULONG		ioAddr2;
#endif /* CPU_FAMILY */

	char        irq;            /* Interrupt Request Level */
    UINT32      irqvec;         /* Interrupt Request vector */

	UINT8		channel;		/* channel number on UART */
	UINT8 		unit;			/* UART number in system */
	BOOL		auto_RS485;		/* automatic RS485 buffering enabled 	*/
	BOOL		auto_RTS;		/* automatic RTS handshaking is enabled */
	BOOL		auto_CTS;		/* automatic CTS handshaking is enabled */
	BOOL		auto_DTR;		/* automatic DTR handshaking is enabled */
	BOOL		auto_DSR;		/* automatic DSR handshaking is enabled */
	BOOL		auto_XONXOFF;	/* automatic Xon/Xoff handshaking is enabled */
	BOOL		fifo_enabled;	/* FIFO is enabled */

	UINT8		transmit_level;	/* transmit trigger level in 650 mode*/
	UINT8		receive_level;	/* receive trigger level in 650 mode */


	UINT8		shadowACR;		/* the current value of ACR (not readable) */
	UINT8		channelMode;	/* INT/POLLED*/
	UINT8		uartRev;		/* the uart revision */
	ULONG		options;	    /* UART hardware options */

} P10_TY_CO_DEV;

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define P10_NUM_CHANNEL		4       /* 4 channels per P10 */

/* equates for additional ioctl requests */
#define P10_TX_FIFO_LEVEL	0x115	/* set transmitter FIFO trigger level */
#define P10_RX_FIFO_LEVEL	0x116	/* set receiver FIFO trigger level */
#define P10_NO_FIFOS		0x117	/* disable FIFOs */


#define P10_MANUAL_RTS		0x101	/* manual RTS control */
#define P10_MANUAL_DTR		0x102	/* manual DTR control */
#define P10_MANUAL_CTS		0x103	/* manual CTS control */
#define P10_MANUAL_DSR		0x104	/* manual DSR control */
#define P10_MANUAL_DCD		0x105	/* manual DCD control */

#define P10_AUTO_RTS		0x110	/* automatic RTS control */
#define P10_AUTO_CTS		0x111	/* automatic CTS control */
#define P10_AUTO_DTR		0x112	/* automatic DTR control */
#define P10_AUTO_DSR		0x113	/* automatic DSR control */
#define P10_AUTO_XONXOFF	0x114	/* automatic in band control (do not use - VxWorks has its own)*/

/* euquates for additional P11 support */
#define P11_RS485_ENABLE	0x120	/* enables/disables RS485 mode on P11 */
#define P11_AUTO_RS485		0x121	/* automatic RS485 (DTR) control */

/* equates for addtional ioctl values */
#define P10_AUTO_OFF		0		/* automatic in band control off */
#define P10_AUTO_RC_ON		1		/* automatic in band control only receive */
#define P10_AUTO_BOTH		2		/* automatic in band control receive and transmit */

#define PCI_VENDOR_ID_OX		0x1415
#define PCI_DEVICE_ID_OX954	    0x9501

#if (CPU == PPC85XX) /* real memory mapped access */
#	define P10_IER                 0x04    /* interrupt enable register offset */
#else
#	define P10_IER                 1       /* interrupt enable register offset */
#endif
/* equates for interrupt enable register */
#define P10_IER_RXRDY		0x01		/* receiver data ready */
#define P10_IER_TBE			0x02		/* transmit bit enable */
#define P10_IER_LST			0x04		/* line status interrupts */
#define P10_IER_MSI			0x08		/* modem status interrupts */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
extern unsigned long p10Drv_CpuToPciMem; /* will be set by default to P10_CPU_PCI_IO_ADRS  */
extern unsigned long p10Drv_CpuToPciIo;	 /* will be set by default to P10_CPU_PCI_MEM_ADRS */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern void   p10DevInit( P10_TY_CO_DEV *pP10Dev ); /* for console initialization at startup */
extern STATUS p10Drv( char *devName, char *inString );
extern void   p10Int( P10_TY_CO_DEV	*pChan );
extern STATUS p10PciFindDevice
    ( int vendorId, int deviceId, int	index,
      int * pBusNo, int * pDeviceNo, int * pFuncNo );



#ifdef __cplusplus
}
#endif

#endif	/* __INCP10_DRVh */


