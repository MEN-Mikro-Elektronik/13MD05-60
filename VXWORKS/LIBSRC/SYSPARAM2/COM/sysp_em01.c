/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_em01.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2009/09/23 08:59:34 $
 *    $Revision: 2.14 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  EM01 specific part
 *
 *    \switches  MM_CODE_SHRINK  - to bring MENMON < 256 kB
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em01.c,v $
 * Revision 2.14  2009/09/23 08:59:34  ufranke
 * R: improve boot speed
 * M: added ee-normalboot flag
 * R: no common ee-nobanner
 * M: moved nobanner to syspTags.c
 *
 * Revision 2.13  2008/06/30 17:58:21  CSchuster
 * R: LM81: 1.2V is now calibrated as well
 * M: add 1.2V parameters to EEPROM and define SYSP string
 *
 * Revision 2.12  2008/06/30 13:48:31  ufranke
 * R: ee-def overwrites LM81 calibration values
 * M: LM81 calibration values read only now
 *
 * Revision 2.11  2008/06/27 15:09:58  ufranke
 * R: LM81 may be calibrated
 * M: added LM81 calibration value section
 *
 * Revision 2.10  2008/02/04 14:33:55  ufranke
 * added
 *  + fpga
 *
 * Revision 2.9  2007/11/21 17:25:23  ufranke
 * added
 *  + MMPARA tag "usbdp"
 * changed
 *  - SYSP_parListEm01Menm is const now
 *
 * Revision 2.8  2007/03/22 11:08:47  ufranke
 * added
 *  + switch MM_CODE_SHRINK
 *
 * Revision 2.7  2006/10/13 19:46:51  cs
 * added:
 *   + SYSP_gstatshow to flags in mm_misc
 *
 * Revision 2.6  2006/10/13 11:50:14  ufranke
 * changed
 *  - SYSP_PDE_STWAIT back to 30
 *    because NAND FLASH will be detected by selftest with timeout
 * removed
 *  - stdis_sram because of SRAM selftest was removed in a earlier version
 *
 * Revision 2.5  2006/05/16 11:09:16  ufranke
 * changed
 *  - SYSP_PDE_STWAIT from fix default 30 to 90 for NAND FLASH
 *
 * Revision 2.4  2006/02/22 18:12:17  UFRANKE
 * changed
 *  - 2.5V, Core, and 3.3V will be protected at default
 *
 * Revision 2.3  2005/06/23 15:54:16  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.2  2005/04/11 09:21:39  kp
 * Disable unsunsed nspeed1-2/nmac settings for now
 *
 * Revision 2.1  2005/03/16 16:29:45  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/em01_eeprom.h>
#include <MEN/em01_cfg.h>

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
const char    SYSP_em1Lm81Calib_2V5[]  =	"psr2v5_cal";
const char    SYSP_em1Lm81Calib_3V3[]  =	"psr3v3_cal";
const char    SYSP_em1Lm81Calib_5V0[]  =	"psr5v_cal";
const char    SYSP_em1Lm81Calib_core[] =	"psrcore_cal";
const char    SYSP_em1Lm81Calib_1V2[]  =	"psr1v2_cal";

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEm01CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM01
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Em01Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm01CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, EM01CFG_IIC_IO );
	if( rv == SYSPARAM_EEPROM_ERROR )
		rv = 0;					/* non fatal */

#if defined(SYSP_CONFIG_MENMON) && !defined(MM_CODE_SHRINK)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );
#endif

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for EM01
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int EM01_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Em01Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}


/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

#define _EEOFFLM81(x) offsetof(LM81_PARMS,x)

