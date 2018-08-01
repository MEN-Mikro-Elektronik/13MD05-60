/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  mipios_vx_api.h
 *
 *      \author  men
 *        $Date: 2009/01/30 09:22:16 $
 *    $Revision: 2.1 $
 * 
 *  	 \brief  Header file for VxWorks MIPIOS API
 *                      
 *     Switches: MIPIOS_MAX_LINES 			- default is 5
 *               MIPIOS_MAX_DEVS_PER_LINE   - default is 10
 *               MIPIOS_MAX_DEVS  			- default is 50
 *               MIPIOS_NUM_FILES  			- default is 100
 *                      
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_vx_api.h,v $
 * Revision 2.1  2009/01/30 09:22:16  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008-2008 by MEN Mikro Elektronik GmbH, Nueremberg, Germany 
 ****************************************************************************/

#ifndef _MIPIOS_VX_API_H
#define _MIPIOS_VX_API_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#define MIPIOS_LM_TASK_PRIORITY			75	/* one more than net task - can by changed by taskPrioritySet() */

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
extern int32 MIPIOS_RandomVal;			/* can be set externaly before MIPIOS_SystemInit() call */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
#ifdef _COMPILE_MIPIOS_INTERNAL_  
	/* internal only */
	extern int32 MIPIOS_DevCreateByDiscover( char *devName, char *lineName, MIPIOS_DEVICE **devHdlP );
	extern int32 MIPIOS_DevIncreaseLinkNumber( MIPIOS_DEVICE *devHdlP );

	extern int32 MIPIOS_SIG_Install( u_int32 linkNbr, u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl, u_int32 sigNo );
	extern int32 MIPIOS_SIG_Remove(  u_int32 linkNbr, u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl, u_int32 sigNo );
	extern int32 MIPIOS_SIG_Signal(  u_int32 linkNbr, u_int32 devFrameNbr, u_int8 retryCnt, u_int32 tgtSigHdl );

#endif /*_COMPILE_MIPIOS_INTERNAL_*/

/* called internaly from OS2M or MDIS API */

	extern int32 MIPIOS_Open( OS2M_DEV_HDR *os2mDevHdl, int32 *pathP );
	extern int32 MIPIOS_Read(  int32 path, int32 *valueP );
	extern int32 MIPIOS_Write( int32 path, int32 value );
	extern int32 MIPIOS_GetBlock( int32 path, u_int8 *buffer, int32 length, int32 *nbrReadBytesP );
	extern int32 MIPIOS_SetBlock( int32 path, const u_int8 *buffer, int32 length, int32 *nbrWrittenBytesP );
	extern int32 MIPIOS_GetStat(  int32 path, int32 code, int32 *dataP );
	extern int32 MIPIOS_SetStat(  int32 path, int32 code, int32 data );
	extern int32 MIPIOS_Close( int32 path );



/* user API */
extern int32 MIPIOS_SystemInit( DESC_SPEC *systemDesc );
extern int32 MIPIOS_LM_LineInit( char *lineName, DESC_SPEC *lineDesc );
extern int32 MIPIOS_DevCreate( char *devName,  DESC_SPEC *lowlevelDesc, char *lineName );
extern int32 MIPIOS_SystemStartConfiguration( char *lineName );
extern int32 MIPIOS_SystemStartOperational( char *lineName );
extern int32 MIPIOS_SystemStop( char *lineName );

extern void  MIPIOS_Show( int verbose );
extern void  MIPIOS_ShowPath( void );


#ifdef __cplusplus
	}
#endif

#endif	/*_MIPIOS_VX_API_H*/
