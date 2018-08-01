/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_sc15a.c
 *
 *      \author  rt
 *        $Date: 2008/11/04 16:05:16 $
 *    $Revision: 2.3 $
 * 
 *     \project  SYSPARAM library
 *       \brief  SC15A board specific part
 *               (cloned from sysp_p504.c V2.3)
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_sc15a.c,v $
 * Revision 2.3  2008/11/04 16:05:16  rt
 * R:1. EE-STARTUP parameter is not available if EE-USEFLPAR is set to 1
 * M:1. Added "startup" alias for "mmstartup"
 *
 * Revision 2.2  2006/09/27 12:49:14  rt
 * added:
 * + EE-SIDE parameter
 *
 * Revision 2.1  2006/08/11 17:33:24  rt
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include "config.h"
#include <MEN/sc15a_eeprom.h>

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super;        /**< SYSPARAM super object  */
static u_int32 G_alloc[1100];           /**< allocation heap  */
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
        .parName = "psr_main",
        .shortDesc = "Supervise EU-CTRL-MAIN voltages",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBool
    },
    {
        .parName = "psr_io1",
        .shortDesc = "Supervise EU-CTRL-IO%d voltages",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBool
    }, 
    {
        .parName = "psr_spare",
        .shortDesc = "Supervise EU-CTRL-SPARE voltages",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBool
    },
    {
        .parName = "temp_crit_main",
        .shortDesc = "Supervise critical temperature limit",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBool
    },
    {
        .parName = "temp_low_main",
        .shortDesc = "Supervise low temperature limit",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBool
    },
    { .parName = NULL   /* terminator */ }
};

static SYSP_HELP_DESC_TBL G_descTblSc15a = { NULL, G_descTbl }; 

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsSc15aCpuProbe( SYSP_MMSMB_SUPER *xSuper );

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
        s += SYSP_SNPRINTF( s, sizeof(buf) "%04X0000%s", SYSP_FROMBE16(p[i]), 
                           i==2 ? "" : ",");
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
/** Initialize/Reinitialize SYSPARAM for SC15A
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Sc15aInit( SYSP_SUPER *super )
{
    int rv;

    SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

    SYSP_ALLOC_INIT( super, G_alloc );
    SYSP_SuperMmsmbInit( xSuper );

    /* add SC15A board specific help descriptors */
    SYSP_AddParamTable( &G_descTblSc15a );
 
    /* probe CPU EEPROM */
    if( (rv = SYSP_NvsSc15aCpuProbe( xSuper )) != 0 )
        return rv;              /* fatal */

#if defined(SYSP_CONFIG_MENMON)
    /* init fake parameters for netif devices */
    SYSP_MmNetIfParamsCreate( super );
#endif
  
    /* build MM para string */
    super->postProcessSetParam( super, NULL, 0 );

    return rv; 
}

