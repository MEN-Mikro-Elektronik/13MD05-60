/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_mpgrp.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/07/27 17:21:01 $
 *    $Revision: 1.13 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Handler for parameter groups using magic/parity
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_mpgrp.c,v $
 * Revision 1.13  2010/07/27 17:21:01  sy
 * R: in EEPROD3 parity bit is not used any more, instead crc is used to check data integrity
 * M: defined different version of HasValidMp and UpdateMp functions
 *
 * Revision 1.12  2010/06/14 16:45:19  sy
 * R:1. parameters are not set to default value when eeprom is filled with 0, because crc for all 0 is 0
 * M: 1. if crc is 0 then default value is assigned
 *
 * Revision 1.11  2010/06/07 16:31:16  sy
 * R:1. in EEPROD3 each member has fixed length
 *     2. in EEPROD3 instead of magic word, CRC is used to protect the data integrity
 * M: 1. Change Sysp_MpGrupSetParam to handln fixed string length
 *      2. Add Sysp_UtMpGrp, HasValidMp and UpdateMp to support CRC
 *
 * Revision 1.10  2010/01/22 13:32:45  RLange
 * R: Support password handling
 * M: Handle SYSPARAM_ACC flag for encryption
 *
 * Revision 1.9  2008/07/01 11:44:37  ufranke
 * R: SYSP_MpGrpSetDefaultsDoNothing() - no default setting if wrong ID/checksum in EEPROM
 * M: fixed to SYSP_MpGrpSetDefaultsDoNotUpdateEeprom()
 *
 * Revision 1.8  2008/06/30 13:50:18  ufranke
 * R: read only group not supported
 * M: added SYSP_MpGrpCreateStdRo()
 *
 * Revision 1.7  2008/06/11 17:24:16  rt
 * R:1. Print a note that user has to reset board after changing an ee-X
 *      parameter if needed.
 * M:1.a) Set SYSPARAM_ACC_NEEDRESET attribute if needed.
 *
 * Revision 1.6  2005/06/23 15:54:30  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.5  2005/02/22 15:01:03  kp
 * avoid OS-9 compiler warnings
 *
 * Revision 1.4  2005/01/18 15:49:32  kp
 * added size to NVS sections
 *
 * Revision 1.3  2004/12/20 08:56:09  kp
 * support parameter attr. to place param into MMPARASTRING or to
 * restart SYSP lib
 *
 * Revision 1.2  2004/11/29 10:52:40  kp
 * getDefault support
 *
 * Revision 1.1  2004/01/08 09:06:55  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

#ifdef _EEPROD3_
#include "port.h"

#if 0
#define LOGLINE   printf(" %s  %d\n", __FUNCTION__, __LINE__)
#else
#define LOGLINE {}
#endif


/**********************************************************************/
/** Check if raw data of mpgrp has valid crc
 *
 */
static int HasValidMp( SYSP_PGRP_MAGPAR *mpGrp )
{
	u_int8 crc[2], raw_crc[2];
	u_int16 crc_val;
	LOGLINE;
	if( OSS_StrCmp(NULL,  (char*) mpGrp->parGrp.grpName, "prod3") == 0 ){
		LOGLINE;
		raw_crc[0] = mpGrp->rawData[sizeof(EEPROD3) -2 ];
		raw_crc[1] = mpGrp->rawData[sizeof(EEPROD3) -1 ];
		crc_val = PORT_EEPROM_ComputeCRC(mpGrp->rawData, mpGrp->size -2);
	}
	else if(OSS_StrCmp(NULL, (char *)mpGrp->parGrp.grpName, "menm") == 0){
		LOGLINE;
		raw_crc[0] = mpGrp->rawData[0 ];
		raw_crc[1] = mpGrp->rawData[1 ];
		crc_val = PORT_EEPROM_ComputeCRC(&mpGrp->rawData[2], mpGrp->size -2);
	}
	else{
		LOGLINE;
		return TRUE;
	}
	crc[0] = (unsigned char)((crc_val >> 8) & 0xFF);
	crc[1] = (unsigned char)( crc_val & 0xFF);
	LOGLINE;
	if( (crc[0] == raw_crc[0]) && (crc[1] == raw_crc[1]) ){
		if( (crc[0] != 0) || (crc[1] != 0) )
			LOGLINE;
			return TRUE;
	}

	LOGLINE;
	return FALSE;
}