/** list of parameters in EM01 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEm01Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL5( SYSP_nobanner, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL6( SYSP_gstatshow, NULL, _EEOFF(mm_misc), "1", 0 ),
	SYSP_PDE_BOOL7( SYSP_normalboot,NULL, _EEOFF(mm_misc), "1", 0 ),

	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "1", 0 ),
	SYSP_PDE_BOOL2( SYSP_stdis_nand,   NULL, _EEOFF(mm_selftest), "0", 0 ),
	/*SYSP_PDE_BOOL3( "stdis_sram",   NULL, _EEOFF(mm_selftest), "0", 0 ), - sram test removed */
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX(   _EEOFF(mm_usr[0]) ),

	SYSP_PDE_CON0( _EEOFF(mm_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "a" ),   	/* TOUCH */
	SYSP_PDE_CON2( _EEOFF(mm_con[2]), "0" ),	/* (none) */
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
/*	SYSP_PDE_NSPEED( SYSP_nspeed1, NULL, _EEOFF(mm_nspeed[1]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed2, NULL, _EEOFF(mm_nspeed[2]), "AUTO" ),*/

	SYSP_PDE_BOOL0( "psr2v5",   NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL1( "psrcore",  NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL2( "psr3v3",   NULL, _EEOFF(mm_powerReset), "1", 0 ),
	SYSP_PDE_BOOL3( "psr5v",   	NULL, _EEOFF(mm_powerReset), "0", 0 ),

	SYSP_PDE_END
};

/** list of parameters in EM01 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEm01Lm81[] = {

	/*{ _nm,                   _alias,                  _offs,            toRaw,        fromRaw,       validate, _deflt, vParam, pdFlags} */
	{ SYSP_em1Lm81Calib_2V5,	NULL,   _EEOFFLM81(lm81_2V5),   SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,   "88", 0xFF, SYSP_PD_NONE }, /* 88 to force not calibrated warning */
	{ SYSP_em1Lm81Calib_core,	NULL,	_EEOFFLM81(lm81_vcore), SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,   "FF", 0xFF, SYSP_PD_NONE },
	{ SYSP_em1Lm81Calib_3V3,	NULL,   _EEOFFLM81(lm81_3V3),   SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,   "FF", 0xFF, SYSP_PD_NONE },
	{ SYSP_em1Lm81Calib_5V0,	NULL,   _EEOFFLM81(lm81_5V0),   SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,   "FF", 0xFF, SYSP_PD_NONE },
	{ SYSP_em1Lm81Calib_1V2,	NULL,   _EEOFFLM81(lm81_1V2),   SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,   "FF", 0xFF, SYSP_PD_NONE },
	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static const SYSP_CONST_STRING SYSP_parListEm01Mmpara[] = {
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
	SYSP_fpga,
	SYSP_rststat,
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	SYSP_nmac0,
	SYSP_rand,
	/*SYSP_nmac1,
	  SYSP_nmac2,*/
	NULL
};

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
static int SYSP_NvsEm01CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = EM01CFG_IIC_ONBOARD;
	nvsAdr.addr.smb.dev = EM01_SMB_SROM_MM;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(EM01_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEm01Mmpara,
								xSuper->initBlk.mmParaStringMaxLen,
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EM01_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEm01Menm, EM01_EEID_MENM,
								 NULL )) != 0 )
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStdRo( nvsEe, "lm81", offsetof( EM01_EEPROM, lm81),
								 sizeof( LM81_PARMS ),
								 SYSP_parListEm01Lm81, EM01_EEID_LM81,
								 NULL )) != 0 )
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvsEe );


	/* startup string */
	if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
								 offsetof( EM01_EEPROM, startup ),
								 EM01_EEPROM_STARTUP_LEN, NULL)) != 0 )
		return rv;

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EM01_EEPROM, vxBline ),
								EM01_VXBLINE_LEN,
								xSuper->initBlk.vxBlineStringAdrs,
								"fec(0,0):", SYSP_DONT_PROVIDE_KERPAR )))
		return rv;

	/* kerpar */
	if( (rv = SYSP_StrGrpCreate( nvsEe, "kerpar", NULL,
								 offsetof( EM01_EEPROM, kerpar ),
								 EM01_KERPAR_LEN, "" )))
		return rv;


	return 0;
}




