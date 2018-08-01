/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_irq.c
 *      Project: OSS library
 *
 *       Author: Franke
 *        $Date: 2012/03/02 18:49:23 $
 *    $Revision: 1.20 $
 *
 *  Description: Interrupt related routines
 *
 *     switches: _SMP_COMPATIBLE_ONLY
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_irq.c,v $
 * Revision 1.20  2012/03/02 18:49:23  ts
 * R: compiler warning about pointer to int conversion without cast
 * M: added cast in logMsg() call
 *
 * Revision 1.19  2011/07/06 15:50:25  dpfeuffer
 * R: debug handle problem analysis for 16G215-01 design test
 * M: some debug messages added
 *
 * Revision 1.18  2010/05/11 16:34:12  ufranke
 * R: DIAB linker error SDA VxWorks 6.7 PPC due to access to global variable
 * M: OSS_IrqNum0 is static now, usage of OSS_GetIrqNum0()
 *
 * Revision 1.17  2009/06/17 09:45:03  ufranke
 * R: link problem if neither _SMP_COMPATIBLE_ONLY nor _UP_COMPATIBLE_ONLY
 *    is defined i.e. in BSP
 * M: _UP_COMPATIBLE_ONLY will be defined internaly in this case
 *
 * Revision 1.16  2009/05/29 11:00:14  ufranke
 * R: UP or SMP compatible now
 * M: changed _SMP_AND_UP_COMPATIBLE to _SMP_COMPATIBLE_ONLY
 *
 * Revision 1.15  2009/04/02 10:51:36  ufranke
 * R: spinLockIsrHeld not exists und UP BSPs
 * M: added workaround for VxWorks 6.7 UP
 *    _UP_COMPATIBLE_ONLY must be defined in custom.mak
 *
 * Revision 1.14  2009/03/31 09:49:53  ufranke
 * R: SMP of VxWorks 6.6 unsafe, wrong settings in pcPentium4, missing patches
 * M: SMP supported at VxWorks 6.7 now
 *
 * Revision 1.13  2008/09/26 14:24:07  ufranke
 * R: SMP support for VxWorks 6.6
 * M: make it SMP and UP compatible
 *
 * Revision 1.12  2008/08/18 16:40:16  cs
 * R: 1. DIAB compiler has more stringent/different error checking
 *       needless warning was thrown in OSS_IrqMaskR()
 * M: 1. added explicit typecast to avoid DIAB compiler warning
 *
 * Revision 1.11  2008/03/20 14:28:02  cs
 * R: a) OSS_IrqHdlRemove did not invalidate the deleted handle
 * M: a) content of passed pointer is set to NULL now
 *
 * Revision 1.10  2007/03/07 12:00:16  cs
 * added casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.9  2005/06/20 11:22:46  SVogel
 * Added function OSS_GetIrqNum0.
 *
 * Revision 1.8  2005/04/12 16:56:11  kp
 * avoid name conflicts
 *
 * Revision 1.7  2004/05/14 09:38:51  UFranke
 * added
 *  + OSS_IrqMaskR()/OSS_IrqRestore()
 *
 * Revision 1.6  2003/01/15 10:17:08  UFranke
 * changed
 *   - OSS_IrqMask() call intLockLevelSet() removed
 *
 * Revision 1.5  2002/02/08 17:53:15  Franke
 * added nested IrqMask/UnMask support and Interface change!!!
 *       the data elements of the IRQ_HANDLE are now known internaly only
 * added OSS_IrqHdlCreate/OSS_IrqHdlRemove
 *
 * Revision 1.4  2000/03/17 14:39:00  kp
 * file description changed
 *
 * Revision 1.3  2000/03/16 16:05:28  kp
 * added OSS_SetIrqNum0()
 *
 * Revision 1.2  1999/08/30 11:03:34  Franke
 * moved OSS_IrqLevelToVector() from oss.c and filled with code
 *
 * Revision 1.1  1999/05/05 11:11:19  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSidIrq[]="$Id: oss_irq.c,v 1.20 2012/03/02 18:49:23 ts Exp $ - "
#ifdef _UP_COMPATIBLE_ONLY
	"UP";
#else
	#ifdef _SMP_COMPATIBLE_ONLY
		"SMP";
	#else
		"UP";
	#endif