/**********************************************************************/
/** Update crc of mpgrp
 *
 */
static void UpdateMp( SYSP_PGRP_MAGPAR *mpGrp )
{
	u_int16 crc_val;

	if( OSS_StrCmp(NULL, (char *)mpGrp->parGrp.grpName, "prod3") == 0 ){
		crc_val = PORT_EEPROM_ComputeCRC(mpGrp->rawData, mpGrp->size -2 );
		mpGrp->rawData[mpGrp->size -2 ] = (unsigned char)((crc_val >> 8) & 0xFF);
		mpGrp->rawData[mpGrp->size -1 ] = (unsigned char)( crc_val & 0xFF);
	}
	else if( OSS_StrCmp(NULL, (char *)mpGrp->parGrp.grpName, "menm") == 0 ){
		crc_val = PORT_EEPROM_ComputeCRC(&mpGrp->rawData[2], mpGrp->size -2 );
		mpGrp->rawData[0] = (unsigned char)((crc_val >> 8) & 0xFF);
		mpGrp->rawData[1] = (unsigned char)( crc_val & 0xFF);
	}

}

#else
/**********************************************************************/
/** Compute the 4-bit parity over a number of bytes
 *
 */
static u_int8 CalcParity( u_int8 *ptr, u_int32 len)
{
	u_int8 parity = 0xF;

	while( len-- ){
		parity ^= (*ptr >> 4);
		parity ^= (*ptr & 0xf);
		ptr++;
	}

	return parity;
}

/**********************************************************************/
/** Check if raw data of mpgrp has valid magic/parity
 *
 */
static int HasValidMp( SYSP_PGRP_MAGPAR *mpGrp )
{
	if( (mpGrp->rawData[0]>>4) == mpGrp->magic )
		if( CalcParity( mpGrp->rawData+1, mpGrp->size-1 ) ==
			(mpGrp->rawData[0] & 0xf) )
			return TRUE;
	return FALSE;
}

/**********************************************************************/
/** Update magic/parity of mpgrp
 *
 */
static void UpdateMp( SYSP_PGRP_MAGPAR *mpGrp )
{
	mpGrp->rawData[0] = (mpGrp->magic<<4) |
		CalcParity( mpGrp->rawData+1, mpGrp->size-1 );
}
#endif
/**********************************************************************/
/** Check if \a pw is the correct password for production data
 *
 */
int SYSP_ProdPasswdOk( const char *pw )
{
	const char *pw_enc = "YCMXCN";	/* "SIGRID" */

	if( pw == NULL )
		return FALSE;

	for( ; *pw; ++pw, ++pw_enc ) {
		if( *pw != (*pw_enc ^ 0x0a) )
			return FALSE;
	}

	return TRUE;
}

/**********************************************************************/
/** Get num parameters in mpgrp
 * \sa SYSP_PAR_GRP.numParams
 */
int SYSP_MpGrpNumParams( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	return mpGrp->numParams;
}

/**********************************************************************/
/** Enum mpgrp params
 * \sa SYSP_PAR_GRP.enumParams
 */
int SYSP_MpGrpEnumParams(
	SYSP_PAR_GRP *parGrp,
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;

	if( idx >= mpGrp->numParams )
		return SYSPARAM_INVAL_VALUE;

	parDesc = &mpGrp->parList[idx];

	SYSP_StrSafeCpy( info->parName,
					 (alias && parDesc->aliasName) ?
					 parDesc->aliasName :
					 parDesc->parName,
					 sizeof(info->parName));
	info->attr = (SYSPARAM_ACC_ATTR)(
		(mpGrp->mpFlags & SYSP_MPGRP_PROD ? SYSPARAM_ACC_PW : SYSPARAM_ACC_RW)|
		SYSPARAM_ACC_PERSISTANT
		| ((parDesc->pdFlags & SYSP_PD_MMPARASTR) ? SYSPARAM_ACC_MMPARASTR : 0)
		| ((parDesc->pdFlags & SYSP_PD_NEEDRESET) ? SYSPARAM_ACC_NEEDRESET : 0)
		| ((parDesc->pdFlags & SYSP_PD_ENCRYPT) ? SYSPARAM_ACC_ENCRYPT : 0)
		);

	return 0;
}

