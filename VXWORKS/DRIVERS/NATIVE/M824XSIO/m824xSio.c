/* m824xSio.c - NS 16550 UART tty driver */
/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m824xSio.c
 *      Project: VxWorks for 824x boards
 *
 *       Author: kp
 *        $Date: 2006/04/27 10:38:05 $
 *    $Revision: 1.4 $
 *
 *  Description: VxWorks SIO driver for PPC 824x DUARTs
 *				 cloned from WRS ns16550Sio.c 01i
 *
 *     Required:  -
 *     Switches:  M824XSIO_SUPPORT_SAM - include SAM FPGA unit code
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m824xSio.c,v $
 * Revision 1.4  2006/04/27 10:38:05  cs
 * cosmetics for APIGEN and VxWorks 6.x (function headers and definitions)
 *
 * Revision 1.3  2005/07/05 12:28:53  kp
 * cosmetic
 *
 * Revision 1.2  2004/09/01 12:17:08  ufranke
 * cosmetics
 *
 * Revision 1.1  2003/03/14 15:16:24  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
/* Copyright 1984-1997 Wind River Systems, Inc. */

#include "copyright_wrs.h"

/*
DESCRIPTION

This is the driver for the PowerPC 8245 DUART. This device
includes two universal asynchronous receiver/transmitters, a baud rate
generator, and a complete modem control capability.

This driver is almost identical to the WRS ns16550Sio driver.

Currently, it provides one additional feature: If switch M824XSIO_SUPPORT_SAM
is set, it uses the SAM FPGA helper unit to control physical line setings
and additional handshake lines. To do this the BSP sysSerialHwInit() routine
must setup the samRegs field of the channel structure.

If Sam is used, The default interface mode is RS422, full duplex. Use
ioctl( M824XSIO_PHYS_INT_SET ) to change this. In addition, in half duplex
mode, one can use ioctl( M824XSIO_ECHO_SUPPRESS ) to suppress receiption
of data sent by that UART.

In Sam mode, CTS cannot be used for flow control right now.

A M824XSIO_CHAN structure is used to describe the serial channel. This data
structure is defined in m824xsioSio.h.

Only asynchronous serial operation is supported by this driver.
The default serial settings are 8 data bits, 1 stop bit, no parity, 9600
baud, and software flow control.

USAGE The BSP's sysHwInit() routine typically calls sysSerialHwInit(),
which creates the M824XSIO_CHAN structure and initializes all the
values in the structure (except the SIO_DRV_FUNCS) before calling
m824xsioDevInit().  The BSP's sysHwInit2() routine typically calls
sysSerialHwInit2(), which connects the chips interrupts via
intConnect() (either the single interrupt `m824xsioInt' or the three
interrupts `m824xsioIntWr', `m824xsioIntRd', and `m824xsioIntEx').

This driver handles setting of hardware options such as parity(odd, even) and
number of data bits(5, 6, 7, 8). Hardware flow control is provided with the
handshakes RTS/CTS. The function HUPCL(hang up on last close) is available.
When hardware flow control is enabled, the signals RTS and DTR are set TRUE
and remain set until a HUPCL is performed.

INCLUDE FILES: MEN/m824xsio.h

*/

/* includes */

#include "vxWorks.h"
#include "intLib.h"
#include "errnoLib.h"
#include "errno.h"
#include "sioLib.h"
#include "MEN/m824xsio.h"

/* local defines       */

#ifndef SIO_HUP
#   define SIO_OPEN	0x100A
#   define SIO_HUP	0x100B
#endif

#define SWAPLONG(l) ((((l)&0xff)<<24) + (((l)&0xff00)<<8) + \
                      (((l)&0xff0000)>>8) + (((l)&0xff000000)>>24))

/* min/max baud rate */

#define M824XSIO_MIN_RATE 50
#define M824XSIO_MAX_RATE 115200

