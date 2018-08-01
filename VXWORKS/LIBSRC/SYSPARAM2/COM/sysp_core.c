/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_core.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/06/07 16:18:54 $
 *    $Revision: 1.18 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Core functions
 *
 *    \switches  SYSP_BOARD_SPEZIFIC_BAUD_CHECK(_x_)  returns 1 for wrong baudrates or 0
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_core.c,v $
 * Revision 1.18  2010/06/07 16:18:54  sy
 * R:1 No function for conversion between u_int16 formatted string and decimal
 * M:1 Add SYSP_FromRawU16D and Add SYSP_ToRawU16D
 *
 * Revision 1.17  2010/02/01 17:23:32  rt
 * R: 1) Incorrect values could be stored to some of the system
 *       parameters (e.g. tdp, hdp...).
 * M: 1) Added SYSP_ValidateSD() for signed decimal parameters.
 *
 * Revision 1.16  2007/11/21 17:22:55  ufranke
 * added
 *  - error message for MENMON if alloc fails
 *
 * Revision 1.15  2007/07/11 14:49:35  rt
 * added:
 * - SYSP_FromRawComMode, SYSP_ToRawComMode, SYSP_ValidateComMode
 * removed:
 * - _atoi
 *
 * Revision 1.14  2007/06/28 15:00:15  rt
 * added:
 * - SYSP_ToRawStrUpper
 * - SYSP_ValidateBaud (baudrate check moved to this function)
 *
 * Revision 1.13  2007/04/13 14:31:26  ufranke
 * added
 *  + SYSP_BOARD_SPEZIFIC_BAUD_CHECK for special baudrates
 *    if the common check fails the baudrate can be checked
 *    by a port specific macro or function
 *
 * Revision 1.12  2007/03/22 14:36:03  ufranke
 * added
 *  - check for EE-BAUD
 *    validate new baudrates
 *
 * Revision 1.11  2007/03/07 11:54:17  cs
 * added casts to avoid signed/unsigned compare warnings
 *
 * Revision 1.10  2005/06/23 15:54:09  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.9  2005/02/22 15:00:52  kp
 * cosmetics
 *
 * Revision 1.8  2005/02/02 16:03:06  kp
 * + SYSP_AllocInfo()
 *
 * Revision 1.7  2005/01/18 15:49:22  kp
 * check size of NVS sections when reading/writing
 *
 * Revision 1.6  2005/01/12 14:19:51  kp
 * + SYSP_PostProcessingHookInstall()
 * + conversion funcs for NSPEED/MCHOICE
 *
 * Revision 1.5  2004/12/20 08:51:42  kp
 * + boolean parameters
 *
 * Revision 1.4  2004/11/29 10:51:47  kp
 * added SysparamGetDefault support
 *
 * Revision 1.3  2004/10/07 09:17:38  kp
 * support ethernet MAC address conversion functions
 *
 * Revision 1.2  2004/09/03 15:20:44  kp
 * added EEPROM parameter handling for 16 bit signed values
 *
 * Revision 1.1  2004/01/08 09:06:47  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"

/** the only global variable. Pointer to super object */
SYSP_SUPER *SYSP_super;

#ifndef SYSP_BOARD_SPEZIFIC_BAUD_CHECK
	#define SYSP_BOARD_SPEZIFIC_BAUD_CHECK(_x_)  1 /* returns 1 for wrong baudrates or 0 */
#else
	extern int SYSP_BOARD_SPEZIFIC_BAUD_CHECK( int );
#endif

/*---------------------------------------------------------------------+
|             SIMPLE MEMORY ALLOCATOR                                  |
+---------------------------------------------------------------------*/

/**********************************************************************/
/** Allocate block from heap specified with \a alloc
 *
 * Tries to allocate \a size bytes from the heap maintained with
 * \a alloc. On Success, returns a pointer to the first byte of
 * the block. The data block is filled with zeroes.
 *
 * If not enough mem available, NULL is returned.
 *
 * \warning This is not a full malloc() implementation. A memory
 * block can only be free'ed when no other allocs have been made in
 * the meantime; i.e. it works like a stack.
 *
 * \sa SYSP_AllocUndo, SYSP_ALLOC, SYSP_ALLOC_UNDO, SYSP_ALLOC_INIT
 */
void *SYSP_Alloc( SYSP_ALLOC *alloc, int size )
{
	void *rv = NULL;

	if( size <= alloc->size ){
		rv = (void *)alloc->mem;
		alloc->size -= size;
		alloc->mem	+= size;
		SYSP_MEMSET( rv, 0, size );
		DBGWRT_3((DBH,"SYSP_Alloc now %d (req=%d)\n", alloc->size, size ));
		if( alloc->size < alloc->minavail )
			alloc->minavail = alloc->size;
	}

#ifdef MENMON
	if( rv == NULL )
		MMCONS_Print("*** %s mem\n", __FUNCTION__ );
#endif
	return rv;
}

/**********************************************************************/
/** Undo previous alloc
 * \sa SYSP_Alloc, SYSP_ALLOC_UNDO
 */
void SYSP_AllocUndo( SYSP_ALLOC *alloc, int size )
{
	alloc->size += size;
	alloc->mem -= size;
}

/**********************************************************************/
/** Get allocator info
 *
 * \param availP 	\OUT receices currently available mem
 * \param minAvailP \OUT receices minimum available mem
 *
 */
void SYSP_AllocInfo( int *availP, int *minAvailP )
{
	if( SYSP_super ){
		*availP 	= SYSP_super->alloc.size;
		*minAvailP 	= SYSP_super->alloc.minavail;
	}
	else {
		*availP = *minAvailP = -1;
	}
}


/*---------------------------------------------------------------------+
|             COMMON ROUTINES FOR MENMON/SMB IMPLEMENTATIONS           |
+---------------------------------------------------------------------*/

/**********************************************************************/
/** Configure one additional postprocessing function
 *
 * This function is called from SysParamSet whenever a parameter's
 * value is changed (regardless if the change was marked persistent)
 *
 * Overwrites any hook previously installed
 */
int SYSP_PostProcessingHookInstall( SYSP_POST_PROCESS_CB cb )
{
	SYSP_SUPER *super = SYSP_super;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	super->postProcessHook = cb;
	return 0;
}

/**********************************************************************/
/** Run all postprocessing functions
 *
 */
int SYSP_RunPostProcessHooks(
	SYSP_SUPER *super,
	const char *parName,
	int attr,
	int errCode)
{
	int rv=0, rv2;

	/* perform any postprocessing (e.g. update MENMON parameter string) */
	if( super->postProcessSetParam ){
		rv2 = super->postProcessSetParam( super, parName, attr );
		if( rv == 0 && rv2 != 0)
			rv = rv2;
	}
	/* perform user hook */
	if( super->postProcessHook ){
		rv2 = super->postProcessHook( parName, attr, errCode );
		if( rv == 0 && rv2 != 0)
			rv = rv2;
	}
	return rv;
}

/**********************************************************************/
/** Super object method to read from EEPROM
 *
 * This implementation supports SMB EEPROMs only and uses the function
 * callback passed through the initBlk.
 *
 * \sa SYSP_SUPER.readNvs
 */
int SYSP_SuperMmsmbRead(
	SYSP_SUPER *super,
	SYSP_NVS_ADR nvsAdr,
	int offset,
	int size,
	u_int8 *data)
{
	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	if( nvsAdr.type != SYSP_NVSADR_SMB )
		return SYSPARAM_EEPROM_ERROR;

	if( xSuper->initBlk.eeRead( xSuper->initBlk.osh,
								nvsAdr.addr.smb.bus,
								nvsAdr.addr.smb.dev,
								offset,
								data,
								size ))
		return SYSPARAM_EEPROM_ERROR;

	return 0;
}

