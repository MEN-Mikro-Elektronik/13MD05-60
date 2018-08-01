/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_f302.c
 *
 *      \author  aw
 *        $Date: 2008/07/14 11:24:27 $
 *    $Revision: 2.2 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  f302 specific part
 *               cloned from EM03
 *
 *    \switches  -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_f302.c,v $
 * Revision 2.2  2008/07/14 11:24:27  aw
 * R: nios clock frequency is needed for reset driver
 * M: add niosclkhz at sysparam lib
 *
 * Revision 2.1  2008/06/27 12:01:57  aw
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"



/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

typedef struct {
    /*--- offset 0x0: production data, hardware information ---*/
	EEPROD2 pd;

    /*--- offset 0x18: system data parameters (50 bytes) ---*/
    struct systemDataParms {
        u_int8      id;             /* struct ID (0x5) and parity */
		u_int8      nmac[7];        /* MAC address */
		u_int16     FPGA_variant;   /* variant of FPGA A,B,C,D,... */
		u_int16	   	FPGA_rev;       /* revision of FPGA 0,1,2,3,... */
        u_int16     FPGA_date;      /* build date of FPGA 10/28/2008 */
        u_int16     FPGA_time;      /* build time of FPGA 10:10 */
        u_int16		BSP_version;    /* BSP version */
        u_int32     chameleon_base; /* Address of chameleon table */
        u_int16     PHY_address;    /* Address of PHY */
        u_int32     niosclkhz;      /* Nios clock frequency */
		u_int8	   	reserved[22];
    } systemData;

} F302_FLASH;

#define SYSP_PDE_FPGA_DATE( parName, aliasName, offset, _deflt ) \
    { parName, aliasName, offset, ToRawDate, FromRawDate, NULL, _deflt, 0, 0 }
    
#define SYSP_PDE_FPGA_TIME( parName, aliasName, offset, _deflt ) \
    { parName, aliasName, offset, ToRawFpgaTime, FromRawFpgaTime, NULL, _deflt, 0, 0 }
   
#define SYSP_PDE_BSP_VER( parName, aliasName, offset, _deflt ) \
    { parName, aliasName, offset, ToRawBspVer, FromRawBspVer, NULL, _deflt, 0, 0 }


/* get address of structure component */
#ifndef offsetof
	#define offsetof(type, part)	((int)((char*)&((type*)0)->part-(char*)0))
#endif

#define _EEOFF(x) offsetof(struct systemDataParms,x)

#define F302_FLASH_MAGIC_ID     0x5
#define NVS_SECTION_SIZE        0x2000   /* size of NVS section, 
                                            max. size of first flash section */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static SYSP_MMSMB_SUPER G_super; 		/**< SYSPARAM super object  */
static u_int32 G_alloc[1100];			/**< allocation heap  */


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int SYSP_NvsFlashCreate( SYSP_MMSMB_SUPER * );

/**********************************************************************/
/** converts raw date to string
 *
 *  \param dest      \INOUT	 pointer to destination
 *  \param destLen   \IN     length of destination buffer
 *  \param src       \IN     pointer to source
 *
 *  \return SYSPARAM_INVAL_VALUE and empty string if raw is 0xffff
 */
int FromRawDate( char *dest, int destLen, void *src )
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
 * 
 * \param dest      \INOUT	 pointer to destination
 * \param src       \IN      pointer to source
 *
 * \return SYSPARAM_INVAL_VALUE if invalid date
 */
int ToRawDate( void *dest, const char *src )
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

	rawDate = ((y & 0x7f) << 9) |
		((m & 0xf) << 5) |
		(d & 0x1f);

	*(u_int16 *)dest = SYSP_TOBE16( rawDate );
	return 0;
 ERREXIT:
	return SYSPARAM_INVAL_VALUE;
}

/**********************************************************************/
/** converts raw time to string
 *
 *  \param dest      \INOUT	 pointer to destination
 *  \param destLen   \IN     length of destination buffer
 *  \param src       \IN     pointer to source
 *
 *  \return SYSPARAM_INVAL_VALUE and empty string if raw is 0xffff
 */
