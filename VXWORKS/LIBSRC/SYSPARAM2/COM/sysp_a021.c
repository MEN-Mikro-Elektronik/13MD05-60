/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_em09.c
 *
 *      \author  aw
 *        $Date: 2009/11/20 20:06:06 $
 *    $Revision: 2.22 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  EM09 specific part
 *               cloned from EM03
 *
 *    \switches  SYSP_EM09_VARIANT_A017 - A017 specific eeprom layout
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em09.c,v $
 * Revision 2.22  2009/11/20 20:06:06  rt
 * R: 1.) Support for EM9A.
 * M: 1.) Added mm_nspeed_3 and SYSP_nmac3 parameters.
 *
 * Revision 2.21  2008/11/17 13:23:44  cs
 * R: MM parameter string was updated at every start of the OS, MM settings were overwritten!
 * M: only call ->postProcessSetParam() when compiled for MenMon
 *
 * Revision 2.20  2008/11/04 16:05:09  rt
 * R:1. EE-STARTUP parameter is not available if EE-USEFLPAR is set to 1
 * M:1. Added "startup" alias for "mmstartup"
 *
 * Revision 2.19  2008/08/04 15:35:49  rt
 * R:1. Improve support for XM50
 * M:1.a) Use 2nd I2C bus for carrier board
 *     b) Added stdis_pci, stdis_rtc parameter
 *
 * Revision 2.18  2008/07/15 15:17:02  cs
 * R: changes made in Rev 2.17 cause serious compiler warnings with VxWorks
 * M: change preprocessor switches to match needs of OS9 and VxWorks
 *
 * Revision 2.17  2008/06/27 14:37:04  aw
 * R: TOOL_FAMILY and diab are undefined under UCLINUX
 * M: Set TOOL_FAMILY and diab
 *
 * Revision 2.16  2008/06/25 13:47:04  rt
 * R:1. Improve support for XM50
 * M:1.a) Special wdt implementation: Only values 0,11,260 valid
 *     b) Default for CON1 is 0 now
 *
 * Revision 2.15  2008/06/16 13:39:50  rt
 * R:1. Give hint to user if eeprom parameter change needs reset to take effect.
 *   2. Improve support for XM50.
 * M:1.a) Set flag SYSP_PD_NEEDRESET for parameters wich need reset.
 *   2.a) Add eeprom parameter stdis_sram.
 *
 * Revision 2.14  2008/05/09 15:19:41  rt
 * added:
 * -A017: stdis_ssd
 * changed:
 * -A017: stdis_nand is enabled per default
 *
 * Revision 2.13  2008/04/29 17:19:51  rt
 * added:
 * -vme_irq
 * -SYSP_fpga
 * -SYSP_buspci0, SYSP_buspci1, SYSP_buspci2
 * fixed:
 * -avoid compiler warnings
 *
 * Revision 2.12  2008/04/18 14:47:23  cs
 * R: part of file not compilable with VxWorks DIAB compiler
 * M: add precompiler switches to skip not compilable parts
 *
 * Revision 2.11  2008/04/11 13:41:50  rt
 * fixed:
 * - do not test alignment for disabled VME windows
 * - enable COM2 instead TOUCH for A017
 * added:
 * - nspeed3 parameter
 * - nmac3 parameter
 * changed:
 * - inbound VME windows disabled per default
 *
 * Revision 2.10  2008/01/09 11:12:08  rt
 * added:
 * -support for A017
 *
 * Revision 2.9  2007/12/04 17:24:13  rt
 * make compilable again
 *
 * Revision 2.8  2007/11/23 15:26:44  cs
 * removed G_allowedChars*, allready defined in sysp_paramhelp.c
 * renamed G_descTbl including postfix Em9
 *
 * Revision 2.7  2007/09/21 12:01:07  rt
 * added:
 * - stdis_fpga
 * - stdis_fubu
 * removed:
 * - stdis_prtctrl
 *
 * Revision 2.6  2007/09/20 17:57:30  rt
 * added:
 * - SYSP_fram0
 *
 * Revision 2.5  2007/09/12 15:01:51  rt
 * added:
 * - SYSP_usbdp
 * - stdis_usb
 *
 * Revision 2.4  2007/08/09 15:56:42  aw
 * add switch OS9000 because not compilable under OS9000
 *
 * Revision 2.3  2007/08/03 18:32:21  rt
 * changed: default for selftest ignore is 0
 *
 * Revision 2.2  2007/07/11 10:54:21  rt
 * - EEPROM parameters updated
 * - added some board specific help descriptors
 *
 * Revision 2.1  2007/06/18 09:20:07  aw
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>
#include <MEN/em09_eeprom.h>
#include <MEN/em09_cfg.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* variant specific defaults */

