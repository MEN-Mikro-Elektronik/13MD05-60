/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  sysp_vxbline.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2008/06/11 17:27:24 $
 *    $Revision: 1.12 $
 *
 *	   \project  SYSPARAM library
 *  	 \brief  Parameter group handler for VxWorks bootline parameters
 *
 *
 *    \switches  HURRICANE
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_vxbline.c,v $
 * Revision 1.12  2008/06/11 17:27:24  rt
 * R:1. Print a note that user has to reset board after changing an ee-X
 *      parameter if needed.
 * M:1. Set SYSP_PD_NEEDRESET flag for some ee-X parameters.
 *
 * Revision 1.11  2005/06/23 15:54:36  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.10  2005/04/12 16:53:14  kp
 * Fixed string length check in SYSP_ToRawVxblSm() SYSP_ToRawVxblIp()
 *
 * Revision 1.9  2005/02/22 15:01:09  kp
 * avoid OS-9 compiler warnings
 *
 * Revision 1.8  2005/01/18 15:49:37  kp
 * added size to NVS sections
 *
 * Revision 1.7  2005/01/12 14:20:02  kp
 * provide separate parameters "netaddr" "netsm"
 *
 * Revision 1.6  2004/12/23 09:32:58  ufranke
 * cosmetics
 *
 * Revision 1.5  2004/12/20 08:58:03  kp
 * allow caller to select wether "kerpar" parameter is in VxBline or
 * or somewhere else
 *
 * Revision 1.4  2004/11/29 10:53:26  kp
 * MM unittest support
 *
 * Revision 1.3  2004/10/07 09:17:42  kp
 * increased max. length of startup script length from 80 to 256 chars
 *
 * Revision 1.2  2004/09/03 15:20:52  kp
 * added HURRICAN switch (no MMglobs)
 *
 * Revision 1.1  2004/01/08 09:06:59  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#include "sysp_intern.h"


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define BOOT_DEV_LEN			20	/* max chars in device name */
#define BOOT_HOST_LEN			20	/* max chars in host name */
#define BOOT_ADDR_LEN			30	/* max chars in net addr */
#define BOOT_TARGET_ADDR_LEN    50  /* IP address + mask + lease times */
#define BOOT_ADDR_LEN			30	/* max chars in net addr */
#define BOOT_FILE_LEN			80	/* max chars in file name */
#define BOOT_USR_LEN			20	/* max chars in user name */
#define BOOT_STARTUP_LEN	   256	/* max chars in startup script */
#define BOOT_PASSWORD_LEN		20	/* max chars in password */
#define BOOT_OTHER_LEN			80	/* max chars in "other" field */

#define BOOT_FIELD_LEN		    80	/* max chars in boot field */
#define BOOT_NUM_LEN		    10  /* max chars in numeric field */


/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

/** structure to convert boot line */
typedef struct				/* BOOT_PARAMS */
{
    char bootDev [BOOT_DEV_LEN];	/* boot device code */
    char hostName [BOOT_HOST_LEN];	/* name of host */
    char targetName [BOOT_HOST_LEN];	/* name of target */
    char ead [BOOT_TARGET_ADDR_LEN];	/* ethernet internet addr */
    char bad [BOOT_TARGET_ADDR_LEN];	/* backplane internet addr */
    char had [BOOT_ADDR_LEN];		/* host internet addr */
    char gad [BOOT_ADDR_LEN];		/* gateway internet addr */
    char bootFile [BOOT_FILE_LEN];	/* name of boot file */
    char startupScript [BOOT_STARTUP_LEN];	/* name of startup script file */
    char usr [BOOT_USR_LEN];	/* user name */
    char passwd [BOOT_PASSWORD_LEN]; /* password */
    char other [BOOT_OTHER_LEN];	/* available for applications */
    unsigned int procNum;				/* processor number */
    unsigned int flags;					/* configuration flags */
    unsigned int unitNum;				/* network device unit number */
} VXBL_BOOT_PARAMS;


typedef struct {
	SYSP_PAR_GRP parGrp;		/**< common parameter group object  */
	VXBL_BOOT_PARAMS bpStruct;	/**< boot parameter structure  */
	const char *vxDefBline;		/**< boot line with defaults  */
	char *vxBline;				/**< current boot line */
	int size;					/**< size of NV area  */
} SYSP_PGRP_VX;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
#define _BPOFF(x) offsetof(VXBL_BOOT_PARAMS,x)

#define SYSP_PDE_VXBLIP( _nm, _alias, _offs ) \
 { _nm, _alias, _offs, SYSP_ToRawVxblIp, SYSP_FromRawVxblIp, NULL,\
   NULL, 0, (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET)}

#define SYSP_PDE_VXBLSM( _nm, _alias, _offs ) \
 { _nm, _alias, _offs, SYSP_ToRawVxblSm, SYSP_FromRawVxblSm, NULL,\
   NULL, 0, (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET)}

static char G_kerparAlias[7];