#define REG(reg, pchan) \
 (*(volatile UINT8 *)((UINT32)pchan->regs + (reg * pchan->regDelta)))
#define REGPTR(reg, pchan) \
 ((volatile UINT8 *)((UINT32)pchan->regs + (reg * pchan->regDelta)))
#define SAMREG(reg, pchan) \
 (*(volatile UINT8 *)((UINT32)pchan->samRegs + (reg)))

/* static forward declarations */

LOCAL 	int 	m824xSioCallbackInstall (SIO_CHAN *, int, STATUS (*)(), void *);
LOCAL 	STATUS 	m824xSioDummyCallback ();
LOCAL 	void 	m824xSioInitChannel (M824XSIO_CHAN *);
LOCAL   STATUS  m824xSioBaudSet (M824XSIO_CHAN *, UINT);
LOCAL 	STATUS  m824xSioModeSet (M824XSIO_CHAN *, UINT);
LOCAL 	STATUS 	m824xSioIoctl (M824XSIO_CHAN *, int, int);
LOCAL 	void 	m824xSioTxStartup (M824XSIO_CHAN *);
LOCAL 	int 	m824xSioPollOutput (M824XSIO_CHAN *, char);
LOCAL 	int 	m824xSioPollInput (M824XSIO_CHAN *, char *);
LOCAL 	STATUS 	m824xSioOptsSet (M824XSIO_CHAN *, UINT);
LOCAL 	STATUS 	m824xSioOpen (M824XSIO_CHAN * pChan );
LOCAL 	STATUS 	m824xSioHup (M824XSIO_CHAN * pChan );
LOCAL int m824xSioSamSetPhysMode( M824XSIO_CHAN *pChan, int mode );
LOCAL int m824xSioSamSetEchoSuppress( M824XSIO_CHAN *pChan, int echoSuppress );
LOCAL int m824xSioSamSetRTS( M824XSIO_CHAN *pChan, int rtsLevel );
/*LOCAL int m824xSioSamGetCTS( M824XSIO_CHAN *pChan );*/
LOCAL int m824xSioSamSetSpeed( M824XSIO_CHAN *pChan );



/* driver functions */

static SIO_DRV_FUNCS m824xSioSioDrvFuncs =
{
    (int (*)())m824xSioIoctl,
    (int (*)())m824xSioTxStartup,
    (int (*)())m824xSioCallbackInstall,
    (int (*)())m824xSioPollInput,
    (int (*)(SIO_CHAN *,char))m824xSioPollOutput
};

/******************************************************************************
*
* m824xSioDummyCallback - dummy callback routine.
*/

LOCAL STATUS m824xSioDummyCallback (void)
{
    return (ERROR);
}

/******************************************************************************
*
* m824xSioDevInit - intialize an M824XSIO channel
*
* This routine initializes some SIO_CHAN function pointers and then resets
* the chip in a quiescent state.  Before this routine is called, the BSP
* must already have initialized all the device addresses, etc. in the
* M824XSIO_CHAN structure.
*
* RETURNS: N/A
*/

void m824xSioDevInit(
    M824XSIO_CHAN * pChan	/* pointer to channel */
    )
{
    int oldlevel = intLock ();

    /* initialize the driver function pointers in the SIO_CHAN's */

    pChan->pDrvFuncs    = &m824xSioSioDrvFuncs;

    /* set the non BSP-specific constants */

    pChan->getTxChar    = m824xSioDummyCallback;
    pChan->putRcvChar   = m824xSioDummyCallback;
    pChan->channelMode  = 0;    /* undefined */
    pChan->options      = (CLOCAL | CREAD | CS8);
    pChan->mcr		= MCR_OUT2;

    /* reset the chip */

    m824xSioInitChannel (pChan);

    intUnlock (oldlevel);
}

