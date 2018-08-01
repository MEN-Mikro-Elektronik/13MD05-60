/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: p10_drv.c
 *      Project: P10/P11 VxWorks Driver
 *
 *       Author: rl
 *        $Date: 2006/07/07 19:40:43 $
 *    $Revision: 1.10 $
 *
 *  Description: VxWorks P10/P11 serial driver
 *
 *				 The function p10PciFindDevice() is optimized for
 *               startup time. It don't looks for subfunctions and looks only
 *               for bus numbers less than 0x40.
 *
 *     Required: -
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: p10_drv.c,v $
 * Revision 1.10  2006/07/07 19:40:43  cs
 * bugfix: using P10_AUTO_XONXOFF disabled FIFOs due to wrong register access
 *         now fixed in setInBandFlowControlMode()
 *
 * Revision 1.9  2006/07/05 12:16:00  cs
 * bugfix: corrected XON/XOFF characters from 0x17/0x19 to 17/19
 *
 * Revision 1.8  2006/05/12 10:19:53  cs
 * fixed changes made for PPC85XX so driver works for common PPC603 as well
 *
 * Revision 1.7  2006/02/23 12:22:43  ts
 * support for EM3 family (85XX)
 *
 * Revision 1.6  2005/07/28 19:39:01  CSchuster
 * added support for onboard OX16PCI954 chips (e.g. 01A500)
 * added support for PPC603 (different memory offsets)
 *
 * Revision 1.5  2003/04/10 09:11:12  UFranke
 * cosmetics for Tornado 2.2
 *
 * Revision 1.4  2003/02/21 13:05:43  UFranke
 * cosmetics
 *   -removed warings of the Tornado 2.2 GNU compiler
 *
 * Revision 1.3  2002/04/29 16:56:08  Franke
 * added SIO_BAUD_SET
 *
 * Revision 1.2  2002/04/24 15:36:13  Franke
 * added
 * - console support at boot initialization
 * - runtime support for MPC106 MAP-A/MAP-B by p10Drv_CpuToPciMem/Io
 * - p10DrvId()
 * replaced
 * - sysIntEnable by intEnable
 * changed
 * - P10_MAX_UNITS from 8 to 2
 * removed
 * - PCI2DRAM_BASE_ADRS
 *
 * Revision 1.1  2000/03/14 14:30:32  loesel
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static char *RCSid="$Id: p10_drv.c,v 1.10 2006/07/07 19:40:43 cs Exp $ build " __DATE__" "__TIME__ ;


#include "vxWorks.h"	/* first, CPU type, family , big/litte endian, etc. */
#include "iv.h"			/* for INUM_TO_IVEC, then config.h is not needed */
#include "vmLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "logLib.h"
#include "stdlib.h"
#include "string.h"
#include "intLib.h"
#include "tickLib.h"
#include "taskLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tyLib.h"

#include "drv/pci/pciConfigLib.h"	/* for PCI const */
#include "drv/pci/pciIntLib.h"		/* for PCI interrupt */

/* should be defined in sysLib.h, but it's under WRS control */
extern STATUS sysIntConnect( VOIDFUNCPTR* vector,
							 VOIDFUNCPTR routine,
							 int parameter );

#undef DBG
#include "p10_drv.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef struct			/* BAUD */
{
    int			rate;					/* baud rate */
    int			preset;					/* counter preset value */
} BAUD;

typedef struct			/* P10_PATH */
{
	UINT32		unitNumber;	/* unit number */
	char		devName[20];/* device name */
	BOOL		installed;	/* flag if already installed */
	int			path[16];	/* the pci path of the unit */
	UINT32		pathLen;	/* the path length (number of entries in path[]) */
} P10_PATH;

typedef struct p10PciResource      		/* P10_PCI_RESOURCES */
{
	UINT32		vendorId;
	UINT32		deviceId;
    UINT32      iobaseCsr;      /* BAR  0 (P10 IO Base)				*/
    UINT32      membaseCsr;     /* BAR  1 (P10 Memory Base)			*/
    UINT32		iobaseCsr2;		/* BAR  2 (P10 Config IO Base)		*/
    UINT32		membaseCsr2;	/* BAR  3 (P10 Config Memory Base)	*/
    char        irq;            /* Interrupt Request Level 			*/
    UINT32      irqvec;         /* Interrupt Request vector 		*/
    UINT32      configType;     /* type of configuration 			*/
    UINT32      boardType;      /* type of board for this unit 		*/
    UINT32      pciBus;         /* PCI Bus number 					*/
    UINT32      pciDevice;      /* PCI Device number 				*/
    UINT32      pciFunc;        /* PCI Function number 				*/
	UINT32		unitNumber;		/* unit number 						*/
} P10_PCI_RESOURCES;


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

#ifdef DBG
# define DBGCMD(_x_)	_x_
  int P10_Logs = 1;
#else
# define DBGCMD(_x_)
#endif

#define DBG_P10( x )    DBGCMD( printErr x )

#define P10_CREATE_TYPE_AT_BOOT 1
#define P10_CREATE_TYPE_LATE    2

#define P10_DEFAULT_BAUD		9600
#define P10_MAX_UNITS			2

/*
 * default values if PCI_CFG_TYPE defined to be PCI_CFG_FORCE
 * note: memory addresses must be aligned on MMU page boundaries
 */
#define PCI_CFG_MODE            0x43
#define SLEEP_MODE_DIS          0x00    /* sleep mode disable */

#ifndef INCLUDE_PCI2MMU
	#define PCI_DEV_MMU_MSK         (~(VM_PAGE_SIZE - 1))   /* Mask MMU page */
 	#define PCI_DEV_ADRS_SIZE       VM_PAGE_SIZE    /* one page */
#endif

/* processor dependent interrupt vector base
 * and Default offsets of PCI MEM / IO
 *
 */
#if ( CPU_FAMILY == I80X86 )
#	define UARTS_OFFS			 0x8
#	define P10_INT_NUM_IRQ0		 0x20  /* irq vector base(intel=>0x20) */
#	define P10_CPU_PCI_IO_ADRS		0
#	define P10_CPU_PCI_MEM_ADRS	0
#endif

#if ( CPU_FAMILY == PPC )
#	if (CPU == PPC603)
#		define UARTS_OFFS			 0x8
#		define P10_INT_NUM_IRQ0	 0 /* interrupt vector base (PPC/MC68k => 0) */
#		define P10_CPU_PCI_IO_ADRS     0xFE000000 /* PPC603 MAP-B */
#		define P10_CPU_PCI_MEM_ADRS    0x80000000 /* PPC603 MAP-B */
#	elif (CPU == PPC85XX)
#		define UARTS_OFFS			 0x20
#		define P10_INT_NUM_IRQ0	 0 /* interrupt vector base (PPC/MC68k => 0) */
#		define P10_CPU_PCI_IO_ADRS     0x0 /* PPC603 MAP-B */
#		define P10_CPU_PCI_MEM_ADRS    0x0 /* PPC603 MAP-B */
#	else
#		define UARTS_OFFS			 0x8
#		define P10_INT_NUM_IRQ0	 0 /* interrupt vector base (PPC/MC68k => 0) */
#		define P10_CPU_PCI_IO_ADRS     0x80000000
					/* MPC106 MAP-A overwrite p10Drv_CpuToPciMem for MAP-B*/
#		define P10_CPU_PCI_MEM_ADRS    0xC0000000
					/* MPC106 MAP-A overwrite p10Drv_CpuToPciIo  for MAP-B*/
#   endif
#endif

#ifndef P10_INT_NUM_IRQ0
    #error "P10_INT_NUM_IRQ0 must be defined"
#endif
#ifndef P10_CPU_PCI_IO_ADRS
	#error "P10_CPU_PCI_IO_ADRS must be defined"
#endif
#ifndef P10_CPU_PCI_MEM_ADRS
	#error "P10_CPU_PCI_MEM_ADRS must be defined"
#endif

#define EXT_INTERRUPT_BASE      P10_INT_NUM_IRQ0    /* irq0     */


/* IO access macros */
#define RD(addr)		sysInByte(addr)
#define WR(addr, data)	sysOutByte(addr, data)


/*
 * The macro UART_REG_ADDR_INTERVAL must be defined
 * when including this header.
 */

/* default definitions */
#if (CPU == PPC85XX) /* real memory mapped access */
#	define THR_OFFSET		0		/* transmitter holding register */
#	define RHR_OFFSET		0		/* receiver holding register */
#	define IER_OFFSET		P10_IER /* interrupt enable register offset */
#	define FCR_OFFSET		8		/* FIFO control register */
#	define ISR_OFFSET		8		/* interrupt status register */
#	define LCR_OFFSET		0xc		/* line control register */
#	define MCR_OFFSET		0x10	/* modem control register */
#	define LSR_OFFSET		0x14	/* line status register */
#	define MSR_OFFSET		0x18	/* modem status register */
#	define SPR_OFFSET		0x1c	/* sratch pad  register */
#else
#	define THR_OFFSET		0		/* transmitter holding register */
#	define RHR_OFFSET		0		/* receiver holding register */
#	define IER_OFFSET		P10_IER /* interrupt enable register offset */
#	define FCR_OFFSET		2		/* FIFO control register */
#	define ISR_OFFSET		2		/* interrupt status register */
#	define LCR_OFFSET		3		/* line control register */
#	define MCR_OFFSET		4		/* modem control register */
#	define LSR_OFFSET		5		/* line status register */
#	define MSR_OFFSET		6		/* modem status register */
#	define SPR_OFFSET		7		/* sratch pad  register */
#endif

#define DLL_OFFSET			0		/* divisor low byte */
#define DLM_OFFSET			1		/* divisor high byte */
#define LCR_DL_ACCESS_KEY	0x80
#define LCR_650_ACCESS_KEY	0xBF


    /* 650 compatible registers */
#define EFR_OFFSET			2		/* enhanced features register */
#define XON1_OFFSET			4		/* XON1 flow control character */
#define XON2_OFFSET			5		/* XON2 flow control character */
#define XOFF1_OFFSET		6		/* XOFF1 flow control character */
#define XOFF2_OFFSET		7		/* XOFF2 flow control character */

    /* 950 specific registers */
#define ASR_OFFSET			1		/* additional status register */
#define RFL_OFFSET			3		/* receiver FIFO fill level */
#define TFL_OFFSET			4		/* transmitter FIFO fill level */
#define ICR_OFFSET			5		/* indexed control register */
#define ACR_INDEX			0		/* index of ARC register */
#define ACR_ICR_READ_EN		0x40	/* enable read of ICR register */
#define ACR_950_READ_EN		0x80	/* enable read of 950 registers */


/* equates for interrupt identification register */
#define P10_IIR_IP			0x01		/* interrupt pending bit */
#define P10_IIR_MASK		0x0F		/* interrupt id bits mask */
#define P10_IIR_MSTAT		0x00		/* modem status interrupt */
#define P10_IIR_THRE		0x02		/* transmit holding register empty */
#define P10_IIR_RBRF		0x04		/* receiver buffer register full */
#define P10_IIR_TIMEOUT		0x0C		/* receiver fifo timeout */
#define P10_IIR_SEOB		0x06		/* serialization error or break */

/* equates for line control register */

