/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m85xxGlobalTimer.c
 *      Project: VxWorks for MEN MPC85xx based boards
 *
 *       Author: ts
 *        $Date: 2008/01/21 20:20:46 $
 *    $Revision: 1.4 $
 *
 *  Description: BSP-local implementation for auxiliary timer. The previous
 *				 default implementation from ADS8560 BSP in m85xxTimer.c
 *               turned out to support not all desired tick rates seamless, the
 *               FITs (fixed period timers) provide only 64 discrete clocks.
 *
 *               For Documentation of PIC global timers see MPC8540
 *				 Reference Manual Sect. 10.3.2 "Global Timer Registers".
 *				 The selected global timer (0-3) can be defined during
 *				 compile time by changing the define USED_GTM (see below)/
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m85xxGlobalTimer.c,v $
 * Revision 1.4  2008/01/21 20:20:46  cs
 * Fixed:
 *   - use local function sysAuxClkInt() as main interrupt function,
 *     sysAuxClkConnect() just inserts a hook to this local function
 *   - don't disable the aux clk when setting clk rate (preserve state)
 *
 * Revision 1.3  2008/01/11 11:54:43  ts
 * Cosmetics, changed reference to Timer0 in comments to a generic GT
 *
 * Revision 1.2  2008/01/08 17:50:45  ts
 * made functions to issue a warning if define INCLUDE_AUX_CLK is not set
 * in config.h and any sysAuxClkXXX function is called
 *
 * Revision 1.1  2008/01/08 16:41:05  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/* includes */

#include "vxWorks.h"
#include "vxLib.h"
#include "intLib.h"
#include "sysEpic.h"

/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/

/* Register interval between each of the 4 timers, dont change! */
#define REG_OFFS	0x40

/*
 * used Global Timer(0..3) may be changed here, see Timer defines in sysEpic.h
 */
#define USED_GTM  0	/* Change to 1/2/3 if another global timer desired*/

#define GTM_IRQ_VECTOR 	(EPIC_VEC_GT_IRQ0 + USED_GTM)
#define GTM_CURR_CNT	(EPIC_TM0_CUR_COUNT_REG + (REG_OFFS*USED_GTM))
#define GTM_BASE_CNT	(EPIC_TM0_BASE_COUNT_REG + (REG_OFFS*USED_GTM))
#define GTM_VEC_REG		(EPIC_TM0_VEC_REG + (REG_OFFS*USED_GTM))
#define GTM_DES_REG		(EPIC_TM0_DES_REG + (REG_OFFS*USED_GTM))

/* assert correct timer selection */
#if (USED_GTM > 3)
# error Only Global Timer number 0/1/2/3 is valid!
#endif


/*--------------------------------------*/
/*	GLOBALS			            		*/
/*--------------------------------------*/

/* sysTimerClkFreq is CCB clk / 8, calculated in sysLib.c */
IMPORT UINT32 	sysTimerClkFreq;

LOCAL int 		sysAuxClkTicksPerSecond = 0;
LOCAL BOOL 		sysAuxClkRunning 		= FALSE;
LOCAL FUNCPTR 	sysAuxClkRoutine 		= NULL;
LOCAL int 		sysAuxClkArg 			= (int) NULL;


/***************************************************************************
*
* sysAuxClkInt
*
* auxiliary clock interrupt handler
* This routine handles the auxiliary clock interrupt on the PowerPC Book E
* architecture. It is attached to the Fix Interval Timer vector by the
* routine sysAuxClkConnect().
*
* RETURNS : N/A
*/

void sysAuxClkInt(
	void
)
{
	/* execute the system clock routine */
	if (sysAuxClkRoutine != NULL){
		(*(FUNCPTR) sysAuxClkRoutine) (sysAuxClkArg);
	}
}


/***************************************************************************
*
* sysAuxClkConnect
*
* connect a routine to the auxiliary clock interrupt
* This routine specifies the interrupt service routine to be called at
* each auxiliary clock interrupt. It does not enable auxiliary clock
* interrupts.
* In the MEN-local Implementation this just calls sysEpicIntConnect
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the
* interrupt.
*
* SEE ALSO: excIntConnectTimer(), sysAuxClkEnable()
*/

