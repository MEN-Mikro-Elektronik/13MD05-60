/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_em10a.c
 *
 *      \author  ts
 *        $Date: 2012/01/11 17:52:27 $
 *    $Revision: 1.2 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  EM10A specific part, only needed for Touch calibration data
 *
 *    \switches  MM_CODE_SHRINK  - to bring MENMON < 256 kB
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em10a.c,v $
 * Revision 1.2  2012/01/11 17:52:27  ts
 * R: readout of brd, brdrev and other CPU parameters failed
 * M: fixed missing call to create CPU Nvs section
 *
 * Revision 1.1  2011/10/10 17:43:47  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/esm_cb_eeprom.h>
#include <MEN/em10a_eeprom.h>
#include <MEN/em10a_cfg.h>

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
/** Probe for EM01 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EM01
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEm10ACpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = EM10CFG_IIC_ONBOARD; 
	nvsAdr.addr.smb.dev = EM10A_SMB_SROM_EEPROD;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(EM10A_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvsEe );

	return 0;
}

/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM10A
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int EM10AInit( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );
	LOGLINE;

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm10ACpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super,  EM10CFG_IIC_IO );
	if( rv == SYSPARAM_EEPROM_ERROR ) {
		rv = 0;					/* non fatal */
		LOGLINE;
	}

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );
	LOGLINE;
	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for EM10, just creates carrier data
 *
 * \param initBlk 	\IN SYSPARAM initialisation structure for
 *						MENMON/SMB implementations. Caller can destroy
 *						\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int EM10A_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = EM10AInit;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}


/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)



