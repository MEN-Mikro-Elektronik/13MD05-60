/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_mmpara.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2007/03/07 11:54:31 $
 *    $Revision: 1.9 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Parameter group handler for MENMON detected parameters
 *
 * This file has the following function groups:
 * - Handler for MMPARA parameter group
 * - Functions for tuple strings (such as Menmon parameter string)
 * - Function to rebuild MENMON parameter string
 *
 * \b MMPARA group:
 *
 * All parameters are read only for user. Firmware can update parameters
 * at any time.
 *
 * Parameters are non-persistant and cannot be set to defaults.
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_mmpara.c,v $
 * Revision 1.9  2007/03/07 11:54:31  cs
 * added casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.8  2005/06/23 15:54:28  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.7  2005/02/22 15:01:01  kp
 * avoid OS-9 compiler warnings
 *
 * Revision 1.6  2005/01/18 15:49:30  kp
 * added size to NVS sections
 *
 * Revision 1.5  2004/12/20 08:55:09  kp
 * place params into MMPARASTRING according to their attributes
 * (no longer put all parameters to MMPARASTRING)
 *
 * Revision 1.4  2004/11/29 10:52:26  kp
 * support for MM unittest
 *
 * Revision 1.3  2004/02/18 15:08:53  kp
 * cosmetic
 *
 * Revision 1.2  2004/01/27 10:10:59  kp
 * Parameter values may now contain nested ticks
 *
 * Revision 1.1  2004/01/08 09:06:54  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

/***************************************************************************/
/*! \page tus Definition of tuple strings

  Tuple strings are used e.g. in the MENMON parameter string to store
  multiple parameters with their value in a single ASCII string.

  The following does not apply to VxWorks bootline, which has a similar,
  but not identical format.

  Example for a tuple string:
  \code "par1=value1 par2='value with blanks' par4=yy"  \endcode

  - Parameter names may consist of ASCII chars [0x21..0x7e], except
    blanks and '=' chars.
  - Parameter names are case sensitive
  - A parameter name must not appear more than once in the tuple string
  - The equal sign '=' must immediately follow the parameter name
  - The parameter value starts after the '=' and ends with the next blank
    or EOS.
  - If the value contains blanks, value must be surrounded by ticks '\\''
  - Ticks can be nested, but only if each opening tick is preceeded by
    a '=' character, e.g:
    \code "par2='value with blanks subpar='nested value''"  \endcode

  - Parameter value may contain ASCII chars [0x20..0x7e]
  - Tuples are separated by any number of blanks

*/

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/** max. length of each parameter value in mmParaStr passed to
	SYSP_MmgrpCreate */
#define SYSP_MMPAR_CREATE_MAX_VAL_LEN 512


/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
typedef struct {
	SYSP_PAR_GRP parGrp;		/**< common parameter group object  */
	const SYSP_CONST_STRING *parList; /**< array of handled parameter names */

	int numParams;				/**< number of parameters handled  */
	int bufSize;				/**< size of \a buf  */
	char *buf;					/**< holds all handled parameters
								   as name=value pairs */
} SYSP_PGRP_MMPARA;

typedef struct {
    /** parameter name begin pointer into TUS (not null terminated). */
	char *parStr;
    /** parameter name end pointer+1 into TUS  */
	char *parStrEnd;
    /** value string begin pointer into TUS (not null terminated)
	 * Value string will NOT include surrounding ticks
	 */
	char *valStr;
    /** value string end pointer+1 into TUS  */
	char *valStrEnd;

	int hasTicks;				/**< true if value surrounnded by ticks  */

} SYSP_TUS_PARAM_INFO;

/**********************************************************************/
/** Get num parameters in mpgrp
 * \sa SYSP_PAR_GRP.numParams
 */
int SYSP_MmGrpNumParams( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_MMPARA *mmGrp = (SYSP_PGRP_MMPARA *)parGrp;
	return mmGrp->numParams;
}

/**********************************************************************/
/** Enum mmgrp params
 * \sa SYSP_PAR_GRP.enumParams
 */
