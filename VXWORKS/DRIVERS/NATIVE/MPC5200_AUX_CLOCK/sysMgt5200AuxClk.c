/* sysMgt5200AuxClk.c - Motorola MGT5200 Auxilary Clock Driver*/

/* Copyright 2002 MEN GmbH */

/*
modification history
--------------------
$Log: sysMgt5200AuxClk.c,v $
Revision 1.6  2006/02/28 12:07:04  UFRANKE
cosmetics

Revision 1.5  2006/02/14 21:22:24  cs
commented out RCSid (needless warning)

Revision 1.4  2005/04/05 14:45:04  ufranke
changed
 - include from DRIVERS/NATIVE/MPC5200/INTRCTRL/mgt5200IntrCtl.h
   not longer BSP specific

Revision 1.3  2005/03/14 12:09:33  ufranke
changed
 - not longer BSP specific

*/

const char *mpc5200AuxClock_RCSid="$Id: sysMgt5200AuxClk.c,v 1.6 2006/02/28 12:07:04 UFRANKE Exp $";


/*
DESCRIPTION

can be shared with GPIO timer ports

*/

/* includes */

#include "vxWorks.h"
#include "intLib.h"
#include "sysLib.h"
#include "iv.h"

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mgt5200.h>

#define MAC_MEM_MAPPED
#include <MEN/maccess.h>

#include "DRIVERS/NATIVE/MPC5200/INTRCTRL/mgt5200IntrCtl.h"
#include "sysMgt5200AuxClk.h"
#include "drv/timer/timerDev.h"

/* local defines */
#define AUX_CLK_RATE_MIN	10
#define AUX_CLK_RATE_MAX	1000

/* extern declarations */
extern u_int32 sysMbarAddrGet( void );


/* locals */
LOCAL FUNCPTR sysAuxClkRoutine        = NULL;
LOCAL int     sysAuxClkArg            = 0;
LOCAL BOOL    sysAuxClkRunning        = FALSE;
LOCAL BOOL    sysAuxClkIntConnected   = FALSE;
LOCAL int     sysAuxClkTicksPerSecond = 60;

LOCAL int     sysAuxIpbClock  = 0;
LOCAL int     sysAuxUsedTimer = -1;



/* initialize the MGT5200 AuxClk driver */
void sysMgt5200AuxClkInit
(
	int ipbClk,		/* MGT internal IPB bus clock */
	int usedTimer	/* used general purose timer 1..7 */
)
{
	/* TIMER 0 can't be used due to MGT5100 bug Errata:	 Number EISaz00549 */
	if( 1 <= usedTimer && usedTimer <= 7 )
		sysAuxUsedTimer = usedTimer;
		
	sysAuxIpbClock = ipbClk;
}/*sysMgt5200AuxClkInit*/

/* check if sysMgt5200AuxClkInit() was called */
static int checkInitialized( void )
{
	if( sysAuxUsedTimer == -1 )
		return( ERROR );
	else
		return( OK );
}/*checkInitialized*/

/*******************************************************************************
*
* sysAuxClkInt - auxiliary clock interrupt handler
*
* This routine handles the auxiliary clock interrupt.  It calls a user routine
* if one was specified by the routine sysAuxClkConnect().
*/

LOCAL void sysAuxClkInt (void)
{
	u_int32 mbar = sysMbarAddrGet();


	if (sysAuxClkRoutine != NULL)
        (*sysAuxClkRoutine) (sysAuxClkArg);
        
	MSETMASK_D32( (mbar+MGT5200_GPT_0_STATUS),
	           (MGT5200_GPT_CHANNEL_OFFS*sysAuxUsedTimer),
			 	MGT5200_GPT_STATUS__TEXP );
}

/*******************************************************************************
*
* sysAuxClkConnect - connect a routine to the auxiliary clock interrupt
*
* This routine specifies the interrupt service routine to be called at each
* auxiliary clock interrupt.  It does not enable auxiliary clock
* interrupts.
*
* RETURNS: OK, or ERROR if the routine cannot be connected to the interrupt.
*
* SEE ALSO: intConnect(), sysAuxClkEnable()
*/

STATUS sysAuxClkConnect
    (
    FUNCPTR routine,	/* routine called at each aux. clock interrupt */
    int     arg		/* argument to auxiliary clock interrupt routine */
    )
    {

	if( checkInitialized() )
		return( ERROR );

    /* connect the ISR to the TIMER exception */
    sysAuxClkRoutine   = routine;
    sysAuxClkArg       = arg;

    return (OK);
    }

/*******************************************************************************
*
* sysAuxClkDisable - turn off auxiliary clock interrupts
*
* This routine disables auxiliary clock interrupts.
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkEnable()
*/

void sysAuxClkDisable (void)
{
	int lockOutKey;
	u_int32 mbar = sysMbarAddrGet();
	
	if( checkInitialized() )
		return;

    if (sysAuxClkRunning)
	{
		intDisable(INT_VEC_TMR_0+sysAuxUsedTimer);

		/* lock critical section */
    	taskLock();
   	    lockOutKey = intLock();     /* disable irqs */

		/* disable counter by
	     * init the control register - do not touch the GPIO bits
	     */
		MCLRMASK_D32( (mbar+MGT5200_GPT_0_CTRL),
			            (MGT5200_GPT_CHANNEL_OFFS*sysAuxUsedTimer),
	        	    	(0xFFFFFFFF & 
	            	  	 ~(MGT5200_GPT_CTRL__GPIO_OUT_HIGH
	                		| MGT5200_GPT_CTRL__MODE_SEL_INTERNAL )
	                	 ) );

		/* unlock critical section */
		intUnlock( lockOutKey );
    	taskUnlock();

		sysAuxClkRunning = FALSE;		/* clock is no longer running */
	}
}