#define P10_LCR_CS5			0x00	/* 5 bits data size */
#define P10_LCR_CS6			0x01	/* 6 bits data size */
#define P10_LCR_CS7			0x02	/* 7 bits data size */
#define P10_LCR_CS8			0x03	/* 8 bits data size */
#define P10_LCR_2_STB		0x04	/* 2 stop bits */
#define P10_LCR_1_STB		0x00	/* 1 stop bit */
#define P10_LCR_PEN			0x08	/* parity enable */
#define P10_LCR_PDIS		0x00	/* parity disable */
#define P10_LCR_EPS			0x10	/* even parity slect */
#define P10_LCR_SP			0x20	/* stick parity select */
#define P10_LCR_SBRK		0x40	/* break control bit */
#define P10_LCR_DLAB		0x80	/* divisor latch access enable */

/* equates for the modem control register */

#define P10_MCR_DTR			0x01	/* dtr output */
#define P10_MCR_RTS			0x02	/* rts output */
#define P10_MCR_OUT1		0x04	/* output #1 */
#define P10_MCR_OUT2		0x08	/* output #2, must set for interrupt use */
#define P10_MCR_LOOP		0x10	/* loop back */

/* equates for line status register */

#define P10_LSR_RXRDY		0x01		/* receiver data available */
#define P10_LSR_OE			0x02		/* overrun error */
#define P10_LSR_PE			0x04		/* parity error */
#define P10_LSR_FE			0x08		/* framing error */
#define P10_LSR_BI			0x10		/* break interrupt */
#define P10_LSR_THRE		0x20		/* transmit holding register empty */
#define P10_LSR_TEMT		0x40		/* transmitter empty */

/* equates for modem status register */

#define P10_MSR_DCTS		0x01		/* cts change */
#define P10_MSR_DDSR		0x02		/* dsr change */
#define P10_MSR_DRI			0x04		/* ring change */
#define P10_MSR_DDCD		0x08		/* data carrier change */
#define P10_MSR_CTS			0x10		/* complement of cts */
#define P10_MSR_DSR			0x20		/* complement of dsr */
#define P10_MSR_RI			0x40		/* complement of ring signal */
#define P10_MSR_DCD			0x80		/* complement of dcd */

/* automatic flow control register values */
#define EFR_AUTO_RTS_EN 	0x40		/* automatic RTS control enable bit */
#define EFR_AUTO_CTS_EN 	0x80		/* automatic CTS control enable bit */
#define ACR_AUTO_DSR_EN 	0x04		/* automatic DSR control enable bit */
#define ACR_AUTO_DTR_EN 	0x08		/* automatic DTR control enable bit */
#define ACR_950_TRIGGER_EN 	0x20		/* 950 trigger enable bit */

/* automatic Xon Xoff flow control values */
#define XON_CHAR			  17		/* Xon character = ^Q */
#define XOFF_CHAR			  19		/* Xoff character = ^S */

/* transmit flow disabled, receive mode */
#define EFR_AUTO_TROFF_RC_DISABLED	0x0		/* receive disabled */
#define EFR_AUTO_TROFF_X1 			0x2		/* receive Xon1/Xoff1 */
#define EFR_AUTO_TROFF_X2			0x1		/* receive Xon2/Xoff2 */

/* transmit Xon1 Xoff1, receive mode */
#define EFR_AUTO_TR1_RC_DISABLED 	0x8		/* receive disabled */
#define EFR_AUTO_TR1_X1				0xA		/* receive Xon1/Xoff1 */
#define EFR_AUTO_TR1_X2				0x9		/* receive Xon2/Xoff2 */
#define EFR_AUTO_TR1_ALL			0xB		/* receive all */

/* transmit Xon2 Xoff2, receive mode */
#define EFR_AUTO_TR2_RC_DISABLED	0x4		/* as above */
#define EFR_AUTO_TR2_X1				0x6		/* as above */
#define EFR_AUTO_TR2_X2				0x5		/* as above */
#define EFR_AUTO_TR2_ALL			0x7		/* as above */

/* RS485 enable bits */
#define ACR_RS485_HIGH_EN	0x18	/* automatic RS485 enable active high */
#define ACR_RS485_LOW_EN	0x10	/* automatic RS486 enable active low */


/* FIFO Control Register */

#define FCR_EN				0x01		/* enable xmit and rcvr */
#define FIFO_ENABLE			FCR_EN
#define FCR_RXCLR			0x02		/* clears rcvr fifo */
#define RxCLEAR				FCR_RXCLR
#define FCR_TXCLR			0x04		/* clears xmit fifo */
#define TxCLEAR				FCR_TXCLR
#define FCR_DMA				0x08		/* dma */
#define FCR_RXTRIG_L		0x40		/* rcvr fifo trigger lvl low */
#define FCR_RXTRIG_H		0x80		/* rcvr fifo trigger lvl high */

#define FCR_DMA_MODE 		0x08 		/* Set to use 550 Tx Trigger Levels */

#define FCR_RX_TRIGGER_16	0x00	/* receiver FIFO trigger level = 16 Byte */
#define FCR_RX_TRIGGER_32	0x40	/* receiver FIFO trigger level = 32 Byte */
#define FCR_RX_TRIGGER_112	0x80	/* receiver FIFO trig level = 112 Byte */
#define FCR_RX_TRIGGER_120	0xC0	/* receiver FIFO trig level = 120 Byte */
#define FCR_TX_TRIGGER_16	0x00	/* transmitter FIFO trig level = 16 Byte */
#define FCR_TX_TRIGGER_32	0x10	/* transmitter FIFO trig level = 32 Byte */
#define FCR_TX_TRIGGER_64	0x20	/* transmitter FIFO trig level = 64 Byte */
#define FCR_TX_TRIGGER_112	0x30	/* transmitter FIFO trig level = 112 Byte */


#define P10_TX_16				FCR_TX_TRIGGER_16
#define P10_TX_32				FCR_TX_TRIGGER_32
#define P10_TX_64				FCR_TX_TRIGGER_64
#define P10_TX_112				FCR_TX_TRIGGER_112

#define P10_RX_16				FCR_RX_TRIGGER_16
#define P10_RX_32				FCR_RX_TRIGGER_32
#define P10_RX_112				FCR_RX_TRIGGER_112
#define P10_RX_120				FCR_RX_TRIGGER_120

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
int	p10TyCoDrvNum = NONE;

/* the MEM/IO offsets will be initialized to values
 * to compatible with former versions
 * i.e. MEN D002 PENTIUM and F001 PPC MPC107 Address MAP-A
 */
unsigned long  p10Drv_CpuToPciMem	= P10_CPU_PCI_MEM_ADRS;
unsigned long  p10Drv_CpuToPciIo	= P10_CPU_PCI_IO_ADRS;


/* test variables for fifo */
int outBytes = 0;
int inBytes = 0;
int intCountIn = 0;
int intCountOut = 0;

/* number of P10 found in system during PCI check */
int	p10Units = 0;

/* number of P10 drivers already installed */
int p10DrvNum = 0;

/* pci resource list for P10 */
P10_PCI_RESOURCES       *p10Rsrc [P10_MAX_UNITS];

P10_TY_CO_DEV			p10TyCoDev[P10_MAX_UNITS][4];


/* baud rate table for P10 */
LOCAL BAUD p10BaudTable [] =
    {
    {300, 3840}, {600, 1920}, {1200, 960}, {2400, 480},
    {4800, 240}, {9600, 120}, {14400, 80}, {19200, 60},
    {28800, 40}, {33400, 34}, {38400, 30}, {57600, 20},
    {115200, 10}, {230400, 5}, {576000, 2}, {1152000, 1}
    };


/* instance of eight P10 PCI-resources */
LOCAL P10_PCI_RESOURCES p10PciResrcs [] =
{
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0}
};

/* instance of eight PCI pathes
   unitNumber, installed, path, pathLen */
P10_PATH p10Path [] =
{
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0},
	{0, {0}, 0, {0}, 0}
};



/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
LOCAL	STATUS	p10SetFifoLevel(P10_TY_CO_DEV *pChan, UINT8 select, UINT8 value);
static	int		p10Ioctl(P10_TY_CO_DEV *pChan, int request, int	arg);
LOCAL	int 	p10Startup(P10_TY_CO_DEV *pChan);
static  int     callbackInstall
    (
    SIO_CHAN *  pSioChan,
    int         callbackType,
    STATUS      (*callback)(void *, ...),
    void *      callbackArg
    );
LOCAL	int		p10Open(P10_TY_CO_DEV *p10TyDev, char *name, int mode);
LOCAL 	STATUS	p10OptsSet(P10_TY_CO_DEV *pChan, UINT options);
LOCAL	STATUS	p10BaudSet(P10_TY_CO_DEV *pChan, UINT baud);
LOCAL	STATUS 	p10InitChannel(P10_TY_CO_DEV *pChan, int createType );
LOCAL int p10PollRxChar
    (
    P10_TY_CO_DEV   *pChan,      /* pointer to channel */
    char            *pChar       /* pointer to char */
    );
LOCAL int p10PollTxChar
    (
    P10_TY_CO_DEV   *pChan,        /* pointer to channel */
    char            outChar       /* char to send */
    );


static SIO_DRV_FUNCS sioDrvFuncs =
{
	(void*)p10Ioctl,		/* ioctl			*/
	(void*)p10Startup,		/* txStartup		*/
	(void*)callbackInstall,/* callbackInstall	*/
	(void*)p10PollRxChar,	/* pollInput		*/
	(void*)p10PollTxChar	/* pollOutput		*/
};

/*---------------------------------------------------------------------------+
|                      special register access functions                     |
+---------------------------------------------------------------------------*/

/******************************** readDivisor ********************************
 *
 *  Description: Read the baud rate divisor
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *  Output.....:  Return        the divisor
 *
 *  Globals....:
 *****************************************************************************/
LOCAL UINT16 readDivisor(P10_TY_CO_DEV *pChan)
{
	UINT16	dlldlm;
	UINT8	oldLCR;
	/* store the current value of LCR
	and then set the top bit to allow
	divisor latch access */
	oldLCR = RD(pChan->ioAddr + LCR_OFFSET);
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR | LCR_DL_ACCESS_KEY);
	/* construct divisor latch, then restore old values*/
	dlldlm = (RD(pChan->ioAddr + DLM_OFFSET)<<8);
	dlldlm += RD(pChan->ioAddr + DLL_OFFSET);
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR);
	return dlldlm;
}

/******************************** writeDivisor *******************************
 *
 *  Description: Write the baud rate divisor
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                divisor       the divisor to write
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void writeDivisor(P10_TY_CO_DEV *pChan, UINT16 divisor)
{
	UINT8	oldLCR;
	/* store old LCR and the access divisor */
	oldLCR = RD(pChan->ioAddr + LCR_OFFSET);
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR | LCR_DL_ACCESS_KEY);
	/* write divisor then restore */
	WR(pChan->ioAddr + DLL_OFFSET, divisor & 0x00FF);
	WR(pChan->ioAddr + DLM_OFFSET, (divisor & 0xff00)>>8);
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR);
}

/********************************* read650 ***********************************
 *
 *  Description: Read the 650 specific registers
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                offset        register offset
 *
 *  Output.....:  Return        value of the register
 *
 *  Globals....:
 *****************************************************************************/
LOCAL UINT8 read650(P10_TY_CO_DEV *pChan, UINT8 offset)
{
	UINT8	result, oldLCR;
	/* store old LCR and write access code */
	oldLCR = RD(pChan->ioAddr + LCR_OFFSET);
	WR(pChan->ioAddr + LCR_OFFSET, LCR_650_ACCESS_KEY);
	/* read the register */
	result = RD(pChan->ioAddr + offset);
	/* restore LCR */
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR);
	return result;
}