#define EM09_DFLT_WDT		"0"
#define EM09_DFLT_MMSTARTUP	""

/** VME inbound window */
#define SYSP_PDE_VMEIN( _nm, _alias, _adrSpace, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawVme##_adrSpace, SYSP_FromRawVme##_adrSpace, \
   SYSP_ValidateVmeIn, _deflt, _adrSpace, \
   (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET) }

/* WDT for PLD watchdog */
#define SYSP_PDE_WDT_PLD(_eeoff, _def) \
 { SYSP_wdt, NULL, _EEOFF(mm_wdt), SYSP_ToRawS16D, SYSP_FromRawS16D, \
   SYSP_ValidateWdtPld, EM09_DFLT_WDT, 0, 0 }


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */


/* not compilable under OS9 */
/* not compilable under VXWORKS with DIAB */
#if ( !(defined(OS9000)) && \
      !(defined(SYSP_CONFIG_VXWORKS) && TOOL_FAMILY == diab) )
static const char G_hwLongDesc[] = "(1=hardware monitor reset enabled)";
static const char G_allowedCharsHexEm9[] = "[0-9A-Fa-f]";
static const char G_allowedCharsDecEm9[] = "[0-9]";
static const char G_allowedCharsBoolEm9[] = "[01]";

/**********************************************************************/
/** Table that describes all statically known board specific parameters
 **/
static const SYSP_HELP_DESC G_HelpDescEm9[] = {
    {
        .parName = "psr1v0",
        .shortDesc = "Supervise 1.%dV voltage",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBoolEm9
    },
    {
        .parName = "psr2v0",
        .shortDesc = "Supervise 2.%dV voltage",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBoolEm9
    },
    {
        .parName = "psr3v0",
        .shortDesc = "Supervise 3.%dV voltage",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBoolEm9
    },
    {
        .parName = "psr5v",
        .shortDesc = "Supervise 5V voltage",
        .longDescFirstLine = G_hwLongDesc,
        .allowedChars = G_allowedCharsBoolEm9
    },
    {
        .parName = "c-mode_com10",
        .shortDesc = "Mode for switchable UART (COM1%d)",
        .longDescFirstLine = "Possible values: RS232 RS422 RS485",
        .allowedChars = "[0-9A-Za-z]"
    },
    {
        .parName = "vme_in_a24",
        .shortDesc = "VME A24 inbound (VME->A17) mapping",
        .longDescFirstLine = "<VME addr, PCI addr, size (0=disabled)>\nsyntax: xx0000,xxxx0000,xx0000 (x=0..F)",
        .allowedChars = G_allowedCharsHexEm9
    },
    {
        .parName = "vme_in_a32",
        .shortDesc = "VME A32 inbound (VME->A17) mapping",
        .longDescFirstLine = "<VME addr, PCI addr, size (0=disabled)>\nsyntax: xxxx0000,xxxx0000,xxxx0000 (x=0..F)",
        .allowedChars = G_allowedCharsHexEm9
    },
    {
        .parName = "vme_in_a64",
        .shortDesc = "VME A64 inbound (VME->A17) mapping",
        .longDescFirstLine = "<VME addr, PCI addr, size (0=disabled)>\nsyntax: xxxxxxxxxx000000,xx000000,xx000000 (x=0..F)",
        .allowedChars = G_allowedCharsHexEm9
    },
    {
        .parName = "vme_irq",
        .shortDesc = "VME interrupt levels",
        .longDescFirstLine = "Controls which interrupt levels should be enabled\n(bit7 = level 7, ..., bit1 = level 1)",
        .allowedChars = G_allowedCharsHexEm9
    },
    { .parName = NULL   /* terminator */ }
};