typedef struct
{
	u_int32  pre;
	u_int32  val;
	u_int32	 diff;
}AUX_CLK_DAT;

AUX_CLK_DAT G_auxClkDat;

void computeSetup
( 
	int ticksPerSecond,
	int ipbClock,
	u_int32 *preP, 
	u_int32 *valP 
)
{
	AUX_CLK_DAT curr;
	int currClk;
	
	
	G_auxClkDat.pre  =  0xFFFF;
	G_auxClkDat.val  =  0xFFFF;
	G_auxClkDat.diff =  0xFFFFFFFF;

	curr.pre  =  0xFFFF;
	curr.val  =  0xFFFF;
	curr.diff =  0xFFFFFFFF;
	
	for( curr.pre = 4; curr.pre < 0xffff; curr.pre++ )
	{
		for( curr.val = 1; curr.val < 0xffff; curr.val++ )
		{
			currClk   = ipbClock / (curr.pre*curr.val);
			if( ticksPerSecond > currClk )
				curr.diff = ticksPerSecond - currClk;
			else 
				curr.diff = currClk - ticksPerSecond;
				
			if( curr.diff < G_auxClkDat.diff )
			{
				G_auxClkDat = curr;
				if( curr.diff == 0 )
					goto FINISH;
			}/*if*/
		}/*for*/
	}/*for*/
	
FINISH:
/*	printf("ticksPerSecond %d tickPerSec %d\n",
			ticksPerSecond,
			ipbClock / (G_auxClkDat.pre*G_auxClkDat.val) ); */
	*preP = G_auxClkDat.pre;
	*valP = G_auxClkDat.val;
}/*computeSetup*/

/*******************************************************************************
*
* sysAuxClkEnable - turn on auxiliary clock interrupts
*
* This routine enables auxiliary clock interrupts.
* The timer is used in "reference mode" i.e. a value is programmed into 
* the reference register and an interrupt occurs when the timer reaches
* that value. 
*
* RETURNS: N/A
*
* SEE ALSO: sysAuxClkConnect(), sysAuxClkDisable(), sysAuxClkRateSet()
*/

void sysAuxClkEnable (void)
{
	int lockOutKey;
	u_int32 mbar = sysMbarAddrGet();
	u_int32 pre = 0xffff;
	u_int32 val = 0xffff;

	if( checkInitialized() )
		return;

	computeSetup( sysAuxClkTicksPerSecond, sysAuxIpbClock, &pre, &val );

	MWRITE_D32( (mbar+MGT5200_GPT_0_COUNTER_IN),
				(MGT5200_GPT_CHANNEL_OFFS*sysAuxUsedTimer),
				  (MGT5200_GPT_COUNTER_IN__PRESCALE_MASK &  (pre << 16))
				| (MGT5200_GPT_COUNTER_IN__VALUE_MASK    &  val )
			  );


    if (! sysAuxClkIntConnected)
    {
            (void) intConnect (INUM_TO_IVEC(INT_VEC_TMR_0+sysAuxUsedTimer), 
			       (VOIDFUNCPTR) sysAuxClkInt, 0 );
            sysAuxClkIntConnected = TRUE;
	}

	/* lock critical section */
   	taskLock();
    lockOutKey = intLock();     /* disable irqs */

	/* init the control register - do not touch the GPIO bits */
	MCLRMASK_D32( (mbar+MGT5200_GPT_0_CTRL),
	            (MGT5200_GPT_CHANNEL_OFFS*sysAuxUsedTimer),
	            (0xFFFFFFFF & 
	              ~(MGT5200_GPT_CTRL__GPIO_OUT_HIGH
	                | MGT5200_GPT_CTRL__MODE_SEL_INTERNAL )
	             ) );

	MSETMASK_D32( (mbar+MGT5200_GPT_0_CTRL),
	            (MGT5200_GPT_CHANNEL_OFFS*sysAuxUsedTimer),
				  MGT5200_GPT_CTRL__COUNTER_ENABLE
				| MGT5200_GPT_CTRL__CONTINOUS	  
				| MGT5200_GPT_CTRL__IRQ_ENABLE    
				| MGT5200_GPT_CTRL__MODE_SEL_INTERNAL
			   );

	/* unlock critical section */
	intUnlock( lockOutKey );
   	taskUnlock();

	intEnable(INT_VEC_TMR_0+sysAuxUsedTimer);
   
	sysAuxClkRunning = TRUE;
}

/*******************************************************************************
*
* sysAuxClkRateGet - get the auxiliary clock rate
*
* This routine returns the interrupt rate of the auxiliary clock.
*
* RETURNS: The number of ticks per second of the auxiliary clock.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateSet()
*/

int sysAuxClkRateGet (void)
    {
    return (sysAuxClkTicksPerSecond);
    }

/*******************************************************************************
*
* sysAuxClkRateSet - set the auxiliary clock rate
*
* This routine sets the interrupt rate of the auxiliary clock.
* It does not enable auxiliary clock interrupts.
*
* RETURNS: OK, or ERROR if the tick rate is invalid or the timer cannot be
* set.
*
* SEE ALSO: sysAuxClkEnable(), sysAuxClkRateGet()
*/

STATUS sysAuxClkRateSet
    (
    int ticksPerSecond	    /* number of clock interrupts per second */
    )
    {

	if( checkInitialized() )
		return( ERROR );

    if( ticksPerSecond < AUX_CLK_RATE_MIN || AUX_CLK_RATE_MAX < ticksPerSecond )
		return (ERROR);

    sysAuxClkTicksPerSecond = ticksPerSecond;

    if (sysAuxClkRunning)
	{
		sysAuxClkDisable ();
		sysAuxClkEnable ();
	}

    return (OK);
    }