/******************************** write650 ***********************************
 *
 *  Description: Write the 650 specific registers
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                offset        register offset
 *                value         value to write
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void write650(P10_TY_CO_DEV *pChan, UINT8 offset, UINT8 value)
{
	UINT8	oldLCR;
	/* store the current LCR, enable access */
	oldLCR = RD(pChan->ioAddr + LCR_OFFSET);
	WR(pChan->ioAddr + LCR_OFFSET, LCR_650_ACCESS_KEY);
	/* write register */
	WR(pChan->ioAddr + offset, value);
	/* restore LCR */
	WR(pChan->ioAddr + LCR_OFFSET, oldLCR);
}

/******************************** writeICR ***********************************
 *
 *  Description: Write to the ICR with the specified index and value
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                index         index of the register
 *                value         value to write
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void writeICR(P10_TY_CO_DEV *pChan, UINT8 index, UINT8 value)
{
	/* writes the ICR set register
	   index by the index parameter */
	WR(pChan->ioAddr + SPR_OFFSET, index);
	WR(pChan->ioAddr + ICR_OFFSET, value);
	/* record changes made to ACR */
	if (index==ACR_INDEX) pChan->shadowACR = value;
}

/******************************* readICR *************************************
 *
 *  Description: Read the ICR register value of the specified index
 *               (not used by this driver)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                index         the index of the register
 *
 *  Output.....:  Return        the value of the register
 *
 *  Globals....:
 *****************************************************************************/
/*LOCAL UINT8 readICR(P10_TY_CO_DEV *pChan, UINT8 index)
{
	UINT8	retVal;

	writeICR(pChan, ACR_INDEX, (UINT8)(pChan->shadowACR | ACR_ICR_READ_EN));
	WR(pChan->ioAddr + SPR_OFFSET, index);
	retVal = RD(pChan->ioAddr + ICR_OFFSET);
	writeICR(pChan, ACR_INDEX, (UINT8)pChan->shadowACR & ~ACR_ICR_READ_EN);
	return retVal;
}*/

/************************** UnlockAdditionalStatus ***************************
 *
 *  Description: Unlock the access to the 950 specific registers
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
/*LOCAL void UnlockAdditionalStatus(P10_TY_CO_DEV *pChan)
{*/
	/* Set the top bit of ACR to enable
	   950 specific register set access */
/*	pChan->shadowACR |= ACR_950_READ_EN;
	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
}*/

/**************************** LockAdditionalStatus ***************************
 *
 *  Description: Lock the access to the 950 specific registers
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
/*LOCAL void LockAdditionalStatus(P10_TY_CO_DEV *pChan)
{*/
	/* Clear the top bit of ACR to disable
	   950 specific register set access */
	/*pChan->shadowACR &= (~ACR_950_READ_EN);
	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
}*/


/*LOCAL UINT8 readASR(P10_TY_CO_DEV *pChan)
{*/
	/* Returns the data stored in the ASR register */
/*	UINT8	retVal;
	UnlockAdditionalStatus(pChan);
	retVal = RD(pChan->ioAddr + ASR_OFFSET);
	LockAdditionalStatus(pChan);
	return retVal;
}*/


/*LOCAL void writeASRBit(P10_TY_CO_DEV *pChan, UINT8 bit, BOOL value)
{*/
	/* Sets the specified ASR bit to 1 if value = TRUE 0 if value = FALSE */
/*	UINT8	currentASR;

	if((bit==0)||(bit==1))
	{
		UnlockAdditionalStatus(pChan);
		currentASR = RD(pChan->ioAddr + ASR_OFFSET);
		if(value)
		{*/
			/* OR bit in if setting */
/*			currentASR |= (1 << bit);
		}
		else
		{*/
			/* Mask bit out if clearing */
/*			currentASR &= ~(1 << bit);
		}
	WR(pChan->ioAddr + ASR_OFFSET, currentASR);
	LockAdditionalStatus(pChan);
	}
}*/

/***************************** setAutoCTSEnable ******************************
 *
 *  Description: Enables/Disables the automatic CTS mode (on chip)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0: disable
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void setAutoCTSEnable(P10_TY_CO_DEV *pChan, BOOL state)
{
	/* Sets the state of automatic CTS flow control enable bit to state */
	UINT8 efr = read650(pChan, EFR_OFFSET);
	/* Set the bit according to the state requested */
	if(state) efr |= EFR_AUTO_CTS_EN;
	else efr &= ~EFR_AUTO_CTS_EN;
	/* Write new value */
	write650(pChan, EFR_OFFSET, efr);
}

/***************************** setAutoRTSEnable ******************************
 *
 *  Description: Enables/Disables the automatic RTS mode (on chip)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0: disable
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void setAutoRTSEnable(P10_TY_CO_DEV *pChan, BOOL state)
{
	/* Sets the state of automatic RTS flow control enable bit to state */
	UINT8 efr = read650(pChan, EFR_OFFSET);
	/* Set the bit according to the state requested */
	if(state) efr |= EFR_AUTO_RTS_EN;
	else efr &= ~EFR_AUTO_RTS_EN;
	/* Write new value */
	write650(pChan, EFR_OFFSET, efr);
}

/***************************** setAutoDSREnable ******************************
 *
 *  Description: Enables/Disables the automatic DSR mode (on chip)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0: disable
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void setAutoDSREnable(P10_TY_CO_DEV *pChan, BOOL state)
{
	/* Sets the state of automatic DSR flow control enable bit to state */
	if(state) pChan->shadowACR |= ACR_AUTO_DSR_EN;
	else pChan->shadowACR &= ~ACR_AUTO_DSR_EN;
	/* Write new value */
	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
}

/***************************** setAutoDTREnable ******************************
 *
 *  Description: Enables/Disables the automatic DTR mode (on chip)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0: disable
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void setAutoDTREnable(P10_TY_CO_DEV *pChan, BOOL state)
{
	/* Sets the state of automatic DTR flow control enable bit to state */
	if(state) pChan->shadowACR |= ACR_AUTO_DTR_EN;
	else pChan->shadowACR &= ~ACR_AUTO_DTR_EN;
	/* Write new value */
	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
}


/************************** set950TriggerEnable ******************************
 *
 *  Description: Enables/Disables the 950 trigger mode
 *               (not supported in this driver so far)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0: disable
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
/*LOCAL void set950TriggerEnable(P10_TY_CO_DEV *pChan, BOOL state)
{

	if(state)
		pChan->shadowACR |= ACR_950_TRIGGER_EN;
	else
		pChan->shadowACR &= ~ ACR_950_TRIGGER_EN;

	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
}*/

/************************** setInBandFlowControlMode *************************
 *
 *  Description: Sets the automatic in band flow control to the given mode
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                mode          the desired mode (see OX16C954 manual)
 *
 *  Output.....:
 *
 *  Globals....:
 *****************************************************************************/
LOCAL void setInBandFlowControlMode(P10_TY_CO_DEV *pChan, UINT8 mode)
{
	/* Sets the automatic inband flow control mode to the */
	/* specified mode index in the table above */
	UINT8 modeTable[11]={0x0,0x02,0x01,0x08,0x0A,0x09,0x0B,0x04,0x06,0x05,0x07};
	UINT8 efr = read650(pChan, EFR_OFFSET) & 0xF0;
	write650(pChan, EFR_OFFSET, (UINT8)(efr | modeTable[mode]));
}

/************************** setRS485BufferEnable *****************************
 *
 *  Description: Enable/Disable automatic RS485 control (DTR)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                state         1: enable, 0:disable
 *                activeHigh    pin sense
 *
 *  Output.....:  Return  		OK
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS setRS485BufferEnable(P10_TY_CO_DEV *pChan, int state, int activeHigh)
{
	if(state)
		/* We are enabling feature - decide on pin sense */
		if(activeHigh) pChan->shadowACR |= ACR_RS485_HIGH_EN;
		else pChan->shadowACR |= ACR_RS485_LOW_EN;
	else
		/* Clear both bits if we are disabling */
		pChan->shadowACR &= ~ ACR_RS485_HIGH_EN;
		/* Write new value */
	writeICR(pChan, ACR_INDEX, pChan->shadowACR);
	return OK;
}

/******************************** setManualRTS *******************************
 *
 *  Description: Set the state of the request to send pin
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                arg           1: set, 0: clear
 *
 *  Output.....:  Return  		OK
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS setManualRTS(P10_TY_CO_DEV *pChan, int arg)
{
	UINT8 mcr;

	mcr = RD(pChan->ioAddr + MCR_OFFSET);
	if(arg)
		/* set RTS bit */
		WR(pChan->ioAddr + MCR_OFFSET, (mcr | 0x02));
	else
		/* clear RTS bit */
		WR(pChan->ioAddr + MCR_OFFSET, (mcr & ~0x02));
	return OK;
}

/******************************** setManualDTR *******************************
 *
 *  Description: Set the state of the data terminal ready pin
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                arg           1: set, 0: clear
 *
 *  Output.....:  Return  		OK
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS setManualDTR(P10_TY_CO_DEV *pChan, int arg)
{
	UINT8 mcr;

	mcr = RD(pChan->ioAddr + MCR_OFFSET);
	if(arg)
		/* set DTR bit */
		WR(pChan->ioAddr + MCR_OFFSET, (mcr | 0x01));
	else
		/* set DTR bit */
		WR(pChan->ioAddr + MCR_OFFSET, (mcr & ~0x01));
	return OK;
}

/******************************** getManualCTS *******************************
 *
 *  Description: Get the state of the clear to send pin
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *
 *  Output.....:  Return  		the state of the pin
 *
 *  Globals....:
 *****************************************************************************/
LOCAL int getManualCTS(P10_TY_CO_DEV *pChan)
{
	int	retVal;

	if ( RD(pChan->ioAddr + MSR_OFFSET) & P10_MSR_CTS )
		retVal = 1;
	else retVal = 0;
	return (retVal);
}

/******************************** getManualDSR *******************************
 *
 *  Description: Get the state of the data set ready pin
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *
 *  Output.....:  Return  		the state of the pin
 *
 *  Globals....:
 *****************************************************************************/
LOCAL int getManualDSR(P10_TY_CO_DEV *pChan)
{
	int	retVal;

	if ( RD(pChan->ioAddr + MSR_OFFSET) & P10_MSR_DSR )
		retVal = 1;
	else retVal = 0;
	return (retVal);
}

/******************************** getManualDCD *******************************
 *
 *  Description: Get the state of the data carrier detect pin
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *
 *
 *  Output.....:  Return  		the state of the pin
 *
 *  Globals....:
 *****************************************************************************/
LOCAL int getManualDCD(P10_TY_CO_DEV *pChan)
{
	int	retVal;

	if ( RD(pChan->ioAddr + MSR_OFFSET) & P10_MSR_DCD )
		retVal = 1;
	else retVal = 0;
	return (retVal);
}


