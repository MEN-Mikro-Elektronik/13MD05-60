/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mipios_communication.c
 *
 *      \author  uf
 *        $Date: 2009/03/31 10:40:02 $
 *    $Revision: 1.2 $
 * 
 *       \brief  MIPIOS for VxWorks.
 *
 * This file contains the MIPIOS communication for VxWorks.
 * The data will be swapped here to little endian format if running
 * on a big endian host.
 *
 *       \note  switches: DBG, INCLUDE_MIPIOS_VX
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mipios_communication.c,v $
 * Revision 1.2  2009/03/31 10:40:02  ufranke
 * changed
 *  - switch from EXCLUDE_MIPIOS_VX to INCLUDE_MIPIOS_VX
 *
 * Revision 1.1  2009/01/30 09:21:01  ufranke
 * checkin at project freeze
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#ifdef INCLUDE_MIPIOS_VX

/* ================================ if MIPIOS should be integrated into MDIS==================================== */
const char *MIPIOS_VX_COM_RCSid="$Id: mipios_communication.c,v 1.2 2009/03/31 10:40:02 ufranke Exp $\n"
"build " __DATE__ " " __TIME__ "\n";

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/


#include <vxWorks.h>
#include <string.h>
#include <taskLib.h>
#include <sysLib.h>

#include <in.h>		
#include <intLib.h>
#include <ioLib.h>	
#include <logLib.h>
#include <pingLib.h>
#include <socket.h>
#include <sockLib.h>


#include <MEN/men_typs.h>   /* MEN type definitions      */
#include <MEN/mdis_err.h>   /* MDIS error codes          */

#include <MEN/oss.h>
#include <MEN/desc.h>
#include <MEN/os2m.h>
#include <MEN/mipios.h>
#include <MEN/mdis_api.h>

#define _COMPILE_MIPIOS_INTERNAL_
#include "mipios_line_manager.h"
#include <MEN/mipios_vx_api.h>
#include "mipios_communication.h"

#ifdef DBG
	const static char errStart[]	= "*** ";
	const static char warnStart[]	= "*** warning ";
	const static char errLine[]		= " (line ";    
	const static char errEnd[]		= ")\n";      
#endif

/* use the debug stuff from the OS2M module for mipios_vx_api.c */
#define DBG_MYLEVEL		    OS2M_DbgLev
#include <MEN/dbg.h>        /* debug module */
#ifdef DBG
	#define  DBH OS2M_dbgHdl
	extern DBG_HANDLE   *OS2M_dbgHdl;
#endif /*DBG*/
extern u_int32 OS2M_DbgLev;


#define _SWAP16(word)	( (((word)>>8) & 0xff) | (((word)<<8)&0xff00) )

#define _SWAP32(dword)	( ((dword)>>24) | ((dword)<<24) | \
							  (((dword)>>8) & 0x0000ff00)   | \
							  (((dword)<<8) & 0x00ff0000)     )


/**********************************************************************/
/** Copy and swap frame to network or CPU byte order.
 *
 * SwapToLittleEndian() copies the mipios <in> frame into <out> and
 * swap word and long word of the MIPIOS frame header on big endian
 * machines.
 *
 * \param in	\IN  mipios frame
 * \param out	\OUT mipios frame
 *
 * \return 0 or error code. 
 */
static void SwapToLittleEndian( MIPIOS_FRAME *in, MIPIOS_FRAME *out )
{
	memcpy( out, in, sizeof(MIPIOS_FRAME) ); /* can size optimized */
	#ifdef __BIG_ENDIAN__
		out->hdr.crc32			= _SWAP32( out->hdr.crc32 );
		out->hdr.linkNumber 	= _SWAP32( out->hdr.linkNumber );
		out->hdr.devFrameNbr 	= _SWAP32( out->hdr.devFrameNbr );
		out->hdr.singleSignal 	= _SWAP32( out->hdr.singleSignal );
		out->hdr.returnCode 	= _SWAP32( out->hdr.returnCode );
		out->hdr.dataSize 		= _SWAP16( out->hdr.dataSize );
	#endif
}

/**********************************************************************/
/** Receive task of a MIPIOS line.
 *
 * MIPIOS_COM_RxTask() wait for a received frame on port.
 * The following field of frame header will be checked:
 *		magic
 *		version
 *		dataSize <= MIPIOS_MAX_DATA_SIZE
 *		crc32
 *
 * and sort it to input buffer by MIPIOS_LM_SortToRxBuffer().
 *
 * \param in	\IN  line handle
 *
 * \return 0 or error code. 
 */
