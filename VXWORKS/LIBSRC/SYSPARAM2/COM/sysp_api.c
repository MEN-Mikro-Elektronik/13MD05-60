/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_api.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/06/07 16:13:30 $
 *    $Revision: 1.9 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Exported common routines of SYSPARAM
 *
 *    \switches  -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_api.c,v $
 * Revision 1.9  2010/06/07 16:13:30  sy
 * R:1. EEPROD3 reserves no space for the '\0' at the end of each parameter
 * M: 1. The length of each parameer is hard coded.
 *
 * Revision 1.8  2007/03/08 11:19:52  cs
 * added:
 *   - casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.7  2006/01/20 13:00:57  UFRANKE
 * removed
 *  - switch  SUPPORT_EEPROD2_ON_ESM_CB
 *
 * Revision 1.6  2006/01/05 15:03:56  cs
 * added
 *     + support for carrier boards with EEPROD2 structure in EEPROM
 * cosmetics
 *
 * Revision 1.5  2005/06/23 15:54:05  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.4  2005/01/12 14:19:46  kp
 * run users post processing hook after each set parameter
 *
 * Revision 1.3  2004/11/29 10:51:35  kp
 * added SysparamGetDefault support
 *
 * Revision 1.2  2004/10/06 17:15:15  kp
 * bug fix: all API functions now return error when super object pointer
 * is 0
 *
 * Revision 1.1  2004/01/08 09:06:45  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

/***************************************************************************/
/*! \mainpage

 The system parameter library provides a generic way to access parameters,
 such as

 - parameters stored in non volatile storage (NVS)
   - board inventory parameters (production data)
   - configurable parameters (e.g. console baudrate)
 - parameters auto-detected by firmware


 \section sysp_api User callable routines
 - SysParamGet()
 - SysParamSet()
 - SysParamSetEx()
 - SysParamEnum()
 - SysParamGetAll()
 - SysParamSetDefaults()
 - SysParamMakePersistant()
 - SysParamErrStr()

 Before any of these functions can be called, a system specific init
 routine must be called, e.g. Em04SysParamInit().

 \sa \ref apidiffs

*/

/***************************************************************************/
/*! \page apidiffs API Differences to original SYSPARAM lib

  New API routines:

  - Users must include sysparam2.h rather sysparam.h
  - New API routine SysParamSetEx() to
     - set parameter without writing it to non volatile storage
	 - set read-only parameter from firmware
  - New API routine SysParamSetFmt() to generate a value with printf
    arguments
  - New API routine SysParamNumParams() and SysParamEnum()
    to enumerate parameters
  - New API routine SysParamMakePersistant() to save all or
    a group of parameters
  - New API routine SysParamErrStr() to convert error code to string

  Different behaviour:
  - Parameters in production data section are visible to SysParamGet()
    and SysParamEnum() even if the production data section is not
	valid. SysParamGet() returns #SYSPARAM_INVAL_VALUE in this case
  - Parameter \em vxBlineFromEeprom	to xxxSysParamInit() function removed.
    vxBline is now read from EEPROM when it contains an empty string
  - vxBline can now be passed a default value (through 	SYSP_VxGrpCreate())
  - SysParamGet() and SysParamGetAll() return SYSPARAM_VAL_TRUNCATED
    if the user's buffer is too small
  - SysParamSet() returns SYSPARAM_VAL_TRUNCATED if not all parameters
    could be stored in VxWorks bootline
  - SysParamSet() returns SYSPARAM_NO_RESOURCE if a string parameter
    is too long (value is not changed in this case)
  - SysParamGet() can be used to test for the existance of a parameter
    without getting the value by passing parVal as NULL
  - VxWorks bootline handler now supports all VxWorks parameters, i.e.
    "b", "u", "p", "o", "f"
  - Much better parameter validation for SysParamSet()
  - New parameter \em alias to SysParamGetAll() to include alias names
    rather official names in output string



*/

/*!
 * \page dummy
 * \menimages
 */

