/* mgt5200IntrCtl.c - MGT5200 interrupt controller driver */
/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: mgt5200IntrCtl.c
 *      Project: VxWorks for MGT5200
 *
 *       Author: kp / windriver
 *        $Date: 2005/08/29 13:38:53 $
 *    $Revision: 1.2 $
 *
 *  Description: MGT5200 interrupt controller driver
 *
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mgt5200IntrCtl.c,v $
 * Revision 1.2  2005/08/29 13:38:53  CSchuster
 * added newline at end of file to avoid compiler warning
 *
 * Revision 1.1  2004/03/01 09:50:55  UFranke
 * Initial Revision - Tornado 2.2
 *
 * Revision 1.1  2003/09/02 10:56:01  UFranke
 * Initial Revision
 *
 * Revision 1.1  2003/05/28 13:52:25  UFranke
 * Initial Revision
 *
 * Revision 1.3  2003/01/22 12:42:21  kp
 * support CAN interrupts
 *
 * Revision 1.2  2002/10/15 10:36:27  kp
 * added support for SMARTCOMM and ethernet interrupt sources
 *
 * Revision 1.1  2002/10/01 09:58:09  UFranke
 * alpha without Ethernet
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char *mgt5200IntRCSid="$Id: mgt5200IntrCtl.c,v 1.2 2005/08/29 13:38:53 CSchuster Exp $\n";

/* globals */

/* locals */
static int intcnt_G = 0;

/* forward static functions */


#ifdef MEN_BSP_TEST
void mgt5200IntShow( void )
{
	MACCESS ma = (MACCESS)sysMbarAddrGet();

	printf("MGT5200 Interrupt Controller:\n");
	printf("=============================\n");
	printf("PER_MASK  \t%04x   %08x\n",  		MGT5200_I_PER_MASK		, (int)MREAD_D32(ma, MGT5200_I_PER_MASK) );
	printf("PER_PRI1  \t%04x   %08x\n",  		MGT5200_I_PER_PRI1		, (int)MREAD_D32(ma, MGT5200_I_PER_PRI1) );
	printf("PER_PRI2  \t%04x   %08x\n",  		MGT5200_I_PER_PRI2 		, (int)MREAD_D32(ma, MGT5200_I_PER_PRI2) );
	printf("PER_PRI3  \t%04x   %08x\n",  		MGT5200_I_PER_PRI3  	, (int)MREAD_D32(ma, MGT5200_I_PER_PRI3) );
	printf("EXT_EN    \t%04x   %08x\n",  		MGT5200_I_EXT_EN      	, (int)MREAD_D32(ma, MGT5200_I_EXT_EN) );
	printf("CRI_PRI_MAIN_MASK \t%04x   %08x\n", MGT5200_I_CRI_PRI_MAIN_MASK , (int)MREAD_D32(ma, MGT5200_I_CRI_PRI_MAIN_MASK ) );
	printf("MAIN_PRI1 \t%04x   %08x\n",  		MGT5200_I_MAIN_PRI1 	, (int)MREAD_D32(ma, MGT5200_I_MAIN_PRI1) );
	printf("MAIN_PRI2 \t%04x   %08x\n",  		MGT5200_I_MAIN_PRI2 	, (int)MREAD_D32(ma, MGT5200_I_MAIN_PRI2) );
	printf("PERMAINCRI_STAT \t%04x   %08x\n",	MGT5200_I_PERMAINCRI_STAT  , (int)MREAD_D32(ma, MGT5200_I_PERMAINCRI_STAT) );
	printf("CRI_STAT  \t%04x   %08x\n",  		MGT5200_I_CRI_STAT  	, (int)MREAD_D32(ma, MGT5200_I_CRI_STAT) );
	printf("MAIN_STAT \t%04x   %08x\n",  		MGT5200_I_MAIN_STAT 	, (int)MREAD_D32(ma, MGT5200_I_MAIN_STAT) );
	printf("PER_STAT  \t%04x   %08x\n",  		MGT5200_I_PER_STAT  	, (int)MREAD_D32(ma, MGT5200_I_PER_STAT) );
	printf("BUS_ERR_STAT \t%04x   %08x\n",  	MGT5200_I_BUS_ERR_STAT  , (int)MREAD_D32(ma, MGT5200_I_BUS_ERR_STAT) );

}/*mgt5200IntShow*/
#endif /*MEN_BSP_TEST*/