/**********************************************************************/
/** Set mpgrp parameter (for non-production data groups)
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_MpGrpSetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;

	/* find parameter descriptor */
	if( (parDesc = SYSP_FindParDesc( mpGrp->parList, parName )) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return SYSPARAM_INVAL_VALUE;
#ifdef _EEPROD3_
	if(parVal[0] == '\0')
		parVal = parDesc->defaultVal;
#endif

	/* validate value */
	if( parDesc->validate ){
		if( (rv = parDesc->validate( parVal, parDesc->vParam )) != 0 )
			return rv;
	}

	/* set new value */
	rv = parDesc->toRaw( &mpGrp->rawData[parDesc->offset], parVal );

	/* update raw data magic/parity */
	UpdateMp( mpGrp );

	if( (rv==0) && (parDesc->pdFlags & SYSP_PD_RESTART) )
		rv = SYSP_INTERNAL_RESTART;

	return rv;
}

/**********************************************************************/
/** Set mpgrp parameter (for production data groups)
 *
 * Additionally checks \a passwd.
 * If the \a hwName (parName=='brd') of the board is changed, return
 * #SYSP_INTERNAL_RESTART to force a restart of the entire SYSPARAM
 * lib.
 *
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_MpGrpSetParamProd(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	int rv;

	/* find parameter descriptor */
	if( SYSP_FindParDesc( mpGrp->parList, parName ) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	/* password check */
	if( SYSP_ProdPasswdOk( passwd ) == FALSE )
		return SYSPARAM_READ_ONLY;

	rv = SYSP_MpGrpSetParam( parGrp, parName, parVal, passwd, attr );

	return rv;
}

/**********************************************************************/
/** Get mpgrp parameter
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_MpGrpGetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	/* find parameter descriptor */
	if( (parDesc = SYSP_FindParDesc( mpGrp->parList, parName )) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */


	if( parVal == NULL )
		return 0;

	rv = parDesc->fromRaw( parVal, parValMaxLen,
						   &mpGrp->rawData[parDesc->offset]);

	return rv;
}

/**********************************************************************/
/** Get mpgrp parameter's default
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_MpGrpGetParamDefault(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	/* find parameter descriptor */
	if( (parDesc = SYSP_FindParDesc( mpGrp->parList, parName )) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return 0;

	if( parDesc->defaultVal == NULL )
		return SYSPARAM_INVAL_VALUE;

	return SYSP_StrSafeCpy( parVal, parDesc->defaultVal, parValMaxLen );
}

/**********************************************************************/
/** Set mpgrp to defaults (for non-production data groups)
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_MpGrpSetDefaults(
	SYSP_PAR_GRP *parGrp,
	int force)
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;
	for( parDesc=mpGrp->parList; parDesc->parName; parDesc++){

		parDesc->toRaw( &mpGrp->rawData[parDesc->offset],
						parDesc->defaultVal );
	}

	/* update raw data magic/parity */
	UpdateMp( mpGrp );
	return rv;
}

/**********************************************************************/
/** Set mpgrp to defaults (for production data groups)
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_MpGrpSetDefaultsProd(
	SYSP_PAR_GRP *parGrp,
	int force)
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	int rv=0;

	if( !force )
		return 0;

	/* set all bytes to FF */
	SYSP_MEMSET( mpGrp->rawData, 0xff, mpGrp->size );

	/* update raw data magic/parity */
	UpdateMp( mpGrp );
	return rv;
}

/**********************************************************************/
/** Set mpgrp to defaults (for production data groups)
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_MpGrpSetDefaultsDoNotUpdateEeprom( SYSP_PAR_GRP *parGrp, int force )
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;

	for( parDesc=mpGrp->parList; parDesc->parName; parDesc++){

		parDesc->toRaw( &mpGrp->rawData[parDesc->offset],
						parDesc->defaultVal );
	}

	return rv;
}

/**********************************************************************/
/** Save mpgrp
 * \sa SYSP_PAR_GRP.save
 */
int SYSP_MpGrpSave( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;

	return parGrp->nvs->writeNvs( parGrp->nvs, parGrp->nvsOffset,
								mpGrp->size, mpGrp->rawData );
}

/**********************************************************************/
/** Create a parameter group object for NVS section that uses magic/XOR parity
 *
 * - Creates and initializes a SYSP_PGRP_MAGPAR object.
 * - sets all method pointers to defaults
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN parameter group name
 * \param nvsOffset	\IN starting byte offset of section within NV storage
 * \param nvsSize	\IN size of section in bytes
 * \param parList	\IN array of parameters in this section
 * \param magic		\IN magic ID of section
 *
 * \return created mpGrp object (or NULL if failed)
 *
 */
