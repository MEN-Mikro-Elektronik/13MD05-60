/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  sysp_mm_netif.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2005/06/23 15:54:46 $
 *    $Revision: 2.5 $
 * 
 *	   \project  SYSPARAM library
 *  	 \brief  Special fake NVS section/parameter group that provides
 *				 the nmacX and nspeedX parameters of network interfaces
 *				 that store those values into their own SROM.
 *      
 * Query NETIF subsystem for known devices that provide their own EEPROM
 *
 * If any NETIF device found, create a NVS_OBJECT that handles ALL those
 * devices.
 *
 * For each device, create one parameter group
 *
 *    \switches  none
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_mm_netif.c,v $
 * Revision 2.5  2005/06/23 15:54:46  kp
 * Copyright string changed (sbo)
 *
 * Revision 2.4  2005/02/02 16:03:17  kp
 * do not use all allocation space for NETIFs
 *
 * Revision 2.3  2005/01/18 15:49:39  kp
 * added size to NVS sections
 *
 * Revision 2.2  2005/01/14 09:36:22  kp
 * updated for new IOCTL names (PERSISTENT_MAC_ADDR)
 *
 * Revision 2.1  2005/01/13 15:26:04  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/
#include "sysp_intern.h"
#include <netif.h>

#define SYSP_MM_NETIF_NUM_PARAM 2

static const char G_nvsName[] = "mmnetif";

typedef struct {
	SYSP_PAR_GRP parGrp;		/**< common parameter group object  */
	int netifNum;				/**< serve the nth (0..n) nspeed/nmac 
								     parameter  */
	NETIF *netif;				/**< the NETIF device  */

	/* current values */
	NETIF_LINK_SPEED spd;
	NETIF_GET_MAC_ADDR mac;
	int macChanged;
} SYSP_PGRP_NETIF;

static char *NmacName( SYSP_PGRP_NETIF *netifGrp, char *buf )
{	
	return SYSP_NmacName( buf, netifGrp->netifNum );
}

static char *NspeedName( SYSP_PGRP_NETIF *netifGrp, char *buf )
{	
	return SYSP_NspeedName( buf, netifGrp->netifNum );
}

/**********************************************************************/
/** Get num parameters in mpgrp
 * \sa SYSP_PAR_GRP.numParams
 */
int SYSP_NetifGrpNumParams( SYSP_PAR_GRP *parGrp )
{
	return SYSP_MM_NETIF_NUM_PARAM;
}

/**********************************************************************/
/** Enum netifgrp params
 * \sa SYSP_PAR_GRP.enumParams
 */
int SYSP_NetifGrpEnumParams(
	SYSP_PAR_GRP *parGrp, 
	int idx,
	int alias,
	SYSPARAM_PARAM_INFO *info)
{
	SYSP_PGRP_NETIF *netifGrp = (SYSP_PGRP_NETIF *)parGrp;
	char buf[20];

	if( idx == 0 )
		NmacName( netifGrp, buf );
	else if( idx == 1 )
		NspeedName( netifGrp, buf );
	else
		return SYSPARAM_INVAL_VALUE;

	SYSP_StrSafeCpy( info->parName, buf, sizeof(info->parName));

	info->attr = SYSPARAM_ACC_RW | SYSPARAM_ACC_PERSISTANT |
		SYSPARAM_ACC_MMPARASTR;

	return 0;
}

/**********************************************************************/
/** Set netifgrp parameter 
 *
 * \sa SYSP_PAR_GRP.setParam
 */
int SYSP_NetifGrpSetParam(
	SYSP_PAR_GRP *parGrp, 
	const char *parName,
	const char *parVal,
	const char *passwd,
	int attr)
{
	SYSP_PGRP_NETIF *netifGrp = (SYSP_PGRP_NETIF *)parGrp;
	int rv=0;
	char buf[20];

	if( !SYSP_STRCMP( parName, NmacName( netifGrp, buf )) ){
		rv = SYSP_ToRawEth( netifGrp->mac.addr, parVal );
		netifGrp->macChanged = TRUE;
	}
	else if( !SYSP_STRCMP( parName, NspeedName( netifGrp, buf )) ){
		if( NETIF_LinkStringToSpeed( parVal, &netifGrp->spd ) != 0 )
			rv = SYSPARAM_INVAL_VALUE;
	}
	else
		rv = SYSPARAM_INVAL_PARAM;
	return rv;
}


