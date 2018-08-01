/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_apiex.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/06/07 16:15:20 $
 *    $Revision: 1.10 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  Extended API routines of SYSPARAM
 *      
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_apiex.c,v $
 * Revision 1.10  2010/06/07 16:15:20  sy
 * R:1. EEPROD3 is not compatible with EEPROD2
 * M: 1. For EEPROD3, new implementation of SYSP_MenBrdProdString function
 *
 * Revision 1.9  2006/05/17 13:01:34  rt
 * added
 *  + SysParamUnsignedIntArray
 *
 * Revision 1.8  2005/06/23 15:54:07  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.7  2005/04/12 16:52:34  kp
 * cosmetics
 *
 * Revision 1.6  2005/02/02 16:03:04  kp
 * + SysParamBool()
 *
 * Revision 1.5  2005/01/18 15:49:19  kp
 * + functions for raw NVS access
 *
 * Revision 1.4  2005/01/14 09:36:15  kp
 * fixed return values in SysParamXXXInt()
 * + SYSP_MenBrdProdString()
 * + SYSP_MenBrdNameWithModel()
 *
 * Revision 1.3  2005/01/12 14:19:49  kp
 * prepared SysParamNvsWrite()
 *
 * Revision 1.2  2004/12/20 08:51:15  kp
 * added functions to extract system parameter values and convert them to int
 *
 * Revision 1.1  2004/01/08 09:06:46  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"


/**********************************************************************/
/** Convert SYSPARAM error code to string
 *
 * \param errCode		\IN SYSPARAM error code
 *
 * \return Error string
 */
const char *SysParamErrStr( int errCode )
{
	switch( errCode ){
	case SYSPARAM_NOT_INIT: 	return "SYSPARAM not initialized";
	case SYSPARAM_INVAL_PARAM: 	return "Unknown parameter";
	case SYSPARAM_INVAL_VALUE: 	return "Invalid value";
	case SYSPARAM_EEPROM_ERROR:	return "Non volatile storage error";
	case SYSPARAM_READ_ONLY:	return "Parameter is read-only";
	case SYSPARAM_NO_RESOURCE:	return "No resources/out of space";
	case SYSPARAM_VAL_TRUNCATED:return "Value truncated";
	case SYSPARAM_NOT_SUPPORTED: return "Operation not supported";	
	default:					return "Unknown SYSPARAM error code";
	}
}

/**********************************************************************/
/** Set system parameter using printf style arguments
 *
 * \warning Formatted string may not exceed 40 chars!
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param passwd		\IN an optional password for protected parameters
 *							can be NULL if parameter is not protected
 * \param attr			\IN attribute flags (see #SYSPARAM_SET_ATTR)
 * \param fmt			\IN printf like format string
 * \param ...			\IN arguments for format
 *
 * \return sysparam error code
 * \sa SysParamSetEx
 */
int SysParamSetFmt( 
	const char *parName,
	const char *passwd, 
	int attr,
	const char *fmt, ... )
{
	va_list ap;
	char buf[40];

	va_start(ap,fmt);
	vsprintf( buf, fmt, ap );
	va_end(ap);
	
	return SysParamSetEx( parName, buf, passwd, attr );
}



/**********************************************************************/
/** Get value of signed integer system parameter
 *
 * On error, or when parameter is empty, default value \a def is applied.
 * If parameter is empty, no error is returned.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param base			\IN radix 10 or 16
 * \param def			\IN default value
 * \param valueP		\OUT receives value of system parameter
 *
 * \return sysparam error code
 * \sa SysParamGet
 */
int SysParamSignedInt( const char *par, int base, int32 def, int32 *valueP )
{
	char buf[20];
	int rv = 0;

	*valueP = def;

	if( (rv = SysParamGet( par, buf, sizeof(buf))) != 0)
		return rv;

	if( buf[0] == '\0' )
		return 0;

	if( base == 10 )
		rv = SYSP_ValidateD( buf, 0x7FFFFFFF );
	else
		rv = SYSP_ValidateX( buf, 0x7FFFFFFF );

	if( rv )
		return rv;


	*valueP = SYSP_STRTOL( buf, NULL, base );

	return 0;
}

/**********************************************************************/
/** Get default value of signed integer system parameter
 *
 * On error, or when parameter is empty, default value \a def is applied.
 * If parameter is empty, no error is returned.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param base			\IN radix 10 or 16
 * \param def			\IN default value (if parameter/value invalid)
 * \param valueP		\OUT receives value of system parameter
 *
 * \return sysparam error code
 * \sa SysParamGet
 */