/* parameter descriptors, offsets in VXBL_BOOT_PARAMS */
const SYSP_PAR_DESC SYSP_parListVx[] = {
	SYSP_PDE_STR( "bootdev",	NULL, _BPOFF(bootDev), NULL,  BOOT_DEV_LEN, 0),
	SYSP_PDE_NINTD( "unitnum", 	NULL, _BPOFF(unitNum), NULL, 0),
	SYSP_PDE_NINTD( "procnum", 	NULL, _BPOFF(procNum), NULL, 0),
	SYSP_PDE_STR( "hostname",	NULL, _BPOFF(hostName), NULL,  BOOT_HOST_LEN
				  , 0),
	SYSP_PDE_STR( "e", SYSP_netip, _BPOFF(ead),NULL, BOOT_TARGET_ADDR_LEN, SYSP_PD_NEEDRESET),
	SYSP_PDE_STR( "h", SYSP_nethost, _BPOFF(had), NULL, BOOT_ADDR_LEN, SYSP_PD_NEEDRESET),
	SYSP_PDE_STR( "g", SYSP_netgw, _BPOFF(gad), NULL, BOOT_ADDR_LEN, SYSP_PD_NEEDRESET),
	SYSP_PDE_STR( "tn",SYSP_netname,_BPOFF(targetName), NULL, BOOT_HOST_LEN,0),
	SYSP_PDE_STR( "s",	G_kerparAlias, _BPOFF(startupScript), NULL,
				  BOOT_STARTUP_LEN, 0),
	SYSP_PDE_STR( SYSP_bf, SYSP_bootfile, _BPOFF(bootFile), NULL,
				  BOOT_FILE_LEN, 0),
	SYSP_PDE_STR( "b",	NULL, _BPOFF(bad), NULL, BOOT_TARGET_ADDR_LEN, 0),
	SYSP_PDE_STR( "u", NULL, _BPOFF(usr), NULL, BOOT_USR_LEN, 0),
	SYSP_PDE_STR( "p", NULL, _BPOFF(passwd), NULL, BOOT_PASSWORD_LEN, 0),
	SYSP_PDE_STR( "o", NULL, _BPOFF(other), NULL, BOOT_OTHER_LEN, 0),
	SYSP_PDE_NINTX( "f", NULL, _BPOFF(flags), NULL, 0),

	/* alternate parameters to access IP and subnet mask individually */
	SYSP_PDE_VXBLIP( SYSP_netaddr, NULL, _BPOFF(ead) ),
	SYSP_PDE_VXBLSM( SYSP_netsm, NULL, _BPOFF(ead) ),

	SYSP_PDE_END
};

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
char *SYSP_VxBootStringToStruct (
	char *bootString,
	VXBL_BOOT_PARAMS *pBootParams);
char *SYSP_VxBootStructToString (
	char *paramString,
	VXBL_BOOT_PARAMS *pBootParams);
static int VxBlValidate( const char *parVal );

/**********************************************************************/
/** Get num parameters in mpgrp
 * \sa SYSP_PAR_GRP.numParams
 */
int SYSP_VxGrpNumParams( SYSP_PAR_GRP *parGrp )
{
	return (sizeof(SYSP_parListVx)/sizeof(SYSP_PAR_DESC)) - 1;
}

/**********************************************************************/
/** Enum vxgrp params
 * \sa SYSP_PAR_GRP.enumParams
 */
int SYSP_VxGrpEnumParams(
	SYSP_PAR_GRP *parGrp,
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{

	const SYSP_PAR_DESC *parDesc;

	if( idx >= parGrp->numParams( parGrp ) )
		return SYSPARAM_INVAL_VALUE;

	parDesc = &SYSP_parListVx[idx];

	SYSP_StrSafeCpy( info->parName,
					 (alias && parDesc->aliasName) ?
					 parDesc->aliasName :
					 parDesc->parName,
					 sizeof(info->parName));
	info->attr = (SYSPARAM_ACC_ATTR)(SYSPARAM_ACC_RW |SYSPARAM_ACC_PERSISTANT)
		| ((parDesc->pdFlags & SYSP_PD_NEEDRESET) ? SYSPARAM_ACC_NEEDRESET : 0);

	return 0;
}

/**********************************************************************/
/** Set vxgrp parameter
 *
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_VxGrpSetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_VX *vxGrp = (SYSP_PGRP_VX *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;


	/* find parameter descriptor */
	if( (parDesc = SYSP_FindParDesc( SYSP_parListVx, parName )) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return SYSPARAM_INVAL_VALUE;

	/* validate value */
	if( (rv = VxBlValidate( parVal )) != 0 )
		return rv;

	if( parDesc->validate ){
		if( (rv = parDesc->validate( parVal, parDesc->vParam )) != 0 )
			return rv;
	}

	/* convert current bootline to struct */
	SYSP_VxBootStringToStruct( vxGrp->vxBline, &vxGrp->bpStruct );

	/* set new value */
	rv = parDesc->toRaw( ((u_int8 *)&vxGrp->bpStruct) + parDesc->offset,
						 parVal );

	/* re-encode the structure */
	{
		char *end;
		end = SYSP_VxBootStructToString( vxGrp->vxBline, &vxGrp->bpStruct );

		if( end - vxGrp->vxBline > vxGrp->size-1 ){
			rv = SYSPARAM_VAL_TRUNCATED;
			vxGrp->vxBline[vxGrp->size-1] = '\0';
		}
	}

	return rv;
}


/**********************************************************************/
/** Get vxgrp parameter
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_VxGrpGetParam(
	SYSP_PAR_GRP *parGrp,
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_VX *vxGrp = (SYSP_PGRP_VX *)parGrp;
	const SYSP_PAR_DESC *parDesc;
	int rv=0;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	/* find parameter descriptor */
	if( (parDesc = SYSP_FindParDesc( SYSP_parListVx, parName )) == NULL )
		return SYSPARAM_INVAL_PARAM; /* unknown parameter */

	if( parVal == NULL )
		return 0;

	/* convert current bootline to struct */
	SYSP_VxBootStringToStruct( vxGrp->vxBline, &vxGrp->bpStruct );

	rv = parDesc->fromRaw( parVal, parValMaxLen,
						   ((u_int8 *)&vxGrp->bpStruct) + parDesc->offset);

	return rv;
}

