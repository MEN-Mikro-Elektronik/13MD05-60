/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_sharedmem.c
 *      Project: User OSS lib
 *
 *       Author: see
 *        $Date: 2010/10/27 10:23:11 $
 *    $Revision: 1.2 $
 *
 *  Description: USER OSS SHARED MEM - Routines to manage shared memory
 *               It is a part of the USR_OSS library.
 *
 *               Shared memory areas are used as a fast way to exchange
 *               data between application and driver.
 *
 *               A shared memory area can be installed from the application or
 *               from the driver itself.
 *
 *               Several applications may have access to the same shared
 *               memory area by linking to an already existing area.
 *
 *               Shared memory areas are identified via a number, the
 *               so-called "shared memory area index" (smNr).
 *
 *               The INIT function creates a global shared mem handle for all
 *               subsequent calls.
 *
 *               Synchronization between application and driver access to
 *               the shared memory is not handled by this functions.
 *
 *               The number of useable shared memory areas is not limited.
 *
 *               Typical usage:
 *
 *                    Installation:
 *                    	UOS_SharedMemInit()
 *                    	UOS_SharedMemSet/Link()
 *                    De-Installation:
 *                    	UOS_SharedMemClear()
 *                    	UOS_SharedMemExit()
 *
 *               Corresponding with OSS driver shared memory functions.
 *
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_sharedmem.c,v $
 * Revision 1.2  2010/10/27 10:23:11  cs
 * R: adapt prototype of UOS_SharedMemInit to new definition in usr_oss.h
 * M: use INT32_OR_64 for MDIS path
 *
 * Revision 1.1  1999/08/31 10:53:49  Franke
 * Initial Revision
 *
 * cloned from shmem.c Revision 1.2  1999/02/15 14:08:47  see
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

const char UOS_SHMEM_RCSid[]="$Id: usr_oss_sharedmem.c,v 1.2 2010/10/27 10:23:11 cs Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"

/********************************* UOS_SharedMemInit *************************
 *
 *  Description: Create shared memory handle
 *
 *	             This function has to be called before any shared memory
 *               area can be created or linked.
 *
 *               Possible errors:
 *               ERR_UOS_MEM_ALLOC       no free memory to create handle
 *
 *---------------------------------------------------------------------------
 *  Input......: path        path number
 *               smHdlP		 pointer to variable where shared memory handle
 *                           will be stored
 *  Output.....: *smHdlP	 shared memory handle | NULL
 *               return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/

int32 UOS_SharedMemInit(
	INT32_OR_64 path,
	UOS_SHMEM_HANDLE **smHdlP
)
{
	/* klocwork id27333 - 27335 */
	return(ERR_UOS_NOT_INSTALLED);
}

/********************************* UOS_SharedMemExit *************************
 *
 *  Description: Remove shared memory handle
 *
 *               The function removes the shared memory handle.
 *
 *---------------------------------------------------------------------------
 *  Input......: smHdlP		 pointer to shared memory handle
 *  Output.....: return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/

int32 UOS_SharedMemExit(
	UOS_SHMEM_HANDLE **smHdlP
)
{
	/* klocwork id27333 - 27335 */
	return(ERR_UOS_NOT_INSTALLED);
}

/********************************* UOS_SharedMemSet *************************
 *
 *  Description: Create shared memory area
 *
 *               The function creates the specified shared memory area "smNr"
 *               via M_LL_BLK_SHMEM_SET setstat at the driver.
 *
 *               The shared memory size is defined by the application.
 *
 *               The shared memory's physical address is mapped to the
 *               application's address space and returned via "appAddrP".
 *
 *               Possible errors:
 *               ERR_UOS_MEM_ALLOC       no free memory to create handle
 *               ERR_UOS_SETSTAT         the above status call failed
 *               ERR_UOS_NO_PERMIT       can't map shared memory address
 *
 *               If error ERR_UOS_SETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: smHdl		 shared memory handle
 *               smNr        shared memory area index (0..n)
 *               size        shared memory area size [bytes]
 *  Output.....: appAddrP    pointer to shared memory area
 *               return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/

int32 UOS_SharedMemSet(
	UOS_SHMEM_HANDLE *smHdl,
	u_int32 smNr,
	u_int32 size,
	void **appAddrP
)
{
	/* klocwork id27333 - 27335 */
	return(ERR_UOS_NOT_INSTALLED);
}

/********************************* UOS_SharedMemLink ************************
 *
 *  Description: Link to exisiting shared memory area
 *
 *               The function links to the existing specified shared memory
 *               area "smNr" via M_LL_BLK_SHMEM_LINK setstat at the driver.
 *
 *               The shared memory's physical address is mapped to the
 *               application's address space and returned via "appAddrP".
 *               The shared memory areas size is returned via "sizeP".
 *
 *               Possible errors:
 *               ERR_UOS_MEM_ALLOC       no free memory to create handle
 *               ERR_UOS_SETSTAT         the above status call failed
 *               ERR_UOS_NO_PERMIT       can't map shared memory address
 *
 *               If error ERR_UOS_SETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: smHdl		 shared memory handle
 *               smNr        shared memory area index (0..n)
 *  Output.....: sizeP       shared memory area size [bytes]
 *               appAddrP    pointer to shared memory area
 *               return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 UOS_SharedMemLink
(
	UOS_SHMEM_HANDLE *smHdl,
	u_int32 smNr,
	u_int32 *sizeP,
	void **appAddrP
)
{
	/* klocwork id27333 - 27335 */
	return(ERR_UOS_NOT_INSTALLED);

}/*UOS_SharedMemLink*/

/********************************* UOS_SharedMemClear *************************
 *
 *  Description: Unlink/Remove shared memory area
 *
 *               The function unlinks from the specified shared memory area
 *               via M_LL_BLK_SHMEM_CLEAR setstat at the driver.
 *
 *               If the last link has been removed, the driver removes and
 *               deallocates the shared memory area.
 *
 *               Possible errors:
 *               ERR_UOS_SETSTAT         the above status call failed
 *
 *               If error ERR_UOS_SETSTAT is returned, the caused driver
 *               error message (MDIS) can be queried via M_errstring().
 *
 *---------------------------------------------------------------------------
 *  Input......: smHdl		 shared memory handle
 *               smNr        shared memory area index (0..n)
 *  Output.....: return		 success (0) or error code
 *  Globals....: -
 ****************************************************************************/
int32 UOS_SharedMemClear
(
	UOS_SHMEM_HANDLE *smHdl,
	u_int32 smNr
)
{
	/* klocwork id27333 - 27335 */
	return(ERR_UOS_NOT_INSTALLED);

}/*UOS_SharedMemClear*/
