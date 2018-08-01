/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  mipios_line_manager.h
 *
 *      \author  men
 *        $Date: 2009/01/30 09:21:07 $
 *    $Revision: 1.1 $
 * 
 *  	 \brief  Header file for VxWorks MIPIOS line manager
 *                      
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_line_manager.h,v $
 * Revision 1.1  2009/01/30 09:21:07  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008-2008 by MEN Mikro Elektronik GmbH, Nueremberg, Germany 
 ****************************************************************************/

#ifndef _MIPIOS_VX_LM_H
#define _MIPIOS_VX_LM_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifdef __BIG_ENDIAN__
	#define _IP_ARR(_x_) _x_[0],_x_[1],_x_[2],_x_[3]   /* for printf(" IP %d.%d.%d.%d ", _IP_ARR( (u_int8*)&ip ) ); */
#else
	#define _IP_ARR(_x_) _x_[3],_x_[2],_x_[1],_x_[0]
#endif

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
typedef struct
{
	int				guardingMode;		/* system, line, device */
	u_int32			newLinkNumber;
}MIPIOS_SYSTEM;


typedef struct
{
	u_int32 rxTotal;
	u_int32 txTotal;
	u_int32 ignored;
	u_int32 socket;
	u_int32 size;
	u_int32 crc;
	u_int32 rxBuff;
	u_int32 txBuff;
	u_int32 tmo;
	u_int32 unknown;
}MIPIOS_CNT;

typedef struct
{
	OSS_SEM_HANDLE	*semBuffFull;
	OSS_SEM_HANDLE	*semBuffAccess;
	int				empty;
	u_int32			ip;
	MIPIOS_FRAME	frame;
}MIPIOS_FRAME_HDL;


typedef struct
{
	char			lineName[MIPIOS_MAX_LINE_NAME];
	DESC_SPEC 		*lineDesc;
	int				state;				/* configuration, operational, stop */

	/* u_int32 		hostIp;            host IP not necessary - ethernet if given by OS specific routing to target IP */
	u_int16 		hostRxPort;
	int				fhRxSocket;
	int				fhTxSocket;
	
	int				connectMode;
	u_int32			maxRetry;
	u_int32			timeoutMs;
	u_int32			timeoutTicks;
	OSS_SEM_HANDLE  *lineSem;

	OSS_SEM_HANDLE  *txSem;
	MIPIOS_FRAME	txFrame;

	MIPIOS_FRAME_HDL	rxFrameHdl;				/* for socket rx buffering */
	MIPIOS_FRAME_HDL	rxAutoConnectFrameHdl;	/* for autoconnect frames  */
	MIPIOS_FRAME_HDL	rxOutputFrameHdl;		/* for MDIS communication  */

	void			*currDevHdl;

	int				lineTaskId;
	int				rxTaskId;
	MIPIOS_CNT		errCnt;
	void			*devTbl[MIPIOS_MAX_DEVS_PER_LINE];
}MIPIOS_LINE;

typedef struct
{
	OS2M_DEV_HDR 	os2mDevHdl;
	char         	name[MIPIOS_MAX_DEV_NAME];
	DESC_SPEC    	*llDesc;
	MIPIOS_LINE	 	*mipiosLineP;
	OSS_SEM_HANDLE  *devSem;
	u_int32 		targetIp;
	u_int16 		targetReceivePort;
	u_int32			linkNumber;
	u_int32			frameNumber;

	int			 	discovered;
	u_int32		 	linkState;
	int				state;				/* configuration, operational, stop */
	int32		 	openCount;

	u_int32			lastSendTick;			/* stores the time of last communication with the device */
	u_int32			installedSignalCount;	/* to decide if we have to poll signals */
	int				haveToPollSignals;		/* flag if we have more signals or it's time to poll */
	u_int32			timeoutMs_guarding;		/* guarding time in [ms] */
	u_int32			timeoutMs_signalPoll;	/* signal poll time in [ms] */
}MIPIOS_DEVICE;

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
extern MIPIOS_LINE    MIPIOS_LineTbl[];

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern int32 MIPIOS_LM_Init( void );
extern int32 MIPIOS_LM_StartConfiguration( char *lineName );
extern int32 MIPIOS_LM_StartOperational( char *lineName );
extern int32 MIPIOS_LM_Stop(  char *lineName );
extern int32 MIPIOS_LM_StrMax( char *str, int max );
extern int32 MIPIOS_LM_CheckLineName( char *lineName );
extern int32 MIPIOS_LM_GetLine( char *lineName, MIPIOS_LINE **lineHdlP );

extern int MIPIOS_LM_SortToRxBuffer
(
	MIPIOS_LINE *lineHdl,
	MIPIOS_FRAME *cpueRxFrameP,
	u_int32 ip,
	char *targetIp,
	MIPIOS_FRAME_HDL	*frameHdlP
);

extern int32 MIPIOS_LM_Open(    MIPIOS_DEVICE *devHdl, int32 *fileHandleP );
extern int32 MIPIOS_LM_Close(   MIPIOS_DEVICE *devHdl, int32 targetPath   );
extern int32 MIPIOS_LM_Write(   MIPIOS_DEVICE *devHdl, int32 targetPath, int32 value );
extern int32 MIPIOS_LM_Read(    MIPIOS_DEVICE *devHdl, int32 targetPath, int32 *valueP );
extern int32 MIPIOS_LM_SetStat( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 code, int32 data );
extern int32 MIPIOS_LM_GetStat( MIPIOS_DEVICE *devHdl, int32 targetPath, int32 code, int32 *dataP );
extern int32 MIPIOS_LM_SetBlock(MIPIOS_DEVICE *devHdl, int32 targetPath, u_int8 *buffer, int32 length, int32 *nbrWrittenBytesP );
extern int32 MIPIOS_LM_GetBlock(MIPIOS_DEVICE *devHdl, int32 targetPath, u_int8 *buffer, int32 length, int32 *nbrReadBytesP );



extern u_int32 MIPIOS_Crc32Get( u_int8 *buf, int len );


#ifdef __cplusplus
	}
#endif

#endif	/*_MIPIOS_VX_LM_H*/
