/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_tags.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/06/07 16:32:43 $
 *    $Revision: 2.21 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Standard parameter name strings
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_tags.c,v $
 * Revision 2.21  2010/06/07 16:32:43  sy
 * R:1 EEPROD3 needs new parameters for the data members.
 * M: 1. Add new parameters for EEPROD3
 *
 * Revision 2.20  2009/09/23 09:00:33  ufranke
 * R: improve boot speed
 * M: added SYSP_normalboot
 * R: no common ee-nobanner
 * M: added SYSP_nobanner
 *
 * Revision 2.19  2008/06/27 15:12:03  ufranke
 * R: MIPIOS needs better random seed
 * M: added SYSP_rand
 *
 * Revision 2.18  2008/05/29 11:12:38  rt
 * added:
 * - SYSP_rststat_cb
 *
 * Revision 2.17  2008/04/11 14:55:14  rt
 * added:
 * -nspeed3
 * -nmac3
 *
 * Revision 2.16  2008/02/04 14:29:45  ufranke
 * added
 *  - fpga
 *
 * Revision 2.15  2008/01/09 11:12:36  rt
 * added:
 * -SYSP_buspci...
 *
 * Revision 2.14  2007/09/21 12:01:52  rt
 * added:
 * - SYSP_stdis_usb
 * - SYSP_stdis_ether
 *
 * Revision 2.13  2007/09/20 17:57:57  rt
 * added:
 * - SYSP_fram0
 *
 * Revision 2.12  2007/09/12 15:33:08  rt
 * added:
 * - SYSP_usbdp[]
 *
 * Revision 2.11  2007/07/11 10:59:13  rt
 * added:
 * - SYSP_comModeValue
 *
 * Revision 2.10  2007/03/22 11:10:38  ufranke
 * added
 *  + SYSP_stdis_nand
 *
 * Revision 2.9  2006/10/13 19:47:33  cs
 * added:
 *   + SYSP_gstatshow
 *
 * Revision 2.8  2005/06/23 15:54:38  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.7  2005/03/09 16:51:36  ufranke
 * added
 *  + SYSP_stdis_wdog
 *
 * Revision 2.6  2005/02/15 12:15:41  ufranke
 * cosmetics
 *  - comment SYSP_NSPEED_VALUE_IDX
 *
 * Revision 2.5  2005/02/02 16:03:13  kp
 * replaced wdt1/2 by wdt
 *
 * Revision 2.4  2005/01/14 09:36:18  kp
 * + SYSP_PrefixParamName()
 *
 * Revision 2.3  2005/01/13 15:26:00  kp
 * + more tags
 * + SYSP_NmacName, SYSP_NspeedName
 *
 * Revision 2.2  2004/12/23 09:36:40  ufranke
 * added
 *  + tags
 *
 * Revision 2.1  2004/12/20 08:56:14  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