/******************************************************************************
*
* m824xSioInitChannel - initialize UART
*
* Initialize the number of data bits, parity and set the selected
* baud rate.
* Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/

LOCAL void m824xSioInitChannel(
    M824XSIO_CHAN * pChan	/* pointer to channel */
    )
{

    /* set the requested baud rate */

    m824xSioBaudSet(pChan, pChan->baudRate);

    /* set the options */

    m824xSioOptsSet(pChan, pChan->options);

	/* set default SAM mode */
	if( pChan->samRegs )
		SAMREG( 0, pChan ) &= 0x4;	/* set reset value */
	m824xSioSamSetEchoSuppress( pChan, 0 );
	m824xSioSamSetPhysMode( pChan, M824XSIO_DF_FDX );

}

/******************************************************************************
*
* m824xSioOptsSet - set the serial options
*
* Set the channel operating mode to that specified.  All sioLib options
* are supported: CLOCAL, HUPCL, CREAD, CSIZE, PARENB, and PARODD.
* When the HUPCL option is enabled, a connection is closed on the last
* close() call and opened on each open() call.
*
* Note, this routine disables the transmitter.  The calling routine
* may have to re-enable it.
*
* RETURNS:
* Returns OK to indicate success, otherwise ERROR is returned
*/

LOCAL STATUS m824xSioOptsSet(
    M824XSIO_CHAN * pChan,	/* pointer to channel */
    UINT options		/* new hardware options */
    )
{
    FAST int     oldlevel;		/* current interrupt level mask */

    pChan->lcr = 0;
    pChan->mcr &= (~(MCR_RTS | MCR_DTR)); /* clear RTS and DTR bits */

    if (pChan == NULL || options & 0xffffff00)
		return ERROR;

    switch (options & CSIZE)
	{
	case CS5:
	    pChan->lcr = CHAR_LEN_5; break;
	case CS6:
	    pChan->lcr = CHAR_LEN_6; break;
	case CS7:
	    pChan->lcr = CHAR_LEN_7; break;
	default:
	case CS8:
	    pChan->lcr = CHAR_LEN_8; break;
	}

    if (options & STOPB)
		pChan->lcr |= LCR_STB;
    else
		pChan->lcr |= ONE_STOP;

    switch (options & (PARENB | PARODD))
	{
	case PARENB|PARODD:
	    pChan->lcr |= LCR_PEN; break;
	case PARENB:
	    pChan->lcr |= (LCR_PEN | LCR_EPS); break;
	default:
	case 0:
	    pChan->lcr |= PARITY_NONE; break;
	}

    REG(IER, pChan) = 0;

    if (!(options & CLOCAL))
	{
		/* !clocal enables hardware flow control(DTR/DSR) */

		pChan->mcr |= (MCR_DTR | MCR_RTS);
    	pChan->ier &= (~TxFIFO_BIT);
		pChan->ier |= IER_EMSI;    /* enable modem status interrupt */
	}
    else
        pChan->ier &= ~IER_EMSI; /* disable modem status interrupt */

    oldlevel = intLock ();

    REG(LCR, pChan) = pChan->lcr;
    REG(MCR, pChan) = pChan->mcr;
	m824xSioSamSetRTS( pChan, pChan->mcr & MCR_RTS );

    /* now reset the channel mode registers */

    REG(FCR, pChan) = (RxCLEAR | TxCLEAR | FIFO_ENABLE);

    if (options & CREAD)
		pChan->ier |= RxFIFO_BIT;

    if (pChan->channelMode == SIO_MODE_INT)
	{
        REG(IER, pChan) = pChan->ier;
	}

    intUnlock (oldlevel);

    pChan->options = options;

	m824xSioSamSetSpeed( pChan );

    return OK;
}

/******************************************************************************
 *
 * m824xSioHup - hang up the modem control lines
 *
 * Resets the RTS and DTR signals and clears both the receiver and
 * transmitter sections.
 *
 * RETURNS: OK
 */