static SYSP_HELP_DESC_TBL G_descTblEm09 = { NULL, G_HelpDescEm9 };
#endif /* !OS9000 & !VXWORKS/DIAB */
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEm09CpuProbe( SYSP_MMSMB_SUPER *xSuper );

#ifdef SYSP_EM09_VARIANT_A017
static u_int64 SYSPEM9_StrToUll( const char *nptr, char **endptr, int base)
{
	u_int64 v=0;

	/* skip white space */
	while( (*nptr == ' ') || (*nptr == '\t') )
		nptr++;

	if (*nptr == '+') ++nptr;
	if (!base) {
		if (*nptr=='0') {
			base=8;
			if ((*(nptr+1)=='x')||(*(nptr+1)=='X')) {
				nptr+=2;
				base=16;
			}
		}
		else
			base=10;
	}
	while(*nptr) {
		register unsigned char c=*nptr;
		c=(c>='a'?c-'a'+10:c>='A'?c-'A'+10:c-'0');
		if (c>=base)
			break;
		v=v*base+c;
		++nptr;
	}
	if (endptr) *endptr=(char *)nptr;
	return v;
}

/**********************************************************************/
/** from VME outbound/inbound window configuration
 */
static int SYSP_FromRawVme( char *dest, int destLen, void *src, int adrSpace )
{
    char buf[50];
    u_int8 *p8 = (u_int8*)src;
    u_int16 *p16 = (u_int16*)src;
    char *s = buf;

	switch( adrSpace )
	{
		case 24:
			s += SYSP_SNPRINTF( s, 256, "%02X0000,%04X0000,%02X0000",
							   p8[0], SYSP_FROMBE16(*((u_int16*)&p8[1])), p8[3] );
			break;

		case 32:
			s += SYSP_SNPRINTF( s, 256, "%04X0000,%04X0000,%04X0000",
							   SYSP_FROMBE16(p16[0]), SYSP_FROMBE16(p16[1]),
							   SYSP_FROMBE16(p16[2]) );
			break;

		case 64:
			s += SYSP_SNPRINTF( s, 256, "%08X%02X000000,%04X0000,%02X000000",
							   SYSP_FROMBE32(*((u_int32*)&p8[0])),
							   p8[4],
							   SYSP_FROMBE16(*((u_int16*)&p8[5])),
							   p8[7] );
			break;

		default:
			return SYSPARAM_INVAL_PARAM;
	}

    return SYSP_StrSafeCpy( dest, buf, destLen );
}

static int SYSP_FromRawVme24( char *dest, int destLen, void *src )
{
	return SYSP_FromRawVme( dest, destLen, src, 24 );
}

static int SYSP_FromRawVme32( char *dest, int destLen, void *src )
{
	return SYSP_FromRawVme( dest, destLen, src, 32 );
}

static int SYSP_FromRawVme64( char *dest, int destLen, void *src )
{
	return SYSP_FromRawVme( dest, destLen, src, 64 );
}

/**********************************************************************/
/** to VME outbound/inbound window configuration
 */
