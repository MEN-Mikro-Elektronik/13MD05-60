/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  dbg_rtp.c
 *
 *      \author  uf
 *        $Date: 2006/06/02 11:13:15 $
 *    $Revision: 1.1 $
 * 
 *	   \project  MDIS OSS for VxWorks
 *  	 \brief  Signal routines
 *      
 *    \switches  -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: dbg_rtp.c,v $
 * Revision 1.1  2006/06/02 11:13:15  ufranke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static const char RCSid[]="$Id: dbg_rtp.c,v 1.1 2006/06/02 11:13:15 ufranke Exp $";

#include <vxWorks.h>
#include <MEN/men_typs.h>
#include <MEN/dbg_vx_rtp.h>
#include <MEN/dbg.h>


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
/**********************************************************************/
/** Initializes the debug output.
 *  
 * \param  name	  not used
 * \param  dbgP   debug handle
 *
 * \return 0 always
 */
int32 DBG_Init
(
    char *name,
    DBG_HANDLE **dbgP
)
{
	*dbgP = NULL;
	return( 0 );
}

/**********************************************************************/
/** Deinitializes the debug output.
 *  
 * \param  dbgP   debug handle
 *
 * \return 0 always
 */
int32  DBG_Exit
(
    DBG_HANDLE **dbgP
)
{
	*dbgP = NULL;
	return( 0 );
}