/**********************************************************************/
/** Set vxgrp to defaults
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_VxGrpSetDefaults(
	SYSP_PAR_GRP *parGrp,
	int force)
{
	SYSP_PGRP_VX *vxGrp = (SYSP_PGRP_VX *)parGrp;

	SYSP_STRCPY( vxGrp->vxBline, vxGrp->vxDefBline );

	return 0;
}

/**********************************************************************/
/** Save vxgrp
 * \sa SYSP_PAR_GRP.save
 */
int SYSP_VxGrpSave( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_VX *vxGrp = (SYSP_PGRP_VX *)parGrp;
	int rv;

	vxGrp->vxBline[vxGrp->size-1] = '\0';

	rv = parGrp->nvs->writeNvs( parGrp->nvs, parGrp->nvsOffset,
								SYSP_STRLEN(vxGrp->vxBline)+1,
								(u_int8 *)vxGrp->vxBline );
	return rv;
}


/**********************************************************************/
/** Check if bootline contains invalid chars
 */
static int VxBlValidate( const char *parVal )
{
	while( *parVal ){
		if( !SYSP_PRINTABLE( *parVal ) )
			return SYSPARAM_INVAL_VALUE;
		parVal++;
	}
	return 0;
}

/**********************************************************************/
/** Create a parameter group object for VxWorks bootline parameters
 *
 * On success, the parameter group is added to the list of \a nvs groups
 *
 * If the passed \a vxBline is an empty string, the vxBline is read from
 * the non volatile storage. If there is a physical read error, or if
 * it contains non-printable chars, default bootline is setup.
 *
 * If the passed \a vxBline is not-empty, it is checked for non-printable
 * chars, and defaults are applied if so.
 *
 * \param nvs		\IN NV storage object (fully initialized)
 * \param nvsOffset	\IN starting byte offset of vxbline within NV storage
 * \param nvsSize	\IN size of vxbline in NV storage in bytes
 * \param vxBline	\IN the vxworks boot line in memory
 * \param vxDefBline \IN VxWorks boot line with defaults, if NULL,
 *						 an empty bootline will be the default
 * \param provideKerpar \IN if SYSP_PROVIDE_KERPAR, let "kerpar" be
 *                          an alias to "s"
 *
 * \return sysparam error code
 *
 */
int SYSP_VxGrpCreate(
	SYSP_NV_STORAGE *nvs,
	int nvsOffset,
	int nvsSize,
	char *vxBline,
	const char *vxDefBline,
	int provideKerpar)
{
	SYSP_PGRP_VX *vxGrp = NULL;
	int grpSize = sizeof(*vxGrp);
	int rv=0;

	/* alloc parameter group and raw data shadow */
	if( (vxGrp = SYSP_ALLOC( grpSize, nvs->super )) == NULL )
		return SYSPARAM_NO_RESOURCE;


	if( vxDefBline == NULL )
		vxDefBline = "";

	if( provideKerpar == SYSP_PROVIDE_KERPAR )
		SYSP_STRCPY( G_kerparAlias, "kerpar" );
	else
		G_kerparAlias[0] = '\0';

	/* init object */

	vxGrp->parGrp.grpName		= "vxbline";
	vxGrp->parGrp.nvs 			= nvs;
	vxGrp->parGrp.nvsOffset	 	= nvsOffset;
	vxGrp->parGrp.numParams		= SYSP_VxGrpNumParams;
	vxGrp->parGrp.enumParams	= SYSP_VxGrpEnumParams;
	vxGrp->parGrp.setParam		= SYSP_VxGrpSetParam;
	vxGrp->parGrp.getParam		= SYSP_VxGrpGetParam;
	vxGrp->parGrp.setDefaults	= SYSP_VxGrpSetDefaults;
	vxGrp->parGrp.save			= SYSP_VxGrpSave;
	vxGrp->vxBline				= vxBline;
	vxGrp->vxDefBline			= vxDefBline;
	vxGrp->size					= nvsSize;

	/* attempt to read raw data */
	if( vxBline[0] == '\0' )
		rv = nvs->readNvs( nvs, nvsOffset, nvsSize, (u_int8 *)vxBline );

	/* make sure it's null terminated */
	vxBline[nvsSize-1] = '\0';

	if( (rv != 0) || VxBlValidate( vxBline )){
		SYSP_STRCPY( vxBline, vxDefBline );
		rv = 0;
	}

	SYSP_ParGrpAdd( nvs, &vxGrp->parGrp );

	if( rv != 0 )
		SYSP_ALLOC_UNDO( grpSize, nvs->super );
	return rv;
}

/**********************************************************************/
static int ScanStringTil( char **bootStringP, char *dest, int destLen,
						  char term)
{
	char *p = *bootStringP;
	int len=0;

	while( *p && (*p != term) && (len<destLen-1) )
		*dest++ = *p++;
	*dest = '\0';

	*bootStringP = p;
	return *p==term ? 1 : 0;
}