STATUS sysAuxClkConnect(
	FUNCPTR routine,	/* routine called at each aux. clock interrupt 	*/
	int arg				/* argument to auxiliary clock interrupt 		*/
)
{

#ifdef INCLUDE_AUX_CLK
	sysAuxClkRoutine = routine;
	sysAuxClkArg = arg;
#else
	logMsg("*** Warning: sysAuxClkConnect called while "
		   "INCLUDE_AUX_CLK undefined!\n",
		   0,0,0,0,0,0);
#endif
	return (OK);
}

/***************************************************************************
*
* sysAuxClkEnable
*
* turn on auxiliary clock interrupts
* This routine enables the auxiliary clock by
*   1) set Bit31 in the chosen Global Timers Base Count Register.
*   2) Enable the Timers Interrupt
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkConnect(), sysAuxClkDisable(), sysAuxClkRateSet()
*/

void sysAuxClkEnable(
	void
)
{
#ifdef INCLUDE_AUX_CLK
	unsigned long regVal = 0;

	if ( !sysAuxClkRunning ) {

		/* 1. Enable Interrupt */
		regVal = sysEpicRegRead( GTM_VEC_REG );
		regVal &= ~EPIC_GTVPR_INTR_MSK; /* clear bit 31 causes Irq enabling */
		sysEpicRegWrite( GTM_VEC_REG, regVal);

		/* 2. Preload Current Count Register with Base Count */
		regVal = sysEpicRegRead(GTM_BASE_CNT) & 0x7fffffff;
		sysEpicRegWrite(GTM_CURR_CNT, regVal);

		/*
		 *  3.
		 *  Bit0 (=MSB) Count Inhibit. Always is set following Reset.
		 *  0: counting enabled  1: counting inhibited
		 */
		regVal = sysEpicRegRead(GTM_BASE_CNT);
		regVal &= ~BIT_EPIC(31);
		sysEpicRegWrite(GTM_BASE_CNT, regVal);

		/* set the running flag */
		sysAuxClkRunning = TRUE;
	}
#else
	logMsg("*** Warning: sysAuxClkEnable called while "
		   "INCLUDE_AUX_CLK undefined!\n",
		   0,0,0,0,0,0);
#endif

}

/***************************************************************************
*
* sysAuxClkDisable
*
* turn off auxiliary clock interrupts
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkEnable()
*/
void sysAuxClkDisable(
	void
	)
{

#ifdef INCLUDE_AUX_CLK
	unsigned long regVal = 0;

	if (sysAuxClkRunning) {
		/* 1. disable Interrupt */
		regVal = sysEpicRegRead( GTM_VEC_REG );
		regVal |= EPIC_GTVPR_INTR_MSK; /* clear bit 31 causes Irq enabling */
		sysEpicRegWrite( GTM_VEC_REG, regVal);

		/*
		 *  2. Bit0 (=MSB) Count Inhibit. Always is set following Reset.
		 *  0: counting enabled  1: counting inhibited
		 */
		regVal = sysEpicRegRead(GTM_BASE_CNT);
		regVal |=BIT_EPIC(31); /* set bit 31 */
		sysEpicRegWrite(GTM_BASE_CNT, regVal);

		/* reset the running flag */
		sysAuxClkRunning = FALSE;
	}
#else
	logMsg("*** Warning: sysAuxClkDisable called while "
		   "INCLUDE_AUX_CLK undefined!\n",
		   0,0,0,0,0,0);
#endif

}

