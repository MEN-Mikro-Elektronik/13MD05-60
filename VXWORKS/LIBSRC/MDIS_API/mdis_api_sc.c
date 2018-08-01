/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  mdis_api_sc.c
 *
 *      \author  uf
 *        $Date: 2010/03/29 16:35:15 $
 *    $Revision: 2.7 $
 *
 *        \brief  MDIS system call handlers for VxWorks 6.x RTPs
 *
 *            	  Some more detailed description please enter here
 *
 *     Switches: MEN_VXWORKS_VERSION was set automatically in VxWorks 6.x
 *               environment and enables RTP support.
 *               EXCLUDE_RTP can be set to disable RTP for VxWorks >= 6.x .
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mdis_api_sc.c,v $
 * Revision 2.7  2010/03/29 16:35:15  cs
 * R: generalized/enhanced exclude of RTP support
 * M: define EX-/INCLUDE_RTP in MDIS rules.mak already, parse it only here
 *
 * Revision 2.6  2009/04/01 15:10:27  ufranke
 * R: RTP support always included
 * M: added EXCLUDE_RTP
 *
 * Revision 2.5  2009/03/31 10:02:47  ufranke
 * cosmetics
 *
 * Revision 2.4  2006/10/13 23:10:32  cs
 * check complete string length of m_open with scMemValidate
 *
 * Revision 2.3  2006/07/17 14:50:23  ufranke
 * cosmetics for VxWorks 6.3
 *
 * Revision 2.2  2006/06/02 10:12:17  ufranke
 * added
 *  + MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL/REMOVE
 *
 * Revision 2.1  2005/12/23 11:18:31  UFRANKE
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005..2009 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

#ifndef INCLUDE_RTP
const char *MAPI_SC_RCSid = "MAPI SC: Pre VxWorks 6.x dummy $Id: mdis_api_sc.c,v 2.7 2010/03/29 16:35:15 cs Exp $";

	/* dummy function for VxWorks versions not supporting user mode */
	int MAPI_SC_InstallMdisSysCall( void )
	{
		return( 0 /* always for VxWorks < 6.0 */);
	}

#else
const char *MAPI_SC_RCSid = "MAPI SC: $Id: mdis_api_sc.c,v 2.7 2010/03/29 16:35:15 cs Exp $";

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include <MEN/men_typs.h>

#include <stdio.h>
#include <taskLib.h>
#include <errnoLib.h>
#include <syscallLib.h>
#include <scMemVal.h>
#include <sysLib.h>

#include <MEN/mdis_rtp.h>
#include <MEN/mdis_api.h>
#include <MEN/oss_vxworks_sig.h>

#include <MEN/dbg.h>        /* debug module */
#ifdef DBG
	extern   u_int32 OS2M_DbgLev;
	#define  DBG_MYLEVEL		    OS2M_DbgLev
	#define  DBH OS2M_dbgHdl
	extern   DBG_HANDLE   *OS2M_dbgHdl;
#endif /*DBG*/

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct
{
	int strLen;
	char *device;
}m_openScArgs;

typedef struct
{
	int fh;
}m_closeScArgs;

typedef struct
{
	int fh;
	int32 *valueP;
}m_readScArgs;

typedef struct
{
	int fh;
	int32 value;
}m_writeScArgs;

typedef struct
{
	int fh;
	int32 code;
	int32 *dataP;
}m_getstatScArgs;

typedef struct
{
	int fh;
	int32 code;
	int32 data;
}m_setstatScArgs;

typedef struct
{
	int fh;
	u_int8 *buffer;
	int32 length;
}m_getblockScArgs;

typedef struct
{
	int fh;
	u_int8 *buffer;
	int32 length;
}m_setblockScArgs;

typedef struct
{
	int 		taskId;
	int 		*kernelTidP;
	int			maxMsgs;
	MSG_Q_ID 	*kernelMsgQIdP;
}oss_VxSigMsgQ_ScArgs;

typedef struct
{
	int 		taskId;
}oss_VxSigMsgQRemove_ScArgs;

/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/
DBGCMD( static char errorStartStr[] = "*** ");
DBGCMD( static char errorLineStr[]   = " (line ");
DBGCMD( static char errorEndStr[]   = ")\n");

/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/

/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/


/**********************************************************************/
/** This function maps the system call to M_open.
 *  \param pArgs	m_openScArgs
 *
 *  \return path number or negative error code
 */