int SYSP_MmgrpEnumParams(
	SYSP_PAR_GRP *parGrp,
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{
	SYSP_PGRP_MMPARA *mmGrp = (SYSP_PGRP_MMPARA *)parGrp;

	if( idx >= mmGrp->numParams )
		return SYSPARAM_INVAL_VALUE;

	SYSP_StrSafeCpy( info->parName, mmGrp->parList[idx],
					 sizeof(info->parName));
	info->attr = (SYSPARAM_ACC_ATTR)
		(SYSPARAM_ACC_RO | (SYSPARAM_ACC_PERSISTANT * 0) |
		SYSPARAM_ACC_MMPARASTR);
	return 0;
}

/**********************************************************************/
/** Set mmgrp parameter
 *
 * Is only allowed when a SYSPARAM_SA_FROM_FIRMWARE is specified
 *
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_MmgrpSetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_MMPARA *mmGrp = (SYSP_PGRP_MMPARA *)parGrp;
	int rv;

	/* check if parameter known */
	if( SYSP_StringArrFindStr( mmGrp->parList, parName ) == -1 )
		return SYSPARAM_INVAL_PARAM;

	/* only allowed when a SYSPARAM_SA_FROM_FIRMWARE is specified */
	if( !(attr & SYSPARAM_SA_FROM_FIRMWARE ))
		return SYSPARAM_READ_ONLY;

	rv = SYSP_TusParamSet( mmGrp->buf, mmGrp->bufSize, parName, parVal );

	return rv;
}


/**********************************************************************/
/** Get mmgrp parameter
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_MmgrpGetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_MMPARA *mmGrp = (SYSP_PGRP_MMPARA *)parGrp;
	int rv;

	/* check if parameter known */
	if( SYSP_StringArrFindStr( mmGrp->parList, parName ) == -1 )
		return SYSPARAM_INVAL_PARAM;

	rv = SYSP_TusParamGet( mmGrp->buf, mmGrp->bufSize, parName,
						   parVal, parValMaxLen );

	if( rv == SYSPARAM_INVAL_PARAM )
		rv = 0;					/* empty parameters are not stored */

	return rv;
}


/**********************************************************************/
/** Create a parameter group object for Menmon parameters
 *
 * Extracts all parameters present in \a parList from the passed \a mmParaStr
 *
 * On success, the parameter group is added to the list of \a nvs groups
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param name		\IN parameter group name
 * \param parList	\IN array of parameters in this section (NULL term.)
 *						array and pointers must be kept intact!
 * \param bufSize	\IN max. size of name=value buffer string (for all params)
 * \param mmParaStr	\IN the current full MENMON parameter string
 * \param mmParaStrMaxLen \IN max length that mmParaStr can hold (incl. '\\0')
 *
 * \return created mmGrp object (or NULL if failed)
 *
 */
int SYSP_MmgrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	const SYSP_CONST_STRING *parList,
	int bufSize,
	const char *mmParaStr,
	int mmParaStrMaxLen)
{
	SYSP_PGRP_MMPARA *mmGrp = NULL;
	int grpSize = sizeof(*mmGrp) + bufSize;
	int rv=0;
	char *parVal = NULL;

	/* alloc parameter group and raw data shadow */
	if( (mmGrp = SYSP_ALLOC( grpSize, nvs->super )) == NULL )
		return SYSPARAM_NO_RESOURCE;

	/* get a temp. buffer to copy parameter values */
	if( (parVal = SYSP_ALLOC( SYSP_MMPAR_CREATE_MAX_VAL_LEN, nvs->super ))
		== NULL ){
		rv = SYSPARAM_NO_RESOURCE;
		goto CLEANUP;
	}

	/* init object */
	mmGrp->parGrp.grpName		= name;
	mmGrp->parGrp.nvs 			= nvs;
	mmGrp->parGrp.numParams		= SYSP_MmGrpNumParams;
	mmGrp->parGrp.enumParams	= SYSP_MmgrpEnumParams;
	mmGrp->parGrp.setParam		= SYSP_MmgrpSetParam;
	mmGrp->parGrp.getParam		= SYSP_MmgrpGetParam;
	mmGrp->parList				= parList;
	mmGrp->numParams 			= SYSP_StringArrayLen( parList );
	mmGrp->bufSize				= bufSize;
	mmGrp->buf					= (char *)(mmGrp+1);

	/* extract all parameters from mmParaStr, add it to mmGrp->buf */
	{
		const SYSP_CONST_STRING *parName;

		for( parName=parList; *parName; parName++ ){
			rv = SYSP_TusParamGet( mmParaStr, mmParaStrMaxLen, *parName,
								   parVal, SYSP_MMPAR_CREATE_MAX_VAL_LEN );

			if( rv != 0 && rv != SYSPARAM_INVAL_PARAM )
				goto CLEANUP;

			if( (rv = SYSP_TusParamSet( mmGrp->buf, mmGrp->bufSize,
										*parName, parVal )) != 0 )
				goto CLEANUP;
		}

	}
	SYSP_ParGrpAdd( nvs, &mmGrp->parGrp );

 CLEANUP:
	SYSP_ALLOC_UNDO( SYSP_MMPAR_CREATE_MAX_VAL_LEN, nvs->super );
	if( rv != 0 )
		SYSP_ALLOC_UNDO( grpSize, nvs->super );
	return rv;
}