/**********************************************************************/
static char *EatBlanks( char *p )
{
	if( p )
		while( *p && (*p==' ' || *p=='\t') )
			p++;

	return p;
}

/**********************************************************************/
static char *GetBase( int base, char *s, u_int32 *val )
{
	char *next;

	*val = SYSP_STRTOUL( s, &next, base );

	return next;
}

/**********************************************************************/
/** Parse VxBline string and return parsed parameters in structure
 *
 * \param bootString	\IN	ptr to line to be parsed
 * \param pBootParams	\OUT ptr to struct to be filled
 * \return A pointer to the last character successfully parsed plus one
 */
char *SYSP_VxBootStringToStruct (
	char *bootString,
	VXBL_BOOT_PARAMS *pBootParams)
{
	char buf[BOOT_FIELD_LEN];
	u_int32 val;
	char *str;

	SYSP_MEMSET( pBootParams, 0, sizeof(pBootParams) );

	/*--- scan boot device ---*/
	bootString = EatBlanks( bootString );
	ScanStringTil( &bootString, pBootParams->bootDev, BOOT_DEV_LEN, '(' );


	if( *bootString == '(' ){
		bootString++;
		/*--- scan unitnum ---*/
		bootString = EatBlanks( bootString );
		bootString = GetBase( 10, bootString, &val );
		if( *bootString == ',' || *bootString == ')' ){
			pBootParams->unitNum = val;
		}
		else
			pBootParams->unitNum = 0;
	}

	if( *bootString == ',' ){
		/*--- scan procnum ---*/
		bootString++;
		bootString = EatBlanks( bootString );
		bootString = GetBase( 10, bootString, &val );
		if( *bootString == ')' ){
			pBootParams->procNum = val;
			bootString++;
		}
		else
			pBootParams->procNum = 0;
	}
	else if( *bootString == ')' )
		bootString++;

	/*--- scan hostname ---*/
	str = EatBlanks( bootString );

	if( ScanStringTil( &str, buf, BOOT_HOST_LEN, ':' ) == 1 ){
		SYSP_STRNCPY( pBootParams->hostName, buf, 16 );
		bootString = str+1;

		/*--- scan filename ---*/
		if( *bootString && *bootString != ' ' ){
			ScanStringTil( &bootString, buf, BOOT_FILE_LEN, ' ' );
			if( *bootString == ' ' || *bootString == '\0' )
				SYSP_STRCPY( pBootParams->bootFile, buf );
		}
	}


	/*------------------+
	|  Scan tags xxx=   |
	+------------------*/
	bootString = EatBlanks( bootString );

	while( ScanStringTil( &bootString, buf, sizeof(buf), '=' ) == 1 ){
		char *dest = NULL;
		int destLen = 0;

		bootString++;

		if( strcmp( buf, "e" ) == 0 ){
			dest = pBootParams->ead;
			destLen = BOOT_TARGET_ADDR_LEN;
		}
		else if( strcmp( buf, "b" ) == 0 ){
			dest = pBootParams->bad;
			destLen = BOOT_TARGET_ADDR_LEN;
		}
		else if( strcmp( buf, "h" ) == 0 )	{
			dest = pBootParams->had;
			destLen = BOOT_ADDR_LEN;
		}
		else if( strcmp( buf, "g" ) == 0 ){
			dest = pBootParams->gad;
			destLen = BOOT_ADDR_LEN;
		}
		else if( strcmp( buf, "u" ) == 0 ) {
			dest = pBootParams->usr;
			destLen = BOOT_USR_LEN;
		}
		else if( strcmp( buf, "pw" ) == 0 ) {
			dest = pBootParams->passwd;
			destLen = BOOT_PASSWORD_LEN;
		}
		else if( strcmp( buf, "tn" ) == 0 ) {
			dest = pBootParams->targetName;
			destLen = BOOT_HOST_LEN;
		}
		else if( strcmp( buf, "s" ) == 0 ) {
			int nestCnt=0;

			dest = pBootParams->startupScript;
			destLen = BOOT_STARTUP_LEN;

			/*
			 * Handle special case for linux kernel parameters:
			 *
			 * The string can be formed like this:
			 * s='console=ttyS0,9600 ip=auto'
			 *
			 * The apostrophe characters are not copied into the destination.
			 */
			while( destLen-1 > 0 ){
				if( *bootString == '\'' ){
					nestCnt ^= 1;
				}
				else if( *bootString=='\0' ||(*bootString==' ' && nestCnt==0)){
					*dest = '\0';
					break;

				}
				else {
					*dest++ = *bootString;
					destLen--;
				}

				++bootString;
			}
			*dest = '\0';
			destLen = -1;
		}
		else if( strcmp( buf, "o" ) == 0 ) {
			dest = pBootParams->other;
			destLen = BOOT_OTHER_LEN;
		}
		else if( strcmp( buf, "f" ) == 0 ) {
			destLen = -1;
			bootString += 2;
			bootString = GetBase( 16, bootString, &val );
			if( *bootString == ' ' || *bootString == '\0' )
				pBootParams->flags = val;
			else
				pBootParams->flags = 0;
		}

		if( destLen != -1 && dest != NULL ){
			ScanStringTil( &bootString, buf, destLen, ' ' );
			if( *bootString == ' ' || *bootString == '\0' )
				SYSP_STRCPY( dest, buf );
		}

		bootString = EatBlanks( bootString );
	}

	return bootString;
}