int SysParamDefSignedInt( const char *par, int base, int32 def, int32 *valueP )
{
	char buf[20];
	int rv = 0;

	*valueP = def;

	if( (rv = SysParamGetDefault( par, buf, sizeof(buf))) != 0)
		return rv;

	if( buf[0] == '\0' )
		return 0;

	*valueP = SYSP_STRTOL( buf, NULL, base );

	return 0;
}

/**********************************************************************/
/** Get value of unsigned integer system parameter
 *
 * On error, or when parameter is empty, default value \a def is applied.
 * If parameter is empty, no error is returned.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param base			\IN radix 10 or 16
 * \param def			\IN default value
 * \param valueP		\OUT receives value of system parameter
 *
 * \return sysparam error code
 * \sa SysParamGet
 */
int SysParamUnsignedInt( 
	const char *par, 
	int base, 
	u_int32 def, 
	u_int32 *valueP)
{
	char buf[20];
	int rv = 0;

	*valueP = def;

	if( (rv = SysParamGet( par, buf, sizeof(buf))) != 0)
		return rv;

	if( buf[0] == '\0' )
		return 0;

	if( base == 10 )
		rv = SYSP_ValidateD( buf, 0 );
	else
		rv = SYSP_ValidateX( buf, 0 );

	if( rv )
		return rv;


	*valueP = SYSP_STRTOUL( buf, NULL, base );

	return 0;
}

/**********************************************************************/
/** Get array of unsigned integer system parameters
 *
 * On error, or when parameter is empty, default value \a def is applied.
 * If parameter is empty, no error is returned.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param base			\IN radix 10 or 16
 * \param no			\IN number of array elements
 * \param def			\IN default values
 * \param valueP		\OUT receives value of system parameter
 *
 * \return sysparam error code
 * \sa SysParamGet
 */
int SysParamUnsignedIntArray( 
	const char *par, 
	int base, 
	int no,
	u_int32* defP, 
	u_int32* valueP)
{
	char buf[100];
	char* next;
	char* ptr;	
	int rv = 0;
	int i;
	
    /* read parameter string */
	if( (rv = SysParamGet( par, buf, sizeof(buf))) != 0)
		goto ABORT;

    /* empty? */
	if( buf[0] == '\0' )
		goto ABORT;
	
	/* convert */
	next = buf;
	
	for( i=0; i<no; i++, next++ )
	{
        ptr = next;
	      	    
	    valueP[i] = SYSP_STRTOUL( ptr, &next, base );
	    
        if( next[0] != ',' ) break;
	}
	
	/* check */
    if( no-1 != i || next[0] != '\0' )
    {
        rv = SYSPARAM_INVAL_VALUE;
        goto ABORT;
    }

	return 0;
	
ABORT:
	/* default values */
	for( i=0; i<no; i++ )
	{
	    valueP[i] = defP[i];
	}
	
	return rv;
}

/**********************************************************************/
/** Get value of boolean system parameter
 *
 * On error, or when parameter is empty, default value \a def is applied.
 *
 * \param parName		\IN parameter name including possible prefix.
 * \param def			\IN default value (0/1)
 *
 * \return value of system parameter or \a def on error
 */
int SysParamBool( const char *par, int def)
{
	int32 value;

	SysParamSignedInt( par, 2, def, &value );
	return value;
}

/**********************************************************************/
/** Determine chars 3..9 (e.g. EM04-00) of MEN product name 
 *
 * Uses SYSP_brd and SYSP_brdmod parameter. SYSP_brd value is padded with
 * '-' if it contains only 4 chars
 *
 * If SYSP_brd not valid, return empty string. 
 * If SYSP_brdmod not valid, model number will be missing from string
 *
 * \param prefix	\IN board prefix (e.g. 'c') (0 if none)
 * \param buf		\OUT filled with name (must have space for 7 chars + EOS)
 *						 
 *
 * \return sysparam error code
 */
int SYSP_MenBrdNameWithModel( char prefix, char *buf )
{
	u_int32 model;
	int rv, len;
	char parName[10];

	if( (rv = SysParamGet( SYSP_PrefixParamName( prefix, SYSP_brd, parName ),
						   buf, 6)) == 0){
		if( (len = SYSP_STRLEN( buf )) == 4 ){
			/* board name, append possible '-' */
			buf[4] = '-';
			buf[5] = '\0';
		}
		else if( len != 5 ){
			rv = SYSPARAM_INVAL_VALUE;
			buf[0] = '\0';
			goto EXIT;
		}

		/* model name */
		if( (rv = SysParamUnsignedInt( SYSP_PrefixParamName( 
										   prefix, SYSP_brdmod, parName ), 
									   10, 0, &model )) == 0 ){
			if( model <= 99 )
				SYSP_SNPRINTF( &buf[5], 2, "%02ld", model );
			else
				rv = SYSPARAM_INVAL_VALUE;
		}
	}
 EXIT:
	return rv;
}

