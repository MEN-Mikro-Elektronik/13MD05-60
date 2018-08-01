/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_p504.c
 *
 *      \author  Ralf.Truebenbach@men.de
 *        $Date: 2008/11/04 16:05:13 $
 *    $Revision: 2.4 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  P504 board specific part
 *      
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_p504.c,v $
 * Revision 2.4  2008/11/04 16:05:13  rt
 * R:1. EE-STARTUP parameter is not available if EE-USEFLPAR is set to 1
 * M:1. Added "startup" alias for "mmstartup"
 *
 * Revision 2.3  2006/05/29 16:23:25  rt
 * cosmetics
 *
 * Revision 2.2  2006/05/29 16:19:56  rt
 * added some MM_Idle() calls (trigger watchdog)
 *
 * Revision 2.1  2006/05/17 13:02:15  rt
 * Initial Revision
 *

 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include "config.h"
#include <MEN/p504_eeprom.h>

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */
static const char G_allowedCharsHex[] = "[0-9A-Fa-f]";
static const char G_allowedCharsDec[] = "[0-9]";
static const char G_allowedCharsBool[] = "[01]";
static const char G_hwLongDesc[] = "(1=hardware monitor reset enabled)";

/**********************************************************************/
/** Table that describes all statically known board specific parameters
 **/
static const SYSP_HELP_DESC G_descTbl[] = {
	{
		.parName = "pci_out_0",
		.shortDesc = "PCI outbound %d mapping",
		.longDescFirstLine = "<CPU addr, PCI addr, size (0=disabled)>",
		.allowedChars = G_allowedCharsHex
	},	
	{
		.parName = "pci_in_0",
		.shortDesc = "PCI inbound %d mapping",
		.longDescFirstLine = "<CPU addr, PCI addr, size (0=disabled)>",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = "pci_pre_in_0",
		.shortDesc = "Enable prefetching IN %d",
		.longDescFirstLine = "(1=prefetchable)",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "pci_snoop_in_0",
		.shortDesc = "Enable snooping IN %d",
		.longDescFirstLine = "(1=snooping enabled)",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "pci_pre_out_0",
		.shortDesc = "Enable prefetching OUT %d",
		.longDescFirstLine = "(1=prefetchable)",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "pci_io_out_0",
		.shortDesc = "PCI I/O space OUT %d",
		.longDescFirstLine = "(0=MEM 1=I/O)",
		.allowedChars = G_allowedCharsBool
	},	
	{
		.parName = "psr1v5",
		.shortDesc = "Supervise 1,%d V",
		.longDescFirstLine = G_hwLongDesc,
		.allowedChars = G_allowedCharsBool
	},	
	{
		.parName = "psr3v3",
		.shortDesc = "Supervise 3,3 V",
		.longDescFirstLine = G_hwLongDesc,
		.allowedChars = G_allowedCharsBool
	},	
	{
		.parName = "psr5v",
		.shortDesc = "Supervise 5,0 V",
		.longDescFirstLine = G_hwLongDesc,
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "temp_crit",
		.shortDesc = "Supervise critical temperature limit",
		.longDescFirstLine = G_hwLongDesc,
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "temp_low",
		.shortDesc = "Supervise low temperature limit",
		.longDescFirstLine = G_hwLongDesc,
		.allowedChars = G_allowedCharsBool
	},
	{ .parName = NULL	/* terminator */ }
};

static SYSP_HELP_DESC_TBL G_descTblP504 = { NULL, G_descTbl }; 

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsP504CpuProbe( SYSP_MMSMB_SUPER *xSuper );

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/** PCI outbound window */
#define SYSP_PDE_PCIOUT( _nm, _alias, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawPci, SYSP_FromRawPci, SYSP_ValidatePciOut,\
   _deflt, 0, (SYSP_PD_MMPARASTR*0) }

/** PCI inbound window */
#define SYSP_PDE_PCIIN( _nm, _alias, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawPci, SYSP_FromRawPci, SYSP_ValidatePciIn,\
   _deflt, 0, (SYSP_PD_MMPARASTR*0) }


/**********************************************************************/
/** from PCI outbound/inbound window configuration
 */
