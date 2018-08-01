/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_eeprod.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/06/28 13:47:52 $
 *    $Revision: 1.6 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Handler for parameter group conforming to eeprod.h
 *
 *    \switches  SYSP_NAME_NO_UPPER_CASE - do not convert board name to
 *										   upper case
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_eeprod.c,v $
 * Revision 1.6  2007/06/28 13:47:52  rt
 * convert board names to upper case (see SYSP_NAME_NO_UPPER_CASE)
 *
 * Revision 1.5  2005/06/23 15:54:14  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.4  2005/04/12 16:52:38  kp
 * avoid naming conflicts when entire SYSPARAM included in one big file
 * (VxWorks syslib...)
 *
 * Revision 1.3  2005/03/03 17:28:54  CSchuster
 * replaced label OK with EXIT_OK (was already defined with VxWorks 6.0)
 *
 * Revision 1.2  2004/12/20 08:52:03  kp
 * use SYSP tags
 *
 * Revision 1.1  2004/01/08 09:06:49  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>


#define _EEPOFF(x) offsetof(EEPROD,x)

static int ProdFromRawU8D( char *dest, int destLen, void *src );
static int ProdFromRawU32D( char *dest, int destLen, void *src );




const SYSP_PAR_DESC SYSP_parListEeprod[] = {
	SYSP_PDE_REV( SYSP_brdrev, 	NULL, _EEPOFF(pd_revision)),

	{ SYSP_sernbr, 	NULL, _EEPOFF(pd_serial),
	  SYSP_ToRawU32D, ProdFromRawU32D, SYSP_ValidateD,
	  NULL, 0, SYSP_PD_MMPARASTR },

	{ SYSP_brdmod, 	NULL, _EEPOFF(pd_model),
	  SYSP_ToRawU8D, ProdFromRawU8D, SYSP_ValidateD,
	  NULL, 0xFF, SYSP_PD_MMPARASTR },

#ifdef SYSP_NAME_NO_UPPER_CASE
	SYSP_PDE_STR( SYSP_brd, 		NULL, _EEPOFF(pd_hwName), NULL, 6,
				  SYSP_PD_MMPARASTR | SYSP_PD_RESTART ),
#else
	SYSP_PDE_STRUPPER( SYSP_brd, 		NULL, _EEPOFF(pd_hwName), NULL, 6,
				  	   SYSP_PD_MMPARASTR | SYSP_PD_RESTART ),
#endif /*SYSP_NAME_NO_UPPER_CASE*/
				 
	SYSP_PDE_END
};


/**********************************************************************/
/** Probe for EEPROD parameter group
 *
 * On success, (and if hwName matched), the created parameter group
 * is added to the NVS' list.
 *
 * \param nvs				\IN NV storage object (fully initialized)
 * \param expHwNames 		\IN array of bord names tolerated
 *								(null-terminated).
 *							   Can be NULL, in this case, any name will match.
 * \param ignoreNvsError 	\IN if TRUE, a NVS read error is ignored
 *
 * \return sysparam error code
 *
 */
int SYSP_EeprodGrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	int ignoreNvsError)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int rv;

	/* attempt to init parameter group, read NVS */
	rv = SYSP_MpGrpCreateProd( nvs, "prod", 0, sizeof(EEPROD),
							 SYSP_parListEeprod, EEID_PD,
							 &mpGrp );

	switch( rv ){
	case SYSPARAM_EEPROM_ERROR:
		if( !ignoreNvsError )
			goto CLEANUP;
		rv = 0;
		break;
	case SYSPARAM_INVAL_VALUE:	/* bad magic/parity, ignore this */
		rv = 0;
		break;
	case 0:
		break;
	default:
		goto CLEANUP;
	}

	/* check expected hwName */
	if( expHwNames != NULL ){
		char brdBuf[6];
		const char **nm;

		mpGrp->parGrp.getParam( &mpGrp->parGrp,
								 SYSP_brd,
								 brdBuf,
								 sizeof(brdBuf));
		for( nm=expHwNames; *nm; nm++ ){
			if( SYSP_STRNCMP( brdBuf, *nm, sizeof(brdBuf)) == 0 ){
				/* brd name matched */
				goto EXIT_OK;
			}
		}
		rv = SYSPARAM_INVAL_VALUE; /* board name not matched */
		goto CLEANUP;
	}
 EXIT_OK:
	SYSP_ParGrpAdd( nvs, &mpGrp->parGrp );
	return 0;


 CLEANUP:
	if( mpGrp )
		SYSP_ALLOC_UNDO( sizeof(*mpGrp) + sizeof(EEPROD), nvs->super );

	return rv;
}


/**********************************************************************/
/** as SYSP_FromRawU8D, but returns SYSPARAM_INVAL_VALUE if raw is 0xff */
static int ProdFromRawU8D( char *dest, int destLen, void *src )
{
	char buf[10];

	if( *(u_int8 *)src == 0xff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}
	SYSP_SNPRINTF( buf, sizeof(buf), "%d", *(u_int8 *)src);
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** as SYSP_FromRawU32D, but returns SYSPARAM_INVAL_VALUE if raw is 0xff */
static int ProdFromRawU32D( char *dest, int destLen, void *src )
{
	char buf[12];

	if( *(u_int32 *)src == 0xffffffff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}
	SYSP_SNPRINTF( buf, sizeof(buf), "%lu", SYSP_FROMBE32(*(u_int32 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

