/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_paramhelp.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/07/11 10:57:28 $
 *    $Revision: 2.11 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Provide help text for commonly used parameters
 *
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_paramhelp.c,v $
 * Revision 2.11  2007/07/11 10:57:28  rt
 * added:
 * - c-tcal help descriptor
 * - c-pnum help descriptor
 *
 * Revision 2.10  2007/03/22 12:06:52  ufranke
 * added
 *  - in help
 *    default port numbers for HTTPd and TELNETd
 *
 * Revision 2.9  2006/10/13 19:47:12  cs
 * added:
 *   + SYSP_gstatshow help
 *
 * Revision 2.8  2006/07/17 08:16:54  ag
 * updated description texts for: vmeirqenb, vme_a24sa, vme_a32ma, vme_req,
 * pci_stgath, pci_specrd, vmode, kmap
 *
 * Revision 2.7  2006/04/19 19:38:41  rt
 * cosmetics
 *
 * Revision 2.6  2006/04/19 18:50:12  rt
 * SYSP_AddParamTable added (for board spec parameter descriptions)
 *
 * Revision 2.5  2006/01/20 10:19:03  ag
 * added VME and PCI options for A012/A015
 *
 * Revision 2.4  2005/06/23 15:54:44  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.3  2005/02/02 16:03:15  kp
 * replaced wdt1/2 by wdt
 *
 * Revision 2.2  2005/01/14 09:36:20  kp
 * added description  for SYSP_netname
 *
 * Revision 2.1  2005/01/13 15:26:03  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

#define _ISDIGIT(c) ((c)>='0' && (c)<='9')

static const char G_allowedCharsIp[] = "[0-9.]";
static const char G_allowedCharsHex[] = "[0-9A-Fa-f]";
static const char G_allowedCharsDec[] = "[0-9]";
static const char G_allowedCharsBool[] = "[01]";

static const char G_wdtLongDesc1[] = "in 1/10s, 0=disable watchdog";

/**********************************************************************/
/** Table that describes all statically known parameters
 **/