static int SYSP_ToRawVme( void *dest, const char *src, int adrSpace )
{
    int i;
    u_int64 n;
    u_int8 *p8 = (u_int8 *)dest;
    u_int16 *p16 = (u_int16 *)dest;

	switch( adrSpace )
	{
		case 24:
			n = SYSP_STRTOUL( src, (char**)&src, 16 );
        	*(p8++) = (u_int8) (n>>16);
        	++src;
			n = SYSP_STRTOUL( src, (char**)&src, 16 );
        	*((u_int16*)(p8)) = SYSP_TOBE16( (u_int16)(n>>16) );
        	p8+=2;
        	++src;
			n = SYSP_STRTOUL( src, (char**)&src, 16 );
        	*(p8) = (u_int8) (n>>16);
			break;

		case 32:
		    for( i=0; i<3; ++i )
		    {
		        n = SYSP_STRTOUL( src, (char**)&src, 16 );
		        *(p16++) = SYSP_TOBE16( (u_int16)(n>>16) );
		        ++src;
		    }
		    break;

		case 64:
			n = SYSPEM9_StrToUll( src, (char**)&src, 16 );
        	*((u_int32*)(p8)) = SYSP_TOBE32( (u_int32)(n>>32) );
        	p8+=4;
        	*(p8++) = (u_int8) (n>>24);
        	++src;
			n = SYSP_STRTOUL( src, (char**)&src, 16 );
        	*((u_int16*)(p8)) = SYSP_TOBE16( (u_int16)(n>>16) );
        	p8+=2;
        	++src;
			n = SYSP_STRTOUL( src, (char**)&src, 16 );
        	*(p8) = (u_int8) (n>>24);
			break;

		default:
			return SYSPARAM_INVAL_PARAM;
	}

    return 0;
}

static int SYSP_ToRawVme24( void *dest, const char *src )
{
	return SYSP_ToRawVme( dest, src, 24 );
}

static int SYSP_ToRawVme32( void *dest, const char *src )
{
	return SYSP_ToRawVme( dest, src, 32 );
}

static int SYSP_ToRawVme64( void *dest, const char *src )
{
	return SYSP_ToRawVme( dest, src, 64 );
}

/**********************************************************************/
/** check VME inbound window configuration
 */
int SYSP_ValidateVmeIn( const char *src, int adrSpace )
{
    int i;
    u_int64 n[3];
    u_int64 mask[3];

    for( i=0; i<3; ++i )
    {
        n[i] = SYSPEM9_StrToUll( src, (char**)&src, 16 );

        if( ((i!=2) && *src != ',' ) ||
            ((i==2) && *src != '\0' ))
            return SYSPARAM_INVAL_VALUE;
        ++src;
    }

    /* alignment error? */
    if( (u_int32)n[2] /*window enabled?*/ &&
		((u_int32)n[0]%(u_int32)n[2] || (u_int32)n[1]%(u_int32)n[2]) )
    {
		DBG_Write(DBH, "\n*** %s: Allignment error\n", __FUNCTION__);
		return SYSPARAM_INVAL_VALUE;
    }

    /* use only some high address bits (save eeprom space) */
	switch( adrSpace )
	{
		case 24:
			mask[0] = 0xFF0000;		/* vme adr */
			mask[1] = 0xFFFF0000; 	/* pci adr */
			mask[2] = 0xFF0000;		/* size */
			break;

		case 32:
			mask[0] = 0xFFFF0000;	/* vme adr */
			mask[1] = 0xFFFF0000; 	/* pci adr */
			mask[2] = 0xFFFF0000;	/* size */
		    break;

		case 64:
			mask[0] = 0xFFFFFFFFFF000000ULL;	/* vme adr */
			mask[1] = 0xFF000000; 				/* pci adr */
			mask[2] = 0xFF000000;				/* size */
			break;

		default:
			return SYSPARAM_INVAL_PARAM;
	}

    if( n[0] & ~mask[0] || n[1] & ~mask[1] || n[2] & ~mask[2] )
    {
    	DBG_Write(DBH, "\n*** %s: Syntax error, lower address bits"
					" have to be zero\n", __FUNCTION__);
        return SYSPARAM_INVAL_VALUE;
    }

    return 0;
}
#endif /*SYSP_EM09_VARIANT_A017*/

#ifdef SYSP_EM09_VARIANT_XM50
/**********************************************************************/
/** check WDT parameter for XM50 PLD watchdog
 */
