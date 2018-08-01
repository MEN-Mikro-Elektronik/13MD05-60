/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: m824xsio.h
 *
 *       Author: kp
 *        $Date: 2003/03/14 15:16:20 $
 *    $Revision: 2.1 $
 * 
 *  Description: Header file for m824xsio (PPC 824x DUART) SIO driver
 *               cloned from WRS ns16552.h       
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m824xsio.h,v $
 * Revision 2.1  2003/03/14 15:16:20  kp
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#ifndef _M824XSIO_H
#define _M824XSIO_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _ASMLANGUAGE

#ifndef __INCns16552Sioh

/* Register offsets from base address */

#define RBR		0x00	/* receiver buffer register */
#define THR		0x00	/* transmit holding register */
#define DLL		0x00	/* divisor latch */
#define IER		0x01	/* interrupt enable register */
#define DLM		0x01	/* divisor latch(MS) */
#define IIR		0x02	/* interrupt identification register */
#define FCR		0x02	/* FIFO control register */
#define LCR		0x03	/* line control register */
#define MCR		0x04 	/* modem control register */
#define LSR		0x05	/* line status register */
#define MSR		0x06	/* modem status register */
#define SCR		0x07	/* scratch register */

#define BAUD_LO(baud)  ((XTAL/(16*baud)) & 0xff)
#define BAUD_HI(baud)  (((XTAL/(16*baud)) & 0xff00) >> 8)

/* Line Control Register */

#define CHAR_LEN_5	0x00	/* 5bits data size */
#define CHAR_LEN_6	0x01	/* 6bits data size */
#define CHAR_LEN_7	0x02	/* 7bits data size */
#define CHAR_LEN_8	0x03	/* 8bits data size */
#define LCR_STB		0x04	/* 2 stop bits */
#define ONE_STOP	0x00	/* one stop bit */
#define LCR_PEN		0x08	/* parity enable */
#define PARITY_NONE	0x00	/* parity disable */
#define LCR_EPS		0x10	/* even parity select */
#define LCR_SP		0x20	/* stick parity select */
#define LCR_SBRK	0x40	/* break control bit */
#define LCR_DLAB	0x80	/* divisor latch access enable */
#define DLAB		LCR_DLAB

/* Line Status Register */

#define LSR_DR		0x01	/* data ready */
#define RxCHAR_AVAIL	LSR_DR	/* data ready */
#define LSR_OE		0x02	/* overrun error */
#define LSR_PE		0x04	/* parity error */
#define LSR_FE		0x08	/* framing error */
#define LSR_BI		0x10	/* break interrupt */
#define LSR_THRE	0x20	/* transmit holding register empty */
#define LSR_TEMT	0x40	/* transmitter empty */
#define LSR_FERR	0x80	/* in fifo mode, set when PE,FE or BI error */

/* Interrupt Identification Register */

#define IIR_IP		0x01
#define IIR_ID		0x0e
#define IIR_RLS		0x06	/* received line status */
#define Rx_INT		IIR_RLS /* received line status */
#define IIR_RDA		0x04	/* received data available */
#define RxFIFO_INT	IIR_RDA /* received data available */
#define IIR_THRE	0x02	/* transmit holding register empty */
#define TxFIFO_INT	IIR_THRE 
#define IIR_MSTAT	0x00	/* modem status */
#define IIR_TIMEOUT	0x0c	/* char receiv tiemout */

/* Interrupt Enable Register */

#define IER_ERDAI	0x01	/* received data avail. & timeout int */
#define RxFIFO_BIT	IER_ERDAI
#define IER_ETHREI	0x02	/* transmitter holding register empty int */
#define TxFIFO_BIT	IER_ETHREI
#define IER_ELSI	0x04	/* receiver line status int enable */
#define Rx_BIT		IER_ELSI
#define IER_EMSI	0x08	/* modem status int enable */

/* Modem Control Register */

#define MCR_DTR		0x01	/* dtr output */
#define DTR		MCR_DTR
#define MCR_RTS		0x02	/* rts output */
#define MCR_OUT1	0x04	/* output #1 */
#define MCR_OUT2	0x08	/* output #2 */
#define MCR_LOOP	0x10	/* loopback enable */

/* Modem Status Register */

#define MSR_DCTS	0x01	/* cts change */
#define MSR_DDSR	0x02	/* dsr change */
#define MSR_TERI	0x04	/* ring indicator change */
#define MSR_DDCD	0x08	/* data carrier indicator change */
#define MSR_CTS		0x10	/* complement of cts */
#define MSR_DSR		0x20	/* complement of dsr */
#define MSR_RI		0x40	/* complement of ring signal */
#define MSR_DCD		0x80	/* complement of dcd */ 

/* FIFO Control Register */

#define FCR_EN		0x01	/* enable xmit and rcvr */
#define FIFO_ENABLE	FCR_EN
#define FCR_RXCLR	0x02	/* clears rcvr fifo */
#define RxCLEAR		FCR_RXCLR
#define FCR_TXCLR	0x04	/* clears xmit fifo */
#define TxCLEAR		FCR_TXCLR
#define FCR_DMA		0x08	/* dma */
#define FCR_RXTRIG_L	0x40	/* rcvr fifo trigger lvl low */
#define FCR_RXTRIG_H	0x80	/* rcvr fifo trigger lvl high */

#endif /* __INCns16552Sioh */


/* ioctl function codes (in accordance to M77 driver) */
#define M824XSIO_ECHO_SUPPRESS		0x105
#define M824XSIO_PHYS_INT_SET		0x106

/* ioctl arguments for M824X_PHYS_INT_SET */
#define M824XSIO_DF_HDX         0x01 	/* differential mode, half duplex */
#define M824XSIO_DF_FDX         0x02   	/* differential mode, full duplex */
#define M824XSIO_SE	            0x07    /* single ended (RS232) */


typedef  struct 	/* M824XSIO_CHAN * */
{
    /* always goes first */

    SIO_DRV_FUNCS *     pDrvFuncs;      /* driver functions */

    /* callbacks */

    STATUS      (*getTxChar) (); /* pointer to xmitr function */	
    STATUS      (*putRcvChar) (); /* pointer to rcvr function */
    void *      getTxArg;
    void *      putRcvArg;

    UINT8 	*regs;		/* NS16552 registers */
    UINT8 	level;		/* 8259a interrupt level for this device */
    UINT8 	ier;		/* copy of ier */
    UINT8 	lcr;		/* copy of lcr, not used by ns16552 driver */
    UINT8 	mcr;		/* copy of modem control register */
    UINT8 	pad1;

    UINT16      channelMode;	/* such as INT, POLL modes */
    UINT16      regDelta;	/* register address spacing */
    int         baudRate;
    UINT32      xtal;		/* UART clock frequency     */     
    UINT32	options;		/* hardware setup options */

	/* vars added to NS16550_CHAN: */
	UINT8	*samRegs;		/* FPGA SAM helper unit base (NULL if none) */
	UINT16	divLatch;		/* UART divisor latch setting */
} M824XSIO_CHAN;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void m824xSioInt (M824XSIO_CHAN *);
extern void m824xSioDevInit (M824XSIO_CHAN *);

#else

extern void m824xSioInt ();
extern void m824xSioDevInit ();

#endif  /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
 
#endif /* _M824XSIO_H */