/**********************************************************************/
/** Helper function for SYSP_MmParaStringBuild
 */
static int MmParaStringBuildGroup(
	char *mmParaStr,
	int mmParaStrMaxLen,
	SYSP_PAR_GRP *parGrp,
	char *parVal )
{
	int idx=0, rv=0, rv2;
	SYSPARAM_PARAM_INFO info;
	char parName[SYSP_PREFIX_LEN + SYSP_PARNAME_LEN];

	while( parGrp->enumParams( parGrp, idx, FALSE, &info ) == 0 ){

		/* check if param wants to be in MM para string */
		if( info.attr & SYSPARAM_ACC_MMPARASTR ){
			/* get parameter value */
			if( ((rv2 = parGrp->getParam(
					  parGrp, info.parName, parVal,
					  SYSP_MMPAR_CREATE_MAX_VAL_LEN )) == 0) ||
				(rv2==SYSPARAM_INVAL_VALUE)){

				/* make full parameter name */
				SYSP_FullParName( parGrp->nvs, info.parName, parName );

				/* set parameter in mmParaStr */
				if( (rv2 = SYSP_TusParamSet( mmParaStr, mmParaStrMaxLen,
											 parName, parVal )) != 0 )
					rv = rv2;
			}
		}
		idx++;
	}
	return rv;
}

/**********************************************************************/
/** Build MENMON parameter string
 *
 * Builds a tuple string from all parameters in all groups of all NV
 * storage objects, whose attributes report that they want to be
 * included in MM parameter string, and builds a tuple string into \a
 * mmParaStr.
 *
 * The official parameter names with prefix will be written to MM para string
 *
 * If a parameter tuple doesn't fit as a whole into \a mmParaStr, the
 * tuple is omitted and the function returns SYSPARAM_NO_RESOURCE.
 *
 * \param mmParaStr		\OUT receives the created MM parameter string
 * \param mmParaStrMaxLen \IN max length of \a mmParaStr (incl. term. 0)
 * \param super			\IN   SYSPARAM super object
 *
 * \return 0 on success, or sysparam error code
 */
int SYSP_MmParaStringBuild(
	char *mmParaStr,
	int mmParaStrMaxLen,
	SYSP_SUPER *super)
{
	SYSP_NV_STORAGE *nvs;
	SYSP_PAR_GRP *parGrp;
	char *parVal;
	int rv = 0, rv2;

	/* init MM para string with magic */
	if( mmParaStrMaxLen < 5 )
		return SYSPARAM_NO_RESOURCE;

	SYSP_STRCPY( mmParaStr, "MPAR" );

	/* get a temp. buffer to copy parameter values */
	if( (parVal = SYSP_ALLOC( SYSP_MMPAR_CREATE_MAX_VAL_LEN, super ))
		== NULL )
		return SYSPARAM_NO_RESOURCE;

	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){

			/* add all parameters from group */
			if( (rv2 = MmParaStringBuildGroup( mmParaStr, mmParaStrMaxLen,
											   parGrp, parVal )) != 0 )
				rv = rv2;
		}
	}
	SYSP_ALLOC_UNDO( SYSP_MMPAR_CREATE_MAX_VAL_LEN, super );
	return rv;
}

/**********************************************************************/
/** Locate parameter \a parName in tuple string \a tus
 *
 * Fill \a info structure with information.
 *
 * \param tus			\IN tuple parameter string
 * \param tusMaxLen		\IN max length of string (incl. term. 0)
 * \param parName		\IN parameter name to search for
 * \param info			\OUT filled with info (invalid if return code is not 0)
 *
 * \return 0 if parameter found, or SYSPARAM_INVAL_PARAM
 */