/**********************************************************************/
/** Super object method to write EEPROM
 *
 * This implementation supports SMB EEPROMs only and uses the function
 * callback passed through the initBlk.
 *
 * \sa SYSP_SUPER.writeNvs
 */
int SYSP_SuperMmsmbWrite(
	SYSP_SUPER *super,
	SYSP_NVS_ADR nvsAdr,
	int offset,
	int size,
	const u_int8 *data)
{
	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	if( nvsAdr.type != SYSP_NVSADR_SMB )
		return SYSPARAM_EEPROM_ERROR;

	if( xSuper->initBlk.eeWrite( xSuper->initBlk.osh,
								nvsAdr.addr.smb.bus,
								nvsAdr.addr.smb.dev,
								offset,
								(u_int8 *)data,
								size ))
		return SYSPARAM_EEPROM_ERROR;

	return 0;
}

/**********************************************************************/
/** SetParam postprocessing routine for MENMON/SMB
 *
 * Updates the MENMON parameter string in memory
 */
int SYSP_SuperMmsmbPostProcessSetParam(
	SYSP_SUPER *super,
	const char *parName,
	int attr)
{
	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;
	return SYSP_MmParaStringBuild( xSuper->initBlk.mmParaStringAdrs,
								   xSuper->initBlk.mmParaStringMaxLen,
								   super );
}

/**********************************************************************/
/** Initialize extended super object for MENMON/SMB
 *
 * Caller must initialize the following xSuper elements:
 * - s.alloc
 * - s.reInit
 * - initBlk
 */
void SYSP_SuperMmsmbInit( SYSP_MMSMB_SUPER *xSuper )
{
	SYSP_SUPER *super = &xSuper->s;

	super->readNvs 				= SYSP_SuperMmsmbRead;
	super->writeNvs 			= SYSP_SuperMmsmbWrite;
	super->postProcessSetParam 	= SYSP_SuperMmsmbPostProcessSetParam;

	super->nvsLst = NULL;
	super->initialized = TRUE;
}


/*---------------------------------------------------------------------+
|             NVS OBJECT METHODS                                       |
+---------------------------------------------------------------------*/


/**********************************************************************/
/** Common NVS read from EEPROM routine
 * \sa SYSP_NV_STORAGE.readNvs
 */
int SYSP_NvsCommonReadNvs(
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	u_int8 *data)
{
	if( (offset < 0) || (size < 0) || ((u_int32)(size+offset) > nvs->nvsSize ))
		return SYSPARAM_INVAL_VALUE;

	return nvs->super->readNvs(
		nvs->super, nvs->nvsAdr, offset, size, data );
}

/**********************************************************************/
/** Common NVS write to EEPROM routine
 * \sa SYSP_NV_STORAGE.writeNvs
 */
int SYSP_NvsCommonWriteNvs(
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	const u_int8 *data)
{
	if( (offset < 0) || (size < 0) || ((u_int32)(size+offset) > nvs->nvsSize ))
		return SYSPARAM_INVAL_VALUE;

	return nvs->super->writeNvs(
		nvs->super, nvs->nvsAdr, offset, size, data );
}

/**********************************************************************/
/** Check if \a parName matches the prefix of \a nvs object
 * \return NULL if prefix doesn't match or pointer to parName without
 *         prefix
 */
static const char *NvsPrefixMatch( SYSP_NV_STORAGE *nvs, const char *parName )
{
	char prefix[SYSP_PREFIX_LEN];
	const char *pureName;

	pureName = SYSP_Prefix( parName, prefix );
	if( SYSP_STRCMP( nvs->prefix, prefix ) == 0 )
		return pureName;
	return NULL;
}

/**********************************************************************/
/** Get number of parameters of NVS
 * \sa SYSP_NV_STORAGE.numParams
 */
int SYSP_NvsCommonNumParams( SYSP_NV_STORAGE *nvs )
{
	SYSP_PAR_GRP *parGrp;
	int nParams=0;

	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next )
		nParams += parGrp->numParams( parGrp );

	return nParams;
}

/**********************************************************************/
/** Enum NVS params
 * \sa SYSP_NV_STORAGE.enumParams
 */
int SYSP_NvsCommonEnumParams(
	SYSP_NV_STORAGE *nvs,
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{
	SYSP_PAR_GRP *parGrp;
	int rv = SYSPARAM_INVAL_VALUE;
	int curIdx=0;
	SYSPARAM_PARAM_INFO info2;

	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){
		int nParams;

		nParams = parGrp->numParams( parGrp );

		if( idx < curIdx + nParams){
			int grpIdx = idx - curIdx;

			/* parameter handled by that group */
			parGrp->enumParams( parGrp, grpIdx, alias, &info2 );
			/* add prefix to parameter name */
			SYSP_FullParName( nvs, info2.parName, info->parName );
			info->attr = info2.attr;
			rv = 0;
			break;
		}
		curIdx += nParams;
	}

	return rv;
}


/**********************************************************************/
/** Set NVS parameter default function
 *  Check for parName baud if baudrate is valid
 * \sa SYSP_NV_STORAGE.setParam
 */
int SYSP_NvsCommonSetParam(
	SYSP_NV_STORAGE *nvs,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PAR_GRP *parGrp;
	int rv = SYSPARAM_INVAL_PARAM;
	const char *pureName;

	if( (pureName = NvsPrefixMatch( nvs, parName )) == NULL )
		goto CLEANUP;	/* prefix doesn't match */

	/* try to set parameter it on all groups */
	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next )
	{
		rv = parGrp->setParam( parGrp, pureName, parVal, passwd, attr );
		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM))
			break;
	}

CLEANUP:
	return rv;
}



/**********************************************************************/
/** Get NVS parameter
 * \sa SYSP_NV_STORAGE.getParam
 */
int SYSP_NvsCommonGetParam(
	SYSP_NV_STORAGE *nvs,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PAR_GRP *parGrp;
	int rv = SYSPARAM_INVAL_PARAM;
	const char *pureName;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( (pureName = NvsPrefixMatch( nvs, parName )) == NULL )
		return rv;				/* prefix doesn't match */

	/* simply try to get it on all groups */
	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){
		rv = parGrp->getParam( parGrp, pureName, parVal, parValMaxLen );
		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM))
			break;
	}
	return rv;
}

/**********************************************************************/
/** Get NVS default parameter
 * \sa SYSP_NV_STORAGE.getParam
 */
int SYSP_NvsCommonGetParamDefault(
	SYSP_NV_STORAGE *nvs,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PAR_GRP *parGrp;
	int rv = SYSPARAM_INVAL_PARAM;
	const char *pureName;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( (pureName = NvsPrefixMatch( nvs, parName )) == NULL )
		return rv;				/* prefix doesn't match */

	/* simply try to get it on all groups */
	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){
		if( parGrp->getParamDefault )
			rv = parGrp->getParamDefault( parGrp, pureName, parVal,
										  parValMaxLen );
		else
			rv = SYSPARAM_INVAL_PARAM; /* no default available */

		if( (rv == 0) || (rv != SYSPARAM_INVAL_PARAM))
			break;
	}
	return rv;
}

/**********************************************************************/
/** Set NVS defaults
 * \sa SYSP_NV_STORAGE.setDefaults
 */
int SYSP_NvsCommonSetDefaults(
	SYSP_NV_STORAGE *nvs,
	int force)
{
	SYSP_PAR_GRP *parGrp;
	int rv = 0, lastErr=0;

   	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){
		if( parGrp->setDefaults &&
			((rv = parGrp->setDefaults( parGrp, force )) != 0) )
			lastErr = rv;
	}
	return lastErr;
}