static int m_open( m_openScArgs *pArgs )
{
    DBGCMD( const char *functionName = __FUNCTION__; )

    if( scMemValidate (pArgs->device, pArgs->strLen, SC_PROT_READ) == ERROR )
	{
        DBGWRT_ERR( ( DBH, "%s%s: scMemValidate %s%d%s",
					  errorStartStr, functionName,
					  errorLineStr, __LINE__, errorEndStr ));
		errno = EINVAL;
		return ERROR;
	}

	return( M_open( pArgs->device ) );
}


/**********************************************************************/
/** This function maps the system call to M_close.
 *  \param pArgs	m_closeScArgs
 *
 *  \return 0 or error code
 */
static int m_close( m_closeScArgs *pArgs )
{
	int fh = pArgs->fh;

	return( M_close( fh ) );
}

/**********************************************************************/
/** This function maps the system call to M_read.
 *  \param pArgs	m_readScArgs
 *
 *  \return 0 or error code
 */
static int m_read( m_readScArgs *pArgs )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int 	fh 		= pArgs->fh;
	int32 	*valueP	= pArgs->valueP;

    if( scMemValidate( valueP, sizeof(*valueP), SC_PROT_WRITE ) == ERROR )
	{
        DBGWRT_ERR( ( DBH, "%s%s: scMemValidate %s%d%s",
					  errorStartStr, functionName,
					  errorLineStr, __LINE__, errorEndStr ));
		errno = EINVAL;
		return ERROR;
	}

	return( M_read( fh, valueP ) );
}

/**********************************************************************/
/** This function maps the system call to M_write.
 *  \param pArgs	m_writeScArgs
 *
 *  \return 0 or error code
 */
static int m_write( m_writeScArgs *pArgs )
{
	int 	fh 		= pArgs->fh;
	int32 	value	= pArgs->value;

	return( M_write( fh, value ) );
}

/**********************************************************************/
/** This function maps the system call to M_getstat.
 *  \param pArgs	m_getstatScArgs
 *
 *  \return 0 or error code
 */
static int m_getstat( m_getstatScArgs *pArgs )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int 	fh 		= pArgs->fh;
	int32 	code	= pArgs->code;
	int32 	*dataP	= pArgs->dataP;

    if( scMemValidate( dataP, sizeof(*dataP), SC_PROT_WRITE ) == ERROR )
	{
        DBGWRT_ERR( ( DBH, "%s%s: scMemValidate %s%d%s",
					  errorStartStr, functionName,
					  errorLineStr, __LINE__, errorEndStr ));
		errno = EINVAL;
		return ERROR;
	}

	return( M_getstat( fh, code, dataP ) );
}

/**********************************************************************/
/** This function maps the system call to M_setstat.
 *  \param pArgs	m_setstatScArgs
 *
 *  \return 0 or error code
 */
static int m_setstat( m_setstatScArgs *pArgs )
{
	int 	fh 		= pArgs->fh;
	int32 	code	= pArgs->code;
	int32 	data	= pArgs->data;

	return( M_setstat( fh, code, data ) );
}

/**********************************************************************/
/** This function maps the system call to M_getblock.
 *  \param pArgs	m_getblockScArgs
 *
 *  \return 0 or error code
 */
static int m_getblock( m_getblockScArgs *pArgs )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int 	fh 		= pArgs->fh;
	u_int8  *buffer	= pArgs->buffer;
	int32 	length	= pArgs->length;

    if( scMemValidate( buffer, length, SC_PROT_WRITE ) == ERROR )
	{
        DBGWRT_ERR( ( DBH, "%s%s: scMemValidate %s%d%s",
					  errorStartStr, functionName,
					  errorLineStr, __LINE__, errorEndStr ));
		errno = EINVAL;
		return ERROR;
	}

	return( M_getblock( fh, buffer, length ) );
}

/**********************************************************************/
/** This function maps the system call to M_setblock.
 *  \param pArgs	m_setblockScArgs
 *
 *  \return 0 or error code
 */
static int m_setblock( m_setblockScArgs *pArgs )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int 	fh 		= pArgs->fh;
	u_int8  *buffer	= pArgs->buffer;
	int32 	length	= pArgs->length;


    if( scMemValidate( buffer, length, SC_PROT_READ ) == ERROR )
	{
        DBGWRT_ERR( ( DBH, "%s%s: scMemValidate %s%d%s",
					  errorStartStr, functionName,
					  errorLineStr, __LINE__, errorEndStr ));
		errno = EINVAL;
		return ERROR;
	}

	return( M_setblock( fh, buffer, length ) );
}

/**********************************************************************/
/** This function maps the system call to OSS_VxSigMsgQInstall.
 *  \param pArgs	oss_VxSigMsgQ_ScArgs
 *
 *  \return 0 or error code
 */