const char
    SYSP_mem0[] =        "mem0",
	SYSP_mem1[] =  		 "mem1",
    SYSP_flash0[] =      "flash0",
    SYSP_flash1[] =      "flash1",
    SYSP_fram0[] =		 "fram0",
	SYSP_cpu[] =  		 "cpu",
	SYSP_cpuclkhz[] = 	 "cpuclkhz",
	SYSP_memclkhz[] = 	 "memclkhz",
	SYSP_pciclkhz[] = 	 "pciclkhz",
	SYSP_clun[] = 		 "clun",
	SYSP_dlun[] = 		 "dlun",
	SYSP_rststat[] = 	 "rststat",
	SYSP_rststat_pwon[] 		= 	 "pwon",
	SYSP_rststat_pdrop[] 		= 	 "pdrop",
	SYSP_rststat_temp[] 		= 	 "temp",
	SYSP_rststat_sw[] 			= 	 "swrst",
	SYSP_rststat_wdog[] 		= 	 "wdog",
	SYSP_rststat_rbut[] 		= 	 "rbut",
	SYSP_rststat_hrst[] 		= 	 "hrst",
	SYSP_rststat_srst[] 		= 	 "srst",
	SYSP_rststat_cb[] 			= 	 "cbrst",	/*carrier board reset*/
	SYSP_rststat_unknown[] 		= 	 "???",
	SYSP_mmst[] = 		 "mmst",
	SYSP_mm[] = 		 "mm",
	SYSP_mm_pmm[] 				= 	 "pmm",
	SYSP_mm_smm[] 				= 	 "smm",
	SYSP_cons[] = 		 "cons",
	SYSP_brd[] = 		 "brd",
	SYSP_brdmod[] = 	 "brdmod",
	SYSP_brdrev[] = 	 "brdrev",
	SYSP_sernbr[] = 	 "sernbr",
	SYSP_prodat[] = 	 "prodat",
	SYSP_repdat[] = 	 "repdat",
	SYSP_con0[] = 		 "con0",
	SYSP_con1[] = 		 "con1",
	SYSP_con2[] = 		 "con2",
	SYSP_con3[] = 		 "con3",
	SYSP_gcon[] = 		 "gcon",
	SYSP_gstatshow[] =   "gstatshow",
	SYSP_vmode[] = 		 "vmode",
	SYSP_cbr[] = 		 "cbr",
	SYSP_baud[] = 		 "baud",
	SYSP_bsadr[] = 		 "bsadr",
	SYSP_bs[] = 		 "bs",
	SYSP_ecl[] = 		 "ecl",
	SYSP_tto[] = 		 "tto",
	SYSP_tries[] = 		 "tries",
	SYSP_tdp[] = 		 "tdp",
	SYSP_hdp[] = 		 "hdp",
	SYSP_wdt[] = 		 "wdt",
	SYSP_stignfault[] =  "stignfault",
	SYSP_stdis[] = 		 "stdis",
	SYSP_stdis_wdog[] =	 "stdis_wdog",
	SYSP_stdis_nand[] =	 "stdis_nand",
	SYSP_stdis_usb[] =	 "stdis_usb",
	SYSP_stdis_ether[] = "stdis_ether",
	SYSP_stwait[] = 	 "stwait",
	SYSP_ldlogodis[] = 	 "ldlogodis",
	SYSP_updcdis[] = 	 "updcdis",
	SYSP_mmstartup[] = 	 "mmstartup",
	SYSP_startup[] = 	 "startup",
	SYSP_kerpar[] = 	 "kerpar",
	SYSP_useflpar[] =	 "useflpar",
	SYSP_bf[] =			 "bf",
	SYSP_bootfile[] =	 "bootfile",
	SYSP_netip[] =		 "netip",
	SYSP_netgw[] =		 "netgw",
	SYSP_nethost[] =	 "nethost",
	SYSP_netname[] = 	 "netname",
	SYSP_netaddr[] = 	 "netaddr",
	SYSP_netsm[]  = 	 "netsm",
	SYSP_nspeed0[] = 	 "nspeed0",
	SYSP_nspeed1[] = 	 "nspeed1",
	SYSP_nspeed2[] = 	 "nspeed2",
	SYSP_nspeed3[] = 	 "nspeed3",
	SYSP_nmac0[] 	= 	 "nmac0",
	SYSP_nmac1[] 	= 	 "nmac1",
	SYSP_nmac2[] 	= 	 "nmac2",
	SYSP_nmac3[] 	= 	 "nmac3",
	SYSP_usbdp[]	=	 "usbdp",
	SYSP_fpga[]		=	 "fpga",
	SYSP_buspci0[] 	=	 "buspci0",	
	SYSP_buspci1[] 	=	 "buspci1",	
	SYSP_buspci2[] 	=	 "buspci2",
	SYSP_rand[] 	=	 "rand",
	SYSP_nobanner[]	=	 "nobanner",
	SYSP_normalboot[] =  "normalboot",    /* true is default: if not true -> fastboot */
	SYSP_p3rec[] =  "inv_rec",
	SYSP_p3part[] =  "inv_part",
	SYSP_p3rev[] =  "inv_rev",
	SYSP_p3ser[] =  "inv_ser",
	SYSP_p3crc[] =  "inv_crc"	
	;

/* NOTE: values must conform to NETIF_LinkStringToSpeed()
         and SYSP_NSPEED_VALUES_IDX */
const char *SYSP_nspeedValue[] = {
	"AUTO", "10HD", "10FD", "100HD", "100FD", "1000", NULL
};

/* NOTE: values must conform to SYSP_COMMODE_VALUES_IDX */
const char *SYSP_comModeValue[] = {
	"RS422", "RS232", "RS485", NULL
};

char *SYSP_NmacName( char *buf, int ifNum )
{
	SYSP_SNPRINTF( buf, 1, "nmac%d", ifNum );
	return buf;
}

char *SYSP_NspeedName( char *buf, int ifNum )
{
	SYSP_SNPRINTF( buf, 1, "nspeed%d", ifNum );
	return buf;
}

char *SYSP_BuspciName( char *buf, int ifNum )
{
	SYSP_SNPRINTF( buf, 1 , "buspci%d", ifNum );
	return buf;
}

/**********************************************************************/
/** Build prefixed parameter name (e.g. for carrier boards)
 *
 * If prefix non-zero, build parameter "x-param", where x is the prefix
 *
 * \param prefix	\IN board prefix (e.g. 'c') (0 if none)
 * \param buf		\OUT filled with name (must have enough space)
 * \return buf
 */
char *SYSP_PrefixParamName( char prefix, const char *parName, char *buf )
{
	if( prefix )
		SYSP_SNPRINTF( buf, 64, "%c-%s", prefix, parName );
	else
		SYSP_SNPRINTF( buf, 64, "%s", parName );

	return buf;
}



