/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_em04a.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2005/07/14 15:51:14 $
 *    $Revision: 1.11 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  EM04A specific part
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_em04a.c,v $
 * Revision 1.11  2005/07/14 15:51:14  CSchuster
 * fixed function calls to
 * 	postProcessSetParam()
 * 	SYSP_NvsCreate()
 * 	SYSP_VxGrpCreate
 *
 * Revision 1.10  2005/06/23 15:54:20  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.9  2005/01/12 14:19:53  kp
 * prepared for new macros (untested)
 *
 * Revision 1.8  2004/09/03 15:20:46  kp
 * added tcp extension parameters (tdp, hdp, tries)
 *
 * Revision 1.7  2004/04/30 16:13:40  kp
 * changed default mmstartup to
 * "NBOOT KERPAR='ip=bootp rw root=/dev/ram0 ramdisk_size=300000"
 *
 * Revision 1.6  2004/04/06 10:03:37  kp
 * changed default value of "selftest" parameter to 2 (ignore selftest errors)
 *
 * Revision 1.5  2004/03/24 16:23:41  kp
 * added "tto" parameter
 *
 * Revision 1.4  2004/03/23 08:39:58  kp
 * changed default startup string to ip=bootp (not ip=auto)
 *
 * Revision 1.3  2004/01/27 10:10:57  kp
 * build the default startup string according to board and model name
 *
 * Revision 1.2  2004/01/12 08:30:39  kp
 * added eccsth parameter
 *
 * Revision 1.1  2004/01/08 09:06:50  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/em04a_eeprom.h>


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[/*490*/1024];			/**< allocation heap  */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsEm04ACpuProbe( SYSP_MMSMB_SUPER *xSuper );




/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for EM04A
 *
 * Probes for CPU EEPROM
 * Does not probe for possible ESM carrier boards (not foreseen for EM04A)
 *
 * \return sysparam error code if fatal error
 */
static int Em04AInit( SYSP_SUPER *super )
{
	int rv;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );
	SYSP_SuperMmsmbInit( xSuper );

	/* probe CPU EEPROM */
	if( (rv = SYSP_NvsEm04ACpuProbe( xSuper )) != 0 )
		return rv;				/* fatal */

	/* build MM para string */
	super->postProcessSetParam( super, NULL, 0 );

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for EM04A
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int Em04ASysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = Em04AInit;
	SYSP_SuperMmsmbInit( xSuper );

	return super->reInit( super );
}




/*----------------------------------------------------------------------+
| CPU EEPROM                                                            |
+----------------------------------------------------------------------*/


#define _EEOFF(x) offsetof(struct menm_parms,x)

/** list of parameters in EM04A CPU EEPROM menmon section*/
static const SYSP_PAR_DESC SYSP_parListEm04AMenm[] = {
	SYSP_PDE_U8X( "misc", 		NULL, _EEOFF(mm_misc), 		"0", 0),
	SYSP_PDE_U8X( "selftest",  	NULL, _EEOFF(mm_selftest), 	"2", 0),
	SYSP_PDE_U8X( "pciopts", 	NULL, _EEOFF(mm_pci_opts), 	"0", 0),

	SYSP_PDE_CBR( _EEOFF(mm_cbaud) ),
	SYSP_PDE_BSADR( _EEOFF(mm_bsadr) ),
	SYSP_PDE_UXX( _EEOFF(mm_usr[0]) ),




	SYSP_PDE_U32D("cbr", 		"baud", _EEOFF(mm_cbaud), 	"9600",
				  SYSP_PD_MMPARASTR),
	SYSP_PDE_U32X("bsadr", 		"bs", _EEOFF(mm_bsadr), 	"0", 0),
	/* user params.. isn't there a better way? */
	SYSP_PDE_U16X("u00", 		NULL, _EEOFF(mm_usr[0]), 	"0", 0),
	SYSP_PDE_U16X("u01", 		NULL, _EEOFF(mm_usr[1]), 	"0", 0),
	SYSP_PDE_U16X("u02", 		NULL, _EEOFF(mm_usr[2]), 	"0", 0),
	SYSP_PDE_U16X("u03", 		NULL, _EEOFF(mm_usr[3]), 	"0", 0),
	SYSP_PDE_U16X("u04", 		NULL, _EEOFF(mm_usr[4]), 	"0", 0),
	SYSP_PDE_U16X("u05", 		NULL, _EEOFF(mm_usr[5]), 	"0", 0),
	SYSP_PDE_U16X("u06", 		NULL, _EEOFF(mm_usr[6]), 	"0", 0),
	SYSP_PDE_U16X("u07", 		NULL, _EEOFF(mm_usr[7]), 	"0", 0),
	SYSP_PDE_U16X("u08", 		NULL, _EEOFF(mm_usr[8]), 	"0", 0),
	SYSP_PDE_U16X("u09", 		NULL, _EEOFF(mm_usr[9]), 	"0", 0),
	SYSP_PDE_U16X("u10", 		NULL, _EEOFF(mm_usr[10]), 	"0", 0),
	SYSP_PDE_U16X("u11", 		NULL, _EEOFF(mm_usr[11]), 	"0", 0),
	SYSP_PDE_U16X("u12", 		NULL, _EEOFF(mm_usr[12]), 	"0", 0),
	SYSP_PDE_U16X("u13", 		NULL, _EEOFF(mm_usr[13]), 	"0", 0),
	SYSP_PDE_U16X("u14", 		NULL, _EEOFF(mm_usr[14]), 	"0", 0),
	SYSP_PDE_U16X("u15", 		NULL, _EEOFF(mm_usr[15]), 	"0", 0),
	SYSP_PDE_U8D( "eccsth", 	NULL, _EEOFF(mm_eccSingleTh), "32", 0),
	SYSP_PDE_U8D( "tto", 		NULL, _EEOFF(mm_tto), 		 "0", 0),

	SYSP_PDE_END
};