SYSP_PGRP_MAGPAR *SYSP_MpGrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int grpSize = sizeof(*mpGrp) + nvsSize;

	/* alloc parameter group and raw data shadow */
	if( (mpGrp = SYSP_ALLOC( grpSize, nvs->super )) == NULL )
		return NULL;

	/* init object */
	mpGrp->parGrp.grpName		= name;
	mpGrp->parGrp.nvs 			= nvs;
	mpGrp->parGrp.nvsOffset	 	= nvsOffset;
	mpGrp->parGrp.numParams		= SYSP_MpGrpNumParams;
	mpGrp->parGrp.enumParams	= SYSP_MpGrpEnumParams;
	mpGrp->parGrp.setParam		= SYSP_MpGrpSetParam;
	mpGrp->parGrp.getParam		= SYSP_MpGrpGetParam;
	mpGrp->parGrp.getParamDefault = SYSP_MpGrpGetParamDefault;
	mpGrp->parGrp.setDefaults	= SYSP_MpGrpSetDefaults;
	mpGrp->parGrp.save			= SYSP_MpGrpSave;
	mpGrp->parList				= parList;
	mpGrp->size					= nvsSize;
	mpGrp->rawData				= (u_int8 *)(mpGrp+1);
	mpGrp->magic				= magic;
	mpGrp->mpFlags				= SYSP_MPGRP_NONE;
	mpGrp->numParams 			= SYSP_ParDescArrayLen( parList );

	return mpGrp;
}

/**********************************************************************/
/** Create and read magic/parity parameter for non-production data section
 *
 * - Creates and initializes a SYSP_PGRP_MAGPAR object.
 * - sets all method pointers to defaults
 * - reads NV storage
 * - checks magic parity
 * - sets defaults if either NV storage read failed or magic/parity bad
 * - Adds group to NVS group list
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN parameter group name
 * \param nvsOffset	\IN starting byte offset of section within NV storage
 * \param nvsSize	\IN size of section in bytes
 * \param parList	\IN array of parameters in this section
 * \param magic		\IN magic ID of section
 * \param mpGrpP	\OUT receives created group object (if non NULL)
 *
 * \return sysparam error code\n
 *   Any value != 0 means that the group has not been created
 *
 */
int SYSP_MpGrpCreateStd(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int rv;

	LOGLINE;

	if( mpGrpP )
		*mpGrpP = NULL;
	LOGLINE;
	if( (mpGrp = SYSP_MpGrpCreate( nvs, name, nvsOffset, nvsSize,
								   parList, magic )) == NULL ) {
		printf("no ressource!\n");
		return SYSPARAM_NO_RESOURCE;
	}
	LOGLINE;

	/* attempt to read raw data */
	rv = nvs->readNvs( nvs, nvsOffset, nvsSize, mpGrp->rawData );

	if( (rv != 0 ) || ! HasValidMp( mpGrp ) ) {
		printf("SYSP_MpGrpCreateStd setDefaults\n");
		mpGrp->parGrp.setDefaults( &mpGrp->parGrp, TRUE );
		LOGLINE;
	} else {
		/* printf("*** invalid data ? rv = %d \n", rv); */
	}

	rv = 0;

	SYSP_ParGrpAdd( nvs, &mpGrp->parGrp );

	if( mpGrpP )
		*mpGrpP = mpGrp;

	return rv;
}

/**********************************************************************/
/** same as SYSP_MpGrpCreateStd but group read only
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN parameter group name
 * \param nvsOffset	\IN starting byte offset of section within NV storage
 * \param nvsSize	\IN size of section in bytes
 * \param parList	\IN array of parameters in this section
 * \param magic		\IN magic ID of section
 * \param mpGrpP	\OUT receives created group object (if non NULL)
 *
 * \return sysparam error code\n
 *   Any value != 0 means that the group has not been created
 *
 */
