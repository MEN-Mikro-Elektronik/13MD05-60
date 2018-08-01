/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  oss_smb.c
 *
 *      \author  uf
 *        $Date: 2006/02/14 22:25:35 $
 *    $Revision: 1.3 $
 *
 *        \brief  SMB handle set/get functions
 *
 *                A table of OSS_SMB_TBL_ENTRIES_MAX is implemented here.
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_smb.c,v $
 * Revision 1.3  2006/02/14 22:25:35  cs
 * removed RCSid
 *
 * Revision 1.2  2006/01/27 14:34:15  UFRANKE
 * fixed
 *  - OSS_GetSmbHdl() returns error if
 *    the corresponding handle was not initialized
 *
 * Revision 1.1  2005/12/23 15:22:24  UFRANKE
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2005 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/

/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/
#define OSS_SMB_TBL_ENTRIES_MAX 0x100

/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/
static int G_smbHdlTblIsInit = 0;
static void* G_smbHdlTbl[OSS_SMB_TBL_ENTRIES_MAX];

/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/

void CheckInit( void )
{
	u_int32 i;
	if( G_smbHdlTblIsInit == 0 )
	{
		for( i=0; i<OSS_SMB_TBL_ENTRIES_MAX; i++ )
			G_smbHdlTbl[i] = NULL;
		G_smbHdlTblIsInit = 1;
	}
}


/**********************************************************************/
/** Set SMB handle
 *
 * \copydoc oss_specification.c::OSS_SetSmbHdl()
 */
int32 OSS_SetSmbHdl
(
    OSS_HANDLE  *osHdl,
    u_int32		busNbr,
	void		*smbHdl
)
{
	int error = 0;
	CheckInit();
	if( busNbr < OSS_SMB_TBL_ENTRIES_MAX )
		G_smbHdlTbl[busNbr] = smbHdl;
	else
		error = ERR_OSS_ILL_PARAM;
	return( error );
}

/**********************************************************************/
/** Get SMB handle
 *
 * \copydoc oss_specification.c::OSS_GetSmbHdl()
 */
int32 OSS_GetSmbHdl
(
    OSS_HANDLE  *osHdl,
    u_int32		busNbr,
	void		**smbHdlP
)
{
	int error = 0;
	CheckInit();

	*smbHdlP = NULL;
	if( busNbr < OSS_SMB_TBL_ENTRIES_MAX )
		*smbHdlP = G_smbHdlTbl[busNbr];
	else
		error = ERR_OSS_ILL_PARAM;

	if( *smbHdlP == NULL )
	{
		error = ERR_OSS_ILL_PARAM;
	}

	return( error );
}