/***************************************************************************
*
* sysAuxClkRateGet
*
* get the auxiliary clock rate
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/

int sysAuxClkRateGet(
	void
)
{
	return (sysAuxClkTicksPerSecond);
}

/***************************************************************************
*
* sysAuxClkRateSet
*
* set the auxiliary clock rate
* This routine uses a Global PIC Timer as the auxiliary clock.
* For used Register definitions see sysEpic.h. See sysEpic.c for EPIC
* function implementations, there the Global timers arent supported yet.
*
* RETURNS: OK, or ERROR if the tick rate is invalid or the timer cannot
* be set.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/

STATUS sysAuxClkRateSet(
	int ticksPerSecond	/* number of clock interrupts per second */
)
{

#ifdef INCLUDE_AUX_CLK

	unsigned long regval = 0;
	unsigned long reloadVal = 0;

	if (ticksPerSecond < AUX_CLK_RATE_MIN || ticksPerSecond > AUX_CLK_RATE_MAX)
		return (ERROR);

	sysAuxClkTicksPerSecond	= ticksPerSecond;

	/*
	 * 1. calculate reload Value for the Timer
	 */
	reloadVal = sysTimerClkFreq / ticksPerSecond;

	/*
	 * 2. set the Timer reload Value and write TFRR
	 */
	regval = sysEpicRegRead(GTM_BASE_CNT);
	regval &= BIT_EPIC(31); /* reset reload value, keep Inhibit Bit */
	regval |= reloadVal;  	/* set desired reload value */
	sysEpicRegWrite( GTM_BASE_CNT, regval );

	/* write Timer clock (=CCBclk / 8) in TFRR as recommended in Ref.Manual */
	sysEpicRegWrite( EPIC_TM_FREQ_REG, sysTimerClkFreq );

	/*
	 * 3. set Timer IRQ Destination to "this" Processor. TODO: Use Value from
	 *    WHOAMI Register to keep compatibility with possible future
	 *    multicore CPUs. Currently on 8540 this is hardwired to '1'. Check
	 *    this on future multicore MPCs!
	 */
	sysEpicRegWrite( GTM_DES_REG, 0x00000001 );

	/*
	 * 4. set Interrupt Vector / priority
	 */
	regval = EPIC_GTVPR_PRIORITY(EPIC_PRIORITY_DEFAULT) |
             EPIC_GTVPR_VECTOR( GTM_IRQ_VECTOR ) |
             (sysEpicRegRead(GTM_VEC_REG) & EPIC_GTVPR_INTR_MSK);

	sysEpicRegWrite( GTM_VEC_REG, regval );

#if 0 /* debug helper */
	printf("sysAuxClkRateSet Values:\n");
	printf("   sysTimerClkFreq = %d\n", sysTimerClkFreq );
	printf("   ticksPerSecond  = %d\n", ticksPerSecond  );
	printf("   reloadVal       = %d\n", reloadVal  );
	printf("   regval          = 0x%08x\n", regval );
#endif

#else
	logMsg("*** Warning: call to sysAuxClkRateSet has no effect, "
		   "INCLUDE_AUX_CLK is undefined!\n",
		   0,0,0,0,0,0);
#endif
	return (OK);
}


#ifdef INCLUDE_SHOW_ROUTINES
/******************************************************************************
*
* sysAuxTimerShow - show the Current status of the global timer used as the
* Aux Timer, displays all relevant Registers
*
* RETURNS: N/A
*/
STATUS sysAuxTimerShow(
void
)
{
	printf("Registers of EPIC Global Timer %d (used as aux. timer):\n",
		   USED_GTM );
	printf("TFRR          0x%08x\n", sysEpicRegRead(EPIC_TM_FREQ_REG));
	printf("TCR           0x%08x\n", sysEpicRegRead(EPIC_TM_CTRL));
	printf("Current Count 0x%08x\n", sysEpicRegRead(GTM_CURR_CNT));
	printf("Base Count    0x%08x (%d dec.)\n",
		   sysEpicRegRead(GTM_BASE_CNT),
		   sysEpicRegRead(GTM_BASE_CNT) & 0x7fffffff);
	printf("Vec/Pri Reg   0x%08x\n", sysEpicRegRead(GTM_VEC_REG));
	printf("Dest. Reg     0x%08x\n", sysEpicRegRead(GTM_DES_REG));
	printf("Routine connected: %p; arg 0x%08x\n",sysAuxClkRoutine,sysAuxClkArg);
	return (OK);
}
#endif /*end INCLUDE_SHOW_ROUTINES */