int SYSP_TusParamInfo(
	const char *tus,
	int tusMaxLen,
	const char *parName,
	SYSP_TUS_PARAM_INFO *info )
{
	const char *s = tus;
	int parFound = FALSE;
	int tickNestCount=0;
	enum state {
		ST_IDLE, ST_IN_PARNAME, ST_IN_PARVAL, ST_END
	} state = ST_IDLE;

	if( tus == NULL || info == NULL || tusMaxLen == 0 )
		return SYSPARAM_INVAL_VALUE;

	while( (state != ST_END) && !(parFound && state==ST_IDLE)){
		int atEnd = (*s == '\0' || (s-tus) >= tusMaxLen);

		/*printf("st=%d s=%p *s=%c\n", state, s, *s );*/

		switch( state ){
		case ST_IDLE:
			if( *s != ' ' ){
				SYSP_MEMSET( info, 0, sizeof(*info) );
				state = ST_IN_PARNAME;
				info->parStr = (char *)s;
			}
			break;

		case ST_IN_PARNAME:
			if( atEnd || *s == '=' ){
				/* end of parameter name found */
				state = ST_IN_PARVAL;
				tickNestCount=0;

				/* is it the requested parameter name? */
				if( SYSP_STRNCMP( info->parStr, parName, s-info->parStr )==0 &&
					(SYSP_STRLEN( parName) == (u_int32)(s-info->parStr)) ){
					/* parameter name match */
					/*printf("parFound %d %d\n",
					  SYSP_STRLEN( parName), s-info->parStr );*/
					info->parStrEnd = (char *)s;
					parFound = TRUE;
				}
			}
			if( *s==' ' ){
				state = ST_IDLE;
				SYSP_MEMSET( info, 0, sizeof(*info) );
			}
			break;

		case ST_IN_PARVAL:
			if( *s == '\'' ){
				if( s[-1] == '=' ) /* opening tick */
					tickNestCount++;
				else
					tickNestCount--;
			}
			if( info->valStr == NULL ){
				if( *s == '\'' )
					info->hasTicks = TRUE;
				else
					info->valStr = (char *)s;
			}
			else {
				/* looking for end of parameter value */
				if( atEnd ||
					(info->hasTicks && tickNestCount <= 0) ||
					(!info->hasTicks && *s == ' ')){

					info->valStrEnd = (char *)s;
					state = ST_IDLE;
				}
			}
			break;
		default:
			break;
		}

		if( atEnd )
			state = ST_END;
		else
			s++;
	}
	return parFound ? 0 : SYSPARAM_INVAL_PARAM;
}

/**********************************************************************/
/** Get value of parameter \a parName from tuple string
 *
 * \param tus			\IN tuple parameter string
 * \param tusMaxLen		\IN max length of tuple string (incl. term. 0)
 * \param parName		\IN parameter name to search for
 * \param parVal		\OUT receives parameter value as a string
 * \param parValMaxLen 	\IN max length of parVal (including terminating 0)
 *
 * \return 0 if parameter found, or \n
 * 	- SYSPARAM_INVAL_PARAM 		- if parameter not found
 *  - SYSPARAM_INVAL_VALUE		- parVal not NULL and parValMaxLen==0
 *  - SYSPARAM_VAL_TRUNCATED 	- if parVal too small
 */
int SYSP_TusParamGet(
	const char *tus,
	int tusMaxLen,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	int rv, maxCpy;
	SYSP_TUS_PARAM_INFO info;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	rv = SYSP_TusParamInfo( tus, tusMaxLen, parName, &info );

	if( rv != 0 )
		return rv;

	if( parVal == NULL )
		return 0;

	maxCpy = info.valStrEnd - info.valStr + 1;
	if( maxCpy > parValMaxLen ){
		maxCpy = parValMaxLen;
		rv = SYSPARAM_VAL_TRUNCATED;
	}

	SYSP_StrSafeCpy( parVal, info.valStr, maxCpy );
	/*printf("SYSP_TusParamGet %s\n", parVal );*/
	return rv;
}


/**********************************************************************/
/** Delete parameter tuple from tuple string
 */