static const SYSP_HELP_DESC G_descTbl[] = {
                   /* 0123456789012345678901234567890123456789 */
	{	/* also used for con1..n */
		.parName = SYSP_con0,
		.shortDesc = "CLUN of console %d",
		.longDescFirstLine = "(FF = autoselect)",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = SYSP_gcon,
		.shortDesc = "CLUN of grafics screen",
		.longDescFirstLine = "(FF = autoselect)",
		.allowedChars = G_allowedCharsHex
	},
	/* todo: vmode */
	{
		.parName = SYSP_cbr,
		.aliasName = SYSP_baud,
		.shortDesc = "Baudrate of all UART consoles",
		.longDescFirstLine = "(e.g. 9600)",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_bsadr,
		.shortDesc = "Bootstrapper address",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = SYSP_ecl,
		.shortDesc = "CLUN of MENMON network interface",
		.longDescFirstLine = "(FF = autoselect)",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = SYSP_tto,
		.shortDesc = "Min. timeout between network retries",
		.longDescFirstLine = "in seconds",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_tries,
		.shortDesc = "Number of network (re)tries",
		.longDescFirstLine = "0=infinite, 1=no retries, 255=default (20) tries",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_tdp,
		.shortDesc = "Telnet server port",
		.longDescFirstLine = "0=disable telnet server, -1=default port 23",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_hdp,
		.shortDesc = "HTTP server port",
		.longDescFirstLine = "0=disable HTTP server, -1=default port 10",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_wdt,
		.shortDesc = "Watchdog timer for OS boot",
		.longDescFirstLine = G_wdtLongDesc1,
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_stignfault,
		.shortDesc = "Ignore post failure",
		.longDescFirstLine = "If 1, continue boot on POST failure",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = SYSP_stdis,
		.shortDesc = "Disable POST",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = SYSP_stwait,
		.shortDesc = "Time to wait before booting",
		.longDescFirstLine = "In 1/10 s",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = SYSP_ldlogodis,
		.shortDesc = "Disable load of boot logo",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = SYSP_updcdis,
		.shortDesc = "Disable automatic update check",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = SYSP_mmstartup,
		.aliasName = SYSP_startup,
		.shortDesc = "Startup string",
		.longDescFirstLine = "List of commands to execute on startup",
		.longDescContinuation = "e.g. \"DBOOT 0; NBOOT FILE=BOOT\""
	},
	{
		.parName = SYSP_kerpar,
		.shortDesc = "Linux kernel parameters",
		.longDescFirstLine = "e.g \"ip=auto rw root=/dev/nfs\""
	},
	{
		.parName = SYSP_useflpar,
		.shortDesc = "Store long parameter strings in flash",
		.allowedChars = G_allowedCharsBool
	},
	{	/* also used for nmac1..n */
		.parName = SYSP_nmac0,
		.shortDesc = "MAC address of network interface %d",
	},
	{	/* also used for nspeed1..n */
		.parName = SYSP_nspeed0,
		.shortDesc = "Speed setting of network interface %d",
	},
	{
		.parName = SYSP_bootfile,
		.aliasName = SYSP_bf,
		.shortDesc = "Name of bootfile",
		.longDescFirstLine = ""
	},
	{
		.parName = SYSP_netip,
		.aliasName = "e",
		.shortDesc = "IP Address/Subnet Mask of this Machine",
		.longDescFirstLine = "(e.g. 192.168.1.1:FFFFFF00)",
		.allowedChars = "[0-9.a-fA-F]"
	},
	{
		.parName = SYSP_netaddr,
		.shortDesc = "IP Address of this Machine",
		.longDescFirstLine = "(e.g. 192.168.1.1)",
		.allowedChars = G_allowedCharsIp
	},
	{
		.parName = SYSP_netsm,
		.shortDesc = "Subnet Mask of this Machine",
		.longDescFirstLine = "(e.g. FFFFFF00)",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = SYSP_netgw,
		.aliasName = "g",
		.shortDesc = "IP Default Gateway Address",
		.longDescFirstLine = "(e.g. 192.168.1.200)",
		.allowedChars = G_allowedCharsIp
	},
	{
		.parName = SYSP_nethost,
		.aliasName = "h",
		.shortDesc = "IP Address of Boot Host",
		.longDescFirstLine = "(e.g. 192.168.1.5)",
		.allowedChars = G_allowedCharsIp
	},
	{
		.parName = SYSP_netname,
		.aliasName = "tn",
		.shortDesc = "Hostname of this Machine",
		.longDescFirstLine = "(e.g. host.example.com)",
		.allowedChars = "[0-9A-Za-z_,.]"
	},
	{
		.parName = "vmeirqenb",
		.shortDesc = "[<mask>] VMEbus IRQ levels enable mask",
		.longDescFirstLine = "Bit7: level7, Bit6: level6 ... Bit 1: level1, Bit0: ACFAIL.\n"
		                     "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = "vme_a24sa",
		.shortDesc = "[<val>] VME A24 slave address [0..F]",
		.longDescFirstLine = "set compare address for VME A23..A20 (1MB steps).\n"
		                     "a value of FF disables the access.\n"
		                     "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = "vme_a32ma",
		.shortDesc = "[<val>] VME A32 master prefix [0..F]",
		.longDescFirstLine = "set address prefix for A31..A24.\n"
		                     "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = "vme_req",
		.shortDesc = "[<0/1>] VME Req. method (0=ROR 1=RWD)",
		.longDescFirstLine = "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "pci_stgath",
		.shortDesc = "<0/1> dis/en CPU->PCI store gathering",
		.longDescFirstLine = "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "pci_specrd",
		.shortDesc = "<0/1> dis/en PCI->CPU memory spec reads",
		.longDescFirstLine = "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "vmode",
		.shortDesc = "[<code>]Graphics card VESA video mode",
		.longDescFirstLine = "           640x480 800x600 1024x768 1280x1024\n"
                             " 256 colors:   101     103      105       107\n"
                             " 64k colors:   111     114      117       11A\n"
                             " 16M colors:   112     115      118        -\n"
                             " text mode:      3      -        -         -\n"
                             " text mode:      1      -        -         -\n"
		                     "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsHex
	},
	{
		.parName = "kmap",
		.shortDesc = "[<code>]\tKeyboard layout (0=US 1=DE)",
		.longDescFirstLine = "ATTENTION: Changes take only effect after a reset!",
		.allowedChars = G_allowedCharsBool
	},
	{
		.parName = "c-tcal",
		.shortDesc = "Touch calibration data",
		.longDescFirstLine = "Use SETUP -> \"Touch Calibration\" for changing",
		.allowedChars = G_allowedCharsDec
	},
	{
		.parName = "c-pnum",
		.shortDesc = "Carrier board partnumber",
		.longDescFirstLine = "For use by customer (string, null term.)",
	},	
	
	
	{
		.parName = SYSP_gstatshow,
		.shortDesc = "Show status output on graphical console",
		.allowedChars = G_allowedCharsBool
	},
	{ .parName = NULL	/* terminator */ }
};

static SYSP_HELP_DESC_TBL G_descTblStandard = { NULL, G_descTbl };

