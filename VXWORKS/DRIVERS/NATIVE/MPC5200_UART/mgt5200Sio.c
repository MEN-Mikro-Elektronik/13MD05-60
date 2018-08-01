/* mgt5200Sio.c - MGT5200 PSC UART tty driver */

/****************************************************************************
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mgt5200Sio.c,v $
 * Revision 1.11  2007/09/11 21:57:47  cs
 * removed
 *   - reference to include file "arc/ppc/ppc5200.h" (not available for VxW5.x)
 *     put necessary defines to "MEN/mgt5200.h"
 *
 * Revision 1.10  2007/08/10 16:01:32  rlange
 * Reworked Interrupt Handling (refering to Windriver mpc52xx sio driver)
 * because of problems at large outputs + open files on ata at the same time
 *
 * Revision 1.9  2007/07/09 15:14:17  RLange
 * Bugfix for Hardware Handshake
 *
 * Revision 1.8  2007/04/18 15:29:50  RLange
 * added handling for hw options for parity/data Bits/stop bits
 *
 * Revision 1.7  2006/02/28 12:07:17  UFRANKE
 * cosmetics
 *
 * Revision 1.6  2006/02/14 21:21:50  cs
 * commented out
 *     - mgt5200SioIntEx (currently not used)
 *     - RCSid (needless warning)
 * removed some unused variables
 * added typecast before mgt5200SioCallbackInstall
 *
 * Revision 1.5  2005/11/30 11:37:22  rlange
 * removed task(Un)Lock + int(Un)Lock, disable RX IRQ if shadow register variable
 * is modified outside the ISR
 *
 * Revision 1.4  2005/11/22 11:39:04  rlange
 * Cosmetic removed logMsg
 *
 * Revision 1.3  2005/11/22 11:33:33  rlange
 * MPC5200 Intr Status Register work around:
 * changed interrupt service routine mgt5200sioInt()
 * Check IMR register so only real TX interrupts are handled.
 *
 * Revision 1.2  2005/03/14 12:10:13  ufranke
 * added
 *  - RCSid
 *
 * Revision 1.2  1998/08/24 11:24:45  Franke
 * cosmetics
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002..2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

const char *mgt5200sio_RCSid="$Id: mgt5200Sio.c,v 1.11 2007/09/11 21:57:47 cs Exp $";

/*
DESCRIPTION
This is the driver for the MGT5200 PSCs.

A MGT5200SIO_CHAN structure is used to describe the serial channel. This data
structure is defined in mgt5200Sio.h.

Only asynchronous serial operation is supported by this driver.
The default serial settings are 8 data bits, 1 stop bit, no parity, 9600
baud, and software flow control.

USAGE

The BSP's sysHwInit() routine typically calls sysSerialHwInit(), which
creates the MGT5200SIO_CHAN structure and initializes all the values
in the structure (except the SIO_DRV_FUNCS) before calling
mgt5200SioDevInit().

The BSP's sysHwInit2() routine typically calls sysSerialHwInit2(),
which connects the chips interrupts via intConnect() `mgt5200SioInt'

INCLUDE FILES: mgt5200Sio.h

*/

/* includes */
#include "vxWorks.h"
#include "intLib.h"
#include "errnoLib.h"
#include "errno.h"
#include "sioLib.h"
#include "mgt5200Sio.h"
#define MAC_MEM_MAPPED
#include <MEN/men_typs.h>
#include <MEN/mgt5200.h>
#include <MEN/maccess.h>

/* local defines       */
#ifndef SIO_HUP
#   define SIO_OPEN	0x100A
#   define SIO_HUP	0x100B
#endif

#define IN8(reg)		MREAD_D8( (MACCESS)pChan->regs, reg )
#define OUT8(reg,val)	MWRITE_D8( (MACCESS)pChan->regs, reg, val )
#define IN16(reg)		MREAD_D16( (MACCESS)pChan->regs, reg )
#define OUT16(reg,val)	MWRITE_D16( (MACCESS)pChan->regs, reg, val )