int MIPIOS_COM_RxTask( MIPIOS_LINE *lineHdl )
{
  	static const char *functionName = __FUNCTION__;
	int error;
	MIPIOS_FRAME 		leRxFrame;		/* rx frame in little endian format like on the network */
	MIPIOS_FRAME 		cpueRxFrame;	/* rx frame in cpu endian format */
    struct sockaddr_in  clientAddr;    /* client's socket address */ 
    int                 sockAddrSize;  /* size of socket address structure */ 
    char                targetIP[INET_ADDR_LEN]; 
                                       /* buffer for client's inet addr */ 

    DBGWRT_ERR(( DBH, "%s line %s: listen on IP/port INADDR_ANY/%d", functionName, lineHdl->lineName, lineHdl->hostRxPort ));

	for(;;)
	{
		/* clear structures - can be optimized */
		bzero( (char *) &leRxFrame, sizeof(leRxFrame) );
		sockAddrSize = sizeof( clientAddr);
		bzero( (char *) &clientAddr, sockAddrSize );

		/* wait blocking for rx frames */
		error = recvfrom (lineHdl->fhRxSocket, (char *) &leRxFrame, sizeof(leRxFrame), 0, (struct sockaddr *) &clientAddr, &sockAddrSize);
        if( error == ERROR ) 
        { 
			lineHdl->errCnt.socket++;
	        DBGWRT_ERR(( DBH, "%s%s: socket() lineName/port %s/%d errCnt.socket %d %s%d%s", errStart, functionName, 
        			 lineHdl->lineName, lineHdl->hostRxPort, lineHdl->errCnt.socket, errLine, __LINE__, errEnd ));

			if( !(lineHdl->errCnt.socket%10) )
			{
				/* give up process time if we have to much errors on end ( on end means in german - in Folge ) */
        		taskDelay(1); 
	    	}

        }
        else
        {
        	/* we got a frame */
		    u_int8				typ;
		    u_int8				cmd;
		    u_int8				acknReq;
		    u_int32				frameNbr;
		    u_int32				retCode;
		    u_int32				crc32;
		    u_int32				crc32Computed;


			lineHdl->errCnt.rxTotal++;
	

			/* get address and port informations */
	        inet_ntoa_b(clientAddr.sin_addr, targetIP); 

			/* check if it is a MIPIOS frame */
			if( leRxFrame.hdr.magic != MIPIOS_MAGIC
				||	leRxFrame.hdr.version != MIPIOS_VERSION_1 
			  )
			{
				/* ignore all non mipios frames to the RX port */
		        DBGWRT_ERR(( DBH, "%s%s: line/port %s/%d from %s magic/version %02x/%02x mismatch %s%d%s", errStart, functionName,
				       			 lineHdl->lineName, lineHdl->hostRxPort, targetIP, 
				       			 leRxFrame.hdr.magic, leRxFrame.hdr.version,
				       			 errLine, __LINE__, errEnd ));

				lineHdl->errCnt.ignored++;
        		continue; /* get next frame */
			}


			/* copy / swap receive buffer to CPU endian buffer */
			SwapToLittleEndian( &leRxFrame, &cpueRxFrame );

			typ 		= cpueRxFrame.hdr.type;
			cmd			= cpueRxFrame.hdr.command & ~MIPIOS_TYPE_RESPONSE_REQ_MASK;
			acknReq		= cpueRxFrame.hdr.command & MIPIOS_TYPE_RESPONSE_REQ_MASK;
			retCode		= cpueRxFrame.hdr.returnCode;
			frameNbr	= cpueRxFrame.hdr.devFrameNbr;
			crc32		= cpueRxFrame.hdr.crc32;
		 	/*printf("<<< RX: frame %d type/cmd %x/%x from %s - %s - retcode %d data@0x%08x size/crc %d/%08x\n", (int)frameNbr,
	 				 typ, cmd, targetIP, acknReq ? "ackn req" : "", (int)retCode, (int)cpueRxFrame.data,
	 				 cpueRxFrame.hdr.dataSize, (int)crc32 ); */
        	
        	/* CRC check */        	
        	if( cpueRxFrame.hdr.dataSize < MIPIOS_MAX_DATA_SIZE+1 ) /* sanity check of size */
        	{
        		crc32Computed = MIPIOS_Crc32Get( ((u_int8*)&cpueRxFrame)+4, (sizeof(MIPIOS_FRAME_HEADER) - 4) + cpueRxFrame.hdr.dataSize );
        		if( crc32 != crc32Computed )
        		{
			        DBGWRT_ERR(( DBH, "%s%s: line/port %s/%d from %s crc %08x/%08x %s%d%s", errStart, functionName, 
	        			 lineHdl->lineName, lineHdl->hostRxPort, targetIP,
	        			 crc32, crc32Computed,
	        			 errLine, __LINE__, errEnd ));
	
					lineHdl->errCnt.crc++;
	        		continue; /* get next frame */
        		}
        		
        		error = MIPIOS_LM_SortToRxBuffer( lineHdl, &cpueRxFrame, clientAddr.sin_addr.s_addr, targetIP, NULL /* use internal frame handle */ );
        		if( error )
        		{
			        DBGWRT_ERR(( DBH, "%s%s: line/port %s/%d from %s MIPIOS_LM_SortToRxBuffer() error %d %s%d%s", errStart, functionName, 
	        			 lineHdl->lineName, lineHdl->hostRxPort, targetIP, error,
	        			 errLine, __LINE__, errEnd ));

					lineHdl->errCnt.rxBuff++;
	        		continue; /* get next frame */
        		}
        	}
        	else
        	{
		        DBGWRT_ERR(( DBH, "%s%s: line/port %s/%d port %s dataSize %d %s%d%s", errStart, functionName,
	        			 lineHdl->lineName, lineHdl->hostRxPort, targetIP,
	        			 cpueRxFrame.hdr.dataSize,
	        			 errLine, __LINE__, errEnd ));

				lineHdl->errCnt.size++;
        		continue; /* get next frame */
        	}
    	}
	}
}