/******************************** RS485Enable ********************************
 *
 *  Description: Enable/Disable the 2-wire mode of the P11 module
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         the channel descriptor
 *                arg           1: enable, 0: disable
 *
 *  Output.....:  Return 		OK
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS RS485Enable(P10_TY_CO_DEV *pChan, UINT8 arg)
{
	UINT8	mic;

	/* read previous channel settings */
	mic = RD(pChan->ioAddr2 + 0x04);

	switch (pChan->channel)
	{
		case 0:
			if (arg)
			{
				/* turn off DTR for RS485 */
				WR(pChan->ioAddr + MCR_OFFSET, (RD(pChan->ioAddr + MCR_OFFSET) & ~P10_MCR_DTR));
				mic = (mic | 0x03);
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			else
			{
				mic = (mic & 0xFC) | 0x02;
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			break;

		case 1:
			if (arg)
			{
				/* turn off DTR for RS485 */
				WR(pChan->ioAddr + MCR_OFFSET, (RD(pChan->ioAddr + MCR_OFFSET) & ~P10_MCR_DTR));
				mic = (mic | 0x0C);
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			else
			{
				mic = (mic & 0xF3) | 0x08;
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			break;

		case 2:
			if (arg)
			{
				/* turn off DTR for RS485 */
				WR(pChan->ioAddr + MCR_OFFSET, (RD(pChan->ioAddr + MCR_OFFSET) & ~P10_MCR_DTR));
				mic = (mic | 0x30);
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			else
			{
				mic = (mic & 0xCF) | 0x20;
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			break;

		case 3:
			if (arg)
			{
				/* turn off DTR for RS485 */
				WR(pChan->ioAddr + MCR_OFFSET, (RD(pChan->ioAddr + MCR_OFFSET) & ~P10_MCR_DTR));
				mic = (mic | 0xC0);
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			else
			{
				mic = (mic & 0x3F) | 0x80;
				WR(pChan->ioAddr2 + 0x04, mic);
			}
			break;


	}
	return OK;
}

/******************************** p10_atox ***********************************
 *
 *  Description: ASCII to hex converter
 *
 *----------------------------------------------------------------------------
 *  Input......:  str           the ASCII string of the hex number
 *
 *  Output.....:  Return 		the converted number
 *
 *  Globals....:
 *****************************************************************************/
UINT32 p10_atox(char *str)
{
    UINT32 l;

    sscanf(str,"%x",(unsigned int*)&l);
    return(l);
}





/*----------------------------------------------------------------------------+
|                            PCI Device functions                             |
+----------------------------------------------------------------------------*/


/******************************** p10PciFindDevice ***************************
 *
 *  Description: find the nth device with the given device & vendor ID
 *
 *----------------------------------------------------------------------------
 *  Input......:  vendorId      the vendor ID of the PCI device
 *                deviceId      the device ID of the PCI device
 *                index         the desired instance of the device
 *
 *  Output.....:  pBusNo        bus number
 *                pDeviceNo     device number
 *                pFuncNo       function number
 *
 *                Return 		OK | ERROR
 *
 *  Globals....:  -
 *****************************************************************************/
STATUS p10PciFindDevice
    (
    int vendorId,	/* vendor ID */
    int deviceId,	/* device ID */
    int	index,		/* desired instance of device */
    int * pBusNo,	/* bus number */
    int * pDeviceNo,	/* device number */
    int * pFuncNo	/* function number */
    )
    {
    STATUS status = ERROR;
    int busNo;
    int deviceNo;
    int funcNo;
    unsigned short device;
    unsigned short vendor;
    char header;

/*    if (pciLibInitStatus != OK) */			/* sanity check */
/*        return (ERROR); */

    for (busNo=0; busNo < 0x40; busNo++)				/*optimized*/
        for (deviceNo=0; deviceNo < 0x1f; deviceNo++)
            for (funcNo=0; funcNo < 1; funcNo++)		/*optimized*/
		{
		/* avoid a special bus cycle */

		if ((deviceNo == 0x1f) && (funcNo == 0x07))
		    continue;

		pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_VENDOR_ID,
				 &vendor);
		pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_DEVICE_ID,
				 &device);
		if ((vendor == (unsigned short)vendorId) &&
		    (device == (unsigned short)deviceId) &&
		    (index-- == 0))
		    {
		    *pBusNo	= busNo;
		    *pDeviceNo	= deviceNo;
		    *pFuncNo	= funcNo;
		    status	= OK;
		    return (status);							/*optimized*/
		    }

		/* goto next device if it is a single function device */

		pciConfigInByte (busNo, deviceNo, funcNo, PCI_CFG_HEADER_TYPE,
				 &header);
		if ((header & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC)
		    break;
		}

    return (status);
    }

#if 1
/******************************** sysPciFindBridge ***************************
 *
 *  Description: Find the pci bridge with the given bus numbers
 *
 *----------------------------------------------------------------------------
 *  Input......:  inBus         bus to find
 *
 *  Output.....:  outBus        found primary bus
 *                Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
LOCAL STATUS sysPciFindBridge(int maxBus, UINT8 *outBus, int *outDev)
{
	int  		pciFunc;

    int			pciBus;
    int			pciDevice;
	UINT8		tmpBus;
	UINT8		tmpBus2;
	UINT8		tmpHelp;
    UINT32 		vendor;



    /*------------------------------------------+
    | search for PCI Bridges and their busses   |
    +------------------------------------------*/



    for (pciBus = 0; pciBus < maxBus; pciBus++)				/* optimized */
        for (pciDevice = 0; pciDevice < 0x1f; pciDevice++)	/* optimized */
            for (pciFunc = 0; pciFunc < 1; pciFunc++)		/* optimized, no multifunction needed */
    {
		/*---------------------------+
		| avoid a special bus cycle  |
		+---------------------------*/

/*		if ((pciDevice == 0x1f) && (pciFunc == 0x07))
		    continue; */


		pciConfigInLong (pciBus, pciDevice, pciFunc, PCI_CFG_VENDOR_ID,
				 &vendor);

		/*---------------------------------------------------+
		| only look at vendor ID field for existence check   |
		| this field must exsist for every PCI device        |
		| if 0xFFFF is returned, go to next device           |
		+----------------------------------------------------*/

		if (((vendor & 0x0000ffff) != 0x0000FFFF))
		{


			/*------------------------------+
			| read out header type          |
			| skip if not PCI_PCI bridge    |
			+-------------------------------*/

			pciConfigInByte (	pciBus,
								pciDevice,
								pciFunc,
								PCI_CFG_HEADER_TYPE,
								&tmpHelp);

			if ((tmpHelp & PCI_HEADER_TYPE_MASK) == PCI_HEADER_PCI_PCI)
			{
				pciConfigInByte (	pciBus,
									pciDevice,
									pciFunc,
									PCI_CFG_SECONDARY_BUS,
									&tmpBus);
				DBG_P10(("P10Drv - PCI Bridge found at 0x%x, 0x%x, secBus: %d\n", pciBus, pciDevice, tmpBus));
				if (tmpBus == maxBus){
					pciConfigInByte (	pciBus,
										pciDevice,
										pciFunc,
										PCI_CFG_PRIMARY_BUS,
										&tmpBus2);
					*outBus = tmpBus2;
					*outDev = pciDevice;
					DBG_P10(("primaryBus: %d\n", tmpBus2));
					return OK;
				}
			}
		}
	}
	return ERROR;
}
#endif

/******************************** sysP10PciInit *********************************
 *
 *  Description: Find all p10 devices and fill the p10Rsrc structure
 *
 *----------------------------------------------------------------------------
 *  Input......:
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
LOCAL STATUS sysP10PciInit()
{
	int     	pciBus;
    int     	pciDevice;
    int  		pciFunc;
    int			unit;
    int			found = 0;
    UINT32		membaseCsr;
    UINT32		iobaseCsr;
    UINT32		membaseCsr2;
    UINT32		iobaseCsr2;
	char    	irq;

	int			inBus;
	UINT8		outBus;
	int			outDev;
	int			pathCount = 0;
	int			devicePath[16];
	int 		i;

#ifndef INCLUDE_PCI2MMU
		UINT16	tmpWord;
		UINT32	tmpLong;
		UINT32	tmpLong2;
#endif

    /*----------------------+
    | search for P10 units  |
    +----------------------*/
	DBG_P10(("P10Drv - PCI Init\n"));

	/* dont find them twice */
	if (p10Units > 0) {
		DBG_P10(("P10Drv - %d P10 modules already found\n", p10Units));
		return OK;
	}

  	/* try to find P10 */
	for (unit = 0; unit < P10_MAX_UNITS; unit++) {
    	if ( p10PciFindDevice(	PCI_VENDOR_ID_OX,
								PCI_DEVICE_ID_OX954,
    							p10Units,
								&pciBus, &pciDevice, &pciFunc ) == OK){
			DBG_P10(( "P10Drv - Found P10 at PCI %d/%d/%d unit=%d\n",
					  pciBus, pciDevice, pciFunc, unit ));
			/* board detected */
			found = TRUE;

			p10Rsrc [unit] = p10PciResrcs + p10Units;

			p10Rsrc [unit]->vendorId  = PCI_VENDOR_ID_OX;
			p10Rsrc [unit]->deviceId  = PCI_DEVICE_ID_OX954;

			p10Rsrc [unit]->pciBus    = pciBus;
			p10Rsrc [unit]->pciDevice = pciDevice;
			p10Rsrc [unit]->pciFunc   = pciFunc;
			p10Rsrc [unit]->unitNumber = p10Units;

			p10Units++;  /* number of P10 units found */

		} else {
			break;
		}
	} /* for */


    if ((found != TRUE) || (pciDevice > 32))
		return (ERROR);

	/*-------------------------------+
	| initialize the units we found  |
	+--------------------------------*/

	for ( unit = 0; unit < p10Units; unit++)

	{

        /* Fill in the resource entry */
        /*p10Rsrc [unit] = p10PciResrcs + unit;*/
		/* get PCI path for that unit */
		/* search for PCI_PCI_BRIDGE
		   with SECONDARY_BUS[0] = p10Rsrc[unit]->pciBus
		   search for PCI_PCI_BRIDGE
		   with SECONDARY_BUS[1] = PRIMARY_BUS[0]
		   etc and save the device numbers */
		DBG_P10(("P10Drv - get pci device path\n"));
		inBus = p10Rsrc[unit]->pciBus;

		do {
			if (ERROR == sysPciFindBridge (	inBus, &outBus,	&outDev)) {
				if( inBus == 0 ) {
					outBus = 0;
					outDev = 0;
				} else
					return ERROR;
			}

			if( !((outBus == 0) && (outDev == 0)) )  {
				devicePath[pathCount] = outDev;
				pathCount++;
				DBG_P10(("P10Drv-unit %d, inBus: %d, outBus: %d, outDev: %d\n",
						 unit, inBus, outBus, outDev));
				inBus = outBus;
			}


		} while ((outBus != 0) | (pathCount>16));


		/* create device path */
		p10Path[unit].unitNumber = unit;
		p10Path[unit].installed = FALSE;
		for (i=0; i<pathCount; i++)
			p10Path[unit].path[i] = devicePath[(pathCount-1) - i];

		p10Path[unit].pathLen = pathCount;

		DBG_P10(("P10Drv - unit %d, pathLen: %d, path: ",
				 unit, p10Path[unit].pathLen));
		if( !pathCount )
			DBG_P10(("onboard"));
		else
			for (i=0; i<pathCount; i++){
				DBG_P10(("%d, ", p10Path[unit].path[i]));
			}
		DBG_P10(("\n"));

		/* reset counter for next unit */
		pathCount = 0;

		/* if sysPci2Mmu.c included (#define INCLUDE_PCI2MMU in config.h),
		   dont map device again */
#ifdef INCLUDE_PCI2MMU

		/*-----------------------------------------------------+
     	| get memory base address and IO base address          |
     	+-----------------------------------------------------*/

		pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	         				PCI_CFG_BASE_ADDRESS_0,
	         				&iobaseCsr);

    	pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
                        	PCI_CFG_BASE_ADDRESS_1,
                        	&membaseCsr);

		pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	         				PCI_CFG_BASE_ADDRESS_2,
	         				&iobaseCsr2);

    	pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
                        	PCI_CFG_BASE_ADDRESS_3,
                        	&membaseCsr2);

    	pciConfigInByte (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
                        	PCI_CFG_DEV_INT_LINE,
                        	&irq);

		DBG_P10(( "1. sysP10pciInit: iobaseCsr 0x%08x membaseCsr 0x%08x"
				  " iobaseCsr2: 0x%08x, membaseCsr2: 0x%08x irq: 0x%x\n",
				  iobaseCsr, membaseCsr, iobaseCsr2, membaseCsr2, irq));

        /*-------------------------------------------------------------------+
        | mask off registers. IO base needs to be masked off because bit0    |
        | will always be set to 1                                            |
        +--------------------------------------------------------------------*/

        iobaseCsr &= PCI_IOBASE_MASK;
		iobaseCsr += p10Drv_CpuToPciIo;

		iobaseCsr2 &= PCI_IOBASE_MASK;
		iobaseCsr2 += p10Drv_CpuToPciIo;



	    membaseCsr &= PCI_MEMBASE_MASK;
	    membaseCsr += p10Drv_CpuToPciMem;

		membaseCsr2 &= PCI_MEMBASE_MASK;
	    membaseCsr2 += p10Drv_CpuToPciMem;

		DBG_P10(( "2.sysP10pciInit: iobaseCsr 0x%08x membaseCsr 0x%08x"
				  " iobaseCsr2: 0x%08x, membaseCsr2: 0x%08x irq: 0x%x\n",
				  iobaseCsr, membaseCsr, iobaseCsr2, membaseCsr2, irq));



		/* if sysPci2Mmu.c NOT included (in config.h), map device */
#else

		/*-------------------------------------------------------------------+
        | get memory base address and IO base address                        |
        | Note: we read it in again, even if we just wrote it out cause the  |
        | device can change what we wrote                                    |
        +-------------------------------------------------------------------*/

		pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	         				PCI_CFG_BASE_ADDRESS_0,
	         				&iobaseCsr);

    	pciConfigInLong (	p10Rsrc[unit]->pciBus,
			 				p10Rsrc[unit]->pciDevice,
			 				p10Rsrc[unit]->pciFunc,
                        	PCI_CFG_BASE_ADDRESS_1,
                        	&membaseCsr);

		pciConfigInLong (	p10Rsrc[unit]->pciBus,
			 				p10Rsrc[unit]->pciDevice,
			 				p10Rsrc[unit]->pciFunc,
	         				PCI_CFG_BASE_ADDRESS_2,
	         				&iobaseCsr2);

    	pciConfigInLong (	p10Rsrc[unit]->pciBus,
			 				p10Rsrc[unit]->pciDevice,
			 				p10Rsrc[unit]->pciFunc,
                        	PCI_CFG_BASE_ADDRESS_3,
                        	&membaseCsr2);

    	pciConfigInByte (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
                        	PCI_CFG_DEV_INT_LINE,
                        	&irq);

		DBG_P10((" P10Drv - iobaseCsr: 0x%08x, membaseCsr: 0x%08x, "
				 "iobaseCsr2: 0x%08x, membaseCsr2: 0x%08x irq: 0x%x\n",
					p10Rsrc[unit]->iobaseCsr,
					p10Rsrc[unit]->membaseCsr,
					p10Rsrc[unit]->iobaseCsr2,
					p10Rsrc[unit]->membaseCsr2,
					p10Rsrc[unit]->irq));

		/*-------------------------------------------------------+
		| we want to make sure the device implements memory BARs |
	 	| first, we disable PCI memory decode by the device      |
	 	+--------------------------------------------------------*/

        pciConfigInWord (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	                 		PCI_CFG_COMMAND,
	                 		&tmpWord);


        pciConfigOutWord (	p10Rsrc [unit]->pciBus,
			  				p10Rsrc [unit]->pciDevice,
			  				p10Rsrc [unit]->pciFunc,
	                  		PCI_CFG_COMMAND,
	                  		(tmpWord & (~PCI_CMD_MEM_ENABLE)));


		/*-----------------------+
		| Read and save the BAR  |
		+----------------------- */

        pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	                 		PCI_CFG_BASE_ADDRESS_1,
	                 		&tmpLong);

		/*------------------------------------------+
		|  check if memory address is implemented   |
		+------------------------------------------*/

        pciConfigOutLong (	p10Rsrc [unit]->pciBus,
			  				p10Rsrc [unit]->pciDevice,
			  				p10Rsrc [unit]->pciFunc,
	                  		PCI_CFG_BASE_ADDRESS_1,
	                  		0xffffffff);

        pciConfigInLong (	p10Rsrc [unit]->pciBus,
			 				p10Rsrc [unit]->pciDevice,
			 				p10Rsrc [unit]->pciFunc,
	                 		PCI_CFG_BASE_ADDRESS_1,
	                 		&tmpLong2);

		if (tmpLong2 != 0)
	    {

   		/*-------------------------------------+
   		|  this is a legitimate memory BAR     |
   		|  so write back the saved register    |
   		+--------------------------------------*/

	    	pciConfigOutLong (	p10Rsrc [unit]->pciBus,
				     			p10Rsrc [unit]->pciDevice,
			    	 			p10Rsrc [unit]->pciFunc,
			     				PCI_CFG_BASE_ADDRESS_1,
			     				tmpLong);
	    } else {

	    /*----------------------------------+
	    | the driver will use IO-mapped BAR |
	    +-----------------------------------*/

	    	membaseCsr = NONE;
	    }


		/*------------------------------------------+
		| re-enable PCI memory decode by the device |
		+-------------------------------------------*/

        pciConfigOutWord (	p10Rsrc [unit]->pciBus,
			  				p10Rsrc [unit]->pciDevice,
			  				p10Rsrc [unit]->pciFunc,
	                  		PCI_CFG_COMMAND,
	                  		(tmpWord));

        /*-------------------------------------------------------------------+
        | mask off registers. IO base needs to be masked off because bit0    |
        | will always be set to 1                                            |
        +--------------------------------------------------------------------*/

        iobaseCsr &= PCI_IOBASE_MASK;
		iobaseCsr += p10Drv_CpuToPciIo;

		iobaseCsr2 &= PCI_IOBASE_MASK;
		iobaseCsr2 += p10Drv_CpuToPciIo;

		if (membaseCsr != NONE)
        	{


	    	membaseCsr &= PCI_MEMBASE_MASK;
	    	membaseCsr += p10Drv_CpuToPciMem;

			membaseCsr2 &= PCI_MEMBASE_MASK;
	    	membaseCsr2 += p10Drv_CpuToPciMem;

			/*------------------------------------+
			| if implemented, map device to MMU   |
			+------------------------------------*/

            } /* if (membaseCsr != NONE) */