static void TusParamDel( char *tus, const SYSP_TUS_PARAM_INFO *info )
{
	char *end = info->valStrEnd;

	if( info->hasTicks ) end++;
	if( *end == ' ' ) end++;

	SYSP_MEMMOVE( info->parStr, end, SYSP_STRLEN(end)+1 );
}

/**********************************************************************/
/** Check if TUS parameter value is ok
 */
static int TusParamValidate( const char *parVal )
{
	while( *parVal ){
		if( !SYSP_PRINTABLE( *parVal ) )
			return SYSPARAM_INVAL_VALUE;
		parVal++;
	}
	return 0;
}

/**********************************************************************/
/** Set value of parameter \a parName in tuple string
 *
 * - If \a parName doesn't exist in tuple string, it is created
 * - If \a parVal is NULL or an empty string, \a parName is deleted from
 *	 tuple string
 *
 * Before the value is modified, it is checked wether it would fit
 * in the tuple string. If it would not fit, no modification is done
 * and #SYSPARAM_VAL_TRUNCATED is returned.
 *
 * \param tus			\IN tuple parameter string
 * \param tusMaxLen		\IN max length of tuple string (incl. term. 0)
 * \param parName		\IN parameter name to search for
 * \param parVal		\IN new parameter value (can be NULL or empty string)
 *
 * \return 0 if parameter found, or \n
 *  - SYSPARAM_NO_RESOURCE	- new value doesn't fit into \a tus
 *  - SYSPARAM_INVAL_VALUE  - invalid characters in \a parVal
 */
int SYSP_TusParamSet(
	char *tus,
	int tusMaxLen,
	const char *parName,
	const char *parVal)
{
	int orgParamBruttoLen=0, newParamBruttoLen;
	int newNeedTicks=FALSE, newTusLen, rv;
	SYSP_TUS_PARAM_INFO info;

	/* check if parName already exists in tus */
	if( SYSP_TusParamInfo( tus, tusMaxLen, parName, &info ) == 0 ){

		orgParamBruttoLen = info.valStrEnd - info.parStr +
			(info.hasTicks ? 1:0);
	}

	if( parVal==NULL || parVal[0] == '\0' ){
        /* delete parameter */
		if( orgParamBruttoLen )
			TusParamDel( tus, &info );
		return 0;
	}

	/* validate characters in parVal */
	if( (rv = TusParamValidate( parVal )) != 0 )
		return rv;

	/* check if new value must be surrounded by ticks */
	newNeedTicks = SYSP_ContainsBlanks( parVal );

	/* get brutto length of new tuple */
	newParamBruttoLen = SYSP_STRLEN(parName) + 1 +
		SYSP_STRLEN( parVal ) + (newNeedTicks ? 2 : 0);

	/* check if new value would fit into tuple string */
	newTusLen = SYSP_STRLEN( tus ) + newParamBruttoLen - orgParamBruttoLen;

	/* if the tuple was not present before, we need space for one blank */
	if( tus[0] != '\0' && !orgParamBruttoLen )
		newTusLen++;

	/*printf("orgParamBruttoLen=%d newParamBruttoLen=%d newTusLen=%d\n",
	  orgParamBruttoLen, newParamBruttoLen, newTusLen );*/

	if( newTusLen >= tusMaxLen )
		return SYSPARAM_NO_RESOURCE; /* will not fit into tus */

	/* if if the tuple was present before, delete it */
	if( orgParamBruttoLen )
		TusParamDel( tus, &info );

	/* and add it to the end of the tuple string */
	{
		int tusLen = SYSP_STRLEN( tus );
		char *s = tus + tusLen;

		if( tusLen > 0 && s[-1] != ' ' )
			*s++ = ' ';			/* tuple separator */

		SYSP_STRCPY( s, parName );
		s += SYSP_STRLEN( s );

		*s++ = '=';

		if( newNeedTicks )
			*s++ = '\'';

		SYSP_STRCPY( s, parVal );
		s += SYSP_STRLEN( s );

		if( newNeedTicks )
			*s++ = '\'';
		*s = '\0';
	}

	return 0;
}


/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST

