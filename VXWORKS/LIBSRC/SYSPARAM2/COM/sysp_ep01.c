/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_ep01.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/04/23 15:32:18 $
 *    $Revision: 2.7 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  EP01 specific part
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_ep01.c,v $
 * Revision 2.7  2010/04/23 15:32:18  rlange
 * R: Message of missing usbdp parameter at usbt command
 * M: Added SYSP_usbdp to parameter list
 *
 * Revision 2.6  2007/08/10 13:00:10  rla
 * changed default for console2 to CLUN b
 * removed bmpaload
 * added miscext field for HW-Handshake support of Z125 Uarts
 *
 * Revision 2.5  2007/03/22 11:10:17  ufranke
 * cosmetics
 *  - used common SYSP_stdis_nand
 *
 * Revision 2.4  2007/02/22 13:50:52  rla
 * added initialization for second ESMCB
 * changed default for sysvar parameter
 *
 * Revision 2.3  2006/12/01 14:50:56  rla
 * Added init ESMCB call, added parameter sysvar and bmpaload
 *
 * Revision 2.2  2006/08/07 08:43:03  rla
 * Changed SMBBUS for Carrier from IO to FPGA
 *
 * Revision 2.1  2006/07/13 16:50:23  RLange
 * Initial Revision
 *
 * cloned from EP01 Rev 2.4
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/ep01_eeprom.h>
#include <MEN/ep01_cfg.h>


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEp01CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EP01
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Ep01Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEp01CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* set device address and size for ESMCB EEPROM */
	SYSP_EsmCarrierInitGlobals( EP01CFG_IIC_ESMCB_ADDR,
								EP01CFG_IIC_ESMCB_SIZE, 'c' );

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, EP01CFG_IIC_FPGA );
	if( rv == SYSPARAM_EEPROM_ERROR )
		rv = 0;					/* non fatal */

	/* set device address and size for ESMCB EEPROM */
	SYSP_EsmCarrierInitGlobals( EP01CFG_IIC_DVICB_ADDR,
								EP01CFG_IIC_DVICB_SIZE, 'd' );

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, EP01CFG_IIC_FPGA );
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
/** Initialize SYSPARAM for EP01
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int EP01_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Ep01Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}


/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EP01 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEp01Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),

	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_stdis_nand,   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_sram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX(   _EEOFF(mm_usr[0]) ),

	SYSP_PDE_CON0( _EEOFF(mm_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "a" ),   	/* TOUCH */
	SYSP_PDE_CON2( _EEOFF(mm_con[2]), "b" ),	/* (none) */
	SYSP_PDE_CON3( _EEOFF(mm_con[3]), "0" ),	/* (none) */
	SYSP_PDE_GCON( _EEOFF(mm_gcon),   "ff"),	/* use first available */
	SYSP_PDE_VMODE( _EEOFF(mm_vmode) ),
	SYSP_PDE_ECL(   _EEOFF(mm_ecl) ),
	SYSP_PDE_TTO(   _EEOFF(mm_tto) ),
	SYSP_PDE_TRIES( _EEOFF(mm_tries) ),
	SYSP_PDE_TDP(   _EEOFF(mm_tdp) ),
	SYSP_PDE_HDP(   _EEOFF(mm_hdp) ),
	SYSP_PDE_STWAIT(_EEOFF(mm_stwait), "30" ),
	SYSP_PDE_WDT(   _EEOFF(mm_wdt), "0" ),
	SYSP_PDE_NSPEED( SYSP_nspeed0, NULL, _EEOFF(mm_nspeed[0]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed1, NULL, _EEOFF(mm_nspeed[1]), "AUTO" ),
/*	SYSP_PDE_NSPEED( SYSP_nspeed2, NULL, _EEOFF(mm_nspeed[2]), "AUTO" ),*/

	SYSP_PDE_BOOL0( "outdis",   NULL, _EEOFF(mm_outdis), "1", 0 ),
	SYSP_PDE_U8D( "dispres", NULL, _EEOFF(mm_dispres), "0", SYSP_PD_MMPARASTR ),
	SYSP_PDE_U8D( "sysvar", NULL, _EEOFF(mm_sysvar), "1", SYSP_PD_MMPARASTR ),
	SYSP_PDE_U8X( "miscext", NULL, _EEOFF(mm_misc2), "0", SYSP_PD_MMPARASTR ),
	SYSP_PDE_BOOL0( "hwhs0", 		NULL, _EEOFF(mm_misc2), "0", 0 ),
	SYSP_PDE_BOOL1( "hwhs1", 		NULL, _EEOFF(mm_misc2), "0", 0 ),
	SYSP_PDE_BOOL2( "hwhs3", 		NULL, _EEOFF(mm_misc2), "0", 0 ),
	SYSP_PDE_BOOL3( "hwhs4", 		NULL, _EEOFF(mm_misc2), "0", 0 ),
	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEp01Mmpara[] = {
	SYSP_mem0,
	SYSP_mem1,
	SYSP_flash0,
	SYSP_flash1,
	SYSP_cpu,
	SYSP_cpuclkhz,
	SYSP_MPC52XX_inclkhz,
	SYSP_memclkhz,
	SYSP_MPC52XX_xlbclkhz,
	SYSP_MPC52XX_ipbclkhz,
	SYSP_pciclkhz,
	SYSP_clun,
	SYSP_dlun,
	SYSP_usbdp,
	SYSP_rststat,
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	SYSP_nmac0,
	SYSP_nmac1,
	/* SYSP_nmac2,*/
	NULL
};

/**********************************************************************/
/** Probe for EP01 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EP01
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEp01CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = EP01CFG_IIC_ONBOARD;
	nvsAdr.addr.smb.dev = EP01_SMB_SROM_MM;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(EP01_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEp01Mmpara,
								xSuper->initBlk.mmParaStringMaxLen,
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EP01_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEp01Menm, EP01_EEID_MENM,
								 NULL )) != 0 )
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );


	/* startup string */
	if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
								 offsetof( EP01_EEPROM, startup ),
								 EP01_EEPROM_STARTUP_LEN, NULL)) != 0 )
		return rv;

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EP01_EEPROM, vxBline ),
								EP01_VXBLINE_LEN,
								xSuper->initBlk.vxBlineStringAdrs,
								"fec(0,0):", SYSP_DONT_PROVIDE_KERPAR )))
		return rv;

	/* kerpar */
	if( (rv = SYSP_StrGrpCreate( nvsEe, "kerpar", NULL,
								 offsetof( EP01_EEPROM, kerpar ),
								 EP01_KERPAR_LEN, "" )))
		return rv;


	return 0;
}