/** max. length of each parameter string for SysParamGetAll */
#define SYSP_GETALL_MAX_PARLEN 		512

/**********************************************************************/
/** Modify value of single system parameter (extended version)
 *
 * This updates the internal representation of the specified parameter
 * \a parName. Parameter name can be either the official parameter name
 * or the alias name.
 *
 * If #SYSPARAM_SA_NON_PERSISTANT is not specified in \a attr, the
 * parameter is also written into non volatile storage (NVS).
 *
 * If #SYSPARAM_SA_NON_PERSISTANT is given, NVS is not updated. NVS
 * can be updated by another call to SysParamSetEx for a parameter in
 * the same parameter group (with SYSPARAM_SA_NON_PERSISTANT clear) or
 * by calling SysParamMakePersistant().
 *
 * Parameters that are read-only for the normal user can be written to
 * by specifying attribute #SYSPARAM_SA_FROM_FIRMWARE in \a attr. This
 * flag should be set only when called from firmware.
 *
 * A parameter's value can be cleared by passing an empty string ("") in
 * \a parVal.
 *
 * <b>Restart when production data name has changed</b>
 * When modifying the name of a production data section ("brd" parameter),
 * the SYSPARAM library is entirely restarted, since the new name may
 * imply a new EEPROM layout. After SYSPARAM has been restart (which
 * is invisible to the user) the available parameters may have changed.
 * Modifying the board name is only possible with #SYSPARAM_SA_NON_PERSISTANT
 * flag cleared.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param parVal		\IN new value for parameter as a string
 * \param passwd		\IN an optional password for protected parameters
 *							can be NULL if parameter is not protected
 * \param attr			\IN attribute flags (see #SYSPARAM_SET_ATTR)
 *
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_PARAM: parameter is not known
 * 	 - #SYSPARAM_INVAL_VALUE: invalid value for parameter or \a parVal too long
 *	 - #SYSPARAM_EEPROM_ERROR: Error during NVS update
 *	 - #SYSPARAM_READ_ONLY: Parameter is read only and
 *		#SYSPARAM_SA_FROM_FIRMWARE was not set in \a attr, or bad password
 * 		was specified.
 *	 - #SYSPARAM_NO_RESOURCE: \a parVal could not be stored due to lack
 *								 of space (in NV storage or temp. buffers).
 *	 - #SYSPARAM_VAL_TRUNCATED: only parts of \a parVal have been stored.
 */
int SysParamSetEx(
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr )
{
	SYSP_PAR_GRP *parGrp;
	SYSP_NV_STORAGE *nvs;
	const char *pureName;
	int rv=SYSPARAM_INVAL_PARAM, rv2;
	SYSP_SUPER *super = SYSP_super;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	/* find parameter group that handles that parameter */
	if( (parGrp = SYSP_ParNameToGroup( super, parName,
									   &pureName )) != NULL ){

		/* allow carrier boards to have EEPROD2 */
		{
			char  tmpPrfx[4];
			const char *tmpParN = SYSP_Prefix( parName, tmpPrfx );

			if( *tmpPrfx && (SYSP_STRNCMP( tmpParN, "brd", 6) == 0) )
			{
				SYSP_PGRP_MAGPAR *mpGrp = (SYSP_PGRP_MAGPAR *)parGrp;

				if( SYSP_IsEsmCB_with_EEPROD2( parVal ) )
				{
					/* brd name matched */
					if( mpGrp->magic != EEID_PD2 )
						mpGrp->magic = EEID_PD2;

				} else
				{
					if( mpGrp->magic != EEID_PD )
						mpGrp->magic = EEID_PD;
				}
			}
		}
		nvs = parGrp->nvs;

		/* set parameter (non persistant) */
		rv = nvs->setParam( nvs, parName, parVal, passwd, attr );			
		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM)){

			/* if persistant, save group to NV storage */
			if( (rv==0 ||
				 rv==SYSPARAM_VAL_TRUNCATED ||
				 rv==SYSP_INTERNAL_RESTART ) &&
				!(attr & SYSPARAM_SA_NON_PERSISTANT) ){
												
				if( parGrp->save ){
					if( (rv2 = parGrp->save( parGrp )) != 0 ){
						rv = rv2;						
					}					
				}
			}
		}
	}

	/*
	 * if the name in a board EEPROM was changed, restart entire SYSPARAM lib
	 */
	if( rv == SYSP_INTERNAL_RESTART ){		
		rv = super->reInit( super );		
	}
	else {		
		rv2 = SYSP_RunPostProcessHooks( super, parName, attr, rv );		
		if( rv == 0 && rv2 != 0)
			rv = rv2;
	}
	return rv;
}