/**********************************************************************/
/** Save NVS
 * \sa SYSP_NV_STORAGE.save
 */
int SYSP_NvsCommonSave(
	SYSP_NV_STORAGE *nvs)
{
	SYSP_PAR_GRP *parGrp;
	int rv = 0, lastErr=0;

	for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){
		if( parGrp->save &&
			((rv = parGrp->save( parGrp )) != 0) )
			lastErr = rv;
	}
	return lastErr;
}


/**********************************************************************/
/** Create a SYSP_NV_STORAGE object
 *
 * \param objSize		\IN size of object (>= SYSP_NV_STORAGE structure)
 * \param nvsName		\IN name of NVS (must remain intact!)
 * \param super			\IN sysparam super object
 * \param prefix		\IN NVS parameter name prefix (up to 3 chars),
 *							caller can destroy pointer later
 * \param nvsAdr		\IN NVS address
 * \param nvsSize		\IN size of NVS section in bytes
 *
 * \return allocated and initialized SYSP_NV_STORAGE object
 *		   or NULL if alloc failed.
 *
 * all method pointers are set to their default functions
 */
SYSP_NV_STORAGE *SYSP_NvsCreate(
	int objSize,
	const char *nvsName,
	SYSP_SUPER *super,
	const char *prefix,
	SYSP_NVS_ADR nvsAdr,
	u_int32 nvsSize)
{
	SYSP_NV_STORAGE *nvs;

	if( (u_int32)objSize < sizeof(SYSP_NV_STORAGE) )
		return NULL;

	if( (nvs = SYSP_ALLOC( objSize, super )) == NULL )
		return NULL;

	nvs->nvsName	= nvsName;
	nvs->super 		= super;
	SYSP_STRNCPY( nvs->prefix, prefix, SYSP_PREFIX_LEN - 1 );
	nvs->nvsAdr 	= nvsAdr;
	nvs->nvsSize	= nvsSize;

	nvs->readNvs 	= SYSP_NvsCommonReadNvs;
	nvs->writeNvs 	= SYSP_NvsCommonWriteNvs;
	nvs->numParams	= SYSP_NvsCommonNumParams;
	nvs->enumParams	= SYSP_NvsCommonEnumParams;
	nvs->setParam	= SYSP_NvsCommonSetParam;
	nvs->getParam	= SYSP_NvsCommonGetParam;
	nvs->getParamDefault = SYSP_NvsCommonGetParamDefault;
	nvs->setDefaults= SYSP_NvsCommonSetDefaults;
	nvs->save		= SYSP_NvsCommonSave;

	return nvs;
}

/**********************************************************************/
/** Add a NV storage object to super list of NVS object
 *
 */
void SYSP_NvsAdd( SYSP_SUPER *super, SYSP_NV_STORAGE *nvs )
{
	SYSP_NV_STORAGE *n;

	/* find last object */
	if( super->nvsLst ){
		for( n=super->nvsLst; n->next; n=n->next )
			;
		n->next = nvs;
	}
	else {
		super->nvsLst = nvs;
	}
	nvs->next = NULL;
}

/**********************************************************************/
/** Find NV storage object by name
 *
 */
SYSP_NV_STORAGE *SYSP_NvsFind( SYSP_SUPER *super, const char *nvsName)
{
	SYSP_NV_STORAGE *n;

	for( n=super->nvsLst; n; n=n->next )
		if( SYSP_STRCMP( nvsName, n->nvsName ) == 0 )
			return n;
	return NULL;
}

/**********************************************************************/
/** Add a parameter group object to NVS list of parameter groups
 *
 */
void SYSP_ParGrpAdd( SYSP_NV_STORAGE *nvs, SYSP_PAR_GRP *parGrp )
{
	SYSP_PAR_GRP *n;

	/* find last object */
	if( nvs->parGrpLst ){
		for( n=nvs->parGrpLst; n->next; n=n->next )
			;
		n->next = parGrp;
	}
	else {
		nvs->parGrpLst = parGrp;
	}
	parGrp->next = NULL;
}

/*---------------------------------------------------------------------+
|             MISC COMMON FUNCTIONS                                    |
+---------------------------------------------------------------------*/
/**********************************************************************/
/** Find parameter group that handles \a parName
 *
 * \param super			\IN SYSPARAM super object
 * \param parName		\IN parameter name to (including NVS prefix),
 * \param *pureNameP	\OUT if not NULL, receives a pointer to the pure
 *							 parameter name without prefix
 * \return parameter group or NULL if not found
 */
SYSP_PAR_GRP *SYSP_ParNameToGroup(
	SYSP_SUPER *super,
	const char *parName,
	const char **pureNameP)
{
	SYSP_NV_STORAGE *nvs;
	SYSP_PAR_GRP *parGrp;
	const char *pureName;

	for( nvs=super->nvsLst; nvs; nvs=nvs->next ){
		if( (pureName = NvsPrefixMatch( nvs, parName )) != NULL ){

			for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next ){

				if( parGrp->getParam( parGrp, pureName, NULL, 0 ) == 0 ){
					if( pureNameP )
						*pureNameP = pureName;
					return parGrp; /* found */
				}
			}
		}
	}
	return NULL;
}


/**********************************************************************/
/** Determine prefix part of \a parName
 *
 * \param parName		\IN parameter name to (including NVS prefix),
 *							e.g. "c-brd"
 * \param prefix		\OUT receives the prefix string (null terminated)
 *							 (3 chars + term. max).
 * \return parameter name without prefix
 * If prefix part is longer than 3 chars, prefix will be empty
 */
const char *SYSP_Prefix( const char *parName, char *prefix )
{
	int i;

	for( i=0; i<SYSP_PREFIX_LEN && parName[i]!='\0'; i++ ){
		prefix[i] = parName[i];
		if( prefix[i] == '-' ){
			prefix[i] = '\0';
			return &parName[i+1];
		}
	}
	prefix[0] = '\0';
	return parName;
}

/**********************************************************************/
/** Make full parameter name incl. prefix
 *
 * \param nvs				\IN nvs object
 * \param parName			\IN parameter name without prefix
 * \param buf				\OUT receives the full parameter name.
 *								 must have space for
 *								SYSP_PREFIX_LEN	+ SYSP_PARNAME_LEN chars
 * \return pointer to buf
 */
char *SYSP_FullParName(
	SYSP_NV_STORAGE *nvs,
	const char *parName,
	char *buf )
{
	char *s = buf;
	if( nvs && nvs->prefix[0] ){
		SYSP_STRCPY( s, nvs->prefix );
		s += SYSP_STRLEN(s);
		*s++ = '-';
	}
	SYSP_STRCPY( s, parName );
	return buf;
}

#define MAX_CHAR_COUNT 256
/**********************************************************************/
/** Check if \a s contains blanks or tabs
 */
int SYSP_ContainsBlanks( const char *s )
{
	int maxCharCnt=0;
	if ( s == NULL)
		return -1;

	for( ; ( (*s) && (maxCharCnt < MAX_CHAR_COUNT)) ; ++s ) {

		switch (*s) {
			case ' ':
			case '\t':
			return 1;
			case '\0':
				return 0;
			default:
			maxCharCnt++;
	}
	}
	return 0;
}


/**********************************************************************/
/** Find parameter description structure in array by parameter name
 *
 * \param parList		\IN parameter list
 * \param parName		\IN parameter name to find (official or alias)
 * \return SYS_PAR_DESC structure or NULL if not found
 */
const SYSP_PAR_DESC *SYSP_FindParDesc(
	const SYSP_PAR_DESC *parList,
	const char *parName )
{
	const SYSP_PAR_DESC *parDesc;

	for( parDesc=parList; parDesc->parName; parDesc++){

		if( SYSP_STRNCMP( parDesc->parName, parName, SYSP_PARNAME_LEN ) == 0 )
			break;
		if( parDesc->aliasName &&
			SYSP_STRNCMP( parDesc->aliasName, parName, SYSP_PARNAME_LEN )== 0)
			break;
	}
	return parDesc->parName ? parDesc : NULL;
}

