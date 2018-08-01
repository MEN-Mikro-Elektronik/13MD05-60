/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_strgrp.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/03/08 11:19:56 $
 *    $Revision: 1.7 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Parameter group handler for NV sections containg a single
 *				 string
 *
 *  Handles only one parameter that has the same name as the param group.
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_strgrp.c,v $
 * Revision 1.7  2007/03/08 11:19:56  cs
 * added:
 *   - casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.6  2005/09/05 11:43:27  kp
 * Support getDefault() method
 *
 * Revision 1.5  2005/06/23 15:54:32  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.4  2005/02/22 15:01:05  kp
 * avoid OS-9 compiler warnings
 *
 * Revision 1.3  2005/01/18 15:49:35  kp
 * added size to NVS sections
 *
 * Revision 1.2  2004/11/29 10:52:58  kp
 * MM unittest support
 *
 * Revision 1.1  2004/01/08 09:06:56  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"


typedef struct {
	SYSP_PAR_GRP parGrp;		/**< common parameter group object  */
	const char *alias;			/**< alias name for parameter  */
	const char *defVal;			/**< string default value  */
	char *str;					/**< current value of string (behind obj) */
	int size;					/**< size of NV area  */
} SYSP_PGRP_STR;

static int StrValidate( const char *parVal );

/**********************************************************************/
/** Get num parameters in mpgrp
 * \sa SYSP_PAR_GRP.numParams
 */
int SYSP_StrGrpNumParams( SYSP_PAR_GRP *parGrp )
{
	return 1;
}

/**********************************************************************/
/** Enum strGrp params
 * \sa SYSP_PAR_GRP.enumParams
 */
int SYSP_StrGrpEnumParams(
	SYSP_PAR_GRP *parGrp,
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;

	if( idx == 0 ){
		SYSP_StrSafeCpy( info->parName,
						 (alias && strGrp->alias)
						 ? strGrp->alias : parGrp->grpName,
						 sizeof(info->parName));
		info->attr = (SYSPARAM_ACC_ATTR)
			(SYSPARAM_ACC_RW | SYSPARAM_ACC_PERSISTANT);
		return 0;
	}
	else {
		return SYSPARAM_INVAL_VALUE;
	}
}


/**********************************************************************/
/** Set strGrp parameter
 *
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_StrGrpSetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;
	int rv=0;

	if( (SYSP_STRCMP( parName, parGrp->grpName ) != 0) &&
		(strGrp->alias==NULL || SYSP_STRCMP( parName, strGrp->alias ) != 0 ))
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	/* validate value */
	if( (parVal == NULL) ||
		((rv = StrValidate( parVal )) != 0) ||
		(SYSP_STRLEN(parVal) > (u_int32)(strGrp->size-1)) )
		return SYSPARAM_INVAL_VALUE;

	/* copy to object buffer */
	SYSP_STRCPY( strGrp->str, parVal );

	return rv;
}


/**********************************************************************/
/** Get strGrp parameter
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_StrGrpGetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( (SYSP_STRCMP( parName, parGrp->grpName ) != 0) &&
		((strGrp->alias==NULL) || (SYSP_STRCMP( parName, strGrp->alias ) !=0)))
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return 0;

	return SYSP_StrSafeCpy( parVal, strGrp->str, parValMaxLen );
}

/**********************************************************************/
/** Get strGrp parameter's default
 * \sa SYSP_PAR_GRP.getParamDefault
 */
int SYSP_StrGrpGetParamDefault(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( (SYSP_STRCMP( parName, parGrp->grpName ) != 0) &&
		((strGrp->alias==NULL) || (SYSP_STRCMP( parName, strGrp->alias ) !=0)))
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return 0;

	return SYSP_StrSafeCpy( parVal, strGrp->defVal, parValMaxLen );
}

/**********************************************************************/
/** Set strGrp to defaults
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_StrGrpSetDefaults(
	SYSP_PAR_GRP *parGrp,
	int force)
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;

	SYSP_STRCPY( strGrp->str, strGrp->defVal );

	return 0;
}

/**********************************************************************/
/** Save strGrp
 * \sa SYSP_PAR_GRP.save
 */