int SYSP_FromRawPci( char *dest, int destLen, void *src )
{
	char buf[50];
	u_int16 *p = (u_int16*)src;
	char *s = buf;
	int i;

	for( i=0; i<3; ++i ) {
		s += SYSP_SNPRINTF( s, 64, "%04X0000%s", SYSP_FROMBE16(p[i]), i==2 ? "" : ",");
	}
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to PCI outbound/inbound window configuration
 */
int SYSP_ToRawPci( void *dest, const char *src )
{
	int i;
	u_int32 n;
	u_int16 *p = (u_int16 *)dest;

	for( i=0; i<3; ++i ) {
		n = SYSP_STRTOUL( src, (char**)&src, 16 );
		p[i] = SYSP_TOBE16( (u_int16)(n>>16) );
		++src;
	}

	return 0;
}

/**********************************************************************/
/** check PCI outbound window configuration
 */
int SYSP_ValidatePciOut( const char *src, int notused )
{
	int i;
	u_int32 n[3];

	for( i=0; i<3; ++i ) {
		n[i] = SYSP_STRTOUL( src, (char**)&src, 16 );

		if( ((i!=2) && *src != ',' ) ||
			((i==2) && *src != '\0' ))
			return SYSPARAM_INVAL_VALUE;
		++src;
	}

	/* alignment error? */
	if( n[0]%n[2] || n[1]%n[2] )
	    return SYSPARAM_INVAL_VALUE;

	/* use only 16 high address bits (save eeprom space) */ 
	if( n[0] & 0xffff || n[1] & 0xffff || n[2] & 0xffff )
	    return SYSPARAM_INVAL_VALUE;

    /* internal address in pci block?  */
    if( (n[0] < CONFIG_PCI_MEM_BASE && n[0] > 0)
        || n[0] >= (CONFIG_PCI_MEM_BASE+0x20000000) )
	    return SYSPARAM_INVAL_VALUE;
   
    return 0;
}

/**********************************************************************/
/** check PCI inbound window configuration
 */
int SYSP_ValidatePciIn( const char *src, int notused )
{
	int i;
	u_int32 n[3];

	for( i=0; i<3; ++i ) {
		n[i] = SYSP_STRTOUL( src, (char**)&src, 16 );

		if( ((i!=2) && *src != ',' ) ||
			((i==2) && *src != '\0' ))
			return SYSPARAM_INVAL_VALUE;
		++src;
	}
	
	/* alignment error? */
	if( n[0]%n[2] || n[1]%n[2] )
	    return SYSPARAM_INVAL_VALUE;
	   
	/* use only 16 high address bits (save eeprom space) */ 
	if( n[0] & 0xffff || n[1] & 0xffff || n[2] & 0xffff )
	    return SYSPARAM_INVAL_VALUE;
	    	
    return 0;
}

/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for P504
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int P504Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

    /* add P504 board specific help descriptors */
    SYSP_AddParamTable( &G_descTblP504 );
 
	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsP504CpuProbe( xSuper )) != 0 )
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
/** Initialize SYSPARAM for P504
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int P504SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;
	
	xSuper->initBlk = *initBlk;

	super->reInit = P504Init;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in P504 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListP504Menm[] = {

	/* misc field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "misc", 			NULL, _EEOFF(mm_misc), "02", 0),
	SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "1", 0 ),
	SYSP_PDE_BOOL2( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL3( "com3se", 		NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL4( SYSP_useflpar, 	NULL, _EEOFF(mm_misc), "0", 
					SYSP_PD_RESTART ),
	SYSP_PDE_BOOL5( "nobanner", 	NULL, _EEOFF(mm_misc), "0", 0 ),
	
	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_U8X( "selftest",  		NULL, _EEOFF(mm_selftest), "10", 0),
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL2( "stdis_int_cf", NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_sram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "1", 0 ),
	SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

	SYSP_PDE_BOOL0( "pci_pre_in_0",     NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	SYSP_PDE_BOOL1( "pci_pre_in_1",     NULL, _EEOFF(mm_pci_opts), "0", 0 ),
	SYSP_PDE_BOOL2( "pci_snoop_in_0", 	NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	SYSP_PDE_BOOL3( "pci_snoop_in_1", 	NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	/*SYSP_PDE_BOOL4( "pci_en_in_2", 	NULL, _EEOFF(mm_pci_opts), "1", 0 ),*/
	SYSP_PDE_BOOL5( "pci_pre_out_0", 	NULL, _EEOFF(mm_pci_opts), "1", 0 ),
	SYSP_PDE_BOOL6( "pci_pre_out_1", 	NULL, _EEOFF(mm_pci_opts), "0", 0 ),
	SYSP_PDE_BOOL7( "pci_pre_out_2", 	NULL, _EEOFF(mm_pci_opts), "0", 0 ),
	SYSP_PDE_BOOL0( "pci_io_out_0", 	NULL, _EEOFF(mm_pci_opts[1]), "0", 0 ),
	SYSP_PDE_BOOL1( "pci_io_out_1", 	NULL, _EEOFF(mm_pci_opts[1]), "0", 0 ),
	SYSP_PDE_BOOL2( "pci_io_out_2", 	NULL, _EEOFF(mm_pci_opts[1]), "1", 0 ),
	
	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_END
};