/*******************************************************************************
*
* mgt5200IntEnable - enable an MGT5200 interrupt number
*
* This routine enables a specified MGT5200 interrupt number.
*
* RETURNS: OK.
*/
static int mgt5200IntEnable
(
    int intNumber        /* interrupt number to enable */
)
{
	int key = 0;
	MACCESS ma = (MACCESS)sysMbarAddrGet();

	if( intNumber >= INT_VEC_MAIN_BEGIN &&
		intNumber <= INT_VEC_MAIN_END )
	{
		if( !intContext() )
			key = intLock();

		MCLRMASK_D32( ma, MGT5200_I_CRI_PRI_MAIN_MASK,
				 	 1L<<(INT_VEC_MAIN_END-intNumber));

		if( intNumber == INT_VEC_IRQ_1 )
			MSETMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000400);
		else if( intNumber == INT_VEC_IRQ_2 )
			MSETMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000200);
		else if( intNumber == INT_VEC_IRQ_3 )
			MSETMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000100);

		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INT_VEC_CRIT_BEGIN &&
			  intNumber <= INT_VEC_CRIT_END )
	{
		if( !intContext() )
			key = intLock();
		if( intNumber == INT_VEC_IRQ_0 )
			MSETMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000800);
		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INT_VEC_PER_BEGIN &&
			  intNumber <= INT_VEC_PER_END )
	{
		if( !intContext() )
			key = intLock();
		MCLRMASK_D32( ma, MGT5200_I_PER_MASK,
					  1L<<(31-(intNumber-INT_VEC_PER_BEGIN)));
		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INUM_SDMA_FIRST &&
			  intNumber <= INUM_SDMA_LAST )
	{
		if( !intContext() )
			key = intLock();
		sdmaIntEnable(intNumber);
		if( !intContext() )
			intUnlock( key );
	}
	else
	{
		return ERROR;
	}/*if*/

	return OK;
}/*mgt5200IntEnable*/

/*******************************************************************************
*
* mgt5200IntDisable - disable an MGT5x00 interrupt number
*
* This routine disables a specified MGT5x00 interrupt number.
*
* RETURNS: OK.
*/
static int mgt5200IntDisable
(
	int intNumber        /* interrupt number to disable */
)
{
	int key=0;
	MACCESS ma = (MACCESS)sysMbarAddrGet();

	if( intNumber >= INT_VEC_MAIN_BEGIN &&
		intNumber <= INT_VEC_MAIN_END )
	{
		if( !intContext() )
			key = intLock();

		if( intNumber == INT_VEC_IRQ_1 )
			MCLRMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000400);
		else if( intNumber == INT_VEC_IRQ_2 )
			MCLRMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000200);
		else if( intNumber == INT_VEC_IRQ_3 )
			MCLRMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000100);

        /*
         * We must disable IRQ1,2,3 in MGT5100_I_CRI_PRI_MAIN_MASK too
         * or PSC communication stops if i.e. INT_B on PP01 is LOW.
         */
		MSETMASK_D32( ma, MGT5200_I_CRI_PRI_MAIN_MASK,
					  		1L<<(INT_VEC_MAIN_END-intNumber));

		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INT_VEC_CRIT_BEGIN &&
			  intNumber <= INT_VEC_CRIT_END )
	{
		if( !intContext() )
			key = intLock();
		if( intNumber == INT_VEC_IRQ_0 )
			MCLRMASK_D32( ma, MGT5200_I_EXT_EN, 0x00000800);
		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INT_VEC_PER_BEGIN &&
			  intNumber <= INT_VEC_PER_END )
	{
		if( !intContext() )
			key = intLock();
		MSETMASK_D32( ma, MGT5200_I_PER_MASK,
					  1L<<(31-(intNumber-INT_VEC_PER_BEGIN)));
		if( !intContext() )
			intUnlock( key );
	}
	else if ( intNumber >= INUM_SDMA_FIRST &&
			  intNumber <= INUM_SDMA_LAST )
	{
		if( !intContext() )
			key = intLock();
		sdmaIntDisable(intNumber);
		if( !intContext() )
			intUnlock( key );
	}
	else
	{
		return ERROR;
	}/*if*/

	return OK;
}/*mgt5200IntDisable*/