#endif


#define _OSS_COMPILE

#include <MEN/men_typs.h>

#ifdef _SMP_COMPATIBLE_ONLY
	#include <vxWorks.h>
	#include <spinLockLib.h>
	#ifndef _UP_COMPATIBLE_ONLY
		IMPORT BOOL spinLockIsrHeld(spinlockIsr_t *pLock);
	#endif
	#include <vxCpuLib.h>
#else
	#ifdef _UP_COMPATIBLE_ONLY
	#else
		/* info about default mode */
		#warning "INFO: _UP_COMPATIBLE_ONLY"
		#define _UP_COMPATIBLE_ONLY
	#endif
#endif

#include <MEN/oss.h>
#include <MEN/mdis_err.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#include <intLib.h>
#include <logLib.h>
#include <sysLib.h>

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
static int32 OSS_IrqNum0 = -1;  /* uninitialized, must be set to a
					        architecture depend value e.g. 0x20 for PC's */

#ifndef _SMP_COMPATIBLE_ONLY
	u_int32 G_nestedCounter = 0;
	static int     G_lockOutKey = 0;   /* store Key to unlock         */
#else
	atomic_t G_nestedCounter = 0;
    static spinlockIsr_t G_spinlock;
    static spinlockIsr_t *G_spinlockHandle = NULL;
#endif

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/********************************* OSS_SetIrqNum0 ****************************
 *
 *  Description: Tell OSS the value of INT_NUM_IRQ0 define (vxWorks specific)
 *
 *			   	 normally 0x20 for PCs 0x0 for other architectures
 *---------------------------------------------------------------------------
 *  Input......: intNumIrq0		value of INT_NUM_IRQ0
 *  Output.....: -
 *  Globals....: OSS_IrqNum0
 ****************************************************************************/
void OSS_SetIrqNum0( int intNumIrq0 )
{
	if( OSS_IrqNum0 == -1 )
	{
		#ifndef _UP_COMPATIBLE_ONLY
			DBGWRT_ERR((DBH,"INFO: MDIS/OSS _SMP_COMPATIBLE_ONLY #CPU's %d\n", vxCpuConfiguredGet()));
		#else
			DBGWRT_ERR((DBH,"INFO: MDIS/OSS _UP_COMPATIBLE_ONLY\n"));
		#endif /*_SMP_COMPATIBLE_ONLY*/

	}

	OSS_IrqNum0 = intNumIrq0;
}

/********************************* OSS_GetIrqNum0 ****************************
 *
 *  Description: Get the OSS irq value (vxWorks specific), which was set before
 *               with the function OSS_SetIrqNum0.
 *
 *			   	 normally 0x20 for PCs 0x0 for other architectures
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: OSS_IrqNum0
 ****************************************************************************/
int OSS_GetIrqNum0( void )
{
	return OSS_IrqNum0;
}

/********************************** OSS_IrqHdlCreate **************************
 *
 *  Description:
 *---------------------------------------------------------------------------
 *  Input......:
 *
 *
 *  Output.....:
 *
 *  Globals....:
 ****************************************************************************/
int32 OSS_IrqHdlCreate
(
    int32			intNbr,
    int32			level,
    OSS_IRQ_HANDLE	**irqHdlP
)
{
    DBGCMD( static char functionName[] = "OSS_IrqHdlCreate"; )
    OSS_IRQ_HANDLE	*irqHandle	= NULL;
    int32			retCode		= 0;
    u_int32         gotsize;

	*irqHdlP = NULL;

	irqHandle = (OSS_IRQ_HANDLE*) OSS_MemGet( OSS_VXWORKS_OS_HDL,
											  sizeof(OSS_IRQ_HANDLE),
											  &gotsize );
    if( irqHandle == NULL )
	{
		DBGWRT_ERR((DBH,"*** %s: Can't alloc irqHandle", functionName ));
        retCode = ERR_OSS_MEM_ALLOC;
        goto CLEANUP;
	}
    OSS_MemFill( OSS_VXWORKS_OS_HDL, gotsize, (char*) irqHandle, 0 );

    irqHandle->ownMemSize 	= gotsize;
    irqHandle->magic    	= OSS_VXWORKS_IRQ_HDL_MAGIC;
    irqHandle->vector    	= intNbr;
    irqHandle->level     	= level;

	DBGWRT_2((DBH,"%s: intNbr=0x%x, level=0x%x, irqHandle=0x%x\n",
		functionName,intNbr,level,irqHandle ));

  #ifdef _SMP_COMPATIBLE_ONLY
	if( G_spinlockHandle == NULL )
	{
		G_spinlockHandle = &G_spinlock;
		spinLockIsrInit( G_spinlockHandle, 0 /* VxWorks 6.6 - placeholder for future enhancements */ );
	}
  #endif /*_SMP_COMPATIBLE_ONLY*/

	*irqHdlP = irqHandle;
CLEANUP:
	return( retCode );
}/*OSS_IrqHdlCreate*/