int SYSP_MpGrpCreateStdRo(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int rv;

	if( mpGrpP )
		*mpGrpP = NULL;

	if( (mpGrp = SYSP_MpGrpCreate( nvs, name, nvsOffset, nvsSize,
								   parList, magic )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	mpGrp->parGrp.setParam 		= SYSP_MpGrpSetParamProd;
	mpGrp->parGrp.setDefaults 	= SYSP_MpGrpSetDefaultsDoNotUpdateEeprom;
	mpGrp->mpFlags 				|= SYSP_MPGRP_PROD;

	/* attempt to read raw data */
	rv = nvs->readNvs( nvs, nvsOffset, nvsSize, mpGrp->rawData );

	if( (rv != 0 ) || ! HasValidMp( mpGrp ) )
		mpGrp->parGrp.setDefaults( &mpGrp->parGrp, TRUE );

	rv = 0;

	SYSP_ParGrpAdd( nvs, &mpGrp->parGrp );

	if( mpGrpP )
		*mpGrpP = mpGrp;

	return rv;

}

/**********************************************************************/
/** Create and read magic/parity parameter for production data section
 *
 * - Creates and initializes a SYSP_PGRP_MAGPAR object.
 * - sets all method pointers to defaults
 * - reads NV storage
 * - checks magic parity
 * - sets defaults if either NV storage read failed or magic/parity bad
 *   (defaults are all FF's).
 * - The group must be added to NVS group list by caller!
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN parameter group name
 * \param nvsOffset	\IN starting byte offset of section within NV storage
 * \param nvsSize	\IN size of section in bytes
 * \param parList	\IN array of parameters in this section. Default
 *						values in parList are ignored!
 * \param magic		\IN magic ID of section
 * \param mpGrpP	\OUT receives created group object
 *
 * \return sysparam error code\n
 *  - SYSPARAM_EEPROM_ERROR: NVS read failed (mpGrp created)
 *	- SYSPARAM_INVAL_VALUE: bad magic/parity (mpGrp created)
 *
 */
int SYSP_MpGrpCreateProd(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int setDef=FALSE;
	int rv;

	*mpGrpP = NULL;

	if( (mpGrp = SYSP_MpGrpCreate( nvs, name, nvsOffset, nvsSize,
								   parList, magic )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	mpGrp->parGrp.setParam 		= SYSP_MpGrpSetParamProd;
	mpGrp->parGrp.setDefaults 	= SYSP_MpGrpSetDefaultsProd;
	mpGrp->mpFlags 				|= SYSP_MPGRP_PROD;

	/* attempt to read raw data */
	rv = nvs->readNvs( nvs, nvsOffset, nvsSize, mpGrp->rawData );

	if( rv != 0 )
		setDef = TRUE;
	else if( ! HasValidMp( mpGrp ) ){
		setDef = TRUE;
		rv = SYSPARAM_INVAL_VALUE;
	}

	if( setDef )
		mpGrp->parGrp.setDefaults( &mpGrp->parGrp, TRUE );

	*mpGrpP = mpGrp;

	return rv;
}

/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST

static u_int8 G_physBuf[256];
static int G_simNvsError;

static int UtReadNvs(
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	u_int8 *data)
{
	if( G_simNvsError )
		return SYSPARAM_EEPROM_ERROR;
	SYSP_MEMMOVE( data, &G_physBuf[offset], size );
	return 0;
}

static int UtWriteNvs(
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	const u_int8 *data)
{
	if( G_simNvsError )
		return SYSPARAM_EEPROM_ERROR;
	SYSP_MEMMOVE( &G_physBuf[offset], data, size );
	return 0;
}

/** check if all parameters listed */
static int UtEnumParams( SYSP_PGRP_MAGPAR *mpGrp )
{
	SYSPARAM_PARAM_INFO info;
	SYSP_PAR_GRP *parGrp = &mpGrp->parGrp;

	/* official names */
	UTASSERT( parGrp->enumParams( parGrp, 0, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "sernbr", 10 ) == 0 );
	if( mpGrp->mpFlags & SYSP_MPGRP_PROD ){
		UTASSERT( info.attr == (SYSPARAM_ACC_PERSISTANT|SYSPARAM_ACC_PW));
	}
	else {
		UTASSERT( info.attr == (SYSPARAM_ACC_PERSISTANT|SYSPARAM_ACC_RW));
	}

	UTASSERT( parGrp->enumParams( parGrp, 1, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "brdmod", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 2, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "brd", 10 ) == 0 );
	if( mpGrp->mpFlags & SYSP_MPGRP_PROD ){
		UTASSERT( info.attr == (SYSPARAM_ACC_PERSISTANT|SYSPARAM_ACC_PW
								|SYSPARAM_ACC_MMPARASTR));
	}
	else {
		UTASSERT( info.attr == (SYSPARAM_ACC_PERSISTANT|SYSPARAM_ACC_RW
								|SYSPARAM_ACC_MMPARASTR));
	}

	UTASSERT( parGrp->enumParams( parGrp, 3, FALSE, &info ) ==
			  SYSPARAM_INVAL_VALUE );

	/* alias names */
	UTASSERT( parGrp->enumParams( parGrp, 0, TRUE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "sernbr", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 1, TRUE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "brdmod", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 2, TRUE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "brd-alias", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 3, TRUE, &info ) ==
			  SYSPARAM_INVAL_VALUE );

	return 0;
 ABORT:
	return -1;
}

SYSP_UNITTEST( "SYSPARAM2: Magic/Parity group", SYSP_UtMpGrp );
#ifdef _EEPROD3_
int SYSP_UtMpGrp( void )
{
	return 0;
}
#else
#define nvsSize 12
/** Checks MpGrp functions */
int SYSP_UtMpGrp( void )
{
	const SYSP_PAR_DESC parList[] = {
		SYSP_PDE_U32D( "sernbr", 	NULL, 1, "1234", 0),
		SYSP_PDE_U8D( "brdmod",  	NULL, 5, "0", 0),
		SYSP_PDE_STR( "brd", 		"brd-alias", 6, "BRD", 6,
					  SYSP_PD_MMPARASTR),
		SYSP_PDE_END
	};
	const int nvsOff = 0x10;
	SYSP_NV_STORAGE *nvs;
	SYSP_NVS_ADR nvsAdr;
	SYSP_PGRP_MAGPAR *mpGrp;
	int rv;

	SYSP_UtSuperInit();

	/* create dummy nvs */
	SYSP_MEMSET( &nvsAdr, 0, sizeof(nvsAdr));
	UTASSERT( nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE),
									"nvs", SYSP_super, "", nvsAdr, nvsSize ));
	nvs->readNvs = UtReadNvs;
	nvs->writeNvs = UtWriteNvs;
	/* Empty EEPROM (prod. data) */
	{
		u_int8 def[nvsSize];
		u_int8 buf[10];
		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		G_simNvsError = FALSE;
		SYSP_MEMSET( def, 0xff, sizeof(def) );
		def[0] = 0xd0 | CalcParity( &def[1], sizeof(def)-1 );

		rv = SYSP_MpGrpCreateProd( nvs, "prod", nvsOff,
								 nvsSize, parList, 0xd, &mpGrp );
		/* defaults correctly applied? */
		UTASSERT( rv == SYSPARAM_INVAL_VALUE );
		UTASSERT( mpGrp != NULL );
		UTASSERT( SYSP_MEMCMP( def, mpGrp->rawData, sizeof(def) ) == 0 );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "brd",
										  buf, sizeof(buf)) ==
				  SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_STRNCMP( buf, "", 10 ) == 0 );

		UTASSERT( UtEnumParams( mpGrp ) == 0 );
		SYSP_ALLOC_UNDO( sizeof(SYSP_PGRP_MAGPAR)+nvsSize, nvs->super );

	}
	/* EEPROM error (prod. data) */
	{
		u_int8 def[nvsSize];
		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		G_simNvsError = TRUE;

		SYSP_MEMSET( def, 0xff, sizeof(def) );
		def[0] = 0xd0 | CalcParity( &def[1], sizeof(def)-1 );

		rv = SYSP_MpGrpCreateProd( nvs, "prod", nvsOff,
								 nvsSize, parList, 0xd, &mpGrp );
		/* defaults correctly applied? */
		UTASSERT( rv == SYSPARAM_EEPROM_ERROR );
		UTASSERT( mpGrp != NULL );
		UTASSERT( SYSP_MEMCMP( def, mpGrp->rawData, sizeof(def) ) == 0 );

		UTASSERT( UtEnumParams( mpGrp ) == 0 );
		SYSP_ALLOC_UNDO( sizeof(SYSP_PGRP_MAGPAR)+nvsSize, nvs->super );
	}

	G_simNvsError = FALSE;

	/* Good EEPROM content (prod. data) */
	{
		u_int8 okCont[] =
			{ 0xd0, 0x00, 0x00, 0x11, 0x5c, 0x03, 'A', 'B', 'c', 0,
			  0xff, 0xff};
		u_int8 buf[10];

		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );


		okCont[0] = 0xd0 | CalcParity( &okCont[1], sizeof(okCont)-1 );
		SYSP_MEMMOVE( &G_physBuf[nvsOff], okCont, sizeof(okCont) );

		rv = SYSP_MpGrpCreateProd( nvs, "prod", nvsOff,
								 nvsSize, parList, 0xd, &mpGrp );

		UTASSERT( rv == 0 );
		UTASSERT( mpGrp != NULL );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "sernbr",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "4444", 10 ) == 0 );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "brd",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "ABc", 10 ) == 0 );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "brdmod",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "3", 10 ) == 0 );

		UTASSERT( UtEnumParams( mpGrp ) == 0 );

	}
	/* mpGrp still valid here */

	/* setparam/save/setDefaults to PROD */
	{
		SYSP_PAR_GRP *parGrp = &mpGrp->parGrp;
		const char *passwd = "SIGRID";
		char buf[10];
		u_int8 okCont[] =
			{ 0xd0, 0x00, 0x00, 0x04, 0xd2, 0x03, 'n', 'e', 'w', 0,
			  0xff, 0xff};

		/* null value */
		UTASSERT( parGrp->setParam( parGrp, "brd", NULL, "SIGRID", 0 ) ==
				  SYSPARAM_INVAL_VALUE );

		/* null passwd */
		UTASSERT( parGrp->setParam( parGrp, "brd", "abc", NULL, 0 ) ==
				  SYSPARAM_READ_ONLY );

		/* bad passwd */
		UTASSERT( parGrp->setParam( parGrp, "brd", "abc", "sigrid", 0 ) ==
				  SYSPARAM_READ_ONLY );

		/* bad parameter name */
		UTASSERT( parGrp->setParam( parGrp, "brdx", "abc", "sigrid", 0 ) ==
				  SYSPARAM_INVAL_PARAM );

		/* ok */
		UTASSERT( parGrp->setParam( parGrp, "brd-alias",
									"new", passwd, 0 ) == 0 );
		UTASSERT( parGrp->setParam( parGrp, "sernbr", "1234",
									passwd, 0 ) == 0 );

		/* save */
		UTASSERT( parGrp->save( parGrp ) == 0);

		okCont[0] = 0xd0 | CalcParity( &okCont[1], sizeof(okCont)-1 );
		UTASSERT( SYSP_MEMCMP( &G_physBuf[nvsOff], okCont, sizeof(okCont))==0);

		/* set defaults (not forced) */
		UTASSERT( parGrp->setDefaults( parGrp, FALSE ) == 0);
		UTASSERT( parGrp->getParam( parGrp, "brd", buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "new", 10 ) == 0 );

		/* set defaults (forced) */
		UTASSERT( parGrp->setDefaults( parGrp, TRUE ) == 0);
		UTASSERT( parGrp->getParam( parGrp, "brd", buf, sizeof(buf))
				  == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_STRNCMP( buf, "", 10 ) == 0 );


		SYSP_ALLOC_UNDO( sizeof(SYSP_PGRP_MAGPAR)+nvsSize, nvs->super );
	}


	/* Invalid EEPROM content (non-prod. data) */
	{
		u_int8 buf[10];

		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		rv = SYSP_MpGrpCreateStd( nvs, "std", nvsOff,
								nvsSize, parList, 0xa, &mpGrp );

		UTASSERT( rv == 0 );
		UTASSERT( mpGrp != NULL );

		/* check if defaults ok */
		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "sernbr",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "1234", 10 ) == 0 );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "brd",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "BRD", 10 ) == 0 );

		UTASSERT( mpGrp->parGrp.getParam( &mpGrp->parGrp, "brdmod",
										  buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "0", 10 ) == 0 );

		UTASSERT( UtEnumParams( mpGrp ) == 0 );

		/* check getParamDefault */
		UTASSERT( mpGrp->parGrp.getParamDefault( &mpGrp->parGrp, "brd",
												 buf, sizeof(buf)) == 0 );

		UTASSERT( SYSP_STRNCMP( buf, "BRD", 10 ) == 0 );

	}


	DBGWRT_1((DBH,"SYSP_UtMpGrp OK\n"));
	return 0;
 ABORT:
	return -1;
}
#endif

#endif /* SYSP_CONFIG_UNITTEST */

