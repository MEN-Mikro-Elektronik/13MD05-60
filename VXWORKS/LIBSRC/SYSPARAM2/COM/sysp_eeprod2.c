/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_eeprod2.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/03/11 16:36:19 $
 *    $Revision: 1.9 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Handler for parameter group conforming to EEPROD2 structure
 *
 *    \switches  SYSP_NAME_NO_UPPER_CASE - do not convert board name
 *										   to upper case
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_eeprod2.c,v $
 * Revision 1.9  2010/03/11 16:36:19  sy
 * R:1. For some Ports, the PROD2 data structure can not be stored from the very begin of EEPROM memory space
 * M: 1. Add function SYSP_Eeprod2GrpCreateEx, which accepts offset parameter
 *
 * Revision 1.8  2007/06/28 13:48:06  rt
 * convert board names to upper case (see SYSP_NAME_NO_UPPER_CASE)
 *
 * Revision 1.7  2005/06/23 15:54:12  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.6  2005/04/12 16:52:36  kp
 * avoid naming conflicts when entire SYSPARAM included in one big file
 * (VxWorks syslib...)
 *
 * Revision 1.5  2005/03/03 17:28:51  CSchuster
 * replaced label OK with EXIT_OK (was already defined with VxWorks 6.0)
 *
 * Revision 1.4  2005/02/22 15:00:54  kp
 * work around for OS-9 compiler (ERROR predefined)
 *
 * Revision 1.3  2004/12/20 08:52:18  kp
 * use SYSP tags
 *
 * Revision 1.2  2004/11/29 10:52:07  kp
 * support for MM unittest
 *
 * Revision 1.1  2004/01/08 09:06:48  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"
#include <MEN/eeprod.h>


#define _EEP2OFF(x) offsetof(EEPROD2,x)

static int Prod2FromRawU8D( char *dest, int destLen, void *src );
static int Prod2FromRawU32D( char *dest, int destLen, void *src );
static int Prod2FromRawDate( char *dest, int destLen, void *src );
static int Prod2ToRawDate( void *dest, const char *src );


const SYSP_PAR_DESC SYSP_parListEeprod2[] = {
	SYSP_PDE_REV( SYSP_brdrev, 	NULL, _EEP2OFF(pd_revision)),

	{ SYSP_sernbr, 	NULL, _EEP2OFF(pd_serial),
	  SYSP_ToRawU32D, Prod2FromRawU32D, SYSP_ValidateD,
	  NULL, 0, SYSP_PD_MMPARASTR },

	{ SYSP_brdmod, 	NULL, _EEP2OFF(pd_model),
	  SYSP_ToRawU8D, Prod2FromRawU8D, SYSP_ValidateD,
	  NULL, 0xFF, SYSP_PD_MMPARASTR },
#ifdef SYSP_NAME_NO_UPPER_CASE
	SYSP_PDE_STR( SYSP_brd, 		NULL, _EEP2OFF(pd_hwName),
				  NULL, 6, SYSP_PD_MMPARASTR | SYSP_PD_RESTART ),
#else
	SYSP_PDE_STRUPPER( SYSP_brd, 		NULL, _EEP2OFF(pd_hwName),
				  	   NULL, 6, SYSP_PD_MMPARASTR | SYSP_PD_RESTART ),
#endif /*SYSP_NAME_NO_UPPER_CASE*/

	{ SYSP_prodat, 	NULL, _EEP2OFF(pd_prodat),
	  Prod2ToRawDate, Prod2FromRawDate, NULL,
	  NULL, 0, SYSP_PD_MMPARASTR },

	{ SYSP_repdat, 	NULL, _EEP2OFF(pd_repdat),
	  Prod2ToRawDate, Prod2FromRawDate, NULL,
	  NULL, 0, SYSP_PD_MMPARASTR },

	SYSP_PDE_END
};


/**********************************************************************/
/** Probe for EEPROD2 parameter group
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
int SYSP_Eeprod2GrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	int ignoreNvsError)
{	
	return SYSP_Eeprod2GrpCreateEx( nvs, expHwNames, 0, ignoreNvsError);	
}

/**********************************************************************/
/** Probe for EEPROD2 parameter group
 *
 * On success, (and if hwName matched), the created parameter group
 * is added to the NVS' list.
 *
 * \param nvs				\IN NV storage object (fully initialized)
 * \param expHwNames 		\IN array of bord names tolerated
 *								(null-terminated).
 * \param offset 		\IN offset of EEPROD2 structure inside EEPROM 
 * \param ignoreNvsError 	\IN if TRUE, a NVS read error is ignored
 *
 * \return sysparam error code
 *
 */