/* min/max baud rate */
#define MGT5200SIO_MIN_RATE 50
#define MGT5200SIO_MAX_RATE 115200

/* static forward declarations */
LOCAL 	int 	mgt5200SioCallbackInstall (SIO_CHAN *, int, STATUS (*)(), void *);
LOCAL 	STATUS 	mgt5200SioDummyCallback ();
LOCAL 	void 	mgt5200SioInitChannel (MGT5200SIO_CHAN *);
LOCAL   STATUS  mgt5200SioBaudSet (MGT5200SIO_CHAN *, UINT);
LOCAL 	STATUS  mgt5200SioModeSet (MGT5200SIO_CHAN *, UINT);
LOCAL 	STATUS 	mgt5200SioIoctl (MGT5200SIO_CHAN *, int, int);
LOCAL 	void 	mgt5200SioTxStartup (MGT5200SIO_CHAN *);
LOCAL 	int 	mgt5200SioPollOutput (MGT5200SIO_CHAN *, char);
LOCAL 	int 	mgt5200SioPollInput (MGT5200SIO_CHAN *, char *);
LOCAL 	STATUS 	mgt5200SioOptsSet (MGT5200SIO_CHAN *, UINT);
LOCAL 	STATUS 	mgt5200SioOpen (MGT5200SIO_CHAN * pChan );
LOCAL 	STATUS 	mgt5200SioHup (MGT5200SIO_CHAN * pChan );

/* driver functions */

static SIO_DRV_FUNCS mgt5200SioDrvFuncs =
{
    (int (*)())mgt5200SioIoctl,
    (int (*)())mgt5200SioTxStartup,
    (int (*)())mgt5200SioCallbackInstall,
    (int (*)())mgt5200SioPollInput,
    (int (*)(SIO_CHAN *,char))mgt5200SioPollOutput
};

/******************************************************************************
*
* mgt5200SioDummyCallback - dummy callback routine.
*
* NOMANUAL
*
* RETURNS: ERROR always
*/
LOCAL STATUS mgt5200SioDummyCallback (void)
{
    return (ERROR);
}

/******************************************************************************
*
* mgt5200SioDevInit - intialize a MGT5200 PCS SIO channel
*
* This routine initializes some SIO_CHAN function pointers and then resets
* the chip in a quiescent state.  Before this routine is called, the BSP
* must already have initialized all the device addresses, etc. in the
* MGT5200SIO_CHAN structure normaly done in sysSerialHwInit().
*
* NOTE: The corresponding PSC will be set to 8n1 and port will be set to
* UART mode.
*
* RETURNS: N/A
*/

void mgt5200SioDevInit
(
    MGT5200SIO_CHAN * pChan	/* pointer to channel */
    )
{
    int oldlevel = intLock ();

    /* initialize the driver function pointers in the SIO_CHAN's */
    pChan->pDrvFuncs    = &mgt5200SioDrvFuncs;

    /* set the non BSP-specific constants */
    pChan->getTxChar    = mgt5200SioDummyCallback;
    pChan->putRcvChar   = mgt5200SioDummyCallback;
    pChan->channelMode  = 0;    /* undefined */

    /* should be init in sysSerialHwInit() if not set defaults */
    if( pChan->options == 0 ){
    	pChan->options  = (CLOCAL | CREAD | CS8);
    }

    /* reset the chip */
    mgt5200SioInitChannel (pChan);

    intUnlock (oldlevel);
}