/**********************************************************************/
/** Initialize SYSPARAM for SC15A
 *
 * \param initBlk       \IN SYSPARAM initialisation structure for
 *                          MENMON/SMB implementations. Caller can destroy
 *                          \a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int Sc15aSysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
    SYSP_SUPER *super;
    SYSP_MMSMB_SUPER *xSuper;

    xSuper = &G_super;
    SYSP_super = super = &xSuper->s;
    
    xSuper->initBlk = *initBlk;

    super->reInit = Sc15aInit;
    SYSP_SuperMmsmbInit( xSuper );

    return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in SC15A CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListSc15aMenm[] = {

    /* misc field as a whole (for backward compat.) and individual bits */
    SYSP_PDE_U8X( "misc",           NULL, _EEOFF(mm_misc), "02", 0),
    SYSP_PDE_BOOL1( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "1", 0 ),
    SYSP_PDE_BOOL2( SYSP_updcdis,   NULL, _EEOFF(mm_misc), "0", 0 ),
    SYSP_PDE_BOOL3( "com3se",       NULL, _EEOFF(mm_misc), "0", 0 ),
    SYSP_PDE_BOOL4( SYSP_useflpar,  NULL, _EEOFF(mm_misc), "0", 
                    SYSP_PD_RESTART ),
    SYSP_PDE_BOOL5( "nobanner",     NULL, _EEOFF(mm_misc), "0", 0 ),
    
    /* selftest field as a whole (for backward compat.) and individual bits */
    SYSP_PDE_U8X( "selftest",       NULL, _EEOFF(mm_selftest), "10", 0),
    SYSP_PDE_BOOL0( SYSP_stdis,     NULL, _EEOFF(mm_selftest), "0", 0 ),
    SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),
    SYSP_PDE_BOOL2( "stdis_int_cf", NULL, _EEOFF(mm_selftest), "0", 0 ),
    SYSP_PDE_BOOL3( "stdis_sram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
    SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "1", 0 ),
    SYSP_PDE_BOOL5( "stdis_prtctrl",NULL, _EEOFF(mm_selftest), "0", 0 ),

    SYSP_PDE_BOOL0( "pci_pre_in_0",     NULL, _EEOFF(mm_pci_opts), "1", 0 ),
    SYSP_PDE_BOOL1( "pci_pre_in_1",     NULL, _EEOFF(mm_pci_opts), "0", 0 ),
    SYSP_PDE_BOOL2( "pci_snoop_in_0",   NULL, _EEOFF(mm_pci_opts), "1", 0 ),
    SYSP_PDE_BOOL3( "pci_snoop_in_1",   NULL, _EEOFF(mm_pci_opts), "1", 0 ),
    /*SYSP_PDE_BOOL4( "pci_en_in_2",    NULL, _EEOFF(mm_pci_opts), "1", 0 ),*/
    SYSP_PDE_BOOL5( "pci_pre_out_0",    NULL, _EEOFF(mm_pci_opts), "1", 0 ),
    SYSP_PDE_BOOL6( "pci_pre_out_1",    NULL, _EEOFF(mm_pci_opts), "0", 0 ),
    SYSP_PDE_BOOL7( "pci_pre_out_2",    NULL, _EEOFF(mm_pci_opts), "0", 0 ),
    SYSP_PDE_BOOL0( "pci_io_out_0",     NULL, _EEOFF(mm_pci_opts[1]), "0", 0 ),
    SYSP_PDE_BOOL1( "pci_io_out_1",     NULL, _EEOFF(mm_pci_opts[1]), "0", 0 ),
    SYSP_PDE_BOOL2( "pci_io_out_2",     NULL, _EEOFF(mm_pci_opts[1]), "1", 0 ),
    
    SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
    SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
    SYSP_PDE_END
};