/**********************************************************************/
/** Get netifgrp parameter
 * \sa SYSP_PAR_GRP.getParam
 */
int SYSP_NetifGrpGetParam(
	SYSP_PAR_GRP *parGrp, 
	const char *parName,
	char *parVal,
	int parValMaxLen )
{
	SYSP_PGRP_NETIF *netifGrp = (SYSP_PGRP_NETIF *)parGrp;
	char buf[20];
	int rv=0;

	if( parVal != NULL ){
		if( parValMaxLen > 0 )
			parVal[0] = '\0';
		else
			return SYSPARAM_INVAL_VALUE;
	}

	if( !SYSP_STRCMP( parName, NmacName( netifGrp, buf )) ){
		if( parVal == NULL )
			goto DONE;
		rv = SYSP_FromRawEth( parVal, parValMaxLen, &netifGrp->mac );
	}
	else if( !SYSP_STRCMP( parName, NspeedName( netifGrp, buf )) ){

		if( parVal == NULL )
			goto DONE;

		NETIF_LinkSpeedToString( buf, &netifGrp->spd );
		rv = SYSP_StrSafeCpy( parVal, buf, parValMaxLen );
	}
	else
		rv = SYSPARAM_INVAL_PARAM;
 DONE:
	return rv;
}

/**********************************************************************/
/** Set netifgrp to defaults 
 *
 * NOTE: THIS WILL NOT CHANGE MAC ADDRESS
 *
 * \sa SYSP_PAR_GRP.setDefaults
 */
int SYSP_NetifGrpSetDefaults(
	SYSP_PAR_GRP *parGrp, 
	int force)
{
	SYSP_PGRP_NETIF *netifGrp = (SYSP_PGRP_NETIF *)parGrp;
	netifGrp->spd.speed = NETIF_SPEED_AUTO;
	netifGrp->spd.duplex = NETIF_DPX_UNKNOWN;
	
	return 0;
}

/**********************************************************************/
/** Save netifgrp
 * \sa SYSP_PAR_GRP.save
 */
int SYSP_NetifGrpSave( SYSP_PAR_GRP *parGrp )
{
	SYSP_PGRP_NETIF *netifGrp = (SYSP_PGRP_NETIF *)parGrp;
	int rv = 0;
	NETIF *netif = netifGrp->netif;

	if( netif->ioctl( netif, NETIF_IOC_SET_PERSISTENT_SPEED,
					  &netifGrp->spd ) != 0 ){
		rv = SYSPARAM_EEPROM_ERROR;
		goto ABORT;
	}

	if( netifGrp->macChanged && 
		netif->ioctl( netif, NETIF_IOC_SET_PERSISTENT_MAC_ADDR,
					  &netifGrp->mac ) != 0 ){
		rv = SYSPARAM_EEPROM_ERROR;
		goto ABORT;
	}
 ABORT:
	return rv;
}

/**********************************************************************/
/** Create a parameter group object for NETIF parameters
 *
 * On success, the parameter group is added to the list of \a nvs groups
 *
 * \param nvs		\IN NV storage object (fully initialized) 
 *
 * \return sysparam error code
 *
 */