/** check if all parameters listed */
static int UtEnumParams( SYSP_PGRP_MMPARA *mmGrp )
{
	SYSPARAM_PARAM_INFO info;
	SYSP_PAR_GRP *parGrp = &mmGrp->parGrp;

	/* official names */
	UTASSERT( parGrp->enumParams( parGrp, 0, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "cpu", 10 ) == 0 );
	UTASSERT( info.attr == (SYSPARAM_ACC_RO | SYSPARAM_ACC_MMPARASTR));

	UTASSERT( parGrp->enumParams( parGrp, 1, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "pciclkhz", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 2, FALSE, &info ) == 0 );
	UTASSERT( SYSP_STRNCMP( info.parName, "clun", 10 ) == 0 );

	UTASSERT( parGrp->enumParams( parGrp, 3, FALSE, &info ) ==
			  SYSPARAM_INVAL_VALUE );

	return 0;
 ABORT:
	return -1;
}

SYSP_UNITTEST( "SYSPARAM2: MmParaStringGroup", SYSP_UtMmGrp );

/** Checks MmGrp functions */
int SYSP_UtMmGrp( void )
{
	SYSP_PGRP_MMPARA *mmGrp;
	SYSP_PAR_GRP *parGrp;
	SYSP_CONST_STRING parList[] = {
		"cpu", "pciclkhz", "clun", NULL
	};
	char mmParaStr[200] = "MPAR pciclkhz=33000000 cpu=MPC8245 xxx=yyy";
	SYSP_NVS_ADR nvsAdr;
	SYSP_NV_STORAGE *nvs;

	SYSP_UtSuperInit();

	/* create dummy nvs */
	SYSP_MEMSET( &nvsAdr, 0, sizeof(nvsAdr));
	UTASSERT( nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE),
									"nvs", SYSP_super, "", nvsAdr, 200 ));

	UTASSERT( (SYSP_MmgrpCreate( nvs, "mmpara", parList,
								 200, mmParaStr, sizeof( mmParaStr ))) == 0  );
	mmGrp = (SYSP_PGRP_MMPARA *)nvs->parGrpLst;
	parGrp = &mmGrp->parGrp;

	UTASSERT( UtEnumParams( mmGrp ) == 0 );

	{
		u_int8 buf[12];

		/* get param */
		UTASSERT( parGrp->getParam( parGrp, "cpu", buf, sizeof(buf)) == 0 );
		UTASSERT( SYSP_STRNCMP( buf, "MPC8245", 12 ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "pciclkhz", buf, sizeof(buf)) ==0);
		UTASSERT( SYSP_STRNCMP( buf, "33000000", 12 ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "clun", buf, sizeof(buf)) ==0);
		UTASSERT( SYSP_STRNCMP( buf, "", 12 ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "xxx", buf, sizeof(buf)) ==
			SYSPARAM_INVAL_PARAM);
		UTASSERT( SYSP_STRNCMP( buf, "", 12 ) == 0 );

		/* add new param */
		UTASSERT( parGrp->setParam( parGrp, "clun", "2", NULL,
									SYSPARAM_SA_FROM_FIRMWARE ) == 0 );
		UTASSERT( parGrp->getParam( parGrp, "clun", buf, sizeof(buf)) ==0);
		UTASSERT( SYSP_STRNCMP( buf, "2", 12 ) == 0 );

		UTASSERT( parGrp->setParam( parGrp, "clun", "2", NULL,
									0 ) == SYSPARAM_READ_ONLY );
	}

	DBGWRT_1((DBH,"SYSP_UtMmGrp OK\n"));
	return 0;
 ABORT:
	return -1;
}

SYSP_UNITTEST( "SYSPARAM2: Tuple string", SYSP_UtTus );