/********************************* OSS_IrqHdlRemove *************************
 *
 *  Description:
 *---------------------------------------------------------------------------
 *  Input......:
 *  Output.....:
 *  Globals....:
 ****************************************************************************/
int32 OSS_IrqHdlRemove
(
    OSS_IRQ_HANDLE	**irqHdlP
)
{
    OSS_IRQ_HANDLE *irqHandle=*irqHdlP;

	DBGWRT_2((DBH,"OSS_IrqHdlRemove: irqHandle=0x%x\n",irqHandle ));

    *irqHdlP = NULL;
    OSS_MemFree( NULL, (int8*) irqHandle, (irqHandle)->ownMemSize );
    irqHandle = NULL;

	return( 0 );
}/*OSS_IrqHdlRemove*/

/********************************* OSS_IrqHdlGetLevel *************************
 *
 *  Description:
 *---------------------------------------------------------------------------
 *  Input......:
 *  Output.....:
 *  Globals....:
 ****************************************************************************/
int32 OSS_IrqHdlGetLevel
(
    OSS_IRQ_HANDLE	*irqHdl
)
{
	return( irqHdl->level );
}/*OSS_IrqHdlRemove*/

/********************************* OSS_IrqHdlGetVector *************************
 *
 *  Description:
 *---------------------------------------------------------------------------
 *  Input......:
 *  Output.....:
 *  Globals....:
 ****************************************************************************/
int32 OSS_IrqHdlGetVector
(
    OSS_IRQ_HANDLE	*irqHdl
)
{
	return( irqHdl->vector );
}/*OSS_IrqHdlGetVector*/


/**********************************************************************/
/** Mask device interrupts.
 * \copydoc oss_specification.c::OSS_IrqMaskR()
 *
 * \sa OSS_IrqRestore()
 */
OSS_IRQ_STATE OSS_IrqMaskR
(
	OSS_HANDLE *ossHdl,
	OSS_IRQ_HANDLE* irqHandle
)
{

	OSS_IrqMask( ossHdl, irqHandle );

	return( (OSS_IRQ_STATE)0xFFFFFFFF );
}

/**********************************************************************/
/** Unmask device interrupts.
 * \copydoc oss_specification.c::OSS_IrqRestore()
 *
 * \sa OSS_IrqMaskR()
 */
void OSS_IrqRestore
(
	OSS_HANDLE *ossHdl,
	OSS_IRQ_HANDLE* irqHandle,
	OSS_IRQ_STATE oldState
)
{

	if( (u_int32)oldState != 0xFFFFFFFF )
		logMsg("*** OSS_IrqRestore\n", 0,0,0,0,0,0 );

	OSS_IrqUnMask( ossHdl, irqHandle );

}



/************************* OSS_IrqMask **************************************
 *
 *  Description:  Masks device interrupt.
 *
 *  NOTE for VxWorks:
 *                System routines should not called after lock interrupts!
 *                Task will locked and rescheduling will disabled!
 *
 *                DBG_X Messages should be done only if not nested,
 *                because OSS_IrqUn/Mask() will be called at DBG_X too.
 *                ( avoid recusive calling )
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                irqHandle  pointer to the os specific irq structure
 *
 *  Output.....:  ---
 *
 *  Globals....:  G_nestedCounter
 *
 ****************************************************************************/