int SYSP_Eeprod2GrpCreateEx(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	u_int32	offset,
	int ignoreNvsError)
{
	SYSP_PGRP_MAGPAR *mpGrp;
	int rv;
	
	/* attempt to init parameter group, read NVS */
	rv = SYSP_MpGrpCreateProd( nvs, "prod2", offset, sizeof(EEPROD2),
							 SYSP_parListEeprod2, EEID_PD2,
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
		SYSP_ALLOC_UNDO( sizeof(*mpGrp) + sizeof(EEPROD2), nvs->super );

	return rv;
}

/**********************************************************************/
/** converts raw date to string
 * returns SYSPARAM_INVAL_VALUE and empty string if raw is 0xffff
 */
static int Prod2FromRawDate( char *dest, int destLen, void *src )
{
	u_int16 rawDate = SYSP_FROMBE16( *(u_int16 *)src );
	char buf[12];

	if( rawDate == 0xffff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}

	SYSP_SNPRINTF( buf, sizeof(buf), "%02d/%02d/%04d",
				  (rawDate & 0x01e0) >> 5,
				  (rawDate & 0x001f),
				  ((rawDate & 0xfe00) >> 9) + EEPROD2_DATE_YEAR_BIAS );

	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** converts string to raw date value
 *
 * String format must be in US notation \c m/d/y
 *
 * Where \c m is the month 1..12 (leading zeroes allowed)
 * Where \c d is the day of month 1..31 (leading zeroes allowed)
 * Where \c y is the year. If no century included, 2000 is added to year.
 *
 * An empty \a src string sets the raw date to 0xffff (invalid)
 *
 * Includes the validation function
 * returns SYSPARAM_INVAL_VALUE if invalid date
 */
static int Prod2ToRawDate( void *dest, const char *src )
{
	const char *s = src;
	u_int16 rawDate;
	int32 m,d,y;

	if( *src == '\0' ){
		/* clear date */
		*(u_int16 *)dest = SYSP_TOBE16( 0xffff );
		return 0;
	}

	/* get month */
	m = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (m < 1) || (m > 12) || (*s++ != '/'))
		goto ERREXIT;

	/* get day */
	d = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (d < 1) || (d > 31) || (*s++ != '/'))
		goto ERREXIT;

	/* get year */
	y = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (y < 100) && (y > 0 )){
		y += (2000 - EEPROD2_DATE_YEAR_BIAS);
	}
	else {
		if( (y < EEPROD2_DATE_YEAR_BIAS) ||
			(y > (EEPROD2_DATE_YEAR_BIAS + 127)))
		goto ERREXIT;
		y -= EEPROD2_DATE_YEAR_BIAS;
	}

	/*printf("toRawDate: %ld %ld %ld\n", m, d, y );*/
	rawDate = ((y & 0x7f) << 9) |
		((m & 0xf) << 5) |
		(d & 0x1f);

	*(u_int16 *)dest = SYSP_TOBE16( rawDate );
	return 0;
 ERREXIT:
	return SYSPARAM_INVAL_VALUE;
}


/**********************************************************************/
/** as SYSP_FromRawU8D, but returns SYSPARAM_INVAL_VALUE if raw is 0xff */
static int Prod2FromRawU8D( char *dest, int destLen, void *src )
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
static int Prod2FromRawU32D( char *dest, int destLen, void *src )
{
	char buf[12];

	if( *(u_int32 *)src == 0xffffffff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}
	SYSP_SNPRINTF( buf, sizeof(buf), "%lu", SYSP_FROMBE32(*(u_int32 *)src));
	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/*--------------- UNIT-TEST -----------------------------------------*/

#ifdef SYSP_CONFIG_UNITTEST

SYSP_UNITTEST( "SYSPARAM2: date conversion functions", SYSP_UtProd2Date );

#define _MKDATE( m,d,y ) (((y & 0x7f) << 9) | ((m & 0xf) << 5) | (d & 0x1f))

/** Checks date conversion functions */
int SYSP_UtProd2Date( void )
{
	/* Prod2FromRawDate */
	{
		u_int16 raw;
		char buf[12];

		SYSP_MEMSET( buf, 0xaa, sizeof(buf));

		/* invalid content */
		raw = SYSP_TOBE16( 0xffff );
		UTASSERT( Prod2FromRawDate( buf, sizeof(buf), &raw )
				  == SYSPARAM_INVAL_VALUE );
		UTASSERT( SYSP_STRCMP( "", buf ) == 0 );

		/* good content */
		raw = SYSP_TOBE16( _MKDATE(1,2,3) );
		UTASSERT( Prod2FromRawDate( buf, sizeof(buf), &raw ) == 0 );
		UTASSERT( SYSP_STRCMP( "01/02/1993", buf ) == 0 );

		raw = SYSP_TOBE16( _MKDATE(12,21,33) );
		UTASSERT( Prod2FromRawDate( buf, sizeof(buf), &raw ) == 0 );
		UTASSERT( SYSP_STRCMP( "12/21/2023", buf ) == 0 );
	}

	/* Prod2ToRawDate */
	{
		u_int16 raw=0xaaaa;

		/* good cases */
		UTASSERT( Prod2ToRawDate( &raw, "1/2/1990" ) == 0 );
		UTASSERT( SYSP_FROMBE16( raw ) == _MKDATE(1,2,0) );

		UTASSERT( Prod2ToRawDate( &raw, "03/31/2004" ) == 0 );
		UTASSERT( SYSP_FROMBE16( raw ) == _MKDATE(3,31,14) );

		UTASSERT( Prod2ToRawDate( &raw, "12/04/04" ) == 0 );
		UTASSERT( SYSP_FROMBE16( raw ) == _MKDATE(12,4,14) );

		UTASSERT( Prod2ToRawDate( &raw, "" ) == 0 );
		UTASSERT( SYSP_FROMBE16( raw ) == 0xffff );

		/* bad cases */
		UTASSERT( Prod2ToRawDate( &raw, "1/2/1989" ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "0/2/1990" ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "13/2/1990" ) ==SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/0/1990" ) == SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/32/1990" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/31/100" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/31/2128" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/31/-1" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/31" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1" )== SYSPARAM_INVAL_VALUE );
		UTASSERT( Prod2ToRawDate( &raw, "1/31a/1990" )==SYSPARAM_INVAL_VALUE );
	}

	DBGWRT_1((DBH,"SYSP_UtProd2Date OK\n"));
	return 0;
 ABORT:
	return -1;
}


#endif /* SYSP_CONFIG_UNITTEST */