int SYSP_NetifGrpCreate( SYSP_NV_STORAGE *nvs, NETIF *netif, int netifNum )
{
	SYSP_PGRP_NETIF *netifGrp = NULL;
	int grpSize = sizeof(*netifGrp);
	int rv=0;

	/* alloc parameter group and raw data shadow */
	if( (netifGrp = SYSP_ALLOC( grpSize, nvs->super )) == NULL )
		return SYSPARAM_NO_RESOURCE;


	/* init object */

	netifGrp->parGrp.grpName		= "netif";
	netifGrp->parGrp.nvs 			= nvs;
	netifGrp->parGrp.nvsOffset	 	= 0;
	netifGrp->parGrp.numParams		= SYSP_NetifGrpNumParams;
	netifGrp->parGrp.enumParams		= SYSP_NetifGrpEnumParams;
	netifGrp->parGrp.setParam		= SYSP_NetifGrpSetParam;
	netifGrp->parGrp.getParam		= SYSP_NetifGrpGetParam;
	netifGrp->parGrp.setDefaults	= SYSP_NetifGrpSetDefaults;
	netifGrp->parGrp.save			= SYSP_NetifGrpSave;

	netifGrp->netif					= netif;
	netifGrp->netifNum				= netifNum;


	/* read current settings */
	if( netif->ioctl( netif, NETIF_IOC_GET_PERSISTENT_SPEED,
					  &netifGrp->spd ) != 0 ){
		rv = SYSPARAM_EEPROM_ERROR;
		goto ABORT;
	}

	if( netif->ioctl( netif, NETIF_IOC_GET_PERSISTENT_MAC_ADDR,
					  netifGrp->mac.addr ) != 0 ){
		rv = SYSPARAM_EEPROM_ERROR;
		goto ABORT;
	}

	SYSP_ParGrpAdd( nvs, &netifGrp->parGrp );
 ABORT:	
	if( rv != 0 )
		SYSP_ALLOC_UNDO( grpSize, nvs->super );
	return rv;
}



/**********************************************************************/
/** Fake NVS read routine (NOP)
 */
static int FakeReadNvs( 
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	u_int8 *data)
{
	return SYSPARAM_NOT_SUPPORTED;
}

/**********************************************************************/
/** Fake NVS write routine (NOP)
 */
static int FakeWriteNvs( 
	SYSP_NV_STORAGE *nvs,
	int offset,
	int size,
	const u_int8 *data)
{
	return SYSPARAM_NOT_SUPPORTED;
}


/**********************************************************************/
/** Create object to handle all fake NETIF parameters
 * 
 * Should be called 
 * - from CPU specific probe routine 
 *   (to ensure parameters exist after a restart)
 */
void SYSP_MmNetIfParamsCreate( SYSP_SUPER *super )
{
	SYSP_NV_STORAGE *nvs;
	SYSP_NVS_ADR dummyAdr;

	if( (nvs = SYSP_NvsCreate( 
			 sizeof( SYSP_NV_STORAGE ),
			 G_nvsName,
			 super,
			 "",
			 dummyAdr,
			 0)) == NULL )
		return;

	nvs->readNvs 	= FakeReadNvs;
	nvs->writeNvs 	= FakeWriteNvs;

	SYSP_NvsAdd( super, nvs );
	SYSP_MmNetIfParamsScan();
}


/**********************************************************************/
/** Scan list of network interfaces and create parameter groups for them
 * 
 * Should be called after all interfaces have been defined/attached in MMBIOS
 */
int SYSP_MmNetIfParamsScan( void )
{
	SYSP_SUPER *super = SYSP_super;
	SYSP_NV_STORAGE *nvs;
	NETIF *netif = NULL;
	int netifNum = 0;
	int numFoundDevices = 0;
	char nmacName[8];
	void *cdev = NULL;

	if( super==NULL || !(super->initialized) )
		return -1;

	if( (nvs = SYSP_NvsFind( super, G_nvsName )) == NULL)
		return -1;					/* no such object created */
	

	while( (cdev = NETIF_Iterate( cdev, &netif )) ){
		
		if( netif && NETIF_HasDedicatedSrom( netif ) ){
			SYSP_SNPRINTF( nmacName, sizeof(nmacName), "nmac%d", netifNum );

			if( SysParamGet( nmacName, NULL, 0 ) != 0 ){

				/* always leave some space in allocation */
				if( super->alloc.size >= 512 ){

					/* parameter does not already exist */
					SYSP_NetifGrpCreate( nvs, netif, netifNum );
				}
			}
			numFoundDevices++;
		}
		netifNum++;

	}
	return numFoundDevices;
}