void OSS_IrqMask
(
    OSS_HANDLE *osHdl,
    OSS_IRQ_HANDLE* irqHandle
)
{
    DBGCMD( static const char *functionName = __FUNCTION__; )
#ifdef _SMP_COMPATIBLE_ONLY
    int fired = 0;
#endif

	DBGWRT_2((DBH,"%s: irqHandle=0x%x\n",functionName,irqHandle ));

	if( irqHandle->magic != OSS_VXWORKS_IRQ_HDL_MAGIC )
	{
		for(;;)
		{
			logMsg("*** OSS_IrqMask(0x%x): OSS_IrqHdlCreate() must be called\n", (int)irqHandle,0,0,0,0,0 );
			taskDelay( sysClkRateGet() * 4 );
		}/*for*/
	}/*if*/

#ifndef _SMP_COMPATIBLE_ONLY

	/* pre VxWorks 6.6 part */
	if( G_nestedCounter == 0 )
	{
	    if( intContext() )
		{
    	    G_lockOutKey = intLock();     /* disable irqs */
		}
		else
		{
        	if( taskLock() == OK )   /* disable rescheduling */
	        {
        	    G_lockOutKey = intLock();     /* disable irqs */
	        }
    	    else
        	{
            	logMsg( "*** %s: taskLock() line %d ***\n",  (int)__FUNCTION__, __LINE__, 3,4,5,6 );
				goto CLEANUP;
	        }/*if*/
	    }/*if*/
	}

	G_nestedCounter++;
	if( G_nestedCounter == 1 )
	{
    	DBGWRT_2((DBH,"%s() irq locked\n", functionName ));
    }

#else

	/* SMP compatible part */
	if( G_spinlockHandle == NULL )
	{
		logMsg("*** %s(): G_spinlockHandle %d\n", (int)__FUNCTION__, (int)G_spinlockHandle,3,4,5,6 );
		goto CLEANUP;
	}

	if( !spinLockIsrHeld( G_spinlockHandle ) )
	{
		spinLockIsrTake( G_spinlockHandle );
		if( G_nestedCounter )
			logMsg("*** %s G_nestedCounter %d by CPU# %d\n", (int)__FUNCTION__, G_nestedCounter, vxCpuIndexGet(), 4,5,6 );

		fired = 1;
	}
	G_nestedCounter++;

	if( fired )
	{
		DBGWRT_3((DBH,"%s: spinlock taken G_nestedCounter %d by CPU# %d\n", functionName, G_nestedCounter, vxCpuIndexGet() )); /* disable if problems with FRAM module */
	}

#endif /* _SMP_COMPATIBLE_ONLY */

CLEANUP:
	return;
}/*OSS_IrqMask*/

/************************* OSS_IrqUnMask *************************************
 *
 *  Description:  Unmasks previously masked device interrupt.
 *
 *  NOTE for VxWorks:
 *                Task will unlock and rescheduling will enabled!
 *
 *                DBG_X Messages should be done only if not nested,
 *                because OSS_IrqUn/Mask() will be called at DBG_X too.
 *                ( avoid recusive calling )
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                irqHandle  pointer to the os specific irq structure
 *
 *  Output.....:  ---
 *
 *  Globals....:  G_nestedCounter
 *
 ****************************************************************************/
void OSS_IrqUnMask
(
    OSS_HANDLE *osHdl,
    OSS_IRQ_HANDLE *irqHandle
)
{
    DBGCMD( static const char *functionName = __FUNCTION__; )

	DBGWRT_2((DBH,"%s: irqHandle=0x%x\n",functionName,irqHandle ));

#ifndef _SMP_COMPATIBLE_ONLY
	if( G_nestedCounter == 1 )
	{
    	DBGWRT_3((DBH,"%s() irq unlocked \n", functionName ));
    }

	G_nestedCounter--;
	if( G_nestedCounter == 0 )
	{
	    intUnlock( G_lockOutKey ); /* enable all masked irqs */

	    if( !intContext() )
		{
        	taskUnlock();   /* enable rescheduling */
		}/*if*/
		goto CLEANUP;
	}

#else
	/* SMP compatible part */
	if( G_nestedCounter <= 0 )
	{
		/* programmer's error */
		DBGWRT_ERR((DBH,"%s%s: G_nestedCounter %d\n", OSS_ErrorStartStr, functionName, G_nestedCounter ));
		goto CLEANUP;
	}

	if( G_spinlockHandle == NULL )
	{
		for(;;)
		{
			goto CLEANUP;
		}/*for*/
	}

	if( G_nestedCounter == 1 )
	{
		DBGWRT_3((DBH,"%s: spinlock released by CPU# %d\n", functionName, vxCpuIndexGet() )); /* disable if problems with FRAM module */
   	}

	G_nestedCounter--;

	if( G_nestedCounter == 0 )
	{
		#ifdef _UP_COMPATIBLE_ONLY
			spinLockIsrGive( G_spinlockHandle );
		#else
			if( spinLockIsrHeld( G_spinlockHandle ) )
			{
				spinLockIsrGive( G_spinlockHandle );
			}
			else
			{
				logMsg("*** OSS_IrqUnMask: G_nestedCounter %d spinlock not held on CPU# %d\n",
						G_nestedCounter, vxCpuIndexGet(), 3, 4,5,6 );
			}
		#endif
	}
#endif /* _SMP_COMPATIBLE_ONLY */

CLEANUP:
	return;
}/*OSS_IrqUnMask*/