static int oss_VxSigMsgQInstall( oss_VxSigMsgQ_ScArgs *pArgs )
{
	int error;
	int 		taskId 			= pArgs->taskId;
	int 		*kernelTidP 	= pArgs->kernelTidP;
	int			maxMsgs			= pArgs->maxMsgs;
	MSG_Q_ID 	*kernelMsgQIdP	= pArgs->kernelMsgQIdP;

	error = OSS_VxSigMsgQInstall( taskId, kernelTidP, maxMsgs, kernelMsgQIdP );
	return( error );
}

/**********************************************************************/
/** This function maps the system call to OSS_VxSigMsgQRemove.
 *  \param pArgs	oss_VxSigMsgQRemove_ScArgs
 *
 *  \return 0 or error code
 */
static int oss_VxSigMsgQRemove( oss_VxSigMsgQRemove_ScArgs *pArgs )
{
	int 		taskId = pArgs->taskId;

	return( OSS_VxSigMsgQRemove( taskId ) );
}

int m_dummy( void )
{
	return( -1 );
}

_WRS_DATA_ALIGN_BYTES(16) SYSCALL_RTN_TBL_ENTRY mdisScRtnTbl[] =
{
	/* MDIS API */
	{ (FUNCPTR) m_open, 	1,	"M_open",		0/*spare - function 0 */  },
	{ (FUNCPTR) m_close, 	1,	"M_close",		0/*spare - function 1 */  },
	{ (FUNCPTR) m_read, 	2,	"M_read",		0/*spare - function 2 */  },
	{ (FUNCPTR) m_write, 	2,	"M_write",		0/*spare - function 3 */  },
	{ (FUNCPTR) m_getstat, 	3,	"M_getstat",	0/*spare - function 4 */  },
	{ (FUNCPTR) m_setstat, 	3,	"M_setstat",	0/*spare - function 5 */  },
	{ (FUNCPTR) m_getblock,	3,	"M_getblock",	0/*spare - function 6 */  },
	{ (FUNCPTR) m_setblock,	3,	"M_setblock",	0/*spare - function 7 */  },

	/* dummy */
	{ (FUNCPTR) m_dummy,	0,	"m_dummy",		0/*spare - function 8 */  },
	{ (FUNCPTR) m_dummy,	0,	"m_dummy",		0/*spare - function 9 */  },

	/* USR_OSS */
	{ (FUNCPTR) oss_VxSigMsgQInstall,	2,	"OSS_VxSigMsgQInstall",		0/*spare - function 10 */  },
	{ (FUNCPTR) oss_VxSigMsgQRemove,	2,	"OSS_VxSigMsgQRemove",		0/*spare - function 10 */  },

};

/**********************************************************************/
/** This function installs the MDIS system call handler functions.
 *  If a system call group  number MDIS_RTP_SYSCALL_GROUP is already
 *  installed it will overwritten after displaying a warning.
 *
 *  \return 0=ok, or error code
 */
int MAPI_SC_InstallMdisSysCall( void )
{
    DBGCMD( const char *functionName = __FUNCTION__; )
	int error;

	error = syscallGroupRegister( MDIS_RTP_SYSCALL_GROUP,
								  MDIS_RTP_SYSCALL_GROUP_NAME,
								  sizeof(mdisScRtnTbl)/sizeof(SYSCALL_RTN_TBL_ENTRY),
								  mdisScRtnTbl, 0 /* do not overwrite existing system call */
							    );
    if( error )
    {
        DBGWRT_ERR( ( DBH, "%s%s: warning syscallGroupRegister group %d exits overwrite %s%d%s",
					  errorStartStr, functionName,
					  MDIS_RTP_SYSCALL_GROUP,
					  errorLineStr, __LINE__, errorEndStr ));

		printf("*** Warning: %s syscallGroupRegister() group %d exits - overwrite\n",
		       __FUNCTION__, MDIS_RTP_SYSCALL_GROUP );
		taskDelay( sysClkRateGet() );

		error = syscallGroupRegister( MDIS_RTP_SYSCALL_GROUP,
								  MDIS_RTP_SYSCALL_GROUP_NAME,
								  sizeof(mdisScRtnTbl)/sizeof(SYSCALL_RTN_TBL_ENTRY),
								  mdisScRtnTbl, 1 /* do overwrite existing system call */
							    );
	    if( error )
    	{
        	DBGWRT_ERR( ( DBH, "%s%s: syscallGroupRegister group %d %s%d%s",
						  errorStartStr, functionName,
						  MDIS_RTP_SYSCALL_GROUP,
						  errorLineStr, __LINE__, errorEndStr ));
    	}/*if*/
    }/*if*/

	return( error );
}
#endif /* INCLUDE_RTP */