int SYSP_StrGrpSave( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_STR *strGrp = (SYSP_PGRP_STR *)parGrp;
	int rv;

	strGrp->str[strGrp->size-1] = '\0';

	rv = parGrp->nvs->writeNvs( parGrp->nvs, parGrp->nvsOffset,
								SYSP_STRLEN(strGrp->str)+1,
								(u_int8 *)strGrp->str );
	return rv;
}


/**********************************************************************/
/** Check if string contains invalid chars
 */
static int StrValidate( const char *parVal )
{
	while( *parVal ){
		if( !SYSP_PRINTABLE( *parVal ) )
			return SYSPARAM_INVAL_VALUE;
		parVal++;
	}
	return 0;
}

/**********************************************************************/
/** Create a parameter group object for simple strings
 *
 * On success, the parameter group is added to the list of \a nvs groups
 *
 * The string is read from the non volatile storage. If there is a
 * physical read error, or if it contains non-printable chars, default
 * string is setup.
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN name of parameter group and name of the parameter
 *						itsself
 * \param alias		\IN alias name of parameter (can be NULL)
 * \param nvsOffset	\IN starting byte offset of string within NV storage
 * \param nvsSize	\IN max size of string in NV storage in bytes
 * \param defVal 	\IN default value for string (can be NULL)
 *
 * \return sysparam error code
 *
 */
int SYSP_StrGrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	const char *alias,
	int nvsOffset,
	int nvsSize,
	const char *defVal)
{
	SYSP_PGRP_STR *strGrp = NULL;
	int grpSize = sizeof(*strGrp) + nvsSize;
	int rv=0;

	/* alloc parameter group and raw data shadow */
	if( (strGrp = SYSP_ALLOC( grpSize, nvs->super )) == NULL )
		return SYSPARAM_NO_RESOURCE;


	if( defVal == NULL )
		defVal = "";

	/* init object */

	strGrp->parGrp.grpName		= name;
	strGrp->parGrp.nvs 			= nvs;
	strGrp->parGrp.nvsOffset	= nvsOffset;
	strGrp->parGrp.numParams	= SYSP_StrGrpNumParams;
	strGrp->parGrp.enumParams	= SYSP_StrGrpEnumParams;
	strGrp->parGrp.setParam		= SYSP_StrGrpSetParam;
	strGrp->parGrp.getParam		= SYSP_StrGrpGetParam;
	strGrp->parGrp.setDefaults	= SYSP_StrGrpSetDefaults;
	strGrp->parGrp.getParamDefault = SYSP_StrGrpGetParamDefault;
	strGrp->parGrp.save			= SYSP_StrGrpSave;
	strGrp->str					= (char *)(strGrp+1);
	strGrp->defVal				= defVal;
	strGrp->size				= nvsSize;
	strGrp->alias				= alias;

	/* attempt to read raw data */
	rv = nvs->readNvs( nvs, nvsOffset, nvsSize, (u_int8 *)strGrp->str );

	/* make sure it's null terminated */
	strGrp->str[nvsSize-1] = '\0';

	if( (rv != 0) || StrValidate( strGrp->str )){
		SYSP_STRCPY( strGrp->str, defVal);
		rv = 0;
	}

	SYSP_ParGrpAdd( nvs, &strGrp->parGrp );

	if( rv != 0 )
		SYSP_ALLOC_UNDO( grpSize, nvs->super );
	return rv;
}

/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST
SYSP_UNITTEST( "SYSPARAM2: String group", SYSP_UtStrGrp );

static u_int8 G_physBuf[0x30];
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

static SYSP_NV_STORAGE *UtRestart(void)
{
	SYSP_NV_STORAGE *nvs;
	SYSP_NVS_ADR nvsAdr;

	SYSP_UtSuperInit();

	/* create dummy nvs */
	SYSP_MEMSET( &nvsAdr, 0, sizeof(nvsAdr));
	nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE),
						  "nvs", SYSP_super, "", nvsAdr, sizeof(G_physBuf) );
	nvs->readNvs = UtReadNvs;
	nvs->writeNvs = UtWriteNvs;
	return nvs;
}