/** Checks Tuple string functions */
int SYSP_UtTus( void )
{
	/* SYSP_TusParamInfo */
	{
		SYSP_TUS_PARAM_INFO info;

		{
			/* normal param */
			char *tus = "MPAR par0=a par1=xxx";
			UTASSERT( SYSP_TusParamInfo( tus, 20, "par1", &info ) == 0 );
			UTASSERT( info.parStr 		== tus+12 );
			UTASSERT( info.parStrEnd 	== tus+16 );
			UTASSERT( info.valStr 		== tus+17 );
			UTASSERT( info.valStrEnd	== tus+20 );
			UTASSERT( info.hasTicks	== FALSE );
		}

		{
			/* value with ticks */
			char *tus = "par1='xxx 1'";
			UTASSERT( SYSP_TusParamInfo( tus, 12, "par1", &info ) == 0 );
			UTASSERT( info.parStr 		== tus+0 );
			UTASSERT( info.parStrEnd 	== tus+4 );
			UTASSERT( info.valStr 		== tus+6 );
			UTASSERT( info.valStrEnd	== tus+11 );
			UTASSERT( info.hasTicks	== TRUE );
		}

		{
			/* value with netsted ticks */
			char *tus = "par1='xxx 1 nest='nested value'' nest=xyz";
			UTASSERT( SYSP_TusParamInfo( tus, 42, "par1", &info ) == 0 );
			UTASSERT( info.parStr 		== tus+0 );
			UTASSERT( info.parStrEnd 	== tus+4 );
			UTASSERT( info.valStr 		== tus+6 );
			UTASSERT( info.valStrEnd	== tus+31 );
			UTASSERT( info.hasTicks	== TRUE );

			UTASSERT( SYSP_TusParamInfo( tus, 50, "nest", &info ) == 0 );
			UTASSERT( info.parStr 		== tus+33 );
			UTASSERT( info.parStrEnd 	== tus+37 );
			UTASSERT( info.valStr 		== tus+38 );
			UTASSERT( info.valStrEnd	== tus+41 );
			UTASSERT( info.hasTicks	== FALSE );
		}

		{
			/* param not found */
			char *tus = "par='xxx 1'";
			UTASSERT( SYSP_TusParamInfo( tus, 12, "par1", &info ) ==
				SYSPARAM_INVAL_PARAM);
		}

		{
			/* param not found (2) */
			char *tus = "par1='xxx 1'";
			UTASSERT( SYSP_TusParamInfo( tus, 12, "par", &info ) ==
				SYSPARAM_INVAL_PARAM);
		}

		{
			/* unterminated value */
			char *tus = "par1='xxx 1";
			UTASSERT( SYSP_TusParamInfo( tus, 12, "par1", &info ) == 0 );
			UTASSERT( info.parStr 		== tus+0 );
			UTASSERT( info.parStrEnd 	== tus+4 );
			UTASSERT( info.valStr 		== tus+6 );
			UTASSERT( info.valStrEnd	== tus+11 );
			UTASSERT( info.hasTicks	== TRUE );
		}

	}

	/* SYSP_TusParamGet */
	{

		{
			/* normal param */
			char *tus = "MPAR par0='a x' par1=xxx";
			char buf[4];

			UTASSERT( SYSP_TusParamGet( tus, 30, "par1", buf, sizeof(buf) )
					  == 0 );
			UTASSERT( SYSP_STRNCMP( "xxx", buf, 4 ) == 0 );

			UTASSERT( SYSP_TusParamGet( tus, 30, "par0", buf, sizeof(buf) )
					  == 0 );
			UTASSERT( SYSP_STRNCMP( "a x", buf, 4 ) == 0 );

			/* buf too small */
			UTASSERT( SYSP_TusParamGet( tus, 30, "par1", buf, 3 )
					  == SYSPARAM_VAL_TRUNCATED );
			UTASSERT( SYSP_STRNCMP( "xx", buf, 3 ) == 0 );

			/* test for param existance */
			UTASSERT( SYSP_TusParamGet( tus, 30, "par1", NULL, 0 )
					  == 0 );
		}

	}

	/* SYSP_TusParamSet */
	{
		{
			/* create new tuple, starting with empty tus */
			char tus[12] = "";

			UTASSERT( SYSP_TusParamSet( tus, 9, "par1", "abc")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par1=abc", 9) == 0 );

			/* same with blanks */
			UTASSERT( SYSP_TusParamSet( tus, 11, "par1", "a c")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par1='a c'", 11) == 0 );

		}
		{
			/* create new tuple, starting with non-empty tus */
			char tus[40] = "par0=xyz par2='a x'";

			UTASSERT( SYSP_TusParamSet( tus, 29, "par1", "abc")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par0=xyz par2='a x' par1=abc", 29)
					  == 0 );
			/* invalid chars in parVal */
			/*UTASSERT( SYSP_TusParamSet( tus, 29, "par1", "a'bc")
			  == SYSPARAM_INVAL_VALUE );*/
			UTASSERT( SYSP_TusParamSet( tus, 29, "par1", "a\x7f")
					  == SYSPARAM_INVAL_VALUE );
		}
		{
			/* modify tuple at begin */
			char tus[40] = "par1=xy par2='a x'"; /* len=18 (without \0) */

			UTASSERT( SYSP_TusParamSet( tus, 20, "par1", "abc")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=abc", 40)
					  == 0 );
			/* modify tuple at end (larger size) */
			UTASSERT( SYSP_TusParamSet( tus, 21, "par1", "defg")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=defg", 40)
					  == 0 );
			/* modify tuple at end (smaller size) */
			UTASSERT( SYSP_TusParamSet( tus, 21, "par1", "yy")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=yy", 40)
					  == 0 );
			/* add a new tuple */
			UTASSERT( SYSP_TusParamSet( tus, 40, "par3", "p3")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=yy par3=p3", 40)
					  == 0 );

			/* modify tuple in the mid (doesn't fit into buffer) */
			UTASSERT( SYSP_TusParamSet( tus, 27, "par1", "hal")
					  == SYSPARAM_NO_RESOURCE );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=yy par3=p3", 40)
					  == 0 );
			/* modify tuple in the mid (fit into buffer) */
			UTASSERT( SYSP_TusParamSet( tus, 28, "par1", "hal")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par3=p3 par1=hal", 40)
					  == 0 );

			/* delete only */
			UTASSERT( SYSP_TusParamSet( tus, 40, "par3", NULL)
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par2='a x' par1=hal", 40)
					  == 0 );
			UTASSERT( SYSP_TusParamSet( tus, 40, "par2", "")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "par1=hal", 40)
					  == 0 );
			UTASSERT( SYSP_TusParamSet( tus, 40, "par1", "")
					  == 0 );
			UTASSERT( SYSP_STRNCMP( tus, "", 40)
					  == 0 );
		}
	}


	DBGWRT_1((DBH,"SYSP_UtTus OK\n"));
	return 0;
 ABORT:
	return -1;
}