/**********************************************************************/
/** Make full production data string
 *
 * Generates a string like this:
 * EM04-00, Rev 01.02.03, Serial 8765
 *
 * \param prefix	\IN board prefix (e.g. 'c') (0 if none)
 * \param buf		\OUT filled with name (must have space for approx. 40 
 *						chars)
 *						 
 * \return sysparam error code
 */
int SYSP_MenBrdProdString( char prefix, char *buf )
{
	int rv = 0;
	char *p;
	char valBuf[20];
	char parName[10];
#ifndef _EEPROD3_
	if( (rv = SYSP_MenBrdNameWithModel( prefix, buf )) != 0 )
		if( buf[0] == '\0')
			return rv;

	p = buf + SYSP_STRLEN(buf);
#else
	p = buf;	
	if( SysParamGet( SYSP_PrefixParamName( 
						 prefix, SYSP_brd, parName ), valBuf, 
					 sizeof(valBuf)) == 0 )
		p += SYSP_SNPRINTF( p, 16, "Brd %s", valBuf );
		
	if( SysParamGet( SYSP_PrefixParamName( 
						 prefix, SYSP_brdmod, parName ), valBuf, 
					 sizeof(valBuf)) == 0 )
		p += SYSP_SNPRINTF( p, 16, ", Brdmod %s", valBuf );
	
#endif	
	if( SysParamGet( SYSP_PrefixParamName( 
						 prefix, SYSP_brdrev, parName ), valBuf, 
					 sizeof(valBuf)) == 0 )
		p += SYSP_SNPRINTF( p, 16, ", Rev %s", valBuf );
	
	if( SysParamGet( SYSP_PrefixParamName( 
						 prefix, SYSP_sernbr, parName ), valBuf, 
					 sizeof(valBuf)) == 0 )
		p += SYSP_SNPRINTF( p, 16, ", Serial %s", valBuf );
	return rv;
}

/**********************************************************************/
/** Write specified NVS section
 *
 * \param nvsName	\IN NVS section to write
 *
 * \return sysparam error code
 */
int SYSP_NvsWriteRaw( 
	SYSP_CONST_STRING nvsName,
	int offset,
	int size,
	const u_int8 *data)
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;

	if( (nvs = SYSP_NvsFind( super, nvsName )) == NULL )
		return SYSPARAM_INVAL_PARAM;

	return nvs->writeNvs( nvs, offset, size, data );
}

/**********************************************************************/
/** Read raw from specified NVS section
 *
 * \param nvsName	\IN NVS section name to read from
 * \param offset		\IN starting byte offset within NVS
 * \param size			\IN number of bytes to read from NVS
 * \param data			\OUT receives read data
 * \return SYSPARAM error code
 */
int SYSP_NvsReadRaw( 
	SYSP_CONST_STRING nvsName,
	int offset,
	int size,
	u_int8 *data)
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;
	
	if( (nvs = SYSP_NvsFind( super, nvsName )) == NULL )
		return SYSPARAM_INVAL_PARAM;

	return nvs->readNvs( nvs, offset, size, data );
}

/**********************************************************************/
/** Enumerate NVS sections
 *
 * \param idx		\IN nth section to query
 * \param info		\OUT filled with info about NVS section
 * 
 * \return 0 if ok (section found), or SYSPARAM_INVAL_VALUE if \a idx
 *		   out of range
 */
int SYSP_NvsEnum( int idx, SYSPARAM_NVS_INFO *info )
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	int i;
	int rv = SYSPARAM_INVAL_VALUE;

	if( super==NULL || !(super->initialized) )
		return SYSPARAM_NOT_INIT;
	
	for( i=0, nvs=super->nvsLst; nvs; nvs=nvs->next, i++ ){
		if( i==idx ){
			SYSP_StrSafeCpy( info->prefix, nvs->prefix, SYSP_PREFIX_LEN );
			SYSP_StrSafeCpy( info->name, nvs->nvsName, SYSP_NVSNAME_LEN );
			info->nvsSize = nvs->nvsSize;
			rv = 0;
			break;
		}
	}
	return rv;
}