#define _EEOFF2(x) offsetof(struct tcpext_parms,x)
/** list of parameters in EM04A TCP extension section*/
static const SYSP_PAR_DESC SYSP_parListEm04ATcpExt[] = {
	SYSP_PDE_S16D( "tdp", 		NULL, _EEOFF2(tcp_tdp),		"-1", 0),
	SYSP_PDE_S16D( "hdp", 		NULL, _EEOFF2(tcp_hdp),		"-1", 0),
	SYSP_PDE_U8D( "tries", 		NULL, _EEOFF2(tcp_tries),	"10", 0),
	SYSP_PDE_END
};


/** list of parameters in "mmpara" parameter group */
static SYSP_CONST_STRING SYSP_parListEm04AMmpara[] = {
	"mem0",
	"mem1",
	"cpu",
	"cpuclkhz",
	"memclkhz",
	"pciclkhz",
	"clun",
	"dlun",
	"rststat",
	"cons",
	"mmst",
	NULL
};

/**********************************************************************/
/** Probe for EM04A CPU EEPROM and non-persistant mmpara section
 *
 * Creates all parameter sections that shall be present in EM04A
 * CPU EEPROM even if there is an EEPROM error.
 *
 * Also creates the MENMON parameter section "mmpara"
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsEm04ACpuProbe( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvs;
	SYSP_NVS_ADR nvsAdr;
	char *defStartup;

	/* create NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = 0xa8;

	if( (nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), "em04acpu",
							   &xSuper->s, "", nvsAdr,
							   sizeof(EM04A_EEPROM) )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* Menmon parameters */
	if( (rv = SYSP_MmgrpCreate( nvs, "mmpara", SYSP_parListEm04AMmpara,
								200, xSuper->initBlk.mmParaStringAdrs,
								xSuper->initBlk.mmParaStringMaxLen )) != 0 )
		return rv;


	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvs, NULL, TRUE )) != 0)
		return rv;

	/* menm section */
	if( (rv = SYSP_MpGrpCreateStd( nvs, "menm", offsetof( EM04A_EEPROM, menm ),
								 sizeof( struct menm_parms ),
								 SYSP_parListEm04AMenm, EM04A_EEID_MENM,
								 NULL )) != 0 )
		return rv;

	/* tcpext section */
	if( (rv = SYSP_MpGrpCreateStd( nvs, "tcpext",
								   offsetof( EM04A_EEPROM, tcpext ),
								   sizeof( struct menm_parms ),
								   SYSP_parListEm04ATcpExt, EM04A_EEID_TCPX,
								   NULL )) != 0 )
		return rv;

	/* startup string */

	/*
	 * get the brd + brdmodel name
	 */
	if( (defStartup = SYSP_ALLOC( 128, &xSuper->s )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	{
		char buf1[10], buf2[10];

		nvs->getParam( nvs, "brd",  buf1, sizeof(buf1) );
		nvs->getParam( nvs, "brdmod",  buf2, sizeof(buf2) );

		SYSP_SNPRINTF( defStartup, 128,
					   "NBOOT KERPAR='ip=bootp rw root=/dev/ram0"
					   " ramdisk_size=300000'");
	}


	if( (rv = SYSP_StrGrpCreate( nvs, "mmstartup", "startup",
								 offsetof( EM04A_EEPROM, startup ),
								 EM04A_EEPROM_STARTUP_LEN,
								 defStartup)) != 0 )
		return rv;

	/* vxbline */
	if( (rv = SYSP_VxGrpCreate( nvs, offsetof( EM04A_EEPROM, vxBline ),
								EM04A_VXBLINE_LEN,
								xSuper->initBlk.vxBlineStringAdrs,
								"fei(0,0):", 0 /* ??? */)))
		return rv;

	SYSP_NvsAdd( &xSuper->s, nvs );
	return 0;
}