/**********************************************************************/
/** get number of entries in SYSP_PAR_DESC_ARRAY
 */
int SYSP_ParDescArrayLen( const SYSP_PAR_DESC *parList )
{
	int n=0;

	while( parList[n].parName != NULL )
		n++;

	return n;
}

/**********************************************************************/
/** Count number of string pointers in array until NULL pointer found
 *
 */
int SYSP_StringArrayLen( const SYSP_CONST_STRING *arr )
{
	int n=0;

	while( arr[n] != NULL )
		n++;

	return n;
}

/**********************************************************************/
/** Check if string array \a contains a string with the same value as \a str
 * \a arr must be NULL terminated
 * \return index in \a arr or -1 if not found
 */
int SYSP_StringArrFindStr( const SYSP_CONST_STRING *arr, SYSP_CONST_STRING str)
{
	const SYSP_CONST_STRING *p;
	int n=0;

	if( str == NULL )
		return -1;

	for( p=arr; *p; p++, n++ ){
		if( SYSP_STRCMP( *p, str ) == 0 )
			return n;
	}

	return -1;
}

/**********************************************************************/
/** copy stringtable \a src to stringtbl \a dst
 *
 * Copy \a max entries (excl. NULL terminator),
 * make sure that \a dst is NULL terminated
 *
 * \return number of entries copied (excluding NULL terminator)
 */
int SYSP_StringArrCopy(
	SYSP_CONST_STRING dst[],
	const SYSP_CONST_STRING src[],
	int max )
{
	const SYSP_CONST_STRING *s = src;
	SYSP_CONST_STRING *d = dst;
	int count=0;

	while( *s ){
		if( count < max ){
			count++;
			*d++ = *s++;
		}
		else
			break;
	}
	*d = NULL;
	return count;
}

/*---------------------------------------------------------------------+
|             RAW<->STRING CONVERSION ROUTINES                         |
+---------------------------------------------------------------------*/

int SYSP_StrSafeCpy( char *dst, const char *src, int destLen )
{
	int n;

	if( destLen == 0 )
		return SYSPARAM_INVAL_VALUE;

	for( n=0; n<destLen && *src; n++ )
		*dst++ = *src++;

	if( n >= destLen ) {
		dst[-1] = '\0';
		return SYSPARAM_VAL_TRUNCATED;
	}
	else {
		*dst = '\0';
		return 0;
	}
}