#endif /* INCLUDE_PCI2MMU */

		/*------------------------------------------------+
		| over write the resource table with values read  |
		+-------------------------------------------------*/

		p10Rsrc[unit]->membaseCsr   = membaseCsr;
		p10Rsrc[unit]->membaseCsr2  = membaseCsr2;
#if (CPU == PPC85XX)
		p10Rsrc[unit]->iobaseCsr    = membaseCsr;
		p10Rsrc[unit]->iobaseCsr2   = membaseCsr2;
#else
		p10Rsrc[unit]->iobaseCsr    = iobaseCsr;
		p10Rsrc[unit]->iobaseCsr2   = iobaseCsr2;
#endif

		p10Rsrc[unit]->irq          = irq;
    	p10Rsrc[unit]->irqvec       = irq + EXT_INTERRUPT_BASE;

		/*---------------------------------------+
		| enable mapped memory and IO addresses  |
		+----------------------------------------*/

        pciConfigOutByte (	p10Rsrc [unit]->pciBus,
			  				p10Rsrc [unit]->pciDevice,
		  					p10Rsrc [unit]->pciFunc,
                   	      	PCI_CFG_COMMAND,
                       	  	PCI_CMD_IO_ENABLE |
                       		PCI_CMD_MEM_ENABLE |
                       		PCI_CMD_MASTER_ENABLE);

        /*--------------------+
        | disable sleep mode  |
        +---------------------*/

   		pciConfigOutByte (	p10Rsrc [unit]->pciBus,
			  				p10Rsrc [unit]->pciDevice,
			  				p10Rsrc [unit]->pciFunc,
		  					PCI_CFG_MODE,
                   	      	SLEEP_MODE_DIS);

	} /* for */

	for (unit = 0; unit < p10Units; unit++)
	{
		DBG_P10(("\nP10Drv - unit: %d, vendorId: 0x%04x, deviceId: 0x%04x,\n"
				 "membase: 0x%08x, iobase: 0x%08x, membase2: 0x%08x,\n"
				 "iobase2: 0x%08x irq: %d, irqvec: 0x%08x\n",
			   unit, p10Rsrc[unit]->vendorId, p10Rsrc[unit]->deviceId,
			   p10Rsrc[unit]->membaseCsr, p10Rsrc[unit]->iobaseCsr,
			   p10Rsrc[unit]->membaseCsr2, p10Rsrc[unit]->iobaseCsr2,
			   p10Rsrc[unit]->irq, p10Rsrc[unit]->irqvec));
    }
	DBG_P10(("P10 Init OK !\n"));

    return(OK);
}  /* sysP10Init */







/*---------------------------------------------------------------------------+
|                           Main driver functions                            |
+---------------------------------------------------------------------------*/
/******************************** p10DrvId *************************************
 *
 *  Description: displays revision and configuration informations.
 *
 *----------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  -
 *****************************************************************************/
void p10DrvId( void )
{
	printf("%s\n", RCSid );
	printf("CPU2PCI MEM %08x\n", (int)p10Drv_CpuToPciMem );
	printf("CPU2PCI I/O %08x\n", (int)p10Drv_CpuToPciIo  );
	printf("IRQ0     	%d\n" , EXT_INTERRUPT_BASE );
}

/****************************** p10DevInit ***********************************
 *
 *  Description:  Set one channel of the P10 to a quiescent state.
 *                Should be called for each channel form sysSerialHwInit()
 *                for console initialization at startup.
 *---------------------------------------------------------------------------
 *  Input......:  pP10Dev			pointer of device data structure
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
void p10DevInit( P10_TY_CO_DEV *pP10Chan )
{
	p10InitChannel( pP10Chan, P10_CREATE_TYPE_AT_BOOT );

	/* set func pointer of structure */
	pP10Chan->u.sioT.pDrvFuncs = &sioDrvFuncs;
}/*p10DevInit*/