/*******************************************************************************
*
* mgt5200SioInitChannel - initialize UART
*
* Initialize the number of data bits, parity and set the selected
* baud rate. Set the modem control signals if the option is selected.
*
* RETURNS: N/A
*/
LOCAL void mgt5200SioInitChannel
(
    MGT5200SIO_CHAN * pChan	/* pointer to channel */
    )
{
	pChan->ier = 0;
	OUT16( PSC_ISR_IMR, pChan->ier );	/* disable interrupts */
	OUT8 ( PSC_SICR, 0x00 );			/* UART mode, no CD */
	OUT16( PSC_SR_CSR, 0x0000 );		/* use prescaled CLKIN pin (IP clk) */

	OUT8( PSC_CR, 0x0020);           	/* reset RX and RXFIFO */
	OUT8( PSC_CR, 0x0030);           	/* reset TX and TXFIFO */
	OUT8( PSC_CR, 0x0040);			 	/* reset error status */
	OUT8( PSC_CR, 0x0050);			  	/* reset break change int */
	OUT8( PSC_CR, 0x0010);           	/* reset mode reg pointer */
	OUT8( PSC_IPCR_ACR, 0x00 ); 	  	/* no interrupts for CD/CTS */


    /* set the requested baud rate */
    mgt5200SioBaudSet(pChan, pChan->baudRate);

    /* set the options */
    mgt5200SioOptsSet(pChan, pChan->options);

	OUT16( PSC_RFALARM, 0x0000 );	/* 0x01f0 */
	OUT16( PSC_TFALARM, 0x0000 );	/* 0x00f0 */

	OUT8( PSC_OP1, 0x01 );			 /* assert RTS (continously) */
	OUT8( PSC_CR, 0x0005);           /* enable receiver & transmitter */
}

/*******************************************************************************
*
* mgt5200SioOptsSet - set the serial options
*
* Set the channel operating mode to that specified.
*
* Note, this routine disables the transmitter.  The calling routine
* may have to re-enable it.
*
* RETURNS:
* Returns OK to indicate success, otherwise ERROR is returned
*/

LOCAL STATUS mgt5200SioOptsSet
(
    MGT5200SIO_CHAN * pChan,	/* pointer to channel */
    UINT options		/* new hardware options */
    )
{

    int mr1Value;
    int mr2Value;
    int lvl;

    if (pChan == NULL || options & 0xffffff00)
        return (ERROR);

    /* Reset the transmitters  & receivers  */

    switch (options & CSIZE)
        {
        case CS5:
            mr1Value = PSC_MR1_UART_BITS_CHAR_5;
            break;
        case CS6:
            mr1Value = PSC_MR1_UART_BITS_CHAR_6;
            break;
        case CS7:
            mr1Value = PSC_MR1_UART_BITS_CHAR_7;
            break;
        default:
        case CS8:
            mr1Value = PSC_MR1_UART_BITS_CHAR_8;
            break;
        }

    if (options & STOPB)
        mr2Value = PSC_MR2_UART_STOP_BITS_2;
    else
        mr2Value = PSC_MR2_UART_STOP_BITS_1;

    switch (options & (PARENB|PARODD))
        {
        case PARENB|PARODD:
            mr1Value |= PSC_MR1_UART_ODD_PARITY;
            break;
        case PARENB:
            mr1Value |= PSC_MR1_UART_EVEN_PARITY;
            break;
        case PARODD:
            mr1Value |= PSC_MR1_UART_PAR_MODE_MULTI;
            break;
        default:
        case 0:
            mr1Value |= PSC_MR1_UART_NO_PARITY; break;
        }

    if (!(options & CLOCAL) )
        {
        /* clocal enables hardware flow control */
        mr1Value |= PSC_MR1_UART_RX_RTS;
        mr2Value |= PSC_MR2_UART_TX_CTS;
        }

    lvl = intLock ();

    /* now reset the channel mode registers */

    OUT8( PSC_CR, PSC_CR_UART_RESET_MODE_PTR | PSC_CR_UART_RX_DISABLE | PSC_CR_UART_TX_DISABLE);
    OUT8( PSC_CR, PSC_CR_UART_RESET_TX);
    OUT8( PSC_CR, PSC_CR_UART_RESET_RX);

    OUT8( PSC_MR, mr1Value);  /* mode register 1  */
    OUT8( PSC_MR, mr2Value);  /* mode register 2  */

    OUT8( PSC_CR, PSC_CR_UART_TX_ENABLE);
    if (options & CREAD)
        OUT8( PSC_CR, PSC_CR_UART_RX_ENABLE);

    intUnlock (lvl);

    pChan->options = options;

    return OK;
}

