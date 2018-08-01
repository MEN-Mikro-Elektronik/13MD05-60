/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_esm_cb.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2012/02/27 16:12:25 $
 *    $Revision: 1.16 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  ESM (embedded system modules) carrier board handlers
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_esm_cb.c,v $
 * Revision 1.16  2012/02/27 16:12:25  ts
 * R: special calibration mechanism and storage needed for customer board
 * M: added ham_corrxxx parameters for HAMED carriers
 *
 * Revision 1.15  2008/08/04 13:48:15  rt
 * R:1. Support for ESMexpress carrier boards
 * M:1. SYSP_NvsEsmxCbProbe() added
 *
 * Revision 1.14  2008/06/11 17:14:57  rt
 * R:1. Print a note that user has to reset board after changing ee-mode_comX.
 * M:1. Set SYSP_PD_NEEDRESET flag.
 *
 * Revision 1.13  2007/07/18 15:38:25  rt
 * replaced strncmp_nocase with local function
 * (since not all OS know this function)
 *
 * Revision 1.12  2007/07/11 14:51:50  rt
 * added:
 * - support for CB_EC06
 *
 * Revision 1.11  2007/02/22 13:50:54  rla
 * added Prefix d for second EMSCB
 *
 * Revision 1.10  2006/12/01 14:47:51  rla
 * Added possibility to change ESMCB address and size by calling SYSP_EsmCarrierInitGlobals
 *
 * Revision 1.9  2006/11/29 10:28:45  ts
 * set reasonable tcal default Values for EM03 Family
 *
 * Revision 1.8  2006/09/15 14:28:54  ts
 * Added Schlafhorst Carrier Variant EC04N
 *
 * Revision 1.7  2006/07/11 10:36:58  rla
 * Adapted for carrier AD56 AD50
 *
 * Revision 1.6  2006/01/05 15:16:53  cs
 * added
 *  + support for boards with EEPROM Layout compatible to the HAMED ESM carrier
 *  + SYSP_IsEsmCB_with_EEPROD2()
 *  + SYSP_NvsHamedCompProbe()
 *
 * Revision 1.5  2005/06/23 15:54:24  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.4  2005/03/03 17:28:56  CSchuster
 * replaced label OK with EXIT_OK (was already defined with VxWorks 6.0)
 *
 * Revision 1.3  2005/01/18 15:49:26  kp
 * added size to NVS sections
 *
 * Revision 1.2  2004/12/20 08:53:30  kp
 * +EC01N, EC04 aware
 *
 * Revision 1.1  2004/01/08 09:06:52  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/ad65_eeprom.h>
#include <MEN/esmx_cb_eeprom.h>

static int G_cbEepromSize = 512;
static int G_cbEepromAddr = 0xac;

static char *G_prefix = "c";

/**********************************************************************/
/** Compare 2 strings. not case sensitive
 *
 */
static int StrNCmpNoCase(const char *s1, const char *s2, int n)
{
	char c1, c2;

	while (1) {
		if( n==0 ) return 0;
		c1=*s1;
		c2=*s2;

		if( c1>='A' && c1<='Z' ) c1 += 'a'-'A';
		if( c2>='A' && c2<='Z' ) c2 += 'a'-'A';
		if (c1 != c2) return(c1 - c2);
		if (c1 == 0 )return(0);
		n--;
		s1++;
		s2++;
	}
}
/**********************************************************************/
/** Set Devive Address and Size for Carrier Board
 *
 *  if data differs from default this routine have to be called before
 *  probing the ESMCB board.
 */
void SYSP_EsmCarrierInitGlobals( int addr, int size, char prefix )
{
	G_cbEepromSize = size;
	G_cbEepromAddr = addr;

	if( prefix == 'd' ){
		G_prefix = "d";
	}
	else{
		G_prefix = "c";
	}
}