int SYSP_ValidateWdtPld( const char *src, int notused )
{
	const char *s = src;
	int wdt;

	/* decimal? */
	while( *s ) {
		if( !IN_RANGE(*s, '0', '9') )
			return SYSPARAM_INVAL_VALUE;

		++s;
	}

	wdt = SYSP_STRTOUL( src, NULL, 10 );

	/* time-out supported? */
    if( wdt!=0 && wdt!=11 /*short time-out*/ && wdt != 260 /*long time-out*/ )
    {
		DBG_Write(DBH, "PLD watchdog supports only following values:\n"\
					   "  0 = disabled\n 11 = 1.12 sec\n260 = 26.0 sec\n");
		return SYSPARAM_INVAL_VALUE;
    }

    return 0;
}
#endif /*SYSP_EM09_VARIANT_XM50*/

/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM09
 *
 * Probes for CPU EEPROM and possible ESM carrier boards
 * \return sysparam error code if fatal error
 */
static int Em09Init( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

#if ( !(defined(OS9000)) && \
      !(defined(SYSP_CONFIG_VXWORKS) && TOOL_FAMILY == diab) )
    /* add EM09 board specific help descriptors */
    SYSP_AddParamTable( &G_descTblEm09 );
#endif /* !OS9000 & !VXWORKS/DIAB */

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm09CpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* probe ESM carrier */
	rv = SYSP_EsmCarrierProbe( super, EM09CFG_IIC_CBOARD );
	if( rv == SYSPARAM_EEPROM_ERROR )
		rv = 0;					/* non fatal */

#if defined(SYSP_CONFIG_MENMON)
	/* init fake parameters for netif devices */
	SYSP_MmNetIfParamsCreate( super );

	/* build MM para string. Do this from MenMon only!! */
	super->postProcessSetParam( super, NULL, 0 );

#else
	/* TBD: read the MM para string to overwrite SYSPARAM,
	 * e.g in case the structures where filled with default values only! */