/** Checks VxBl functions */
int SYSP_UtStrGrp( void )
{
	const int nvsOff = 0x10;
	const int nvsSize = 0x10;
	SYSP_PGRP_STR *strGrp;
	SYSP_PAR_GRP *parGrp;
	SYSP_NV_STORAGE *nvs;
	char parVal[20];


	/* Empty EEPROM  */
	{
		nvs = UtRestart();
		G_simNvsError = FALSE;
		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		UTASSERT( SYSP_StrGrpCreate( nvs, "startup", NULL,
									 nvsOff, nvsSize,
									 "default string" ) == 0 );

		strGrp = (SYSP_PGRP_STR *)nvs->parGrpLst;
		parGrp = &strGrp->parGrp;

		UTASSERT( parGrp->getParam( parGrp, "startup", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "default string" ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "mmstartup", parVal, 20) ==
			SYSPARAM_INVAL_PARAM);
	}

	/* Read error on EEPROM */
	{
		nvs = UtRestart();
		G_simNvsError = TRUE;

		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		UTASSERT( SYSP_StrGrpCreate( nvs, "mmstartup", "startup",
									 nvsOff, nvsSize,
									 "default string2" ) == 0 );

		strGrp = (SYSP_PGRP_STR *)nvs->parGrpLst;
		parGrp = &strGrp->parGrp;

		UTASSERT( parGrp->getParam( parGrp, "startup", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "default string2" ) == 0 );
	}

	G_simNvsError = FALSE;

	/* good string content */
	{
		nvs = UtRestart();

		SYSP_STRCPY( (char *)&G_physBuf[nvsOff], "initial value" );

		UTASSERT( SYSP_StrGrpCreate( nvs, "mmstartup", "startup",
									 nvsOff, nvsSize,
									 "default string2" ) == 0 );

		strGrp = (SYSP_PGRP_STR *)nvs->parGrpLst;
		parGrp = &strGrp->parGrp;

		UTASSERT( parGrp->getParam( parGrp, "mmstartup", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "initial value" ) == 0 );
	}


	/* enum params */
	{
		SYSPARAM_PARAM_INFO info;

		/* official name */
		UTASSERT( parGrp->enumParams( parGrp, 0, FALSE, &info ) == 0 );
		UTASSERT( SYSP_STRNCMP( info.parName, "mmstartup", 10 ) == 0 );
		UTASSERT( info.attr == (SYSPARAM_ACC_RW | SYSPARAM_ACC_PERSISTANT) );

		UTASSERT( parGrp->enumParams( parGrp, 0, TRUE, &info ) == 0 );
		UTASSERT( SYSP_STRNCMP( info.parName, "startup", 10 ) == 0 );

		/* termination */
		UTASSERT( parGrp->enumParams( parGrp, 1, FALSE, &info ) ==
				  SYSPARAM_INVAL_VALUE );

	}

	/* set params */
	{
		UTASSERT( parGrp->setParam( parGrp, "startup", "blabla", NULL, 0 )
				  == 0 );
		UTASSERT( SYSP_STRCMP( strGrp->str, "blabla" ) == 0 );

		UTASSERT( parGrp->setParam( parGrp, "startup", "x\n", NULL, 0 )
				  == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_STRCMP( strGrp->str, "blabla" ) == 0 );

		UTASSERT( parGrp->setParam( parGrp, "startupx", "x\n", NULL, 0 )
				  == SYSPARAM_INVAL_PARAM );

	}

	/* save */
	{
		UTASSERT( parGrp->save( parGrp ) == 0 );
		UTASSERT( SYSP_STRCMP( (char *)&G_physBuf[nvsOff],
							   "blabla") == 0 );

	}
	DBGWRT_1((DBH,"SYSP_UtStrGrp OK\n"));

	return 0;
 ABORT:
	return -1;
}


#endif /* SYSP_CONFIG_UNITTEST */