LOCAL STATUS m824xSioHup(
    M824XSIO_CHAN * pChan 	/* pointer to channel */
    )
{
    FAST int     oldlevel;	/* current interrupt level mask */

    oldlevel = intLock ();

    pChan->mcr &= (~(MCR_RTS | MCR_DTR));
    REG(MCR, pChan) = pChan->mcr;
	m824xSioSamSetRTS( pChan, pChan->mcr & MCR_RTS );

    REG(FCR, pChan) = (RxCLEAR | TxCLEAR);

    intUnlock (oldlevel);

    return (OK);

}

/******************************************************************************
 *
 * m824xSioOpen - Set the modem control lines
 *
 * Set the modem control lines(RTS, DTR) TRUE if not already set.
 * It also clears the receiver, transmitter and enables the fifo.
 *
 * RETURNS: OK
 */

LOCAL STATUS m824xSioOpen(
    M824XSIO_CHAN * pChan 	/* pointer to channel */
    )
{
    FAST int     oldlevel;	/* current interrupt level mask */
    char mask;

    mask = REG(MCR, pChan) & (MCR_RTS | MCR_DTR);

    if (mask != (MCR_RTS | MCR_DTR))
	{
    	/* RTS and DTR not set yet */

    	oldlevel = intLock ();

		/* set RTS and DTR TRUE */

    	pChan->mcr |= (MCR_DTR | MCR_RTS);
    	REG(MCR, pChan) = pChan->mcr;
		m824xSioSamSetRTS( pChan, pChan->mcr & MCR_RTS );

    	/* clear Tx and receive and enable FIFO */

        REG(FCR, pChan) = (RxCLEAR | TxCLEAR | FIFO_ENABLE);

    	intUnlock (oldlevel);
	}

    return (OK);
}

/******************************************************************************
 *
 * m824xSioBaudSet - change baud rate for channel
 *
 * This routine sets the baud rate for the UART. The interrupts are disabled
 * during chip access.
 *
 * RETURNS: OK
 */

LOCAL STATUS  m824xSioBaudSet(
    M824XSIO_CHAN * pChan,	/* pointer to channel */
    UINT	   baud		/* requested baud rate */
    )
{
    int   oldlevel;
    /* disable interrupts during chip access */

    oldlevel = intLock ();

    /* Enable access to the divisor latches by setting DLAB in LCR. */

    REG(LCR, pChan) = LCR_DLAB | pChan->lcr;

    /* Set divisor latches. */

	pChan->divLatch = pChan->xtal/(16 * baud);
    REG(DLL, pChan) = pChan->divLatch;
    REG(DLM, pChan) = pChan->divLatch >> 8;

	m824xSioSamSetSpeed( pChan );

    /* Restore line control register */

    REG(LCR, pChan) = pChan->lcr;

    pChan->baudRate = baud;

    intUnlock (oldlevel);

    return (OK);
}

/******************************************************************************
 *
 * m824xSioModeSet - change channel mode setting
 *
 * This driver supports both polled and interrupt modes and is capable of
 * switching between modes dynamically.
 *
 * If interrupt mode is desired this routine enables the channels receiver and
 * transmitter interrupts. If the modem control option is TRUE, the Tx interrupt
 * is disabled if the CTS signal is FALSE. It is enabled otherwise.
 *
 * If polled mode is desired the device interrupts are disabled.
 *
 * RETURNS:
 * Returns a status of OK if the mode was set else ERROR.
 */