/******************************** p10Drv *************************************
 *
 *  Description: Initialize the TTY driver
 *
 *----------------------------------------------------------------------------
 *  Input......:  devName       device name for the module
 *                inString      the PCI path of the module
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
STATUS p10Drv (char *devName, char *inString)
{
    P10_PCI_RESOURCES	*myP10Rsrc;
    int					devicePath[16];
    BOOL				equal = TRUE;
    char				tyName [20];
    int 				i;
    int					k;
    int					jj = 0;
    int					channel;
    int					unit;
    char				helpChar[5];
    char				*pHelpChar = &helpChar[0];



    /*------------------------+
    | check device name given |
    +------------------------*/
    if ((devName == NULL) || (devName[0] != '/'))
    {
    	fprintf (stderr, "p10Drv - no or wrong device name!\n");
    	goto USAGE;
    }

    /*--------------------------+
    | check for duplicate names |
    +--------------------------*/
    for ( i = 0; i < NELEMENTS (p10Path); i++)
    {
    	if (!strcmp (p10Path[i].devName, devName))
    	{
    		fprintf (stderr, "p10Drv - duplicate device name!\n");
    		goto USAGE;
    	}
    }


    /*---------------------------+
    | check and init pci modules |
    +---------------------------*/
    if (p10Units == 0)
    {
    	if (sysP10PciInit() == ERROR)
    	{
    		fprintf (stderr, "P10Drv - no P10 modules found!\n");
    		goto USAGE;
    	}
    }

    /*----------------------------------+
    | check if driver already installed |
    +----------------------------------*/
	if (p10DrvNum == p10Units)
	{
		fprintf (stderr, "P10Drv - drivers for all modules installed\n");
		goto USAGE;
	}


	/*-----------------------------+
	| no parsing for single module |
	+-----------------------------*/
	if (p10Units == 1)
	{
		DBG_P10(("P10Drv - single P10 module found, no parsing\n"));
		k = 0;
	}


	/*----------------------+
	| parsing argument list |
	+----------------------*/
	else
	{
		if (inString == NULL)
		{
			fprintf (stderr, "P10Drv - no PCI path given!\n");
			goto USAGE;
		}

		/* check arguments with p10Path */
		/* get arguments from argument list */
		DBG_P10(("P10Drv - parsing argument list\n"));


		for( i=0; i<strlen(inString); i++ )
		{
			if( inString[i] == 'x')
			{
				helpChar[0] = '0';
				helpChar[1] = inString[i];
				helpChar[2] = inString[i+1];
				helpChar[3] = inString[i+2];
				helpChar[4] = '\0';

				devicePath[jj] = (int)p10_atox(pHelpChar);
				jj++;
			}
		}



		/*---------------------------+
		| compare argument list with |
		| pci path list              |
		+---------------------------*/
		for (k=0; k<p10Units; k++)
		{
			if (p10Path[k].installed)
				continue;

			equal = TRUE;
			for (i=0; i<p10Path[k].pathLen; i++) {
				if (devicePath[i] != p10Path[k].path[i])
					equal = FALSE;

			}

			if ((p10Path[k].pathLen > 0) && (equal == TRUE))
				break;
			else if ( (p10Path[k].pathLen == 0) &&
					  ( (!strcmp (inString, "onboard")) ||
					    (!strcmp (inString, "ONBOARD")) ) )
				break;
		}

		if (equal != TRUE){
			fprintf (stderr, "P10Drv - arguments not valid!\n");
			return ERROR;
		}
	}
	myP10Rsrc = p10Rsrc[k];

	/*-----------------------------------------------+
	| fill register structure with PCI register data |
	+-----------------------------------------------*/
	DBG_P10(("P10Drv -  filling driver structure\n"));

    unit = myP10Rsrc->unitNumber;
    for (i = 0; i < P10_NUM_CHANNEL; i++)
    {
    	/* clean */
	    bzero( (char*)&p10TyCoDev[unit][i], sizeof(P10_TY_CO_DEV) );

	    /* fill */
    	p10TyCoDev[unit][i].ioAddr 		= myP10Rsrc->iobaseCsr+i*UARTS_OFFS;
    	p10TyCoDev[unit][i].ioAddr2 	= myP10Rsrc->iobaseCsr2;
    	p10TyCoDev[unit][i].channel 	= i;
    	p10TyCoDev[unit][i].irq    		= myP10Rsrc->irq;
    	p10TyCoDev[unit][i].irqvec 		= myP10Rsrc->irqvec;
    	p10TyCoDev[unit][i].created		= FALSE;
    	p10TyCoDev[unit][i].auto_RS485  = FALSE;
		p10TyCoDev[unit][i].auto_RTS    = FALSE;
		p10TyCoDev[unit][i].auto_CTS    = FALSE;
		p10TyCoDev[unit][i].auto_DTR    = FALSE;
		p10TyCoDev[unit][i].auto_DSR    = FALSE;
		p10TyCoDev[unit][i].auto_XONXOFF= FALSE;
		p10TyCoDev[unit][i].shadowACR   = 0;
    }

	DBG_P10(("p10TyCoDev[%d][0]: ioAddr 0x%x, irq: %d, irqvec: 0x%x\n",
			 unit, p10TyCoDev[unit][0].ioAddr, p10TyCoDev[unit][0].irq,
			 p10TyCoDev[unit][0].irqvec));


	/*---------------------+
	| calling ioDrvInstall |
	+---------------------*/
	if( p10TyCoDrvNum == NONE ) /* only once */
	{
		DBG_P10(("P10Drv -  calling ioDrvInstall\n"));
    	p10TyCoDrvNum = iosDrvInstall( p10Open, (FUNCPTR) NULL, p10Open,
					                  (FUNCPTR) NULL,
					                   tyRead, tyWrite, p10Ioctl
					                 );
    }/*if*/

	/*----------------------------------------+
	| creates/initialize all (4) channels of  |
	| the P10                                 |
	+----------------------------------------*/
	for ( channel = 0; channel < P10_NUM_CHANNEL; channel++ )
	{
		DBG_P10(("P10Drv - calling tyDevInit (%d)\n", channel));
	    if( tyDevInit( &p10TyCoDev[unit][channel].u.ttyT.tyDev, 512, 512,
			           (FUNCPTR)p10Startup ) != OK
		   )
		{
			printErr( "*** P10Drv - tyDevInit(%d) failed!\n", channel);
			goto CLEANUP;
		}

		sprintf (tyName, "%s%s%d", devName, "/", channel);
		if( ERROR == p10InitChannel( &p10TyCoDev[unit][channel],
		                              P10_CREATE_TYPE_LATE )
		  )
		{
			printErr( "*** P10Drv - p10InitChannel(%d) failed!\n", channel);
			goto CLEANUP;
		}

		sprintf (tyName, "%s%s%d", devName, "/", channel);
	    if( ERROR ==
	        iosDevAdd ( &p10TyCoDev[unit][channel].u.ttyT.tyDev.devHdr,
	                    tyName, p10TyCoDrvNum )
	      )
	    {
			printErr( "*** P10Drv - iosDevAdd\n" );
			goto CLEANUP;
	    }/*if*/
	}

	/*-----------------------------+
	| connecting interrupt routine |
	+-----------------------------*/
	DBG_P10(("P10Drv -  sysIntConnect P10/%d\n", unit));
	sysIntConnect( (VOIDFUNCPTR*)  INUM_TO_IVEC(myP10Rsrc->irqvec),
                   (VOIDFUNCPTR)   p10Int,
                   (int)           &p10TyCoDev[unit][0] );

	/*-------------------------+
	| enable interrupt         |
	+-------------------------*/
	DBG_P10(("P10Drv -  enable interrupt\n"));
   	intEnable( myP10Rsrc->irq );

    /* enable the receiver interrupts */
	for ( channel = 0; channel < P10_NUM_CHANNEL; channel++ )
	{
	    WR(p10TyCoDev[unit][channel].ioAddr + IER_OFFSET, P10_IER_RXRDY);
		p10TyCoDev[unit][channel].channelMode = SIO_MODE_INT;
	}

    p10Path[k].installed = TRUE;
	strcpy (p10Path[k].devName, devName);
    p10DrvNum++;


	return OK;




USAGE:
	fprintf (stderr, "USAGE: p10Drv [deviceName], [pciPath]\n");
	fprintf (stderr, "EXAMPLE: p10Drv ('/p10', '0x08 0x0d 0x0e')\n");
	fprintf (stderr, "The device name must start with '/'\n\n");
	return ERROR;

CLEANUP:

	fprintf (stderr, "p10Drv - driver installation aborted!\n");
	return ERROR;
}

/******************************** p10InitChannel *****************************
 *
 *  Description: Initialize a P10 channel.
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan
 *                createType
 *  Output.....:  Return 		OK | ERROR
 *  Globals....:  -
 *****************************************************************************/
LOCAL STATUS p10InitChannel
(
	P10_TY_CO_DEV *pChan,
	int createType
)
{
	int efr;

	DBG_P10(("P10Drv - p10InitChannel\n"));

    /* if this device already exists, don't create it */
    if (pChan->created)
		return (ERROR);

    /* intLock() was removed, because I assume, that the interrupt handler
     * can't access the channel, if the IRQ routine installation/IRQ enable
     * happens, after calling this function.
     */

	/* disable interrupts of the channel */
    WR( pChan->ioAddr + IER_OFFSET, 0x00 );

    /*--------------------------+
    | set baudrate to 9600 Baud |
    +---------------------------*/
    p10BaudSet (pChan, P10_DEFAULT_BAUD);

    /*--------------------------------+
    | set 16C950 mode                 |
    | with 16 Byte FIFO trigger level |
    +--------------------------------*/
	/* change to 16C650 mode */
	efr = read650(pChan, EFR_OFFSET);

	write650(pChan, EFR_OFFSET, (UINT8)(efr | 0x10));

	WR(pChan->ioAddr + FCR_OFFSET, FIFO_ENABLE);

   	/*WR(pChan->ioAddr + ISR_OFFSET, ( RxCLEAR | TxCLEAR | FIFO_ENABLE));*/

   	/* setting trigger levels to 16  Byte */
   	WR(pChan->ioAddr + FCR_OFFSET,
   	  (FCR_RX_TRIGGER_16 | FCR_TX_TRIGGER_16 |FCR_DMA_MODE | RxCLEAR | TxCLEAR | FIFO_ENABLE));

   	pChan->transmit_level = FCR_TX_TRIGGER_16;
   	pChan->receive_level = FCR_RX_TRIGGER_16;
   	pChan->fifo_enabled = TRUE;

    /* 8 data bits, 1 stop bit, no parity */
    WR(pChan->ioAddr + LCR_OFFSET, (P10_LCR_CS8 | P10_LCR_1_STB));
    WR(pChan->ioAddr + MCR_OFFSET, (P10_MCR_RTS | P10_MCR_DTR | P10_MCR_OUT2));

    RD(pChan->ioAddr + RHR_OFFSET);	/* clear the port */

    pChan->options = (CLOCAL | CREAD | CS8);

    /* mark the device as created */
    pChan->created = createType;

    return( OK );
}



/********************************* p10SetFifoLevel ***************************
 *
 *  Description: Initialize hardware to use fifos
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan			channel device pointer
 *                select		select transmitter(1) or receiver(0)
 *                value         value of trigger level
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
LOCAL STATUS p10SetFifoLevel
    (
    P10_TY_CO_DEV	*pChan,
    UINT8			select,
    UINT8           value
    )
{
	int		oldLevel;
	int     efr;
	STATUS  error = ERROR;

	/* check index number */
	oldLevel = intLock ();



    /* change to 16C650 mode */
	efr = read650(pChan, EFR_OFFSET);

	write650(pChan, EFR_OFFSET, (UINT8)(efr | 0x10));

	/* setting transmitt trigger level */
	if (select == 1)
	{
		pChan->transmit_level = value & 0x30;
		WR(pChan->ioAddr + FCR_OFFSET, FIFO_ENABLE);
		WR(pChan->ioAddr + FCR_OFFSET, (UINT8)(pChan->receive_level | pChan->transmit_level | FCR_DMA_MODE | RxCLEAR | TxCLEAR | FIFO_ENABLE));
    	pChan->fifo_enabled = TRUE;
    	error = OK;
	}
	/* setting receive trigger level */
	else if (select == 0)
	{
		pChan->receive_level = value & 0xC0;
		WR(pChan->ioAddr + FCR_OFFSET, FIFO_ENABLE);
		WR(pChan->ioAddr + FCR_OFFSET, (UINT8)(pChan->receive_level | pChan->transmit_level | FCR_DMA_MODE | RxCLEAR | TxCLEAR | FIFO_ENABLE));
    	pChan->fifo_enabled = TRUE;
    	error = OK;
	}
	/* disable FIFOS */
	else if (select == 0xFF)
	{

		WR(pChan->ioAddr + FCR_OFFSET, (UINT8)( RxCLEAR | TxCLEAR | FIFO_ENABLE));
		WR(pChan->ioAddr + FCR_OFFSET, 0x00);
		pChan->fifo_enabled = FALSE;
		error = OK;
    }


    intUnlock(oldLevel);


	return (error);

}