/**********************************************************************/
/** Modify value of single system parameter
 *
 * This updates the internal representation of the specified parameter
 * \a parName and the parameter is written to non-volatile storage.
 *
 * It is a shortcut to
 * \code SysParamSetEx( parName, parVal, passwd, 0 ); \endcode
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param parVal		\IN new value for parameter as a string
 * \param passwd		\IN an optional password for protected parameters
 *							can be NULL if parameter is not protected
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_PARAM: parameter is not known
 * 	 - #SYSPARAM_INVAL_VALUE: invalid value for parameter or \a parVal too long
 *	 - #SYSPARAM_EEPROM_ERROR: Error during NVS update
 *	 - #SYSPARAM_READ_ONLY: Parameter is read only or bad password
 * 							was specified.
 *	 - #SYSPARAM_NO_RESOURCE: \a parVal could not be stored due to lack
 *								 of space (in NV storage or temp. buffers).
 *	 - #SYSPARAM_VAL_TRUNCATED: only parts of \a parVal have been stored.
 *
 * \sa SysParamSetEx
 */
int SysParamSet(
	const char *parName,
	const char *parVal,
	const char *passwd )
{
	return SysParamSetEx( parName, parVal, passwd, 0 );	
}

/**********************************************************************/
/** Get value of single system parameter
 *
 * This gets the current value of the parameter \a parName. Parameter
 * name can be either the official parameter name or the alias name.
 *
 * If SysParamGet() returns non-zero, \a parVal will contain an empty
 * string (unless \a parValMaxLen is \c 0).
 *
 * If the parameter value (as a string) is longer than \a parValMaxLen,
 * string in \a parVal is truncated.
 * \a parVal is always guaranteed to be null-terminated.
 *
 * \a parVal may contain blanks.
 *
 * This function can be used to probe for the existance of a parameter name.
 * Pass \a parVal as NULL and parValMaxLen as 0 in this case.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param parVal		\OUT receives the parameter's value as a string
 * \param parValMaxLen	\IN maximum bytes that \a parVal can hold, including
 *							the terminating '\\0'
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_PARAM: parameter is not known
 *	 - #SYSPARAM_INVAL_VALUE: parValMaxLen is 0 and \a parVal is not NULL.
 *							  Or internal representation is damaged
 *	 - #SYSPARAM_VAL_TRUNCATED: only a part of value could be copied to
 *						   		\a parVal (and \a parVal was not NULL)
 *
 */
int SysParamGet(
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_NV_STORAGE *nvs;
	int rv=SYSPARAM_INVAL_PARAM;
	SYSP_SUPER *super = SYSP_super;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;


	/* loop trough NVS objects until parameter is handled */
	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		rv = nvs->getParam( nvs, parName, parVal, parValMaxLen );
		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM))
			break;
	}
	
#ifdef _EEPROD3_	
	{
		int len = 0;
		if( OSS_StrCmp( NULL, (char*)parName, (char*)SYSP_brd) == 0)
			len = 4;		
		else if ( OSS_StrCmp( NULL, (char*)parName, (char*)SYSP_brdmod) == 0)
			len = 9;
		else if ( OSS_StrCmp( NULL, (char*)parName, (char*)SYSP_brdrev) == 0)	
			len = 8;
		else if ( OSS_StrCmp( NULL, (char*)parName, (char*)SYSP_sernbr) == 0)	
			len = 6;
		if(len)
			parVal[len] = '\0';
	}