/************************* OSS_IrqLevelToVector *****************************
 *
 *  Description:  Compute interrupt vector from interrupt number and bus type.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl		os handle
 *                busType	bus type
 *                irqNbr	interrupt number  0..15
 *
 *  Output.....:  vector	interrupt vector  e.g. irqNbr + OSS_IrqNum0
 *                return	0 | error code
 *
 *  Globals....:  OSS_IrqNum0 architecture depend e.g. 0x20 for PC
 ****************************************************************************/
int32 OSS_IrqLevelToVector
(
	OSS_HANDLE *osHdl,
	int32      busType,
	int32      irqNbr,
	int32      *vector
)
{
    DBGCMD( static const char functionName[] = "OSS_IrqLevelToVector"; )

	DBGWRT_2((DBH,"%s\n", functionName));

	if( busType == OSS_BUSTYPE_PCI
		|| busType == OSS_BUSTYPE_ISA
		|| busType == OSS_BUSTYPE_ISAPNP
	  )
	{
		if( OSS_IrqNum0 == -1 )
		{
	        DBGWRT_ERR( ( DBH, "%s%s: OSS_IrqNum0 not initialized %s%d%s\n",
    	                  OSS_ErrorStartStr, functionName,
            	          OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			return( ERR_OSS );
		}/*if*/
		*vector = irqNbr + OSS_IrqNum0;
	}
	else
	{
        DBGWRT_ERR( ( DBH, "%s%s: busType %d not supported %s%d%s\n",
                      OSS_ErrorStartStr, functionName,
                      busType,
                      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		return( ERR_OSS_ILL_PARAM );
	}/*if*/

	return( 0 );
}/*OSS_IrqLevelToVector*/



/************************* OSS_MsiVectorMap *****************************
 *
 *  Description:  Derive the actual MSI vector from requested MSI vector,
 *		  according to the actually by OS allowed number of MSI vectors .
 *
 *---------------------------------------------------------------------------
 *  Input......:  mmc		MSI capability, how many MSIs a PCIe device can
 *                mme		Enabled MSI by OS
 *                reqVect	reqeusted Vect
 *
 *  Output.....:  pVect		pointer to save actual MSI vector used by OS for this IRQ
 *                return	0 | error code
 *
 ****************************************************************************/
int32 OSS_MsiVectorMap
(
	int32      mmc,
	int32      mme,
	int32      reqVect,
	int32      *pVect
)
{
	int32 	maxVect;

	DBGCMD( static const char functionName[] = "OSS_MsiVectorMap");

	DBGWRT_2((DBH,"%s\n", functionName));

	maxVect = (1 << mmc )- 1;/*max. possible MSI vector from the device*/

	if(reqVect > maxVect){
		DBGWRT_ERR( ( DBH, "%s%s: Requested irq vector(%d) is beyond the"
							"MSI capability(%d) of this PCIe device\n",
    	            OSS_ErrorStartStr, functionName,
            	    reqVect, maxVect));

		return ERR_OSS_PCIE_ILL_MSI;
	}

	*pVect =  reqVect >> (mmc - mme);

	return( 0 );
} /* OSS_MsiVectorMap*/