LOCAL STATUS m824xSioModeSet(
    M824XSIO_CHAN * pChan,	/* pointer to channel */
    UINT	newMode		/* mode requested */
    )
{
    FAST int     oldlevel;	/* current interrupt level mask */
    char mask;

    if ((newMode != SIO_MODE_POLL) && (newMode != SIO_MODE_INT))
		return (ERROR);

    oldlevel = intLock ();

    if (newMode == SIO_MODE_INT)
	{
        /* Enable appropriate interrupts */

		if (pChan->options & CLOCAL)
			REG(IER, pChan) = pChan->ier | RxFIFO_BIT | TxFIFO_BIT;
		else
		{
			mask = REG(MSR, pChan) & MSR_CTS;

			/* if the CTS is asserted enable Tx interrupt */

			if (mask & MSR_CTS)
				pChan->ier |= TxFIFO_BIT;    /* enable Tx interrupt */
			else
           		pChan->ier &= (~TxFIFO_BIT); /* disable Tx interrupt */

			REG(IER, pChan) = pChan->ier;
		}
	}
    else
	{
        /* disable all m824xSio interrupts */

        REG(IER, pChan) = 0;
	}

    pChan->channelMode = newMode;

    intUnlock (oldlevel);

    return (OK);
}

/******************************************************************************
 *
 * m824xSioIoctl - special device control
 *
 * Includes commands to get/set baud rate, mode(INT,POLL), hardware options(
 * parity, number of data bits), and modem control(RTS/CTS and DTR/DSR).
 * The ioctl command SIO_HUP is sent by ttyDrv when the last close() function
 * call is made. Likewise SIO_OPEN is sent when the first open() function call
 * is made.
 *
 * RETURNS: OK on success, EIO on device error, ENOSYS on unsupported
 *          request.
 */

LOCAL STATUS m824xSioIoctl(
    M824XSIO_CHAN * 	pChan,		/* pointer to channel */
    int			request,	/* request code */
    int        		arg		/* some argument */
    )
{
    FAST STATUS  status;

    status = OK;

    switch (request)
	{
	case SIO_BAUD_SET:
	    if (arg < M824XSIO_MIN_RATE || arg > M824XSIO_MAX_RATE)
			status = EIO;		/* baud rate out of range */
	    else
	        status = m824xSioBaudSet (pChan, arg);
	    break;

	case SIO_BAUD_GET:
		*(int *)arg = pChan->baudRate;
		break;

	case SIO_MODE_SET:
	    status = (m824xSioModeSet (pChan, arg) == OK) ? OK : EIO;
		break;

	case SIO_MODE_GET:
		*(int *)arg = pChan->channelMode;
		break;

	case SIO_AVAIL_MODES_GET:
		*(int *)arg = SIO_MODE_INT | SIO_MODE_POLL;
		break;

	case SIO_HW_OPTS_SET:
		status = (m824xSioOptsSet (pChan, arg) == OK) ? OK : EIO;
		break;

	case SIO_HW_OPTS_GET:
		*(int *)arg = pChan->options;
		break;

	case SIO_HUP:
		/* check if hupcl option is enabled */

		if (pChan->options & HUPCL)
	    	status = m824xSioHup (pChan);
		break;

	case SIO_OPEN:
		/* check if hupcl option is enabled */

		if (pChan->options & HUPCL)
	    	status = m824xSioOpen (pChan);
	    break;

	case M824XSIO_PHYS_INT_SET:
		status = m824xSioSamSetPhysMode( pChan, arg );
		break;

	case M824XSIO_ECHO_SUPPRESS:
		status = m824xSioSamSetEchoSuppress( pChan, arg );
		break;

	default:
		status = ENOSYS;
	}
    return (status);
}

/******************************************************************************
*
* m824xSioIntWr - handle a transmitter interrupt
*
* This routine handles write interrupts from the UART. It reads a character
* and puts it in the transmit holding register of the device for transfer.
*
* If there are no more characters to transmit, transmission is disabled by
* clearing the transmit interrupt enable bit in the IER(int enable register).
*
* RETURNS: N/A
*
*/

void m824xSioIntWr(
    M824XSIO_CHAN * pChan		/* pointer to channel */
    )
{
    char           outChar;

    if ((*pChan->getTxChar) (pChan->getTxArg, &outChar) != ERROR)
        REG(THR,pChan) = outChar;	/* write char to Transmit Holding Reg */
    else
	{
        pChan->ier &= (~TxFIFO_BIT);	/* indicates to disable Tx Int */
        REG(IER, pChan) = pChan->ier;
	}
}