#define _EEOFFX(x) offsetof(struct menmx_parms,x)
/** list of parameters in SC15A CPU EEPROM, menmon extended section */
static const SYSP_PAR_DESC SYSP_parListSc15aMenmX[] = {
    SYSP_PDE_CON0( _EEOFFX(mmx_con[0]), "8" ),  /* COM1 */
    SYSP_PDE_CON1( _EEOFFX(mmx_con[1]), "9" ),  /* COM2 */
    SYSP_PDE_CON2( _EEOFFX(mmx_con[2]), "f" ),  /* COM3 */
    SYSP_PDE_CON3( _EEOFFX(mmx_con[3]), "0" ),  /* (none) */
    SYSP_PDE_GCON( _EEOFFX(mmx_gcon),   "ff"),  /* use first available */
    SYSP_PDE_VMODE( _EEOFFX(mmx_vmode) ),
    SYSP_PDE_ECL(   _EEOFFX(mmx_ecl) ),
    SYSP_PDE_TTO(   _EEOFFX(mmx_tto) ),
    SYSP_PDE_TRIES( _EEOFFX(mmx_tries) ),
    SYSP_PDE_TDP(   _EEOFFX(mmx_tdp) ),
    SYSP_PDE_HDP(   _EEOFFX(mmx_hdp) ),
    SYSP_PDE_STWAIT(_EEOFFX(mmx_stwait), "30" ),
    SYSP_PDE_WDT(   _EEOFFX(mmx_wdt), "0" ),
	SYSP_PDE_NSPEED( SYSP_nspeed0, NULL, _EEOFF(mm_nspeed[0]), "AUTO" ),
    SYSP_PDE_U8X(   "side",       NULL, _EEOFFX(mmx_side), "a", 0),
    SYSP_PDE_BOOL0( "psr_main",   NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
    SYSP_PDE_BOOL1( "psr_io1",    NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
    SYSP_PDE_BOOL2( "psr_io2",    NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
    SYSP_PDE_BOOL3( "psr_spare",  NULL, _EEOFFX(mmx_hw_monitor), "1", 0 ),
    SYSP_PDE_BOOL4( "temp_crit_main",  NULL, _EEOFFX(mmx_hw_monitor), "0", 0 ),
    SYSP_PDE_BOOL5( "temp_low_main",   NULL, _EEOFFX(mmx_hw_monitor), "0", 0 ),
    SYSP_PDE_BOOL6( "temp_crit_io1",  NULL, _EEOFFX(mmx_hw_monitor), "0", 0 ),
    SYSP_PDE_BOOL7( "temp_low_io1",   NULL, _EEOFFX(mmx_hw_monitor), "0", 0 ),
    SYSP_PDE_BOOL0( "temp_crit_io2",  NULL, _EEOFFX(mmx_hw_monitor[1]), "0", 0 ),
    SYSP_PDE_BOOL1( "temp_low_io2",   NULL, _EEOFFX(mmx_hw_monitor[1]), "0", 0 ),
    SYSP_PDE_BOOL2( "temp_crit_spare",  NULL, _EEOFFX(mmx_hw_monitor[1]), "0", 0 ),
    SYSP_PDE_BOOL3( "temp_low_spare",   NULL, _EEOFFX(mmx_hw_monitor[1]), "0", 0 ),
    SYSP_PDE_PCIOUT( "pci_out_0", NULL, _EEOFFX(mmx_pci_out[0]),  "80000000,80000000,10000000" ),
    SYSP_PDE_PCIOUT( "pci_out_1", NULL, _EEOFFX(mmx_pci_out[6]),  "90000000,90000000,30000000" ),
    SYSP_PDE_PCIOUT( "pci_out_2", NULL, _EEOFFX(mmx_pci_out[12]), "00000000,00000000,00000000" ),
    SYSP_PDE_PCIIN(  "pci_in_0",  NULL, _EEOFFX(mmx_pci_in[0]),   "00000000,00000000,00000000" ),
    SYSP_PDE_PCIIN(  "pci_in_1",  NULL, _EEOFFX(mmx_pci_in[6]),   "00000000,00000000,00000000" ),
        
    SYSP_PDE_UXX( _EEOFFX(mmx_usr[0]) ),
    
    SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in SC15A boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListSc15aFlStr[] = {
    SYSP_PDE_STR( SYSP_mmstartup, SYSP_startup, _EEOFFF(fl_startup), "", 
                  SC15A_FLASH_STARTUP_LEN, 0 ),
    SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "", 
                  SC15A_FLASH_KERPAR_LEN, 0 ),
    
    SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListSc15aMmpara[] = {
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
/** Probe for SC15A CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in SC15A 
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsSc15aCpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
    int rv;
    SYSP_NV_STORAGE *nvsEe, *nvsFl;
    SYSP_NVS_ADR nvsAdr;
    int useFlashStrings;

    /* create EEPROM NVS section */
    nvsAdr.type = SYSP_NVSADR_SMB;
    nvsAdr.addr.smb.bus = 0;
    nvsAdr.addr.smb.dev = SC15A_EE_SROM_MENMON;

    if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
                                 &xSuper->s, "", nvsAdr, 
                                 sizeof(SC15A_EEPROM) )) == NULL )
        return SYSPARAM_NO_RESOURCE;

    /* Menmon parameters */
    if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListSc15aMmpara,
                                xSuper->initBlk.mmParaStringMaxLen, 
                                xSuper->initBlk.mmParaStringAdrs,
                                xSuper->initBlk.mmParaStringMaxLen )) != 0 )
        return rv;

    /* CPU EEPROD section */
    if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
        return rv;
        
    /* menm section */
    if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( SC15A_EEPROM, menm),
                                 sizeof( struct menm_parms ),
                                 SYSP_parListSc15aMenm, SC15A_EEID_MENM, 
                                 NULL )) != 0 )
        return rv;

    /* menmx section */
    if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menmx", 
                                   offsetof( SC15A_EEPROM, menmx ),
                                   sizeof( struct menmx_parms ),
                                   SYSP_parListSc15aMenmX, SC15A_EEID_MENMX, 
                                   NULL )) != 0 )
        return rv;

    MM_Idle();
    SYSP_NvsAdd( &xSuper->s, nvsEe ); 
    useFlashStrings = SysParamBool( SYSP_useflpar, FALSE ); 

    MM_Idle();
    if( useFlashStrings == FALSE ){
        /* startup string */
        if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
                                     offsetof( SC15A_EEPROM, startup ),
                                     SC15A_EE_STARTUP_LEN, NULL)) != 0 )
            return rv;
    }

    /* vxbline */
    MM_Idle();
    if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( SC15A_EEPROM, vxBline ),
                                SC15A_VXBLINE_LEN, 
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
                                     SC15A_FLASH_LAST_SECTOR_SIZE )) == NULL )
            return SYSPARAM_NO_RESOURCE;

        MM_Idle();

        if( (rv = SYSP_MpGrpCreateStd( 
                 nvsFl, "flstr", 
                 offsetof( SC15A_BFLASH_PARMS, flstr ),
                 sizeof( struct flstr_parms ),
                 SYSP_parListSc15aFlStr, SC15A_FLID_STR, 
                 NULL )) != 0 )
            return rv;
        
        MM_Idle();
        SYSP_NvsAdd( &xSuper->s, nvsFl );
    }

    return 0;
}