/**********************************************************************/
/** RX port open routine
 *
 * OpenRxSocket() opens a RX port for the line.  
 *
 * \param lineHdl		\IN  line handle
 * \param *rxSocketFhP	\OUT socket file handle
 *
 * \return 0 or error code. 
 */
static int32 OpenRxSocket( MIPIOS_LINE *lineHdl, int *rxSocketFhP )
{
  	static const char *functionName = __FUNCTION__;
	int32 	error = ERR_MIPIOS_ILL_PARAM;
    int                 sockAddrSize;  /* size of socket address structure */ 
    struct sockaddr_in  serverAddr;    /* server's socket address */ 
    int     sFd = -1;
	
    DBGWRT_ERR(( DBH, "%s line %s: listen on IP/port INADDR_ANY/%d", 
    		functionName, lineHdl->lineName, lineHdl->hostRxPort ));

    /* create a datagram (UDP) socket */ 
 	printf("%s: socket\n",__FUNCTION__ );
 	sFd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sFd == ERROR ) 
    { 
    	error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: socket() lineName/port %s/%d %s%d%s",
        			 errStart, functionName, lineHdl->lineName, lineHdl->hostRxPort, errLine, __LINE__, errEnd ));
        goto CLEANUP;
    } 

    /* set up the local port */ 
    sockAddrSize 				= sizeof (struct sockaddr_in); 
    bzero ((char *) &serverAddr, sockAddrSize); 
    serverAddr.sin_len 			= (u_char) sockAddrSize; 
    serverAddr.sin_family 		= AF_INET; 
    serverAddr.sin_port 		= htons( lineHdl->hostRxPort ); 
    serverAddr.sin_addr.s_addr 	= htonl( INADDR_ANY ); 
 
    /* bind to local port */ 
    error = bind( sFd, (struct sockaddr *) &serverAddr, sockAddrSize );
    if( error == ERROR ) 
    { 
        close( sFd ); 
        sFd = -1;

    	error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: bind() lineName/port %s/%d %s%d%s",
        			 errStart, functionName, lineHdl->lineName, lineHdl->hostRxPort, errLine, __LINE__, errEnd ));
        goto CLEANUP;
    } 

	if( DBG_MYLEVEL )
	{
		mipios_show_socket_option( sFd );
	}


CLEANUP:
	*rxSocketFhP = sFd;
	return( error );
}


/**********************************************************************/
/** RX communication init
 *
 * MIPIOS_COM_RxInit() opens a RX port for the line.  
 *
 * \param lineHdl		\IN  line handle
 *
 * \return 0 or error code. 
 */
