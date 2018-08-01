/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_em04.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2006/06/01 17:58:08 $
 *    $Revision: 2.8 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  EM04 specific part
 *      
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em04.c,v $
 * Revision 2.8  2006/06/01 17:58:08  cs
 * bugfix in parameter passing to SYP_PDE_STWAIT
 *
 * Revision 2.7  2006/05/16 11:10:05  ufranke
 * cosmetics
 *  - stwait default is board dependend now
 *
 * Revision 2.6  2005/06/23 15:54:22  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.5  2005/02/22 15:00:56  kp
 * add NETIF parameter section only in MENMON
 *
 * Revision 2.4  2005/02/09 16:19:22  kp
 * Added parameters:
 * - stdis_sram
 * - stdis_touch
 * - stdis_prtctrl
 * - nobanner
 *
 * Revision 2.3  2005/02/02 16:03:09  kp
 * adapted all parameters according to spec.
 * Make more room for temp. MENMON parameter string
 *
 * Revision 2.2  2005/01/18 15:49:24  kp
 * added size to NVS sections
 *
 * Revision 2.1  2005/01/12 14:19:55  kp
 * general cleanup
 *
 * Revision 2.0  2004/12/20 08:53:06  kp
 * rewritten for boot flash support, EEPROM extension section and boolean
 * params
 *
 * Revision 1.2  2004/10/07 09:17:40  kp
 * must increase allocation heap due to increased vxbline startup length
 *
 * Revision 1.1  2004/01/08 09:06:51  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/em04_eeprom.h>


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEm04CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM04
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Em04Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm04CpuProbe( xSuper )) != 0 )
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
/** Initialize SYSPARAM for EM04
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int Em04SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;
	
	xSuper->initBlk = *initBlk;

	super->reInit = Em04Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EM04 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEm04Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "misc", 			NULL, _EEOFF(mm_misc), "0", 0),
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL3( "com3se", 		NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL4( SYSP_useflpar, 	NULL, _EEOFF(mm_misc), "0", 
					SYSP_PD_RESTART ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),
	
	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL2( "stdis_int_cf", NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_sram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_U8X( "pciopts", 		NULL, _EEOFF(mm_pci_opts), "0", 0),

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX(   _EEOFF(mm_usr[0]) ),
	SYSP_PDE_END
};

#define _EEOFFX(x) offsetof(struct menmx_parms,x)
/** list of parameters in EM04 CPU EEPROM, menmon extended section */
static const SYSP_PAR_DESC SYSP_parListEm04MenmX[] = {
	SYSP_PDE_CON0( _EEOFFX(mmx_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFFX(mmx_con[1]), "a" ),  /* TOUCH */
	SYSP_PDE_CON2( _EEOFFX(mmx_con[2]), "0" ),	/* (none) */
	SYSP_PDE_CON3( _EEOFFX(mmx_con[3]), "0" ),	/* (none) */
	SYSP_PDE_GCON( _EEOFFX(mmx_gcon), 	"ff"),	/* use first available */
	SYSP_PDE_VMODE( _EEOFFX(mmx_vmode) ),
	SYSP_PDE_ECL(   _EEOFFX(mmx_ecl) ),
	SYSP_PDE_TTO(   _EEOFFX(mmx_tto) ),
	SYSP_PDE_TRIES( _EEOFFX(mmx_tries) ),
	SYSP_PDE_TDP(   _EEOFFX(mmx_tdp) ),
	SYSP_PDE_HDP(   _EEOFFX(mmx_hdp) ),
	SYSP_PDE_STWAIT(_EEOFFX(mmx_stwait), "30" ),
	SYSP_PDE_WDT(   _EEOFFX(mmx_wdt), "0" ),
	
	SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in EM04 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListEm04FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, NULL, _EEOFFF(fl_startup), "", 
				  EM04_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "", 
				  EM04_FLASH_KERPAR_LEN, 0 ),
	
	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEm04Mmpara[] = {
	SYSP_mem0,
	SYSP_mem1,
	SYSP_flash0,
	SYSP_cpu,
	SYSP_cpuclkhz,
	SYSP_memclkhz,
	SYSP_pciclkhz,
	SYSP_clun,
	SYSP_dlun,
	SYSP_rststat,	
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	NULL
};

/**********************************************************************/
/** Probe for EM04 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EM04 
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEm04CpuProbe( SYSP_MMSMB_SUPER *xSuper )
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
								 sizeof(EM04_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEm04Mmpara,
								xSuper->initBlk.mmParaStringMaxLen, 
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_EeprodGrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EM04_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEm04Menm, EM04_EEID_MENM, 
								 NULL )) != 0 )
		return rv;

	/* menmx section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menmx", 
								   offsetof( EM04_EEPROM, menmx ),
								   sizeof( struct menmx_parms ),
								   SYSP_parListEm04MenmX, EM04_EEID_MENMX, 
								   NULL )) != 0 )
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );
	useFlashStrings = SysParamBool( SYSP_useflpar, FALSE );
		

	if( useFlashStrings == FALSE ){
		/* startup string */
		if( (rv = SYSP_StrGrpCreate( nvsEe, "mmstartup", "startup",
									 offsetof( EM04_EEPROM, startup ),
									 EM04_EEPROM_STARTUP_LEN, NULL)) != 0 )
			return rv;
	}
		
	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EM04_EEPROM, vxBline ),
								EM04_VXBLINE_LEN, 
								xSuper->initBlk.vxBlineStringAdrs,
								"fei(0,0):", !useFlashStrings )))
		return rv;

	
	/* boot flash section */
	if( useFlashStrings ){
		/* create FLASH NVS section */
		nvsAdr.type = SYSP_NVSADR_SMB; /* well, not really */
		nvsAdr.addr.smb.bus = SYSPARAM_SMB_BUS_FLASH;	
		nvsAdr.addr.smb.dev = 0xff; /* last sector */

		if( (nvsFl = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpufl",
									 &xSuper->s, "", nvsAdr,
									 EM04_FLASH_LAST_SECTOR_SIZE )) == NULL )
			return SYSPARAM_NO_RESOURCE;

		if( (rv = SYSP_MpGrpCreateStd( 
				 nvsFl, "flstr", 
				 offsetof( EM04_BFLASH_PARMS, flstr ),
				 sizeof( struct flstr_parms ),
				 SYSP_parListEm04FlStr, EM04_FLID_STR, 
				 NULL )) != 0 )
			return rv;

		SYSP_NvsAdd( &xSuper->s, nvsFl );
	}

	return 0;
}