/******************************** p10Hup *********************************
 *
 *  Description: Hang up the modem control lines
 *               Resets RTS and DTR signals
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         pointer to channel device
 *
 *  Output.....:  Return 		always OK
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS p10Hup
    (
    P10_TY_CO_DEV 		*pChan 		/* pointer to channel */
    )
    {
    FAST int     oldlevel;	/* current interrupt level mask */

    oldlevel = intLock ();

    WR(pChan->ioAddr + MCR_OFFSET, P10_MCR_OUT2);

	/* reset FIFOs to 16 Byte */
	WR(pChan->ioAddr + FCR_OFFSET, FIFO_ENABLE);
	WR(pChan->ioAddr + FCR_OFFSET, (FCR_RX_TRIGGER_16 | FCR_TX_TRIGGER_16 |FCR_DMA_MODE | RxCLEAR | TxCLEAR | FIFO_ENABLE) );
	pChan->transmit_level = FCR_TX_TRIGGER_16;
    pChan->receive_level = FCR_RX_TRIGGER_16;
    pChan->fifo_enabled = TRUE;
    intUnlock (oldlevel);

    return (OK);

}

/******************************** p10Open ************************************
 *
 *  Description: open a file to the channel
 *
 *  This routine opens a file to the channel and gives back the file
 *  descriptor to the system. Called by tty function open.
 *
 *
 *----------------------------------------------------------------------------
 *  Input......:  p10TyDev      device tty structure
 *                name          the name of the device (not used)
 *                mode          the mode (not used)
 *
 *  Output.....:  Return 		pointer to p10TyDev
 *
 *  Globals....:
 *****************************************************************************/
LOCAL int p10Open
    (
    P10_TY_CO_DEV *pChan,
    char *name,
    int mode)

{
	FAST int	oldlevel;	/* current interrupt level mask */
    char		mask;
	int			efr;

    /* read modem control register */

    mask = (RD(pChan->ioAddr + MCR_OFFSET) & (P10_MCR_RTS | P10_MCR_DTR));

	oldlevel = intLock ();

	/* change to 16C950 mode */
	efr = read650(pChan, EFR_OFFSET);

	write650(pChan, EFR_OFFSET, (UINT8)(efr | 0x10));

	WR(pChan->ioAddr + FCR_OFFSET, FIFO_ENABLE);

    /* setting trigger levels to 16  Byte */
    WR(pChan->ioAddr + FCR_OFFSET, (FCR_RX_TRIGGER_16 | FCR_TX_TRIGGER_16 |FCR_DMA_MODE | RxCLEAR | TxCLEAR | FIFO_ENABLE));
	pChan->transmit_level = FCR_TX_TRIGGER_16;
    pChan->receive_level = FCR_RX_TRIGGER_16;
	pChan->fifo_enabled = TRUE;

    if (mask != (P10_MCR_RTS | P10_MCR_DTR))
    {
    	/* RTS and DTR not set yet */

		/* set RTS and DTR TRUE */

    	WR(pChan->ioAddr + MCR_OFFSET, (P10_MCR_RTS | P10_MCR_DTR | \
    					P10_MCR_OUT2));
	}

	intUnlock (oldlevel);
    return ((int) pChan);
}

/******************************** p10BaudSet *********************************
 *
 *  Description: Set the baud rate for the channel
 *               The interrupts are disabled during chip access
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         pointer to channel device
 *                baud          requested baud rate
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:
 *****************************************************************************/
LOCAL STATUS  p10BaudSet
	(
	P10_TY_CO_DEV	*pChan,	/* pointer to channel */
   	UINT	   		baud		/* requested baud rate */
   	)
	{
    STATUS	status;
    FAST	int	ix;



    status = ERROR;

    for (ix = 0; ix < NELEMENTS (p10BaudTable); ix++)
	{
		if (p10BaudTable [ix].rate == baud)	/* lookup baud rate value */
	    {
		    /* disable interrupts during chip access */
	    	writeDivisor(pChan, p10BaudTable[ix].preset);
			status = OK;
			break;
		}
	}


    return (status);
	}


/******************************** p10OptsSet ***********************************
 *
 *  Description: Set the channel serial options
 *
 *               Supported options:
 *               CLOCAL, HUPCL, CREAD, CSIZE, PARENB, PARODD
 *
 *------------------------------------------------------------------------------
 *  Input......:  pChan         pointer to channel device
 *                options       new hardware options to set
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:
 *******************************************************************************/
LOCAL STATUS p10OptsSet
    (
    P10_TY_CO_DEV 		*pChan,		/* pointer to channel */
    UINT 				options		/* new hardware options */
    )
    {
    FAST int     oldlevel;		/* current interrupt level mask */
    char lcr = 0;
    char mcr = P10_MCR_OUT2;
    char ier = RD(pChan->ioAddr + IER_OFFSET);
    char mask;

    if (pChan == NULL || options & 0xffffff00)
	return ERROR;

    switch (options & CSIZE)
	{
	case CS5:
	    lcr = P10_LCR_CS5; break;
	case CS6:
	    lcr = P10_LCR_CS6; break;
	case CS7:
	    lcr = P10_LCR_CS7; break;
	default:
	case CS8:
	    lcr = P10_LCR_CS8; break;
	}

    if (options & STOPB)
	lcr |= P10_LCR_2_STB;
    else
	lcr |= P10_LCR_1_STB;

    switch (options & (PARENB | PARODD))
	{
	case PARENB|PARODD:
	    lcr |= P10_LCR_PEN; break;
	case PARENB:
	    lcr |= (P10_LCR_PEN | P10_LCR_EPS); break;
	default:
	case 0:
	    lcr |= P10_LCR_PDIS; break;
	}

	/* disable interrupts */
    WR(pChan->ioAddr + IER_OFFSET, 0x00);

    if (!(options & CLOCAL))
	{
	/* !clocal enables hardware flow control(DTR/DSR) */

	mcr |= (P10_MCR_DTR | P10_MCR_RTS);
	ier |= P10_IER_MSI;    /* enable modem status interrupt */

    	mask = RD(pChan->ioAddr + MSR_OFFSET) & P10_MSR_CTS;

   	/* if the CTS is asserted enable Tx interrupt */

   	if (mask & P10_MSR_CTS)
        	ier |= P10_IER_TBE;
	else
    		ier &= (~P10_IER_TBE);

	}
    else
        ier &= ~P10_IER_MSI; /* disable modem status interrupt */

    oldlevel = intLock ();

    WR(pChan->ioAddr + LCR_OFFSET, lcr);
    WR(pChan->ioAddr + MCR_OFFSET, mcr);

    /* now clear the port */



    RD(pChan->ioAddr + RHR_OFFSET);

    if (options & CREAD)
		ier |= P10_IER_RXRDY;

	/* set saved interrupt mask */
    WR(pChan->ioAddr + IER_OFFSET, ier);

    intUnlock (oldlevel);

    pChan->options = options;

    return OK;
}



/******************************** p10Ioctl ***********************************
 *
 *  Description: Special device control
 *
 *               The following commands are included:
 *               get/set baud rate, set mode (INT),
 *               hardware options (parity, number of data bits),
 *               modem control (RTS/CTS and DTR/DSR),
 *               modem hang up (SIO_HUP, SIO_OPEN)
 *
 *----------------------------------------------------------------------------
 *  Input......:  pChan         pointer to channel device
 *                request       request code
 *                arg           argument for request
 *
 *  Output.....:  Return 		OK | EIO on device error
 *                              | ENOSYS on unsupported request
 *
 *  Globals....:
 *****************************************************************************/
 static int p10Ioctl
    (
    P10_TY_CO_DEV 		*pChan,	/* device to control */
    int 				request,		/* request code */
    int 				arg			/* some argument */
    )
    {
    int ix;
    int divisor;
    int oldlevel;
    int status = OK;

    switch (request)
	{
		case SIO_BAUD_SET:
		case FIOBAUDRATE:
		    oldlevel = intLock ();
	    	status = (p10BaudSet(pChan, arg) == OK) ? OK : EIO;
		    intUnlock(oldlevel);
	    	break;

        case SIO_BAUD_GET: /* not supported in standard drivers */
            status = EIO;
            oldlevel = intLock();
            divisor = readDivisor ( pChan );
            intUnlock(oldlevel);

            for (ix = 0; ix < NELEMENTS (p10BaudTable); ix++)
            {
                if ( divisor  ==  (p10BaudTable[ix].preset))
                {
                    *(int *)arg = p10BaudTable[ix].rate;
                    status = OK;
                }
            }

            break;


    case SIO_HW_OPTS_SET: /* not supported in standard drivers */
    	    status = (p10OptsSet (pChan, arg) == OK) ? OK : EIO;
    	    break;

    case SIO_HW_OPTS_GET: /* not supported in standard drivers */
            *(int *)arg = pChan->options;
            break;

    case SIO_HUP: /* not supported in standard drivers */
            /* check if hupcl option is enabled */

    	    if (pChan->options & HUPCL)
	    	status = p10Hup (pChan);
            break;

    case SIO_MODE_SET:
	        pChan->channelMode = arg;
            break;

    case SIO_MODE_GET:
            *(int *)arg = pChan->channelMode;
            break;

    case SIO_AVAIL_MODES_GET:
            *(int *)arg = SIO_MODE_INT;
            break;

	case P10_TX_FIFO_LEVEL:
			status = (p10SetFifoLevel (pChan, 1, arg) == OK) ? OK : EIO;
			break;

	case P10_RX_FIFO_LEVEL:
			status = (p10SetFifoLevel (pChan, 0, arg) == OK) ? OK : EIO;
			break;

	case P10_NO_FIFOS:
			status = (p10SetFifoLevel (pChan, 0xFF, arg) == OK) ? OK : EIO;
			break;




	case P10_MANUAL_RTS:
			/* check if RTS line in use */
			if (pChan->auto_RTS)
				status = EIO;
			else status = setManualRTS(pChan, arg);
			break;

	case P10_MANUAL_DTR:
			/* check if DTR line in use */
			if ((pChan->auto_RS485) | (pChan->auto_DTR))
				status = EIO;
			else status = setManualDTR(pChan, arg);
			break;

	case P10_MANUAL_CTS:
			/* check if CTS line */
			if (pChan->auto_CTS)
				status = EIO;
			else status = getManualCTS(pChan);
			break;

	case P10_MANUAL_DSR:
			/* check if DSR line */
			if (pChan->auto_DSR)
				status = EIO;
			else status = getManualDSR(pChan);
			break;

	case P10_MANUAL_DCD:
			/* check if DSR line */
			status = getManualDCD(pChan);
			break;



	case P10_AUTO_RTS:
			if (arg)
			{
				pChan->auto_RTS = TRUE;
				setAutoRTSEnable(pChan, 1);
			}
			else
			{
				pChan->auto_RTS = FALSE;
				setAutoRTSEnable(pChan, 0);
			}
			break;

	case P10_AUTO_CTS:
			if (arg)
			{
				pChan->auto_CTS = TRUE;
				setAutoCTSEnable(pChan, 1);
			}
			else
			{
				pChan->auto_CTS = FALSE;
				setAutoCTSEnable(pChan, 0);
			}
			break;

	case P10_AUTO_DTR:
			if (pChan->auto_RS485)
				status = EIO;
			else
			{
				if (arg)
				{
					pChan->auto_DTR = TRUE;
					setAutoDTREnable(pChan, 1);
				}
				else
				{
					pChan->auto_DTR = FALSE;
					setAutoDTREnable(pChan, 0);
				}
			}
			break;

	case P10_AUTO_DSR:
			if (arg)
			{
				pChan->auto_DSR = TRUE;
				setAutoDSREnable(pChan, 1);
			}
			else
			{
				pChan->auto_DSR = FALSE;
				setAutoDSREnable(pChan, 0);
			}
			break;



	case P10_AUTO_XONXOFF:
			/* set Xon and Xoff characters */
			write650(pChan, XON1_OFFSET, XON_CHAR);
			write650(pChan, XOFF1_OFFSET, XOFF_CHAR);
			write650(pChan, XON2_OFFSET, XON_CHAR);
			write650(pChan, XOFF2_OFFSET, XOFF_CHAR);

			/* enable/disable autoinband */
			setInBandFlowControlMode(pChan, arg);
			if (arg)
				pChan->auto_XONXOFF = TRUE;
			else
				pChan->auto_XONXOFF = FALSE;
			break;

	/* P11 RS485/RS422 settings */
	case P11_RS485_ENABLE:
			/* set MIO of the according UART low/high */
			status = (RS485Enable(pChan, arg) == OK) ? OK : EIO;
	    	break;

	/* enable/disable automatic RS485 control (DTR) */
	case P11_AUTO_RS485:
			/* check if DTR line in use */
			if (pChan->auto_DTR)
				status = EIO;
			else
			{
				if (arg)
				{
					pChan->auto_RS485 = TRUE;
					status = setRS485BufferEnable(pChan, 1, 0);
				}
				else
				{
					pChan->auto_RS485 = FALSE;
					status = setRS485BufferEnable(pChan, 0, 0);
				}
			}
			break;

	default:
		if( pChan->created == P10_CREATE_TYPE_LATE )
	    	status = tyIoctl (&pChan->u.ttyT.tyDev, request, arg);
	    else
		    status = ENOSYS;
	    break;
	}/*switch*/

    return (status);
    }/*p10Ioctl*/