/**********************************************************************/
/** Probe for all possible known ESM carrier boards
 *
 * Prefix will be 'c' always.
 * SMB addresses are hardcoded (0xac).
 *
 * If nothing could be read from SMB (no board EEPROM there), no
 * parameter group is created and #SYSPARAM_EEPROM_ERROR is returned.
 *
 * If an unknown/unprogrammed board is found, a parameter group for
 * standard EEPROD section is setup
 *
 * \param super			\IN sysparam super object
 * \param smbBusNo		\IN SMB bus number
 *
 * \return sysparam error code
 */
int SYSP_EsmCarrierProbe( SYSP_SUPER *super, int smbBusNo)
{
	SYSP_NVS_ADR nvsAdr;
	SYSP_NV_STORAGE *nvs;
	int rv;

	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = smbBusNo;
	nvsAdr.addr.smb.dev = G_cbEepromAddr;

	/* create NVS section */
	if( (nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "esmcb",
							   super, G_prefix, nvsAdr, G_cbEepromSize
			 )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/*---------------------------------+
	|  Probe all known carrier boards  |
	+---------------------------------*/
	if( (rv = SYSP_NvsAd65EtcProbe( nvs ) == 0 ))
		goto EXIT_OK;

	if( (rv = SYSP_NvsHamedCompProbe( nvs ) == 0 ))
		goto EXIT_OK;

	if( (rv = SYSP_NvsSaurerProbe( nvs ) == 0 ))
		goto EXIT_OK;

	if( (rv = SYSP_NvsEsmxCbProbe( nvs ) == 0 ))
		goto EXIT_OK;

	/* not a known board, setup default handler */
	if( (rv = SYSP_EeprodGrpCreate( nvs, NULL, FALSE ) == 0 ))
		goto EXIT_OK;

	/* failed */
	SYSP_ALLOC_UNDO( sizeof(SYSP_NV_STORAGE), super ); /* free nvs */
	return rv;

 EXIT_OK:
	/* add NVS to super list */
	SYSP_NvsAdd( super, nvs );
	return 0;
}


/*----------------------------------------------------------------------+
| AD65 and friends                                                      |
+----------------------------------------------------------------------*/
#define _AD65OFF(x) offsetof(struct ad65_menm_parms,x)

const SYSP_PAR_DESC SYSP_parListAd65Tcal[] = {

#if defined( MEN_EM03 ) || defined( MEN_EM09 )
	/* set reasonable defaults for touch devices (e.g. Schlafhorst) */
	SYSP_PDE_TCAL( "tcal", NULL, _AD65OFF(tcal), "0, 3700, 0, 3700"),
#else
	SYSP_PDE_TCAL( "tcal", NULL, _AD65OFF(tcal), "0,0,0,0"),
#endif

	SYSP_PDE_END
};

/**********************************************************************/
/** Probe for AD65 and boards with same EEPROM layout
 *
 * \param nvs			\IN NV storage object
 *
 * \return sysparam error code
 */
int SYSP_NvsAd65EtcProbe( SYSP_NV_STORAGE *nvs )
{
	int rv;
	const char *compatBoards[] = {
		"AD65", "AD66", "EC01", "EC01N", "EC04", "EC04N", 
		"AD56", "AD50", NULL
	};
	/* check presence of EEPROD section, don't ignore read error */
	if( (rv = SYSP_EeprodGrpCreate( nvs, compatBoards, FALSE )) != 0 )
		return rv;

	/* touch calibration data */
	rv = SYSP_MpGrpCreateStd( nvs, "tcal", offsetof( AD65_EEPROM, menm ),
							  sizeof( struct ad65_menm_parms ),
							  SYSP_parListAd65Tcal, AD65_EEID_MENM, NULL);
	if( rv != 0 ){
		DBGWRT_ERR((DBH,"*** SYSP_NvsAd65EtcProbe err %d creating tcal grp\n",
					rv ));
		rv = 0;
	}
	return rv;
}


/*----------------------------------------------------------------------+
| HAMED and friends                                                      |
+----------------------------------------------------------------------*/
#define _ESMCBMENMOFF(x) offsetof(struct esm_cb_menm_parms,x)
#define _ESMCBINFOOFF(x) offsetof(struct esm_cb_info_parms,x)

const SYSP_PAR_DESC SYSP_parListEsmCbInfo[] = {
	SYSP_PDE_U32X( SYSP_esm_cb_hwflag, NULL,
				   _ESMCBINFOOFF(ci_hw_flag), "0", SYSP_PD_MMPARASTR ),
 	SYSP_PDE_STR( SYSP_esm_cb_partnum, NULL,
				  _ESMCBINFOOFF(ci_custPNum), "", 16, SYSP_PD_MMPARASTR ),
	SYSP_PDE_END
};

const SYSP_PAR_DESC SYSP_parListEsmCbInfoSaurer[] = {
 	SYSP_PDE_STR( SYSP_esm_cb_partnum, NULL,
				  _ESMCBINFOOFF(ci_custPNum), "", 16, SYSP_PD_MMPARASTR ),
	SYSP_PDE_END
};

const SYSP_PAR_DESC SYSP_parListEsmCbTcal[] = {
	SYSP_PDE_TCAL( "tcal", NULL, _ESMCBMENMOFF(tcal), "0,0,0,0"),
	SYSP_PDE_S16D( "ham_corrX1", NULL,  _ESMCBMENMOFF(mm_resvd2), "0", SYSP_PD_MMPARASTR),
	SYSP_PDE_S16D( "ham_corrX2", NULL, (_ESMCBMENMOFF(mm_resvd2) + 2), "0", SYSP_PD_MMPARASTR),
	SYSP_PDE_S16D( "ham_corrY1", NULL, (_ESMCBMENMOFF(mm_resvd2) + 4), "0", SYSP_PD_MMPARASTR),
	SYSP_PDE_S16D( "ham_corrY2", NULL, (_ESMCBMENMOFF(mm_resvd2) + 6), "0", SYSP_PD_MMPARASTR),
	SYSP_PDE_END
};


const SYSP_PAR_DESC SYSP_parListEsmCbMenmSaurer[] = {
	/* set reasonable defaults for touch devices (e.g. Saurer) */
	SYSP_PDE_TCAL( "tcal", NULL, _ESMCBMENMOFF(tcal), "0, 3700, 0, 3700"),
	/* RS232/422/485 switch settings */
	SYSP_PDE_COMMODE( "mode_com11", "mode_com3", 
					  _ESMCBMENMOFF(mm_mode_com1[1]), "RS422", SYSP_PD_NEEDRESET ),
	SYSP_PDE_COMMODE( "mode_com12", "mode_com4", 
					  _ESMCBMENMOFF(mm_mode_com1[2]), "RS422", SYSP_PD_NEEDRESET ),
	SYSP_PDE_END
};

/** look in Table to see if EEPROD2 structure should be used with carrier board
 *
 * \param brdName			\IN board name
 *
 * \return TRUE if match, FALSE for fail
 */
int SYSP_IsEsmCB_with_EEPROD2( const char *brdName )
{
	const char **eep2SupBrd;
	const char* ESM_CB_SupportingEeprod2[] =
	{
		"HAMED",
		"EC06",
		"XC01", 
		NULL
	};

	for(eep2SupBrd = ESM_CB_SupportingEeprod2; *eep2SupBrd; eep2SupBrd++)
	{
		if( StrNCmpNoCase( brdName, *eep2SupBrd, 6) == 0 ){
			/* brdName matched */
			return TRUE;
		}
	}
	return FALSE;
}

/**********************************************************************/
/** Probe for HAMED and boards with same EEPROM layout
 *
 * \param nvs			\IN NV storage object
 *
 * \return sysparam error code
 */
int SYSP_NvsHamedCompProbe( SYSP_NV_STORAGE *nvs )
{
	int rv = 0;

	const char* compatBoards[] =
	{
		"HAMED",
		NULL
	};

	/* check presence of EEPROD2 section, don't ignore read error */
	if( (rv = SYSP_Eeprod2GrpCreate( nvs, compatBoards, FALSE )) != 0 )
		return rv;

	/* carrier board info data */
	if( (rv = SYSP_MpGrpCreateStd(  nvs, "esmCbInfo",
									offsetof(ESM_CB_EEPROM, cbinfo),
							  		sizeof( struct esm_cb_info_parms ),
							  		SYSP_parListEsmCbInfo,
							  		ESM_CB_EEID_CBINFO,
							  		NULL)) != 0)
	{
		DBGWRT_ERR((DBH,"*** SYSP_NvsHamedCompProbe err %d "
						"creating esmCbInfo grp\n",
					rv ));
		rv = 0;
	}

	/* touch calibration data */
	rv = SYSP_MpGrpCreateStd( nvs, "tcal", offsetof( ESM_CB_EEPROM, menm ),
							  sizeof( struct esm_cb_menm_parms ),
							  SYSP_parListEsmCbTcal, ESM_CB_EEID_MENM, NULL);

	if( rv != 0 ){
		DBGWRT_ERR((DBH,"*** SYSP_NvsHamedCompProbe err %d creating tcal grp\n",
					rv ));
		rv = 0;
	}

	return rv;
}


/**********************************************************************/
/** Probe for Saurer and boards with same EEPROM layout
 *
 * \param nvs			\IN NV storage object
 *
 * \return sysparam error code
 */
int SYSP_NvsSaurerProbe( SYSP_NV_STORAGE *nvs )
{
	int rv = 0;

	const char* compatBoards[] =
	{
		"EC06", 
		NULL
	};

	/* check presence of EEPROD2 section, don't ignore read error */
	if( (rv = SYSP_Eeprod2GrpCreate( nvs, compatBoards, FALSE )) != 0 )
		return rv;

	/* carrier board info data */
	if( (rv = SYSP_MpGrpCreateStd(  nvs, "esmCbInfo",
									offsetof(ESM_CB_EEPROM, cbinfo),
							  		sizeof( struct esm_cb_info_parms ),
							  		SYSP_parListEsmCbInfoSaurer,
							  		ESM_CB_EEID_CBINFO,
							  		NULL)) != 0)
	{
		DBGWRT_ERR((DBH,"*** SYSP_NvsSaurerProbe err %d "
						"creating esmCbInfo grp\n",
					rv ));
		rv = 0;
	}

	/* touch calibration data */
	rv = SYSP_MpGrpCreateStd( nvs, "esmCbMenm", offsetof( ESM_CB_EEPROM, menm ),
							  sizeof( struct esm_cb_menm_parms ),
							  SYSP_parListEsmCbMenmSaurer,
							  ESM_CB_EEID_MENM, NULL);

	if( rv != 0 ){
		DBGWRT_ERR((DBH,"*** SYSP_NvsSaurerProbe err %d creating esmCbMenm grp\n",
					rv ));
		rv = 0;
	}

	return rv;
}

/**********************************************************************/
/** Probe for ESMexpress carrier boards and boards with same 
 *  EEPROM layout
 *
 * \param nvs			\IN NV storage object
 *
 * \return sysparam error code
 */
int SYSP_NvsEsmxCbProbe( SYSP_NV_STORAGE *nvs )
{
	int rv = 0;

	const char* compatBoards[] =
	{
		"XC01", 
		NULL
	};

	/* check presence of EEPROD2 section, don't ignore read error */
	if( (rv = SYSP_Eeprod2GrpCreate( nvs, compatBoards, FALSE )) != 0 )
		return rv;

	/* size of ESMexpress carrier eeprom is 256 bytes */
	nvs->nvsSize = ESMX_CB_EEPROM_SIZE;

	/* no more groups up to now */

	return rv;
}