/*****************************************************************************
*
* mgt5200IntInit - initialize the MGT5200 interrupt controller
*
* This routine initializes the MGT5200 main, critical and peripheral
* interrupt controller. Critical interrupt will be routed to normal
* core interrupt causing an external interrupt exception.
*
* RETURNS: OK, always.
*/
STATUS mgt5200IntInit (void)
{
	MACCESS ma = (MACCESS)sysMbarAddrGet();

	/* disable all peripheral interrupts */
	MWRITE_D32( ma, MGT5200_I_PER_MASK, 0xfffffc00 );

	/*
	 * route critical interrupts to normal core int pin,
	 * preparer and disable external IRQs
	 */
	MWRITE_D32( ma, MGT5200_I_EXT_EN,
				(  0x00ff0000     /* IRQ 0..3 level sensitive / active low */
				  | 0x00001000     /* External Enable master bit */
	              | 0x00000001 )     /* route CRIT to normal core IRQ */
				&(~0x00000f00)   /* disable IRQ 0..3           */
	          );

	/* disable all main interrupts */
	MWRITE_D32( ma, MGT5200_I_CRI_PRI_MAIN_MASK, 0x1ffff );

	/* init peripheral priorities (? other irq sources) */
	MWRITE_D32( ma, MGT5200_I_PER_PRI1,
				(INT_PRI_SMARTCOMM << 28) | (INT_PRI_PSC1 << 24)
				| (INT_PRI_ETHERNET << 8));

	MWRITE_D32( ma, MGT5200_I_PER_PRI2,
				(INT_PRI_SPI_M   << 8) );

	MWRITE_D32( ma, MGT5200_I_PER_PRI3,
				(INT_PRI_CAN_1 << 24) | (INT_PRI_CAN_2 << 20));

	/* init main priorities */
	MWRITE_D32( ma, MGT5200_I_MAIN_PRI1,
				(INT_PRI_IRQ_1 << 28)
				| (INT_PRI_IRQ_2 << 24)
				| (INT_PRI_IRQ_3 << 20)
				| (INT_PRI_LO_INT << 16));

	/* enable LO_INT from peripheral to main int controller */
	mgt5200IntEnable( INT_VEC_LO_INT );


    return (OK);
}/*mgt5200IntInit*/


/******************************************************************************
*
* mgt5200IntHandler - handle an MGT5200 interrupt
*
* This routine is called to handle an MGT5200 external interrupt exception.
* Can be either INT, CINT or SMI.
*
* RETURNS: N/A
* SEE ALSO: sysIntHandlerExec()
*/
void mgt5200IntHandler( void )
{
	MACCESS ma = (MACCESS)sysMbarAddrGet();
	ULONG status, reEval=0;
	int vector = -1;

	intcnt_G++;

	status = MREAD_D32( ma, MGT5200_I_PERMAINCRI_STAT );

	if( status & (1L<<10) )
	{		/* int from critical int controller? */
		vector = ((status>>8) & 0x3) + INT_VEC_CRIT_BEGIN;
		reEval |= (1L<<10);

		if( vector == INT_VEC_HI_INT )
		{ /* peripheral int? */
			vector = ((status >> 24) & 0x1f ) + INT_VEC_PER_BEGIN;
			reEval |= (1L<<29);
			if( vector != 0x30 )
				logMsg("status %08x vector %08x\n", status, vector, 0,0,0,0 );
		}
	}
	else if( status & (1L<<21) )
	{		/* int from main controller? */
		vector = ((status>>16) & 0x1f) + INT_VEC_MAIN_BEGIN;
		reEval |= (1L<<21);

		if( vector == INT_VEC_LO_INT )
		{ /* peripheral int? */
			vector = ((status >> 24) & 0x1f ) + INT_VEC_PER_BEGIN;
			reEval |= (1L<<29);
		}
	}
	else
	{
		/* we are not often here */
		return;
	}

	/*
	 * set the upper bits of PSs, MSe and CSe fields to force re-evaluation
	 * of interrupt priorities in interrupt controller (IRQ nesting).
	 *
	 * TODO: HAVE TO MASK CURRENT INTERRUPT (AND LOWER PRIO INTERRUPTS)
	 */
	/*MWRITE_D32( ma, MGT5200_I_PERMAINCRI_STAT, reEval );*/

    /*intUnlock (_PPC_MSR_EE);*/

	/*
	 * dispatch handler routines
	 */
	if( vector >= 0 )
	{
		sysIntHandlerExec (vector, 1);
	}
	else
	 logMsg("*** unhandled status %08x vector %08x\n", status, vector, 0,0,0,0 );
}

#ifdef MEN_BSP_TEST
void intCountShow( void )
{
	printf("CNT   %d\n", intcnt_G );
}
#endif /*MEN_BSP_TEST*/