#endif
	return rv;
}

/**********************************************************************/
/** Get default value of single system parameter
 *
 * This gets the \b default value of the parameter \a parName. Parameter
 * name can be either the official parameter name or the alias name.
 *
 * This is currently not supported by all types of parameters, If
 * it is not supported, #SYSPARAM_INVAL_VALUE is returned
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param parVal		\OUT receives the parameter's value as a string
 * \param parValMaxLen	\IN maximum bytes that \a parVal can hold, including
 *							the terminating '\\0'
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_PARAM: parameter is not known
 *	 - #SYSPARAM_INVAL_VALUE: parValMaxLen is 0 and \a parVal is not NULL.
 *							  Or default value not available
 *	 - #SYSPARAM_VAL_TRUNCATED: only a part of value could be copied to
 *						   		\a parVal (and \a parVal was not NULL)
 *
 */
int SysParamGetDefault(
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_NV_STORAGE *nvs;
	int rv=SYSPARAM_INVAL_PARAM;
	SYSP_SUPER *super = SYSP_super;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;


	/* loop trough NVS objects until parameter is handled */
	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		rv = nvs->getParamDefault( nvs, parName, parVal, parValMaxLen );
		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM))
			break;
	}

	return rv;
}

/**********************************************************************/
/** Get all parameters with their values
 *
 * Build a string containing all known parameters and their values.
 *
 * Each parameter/value tuple is put into a separate line.
 *
 * If a tuple does not fit as a whole in \a buf, it is omitted.
 *
 * Example:
 * \verbatim
brd=EM04\n
kerpar=ip=auto rw\n
e=192.1.1.22:FFFFFF00\n
mmstartup=\n
\endverbatim
 *
 * \param buf		\OUT receives the all parameter/value pairs as a string
 * \param bufLen	\IN maximum bytes that \a buf can hold, including
 *							the terminating '\\0'
 * \param alias		\IN if TRUE, return the alias parameter names rather
 *						than the official names
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_VAL_TRUNCATED: only a part of pairs could be copied to
 *						   		\a buf
 *	 - #SYSPARAM_NO_RESOURCE: Could not allocate temp. buffer
 * \sa SysParamEnum
 */
int SysParamGetAll(
	char *buf,
	int bufLen,
	int alias )
{
	SYSP_NV_STORAGE *nvs;
	int rv=0, curBufLen=0;
	SYSP_SUPER *super = SYSP_super;
	char *parVal;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	if( bufLen > 0 )
		buf[0] = '\0';
	else
		return SYSPARAM_INVAL_VALUE;

	/* get a temp. buffer to copy parameter values */
	if( (parVal = SYSP_ALLOC( SYSP_GETALL_MAX_PARLEN, super ))
		== NULL )
		return SYSPARAM_NO_RESOURCE;

	/* loop trough NVS objects */
	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		int idx = 0, rv2;
		SYSPARAM_PARAM_INFO info;

		/* enumerate all parameters of NVS */
		while( nvs->enumParams( nvs, idx, alias, &info ) == 0 ){

			/* get parameter value */
			if( ((rv2 = nvs->getParam( nvs, info.parName, parVal,
									   SYSP_GETALL_MAX_PARLEN )) == 0) ||
				(rv2==SYSPARAM_INVAL_VALUE)){

				/* add it to buf if it fits */
				if( (curBufLen + SYSP_STRLEN( info.parName ) + 1 +
					 SYSP_STRLEN( parVal ) + 1 ) < (u_int32)(bufLen) ){

					rv2 = SYSP_SNPRINTF( buf, 512, "%s=%s\n", info.parName, parVal);
					curBufLen += rv2;
					buf += rv2;
				}
				else
					rv = SYSPARAM_VAL_TRUNCATED;
			}
			else if( rv2 == SYSPARAM_VAL_TRUNCATED ){
				rv = SYSPARAM_VAL_TRUNCATED;
			}
			idx++;
		}
	}
	SYSP_ALLOC_UNDO( SYSP_GETALL_MAX_PARLEN, super );
	return rv;
}