/*******************************************************************************
*
* mgt5200SioHup - hang up the modem control lines
*
* Resets the RTS and DTR signals and clears both the receiver and
* transmitter sections.
*
* RETURNS: OK
*/
LOCAL STATUS mgt5200SioHup
(
    MGT5200SIO_CHAN * pChan 	/* pointer to channel */
    )
{
    FAST int     oldlevel;	/* current interrupt level mask */

    oldlevel = intLock ();

    intUnlock (oldlevel);

    return (OK);

}

/*******************************************************************************
*
* mgt5200SioOpen - Set the modem control lines
*
* Set the modem control lines(RTS, DTR) TRUE if not already set.
* It also clears the receiver, transmitter and enables the fifo.
*
* RETURNS: OK
*/
LOCAL STATUS mgt5200SioOpen
(
    MGT5200SIO_CHAN * pChan 	/* pointer to channel */
    )
{
#if 0
    FAST int     oldlevel;	/* current interrupt level mask */
    char mask;

    mask = REG(MCR, pChan) & (MCR_RTS | MCR_DTR);

    if (mask != (MCR_RTS | MCR_DTR))
	{
    	/* RTS and DTR not set yet */

    	oldlevel = intLock ();

		/* set RTS and DTR TRUE */

    	/*pChan->mcr |= (MCR_DTR | MCR_RTS); */
    	/*REG(MCR, pChan) = pChan->mcr; */

    	/* clear Tx and receive and enable FIFO */

        REG(FCR, pChan) = (RxCLEAR | TxCLEAR | FIFO_ENABLE);

    	intUnlock (oldlevel);
	}
#endif

    return (OK);
}

/******************************************************************************
*
* mgt5200SioBaudSet - change baud rate for channel
*
* This routine sets the baud rate for the UART. The interrupts are disabled
* during chip access.
*
* RETURNS: OK
*/
LOCAL STATUS  mgt5200SioBaudSet
(
    MGT5200SIO_CHAN * pChan,	/* pointer to channel */
    UINT	   baud		/* requested baud rate */
    )
{
    int   oldlevel;
	ULONG tmp;

    /* disable interrupts during chip access */

    oldlevel = intLock ();

	tmp = pChan->ipbClk/16/baud;		/* calculate baudrate */
	if( tmp & 1 ) tmp++;				/* round */
	tmp >>= 1;
	OUT8( PSC_CTUR, tmp >> 8 );
	OUT8( PSC_CTLR, tmp & 0xff );

    pChan->baudRate = baud;

    intUnlock (oldlevel);

    return (OK);
}

/*******************************************************************************
*
* mgt5200SioModeSet - change channel mode setting
*
* This driver supports both polled and interrupt modes and is capable of
* switching between modes dynamically.
*
* If interrupt mode is desired this routine enables the channels receiver and
* transmitter interrupts.
* If polled mode is desired the device interrupts are disabled.
*
* RETURNS:
* Returns a status of OK if the mode was set else ERROR.
*/
LOCAL STATUS mgt5200SioModeSet
(
    MGT5200SIO_CHAN * pChan,	/* pointer to channel */
    UINT	newMode		/* mode requested */
    )
{
    FAST int     oldlevel;	/* current interrupt level mask */

    if ((newMode != SIO_MODE_POLL) && (newMode != SIO_MODE_INT))
		return (ERROR);

    oldlevel = intLock ();

    if (newMode == SIO_MODE_INT)
	{
        /* Enable appropriate interrupts */
		OUT16( PSC_ISR_IMR, (pChan->ier = 0x0300) );/* enable Rx/Tx int */

	}
    else
	{
        /* disable all mgt5200Sio interrupts */
		OUT16( PSC_ISR_IMR, (pChan->ier = 0x0000) );/* enable Rx int */
	}

    pChan->channelMode = newMode;

    intUnlock (oldlevel);

    return (OK);
}

