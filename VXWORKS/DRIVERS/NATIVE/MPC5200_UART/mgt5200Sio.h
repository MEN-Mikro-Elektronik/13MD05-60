/* mgt5200Sio.h - MGT5200 PSC header file */

/****************************************************************************
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mgt5200Sio.h,v $
 * Revision 1.2  2005/03/14 12:09:50  ufranke
 * cosmetics
 *
 * Revision 1.2  1998/08/24 11:24:45  Franke
 * cosmetics
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002..2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef __INmgt5200Sioh
#define __INmgt5200Sioh

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE


typedef  struct 	/* MGT5200SIO_CHAN */
{
    /* always goes first */

    SIO_DRV_FUNCS *     pDrvFuncs;      /* driver functions */

    /* callbacks */

    STATUS  (*getTxChar) (); 	/* pointer to xmitr function */	
    STATUS  (*putRcvChar) (); 	/* pointer to rcvr function */
    void *  getTxArg;
    void *  putRcvArg;

    void 	*regs;		/* PSC registers */
	UINT16	ier;		/* interrupt enable shadow */

    UINT16  channelMode;/* such as INT, POLL modes */
    int     baudRate;
    UINT32  ipbClk;		/* UART clock frequency     */     
    UINT32	options;	/* hardware setup options */

} MGT5200SIO_CHAN;

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern void mgt5200SioInt (MGT5200SIO_CHAN *);
extern void mgt5200SioDevInit (MGT5200SIO_CHAN *);

#else

extern void mgt5200SioInt ();
extern void mgt5200SioDevInit ();

#endif  /* __STDC__ */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
 
#endif /* __INCmgt5200Sioh */