#define _EEOFFX(x) offsetof(struct menmx_parms,x)
/** list of parameters in P504 CPU EEPROM, menmon extended section */
static const SYSP_PAR_DESC SYSP_parListP504MenmX[] = {
	SYSP_PDE_CON0( _EEOFFX(mmx_con[0]), "8" ),	/* COM1 */
	SYSP_PDE_CON1( _EEOFFX(mmx_con[1]), "9" ),  /* COM2 */
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
	SYSP_PDE_BOOL0( "psr1v5",   NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
	SYSP_PDE_BOOL1( "psr3v3",   NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
	SYSP_PDE_BOOL2( "psr5v",   	NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
	SYSP_PDE_BOOL3( "psr1v8",  	NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
	SYSP_PDE_BOOL4( "temp_crit",  NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
	SYSP_PDE_BOOL5( "temp_low",   NULL, _EEOFFX(mmx_hw_monitor), "0", 0 ),
	SYSP_PDE_PCIOUT( "pci_out_0", NULL, _EEOFFX(mmx_pci_out[0]),  "80000000,80000000,10000000" ),
	SYSP_PDE_PCIOUT( "pci_out_1", NULL, _EEOFFX(mmx_pci_out[6]),  "90000000,90000000,10000000" ),
	SYSP_PDE_PCIOUT( "pci_out_2", NULL, _EEOFFX(mmx_pci_out[12]), "00000000,00000000,00000000" ),
	SYSP_PDE_PCIIN( "pci_in_0",  NULL, _EEOFFX(mmx_pci_in[0]),   "02000000,00000000,02000000" ),
	SYSP_PDE_PCIIN( "pci_in_1",  NULL, _EEOFFX(mmx_pci_in[6]),   "02000000,00000000,00400000" ),
		
	/*SYSP_PDE_UXX(   _EEOFFX(mm_usr[0]) ),*/
	
	SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in P504 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListP504FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, SYSP_startup, _EEOFFF(fl_startup), "", 
				  P504_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "", 
				  P504_FLASH_KERPAR_LEN, 0 ),
	
	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListP504Mmpara[] = {
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
	SYSP_nmac0, /* PHY0 - FCC1 */
	SYSP_nmac1, /* PHY1 - FCC2 */
	NULL
};

/**********************************************************************/
/** Probe for P504 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in P504 
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsP504CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe, *nvsFl;
	SYSP_NVS_ADR nvsAdr;
	int useFlashStrings;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = P504_EE_SROM_MENMON;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr, 
								 sizeof(P504_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListP504Mmpara,
								xSuper->initBlk.mmParaStringMaxLen, 
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_EeprodGrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( P504_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListP504Menm, P504_EEID_MENM, 
								 NULL )) != 0 )
		return rv;

	/* menmx section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menmx", 
								   offsetof( P504_EEPROM, menmx ),
								   sizeof( struct menmx_parms ),
								   SYSP_parListP504MenmX, P504_EEID_MENMX, 
								   NULL )) != 0 )
		return rv;

    MM_Idle();
	SYSP_NvsAdd( &xSuper->s, nvsEe ); 
	useFlashStrings = SysParamBool( SYSP_useflpar, FALSE );	

    MM_Idle();
	if( useFlashStrings == FALSE ){
		/* startup string */
		if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
									 offsetof( P504_EEPROM, startup ),
									 P504_EE_STARTUP_LEN, NULL)) != 0 )
			return rv;
	}
 
	/* vxbline */
    MM_Idle();
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( P504_EEPROM, vxBline ),
								P504_VXBLINE_LEN, 
								xSuper->initBlk.vxBlineStringAdrs,
								"motfcc(0,0):", !useFlashStrings )))
		return rv;

	/* boot flash section */
    MM_Idle();
	if( useFlashStrings ){
		/* create FLASH NVS section */
		nvsAdr.type = SYSP_NVSADR_SMB; /* well, not really */
		nvsAdr.addr.smb.bus = SYSPARAM_SMB_BUS_FLASH;
		nvsAdr.addr.smb.dev = 0xff; /* last sector */

		if( (nvsFl = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpufl",
									 &xSuper->s, "", nvsAdr,
									 P504_FLASH_LAST_SECTOR_SIZE )) == NULL )
			return SYSPARAM_NO_RESOURCE;

        MM_Idle();

		if( (rv = SYSP_MpGrpCreateStd( 
				 nvsFl, "flstr", 
				 offsetof( P504_BFLASH_PARMS, flstr ),
				 sizeof( struct flstr_parms ),
				 SYSP_parListP504FlStr, P504_FLID_STR, 
				 NULL )) != 0 )
			return rv;
        
        MM_Idle();
		SYSP_NvsAdd( &xSuper->s, nvsFl );
	}

	return 0;
}