/******************************************************************************
*
* m824xSioIntRd - handle a receiver interrupt
*
* This routine handles read interrupts from the UART.
*
* RETURNS: N/A
*
*/

void m824xSioIntRd
(
    M824XSIO_CHAN * pChan	/* pointer to channel */
    )
{
    char   inchar;

    /* read character from Receive Holding Reg. */

    inchar = REG(RBR, pChan);

    (*pChan->putRcvChar) (pChan->putRcvArg, inchar);
}

/******************************************************************************
*
* m824xSioIntEx - miscellaneous interrupt processing
*
* This routine handles miscellaneous interrupts on the UART.
* Not implemented yet.
*
* RETURNS: N/A
*
*/

void m824xSioIntEx (
    M824XSIO_CHAN *pChan		/* pointer to channel */
    )
{

    /* Nothing for now... */
}

/******************************************************************************
*
* m824xSioInt - interrupt level processing
*
* This routine handles four sources of interrupts from the UART. They are
* prioritized in the following order by the Interrupt Identification Register:
* Receiver Line Status, Received Data Ready, Transmit Holding Register Empty
* and Modem Status.
*
* When a modem status interrupt occurs, the transmit interrupt is enabled if
* the CTS signal is TRUE.
*
* RETURNS: N/A
*
*/

void m824xSioInt (
    M824XSIO_CHAN * pChan	/* pointer to channel */
    )
{
    FAST volatile char        intStatus;

    /* read the Interrrupt Status Register (Int. Ident.) */

    intStatus = (REG(IIR, pChan)) & 0x0f;

    /*
     * This UART chip always produces level active interrupts, and the IIR
     * only indicates the highest priority interrupt.
     * In the case that receive and transmit interrupts happened at
     * the same time, we must clear both interrupt pending to prevent
     * edge-triggered interrupt(output from interrupt controller) from locking
     * up. One way doing it is to disable all the interrupts at the beginning
     * of the ISR and enable at the end.
     */

    REG(IER,pChan) = 0;    /* disable interrupt */

    switch (intStatus)
	{
	case IIR_RLS:
		/* overrun,parity error and break interrupt */

		intStatus = REG(LSR, pChan); /* read LSR to reset interrupt */
	    break;

	case IIR_RDA:     		/* received data available */
	case IIR_TIMEOUT:
		/*
		 * receiver FIFO interrupt. In some case, IIR_RDA will
		 * not be indicated in IIR register when there is more
		 * than one character in FIFO.
		 */

		m824xSioIntRd (pChan);  	/* RxChar Avail */
		break;

	case IIR_THRE:  /* transmitter holding register ready */
	{
		char outChar;

		if ((*pChan->getTxChar) (pChan->getTxArg, &outChar) != ERROR)
			REG(THR, pChan) = outChar;   /* char to Transmit Holding Reg */
		else
			pChan->ier &= (~TxFIFO_BIT); /* indicates to disable Tx Int */

	}
	break;

	case IIR_MSTAT: /* modem status register */
	{
		char	msr;

		msr = REG(MSR, pChan);

		/* if CTS is asserted by modem, enable tx interrupt */

		if ((msr & MSR_CTS) && (msr & MSR_DCTS))
			pChan->ier |= TxFIFO_BIT;
		else
           	pChan->ier &= (~TxFIFO_BIT);
	}
	break;

	default:
	    break;
	}

    REG(IER, pChan) = pChan->ier; /* enable interrupts accordingly */

}

/******************************************************************************
*
* m824xSioTxStartup - transmitter startup routine
*
* Call interrupt level character output routine and enable interrupt if it is
* in interrupt mode with no hardware flow control.
* If the option for hardware flow control is enabled and CTS is set TRUE,
* then the Tx interrupt is enabled.
*
* RETURNS: N/A
*/