/**********************************************************************/
/** Set all non volatile parameters to defaults
 *
 * All parameter groups that are stored in non volatile storage are
 * set to their default values and are written into NV storage.
 *
 * Inventory/production parameter groups are not modified by this call.
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_EEPROM_ERROR: Error during NVS update
 *
 */
int SysParamSetDefaults( void )
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	int rv = 0, lastErr=0;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		/* set defaults in NVS */
		if( (rv = nvs->setDefaults( nvs, FALSE )) != 0 )
			lastErr = rv;
		/* and save it */
		if( (rv = nvs->save( nvs )) != 0 )
			lastErr = rv;
	}
	/* perform any postprocessing (e.g. update MENMON parameter string) */
	rv = SYSP_RunPostProcessHooks( super, NULL, 0, rv );
	if( lastErr == 0 && rv != 0)
		lastErr = rv;

	return lastErr;
}

/**********************************************************************/
/** Write temporary changes to NV storage
 *
 * Make temporary changes persistant. Temporary changes are those
 * made by calls to SysParamSetEx() with #SYSPARAM_SA_NON_PERSISTANT
 * flag set.
 *
 * This function can either write all parameter groups (if \a parName
 * is NULL) or a specific parameter group (in this case parName must
 * specify a parameter of that group).
 *
 * For example, to save the VxWorks bootline parameter group to NV storage,
 * call SysParamMakePersistant("s").
 *
 * \param parName		\IN parameter name including possible prefix.
 *
 * \return 0 on success, or sysparam error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_PARAM: \a parName is not NULL and is not known
 *	 - #SYSPARAM_EEPROM_ERROR: Error during NVS update
 *   - #SYSPARAM_READ_ONLY: Specified parameter cannot be made persistant
 *
 */
int SysParamMakePersistant( const char *parName )
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	int rv = 0, lastErr=0;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	if( parName == NULL ){
		/* save all groups */
		for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
			if( (rv = nvs->save( nvs )) != 0 )
				lastErr = rv;
		}
		rv = lastErr;
	}

	else {
		SYSP_PAR_GRP *parGrp;

		rv = SYSPARAM_INVAL_PARAM;

		/* find parameter group that handles this parameter */
		if( (parGrp = SYSP_ParNameToGroup( super, parName, NULL )) != NULL ){

			if( parGrp->save )
				rv = parGrp->save( parGrp );
			else
				rv = SYSPARAM_READ_ONLY;
		}
	}
	return rv;
}

/**********************************************************************/
/** Enumerate all parameters of Sysparam
 *
 * Writes info about parameter specified by \a idx to \a info.
 *
 * \param idx			\IN  the nth parameter to query
 * \param alias			\IN  if TRUE, return alias name
 * \param info			\OUT receives the parameter name
 *							 and the parameter's access attributes
 *
 * \return SYSPARAM error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 *	 - #SYSPARAM_INVAL_VALUE if \a idx out of range
 */
int SysParamEnum( int idx, int alias, SYSPARAM_PARAM_INFO *info)
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	int rv = SYSPARAM_INVAL_VALUE;
	int curIdx=0;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	/* loop trough NVS objects */
	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		int nParams;

		nParams = nvs->numParams( nvs );

		if( idx < curIdx + nParams){
			int nvsIdx = idx - curIdx;

			/* parameter handled by that NVS */
			nvs->enumParams( nvs, nvsIdx, alias, info );
			rv = 0;
			break;
		}
		curIdx += nParams;
	}

	return rv;
}

/**********************************************************************/
/** Get total number of parameters handled by Sysparam
 *
 * \param *numP			\OUT receives number of parameters
 *
 * \return SYSPARAM error code\n
 *   - #SYSPARAM_NOT_INIT: xxxSysParamInit() has not been called
 */
int SysParamNumParams( int *numP )
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	int nParams=0;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	for( nvs=super->nvsLst; nvs; nvs=nvs->next )
		nParams += nvs->numParams( nvs );

	*numP = nParams;
	return 0;
}