/*******************************************************************************
*
* mgt5200SioIoctl - special device control
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
LOCAL STATUS mgt5200SioIoctl
(
    MGT5200SIO_CHAN * 	pChan,		/* pointer to channel */
    int			request,	/* request code */
    int        		arg		/* some argument */
    )
{
    FAST STATUS  status;

    status = OK;

    switch (request)
	{
	case SIO_BAUD_SET:
	    if (arg < MGT5200SIO_MIN_RATE || arg > MGT5200SIO_MAX_RATE)
			status = EIO;		/* baud rate out of range */
	    else
	        status = mgt5200SioBaudSet (pChan, arg);
	    break;

	case SIO_BAUD_GET:
		*(int *)arg = pChan->baudRate;
		break;

	case SIO_MODE_SET:
	    status = (mgt5200SioModeSet (pChan, arg) == OK) ? OK : EIO;
		break;

	case SIO_MODE_GET:
		*(int *)arg = pChan->channelMode;
		break;

	case SIO_AVAIL_MODES_GET:
		*(int *)arg = SIO_MODE_INT | SIO_MODE_POLL;
		break;

	case SIO_HW_OPTS_SET:
		status = (mgt5200SioOptsSet (pChan, arg) == OK) ? OK : EIO;
		break;

	case SIO_HW_OPTS_GET:
		*(int *)arg = pChan->options;
		break;

	case SIO_HUP:
		/* check if hupcl option is enabled */

		if (pChan->options & HUPCL)
	    	status = mgt5200SioHup (pChan);
		break;

	case SIO_OPEN:
		/* check if hupcl option is enabled */

		if (pChan->options & HUPCL)
	    	status = mgt5200SioOpen (pChan);
	    break;

	default:
		status = ENOSYS;
	}
    return (status);
}


/*******************************************************************************
*
* mgt5200SioIntWr - handle a transmitter interrupt
*
* This routine handles write interrupts from the UART. It reads a character
* and puts it in the transmit holding register of the device for transfer.
*
* If there are no more characters to transmit, transmission is disabled by
* clearing the transmit interrupt enable bit in the IER(int enable register).
*
* NOMANUAL
*
* RETURNS: N/A
*
*/
void mgt5200SioIntWr
(
    MGT5200SIO_CHAN * pChan		/* pointer to channel */
    )
{
    char           outChar;

    if ((*pChan->getTxChar) (pChan->getTxArg, &outChar) != ERROR)
        OUT8( PSC_RB_TB, outChar );	/* write char to Transmit Holding Reg */
    else
	{
        pChan->ier &= ~0x0100; /* disable Tx, enable Rx */
        OUT16( PSC_ISR_IMR, pChan->ier );
	}
}


/*******************************************************************************
*
* mgt5200SioIntRd - handle a receiver interrupt
*
* This routine handles read interrupts from the UART.
*
* NOMANUAL
*
* RETURNS: N/A
*/
LOCAL void mgt5200SioIntRd
(
    MGT5200SIO_CHAN * pChan	/* pointer to channel */
    )
{
    char   inchar;

    /* read character from Receive Holding Reg. */
	while (IN16( PSC_SR_CSR ) & PSC_SR_CSR_UART_RX_READY){
    inchar = IN8( PSC_RB_TB );

    (*pChan->putRcvChar) (pChan->putRcvArg, inchar);

		if (pChan->channelMode != SIO_MODE_INT)
	        break;
	}
}

