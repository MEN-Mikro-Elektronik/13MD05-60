/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_sc20.c
 *
 *      \author  klaus.popp@men.de/rla
 *        $Date: 2010/12/15 09:17:34 $
 *    $Revision: 2.3 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  SC20 specific part
 *
 *    \switches  MM_CODE_SHRINK  - to bring MENMON < 256 kB
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_sc20.c,v $
 * Revision 2.3  2010/12/15 09:17:34  rlange
 * R: Customer requirement Support of no output mode
 * M: Added parameter outdis
 *
 * Revision 2.2  2010/01/22 13:33:32  RLange
 * R: Support Password handling
 * M: Added pwd to EEPROM structure
 *
 * Revision 2.1  2009/12/10 11:20:21  RLange
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009-2009 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/sc20_eeprom.h>
#include <MEN/sc20_cfg.h>

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsSc20CpuProbe( SYSP_MMSMB_SUPER *xSuper );


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for SC20
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Sc20Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsSc20CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

#if defined(SYSP_CONFIG_MENMON) && !defined(MM_CODE_SHRINK)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );
#endif

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for SC20
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int SC20_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Sc20Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}


/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in SC20 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListSc20Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL6( "outdis", 	NULL, _EEOFF(mm_misc), "0", 0 ),

	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "0", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL2( SYSP_stdis_ether, NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_rtc",    NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),

	SYSP_PDE_CON0( _EEOFF(mm_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "0" ),   	/* (none) */
	SYSP_PDE_CON2( _EEOFF(mm_con[2]), "0" ),	/* (none) */
	SYSP_PDE_CON3( _EEOFF(mm_con[3]), "0" ),	/* (none) */
	SYSP_PDE_ECL(   _EEOFF(mm_ecl) ),
	SYSP_PDE_TTO(   _EEOFF(mm_tto) ),
	SYSP_PDE_TRIES( _EEOFF(mm_tries) ),
	SYSP_PDE_TDP(   _EEOFF(mm_tdp) ),
	SYSP_PDE_HDP(   _EEOFF(mm_hdp) ),
	SYSP_PDE_STWAIT(_EEOFF(mm_stwait), "30" ),
	SYSP_PDE_NSPEED( SYSP_nspeed0, NULL, _EEOFF(mm_nspeed), "AUTO" ),

	SYSP_PDE_STR( "cname", NULL, _EEOFF(mm_custname[0]), "", SC20CFG_CUSTID_LEN,   0 ),
	SYSP_PDE_STR( "cid",   NULL, _EEOFF(mm_custid[0]),   "", SC20CFG_CUSTNAME_LEN, 0 ),
	SYSP_PDE_STR( "pwd",   NULL, _EEOFF(mm_pwd[0]),      "", SC20CFG_CUSTID_LEN, SYSP_PD_ENCRYPT ),

	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static const SYSP_CONST_STRING SYSP_parListSc20Mmpara[] = {
	SYSP_mem0,
	SYSP_flash0,
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
	SYSP_mmst,
	SYSP_mm,
	SYSP_cons,
	SYSP_nmac0,
	NULL
};

/**********************************************************************/
/** Probe for SC20 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in SC20
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsSc20CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe;
	SYSP_NVS_ADR nvsAdr;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = SC20CFG_IIC_ONBOARD;
	nvsAdr.addr.smb.dev = SC20_SMB_SROM_MM;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(SC20_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListSc20Mmpara,
								xSuper->initBlk.mmParaStringMaxLen,
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( SC20_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListSc20Menm, SC20_EEID_MENM,
								 NULL )) != 0 )
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvsEe );


	/* startup string */
	if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
								 offsetof( SC20_EEPROM, startup ),
								 SC20_EEPROM_STARTUP_LEN, NULL)) != 0 )
		return rv;

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( SC20_EEPROM, vxBline ),
								SC20_VXBLINE_LEN,
								xSuper->initBlk.vxBlineStringAdrs,
								"fec(0,0):", SYSP_DONT_PROVIDE_KERPAR )))
		return rv;

	/* kerpar */
	if( (rv = SYSP_StrGrpCreate( nvsEe, "kerpar", NULL,
								 offsetof( SC20_EEPROM, kerpar ),
								 SC20_KERPAR_LEN, "" )))
		return rv;


	return 0;
}