int FromRawFpgaTime( char *dest, int destLen, void *src )
{
	u_int16 rawDate = SYSP_FROMBE16( *(u_int16 *)src );
	char buf[12];

	if( rawDate == 0xffff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}

	SYSP_SNPRINTF( buf, sizeof(buf), "%02d:%02d",
				  (rawDate & 0xff00) >> 8,
				  (rawDate & 0x00ff) );

	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** converts string to raw time value
 *
 * String format must be in notation \c h:m
 *
 * Where \c h is the hour 0..23
 * Where \c m is the minute 0..59
 *
 * An empty \a src string sets the raw time to 0xffff (invalid)
 *
 * Includes the validation function
 * 
 * \param dest      \INOUT	 pointer to destination
 * \param src       \IN      pointer to source
 *
 * \return SYSPARAM_INVAL_VALUE if invalid time
 */
int ToRawFpgaTime( void *dest, const char *src )
{
	const char *s = src;
	u_int16 rawDate;
	int32 h,m;
	
	if( *src == '\0' ){
		/* clear time */
		*(u_int16 *)dest = SYSP_TOBE16( 0xffff );
		return 0;
	}

	/* get Clock */
	h = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (h < 1) )
		goto ERREXIT;
		
	s++;
		
	m = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (m < 1) )
		goto ERREXIT;
		
	rawDate = ((h & 0xff) << 8)|(m & 0xff);

	*(u_int16 *)dest = SYSP_TOBE16( rawDate );
	
	return 0;
 ERREXIT:
	return SYSPARAM_INVAL_VALUE;
}

/**********************************************************************/
/** converts raw BSP version to string
 *
 *  \param dest      \INOUT	 pointer to destination
 *  \param destLen   \IN     length of destination buffer
 *  \param src       \IN     pointer to source
 *
 *  \return SYSPARAM_INVAL_VALUE and empty string if raw is 0xffff
 */
int FromRawBspVer( char *dest, int destLen, void *src )
{
	u_int16 rawVer = SYSP_FROMBE16( *(u_int16 *)src );
	char buf[12];

	if( rawVer == 0xffff ){
		*dest = '\0';
		return SYSPARAM_INVAL_VALUE;
	}

	SYSP_SNPRINTF( buf, sizeof(buf), "%02d.%02d",
				  (rawVer & 0xff00) >> 8,
				  (rawVer & 0x00ff) );

	return SYSP_StrSafeCpy( dest, buf, destLen );
}

/**********************************************************************/
/** converts string to raw BSP version value
 *
 * String format must be in notation \c mainNbr:smallNbr
 *
 * An empty \a src string sets the raw time to 0xffff (invalid)
 *
 * Includes the validation function
 * 
 * \param dest      \INOUT	 pointer to destination
 * \param src       \IN      pointer to source
 *
 * \return SYSPARAM_INVAL_VALUE if invalid time
 */
int ToRawBspVer( void *dest, const char *src )
{
	const char *s = src;
	u_int16 rawVer;
	int32 mainNbr, smallNbr;
	
	if( *src == '\0' ){
		/* clear time */
		*(u_int16 *)dest = SYSP_TOBE16( 0xffff );
		return 0;
	}

	mainNbr = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (mainNbr < 0) )
		goto ERREXIT;
		
	s++;
		
	smallNbr = SYSP_STRTOUL( s, (char **)&s, 10 );
	if( (smallNbr < 0) )
		goto ERREXIT;
		
	rawVer = ((mainNbr & 0xff) << 8)|(smallNbr & 0xff);

	*(u_int16 *)dest = SYSP_TOBE16( rawVer );
	
	return 0;
 ERREXIT:
	return SYSPARAM_INVAL_VALUE;
}


/**********************************************************************/
/** Initialize/Reinitialize SYSPARAM for f302
 *
 * Probes for CPU flash
 * \return sysparam error code if fatal error
 */
