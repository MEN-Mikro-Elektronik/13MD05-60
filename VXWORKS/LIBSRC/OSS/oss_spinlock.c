/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  oss_spinlock.c
 *
 *      \author  thomas.schnuerer@men.de
 *        $Date: 2013/11/14 17:56:27 $
 *    $Revision: 1.5 $
 *
 *	   \project  vxWorks OSS lib
 *  	 \brief  Spinlock functions
 *
 *    \switches  DBG - enable debugging
 *
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_spinlock.c,v $
 * Revision 1.5  2013/11/14 17:56:27  ts
 * R: VxWorks spinlocks are available from version 6.6 on only
 * M: made VxWorks native spinlock functions depending on VxWorks Version
 *
 * Revision 1.4  2013/11/11 15:05:28  ts
 * R: spinLockLib was not used until now
 * M: use spinLockLib functions for acquiring/init of spinLocks
 *
 * Revision 1.3  2012/04/03 15:37:39  channoyer
 * R: Memory leak
 * M: Correction of parameter size of OSS_MemFree()
 *
 * Revision 1.2  2011/11/08 10:57:04  channoyer
 * R: Error with VxWorks 6.9
 * M: Replace comment style
 *
 * Revision 1.1  2011/08/15 18:56:21  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2011 MEN Mikro Elektronik GmbH. All rights reserved.
 ****************************************************************************/

/* #include "oss_intern.h" */

static const char RCSid[]="$Id: oss_spinlock.c,v 1.5 2013/11/14 17:56:27 ts Exp $";

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mdis_err.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#if ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6)) || (_WRS_VXWORKS_MAJOR >= 7)
#include <spinLockLib.h>
/* Please note that spinLockIsrTake/Give are defined in spinlockLib.h but spinLockIsrHeld() is defined in spinLockLibP.h */
#include <private/spinLockLibP.h>
#endif
#include "oss_intern.h"

/**********************************************************************/
/** Create a spin lock.
 *
 *  \copydoc oss_specification.c::OSS_SpinLockCreate()
 *
 *  \windows IRQL requirement: Any IRQL
 *
 *  \sa OSS_SpinLockRemove
 */

int32 OSS_SpinLockCreate(OSS_HANDLE *oss, OSS_SPINL_HANDLE **spinlP )
{

	/* init spinlock. WR 6.9 Documentation says: Currently no flags are defined, they are a placeholder for future enhancements */
	spinLockIsrInit( *spinlP, 0);
    return(OK);
}

/**********************************************************************/
/** Destroy spin lock handle.
 *
 *  \copydoc oss_specification.c::OSS_SpinLockRemove()
 *
 *  \windows Do nothing\n
 *  IRQL requirement: Any IRQL
 *
 *  \sa OSS_SpinLockCreate
 */
int32 OSS_SpinLockRemove( OSS_HANDLE *oss, OSS_SPINL_HANDLE **spinlP )
{
	/* ts: no function to actually delete a spinlock according to WR kernel developer guide */
	return OK;
}

/**********************************************************************/
/** Acquire spin lock.
 *
 *  \copydoc oss_specification.c::OSS_SpinLockAcquire()
 *
 *  \windows IRQL requirement: Any IRQL
 *
 *  \sa OSS_SpinLockRelease
 */
int32 OSS_SpinLockAcquire( OSS_HANDLE *oss, OSS_SPINL_HANDLE *spinl )
{

	/* invalid spin lock handle? */
    if( spinl == NULL ) {
    	DBGWRT_ERR( ( DBH, " *** OSS_SpinLockAcquire: invalid spin lock handle\n" ));
    	return ERR_OSS_ILL_HANDLE;
    }

	spinLockIsrTake( spinl );

    return(OK);
}

/**********************************************************************/
/** Release spin lock.
 *
 *  \copydoc oss_specification.c::OSS_SpinLockRelease()
 *
 *  \windows IRQL requirement: Any IRQL
 *
 *  \sa OSS_SpinLockAcquire
 */
int32 OSS_SpinLockRelease(
    OSS_HANDLE          *oss,
    OSS_SPINL_HANDLE    *spinl )
{

	/* invalid spin lock handle? */
    if( spinl == NULL ) {
    	DBGWRT_ERR( ( DBH, "*** OSS_SpinLockRelease: invalid spin lock handle\n" ));
    	return ERR_OSS_ILL_HANDLE;
    }
    spinLockIsrGive( spinl );
	return(OK);
}