LOCAL void m824xSioTxStartup(
    M824XSIO_CHAN * pChan 	/* pointer to channel */
    )
{
    char mask;

    if (pChan->channelMode == SIO_MODE_INT)
	{
		if (pChan->options & CLOCAL)
		{
			/* No modem control */

			pChan->ier |= TxFIFO_BIT;
			REG(IER,pChan) = pChan->ier;
		}
		else
		{
			mask = REG(MSR, pChan) & MSR_CTS;

			/* if the CTS is asserted enable Tx interrupt */

			if (mask & MSR_CTS)
				pChan->ier |= TxFIFO_BIT;    /* enable Tx interrupt */
			else
           		pChan->ier &= (~TxFIFO_BIT); /* disable Tx interrupt */

			REG(IER, pChan) = pChan->ier;
		}
	}
}

/******************************************************************************
*
* m824xSioPollOutput - output a character in polled mode.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the output buffer if full.
*/

LOCAL int m824xSioPollOutput(
    M824XSIO_CHAN *  pChan,	/* pointer to channel */
    char            outChar	/* char to send */
    )
{
    char pollStatus = REG(LSR, pChan);
    char msr = REG(MSR, pChan);

    /* is the transmitter ready to accept a character? */

    if ((pollStatus & LSR_THRE) == 0x00)
        return (EAGAIN);

    if (!(pChan->options & CLOCAL))	 /* modem flow control ? */
	{
    	if (msr & MSR_CTS)
    		REG(THR, pChan) = outChar;
		else
			return (EAGAIN);
	}
    else
    	REG(THR, pChan) = outChar;       /* transmit character */

    return (OK);
}
/******************************************************************************
*
* m824xSioPollInput - poll the device for input.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer if empty.
*/

LOCAL int m824xSioPollInput(
    M824XSIO_CHAN *  pChan,	/* pointer to channel */
    char *          pChar 	/* pointer to char */
    )
{

    char pollStatus = REG(LSR, pChan);

    if ((pollStatus & LSR_DR) == 0x00)
        return (EAGAIN);

    /* got a character */

    *pChar = REG(RBR, pChan);

    return (OK);
}

/******************************************************************************
*
* m824xSioCallbackInstall - install ISR callbacks to get/put chars.
*
* This routine installs the callback functions for the driver
*
* RETURNS: OK on success or ENOSYS on unsupported callback type.
*/

LOCAL int m824xSioCallbackInstall(
    SIO_CHAN *  pSioChan,	/* pointer to device to control */
    int         callbackType,	/* callback type(tx or receive) */
    STATUS      (*callback)(),	/* pointer to callback function */
    void *      callbackArg	/* callback function argument */
    )
{
    M824XSIO_CHAN * pChan = (M824XSIO_CHAN *)pSioChan;

    switch (callbackType)
	{
	case SIO_CALLBACK_GET_TX_CHAR:
		pChan->getTxChar    = callback;
		pChan->getTxArg     = callbackArg;
		return (OK);
	case SIO_CALLBACK_PUT_RCV_CHAR:
		pChan->putRcvChar   = callback;
		pChan->putRcvArg    = callbackArg;
		return (OK);
	default:
		return (ENOSYS);
	}

}

#ifdef M824XSIO_SUPPORT_SAM
/******************************************************************************
*
* m824xSioSamSetPhysMode - set physical interface mode
*
* RETURNS: OK on success or ERROR if mode not supported.
*/
LOCAL int m824xSioSamSetPhysMode(
	M824XSIO_CHAN *pChan,
	int mode )
{
	UINT8 modeMask;

	if( pChan->samRegs == NULL )
		return ERROR;

	switch( mode ){
	case M824XSIO_DF_HDX: 	modeMask = 0xc; break;
	case M824XSIO_DF_FDX: 	modeMask = 0x4; break;
	case M824XSIO_SE: 		modeMask = 0x0; break;
	default: return ERROR;
	}

	SAMREG( 0, pChan ) &= ~0x1c;
	SAMREG( 0, pChan ) |= modeMask;
	return OK;
}

