/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_a012.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2006/07/17 08:16:52 $
 *    $Revision: 2.3 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  A12 specific part
 *      
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_a12.c,v $
 * Revision 2.3  2006/07/17 08:16:52  ag
 * set default for SYSP_PDE_STWAIT to 30
 *
 * Revision 2.2  2006/01/20 10:19:01  ag
 * changed parameter names
 *
 * Revision 2.1  2005/12/22 16:16:49  ag
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/a12_eeprom.h>


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsA12CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for A12
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int A12Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsA12CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

#if defined(SYSP_CONFIG_MENMON)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );
#endif

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv; 
}

/**********************************************************************/
/** Initialize SYSPARAM for A12
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int A12SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;
	
	xSuper->initBlk = *initBlk;

	super->reInit = A12Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in A12 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListA12Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "misc", 			NULL, _EEOFF(mm_misc), "0", 0),
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),
	
	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_U8X( "vmeirqenb", "vme_irq", _EEOFF(mm_vme_irqenb), "fe", 
	              SYSP_PD_MMPARASTR),
	SYSP_PDE_U8X( "vme_a24sa",    NULL, _EEOFF(mm_vme_a24slaveAddr), "ff", 0),
	SYSP_PDE_U8X( "pciopts",         NULL, _EEOFF(mm_pci_opts), "3", 0),
	SYSP_PDE_BOOL0( "pci_stgath",  NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	SYSP_PDE_BOOL1( "pci_specrd", NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	SYSP_PDE_U8X( "vme_opts",        NULL, _EEOFF(mm_vme_opts), "0", 0),
	SYSP_PDE_BOOL0( "vme_req",    NULL, _EEOFF(mm_vme_opts), "0", 0 ),
	SYSP_PDE_U8X( "vme_a32ma",  NULL, _EEOFF(mm_vmePLDz002_a32masterBase), "0", 0),
	SYSP_PDE_U16X( "vmode",  NULL, _EEOFF(mm_vmode), "101", 0),
	SYSP_PDE_U8X( "kmap",  NULL, _EEOFF(mm_keymap), "0", 0),


	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_END
};

#define _EEOFFX(x) offsetof(struct menmx_parms,x)
/** list of parameters in A12 CPU EEPROM, menmon extended section */
static const SYSP_PAR_DESC SYSP_parListA12MenmX[] = {
	SYSP_PDE_CON0( _EEOFFX(mmx_con[0]), "4" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFFX(mmx_con[1]), "20" ), /* VGA */
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
	SYSP_PDE_UXX(   _EEOFFX(mmx_usr[0]) ),
	SYSP_PDE_END
};

#if 0
#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in A12 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListA12FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, NULL, _EEOFFF(fl_startup), "", 
				  A12_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "", 
				  A12_FLASH_KERPAR_LEN, 0 ),
	
	SYSP_PDE_END
};
#endif /* 0 */

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListA12Mmpara[] = {
	SYSP_mem0,
	SYSP_mem1,
	SYSP_flash0,
	SYSP_cpu,
	"cpuclk",
	SYSP_cpuclkhz,
	"memclk",
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
/** Probe for A12 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in A12 
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsA12CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = 0xa0; /* <- ändern 0xa8 */

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr, 
								 sizeof(A12_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListA12Mmpara,
								xSuper->initBlk.mmParaStringMaxLen, 
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_EeprodGrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( A12_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListA12Menm, EEID_MENM, 
								 NULL )) != 0 )
		return rv;

	/* startup string */
	if( (rv = SYSP_StrGrpCreate( nvsEe, "mmstartup", "startup",
								 offsetof( A12_EEPROM, startup ),
								 A12_EEPROM_STARTUP_LEN, NULL)) != 0 )
		return rv;


	/* menmx section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menmx", 
								   offsetof( A12_EEPROM, menmx ),
								   sizeof( struct menmx_parms ),
								   SYSP_parListA12MenmX, EEID_MENMX, 
								   NULL )) != 0 )
		return rv;
		
	/* startup string */
	if( (rv = SYSP_StrGrpCreate( nvsEe, "mmstartup", "startup",
								 offsetof( A12_EEPROM, startup ),
								 A12_EEPROM_STARTUP_LEN, NULL)) != 0 )
		return rv;

		
	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( A12_EEPROM, vxBline ),
								A12_VXBLINE_LEN, 
								xSuper->initBlk.vxBlineStringAdrs,
								"fei(0,0):", FALSE ))) 
		return rv;                         /* don't subst 's' with kerpar */

	/* kerpar */
	if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_kerpar, NULL,
								 offsetof( A12_EEPROM, kerpar ),
								 A12_XKERPAR_LEN, "" )))
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );

	return 0;
}