SYSP_UNITTEST( "SYSPARAM2: MmParaStringBuild", SYSP_UtMmParaStringBuild );

/** Checks SYSP_MmParaStringBuild function */
int SYSP_UtMmParaStringBuild( void )
{
	SYSP_NVS_ADR nvsAdr;
	SYSP_NV_STORAGE *nvs;
	SYSP_CONST_STRING parList[] = {
		"cpu", "pciclkhz", "clun", NULL
	};
	char mmParaStrIni[200] = "MPAR pciclkhz=33000000 cpu=MPC8245 xxx=yyy";
	char mmParaStr[100];
	SYSP_PAR_GRP *parGrp;

	SYSP_UtSuperInit();

	/* create dummy nvs */
	SYSP_MEMSET( &nvsAdr, 0, sizeof(nvsAdr));
	UTASSERT( nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE),
									"nvs", SYSP_super, "", nvsAdr, 200 ));

	SYSP_NvsAdd( SYSP_super, nvs );

	UTASSERT( (SYSP_MmgrpCreate( nvs, "mmpara", parList,
								 200, mmParaStrIni,
								 sizeof( mmParaStrIni ))) == 0  );

	/* normal */
	UTASSERT( SYSP_MmParaStringBuild( mmParaStr, sizeof(mmParaStr),
									  SYSP_super ) == 0 );
	UTASSERT( SYSP_STRCMP( mmParaStr,
						   "MPAR cpu=MPC8245 pciclkhz=33000000" ) == 0 );

	/* set a very long parameter that doesn't fit and use NVS prefix*/
	parGrp = nvs->parGrpLst;

	SYSP_MEMSET( mmParaStr, 0x41, 100 );
	mmParaStr[99] = '\0';

	SYSP_STRCPY( nvs->prefix, "c");
	UTASSERT( parGrp->setParam( parGrp, "clun", mmParaStr, NULL,
								SYSPARAM_SA_FROM_FIRMWARE ) == 0 );

	UTASSERT( SYSP_MmParaStringBuild( mmParaStr, sizeof(mmParaStr),
									  SYSP_super ) == SYSPARAM_NO_RESOURCE );
	UTASSERT( SYSP_STRCMP( mmParaStr,
						   "MPAR c-cpu=MPC8245 c-pciclkhz=33000000" ) == 0 );




	DBGWRT_1((DBH,"SYSP_UtMmParaStringBuild OK\n"));
	return 0;
 ABORT:
	return -1;
}


#endif /* SYSP_CONFIG_UNITTEST */