int32 MIPIOS_COM_RxInit( MIPIOS_LINE *lineHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32	error;
	char	rxTaskName[MIPIOS_MAX_LINE_NAME+3];
	int 	taskId;

	/* open receive socket */
	error = OpenRxSocket( lineHdl, &lineHdl->fhRxSocket );
	if( error )
	{
		error = ERR_MIPIOS_SOCKET;
        DBGWRT_ERR(( DBH, "%s%s: OpenRxSocket %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* spawn RX task */
	sprintf( rxTaskName, "%s_rx", lineHdl->lineName );
	
	lineHdl->rxTaskId = taskSpawn( rxTaskName, 		/* task name equal to line name + _rx	*/
									MIPIOS_LM_TASK_PRIORITY-1, 	/* task priority one below line manager	*/
									0,        					/* options 			*/
									0x8000,   					/* 32kB stack 		*/
									MIPIOS_COM_RxTask,			/* function 		*/
									(int)lineHdl,				/* arg1 			*/
									0,0,0,0,0,0,0,0,0 );
	if( lineHdl->rxTaskId == ERROR )
	{
		error = ERR_MIPIOS_ILL_PARAM;
        DBGWRT_ERR(( DBH, "%s%s: taskSpawn %s %s%d%s", errStart, functionName, rxTaskName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

	/* check if task is running */
	taskDelay( 2 + sysClkRateGet()/20 ); /* give time to start the task */
	taskId = taskNameToId( rxTaskName );
	if( taskId != lineHdl->rxTaskId )
	{
		error = ERR_MIPIOS;
        DBGWRT_ERR(( DBH, "%s%s: %s not running %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
        goto CLEANUP;
	}

CLEANUP:	
	return( error );
}

/**********************************************************************/
/** TX communication init
 *
 * MIPIOS_COM_TxInit() opens a TX socket for a line.  
 *
 * \param lineHdl		\IN  line handle
 *
 * \return 0 or error code. 
 */
int32 MIPIOS_COM_TxInit( MIPIOS_LINE *lineHdl )
{
  	static const char *functionName = __FUNCTION__;
	int32	error;

	/* open UDP transmit socket */
	error = socket (AF_INET, SOCK_DGRAM, 0);
    if( error == ERROR ) 
    { 
		error = ERR_MIPIOS_SOCKET;
        DBGWRT_ERR(( DBH, "%s%s: %s tx socket %s%d%s", errStart, functionName, lineHdl->lineName, errLine, __LINE__, errEnd ));
		goto CLEANUP;
    } 
	else
	{
		lineHdl->fhTxSocket = error; /* save socket file handle */
		error = 0; /* say no error */	
	}

CLEANUP:	
	return( error );
}


/**********************************************************************/
/** Send a frame via socket to IP/port.
 *
 * MIPIOS_COM_Send() sends a frame via socket to IP/port.  
 *
 * \param fhTxSocket	\IN  socket file handle
 * \param ip			\IN  IP address
 * \param port			\IN  destination port
 * \param txFrameP		\IN  frame to transmit
 *
 * \return 0 or error code. 
 */
u_int32 MIPIOS_COM_Send( int fhTxSocket, u_int32 ip, u_int16 port, MIPIOS_FRAME *txFrameP )
{
  	static const char *functionName = __FUNCTION__;
	u_int32 error = -1;
	MIPIOS_FRAME_HEADER *hdr = &txFrameP->hdr;
	MIPIOS_FRAME littleEndianSendFrame;	
    struct sockaddr_in  serverAddr;    /* server's socket address */ 
    int                 sockAddrSize;  /* size of socket address structure */ 

	DBGWRT_1(( DBH, "%s: # l/f/r %x/%d/%d t/c %02x/%02x size/crc %d/%08x\n",	functionName, 
				hdr->linkNumber, hdr->devFrameNbr, hdr->retryCnt, hdr->type, hdr->command, hdr->dataSize, hdr->crc32 ));


    sockAddrSize = sizeof (struct sockaddr_in); 
    bzero ((char *) &serverAddr, sockAddrSize); 
    serverAddr.sin_len = (u_char) sockAddrSize; 
    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_port = htons( port ); 
 
  #ifdef __BIG_ENDIAN__
    serverAddr.sin_addr.s_addr = ip;
  #else
	serverAddr.sin_addr.s_addr = OSS_SWAP32( ip );
  #endif

	SwapToLittleEndian( txFrameP, &littleEndianSendFrame );
	error = sendto( fhTxSocket, (caddr_t) &littleEndianSendFrame, sizeof(littleEndianSendFrame), 0, (struct sockaddr *) &serverAddr, sockAddrSize );
    if( error == ERROR || error != sizeof(littleEndianSendFrame) ) 
    { 
        DBGWRT_ERR(( DBH, "%s%s: sendto() %s%d%s", errStart, functionName, errLine, __LINE__, errEnd ));
		error = ERR_MIPIOS;
		goto CLEANUP;
    } 
    
    error = 0; /* successfull sendto() if we are here */

CLEANUP:				
	return( error );
}


#endif /*INCLUDE_MIPIOS_VX*/