#endif

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for EM09
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int EM09_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Em09Init;
	SYSP_SuperMmsmbInit( xSuper );	/* just init function pointers for access */

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EM09 CPU EEPROM, menmon section */
static const SYSP_PAR_DESC SYSP_parListEm09Menm[] = {

	/* misc field */
	SYSP_PDE_BOOL0( SYSP_ldlogodis, NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_updcdis, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL2( "lxdcache", 	NULL, _EEOFF(mm_misc), "1", 0 ),
	SYSP_PDE_BOOL3( SYSP_useflpar, 	NULL, _EEOFF(mm_misc), "0",
					SYSP_PD_RESTART ),
	SYSP_PDE_BOOL4( SYSP_nobanner, 	NULL, _EEOFF(mm_misc), "0", 0 ),
	SYSP_PDE_BOOL5( "noecc", 		NULL, _EEOFF(mm_misc), "0", SYSP_PD_NEEDRESET ),

	/* selftest field as a whole (for backward compat.) and individual bits */
	SYSP_PDE_BOOL0( SYSP_stdis, 	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stignfault,NULL, _EEOFF(mm_selftest), "0", 0 ),

#ifdef SYSP_EM09_VARIANT_A017
	SYSP_PDE_BOOL2( SYSP_stdis_nand,NULL, _EEOFF(mm_selftest), "1", 0 ),
	SYSP_PDE_BOOL3( "stdis_fram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_fpga",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL6( "stdis_ssd",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stdis_ether, 	NULL, _EEOFF(mm_selftest2), "0", 0 ),
#elif defined( SYSP_EM09_VARIANT_XM50 )
	SYSP_PDE_BOOL3( "stdis_fram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_pci",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL6( "stdis_rtc",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL0( "stdis_sram", 	NULL, _EEOFF(mm_selftest2), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stdis_ether, 	NULL, _EEOFF(mm_selftest2), "0", 0 ),
#elif defined( SYSP_EM09_VARIANT_STD )
	SYSP_PDE_BOOL2( SYSP_stdis_nand,NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL3( "stdis_fram",   NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL4( "stdis_touch",  NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL5( "stdis_fpga",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL6( SYSP_stdis_usb,	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL7( "stdis_fubu",	NULL, _EEOFF(mm_selftest), "0", 0 ),
	SYSP_PDE_BOOL0( "stdis_sram", 	NULL, _EEOFF(mm_selftest2), "0", 0 ),
	SYSP_PDE_BOOL1( SYSP_stdis_ether, 	NULL, _EEOFF(mm_selftest2), "0", 0 ),
#endif /*SYSP_EM09_VARIANT_STD*/

	SYSP_PDE_CBR(   _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX(   _EEOFF(mm_usr[0]) ),

	SYSP_PDE_CON0( _EEOFF(mm_con[0]), "8" ),	/* COM1 */
#ifdef SYSP_EM09_VARIANT_A017
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "9" ),  	/* COM2 */
#elif defined (SYSP_EM09_VARIANT_XM50)
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "0" ),  	/* (none) */
#else
	SYSP_PDE_CON1( _EEOFF(mm_con[1]), "a" ),  	/* TOUCH */
#endif /*SYSP_EM09_VARIANT_A017*/
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
#ifdef SYSP_EM09_VARIANT_XM50
	SYSP_PDE_WDT_PLD( _EEOFF(mm_wdt), EM09_DFLT_WDT ),
#else
	SYSP_PDE_WDT( _EEOFF(mm_wdt), EM09_DFLT_WDT ),
#endif /*SYSP_EM09_VARIANT_XM50*/
	SYSP_PDE_U8D( "eccsth", 		NULL, _EEOFF(mm_eccSingleTh), "32",
				  (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET)),
	SYSP_PDE_NSPEED( SYSP_nspeed0, NULL, _EEOFF(mm_nspeed[0]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed1, NULL, _EEOFF(mm_nspeed[1]), "AUTO" ),
	SYSP_PDE_NSPEED( SYSP_nspeed2, NULL, _EEOFF(mm_nspeed[2]), "AUTO" ),
#if defined( SYSP_EM09_VARIANT_A017 ) || defined( SYSP_EM09_VARIANT_STD )
	SYSP_PDE_NSPEED( SYSP_nspeed3, NULL, _EEOFF(mm_nspeed_3), "AUTO" ),
#endif
#ifdef SYSP_EM09_VARIANT_STD /*remove psr...???*/
	SYSP_PDE_BOOL0( "psr1v1",   NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
	SYSP_PDE_BOOL2( "psr1v2",   NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
	SYSP_PDE_BOOL3( "psr1v8",   NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
	SYSP_PDE_BOOL4( "psr2v5",  	NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
	SYSP_PDE_BOOL5( "psr3v3",  	NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
	SYSP_PDE_BOOL5( "psr5v",  	NULL, _EEOFF(mm_powerReset), "1", SYSP_PD_NEEDRESET ),
#endif /*SYSP_EM09_VARIANT_STD*/
#ifdef SYSP_EM09_VARIANT_A017
	SYSP_PDE_VMEIN( "vme_in_a24", NULL, 24 /*A24*/, _EEOFF(mm_vme_in[0]), "000000,20000000,000000" ),
	SYSP_PDE_VMEIN( "vme_in_a32", NULL, 32 /*A32*/, _EEOFF(mm_vme_in[4]), "00000000,20000000,00000000" ),
	SYSP_PDE_VMEIN( "vme_in_a64", NULL, 64 /*A64*/, _EEOFF(mm_vme_in[10]), "0000000000000000,20000000,00000000" ),
	SYSP_PDE_U8X( "vme_irq", NULL, _EEOFF(mm_vme_irq), "0", (SYSP_PD_MMPARASTR | SYSP_PD_NEEDRESET) ),
#endif
	SYSP_PDE_END
};

#define _EEOFFF(x) offsetof(struct flstr_parms,x)
/** list of parameters in EM09 boot flash, long string */
static const SYSP_PAR_DESC SYSP_parListEm09FlStr[] = {
	SYSP_PDE_STR( SYSP_mmstartup, SYSP_startup, _EEOFFF(fl_startup),
				  EM09_DFLT_MMSTARTUP,
				  EM09_FLASH_STARTUP_LEN, 0 ),
	SYSP_PDE_STR( SYSP_kerpar, NULL, _EEOFFF(fl_kerpar), "",
				  EM09_FLASH_KERPAR_LEN, 0 ),

	SYSP_PDE_END
};

/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEm09Mmpara[] = {
	SYSP_mem0,
	SYSP_mem1,
	SYSP_flash0,
	SYSP_fram0,
	SYSP_cpu,
	SYSP_cpuclkhz,
	SYSP_memclkhz,
	SYSP_pciclkhz,
	SYSP_MPC85XX_ccbclkhz,
	SYSP_MPC85XX_brgclkhz,
	SYSP_MPC85XX_immr,
	SYSP_clun,
	SYSP_dlun,
	SYSP_usbdp,
	SYSP_rststat,
	SYSP_mmst,
	SYSP_mm,
#ifndef SYSP_EM09_VARIANT_XM50
	SYSP_fpga,
#endif
	SYSP_cons,
	SYSP_nmac0,
	SYSP_nmac1,
	SYSP_nmac2,
#ifdef SYSP_EM09_VARIANT_A017
	SYSP_nmac3,
	SYSP_buspci0,	/* PCI interface */
	SYSP_buspci1,	/* PCIe interface */
#elif defined( SYSP_EM09_VARIANT_STD )
	SYSP_nmac3,
#elif defined( SYSP_EM09_VARIANT_XM50 )
	SYSP_buspci0,	/* PCI1 interface */
	SYSP_buspci1,	/* PCIe interface */
	SYSP_buspci2,	/* PCI2 interface */
#endif
	NULL
};

/**********************************************************************/
/** Probe for EM09 CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EM09
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEm09CpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsEe, *nvsFl;
	SYSP_NVS_ADR nvsAdr;
	int useFlashStrings;

	/* create EEPROM NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = EM09_SMB_SROM_MM;

	if( (nvsEe = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "cpuee",
								 &xSuper->s, "", nvsAdr,
								 sizeof(EM09_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvsEe, "mmpara", SYSP_parListEm09Mmpara,
								xSuper->initBlk.mmParaStringMaxLen,
								xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsEe, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvsEe, "menm", offsetof( EM09_EEPROM, menm),
								 sizeof( struct menm_parms ),
								 SYSP_parListEm09Menm, EM09_EEID_MENM,
								 NULL )) != 0 )
		return rv;


	SYSP_NvsAdd( &xSuper->s, nvsEe );

	useFlashStrings = SysParamBool( SYSP_useflpar, FALSE );

	if( useFlashStrings == FALSE ){
		/* startup string */
		if( (rv = SYSP_StrGrpCreate( nvsEe, SYSP_mmstartup, SYSP_startup,
									 offsetof( EM09_EEPROM, startup ),
									 EM09_EEPROM_STARTUP_LEN,
									 EM09_DFLT_MMSTARTUP)) != 0 )
			return rv;
	}

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvsEe, offsetof( EM09_EEPROM, vxBline ),
								EM09_VXBLINE_LEN,
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
									 EM09CFG_BFLASH_SYSP_SIZE )) == NULL )
			return SYSPARAM_NO_RESOURCE;

		if( (rv = SYSP_MpGrpCreateStd(
				 nvsFl, "flstr",
				 offsetof( EM09_BFLASH_PARMS, flstr ),
				 sizeof( struct flstr_parms ),
				 SYSP_parListEm09FlStr, EM09_FLID_STR,
				 NULL )) != 0 )
			return rv;

		SYSP_NvsAdd( &xSuper->s, nvsFl );
	}

	return 0;
}