/**********************************************************************/
/** Add a table for board specific parameters
 *
 * \param newTable		\IN  table to add
 * \return 0 on success, or sysparam error code\n
 */
int SYSP_AddParamTable( SYSP_HELP_DESC_TBL* newTable )
{
    SYSP_HELP_DESC_TBL* tbl;

    /* find last table */
    for( tbl=&G_descTblStandard; tbl->next; tbl=tbl->next );

    /* add table */
    tbl->next = newTable;
    newTable->next = NULL;

    return 0;
}

/**********************************************************************/
/** Find entry in desc table according to \a parName
 *
 * If \a parName ends with a digit, it matches any entry that similar
 * entries (e.g. parName=nspeed2 matches entry nspeed0), however
 * this works only with official, not alias names.
 *
 * \param parName		\IN  parameter name to search for
 * \return entry in desc table or NULL if not found
 */
static const SYSP_HELP_DESC *FindDescEntry( const char *parName )
{
	int parNameLen = strlen(parName);
	char lastChar;
    const SYSP_HELP_DESC* desc;
    const SYSP_HELP_DESC_TBL* tbl;

	if( parNameLen == 0 )
		return NULL;

	lastChar = parName[parNameLen-1];

    /* start with standard table than look for additional tables */
	for( tbl=&G_descTblStandard; tbl; tbl=tbl->next ) {

	    desc = tbl->desc;

    	while( desc->parName ){
    		if( strcmp( parName, desc->parName ) == 0 )
    			goto DONE;

    		if( desc->aliasName && strcmp( parName, desc->aliasName ) == 0 )
    			goto DONE;

    		if( _ISDIGIT(lastChar) &&
    			(strncmp( parName, desc->parName, parNameLen-1 ) == 0) &&
    			_ISDIGIT( desc->parName[parNameLen-1] ))
    			goto DONE;

    		desc++;
    	}
	}

    desc = NULL;

 DONE:
	return desc;
}



/**********************************************************************/
/** Get help strings for standard parameter
 *
 * \param parName		\IN  parameter name including possible prefix.
 * \param parHelp		\OUT filled with info about parameter name
 *
 * On error, all fields of \a parHelp are filled with zeroes
 *
 * \return 0 on success, or sysparam error code\n
 *	 - #SYSPARAM_INVAL_PARAM: parameter is not known
 */
int SysParamHelp(
	const char *parName,
	SYSPARAM_PARAM_HELP *parHelp)
{
	const SYSP_HELP_DESC *desc;
	int rv = SYSPARAM_INVAL_PARAM;

	SYSP_MEMSET( parHelp, 0, sizeof(*parHelp ));

	/* Find descriptor table entry */
	if( (desc = FindDescEntry( parName )) == NULL )
		goto ABORT;

	if( desc->shortDesc == NULL )
		goto ABORT;

	/* copy static entries from descriptor */

	/* copy short description. In case parameter name ends with a digit,
	   replace a %d in desc->shortDesc by its value
	*/
	sprintf( parHelp->shortDesc, desc->shortDesc,
			 parName[strlen(parName)-1]-'0' );

	parHelp->longDescFirstLine 		= desc->longDescFirstLine;
	parHelp->longDescContinuation	= desc->longDescContinuation;
	parHelp->allowedChars			= desc->allowedChars;
	parHelp->choice[0]				= NULL;	/* set to default */

	/* set the choices */
	if( desc->parName == SYSP_nspeed0 )
		SYSP_StringArrCopy( parHelp->choice, SYSP_nspeedValue,
							sizeof(parHelp->choice)/sizeof(char *));

	rv = 0;

 ABORT:
	return rv;
}
/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST

SYSP_UNITTEST( "SYSPARAM2: SysParamHelp", SYSP_UtHelp );


int SYSP_UtHelp( void )
{
	/* FindDescEntry */
	{
		UTASSERT( FindDescEntry( SYSP_mmstartup ) == &G_descTbl[15] );
		UTASSERT( FindDescEntry( "mmstartup" ) == &G_descTbl[15] );
		UTASSERT( FindDescEntry( "mmstartu" ) == NULL );
		UTASSERT( FindDescEntry( "mmstartup0" ) == NULL );

		UTASSERT( FindDescEntry( "con0" ) == &G_descTbl[0]);
		UTASSERT( FindDescEntry( "con1" ) == &G_descTbl[0]);
		UTASSERT( FindDescEntry( "co1" ) == NULL );
		UTASSERT( FindDescEntry( "conn1" ) == NULL );
	}




	return 0;
 ABORT:
	return -1;
}

#endif /* SYSP_CONFIG_UNITTEST */


