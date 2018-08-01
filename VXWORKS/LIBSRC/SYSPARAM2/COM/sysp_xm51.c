/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_xm51.c
 *
 *      \author  mko
 *        $Date: 2012/09/07 09:39:37 $
 *    $Revision: 2.3 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  XM51 specific part
 *
 *    \switches  
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_xm51.c,v $
 * Revision 2.3  2012/09/07 09:39:37  MKolpak
 * R: API changed
 * M: Removed parameter
 *
 * Revision 2.2  2012/09/06 14:15:29  MKolpak
 * M: Initial version
 *
 * Revision 2.1  2012/04/13 11:27:05  MKolpak
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2007 by MEN Mikro Elektronik GmbH, NurHberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/xm51_eeprom.h>
#include <MEN/xm51_cfg.h>

#define SYSP_DATA_SIZE	1100

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 			/**< SYSPARAM super object  */
static u_int32 G_alloc[SYSP_DATA_SIZE];		/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

#if 0
#define LOGLINE   printf(" %s  %d\n", __FUNCTION__, __LINE__)
#else
#define LOGLINE {}
#endif


/**********************************************************************/
/** Probe for XM51 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in XM51
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsXm51CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = SYS_IDPROM_CPU_BUSNO; 
	nvsAdr.addr.smb.dev = SYS_IDPROM_CPU_SMB_ADDR;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(XM51_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvsEe );

	return 0;
}

#ifdef AE52
/**********************************************************************/
/** Probe for AE52 EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in XM51
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsAe52CarrProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = SYS_IDPROM_CARR_BUSNO; 
	nvsAdr.addr.smb.dev = SYS_IDPROM_CARR_SMB_ADDR;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "caree",
								 &xSuper->s, "", nvsAdr,
								 sizeof(XM51_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Carrier EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvsEe );

	return 0;
}


#endif /* AE52 */

/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for XM51
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int XM51Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );
	LOGLINE;

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsXm51CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */
#ifdef AE52    
    /* probe Carrier EEPROM */
	if( (rv = SYSP_NvsAe52CarrProbe( xSuper )) != 0 )
		return rv;				/* fatal */
#endif    

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for XM51, just creates carrier data
 *
 * \param initBlk 	\IN SYSPARAM initialisation structure for
 *						MENMON/SMB implementations. Caller can destroy
 *						\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int XM51_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = XM51Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}


/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)