static int F302Init( SYSP_SUPER *super )
{
	int rv = 0;

	SYSP_MMSMB_SUPER *xSuper = (SYSP_MMSMB_SUPER *)super;

	SYSP_ALLOC_INIT( super, G_alloc );	
	SYSP_SuperMmsmbInit( xSuper );	

	/* create NVS object and group sections for Flash */
	if( (rv = SYSP_NvsFlashCreate( xSuper )) != 0 )
		return rv;				/* fatal */

	return rv;
}

/**********************************************************************/
/** Initialize SYSPARAM for f302
 *
 * \param initBlk 		\IN SYSPARAM initialisation structure for
 *							MENMON/SMB implementations. Caller can destroy
 *							\a initBlk after this call has finished
 *
 * \return sysparam error code if fatal error
 */
int F302_SysParamInit( const SYSPARAM_MMSMB_INIT *initBlk )
{
	SYSP_SUPER *super;
	SYSP_MMSMB_SUPER *xSuper;

	xSuper = &G_super;
	SYSP_super = super = &xSuper->s;

	xSuper->initBlk = *initBlk;

	super->reInit = F302Init;

	SYSP_SuperMmsmbInit( xSuper );
    
	return super->reInit( super );
	
}

/** list of parameters in F302 CPU Flash, system data section */
static const SYSP_PAR_DESC SYSP_parListF302SystemData[] = {
	SYSP_PDE_ETH      ( SYSP_nmac0,       NULL, _EEOFF(nmac),           "00C03A800000" ),
	SYSP_PDE_U16X     ( "FPGA_variant",   NULL, _EEOFF(FPGA_variant),   NULL, 0 ),
	SYSP_PDE_U16X     ( "FPGA_rev", 	  NULL, _EEOFF(FPGA_rev),       NULL, 0 ),
	SYSP_PDE_FPGA_DATE( "FPGA_date", 	  NULL, _EEOFF(FPGA_date),      NULL    ),
	SYSP_PDE_FPGA_TIME( "FPGA_time",      NULL, _EEOFF(FPGA_time),      NULL    ),
	SYSP_PDE_BSP_VER  ( "BSP_version",    NULL, _EEOFF(BSP_version),    "0.0"   ),
	SYSP_PDE_U32X     ( "chameleon_base", NULL, _EEOFF(chameleon_base), NULL, 0 ),
	SYSP_PDE_U16X     ( "PHY_address0",   NULL, _EEOFF(PHY_address),    "0",  0 ),
	SYSP_PDE_U32X     ( "niosclkhz",      NULL, _EEOFF(niosclkhz), NULL, 0 ),
	SYSP_PDE_END
};

/**********************************************************************/
/** Create NVS object and all parameter sections that shall be present 
 *  in F302 Flash
 *
 * \return sysparam error code if fatal error
 */
static int SYSP_NvsFlashCreate( SYSP_MMSMB_SUPER *xSuper )
{
	int rv;
	SYSP_NV_STORAGE *nvsFlash;
	SYSP_NVS_ADR nvsAdr;

	/* create NVS section */
	nvsAdr.type = SYSP_NVSADR_SMB;
	nvsAdr.addr.smb.bus = 0;
	nvsAdr.addr.smb.dev = 0;
	

	if( (nvsFlash = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE), 
	                                "eeprod2",
								    &xSuper->s, 
								    "", 
								    nvsAdr,
								    NVS_SECTION_SIZE )) 
        == NULL )
    {
		return SYSPARAM_NO_RESOURCE;
	}	

	/* CPU EEPROD section */
	if( (rv = SYSP_Eeprod2GrpCreate( nvsFlash, NULL, TRUE )) != 0)
	{
		return rv;
	}

	/* system data section */
	if( (rv = SYSP_MpGrpCreateStd( nvsFlash, 
	                               "systemData", 
	                               offsetof( F302_FLASH, systemData),
								   sizeof( struct systemDataParms ),
								   SYSP_parListF302SystemData, 
								   F302_FLASH_MAGIC_ID,
								   NULL )) 
        != 0 )
    {
		return rv;
	}

    SYSP_NvsAdd( &xSuper->s, nvsFlash );
	
	return 0;
}


