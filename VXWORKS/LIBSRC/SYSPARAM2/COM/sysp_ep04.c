/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_ep04.c
 *
 *      \author  rt
 *        $Date: 2009/11/30 11:46:02 $
 *    $Revision: 2.5 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  EP04 specific part
 *               (cloned from sysp_em03.c V2.9)
 *
 *    \switches
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_ep04.c,v $
 * Revision 2.5  2009/11/30 11:46:02  rt
 * R: 1) Not compilable under Linux.
 * M: 1) Removed config.h include.
 *
 * Revision 2.4  2008/11/04 16:05:11  rt
 * R:1. EE-STARTUP parameter is not available if EE-USEFLPAR is set to 1
 * M:1. Added "startup" alias for "mmstartup"
 *
 * Revision 2.3  2007/03/08 11:19:54  cs
 * include ep04_cfg.h
 * cosmetics
 *
 * Revision 2.2  2007/02/28 18:54:06  rt
 * removed:
 * - SYSP_nspeed2
 *
 * Revision 2.1  2007/02/28 14:53:50  rt
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/ep04_eeprom.h>
#include <MEN/ep04_cfg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* variant specific defaults */

#define EP4_DFLT_MMSTARTUP	""


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEp04CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EP04
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Ep04Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEp04CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */
#if 0
	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, 0 );
	if( rv == SYSPARAM_EEPROM_ERROR )
		rv = 0;					/* non fatal */
#endif

#if defined(SYSP_CONFIG_MENMON)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );
#endif

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for EP04
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int Ep04SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Ep04Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EP04 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEp04Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL3( "lxdcache", 	NULL, _EEOFF(mm_misc), "1", 0 ),
	SYSP_PDE_BOOL4( SYSP_useflpar, 	NULL, _EEOFF(mm_misc), "0",
					SYSP_PD_RESTART ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),

	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL2( "stdis_nand",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

	/*SYSP_PDE_U8X( "pciopts", 		NULL, _EEOFF(mm_pci_opts), "0", 0),???*/

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX(   _EEOFF(mm_usr[0]) ),

	SYSP_PDE_CON0( _EEOFF(mm_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "a" ),  	/* TOUCH */
	SYSP_PDE_CON2( _EEOFF(mm_con[2]), "0" ),	/* (none) */
	SYSP_PDE_CON3( _EEOFF(mm_con[3]), "0" ),	/* (none) */
	SYSP_PDE_GCON( _EEOFF(mm_gcon), 	"ff"),	/* use first available */
	SYSP_PDE_VMODE( _EEOFF(mm_vmode) ),
	SYSP_PDE_ECL(   _EEOFF(mm_ecl) ),
	SYSP_PDE_TTO(   _EEOFF(mm_tto) ),
	SYSP_PDE_TRIES( _EEOFF(mm_tries) ),
	SYSP_PDE_TDP(   _EEOFF(mm_tdp) ),
	SYSP_PDE_HDP(   _EEOFF(mm_hdp) ),
	SYSP_PDE_STWAIT(_EEOFF(mm_stwait), "30" ),
	SYSP_PDE_WDT(   _EEOFF(mm_wdt), "0" ),

	/* nspeed0/1 hardcoded to 1000 (see SYSP_parListEp04Mmpara) */
	/*SYSP_PDE_NSPEED( SYSP_nspeed2, NULL, _EEOFF(mm_nspeed[2]), "AUTO" ),*/

	SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in EP04 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListEp04FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, SYSP_startup, _EEOFFF(fl_startup),
				  EP4_DFLT_MMSTARTUP,
				  EP04_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "",
				  EP04_FLASH_KERPAR_LEN, 0 ),

	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEp04Mmpara[] = {
	SYSP_mem0,
	SYSP_flash0,
	SYSP_cpu,
	SYSP_cpuclkhz,
	SYSP_memclkhz,
	SYSP_pciclkhz,
	SYSP_MPC85XX_ccbclkhz,
	SYSP_MPC85XX_immr,
	SYSP_clun,
	SYSP_dlun,
	SYSP_rststat,
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	SYSP_nmac0,
	SYSP_nmac1,
	SYSP_nspeed0,	/* fix for SERDES (=1000) */
	SYSP_nspeed1,	/* fix for SERDES (=1000) */
	NULL
};

/**********************************************************************/
/** Probe for EP04 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EP04
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEp04CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe, *nvsFl;
	SYSP_NVS_ADR nvsAdr;
	int useFlashStrings;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = EP04_SMB_SROM_MM_BUS;
	nvsAdr.addr.smb.dev = EP04_SMB_SROM_MM;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(EP04_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEp04Mmpara,
								xSuper->initBlk.mmParaStringMaxLen,
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EP04_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEp04Menm, EP04_EEID_MENM,
								 NULL )) != 0 )
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );

	useFlashStrings = SysParamBool( SYSP_useflpar, FALSE );

	if( useFlashStrings == FALSE ){
		/* startup string */
		if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
									 offsetof( EP04_EEPROM, startup ),
									 EP04_EEPROM_STARTUP_LEN,
									 EP4_DFLT_MMSTARTUP)) != 0 )
			return rv;
	}

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EP04_EEPROM, vxBline ),
								EP04_VXBLINE_LEN,
								xSuper->initBlk.vxBlineStringAdrs,
								"mottsec(0,0):", !useFlashStrings )))
		return rv;


	/* boot flash section */
	if( useFlashStrings ){
		/* create FLASH NVS section */
		nvsAdr.type = SYSP_NVSADR_SMB; /* well, not really */
		nvsAdr.addr.smb.bus = SYSPARAM_SMB_BUS_FLASH;
		nvsAdr.addr.smb.dev = 0xff; /* last sector */

		if( (nvsFl = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpufl",
									 &xSuper->s, "", nvsAdr,
									 EP04CFG_BFLASH_SYSP_SIZE )) == NULL )
			return SYSPARAM_NO_RESOURCE;

		if( (rv = SYSP_MpGrpCreateStd(
				 nvsFl, "flstr",
				 offsetof( EP04_BFLASH_PARMS, flstr ),
				 sizeof( struct flstr_parms ),
				 SYSP_parListEp04FlStr, EP04_FLID_STR,
				 NULL )) != 0 )
			return rv;

		SYSP_NvsAdd( &xSuper->s, nvsFl );
	}

	return 0;
}