#if 0 /* currently not used */
/*******************************************************************************
*
* mgt5200SioIntEx - miscellaneous interrupt processing
*
* This routine handles miscellaneous interrupts on the UART.
* Not implemented yet.
*
* NOMANUAL
*
* RETURNS: N/A
*
*/
LOCAL void mgt5200SioIntEx
(
    MGT5200SIO_CHAN *pChan		/* pointer to channel */
    )
{
    /* Nothing for now... */
}
#endif /* 0 currently not used */

/********************************************************************************
*
* mgt5200SioInt - PSC interrupt
*
* This routine handles 2 sources of interrupts from the UART. They are
* prioritized in the following order by the Interrupt Identification Register:
* Received Data Ready and Transmit Holding Register Empty.
*
* NOMANUAL
*
* RETURNS: N/A
*/
void mgt5200SioInt
(
    MGT5200SIO_CHAN * pChan	/* pointer to channel */
    )
{
    USHORT intStatus;

	intStatus = IN16( PSC_ISR_IMR );

	while( ( ((intStatus = IN16( PSC_ISR_IMR )) & 0x0100) && (pChan->ier & 0x0100) ) ||
		   ( (intStatus & 0x0200) && (pChan->ier & 0x0200) ) ){
	if( intStatus & 0x0200 ){
		mgt5200SioIntRd (pChan);  	/* RxChar Avail */
	}
		if( intStatus & 0x0100 ){
			mgt5200SioIntWr (pChan);  	/* TxEmpty Avail */
		}
	}
}

/*******************************************************************************
*
* mgt5200SioTxStartup - transmitter startup routine
*
* Call interrupt level character output routine and enable interrupt if it is
* in interrupt mode with no hardware flow control.
* If the option for hardware flow control is enabled and CTS is set TRUE,
* then the Tx interrupt is enabled.
*
* RETURNS: N/A
*/
LOCAL void mgt5200SioTxStartup
(
    MGT5200SIO_CHAN * pChan 	/* pointer to channel */
    )
{

			pChan->ier |= 0x0100;
			OUT16( PSC_ISR_IMR, pChan->ier );
		}

/******************************************************************************
*
* mgt5200SioPollOutput - output a character in polled mode.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the output buffer if full.
*/
LOCAL int mgt5200SioPollOutput
(
    MGT5200SIO_CHAN *  pChan,	/* pointer to channel */
    char            outChar	/* char to send */
    )
{
    /* is the transmitter ready to accept a character? */
	if( (IN16(PSC_SR_CSR) & 0x0400) == 0 )
		return (EAGAIN);

   	OUT8( PSC_RB_TB, outChar );

    return (OK);
}
/******************************************************************************
*
* mgt5200SioPollInput - poll the device for input.
*
* RETURNS: OK if a character arrived, EIO on device error, EAGAIN
* if the input buffer if empty.
*/
LOCAL int mgt5200SioPollInput
(
    MGT5200SIO_CHAN *  pChan,	/* pointer to channel */
    char *          pChar 	/* pointer to char */
    )
{

    if ( (IN16(PSC_SR_CSR) & 0x0100) == 0 )
        return (EAGAIN);

    /* got a character */
    *pChar = IN8( PSC_RB_TB );

    return (OK);
}

/******************************************************************************
*
* mgt5200SioCallbackInstall - install ISR callbacks to get/put chars.
*
* This routine installs the callback functions for the driver
*
* RETURNS: OK on success or ENOSYS on unsupported callback type.
*/
LOCAL int mgt5200SioCallbackInstall
(
    SIO_CHAN *  pSioChan,	/* pointer to device to control */
    int         callbackType,	/* callback type(tx or receive) */
    STATUS      (*callback)(),	/* pointer to callback function */
    void *      callbackArg	/* callback function argument */
    )
{
    MGT5200SIO_CHAN * pChan = (MGT5200SIO_CHAN *)pSioChan;

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


