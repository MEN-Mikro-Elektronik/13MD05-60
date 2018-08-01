/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_em03.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/03/22 11:09:30 $
 *    $Revision: 2.10 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  EM03 specific part
 *      
 *    \switches  SYSP_EM3_VARIANT_ALCA - alcatel EM3A specific defaults
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em03.c,v $
 * Revision 2.10  2007/03/22 11:09:30  ufranke
 * cosmetics
 *  - used common SYSP_stdis_nand
 *
 * Revision 2.9  2006/08/18 09:18:23  ub
 * Fixed typo in definition of ee-stwait
 *
 * Revision 2.8  2006/07/06 10:24:59  ufranke
 * cosmetics
 *
 * Revision 2.7  2005/09/05 11:43:25  kp
 * Added alcatel variant SYSP_EM3_VARIANT_ALCA
 *
 * Revision 2.6  2005/07/27 09:15:14  kp
 * changed default for SYSP_stignfault to 1.
 * stdis_sram -> stdis_fram
 *
 * Revision 2.5  2005/07/26 16:16:21  kp
 * + lxdcache
 *
 * Revision 2.4  2005/06/23 15:54:18  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.3  2005/05/25 09:30:30  kp
 * + psrXXX parameters, changed default vxWorks bootline
 *
 * Revision 2.2  2005/05/06 10:31:45  kp
 * cosmetic
 *
 * Revision 2.1  2005/04/19 10:01:21  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/em03_eeprom.h>
#include <MEN/em03_cfg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* variant specific defaults */

#define EM3_DFLT_WDT		"0"
#define EM3_DFLT_MMSTARTUP	""

#ifdef SYSP_EM3_VARIANT_ALCA
# undef  EM3_DFLT_WDT
# define EM3_DFLT_WDT		"600"
# undef  EM3_DFLT_MMSTARTUP
# define EM3_DFLT_MMSTARTUP "NBOOT KERPAR='ip=bootp rw root=/dev/ram0 ramdisk_size=300000'"
#endif /* SYSP_EM3_VARIANT_ALCA */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEm03CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM03
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Em03Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm03CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, 0 );
	if( rv == SYSPARAM_EEPROM_ERROR )
		rv = 0;					/* non fatal */

#if defined(SYSP_CONFIG_MENMON)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );
#endif

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv; 
}

/**********************************************************************/
/** Initialize SYSPARAM for EM03
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int EM03_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;
	
	xSuper->initBlk = *initBlk;

	super->reInit = Em03Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EM03 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEm03Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL3( "lxdcache", 	NULL, _EEOFF(mm_misc), "1", 0 ),
	SYSP_PDE_BOOL4( SYSP_useflpar, 	NULL, _EEOFF(mm_misc), "0", 
					SYSP_PD_RESTART ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL7( "noecc", 		NULL, _EEOFF(mm_misc), "0", 0 ),
	
	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "1", 0 ),
	SYSP_PDE_BOOL2( SYSP_stdis_nand,   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_fram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
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
	SYSP_PDE_WDT(   _EEOFF(mm_wdt), EM3_DFLT_WDT ),

	SYSP_PDE_U8D( "eccsth", 		NULL, _EEOFF(mm_eccSingleTh), "32", 
				  (SYSP_PD_MMPARASTR*0)),
	SYSP_PDE_NSPEED( SYSP_nspeed0, NULL, _EEOFF(mm_nspeed[0]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed1, NULL, _EEOFF(mm_nspeed[1]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed2, NULL, _EEOFF(mm_nspeed[2]), "AUTO" ),

	SYSP_PDE_BOOL0( "psr2v5",   NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL2( "psr3v3",   NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL3( "psr5v",   	NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL4( "psr1v5",  	NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL5( "psr1v0",  	NULL, _EEOFF(mm_powerReset), "1", 0 ),

	SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in EM03 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListEm03FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, NULL, _EEOFFF(fl_startup), 
				  EM3_DFLT_MMSTARTUP, 
				  EM03_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "", 
				  EM03_FLASH_KERPAR_LEN, 0 ),
	
	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEm03Mmpara[] = {
	SYSP_mem0,
	SYSP_mem1,
	SYSP_flash0,
	SYSP_cpu,
	SYSP_cpuclkhz,
	SYSP_memclkhz,
	SYSP_pciclkhz,
	SYSP_MPC85XX_ccbclkhz,
	SYSP_MPC85XX_brgclkhz,
	SYSP_MPC85XX_immr,
	SYSP_clun,
	SYSP_dlun,
	SYSP_rststat,	
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	SYSP_nmac0,
	SYSP_nmac1,
	SYSP_nmac2,
	NULL
};

/**********************************************************************/
/** Probe for EM03 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EM03 
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEm03CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe, *nvsFl;
	SYSP_NVS_ADR nvsAdr;
	int useFlashStrings;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = 0xa8;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr, 
								 sizeof(EM03_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEm03Mmpara,
								xSuper->initBlk.mmParaStringMaxLen, 
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EM03_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEm03Menm, EM03_EEID_MENM, 
								 NULL )) != 0 )
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );

	useFlashStrings = SysParamBool( SYSP_useflpar, FALSE );
		
	if( useFlashStrings == FALSE ){
		/* startup string */
		if( (rv = SYSP_StrGrpCreate( nvsEe, "mmstartup", "startup",
									 offsetof( EM03_EEPROM, startup ),
									 EM03_EEPROM_STARTUP_LEN, 
									 EM3_DFLT_MMSTARTUP)) != 0 )
			return rv;
	}
		
	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EM03_EEPROM, vxBline ),
								EM03_VXBLINE_LEN, 
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
									 EM03CFG_BFLASH_SYSP_SIZE )) == NULL )
			return SYSPARAM_NO_RESOURCE;

		if( (rv = SYSP_MpGrpCreateStd( 
				 nvsFl, "flstr", 
				 offsetof( EM03_BFLASH_PARMS, flstr ),
				 sizeof( struct flstr_parms ),
				 SYSP_parListEm03FlStr, EM03_FLID_STR, 
				 NULL )) != 0 )
			return rv;

		SYSP_NvsAdd( &xSuper->s, nvsFl );
	}

	return 0;
}