/********************************** p10Int *********************************
 *
 *  Description: Handle a receiver/transmitter interrupt
 *
 *	This routine handles four sources of interrupts from the UART. They are
 *  priotized in the following order by the Interrupt Identification Register:
 *  Receiver Line Status, Received Data Ready/Receiver FIFO timeout,
 *  Transmit Holding Register Empty/Transmit FIFO threshold,
 *  and Modem Status.
 *
 *  If there is another character to be transmitted, it sends it.  If
 *  not, or if a device has never been created for this channel, just
 *  disable the interrupt.
 *  When a modem status interrupt occurs, the transmit interrupt is enabled if
 *  the CTS signal is TRUE.
 *
 *-----------------------------------------------------------------------------
 *  Input......:  pChan        pointer to channel device
 *
 *  Output.....:  Return
 *
 *  Globals....:
 ****************************************************************************/
void p10Int( P10_TY_CO_DEV	*pChan )
{
    char outChar;
	char inChar;
    char interruptID;
    char intID;
    char lineStatus;
    char ier;
	char msr;
	int  triggerLevel = 1;
	char interruptSource;
	int  uart;

	/* read the global interrupt source register */
	interruptSource = RD((pChan->ioAddr2 ) + 0x1C ) & 0x0F;

	/* get uart from interrupt source */
	if (interruptSource & 0x1)
		uart = 0;
	else if (interruptSource & 0x2)
		uart = 1;
	else if (interruptSource & 0x4)
		uart = 2;
	else if (interruptSource & 0x8)
		uart = 3;
	else
	   return;

	/* set channel pointer - it's assumed
	 * that channel 0,1,2,3 memory is continuous
	 */
	pChan += uart;

	interruptID = RD(pChan->ioAddr + ISR_OFFSET);
	intID = interruptID & P10_IIR_MASK;

#ifdef DBG
	if( P10_Logs > 0 )
		logMsg("p10Int [%d/%02x]\n",
		        uart, intID, 0, 0,0,0 );
#endif

    switch (intID) {
    	/* serial error or break */
    	case P10_IIR_SEOB:
    			lineStatus = RD(pChan->ioAddr + LSR_OFFSET);
    			break;

    	/* data received interrupt */
    	case P10_IIR_RBRF:
    	case P10_IIR_TIMEOUT:
    			while (RD(pChan->ioAddr + LSR_OFFSET) & P10_LSR_RXRDY)
    			{
    				inChar = RD(pChan->ioAddr + RHR_OFFSET);
    				if (pChan->created == P10_CREATE_TYPE_LATE)
	       				tyIRd (&pChan->u.ttyT.tyDev, inChar );
    				else
    				{
    				 	/* P10_CREATE_TYPE_AT_BOOT */
		                (*pChan->u.sioT.putRcvChar)
		                	( pChan->u.sioT.putRcvArg, inChar );
                    }/*if*/

	       			inBytes++;
	       		}
	       		intCountIn++;
	       		break;

       	/* transmit interrupt */
	   	case P10_IIR_THRE:
	   			/* OX16C950 FIFO trigger */
	   			if (pChan->fifo_enabled)
	   			{
	   				switch (pChan->transmit_level)
	   				{
	   					case FCR_TX_TRIGGER_16:	triggerLevel = 16; break;
	   					case FCR_TX_TRIGGER_32: triggerLevel = 32; break;
	   					case FCR_TX_TRIGGER_64: triggerLevel = 64; break;
	   					case FCR_TX_TRIGGER_112: triggerLevel = 112; break;
	   				}
	   			}
	   			else triggerLevel = 1;

	   			/* put data in FIFO until trigger level reached (fifo full)*/
				switch( pChan->created )
				{
					case P10_CREATE_TYPE_LATE:
			   			while (triggerLevel--)
	   					{
	   						if( tyITx (&pChan->u.ttyT.tyDev, &outChar) == OK )
	       						WR(pChan->ioAddr + THR_OFFSET, outChar);
	       					else
	       						break;
			       			outBytes++;
	       				}/*while*/
						break;

					case P10_CREATE_TYPE_AT_BOOT:
					default:
						if( pChan->u.sioT.getTxChar == NULL )
							break;
			   			while (triggerLevel--)
	   					{
	   						if( (*pChan->u.sioT.getTxChar)
	   						       (pChan->u.sioT.getTxArg, &outChar) == OK
	   						  )
	       						WR(pChan->ioAddr + THR_OFFSET, outChar);
	       					else
	       						break;
			       			outBytes++;
	       				}/*while*/
						break;
				}/*switch*/

				if( triggerLevel > 0 )
				{
	   				/* disable TX interrupt for reenabling in p10Startup() */
   					WR( pChan->ioAddr + IER_OFFSET,
   					    (P10_IER_MSI | P10_IER_LST | P10_IER_RXRDY) );
   				}/*if*/

	       		intCountOut++;
	       	break;

	   	/* modem status interrupt */
	   	case P10_IIR_MSTAT:
	   			msr = RD(pChan->ioAddr + MSR_OFFSET);
       			ier = (P10_IER_RXRDY | P10_IER_MSI);

				/* if CTS is asserted by modem, enable tx interrupt */
				if ((msr & P10_MSR_CTS) && (msr & P10_MSR_DCTS))
	       			WR(pChan->ioAddr + IER_OFFSET, (P10_IER_TBE | ier));
       			else
	       			WR(pChan->ioAddr + IER_OFFSET, ier);
	       	break;

		default:
	       	break;
	} /*switch*/
}/*p10Int*/

/******************************** p10Startup ***********************************
 *
 *  Description: Transmitter startup routine
 *
 *  Call interrupt level character output routine and enable interrupt if it is
 *  in interrupt mode with no hardware flow control.
 *  If the option for hardware flow control is enabled and CTS is set TRUE,
 *  then the Tx interrupt is enabled.
 *
 *------------------------------------------------------------------------------
 *  Input......:  pChan         pointer to channel device
 *
 *  Output.....:  Return 		always OK
 *
 *  Globals....:  p10Rsrc
 *******************************************************************************/
LOCAL int p10Startup
    (
    P10_TY_CO_DEV 			*pChan		/* tty device to start up */
    )
{
	char  ier = P10_IER_RXRDY;
    char  mask;

    if (pChan->channelMode == SIO_MODE_INT)
	{
	    if (pChan->options & CLOCAL)
		{
			    /* No modem control */
		    	ier |= P10_IER_TBE;
		}
		else
		{
		    mask = (RD(pChan->ioAddr + MSR_OFFSET) & P10_MSR_CTS);

	    	/* if the CTS is asserted enable Tx interrupt */
		    if( mask & P10_MSR_CTS )
				ier |= (P10_IER_TBE | P10_IER_MSI);
		    else
				ier |= (P10_IER_MSI);
		}/*if*/
	    WR(pChan->ioAddr + IER_OFFSET, ier);
	}/*if*/

    return (OK);
}/*p10Startup*/

/******************************************************************************
*
* i8250CallbackInstall - install ISR callbacks to get put chars.
*
* This routine installs the callback functions for the driver
*
* RETURNS: OK on success or ENOSYS on unsupported callback type.
*/
static int callbackInstall
    (
    SIO_CHAN *  pSioChan,
    int         callbackType,
    STATUS      (*callback)(void *, ...),
    void *      callbackArg
    )
    {
    P10_TY_CO_DEV * pChan = (P10_TY_CO_DEV *)pSioChan;

    switch (callbackType)
        {
        case SIO_CALLBACK_GET_TX_CHAR:
            pChan->u.sioT.getTxChar    = (void*)callback;
            pChan->u.sioT.getTxArg     = callbackArg;
            return (OK);
        case SIO_CALLBACK_PUT_RCV_CHAR:
            pChan->u.sioT.putRcvChar   = (void*)callback;
            pChan->u.sioT.putRcvArg    = callbackArg;
            return (OK);
        default:
            return (ENOSYS);
        }
    }


/******************************************************************************
*
* i8250PRxChar - poll the device for input.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer if empty.
*/
LOCAL int p10PollRxChar
    (
    P10_TY_CO_DEV *    pChan,      /* pointer to channel */
    char *          pChar       /* pointer to char */
    )
    {
		logMsg("*** p10PollRxChar() not implemented\n", 0,0,0,0,0,0 );
    	return( EAGAIN );
    }

/******************************************************************************
*
* i8250PTxChar - output a character in polled mode.
*
* Checks if the transmitter is empty. If empty, a character is written to
* the data register.
*
* If the hardware flow control is enabled the handshake signal CTS has to be
* asserted in order to send a character.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the output buffer if full.
*/
LOCAL int p10PollTxChar
    (
    P10_TY_CO_DEV *  pChan,        /* pointer to channel */
    char          outChar       /* char to send */
    )
    {
		logMsg("*** p10PollTxChar() not implemented\n", 0,0,0,0,0,0 );
	    return (EAGAIN);
    }