/**********************************************************************/
/** Convert struct to vxBline
 *
 * Note: no overflow checking of the string is made!
 * Worst case, the string can have approx. 656 chars.
 *
 * \param paramString	\OUT ptr to line to be filled
 * \param pBootParams	\IN  ptr to struct to be encoded
 * \return pointer into bootString to terminating '\\0'
 */
char *SYSP_VxBootStructToString (
	char *paramString,
	VXBL_BOOT_PARAMS *pBootParams)
{

	if ( pBootParams == NULL )
		return NULL;

	if ( ( pBootParams->hostName == NULL ) && ( pBootParams->bootFile == NULL ) ) {
		paramString += SYSP_SNPRINTF( paramString, 2, ":" );
	} else {
	paramString += SYSP_SNPRINTF( paramString, 256, "%s%s%s",
							pBootParams->hostName,
							(*pBootParams->hostName || *pBootParams->bootFile)
							? ":" : "",
							pBootParams->bootFile);
	}

	paramString += SYSP_SNPRINTF( paramString, 256, "%s(%u,%u)",
							pBootParams->bootDev,
							pBootParams->unitNum,
							pBootParams->procNum);

	if( pBootParams->ead[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " e=%s", pBootParams->ead );
	if( pBootParams->bad[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " b=%s", pBootParams->bad );
	if( pBootParams->had[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " h=%s", pBootParams->had );
	if( pBootParams->gad[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " g=%s", pBootParams->gad );
	if( pBootParams->usr[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " u=%s", pBootParams->usr );
	if( pBootParams->passwd[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " pw=%s", pBootParams->passwd );
	if( pBootParams->flags != 0 )
		paramString += SYSP_SNPRINTF( paramString, 32, " f=0x%x", pBootParams->flags );
	if( pBootParams->targetName[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 32, " tn=%s", pBootParams->targetName);

	if( pBootParams->startupScript[0] != '\0' ) {
		if( SYSP_ContainsBlanks( pBootParams->startupScript ) ) {
			paramString += SYSP_SNPRINTF( paramString, 256, " s='%s'", pBootParams->startupScript );
		} else {
			pBootParams->startupScript[BOOT_STARTUP_LEN-1] = '\0';
			paramString += SYSP_SNPRINTF( paramString, 256, " s=%s", pBootParams->startupScript );
		}
	}

	if( pBootParams->other[0] != '\0' )
		paramString += SYSP_SNPRINTF( paramString, 64, " o=%s", pBootParams->other );

	return paramString;
}

/**********************************************************************/
/** Extract IP address from VxBl "e" field
 * Return everything until first ":" or until EOS if no ":"
 */
int SYSP_FromRawVxblIp( char *dest, int destLen, void *src )
{
	int rv = 0;
	const char *end;
	int len;

	if( (end = SYSP_STRCHR( (const char *)src, ':' )) == NULL )
		len = SYSP_STRLEN( (const char *)src );
	else
		len = end - (const char *)src;

	if( len > destLen-1 ){
		len = destLen-1;
		rv = SYSPARAM_VAL_TRUNCATED;
	}

	SYSP_STRNCPY( dest, src, len );
	dest[len] = '\0';
	return rv;
}

/**********************************************************************/
/** Extract subnet mask from VxBl "e" field
 * Return everything after first ":" between next ":" or EOS
 */
int SYSP_FromRawVxblSm( char *dest, int destLen, void *src )
{
	const char *p;

	if( (p = SYSP_STRCHR( (const char *)src, ':' )) == NULL )
		return SYSP_StrSafeCpy( dest, "", destLen ); /* no ":" */

	return SYSP_FromRawVxblIp( dest, destLen, (void *)(p+1) );
}

/**********************************************************************/
/** modify VxBl IP in "e"
 * NOTE: Destroys informatation in ead other than IP and subnetmask (e.g.
 * lease time)
 * \param dest 	pointer into ead field
 * \param src	users IP address
 */
int SYSP_ToRawVxblIp( void *dest, const char *src )
{
	char smBuf[10];
	int totLen;

    /* save away subnet mask */
	SYSP_FromRawVxblSm( smBuf, sizeof(smBuf), dest );

	totLen = SYSP_STRLEN(src);
	if( smBuf[0] != '\0' ){
		totLen += SYSP_STRLEN(smBuf) + 1;
	}
	if( totLen > BOOT_TARGET_ADDR_LEN-1 )
		return SYSPARAM_INVAL_VALUE;

	if( smBuf[0] != '\0' )
		SYSP_SNPRINTF( (char *)dest, 64, "%s:%s", src, smBuf );
	else
		SYSP_SNPRINTF( (char *)dest, 64, "%s", src);

	return 0;
}

/**********************************************************************/
/** modify VxBl SM in "e"
 *
 * NOTE: Destroys informatation in ead other than IP and subnetmask (e.g.
 * lease time)
 * \param dest 	pointer into ead field
 * \param src	users subnet mask
 */
int SYSP_ToRawVxblSm( void *dest, const char *src )
{
	char ipBuf[16];
	int totLen;

    /* save away subnet mask */
	SYSP_FromRawVxblIp( ipBuf, sizeof(ipBuf), dest );

	totLen = SYSP_STRLEN(src) + 1;
	if( ipBuf[0] != '\0' ){
		totLen += SYSP_STRLEN(ipBuf);
	}
	if( totLen > BOOT_TARGET_ADDR_LEN-1 )
		return SYSPARAM_INVAL_VALUE;

	SYSP_SNPRINTF( (char *)dest, 64, "%s:%s", ipBuf, src );

	return 0;
}



/*--------------- MENMON EXTENSION -----------------------------------------*/

#ifdef SYSP_CONFIG_MENMON

static SYSP_PGRP_VX *SYSP_VxGrpFind( SYSP_SUPER *super )
{
	SYSP_NV_STORAGE *nvs;
	SYSP_PAR_GRP *parGrp;

	for( nvs=super->nvsLst; nvs; nvs=nvs->next )
		for( parGrp=nvs->parGrpLst; parGrp; parGrp=parGrp->next )
			if( SYSP_STRCMP( parGrp->grpName, "vxbline" ) == 0 )
				return (SYSP_PGRP_VX *)parGrp;
	return NULL;
}

/**********************************************************************/
/** Interactive VxBline modification in MENMON
 *
 * Display the current value of each parameter and prompt for a new
 * value. Typing a RETURN leaves the parameter unchanged. Typing a
 * period (.) clears the parameter.  ^D exits. A '-' edits the previous
 * field
 *
 * This locates the "vxbline" parameter section by using the global
 * \em SYSP_super pointer.
 *
 * After modification done, modified vxBline is saved to NVS storage,
 * and vxBline string has been updated in memory
 */
void SysParamVxBootParamsPrompt(void)
{
	SYSP_PGRP_VX *vxGrp;
#ifndef HURRICANE
	MMglob *mm = MAIN_MenmonGlobalsP;
#endif
	VXBL_BOOT_PARAMS *params;
	int field = 0, dlen;
	char *line, *str;
	u_int32 val;
	const char *prompt[] = {
		"boot device",			/* 0 */
		"processor number",		/* 1 */
		"host name",			/* 2 */
		"file name",			/* 3 */
		"inet on ethernet (e)",	/* 4 */
		"inet on backplane (b)", /* 5 */
		"host inet (h)",		/* 6 */
		"gateway inet (g)",		/* 7 */
		"user (u)",				/* 8 */
		"ftp password (pw) (blank = use rsh)", /* 9 */
		"flags (f)",			/* 10 */
		"target name (tn)",		/* 11 */
		"startup script (s)",	/* 12 */
		"other (o)"				/* 13 */
	};

	if( (vxGrp = SYSP_VxGrpFind( SYSP_super )) == NULL )
		return;

	params = &vxGrp->bpStruct;

	/*--- convert initial values to struct ---*/
	SYSP_VxBootStringToStruct( vxGrp->vxBline, params );

	mm_printf(mm, "\n%s\n",
			  "'.' = clear field;  '-' = go to previous field;  ^D = quit");

	while( field < 14 ){
		str = NULL;
		dlen = 0;

		/*--- print prompt and initial value ---*/
		mm_printf( mm, "\n%-22s:", prompt[field] );

		switch( field ){
		case 0:
			mm_printf( mm, "%s%d ", params->bootDev, params->unitNum );
			break;
		case 1:
			mm_printf( mm, "%d ", params->procNum );
			break;
		case 2:	str = params->hostName; dlen=BOOT_HOST_LEN; break;
		case 3:	str = params->bootFile; dlen=BOOT_FILE_LEN; break;
		case 4: str = params->ead; dlen=BOOT_TARGET_ADDR_LEN; break;
		case 5: str = params->bad; dlen=BOOT_TARGET_ADDR_LEN; break;
		case 6: str = params->had; dlen=BOOT_ADDR_LEN; break;
		case 7: str = params->gad; dlen=BOOT_ADDR_LEN; break;
		case 8: str = params->usr; dlen=BOOT_USR_LEN; break;
		case 9: str = params->passwd; dlen=BOOT_PASSWORD_LEN; break;
		case 10: mm_printf( mm, "0x%x ", params->flags ); break;
		case 11: str = params->targetName; dlen=BOOT_HOST_LEN; break;
		case 12: str = params->startupScript; dlen=BOOT_STARTUP_LEN; break;
		case 13: str = params->other; dlen=BOOT_OTHER_LEN; break;
		default: dlen=0; break;
		}
		if( str != NULL && *str)
			mm_printf(mm, "%s ", str );

		/*--- ask for new value ---*/
		line = mm_get_line_case_sens( mm, "" );
		line = EatBlanks(line);

		switch( line[0] ){
		case '\0': field++; continue; 					/* RETURN */
		case '-': if( field > 0 ) field--; continue; 	/* - */
		case 0x4: field=1000; continue;					/* CTRL-D */
		case '.': line[0] = '\0'; break;				/* clear field */
		}

		/*--- use new values ---*/
		switch( field ){
		case 0:
		{
			char *p=line;
			int i=0;
			int detectUnitNbr = 1;

			params->unitNum = 0;
			while(*p){
				if( *p == '=' )
					detectUnitNbr = 0;

				if( detectUnitNbr &&
				    *p >= '0' && *p <= '9' )
					params->unitNum = params->unitNum * 10 + (*p - '0');
				else {
					if( i<BOOT_DEV_LEN )
						params->bootDev[i++] = *p;
				}
				p++;
			}
			params->bootDev[i] = '\0';
			break;
		}

		case 1:
			mm_get_base( 10, line, &val );
			params->procNum = val;
			break;
		case 10:
			if( line[0] == '0' && line[1] == 'x' )
				line += 2;
			mm_get_base( 16, line, &val );
			params->flags = val;
			break;
		default:
			if( str != NULL ){
				while( *line && dlen > 1){
					*str++ = *line++;
					dlen--;
				}
				*str = '\0';
			}
			break;
		}
		field++;
	}

	SYSP_VxBootStructToString( vxGrp->vxBline, params );

	mm_printf(mm,"\n");

	/* Save it to EEPROM */
	{
		int rv;
		if( (rv = vxGrp->parGrp.save( &vxGrp->parGrp )))
			mm_printf(mm,"Error saving VxBline: %s\n", SysParamErrStr(rv));
	}
}

#endif /* SYSP_CONFIG_MENMON */

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

static SYSP_NV_STORAGE *UtRestart(void)
{
	SYSP_NV_STORAGE *nvs;
	SYSP_NVS_ADR nvsAdr;

	SYSP_UtSuperInit();

	/* create dummy nvs */
	SYSP_MEMSET( &nvsAdr, 0, sizeof(nvsAdr));
	nvs = SYSP_NvsCreate( sizeof(SYSP_NV_STORAGE),
						  "nvs", SYSP_super, "", nvsAdr, sizeof(G_physBuf));
	nvs->readNvs = UtReadNvs;
	nvs->writeNvs = UtWriteNvs;
	return nvs;
}

SYSP_UNITTEST( "SYSPARAM2: VxBline", SYSP_UtVxBl );

/** Checks VxBl functions */
int SYSP_UtVxBl( void )
{
	const int nvsOff = 0x10;
	const int nvsSize = 0x1e0;
	SYSP_PGRP_VX *vxGrp;
	SYSP_PAR_GRP *parGrp;
	char vxBline[656];
	SYSP_NV_STORAGE *nvs;


	/* SYSP_FromRawVxblIp */
	{
		u_int8 raw[10];
		char buf[5];

		/* good case */
		SYSP_STRCPY( raw, "ABC" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblIp( buf, 4, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "ABC\0\xaa", 5 ) == 0 );

		/* dest too small */
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblIp( buf, 3, raw ) == SYSPARAM_VAL_TRUNCATED);
		UTASSERT( SYSP_MEMCMP( buf, "AB\0\xaa", 4 ) == 0 );

		SYSP_STRCPY( raw, "ABC:" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblIp( buf, 4, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "ABC\0\xaa", 5 ) == 0 );

		SYSP_STRCPY( raw, ":FFF" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblIp( buf, 4, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "\0\xaa", 2 ) == 0 );
	}

	/* SYSP_FromRawVxblSm */
	{
		u_int8 raw[10];
		char buf[10];

		/* no ':' */
		SYSP_STRCPY( raw, "ABC" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblSm( buf, 4, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "\0\xaa", 2 ) == 0 );

		/* ':' present */
		SYSP_STRCPY( raw, "ABC:FF00" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblSm( buf, 5, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "FF00\0\xaa", 6 ) == 0 );


		/* dest too small */
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblSm( buf, 4, raw ) == SYSPARAM_VAL_TRUNCATED);
		UTASSERT( SYSP_MEMCMP( buf, "FF0\0\xaa", 5 ) == 0 );

		/* two ':' present */
		SYSP_STRCPY( raw, "ABC:FF00:ABC" );
		SYSP_MEMSET( buf, 0xaa, sizeof(buf));
		UTASSERT( SYSP_FromRawVxblSm( buf, 5, raw ) == 0 );
		UTASSERT( SYSP_MEMCMP( buf, "FF00\0\xaa", 6 ) == 0 );

	}

	/* SYSP_ToRawVxblIp */
	{
		u_int8 raw[50];

		SYSP_STRCPY( raw, "192.168.1.22:FF00FF00" );
		UTASSERT( SYSP_ToRawVxblIp( raw, "192.1.1.29" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.1.1.29:FF00FF00" ) == 0 );

		SYSP_STRCPY( raw, "192.168.1.22" );
		UTASSERT( SYSP_ToRawVxblIp( raw, "192.1.1.29" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.1.1.29" ) == 0 );

		SYSP_STRCPY( raw, ":FFFF0000" );
		UTASSERT( SYSP_ToRawVxblIp( raw, "192.1.1.29" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.1.1.29:FFFF0000" ) == 0 );

		SYSP_STRCPY( raw, "" );
		UTASSERT( SYSP_ToRawVxblIp( raw, "192.1.1.29" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.1.1.29" ) == 0 );
	}

	/* SYSP_ToRawVxblSm */
	{
		u_int8 raw[50];

		SYSP_STRCPY( raw, "192.168.1.22:FF00FF00" );
		UTASSERT( SYSP_ToRawVxblSm( raw, "FFFF0000" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.168.1.22:FFFF0000" ) == 0 );

		SYSP_STRCPY( raw, "192.168.1.22" );
		UTASSERT( SYSP_ToRawVxblSm( raw, "FFFF0000" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, "192.168.1.22:FFFF0000" ) == 0 );

		SYSP_STRCPY( raw, "" );
		UTASSERT( SYSP_ToRawVxblSm( raw, "FFFF0000" ) == 0 );
		UTASSERT( SYSP_STRCMP( raw, ":FFFF0000" ) == 0 );

	}

	/* Empty EEPROM, empty initial VxBline, non-empty default */
	{
		nvs = UtRestart();
		G_simNvsError = FALSE;
		SYSP_MEMSET( G_physBuf, 0xff, sizeof( G_physBuf ) );

		vxBline[0] = '\0';
		UTASSERT( SYSP_VxGrpCreate( nvs, nvsOff, nvsSize, vxBline,
									"fei(0,0):", TRUE) == 0 );

		UTASSERT( SYSP_STRCMP( vxBline, "fei(0,0):" ) == 0 );
	}

	/* Read error on EEPROM, empty initial VxBline, non-empty default */
	{
		nvs = UtRestart();
		G_simNvsError = TRUE;

		vxBline[0] = '\0';
		UTASSERT( SYSP_VxGrpCreate( nvs, nvsOff, nvsSize, vxBline,
									"fei(0,0):", TRUE) == 0 );

		UTASSERT( SYSP_STRCMP( vxBline, "fei(0,0):" ) == 0 );
	}

	G_simNvsError = FALSE;
	/* non-empty, good initial VxBline */
	{
		nvs = UtRestart();

		SYSP_STRCPY( vxBline, "gei(0,0):host" );
		UTASSERT( SYSP_VxGrpCreate( nvs, nvsOff, nvsSize, vxBline,
									"fei(0,0):", TRUE) == 0 );

		UTASSERT( SYSP_STRCMP( vxBline, "gei(0,0):host" ) == 0 );
	}

	/* non-empty, damaged initial VxBline */
	{
		nvs = UtRestart();

		SYSP_STRCPY( vxBline, "gei(0,\xaa):host" );
		UTASSERT( SYSP_VxGrpCreate( nvs, nvsOff, nvsSize, vxBline,
									"fei(0,0):", TRUE) == 0 );

		UTASSERT( SYSP_STRCMP( vxBline, "fei(0,0):" ) == 0 );
	}

	vxGrp = (SYSP_PGRP_VX *)nvs->parGrpLst;
	parGrp = &vxGrp->parGrp;

	/* enum params */
	{
		SYSPARAM_PARAM_INFO info;

		/* official name */
		UTASSERT( parGrp->enumParams( parGrp, 0, FALSE, &info ) == 0 );
		UTASSERT( SYSP_STRNCMP( info.parName, "bootdev", 10 ) == 0 );
		UTASSERT( info.attr == (SYSPARAM_ACC_RW | SYSPARAM_ACC_PERSISTANT) );

		/* alias name */
		UTASSERT( parGrp->enumParams( parGrp, 5, TRUE, &info ) == 0 );
		UTASSERT( SYSP_STRNCMP( info.parName, "nethost", 10 ) == 0 );
		UTASSERT( info.attr == (SYSPARAM_ACC_RW | SYSPARAM_ACC_PERSISTANT) );

		/* termination */
		UTASSERT( parGrp->enumParams( parGrp, 17, FALSE, &info ) ==
				  SYSPARAM_INVAL_VALUE );

	}

	/* set params */
	{
		UTASSERT( parGrp->setParam( parGrp, "tn", "target", NULL, 0 ) == 0 );
		UTASSERT( SYSP_STRCMP( vxBline, "fei(0,0): tn=target" ) == 0 );

		UTASSERT( parGrp->setParam( parGrp, "unitnum", "20", NULL, 0 ) == 0 );
		UTASSERT( parGrp->setParam( parGrp, "procnum", "14", NULL, 0 ) == 0 );
		UTASSERT( SYSP_STRCMP( vxBline, "fei(20,14): tn=target" ) == 0 );

		UTASSERT( parGrp->setParam( parGrp, "f", "80", NULL, 0 ) == 0 );
		UTASSERT( SYSP_STRCMP( vxBline, "fei(20,14): f=0x80 tn=target" ) == 0);

		UTASSERT( parGrp->setParam( parGrp, "kerpar", "with blanks",
									NULL, 0 ) == 0 );
		UTASSERT( SYSP_STRCMP( vxBline, "fei(20,14): f=0x80 tn=target "
							   "s='with blanks'" ) == 0);

	}
	/* get params */
	{
		char parVal[20];

		UTASSERT( parGrp->getParam( parGrp, "tn", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "target" ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "bootdev", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "fei" ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "procnum", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "14" ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "kerpar", parVal, 20) == 0 );
		UTASSERT( SYSP_STRCMP( parVal, "with blanks" ) == 0 );

		UTASSERT( parGrp->getParam( parGrp, "s", parVal, 10) ==
				  SYSPARAM_VAL_TRUNCATED );
		UTASSERT( SYSP_STRCMP( parVal, "with blan" ) == 0 );

	}

	/* save */
	{
		UTASSERT( parGrp->save( parGrp ) == 0 );
		UTASSERT( SYSP_STRCMP( (char *)&G_physBuf[nvsOff],
							   "fei(20,14): f=0x80 tn=target "
							   "s='with blanks'") == 0 );

	}
	/* set/get long startup string */
	{
		char parVal[256], rParVal[256];

		memset( parVal, 'A', sizeof(parVal));
		parVal[sizeof(parVal)-1] = '\0';

		UTASSERT( parGrp->setParam( parGrp, "kerpar", parVal, NULL, 0 ) == 0 );


		UTASSERT( parGrp->getParam( parGrp, "s",
									rParVal, sizeof(rParVal)) == 0);
		UTASSERT( SYSP_STRCMP( parVal, rParVal ) == 0 );

	}

	DBGWRT_1((DBH,"SYSP_UtVxBl OK\n"));
	return 0;
 ABORT:
	return -1;
}


#endif /* SYSP_CONFIG_UNITTEST */