/******************************************************************************
*
* m824xSioSamSetEchoSuppress - set/clear echo suppression
*
* RETURNS: OK or ERROR if not initialized
*/
LOCAL int m824xSioSamSetEchoSuppress(
	M824XSIO_CHAN *pChan,
	int echoSuppress )
{
	if( pChan->samRegs == NULL )
		return ERROR;

	if( echoSuppress )
		SAMREG( 0, pChan ) |= 0x10;
	else
		SAMREG( 0, pChan ) &= ~0x10;

	return OK;
}

/******************************************************************************
*
* m824xSioSamSetRTS - set/clear RTS
* rtsLevel 1=RTS to +12V (active)
*
* RETURNS: OK or ERROR if not initialized
*
*/
LOCAL int m824xSioSamSetRTS(
	M824XSIO_CHAN *pChan,
	int rtsLevel )
{
	if( pChan->samRegs == NULL )
		return ERROR;

	if( rtsLevel )
		SAMREG( 0, pChan ) &= ~0x1;
	else
		SAMREG( 0, pChan ) |= ~0x1;

	return OK;
}

#if 0							/* not yet used */
/******************************************************************************
*
* m824xSioSamGetCTS - get state of CTS
*
* RETURNS: value or 0 if not initialized
*/
LOCAL int m824xSioSamGetCTS( M824XSIO_CHAN *pChan )
{
	if( pChan->samRegs == NULL )
		return 0;

	return !(SAMREG( 0, pChan ) & 0x2);
}
#endif /* 0 */

/******************************************************************************
*
* m824xSioSamSetSpeed - set SAM word length register
*
* RETURNS: OK or ERROR if not initialized
*/
LOCAL int m824xSioSamSetSpeed( M824XSIO_CHAN *pChan )
{
	u_int32 wlv;
	int bitsPerChar=2;			/* start/stop bit */

	if( pChan->samRegs == NULL )
		return ERROR;

	/* compute the total word size in bits (bits in .5 bit units...) */
    switch (pChan->options & CSIZE)
	{
	case CS5: bitsPerChar += 5; break;
	case CS6: bitsPerChar += 6; break;
	case CS7: bitsPerChar += 7; break;
	default:  bitsPerChar += 8; break;
	}
    if (pChan->options & STOPB)
		bitsPerChar++;

    if(pChan->options & PARENB )
		bitsPerChar++;

	bitsPerChar *= 2;

	/* compute the RTS generator monoflop length */
	wlv = (pChan->divLatch * bitsPerChar * 8) - 1;

	/* init the RTS generator */
	*(u_int32*)(pChan->samRegs+0x4) = SWAPLONG(wlv);

	/*printf("SAM wlv=%ld\n", wlv );*/
	return OK;
}

#else
/* not M824XSIO_SUPPORT_SAM */
LOCAL int m824xSioSamSetPhysMode(
	M824XSIO_CHAN *pChan,
	int mode )
{ return ERROR; }
LOCAL int m824xSioSamSetEchoSuppress(
	M824XSIO_CHAN *pChan,
	int echoSuppress )
{ return ERROR; }
LOCAL int m824xSioSamSetRTS(
	M824XSIO_CHAN *pChan,
	int rtsLevel )
{ return ERROR; }
/*LOCAL int m824xSioSamGetCTS( M824XSIO_CHAN *pChan )
{ return 0; } not yet used*/
LOCAL int m824xSioSamSetSpeed( M824XSIO_CHAN *pChan )
{ return ERROR; }
#endif /* M824XSIO_SUPPORT_SAM */