/**********************************************************************/
/** from int (native endian), decimal */
int SYSP_FromRawNatIntD( char *dest, int destLen, void *src )
{
	char buf[12];

	SYSP_SNPRINTF( buf, sizeof(buf), "%u", (*(unsigned int *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to int (native endian), decimal */
int SYSP_ToRawNatIntD( void *dest, const char *src )
{
	*(unsigned int *)dest = SYSP_STRTOUL( src, NULL, 10 );
	return 0;
}
/**********************************************************************/
/** from int (native endian), hexadecimal */
int SYSP_FromRawNatIntX( char *dest, int destLen, void *src )
{
	char buf[12];

	SYSP_SNPRINTF( buf, sizeof(buf), "%x", (*(unsigned int *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to int (native endian), hexadecimal */
int SYSP_ToRawNatIntX( void *dest, const char *src )
{
	*(unsigned int *)dest = SYSP_STRTOUL( src, NULL, 16 );
	return 0;
}

/**********************************************************************/
/** from u_int32 (big endian), decimal */
int SYSP_FromRawU32D( char *dest, int destLen, void *src )
{
	char buf[12];

	SYSP_SNPRINTF( buf, sizeof(buf), "%lu", SYSP_FROMBE32(*(u_int32 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int32 (big endian), decimal */
int SYSP_ToRawU32D( void *dest, const char *src )
{
	u_int32 tmp = SYSP_STRTOUL( src, NULL, 10 );
	*(u_int32 *)dest = SYSP_TOBE32( tmp );
	return 0;
}

/**********************************************************************/
/** from u_int32 (big endian), hexadecimal */
int SYSP_FromRawU32X( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%lx", SYSP_FROMBE32(*(u_int32 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int32 (big endian), hexadecimal */
int SYSP_ToRawU32X( void *dest, const char *src )
{
	u_int32 tmp = SYSP_STRTOUL( src, NULL, 16 );
	*(u_int32 *)dest = SYSP_TOBE32( tmp );
	return 0;
}

/**********************************************************************/
/** from u_int16 (big endian), hexadecimal */
int SYSP_FromRawU16X( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%x", SYSP_FROMBE16(*(u_int16 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int16 (big endian), hexadecimal */
int SYSP_ToRawU16X( void *dest, const char *src )
{
	u_int16 tmp = SYSP_STRTOUL( src, NULL, 16 );
	*(u_int16 *)dest = SYSP_TOBE16( tmp );
	return 0;
}

/**********************************************************************/
/** from int16 (big endian), decimal */
int SYSP_FromRawS16D( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%d", (int16)SYSP_FROMBE16(*(u_int16 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to int16 (big endian), decimal */
int SYSP_ToRawS16D( void *dest, const char *src )
{
	int16 tmp = SYSP_STRTOL( src, NULL, 10 );
	*(int16 *)dest = SYSP_TOBE16( tmp );
	return 0;
}

/**********************************************************************/
/** from u_int16 (big endian), decimal */
int SYSP_FromRawU16D( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%u", SYSP_FROMBE16(*(u_int16 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int16 (big endian), decimal */
int SYSP_ToRawU16D( void *dest, const char *src )
{
	u_int16 tmp = SYSP_STRTOL( src, NULL, 10 );
	*(u_int16 *)dest = SYSP_TOBE16( tmp );
	return 0;
}

/**********************************************************************/
/** from u_int8, hexadecimal */
int SYSP_FromRawU8X( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%x", *(u_int8 *)src);
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int8, hexadecimal */
int SYSP_ToRawU8X( void *dest, const char *src )
{
	*(u_int8 *)dest = SYSP_STRTOUL( src, NULL, 16 );
	return 0;
}

/**********************************************************************/
/** from u_int8, decimal */
int SYSP_FromRawU8D( char *dest, int destLen, void *src )
{
	char buf[10];

	SYSP_SNPRINTF( buf, sizeof(buf), "%d", *(u_int8 *)src);
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to u_int8, decimal */
int SYSP_ToRawU8D( void *dest, const char *src )
{
	*(u_int8 *)dest = SYSP_STRTOUL( src, NULL, 10 );
	return 0;
}

/**********************************************************************/
/** from string
 * returns an empty string if \a src (raw data) contains any non-printable
 * char.
 */
int SYSP_FromRawStr( char *dest, int destLen, void *src )
{
	int n;
	for( n=0; n<destLen-1 && ((u_int8 *)src)[n]; n++ ){
		if( ! SYSP_PRINTABLE( ((u_int8 *)src)[n] ) ){
			*dest = '\0';
			return SYSPARAM_INVAL_VALUE;
		}
	}
	return SYSP_StrSafeCpy( dest, (char *)src, destLen );
}

/**********************************************************************/
/** to string (hope it was checked by ValidateStr)*/
int SYSP_ToRawStr( void *dest, const char *src )
{
	SYSP_STRCPY( (char *)dest, src );
	return 0;
}

/**********************************************************************/
/** to string (convert to upper case; hope it was checked by ValidateStr)
 */
int SYSP_ToRawStrUpper( void *dest, const char *src )
{
	char* ptr;

	SYSP_STRCPY( (char *)dest, src );

	ptr = (char*)dest;

	while( *ptr != '\0' ){
		if( *ptr >= 'a' && *ptr <= 'z' ) /* make upper case */
			*ptr -= 'a' - 'A';
		ptr++;
	}
	return 0;
}

/**********************************************************************/
/** from revision number
 * returns empty string if all bytes are 0xff
 */
int SYSP_FromRawRev( char *dest, int destLen, void *src )
{
	char buf[12];
	u_int8 *p = (u_int8*)src;

	if( p[0]==0xff && p[1]==0xff && p[2]==0xff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}

	SYSP_SNPRINTF( buf, sizeof(buf), "%02d.%02d.%02d", p[0], p[1], p[2]);
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to revision number
 */
int SYSP_ToRawRev(void *dest, const char *src )
{
	int i;
	u_int32 n;
	u_int8 *p = (u_int8*)dest;

	for( i=0; i<3; ++i ) {
		n = SYSP_STRTOUL( src, (char**)&src, 10 );
		p[i] = n & 0xff;
		++src;
	}

	return 0;
}

/**********************************************************************/
/** check revision number
 */
int SYSP_ValidateRev( const char *s, int notused )
{
	int i;
	u_int32 n;

	for( i=0; i<3; ++i ) {
		n = SYSP_STRTOUL( s, (char**)&s, 10 );

		if( ((i!=2) && *s != '.' ) ||
			((i==2) && *s != '\0' ) ||
			( n > 254 ) )
			return SYSPARAM_INVAL_VALUE;
		++s;
	}

	return 0;
}
/**********************************************************************/
/** from ethernet MAC number
 * returns empty string if all bytes are 0xff
 */
int SYSP_FromRawEth( char *dest, int destLen, void *src )
{
	char buf[12];
	u_int8 *p = (u_int8*)src;
	int i;

	/* check if all bytes are 0xff */
	for( p=(u_int8*)src, i=0; i<6; i++, p++ )
		if( *p != 0xff )
			break;

	if( i == 6 ){
		if( destLen )
			*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}

	{
		char *s = buf;

		for( p=(u_int8*)src, i=0; i<6; i++, p++ )
			s += SYSP_SNPRINTF( s, 6, "%02x", *p );
	}

	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to ethernet MAC number
 */
int SYSP_ToRawEth(void *dest, const char *src )
{
	int i;
	u_int32 n;
	u_int8 *p = (u_int8*)dest;

	for( i=0; i<6; ++i ) {

		p[i] = 0xff;			/* set default */

		if( strlen(src) >= 2 ){
			char buf[3], *bp;
			buf[0] = src[0];
			buf[1] = src[1];
			buf[2] = '\0';

			n = SYSP_STRTOUL( buf, &bp, 16 );

			if( *bp == '\0' )
				p[i] = n & 0xff;
			src += 2;
		}
	}
	return 0;
}

/**********************************************************************/
/** check ethernet MAC number
 */
int SYSP_ValidateEth( const char *s, int notused )
{
	int i;

	if( SYSP_STRLEN( s ) != 12 )
		return SYSPARAM_INVAL_VALUE;

	for( i=0; i<6; ++i ) {
		char buf[3], *p;

		buf[0] = s[0];
		buf[1] = s[1];
		buf[2] = '\0';

		SYSP_STRTOUL( buf, &p, 16 );

		if( *p != '\0' )		/* strtoul detected error */
			return SYSPARAM_INVAL_VALUE;

		s += 2;
	}

	return 0;
}

/**********************************************************************/
/** from touch calibration
 */
int SYSP_FromRawTcal( char *dest, int destLen, void *src )
{
	char buf[50];
	u_int32 *p = (u_int32*)src;
	char *s = buf;
	int i;

	for( i=0; i<4; ++i ) {
		s += SYSP_SNPRINTF( s, sizeof(buf), "%ld%s", SYSP_FROMBE32(p[i]),
						   i==3 ? "" : ",");
	}
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** to touch calibration
 */
int SYSP_ToRawTcal(void *dest, const char *src )
{
	int i;
	u_int32 n;
	u_int32 *p = (u_int32 *)dest;

	for( i=0; i<4; ++i ) {
		n = SYSP_STRTOUL( src, (char**)&src, 10 );
		p[i] = SYSP_TOBE32(n);
		++src;
	}

	return 0;
}

/**********************************************************************/
/** check revision number
 */
int SYSP_ValidateTcal( const char *s, int notused )
{
	int i;

	for( i=0; i<4; ++i ) {
		SYSP_STRTOUL( s, (char**)&s, 10 );

		if( ((i!=3) && *s != ',' ) ||
			((i==3) && *s != '\0' ))
			return SYSPARAM_INVAL_VALUE;
		++s;
	}

	return 0;
}

/**********************************************************************/
/** from boolean */
int SYSP_FromRawBool( char *dest, int destLen, void *src, u_int8 bitNo )
{
	char buf[10];

	SYSP_SNPRINTF( buf,  sizeof(buf), "%d", *(u_int8 *)src & (1U<<bitNo) ? 1 : 0);
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

int SYSP_FromRawBool7( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 7 );
}
int SYSP_FromRawBool6( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 6 );
}
int SYSP_FromRawBool5( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 5 );
}
int SYSP_FromRawBool4( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 4 );
}
int SYSP_FromRawBool3( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 3 );
}
int SYSP_FromRawBool2( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 2 );
}
int SYSP_FromRawBool1( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 1 );
}
int SYSP_FromRawBool0( char *dest, int destLen, void *src )
{
	return SYSP_FromRawBool( dest, destLen, src, 0 );
}

/**********************************************************************/
/** to boolean (accept only '0' or '1') */
int SYSP_ToRawBool( void *dest, const char *src, u_int8 bitNo )
{
	if( *src == '1' )
		*(u_int8 *)dest |= (1U<<bitNo);
	else
		*(u_int8 *)dest &= ~(1U<<bitNo);

	return 0;
}

int SYSP_ToRawBool7( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 7 );
}
int SYSP_ToRawBool6( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 6 );
}
int SYSP_ToRawBool5( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 5 );
}
int SYSP_ToRawBool4( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 4 );
}
int SYSP_ToRawBool3( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 3 );
}
int SYSP_ToRawBool2( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 2 );
}
int SYSP_ToRawBool1( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 1 );
}
int SYSP_ToRawBool0( void *dest, const char *src )
{
	return SYSP_ToRawBool( dest, src, 0 );
}


/**********************************************************************/
/** check boolean (accept only '0' or '1')
 */
int SYSP_ValidateBool( const char *src, int max )
{
	if( (*src == '0' || *src == '1') && src[1] == '\0' )
		return 0;
	return SYSPARAM_INVAL_VALUE;
}


/**********************************************************************/
/** from multi choice index to string
 *
 * \a src is assumed to be an u_int8 value containing the multiple choice
 * index (0..n).
 *
 * If \a src's content is out of range, an empty string is returned
 */
int SYSP_FromRawMchoice(
	SYSP_CONST_STRING *choiceArr,
	char *dest,
	int destLen,
	void *src )
{
	u_int8 idx = *(u_int8 *)src;

	if( idx >= SYSP_StringArrayLen( choiceArr ) )
		return SYSP_StrSafeCpy( dest, "", destLen );

	return SYSP_StrSafeCpy( dest, choiceArr[idx], destLen );
}

/**********************************************************************/
/** to multi choice index
 *
 * if \a src does not match any choice, set index to 0
 */
int SYSP_ToRawMchoice(
	SYSP_CONST_STRING *choiceArr,
	void *dest,
	const char *src )
{
	int n;

	n = SYSP_StringArrFindStr( choiceArr, src );
	if( n<0 || n>255 )
		n = 0;

	*(u_int8 *)dest = n;
	return 0;
}

/**********************************************************************/
/** check multi choice value
 */
int SYSP_ValidateMchoice(
	SYSP_CONST_STRING *choiceArr,
	const char *s )
{
	int n;

	n = SYSP_StringArrFindStr( choiceArr, s );
	if( n<0 || n>255 )
		return SYSPARAM_INVAL_VALUE;
	return 0;
}

/**********************************************************************/
/** from Nspeed number
 */
int SYSP_FromRawNspeed( char *dest, int destLen, void *src )
{
	return SYSP_FromRawMchoice( SYSP_nspeedValue, dest, destLen, src );
}

/**********************************************************************/
/** to Nspeed number
 */
int SYSP_ToRawNspeed(void *dest, const char *src )
{
	return SYSP_ToRawMchoice( SYSP_nspeedValue, dest, src );
}

/**********************************************************************/
/** check Nspeed value
 */
int SYSP_ValidateNspeed( const char *s, int notused )
{
	return SYSP_ValidateMchoice( SYSP_nspeedValue, s );
}

/**********************************************************************/
/** from COM mode number
 */
int SYSP_FromRawComMode( char *dest, int destLen, void *src )
{
	return SYSP_FromRawMchoice( SYSP_comModeValue, dest, destLen, src );
}

/**********************************************************************/
/** to COM mode number
 */
int SYSP_ToRawComMode(void *dest, const char *src )
{
	return SYSP_ToRawMchoice( SYSP_comModeValue, dest, src );
}

/**********************************************************************/
/** check COM mode value
 */
int SYSP_ValidateComMode( const char *s, int notused )
{
	char* ptr = (char*) s;

	while( *ptr != '\0' ){
		if( *ptr >= 'a' && *ptr <= 'z' ) /* make upper case */
			*ptr -= 'a' - 'A';
		ptr++;
	}
	return SYSP_ValidateMchoice( SYSP_comModeValue, s );
}

/**********************************************************************/
/** check baudrate value
 * If \a max is 0, no range checking is done
 */
int SYSP_ValidateBaud( const char *src, int max )
{
	const char *s = src;
	int baud;

	while( *s ) {
		if( !IN_RANGE(*s, '0', '9') )
			return SYSPARAM_INVAL_VALUE;

		++s;
	}

	if( max )
		if( SYSP_STRTOUL( src, NULL, 10 ) > (unsigned long)max )
			return SYSPARAM_INVAL_VALUE;

	baud = SYSP_STRTOUL( src, NULL, 10 );

	/* MMCONS_Print("%s check baud %s %d\n", __FUNCTION__, src, baud ); */

	if( baud < 300 || 115200 < baud || (baud%300) )
	{
		if( SYSP_BOARD_SPEZIFIC_BAUD_CHECK(baud) )
		{
			return SYSPARAM_INVAL_VALUE;	/* wrong baudrate */
		}
	}

	return 0;
}

/**********************************************************************/
/** check decimal unsigned integers
 * If \a max is 0, no range checking is done
 */
int SYSP_ValidateD( const char *src, int max )
{
	const char *s = src;

	while( *s ) {
		if( !IN_RANGE(*s, '0', '9') )
			return SYSPARAM_INVAL_VALUE;

		++s;
	}

	if( max )
		if( SYSP_STRTOUL( src, NULL, 10 ) > (unsigned long)max )
			return SYSPARAM_INVAL_VALUE;

	return 0;
}

/**********************************************************************/
/** check decimal signed integers
 * If \a max is 0, no range checking is done
 */
int SYSP_ValidateSD( const char *src, int max )
{
	const char *s = src;

	if( *s == '-' ) {
		++s;
	}
	while( *s ) {
		if( !IN_RANGE(*s, '0', '9') )
			return SYSPARAM_INVAL_VALUE;

		++s;
	}

	if( max )
		if( SYSP_STRTOL( src, NULL, 10 ) > (unsigned long)max )
			return SYSPARAM_INVAL_VALUE;

	return 0;
}

/**********************************************************************/
/** check hexadecimal unsigned integers
 * If \a max is 0, no range checking is done
 */
int SYSP_ValidateX( const char *src, int max )
{
	const char *s = src;

	while( *s ) {
		if( !(IN_RANGE(*s, '0', '9') ||
			  IN_RANGE(*s, 'a', 'f') ||
			  IN_RANGE(*s, 'A', 'F')) )
			return SYSPARAM_INVAL_VALUE;

		++s;
	}

	if( max )
		if( SYSP_STRTOUL( src, NULL, 16 ) > (unsigned long)max )
			return SYSPARAM_INVAL_VALUE;

	return 0;
}

/**********************************************************************/
/** check length of string
 * \a max is the max allowed string length, excluding term '\\0'
 */
int SYSP_ValidateStr( const char *src, int max )
{
	if( SYSP_STRLEN( src ) >= (u_int32)max )
		return SYSPARAM_INVAL_VALUE;
	return 0;
}

/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST

SYSP_UNITTEST( "SYSPARAM2: basic core functions", SYSP_UtCore );

/** Checks basic core functions */
int SYSP_UtCore( void )
{
	/* SYSP_StrSafeCpy */
	{
		const char *a = "ABC", *exp1="ABC\0\xaa", *exp2="AB\0\xaa\xaa",
			*exp3 = "A\0\xaa\xaa\xaa";
		char buf[5];

		SYSP_MEMSET( buf, 0xaa, 5);
		UTASSERT( SYSP_StrSafeCpy( buf, a, 5 ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp1, buf, 5 ) == 0 );

		SYSP_MEMSET( buf, 0xaa, 5);
		UTASSERT( SYSP_StrSafeCpy( buf, a, 4 ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp1, buf, 5 ) == 0 );

		SYSP_MEMSET( buf, 0xaa, 5);
		UTASSERT( SYSP_StrSafeCpy( buf, a, 3 ) == SYSPARAM_VAL_TRUNCATED);
		UTASSERT( SYSP_MEMCMP( exp2, buf, 5 ) == 0 );

		SYSP_MEMSET( buf, 0xaa, 5);
		UTASSERT( SYSP_StrSafeCpy( buf, a, 2 ) == SYSPARAM_VAL_TRUNCATED);
		UTASSERT( SYSP_MEMCMP( exp3, buf, 5 ) == 0 );

		UTASSERT( SYSP_StrSafeCpy( buf, a, 0 ) == SYSPARAM_INVAL_VALUE);
	}

	/* SYSP_From/ToRawU32D */
	{
		u_int8 raw[4];
		u_int8 exp1[] = { 0x00, 0x00, 0x00, 0x01 };
		u_int8 exp2[] = { 0xff, 0xff, 0xff, 0xff };
		char buf[12];

		UTASSERT( SYSP_ToRawU32D( raw, "1" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp1, raw, sizeof(raw) ) == 0 );

		UTASSERT( SYSP_ToRawU32D( raw, "4294967295" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp2, raw, sizeof(raw) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU32D( buf, sizeof(buf), exp1 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "1", buf, sizeof(buf) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU32D( buf, sizeof(buf), exp2 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "4294967295", buf, sizeof(buf) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU32D( buf, 4, exp2 ) == SYSPARAM_VAL_TRUNCATED );
		UTASSERT( SYSP_STRNCMP( "429", buf, sizeof(buf) ) == 0 );
	}

	/* SYSP_From/ToRawU32X */
	{
		u_int8 raw[4];
		u_int8 exp1[] = { 0x04, 0x03, 0x02, 0x01 };
		u_int8 exp2[] = { 0xff, 0xff, 0xff, 0xff };
		char buf[12];

		UTASSERT( SYSP_ToRawU32X( raw, "04030201" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp1, raw, sizeof(raw) ) == 0 );

		UTASSERT( SYSP_ToRawU32X( raw, "FFffFFff" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp2, raw, sizeof(raw) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU32X( buf, sizeof(buf), exp1 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "4030201", buf, sizeof(buf) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU32X( buf, sizeof(buf), exp2 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "ffffffff", buf, sizeof(buf) ) == 0 );
	}

	/* SYSP_From/ToRawU16X */
	{
		u_int8 raw[2];
		u_int8 exp1[] = { 0x04, 0x03 };
		u_int8 exp2[] = { 0xff, 0xff };
		char buf[12];

		UTASSERT( SYSP_ToRawU16X( raw, "0403" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp1, raw, sizeof(raw) ) == 0 );

		UTASSERT( SYSP_ToRawU16X( raw, "FFff" ) == 0 );
		UTASSERT( SYSP_MEMCMP( exp2, raw, sizeof(raw) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU16X( buf, sizeof(buf), exp1 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "403", buf, sizeof(buf) ) == 0 );

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawU16X( buf, sizeof(buf), exp2 ) == 0 );
		UTASSERT( SYSP_STRNCMP( "ffff", buf, sizeof(buf) ) == 0 );
	}

	/* SYSP_From/ToRawU8X/U8D untested */

	/* SYSP_FromRawStr */
	{
		u_int8 raw[4];
		char buf[5];

		/* good case */
		SYSP_MEMMOVE( raw, "ABC\0", sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawStr( buf, 4, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "ABC\0\xaa", 5 ) == 0 );

		/* unterm. string in raw */
		SYSP_MEMSET( raw, 'A', sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawStr( buf, 4, raw ) == SYSPARAM_VAL_TRUNCATED );
		UTASSERT( SYSP_MEMCMP( buf, "AAA\0\xaa", 5 ) == 0 );

		/* nonprintable chars in raw */
		SYSP_MEMSET( raw, 'A', sizeof(raw));
		raw[2] = 0x7f;
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawStr( buf, 4, raw ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_MEMCMP( buf, "\0\xaa", 2 ) == 0 );

	}

	/* SYSP_From/To/ValidateRawRev */
	{
		u_int8 raw[3];
		char buf[12];

		/* good case */
		SYSP_MEMMOVE( raw, "\x12\x34\x6", sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawRev( buf, 12, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "18.52.06\0\xaa", 10 ) == 0 );

		/* raw contains all FF */
		SYSP_MEMMOVE( raw, "\xff\xff\xff", sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawRev( buf, 12, raw ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_MEMCMP( buf, "\0", 1 ) == 0 );

		/* validate */
		UTASSERT( SYSP_ValidateRev( "00.00.00", 0 ) == 0 );
		UTASSERT( SYSP_ValidateRev( "1.2.3", 0 ) == 0 );
		UTASSERT( SYSP_ValidateRev( "254.254.254", 0 ) == 0 );
		UTASSERT( SYSP_ValidateRev( "254.254.255", 0 ) ==SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_ValidateRev( "254.254", 0 ) ==SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_ValidateRev( "00.A1.01", 0 ) ==SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_ValidateRev( "..", 0 ) == 0);

		/* toRaw */
		UTASSERT( SYSP_ToRawRev( raw, "01.44.55" ) == 0 );
		UTASSERT( SYSP_MEMCMP( "\x01\x2c\x37", raw, sizeof(raw) ) == 0 );


	}

	/* SYSP_From/To/ValidateTcal */
	{
		u_int8 raw[16];
		char buf[50];
		const u_int8 t1 [] = {
			0x00, 0x00, 0x00, 0x01,
			0x44, 0x00, 0x00, 0x02,
			0x00, 0x33, 0x22, 0x03,
			0x00, 0x00, 0x00, 0x04
		};

		/* good case */
		SYSP_MEMMOVE( raw, t1, sizeof(raw));
		UTASSERT( SYSP_FromRawTcal( buf, 50, raw ) == 0 );
		UTASSERT( SYSP_STRCMP( buf, "1,1140850690,3351043,4" ) == 0 );

		/* validate */
		UTASSERT( SYSP_ValidateTcal( "0,1,2,3", 0 ) == 0 );
		UTASSERT( SYSP_ValidateTcal( "10000,20000,30000,1140850690", 0 )== 0 );
		UTASSERT( SYSP_ValidateTcal( "1,3,4", 0 ) ==SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_ValidateTcal( "1.2", 0 ) ==SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_ValidateTcal( "1A000,20000,30000,1140850690", 0 )
			==SYSPARAM_INVAL_VALUE);


		/* toRaw */
		UTASSERT( SYSP_ToRawTcal( raw, "1,1140850690,3351043,4" ) == 0 );
		UTASSERT( SYSP_MEMCMP( t1, raw, sizeof(raw) ) == 0 );


	}


	/* SYSP_ValidateX/D/Str */
	{
		UTASSERT( SYSP_ValidateD( "1234", 0 ) == 0 );
		UTASSERT( SYSP_ValidateD( "123a", 0 ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateD( "-123", 0 ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateD( " 123", 0 ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateD( "123 ", 0 ) == SYSPARAM_INVAL_VALUE );

		UTASSERT( SYSP_ValidateD( "123", 123 ) == 0 );
		UTASSERT( SYSP_ValidateD( "124", 123 ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateD( "4294967295", 4294967295UL ) == 0 );
		UTASSERT( SYSP_ValidateD( "4294967295", 4294967294UL ) ==
				  SYSPARAM_INVAL_VALUE );

		UTASSERT( SYSP_ValidateX( "1234", 0 ) == 0 );
		UTASSERT( SYSP_ValidateX( "aAbB7", 0 ) == 0 );
		UTASSERT( SYSP_ValidateX( "FFFFFFFF", 0xFFFFFFFFUL ) == 0 );
		UTASSERT( SYSP_ValidateX( "FFFFFFFF", 0xFFFFFFFEUL ) ==
				  SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateX( "0xFFFFFFFF", 0 ) ==
				  SYSPARAM_INVAL_VALUE );

		UTASSERT( SYSP_ValidateStr( "ABC", 4 ) == 0 );
		UTASSERT( SYSP_ValidateStr( "ABC", 3 ) == SYSPARAM_INVAL_VALUE);
	}

	/* SYSP_Prefix */
	{
		char pfx[5];

		UTASSERT( SYSP_STRNCMP( SYSP_Prefix( "abc", pfx ), "abc", 4 ) == 0 );
		UTASSERT( SYSP_STRNCMP( pfx, "", 5 ) == 0 );

		UTASSERT( SYSP_STRNCMP( SYSP_Prefix( "c-abc", pfx ), "abc", 4 ) == 0 );
		UTASSERT( SYSP_STRNCMP( pfx, "c", 5 ) == 0 );

		UTASSERT( SYSP_STRNCMP( SYSP_Prefix( "cde-abc", pfx ), "abc", 4 )==0 );
		UTASSERT( SYSP_STRNCMP( pfx, "cde", 5 ) == 0 );

		UTASSERT( SYSP_STRNCMP( SYSP_Prefix( "cdef-abc", pfx ), "cdef-abc", 8
					  )==0 );
		UTASSERT( SYSP_STRNCMP( pfx, "", 5 ) == 0 );

		UTASSERT( SYSP_STRNCMP( SYSP_Prefix( "s\0-----", pfx ), "s", 8  )==0 );
		UTASSERT( SYSP_STRNCMP( pfx, "", 5 ) == 0 );
	}

	/* SYSP_From/To/ValidateEth */
	{
		u_int8 raw[6];
		char buf[14];

		/* good case */
		SYSP_MEMMOVE( raw, "\x00\xc0\x3a\x04\x05\x06", sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawEth( buf, 13, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "00c03a040506\0\xaa", 14 ) == 0 );

		/* raw contains all FF */
		SYSP_MEMMOVE( raw, "\xff\xff\xff\xff\xff\xff", sizeof(raw));
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawEth( buf, 13, raw ) ==  SYSPARAM_INVAL_VALUE);
		UTASSERT( SYSP_MEMCMP( buf, "\0", 1 ) == 0 );

		/* validate */
		UTASSERT( SYSP_ValidateEth( "00c03a006677", 0 ) == 0 );
		UTASSERT( SYSP_ValidateEth( "00ck3a006677", 0 ) ==
				  SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateEth( "00c03a006677a", 0 ) ==
				  SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateEth( "00c03a00667", 0 ) ==
				  SYSPARAM_INVAL_VALUE );

		/* to ethernet mac */
		SYSP_MEMSET( raw, 0xaa, sizeof(raw));
		UTASSERT( SYSP_ToRawEth( raw, "00c03a006677" ) == 0 );
		UTASSERT( SYSP_MEMCMP( "\x00\xc0\x3a\x00\x66\x77",
							   raw, sizeof(raw) ) == 0 );

		/* to ethernet mac, mac too short */
		SYSP_MEMSET( raw, 0xaa, sizeof(raw));
		UTASSERT( SYSP_ToRawEth( raw, "00c03a00667" ) == 0 );
		UTASSERT( SYSP_MEMCMP( "\x00\xc0\x3a\x00\x66\xff",
							   raw, sizeof(raw) ) == 0 );


	}


	/* SYSP_From/To/ValidateNspeed */
	{
		u_int8 raw;
		char buf[14];

		raw = 0;
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawNspeed( buf, 5, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "AUTO\0\xaa", 5 ) == 0 );

		raw = 5;
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawNspeed( buf, 5, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "1000\0\xaa", 5 ) == 0 );

		raw = 6;				/* out of range */
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawNspeed( buf, 5, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "\0\xaa", 2 ) == 0 );

		UTASSERT( SYSP_ToRawNspeed( &raw, "AUTO" ) == 0 );
		UTASSERT( raw == 0 );
		UTASSERT( SYSP_ToRawNspeed( &raw, "10HD" ) == 0 );
		UTASSERT( raw == 1 );
		UTASSERT( SYSP_ToRawNspeed( &raw, "1000" ) == 0 );
		UTASSERT( raw == 5 );
		UTASSERT( SYSP_ToRawNspeed( &raw, "XYZ" ) == 0 );
		UTASSERT( raw == 0 );

		UTASSERT( SYSP_ValidateNspeed( "AUTO", 0 ) == 0 );
		UTASSERT( SYSP_ValidateNspeed( "1000", 0 ) == 0 );
		UTASSERT( SYSP_ValidateNspeed( "XYZ", 0 ) == SYSPARAM_INVAL_VALUE );
	}

	/* SYSP_From/To/ValidateBool */
	{
		u_int8 raw;
		char buf[14];

		/* good cases */
		raw = 0;
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawBool7( buf, 13, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "0\0\xaa", 3 ) == 0 );

		raw = 0x80;
		UTASSERT( SYSP_FromRawBool7( buf, 13, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "1\0\xaa", 3 ) == 0 );
		UTASSERT( SYSP_FromRawBool6( buf, 13, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "0\0\xaa", 3 ) == 0 );

		raw = 0x40;
		UTASSERT( SYSP_FromRawBool6( buf, 13, &raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "1\0\xaa", 3 ) == 0 );

		/* validate */
		UTASSERT( SYSP_ValidateBool( "0", 0 ) == 0 );
		UTASSERT( SYSP_ValidateBool( "1", 0 ) == 0 );
		UTASSERT( SYSP_ValidateBool( "11", 0 ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_ValidateBool( "t", 0 ) == SYSPARAM_INVAL_VALUE );

		/* to raw bool */
		raw = 0xaa;
		UTASSERT( SYSP_ToRawBool6( &raw, "1" ) == 0 );
		UTASSERT( raw == (0xaa | 0x40) );

		UTASSERT( SYSP_ToRawBool7( &raw, "0" ) == 0 );
		UTASSERT( raw == ((0xaa | 0x40) & ~0x80) );

	}

	DBGWRT_1((DBH,"SYSP_UtCore OK\n"));
	return 0;
 ABORT:
	return -1;
}

SYSP_UNITTEST( "SYSPARAM2: StringTbl routines", SYSP_UtStringTbl );



int SYSP_UtStringTbl(void)
{
	const char *tbl0[] = { NULL };
	const char *tbl1[] = { "ABC", NULL };
	const char *tbl2[] = { "ABC", "DEF", NULL };

	/* SYSP_StringArrayLen */
	{
		UTASSERT( SYSP_StringArrayLen( tbl0 ) == 0 );
		UTASSERT( SYSP_StringArrayLen( tbl1 ) == 1 );
		UTASSERT( SYSP_StringArrayLen( tbl2 ) == 2 );
	}

	/* SYSP_StringArrFindStr */
	{
		UTASSERT( SYSP_StringArrFindStr( tbl0, "ABC" ) == -1 );
		UTASSERT( SYSP_StringArrFindStr( tbl1, NULL ) == -1 );

		UTASSERT( SYSP_StringArrFindStr( tbl1, "ABC" ) == 0 );
		UTASSERT( SYSP_StringArrFindStr( tbl2, "ABC" ) == 0 );
		UTASSERT( SYSP_StringArrFindStr( tbl2, "DEF" ) == 1 );
		UTASSERT( SYSP_StringArrFindStr( tbl2, "DE"  ) == -1 );
		UTASSERT( SYSP_StringArrFindStr( tbl2, "DEFG" ) == -1 );
	}

	/* SYSP_StringArrCopy */
	{
		const char *dst[5];

		SYSP_MEMSET( dst, 0xaa, sizeof(dst));
		UTASSERT( SYSP_StringArrCopy( dst, tbl0, 0 ) == 0 );
		UTASSERT( dst[0] == NULL );
		UTASSERT( dst[1] == (const char *)0xaaaaaaaa );

		SYSP_MEMSET( dst, 0xaa, sizeof(dst));
		UTASSERT( SYSP_StringArrCopy( dst, tbl0, 1 ) == 0 );
		UTASSERT( dst[0] == NULL );
		UTASSERT( dst[1] == (const char *)0xaaaaaaaa );

		SYSP_MEMSET( dst, 0xaa, sizeof(dst));
		UTASSERT( SYSP_StringArrCopy( dst, tbl1, 10 ) == 1 );
		UTASSERT( dst[0] == tbl1[0] );
		UTASSERT( dst[1] == NULL );
		UTASSERT( dst[2] == (const char *)0xaaaaaaaa );

		SYSP_MEMSET( dst, 0xaa, sizeof(dst));
		UTASSERT( SYSP_StringArrCopy( dst, tbl2, 10 ) == 2 );
		UTASSERT( dst[0] == tbl2[0] );
		UTASSERT( dst[1] == tbl2[1] );
		UTASSERT( dst[2] == NULL );
		UTASSERT( dst[3] == (const char *)0xaaaaaaaa );

		SYSP_MEMSET( dst, 0xaa, sizeof(dst));
		UTASSERT( SYSP_StringArrCopy( dst, tbl2, 1 ) == 1 );
		UTASSERT( dst[0] == tbl2[0] );
		UTASSERT( dst[1] == NULL );
		UTASSERT( dst[2] == (const char *)0xaaaaaaaa );
	}

	return 0;
 ABORT:
	return -1;

}


#endif /* SYSP_CONFIG_UNITTEST */


