/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  can_test.c
 *
 *      \author  aw
 *        $Date: 2009/03/09 11:02:17 $
 *    $Revision: 1.2 $
 * 
 *  	 \brief  Can test tool for project critical software. F50 has to be 
 *               connected to F50 testboard.  
 *               cloned from mscan_alyzer.c
 *
 *
 *     Switches: -
 *     Required: libraries: mdis_api, usr_oss, usr_utl, mscan_api
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: can_test.c,v $
 * Revision 1.2  2009/03/09 11:02:17  AWanka
 * cosmetics
 *
 * Revision 1.1  2009/03/06 16:59:29  AWanka
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_err.h>
#include <MEN/mscan_api.h>
#include <MEN/mscan_drv.h>		

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CHK(expression) \
 if( !(expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
      printf("%s\n\n",mscan_errmsg(UOS_ErrnoGet()));\
     goto ABORT;\
 }

#define TX_OBJ		1
#define RX_OBJ_STD	2
#define RX_OBJ_EXT	3

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

typedef struct idPoolHandle{
	u_int32 id[6];
}ID_POOL;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int ReceiveFrames( int32 *, u_int8, ID_POOL, int );
static int SendFrames( int32 *, u_int8, ID_POOL );
static void DumpFrame( char *, const MSCAN_FRAME * );
static int CmpFrames( const MSCAN_FRAME *, const MSCAN_FRAME * );
static void MakeFrame( MSCAN_FRAME *, int );
static void NxtId( int, u_int32 * );
static void FillIdPool(ID_POOL *);
static int InitCan(int32 *, u_int8, u_int32 );

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/**********************************************************************/
/** Program entry point
 *  This routine performs a test on the CAN interfaces. The test board
 *  A_07320 has to be connected to the CAN interface under test. One CAN sends 
 *  4 frames and the other 5 CAN ports receives the frames.
 *
 *  \param port         \IN number of the CAN port, which sends the frames
 *                          1: CAN S1
 *                          2: CAN A
 *                          3: CAN J
 *                          4: CAN Display
 *                          5: CAN B
 *                          6: CAN S2
 *  \param bitrate      \IN bitrate value for the CANs
 *  \param runs         \IN number of passes
 *  \param debugLevel   \IN 0: print error messages
 *                          1: no outputs
 *
 *  \return  all  0: OK
 *           Bit  0: send failed
 *           Bit  8: receive CAN S1 failed
 *           Bit  9: receive CAN A  failed             
 *           Bit 10: receive CAN J  failed
 *           Bit 11: receive CAN Display failed
 *           Bit 12: receive CAN B failed
 *           Bit 13: receive CAN S2 failed
 *           Bit 16: MSCAN init failed
 */
u_int32 GC_CanLoopframes
( 
	u_int8 port, 
	u_int32 bitrate, 
	int runs, 
	int debugLevel 
)
{
	int32	path[6]; 
	u_int32 ret=0;
	int run;
    u_int8	i = 0;
	u_int8 sendDev = port - 1;
	ID_POOL idPool;

	
	for(i=0; i<6; i++){
		if(InitCan(&path[i], i, bitrate) != OK){
			ret = 0x10000;
			goto ABORT;
		}
	}
	for( run=1; run<=runs; run++ ){
		FillIdPool(&idPool);
		if(SendFrames(&path[sendDev], sendDev, idPool) == ERROR){
			ret = 1;
			if(debugLevel == 0){
				printf("send frame failed\n");
			}
		}
		else{
			if(debugLevel == 0){
				printf("send frame OK\n");
			}
		}
		taskDelay(10);
		for(i=0; i<6; i++){
			if(sendDev != i){
				if(ReceiveFrames(&path[i], sendDev, idPool, debugLevel) 
                   == ERROR ){
					if(debugLevel == 0){
						printf("receive frame failed\n");
					}
					ret |= 1 << (i+8);
				}
				else{
					if(debugLevel == 0){
						printf("receive frame OK\n");
					}
				}
			}
		}
	}

	taskDelay( 100 );
	for(i=0; i<6; i++){
		CHK( mscan_enable( path[i], FALSE ) == 0 );
		CHK( mscan_term(path[i]) == 0 );
		path[i]=-1;
	}

 ABORT:
	
	for(i=0; i<6; i++){
		if( path[i] != -1 ) {
    	    mscan_enable( path[i], FALSE );
			mscan_term(path[i]);
    	}
	}

	return(ret);
}

/**********************************************************************/
/** send frames
 *   Send 4 CAN frames. Every frame gets an new Id from function NxtId.
 * 
 * \param pathP    \IN pointer to the path number
 * \param sendPort \IN port number from CAN which should send
 * \param idPool   \IN buffer with Ids to be send
 * 
 * \return 0=OK, -1=ERROR
 */
static int SendFrames
( 
	int32 *pathP, 
	u_int8 sendPort, 
	ID_POOL idPool  
)
{
	int retVal = ERROR, i;
	u_int32 txId = idPool.id[sendPort];
	MSCAN_FRAME txFrm;

	NxtId( sendPort, &txId );

	for( i=0; i<4; i++ ){
		MakeFrame( &txFrm, txId );
		if( mscan_write_msg( *pathP, TX_OBJ, -1, &txFrm ) == 0 ){
			NxtId( sendPort, &txId );
		}
		else{
			retVal = ERROR;
			goto ABORT;
		}
	}
    retVal = OK;

 ABORT:

	return retVal;
}

/**********************************************************************/
/** receive frames
 *   Receive 4 CAN frames. The received frame will be compared with
 *   the expected frame.
 * 
 *  \param receivePathP    	\IN pointer to the path number from the receiving
 *                              CAN
 *  \param sendPort 		\IN port number from CAN which sent out
 *  \param idPool   		\IN buffer with Ids to be send
 *  \param debugLevel       \IN 0: print error messages
 *                              1: no outputs
 * 
 * \return 0=OK, -1=ERROR
 */
static int ReceiveFrames
( 
	int32 *receivePathP, 
	u_int8 sendPort, 
	ID_POOL idPool,
	int debugLevel
)
{
	int rv = ERROR;
	u_int32 rxId = idPool.id[sendPort];
	MSCAN_FRAME rxFrm, sbFrm;
	u_int8 n = 0;

	NxtId( sendPort, &rxId );
	
	while( n != 4 ){

		if( (mscan_read_msg( *receivePathP, RX_OBJ_STD, 50, &rxFrm ) == 0) ){
			/* received something */
			MakeFrame( &sbFrm, rxId );

			if( CmpFrames( &rxFrm, &sbFrm ) != 0 ){
				if(debugLevel == 0){
					DumpFrame( "Sb  ", &sbFrm );
					DumpFrame( "Recv", &rxFrm );
				}
				CHK(0);
			}
			
			NxtId( sendPort, &rxId );
		}
		else{
			rv = ERROR;
			goto ABORT;
		}
		n++;
	}

	rv = 0;

 ABORT:

	return rv;
}

/**********************************************************************/
/** Fill Idpool
 *   This function fills the id pool with random values. Every CAN has an
 *   own id range for their frames that they send.
 *
 *   CAN 0: 0x000 - 0x0FF
 *   CAN 1: 0x100 - 0x1FF
 *   CAN 2: 0x200 - 0x2FF
 *   CAN 3: 0x300 - 0x3FF
 *   CAN 4: 0x400 - 0x4FF
 *   CAN 5: 0x500 - 0x5FF
 * 
 * \param idPoolP   			\IN buffer with Ids to be send
 * 
 */
static void FillIdPool(ID_POOL *idPoolP)
{
	struct timeval now;
	u_int32 rvalue;

	/* re-initialize the random value */
	microtime (&now);  
	srand ((u_long)now.tv_usec);
	
	rvalue = (u_int32)rand();

	idPoolP->id[0] =  0xFF & rvalue;
	idPoolP->id[1] = (0xFF & rvalue) | 0x100;
	idPoolP->id[2] = (0xFF & rvalue) | 0x200;
	idPoolP->id[3] = (0xFF & rvalue) | 0x300;
	idPoolP->id[4] = (0xFF & rvalue) | 0x400;
	idPoolP->id[5] = (0xFF & rvalue) | 0x500;
}

/**********************************************************************/
/** MakeFrame
 *   The function MakeFrame creates a frame with the base of id. The length 
 *   of the frame is dependent on the id and is growing by 1 byte until 8.
 * 
 * \param frm    	\INOUT  pointer to the created frame
 * \param id 		\IN     id of the frame
 * 
 */
static void MakeFrame( MSCAN_FRAME *frm, int id )
{
	int i;
	u_int8 d;

	frm->id = id;
	frm->flags = 0;
	frm->dataLen = id % 9;

	d = id & 0xff;

	for( i=0; i<frm->dataLen; i++ )
		frm->data[i] = d++;
	for( ; i<8; i++ )
		frm->data[i] = 0;
}

/**********************************************************************/
/** Build next ID for frames send by MSCAN
 *
 * \param path \IN     path of the CAN
 * \param idP  \INOUT  pointer to the frame id
 *
 */
static void NxtId( int path, u_int32 *idP )
{
	u_int32 id = *idP;

	id++;
	switch( path ){
	case 0: 
		if( id==0x100 ){
			id=0x0;
		}
		break;
	case 1: 
		if( id==0x200 ){
			id=0x100;
		}
		break;
	case 2: 
		if( id==0x300 ){
			id=0x200;
		}
		break;
	case 3: 
		if( id==0x400 ){
			id=0x300;
		}
		break;
	case 4: 
		if( id==0x500 ){
			id=0x400;
		}
		break;
	case 5: 
		if( id==0x600 ){
			id=0x500;
		}
		break;
	}
	*idP = id;
}

/**********************************************************************/
/** InitCan
 *   Receive 4 CAN frames. The received frame will be compared with
 *   the expected frame.
 * 
 *  \param pathP    	\IN pointer to the path number from the CAN to 
 *                          initialize
 *  \param deviceNumber \IN 0: CAN S1
 *                          1: CAN A
 *                          2: CAN J
 *                          3: CAN Display
 *                          4: CAN B
 *                          5: CAN S2
 *  \param bitrate      \IN bitrate value for the CAN
 * 
 *  \return 0=OK, -1=ERROR
 */
static int InitCan(int32 *pathP, u_int8 deviceNumber, u_int32 bitrate)
{
	int brp, sjw, tseg1, tseg2;
	u_int32 spl=0;
	char	device[20];
	MSCAN_FILTER flt;
	int n;
	int32	ret=1;

	brp       = 0;
    sjw       = 1;
    tseg1     = 5;
    tseg2     = 2;

	/*--------------------+
	|  open paths         |
	+--------------------*/
	sprintf(device,"/can/%d", deviceNumber+1);
	CHK( (*pathP = mscan_init(device)) >= 0 );

	CHK( M_setstat( *pathP, MSCAN_MAXIRQTIME, 0 ) == 0 );
	/*--------------------+
	|  config             |
	+--------------------*/
	mscan_enable( *pathP, FALSE );

	taskDelay(20);

	if( brp == 0 ) {
	    CHK( mscan_set_bitrate( *pathP, (MSCAN_BITRATE)bitrate, spl ) == 0 );  
	}
	else {
	    printf("Bustiming: BRP=%d SJW=%d TSEG1=%d TSEG2=%d\n",brp, sjw, tseg1,
		tseg2);
	    CHK( mscan_set_bustiming( *pathP, brp, sjw, tseg1, tseg2, 0 ) == 0 );
	}
	
	/*--- config error object ---*/
	CHK( mscan_config_msg( *pathP, 0, MSCAN_DIR_RCV, 10, NULL ) == 0 );
	
	/*--- config Tx object ---*/
	CHK( mscan_config_msg( *pathP, TX_OBJ, MSCAN_DIR_XMT, 100, NULL ) == 0 );

	/*--- config Rx object ---*/
	flt.code = 0x000;
	flt.mask = 0xfff;
	flt.cflags = 0;
	flt.mflags = MSCAN_USE_ACCFIELD;
	
	memset( &flt.accField, 0, sizeof( flt.accField ));
	
	for( n=0; n<0x7ff; n++ ){
		MSCAN_ACCFIELD_SET( flt.accField, n );
	}
	
	CHK( mscan_config_msg( *pathP, RX_OBJ_STD, MSCAN_DIR_RCV, 500, 
						   &flt ) == 0 );
	flt.code = 0x000;
	flt.mask = 0xff3fffff;
	flt.cflags = MSCAN_EXTENDED;
	flt.mflags = 0;
	memset( &flt.accField, 0, sizeof( flt.accField ));

	CHK( mscan_config_msg( *pathP, RX_OBJ_EXT, MSCAN_DIR_RCV, 500, 
						   &flt ) == 0 );
	
	/*--- make sure CAN not in loopback mode ---*/
	CHK( mscan_set_loopback( *pathP, FALSE ) == 0 );
	
	/*--- enable bus ---*/
	CHK( mscan_enable( *pathP, TRUE ) == 0 );

	/* after switching to normal mode CAN controller needs at least 21ms delay 
	   before he can work */
	taskDelay(30);

	ret = 0;

ABORT:
	
	return(ret);
}

/**********************************************************************/
/** CmpFrames
 *   This function compares two CAN frames. Not only the data is compared also
 *   the id, flags and data length.
 * 
 *  \param frm1    	\IN frame 1
 *  \param frm2     \IN frame 2
 * 
 *  \return 0=OK, -1=ERROR
 */
static int CmpFrames( const MSCAN_FRAME *frm1, const MSCAN_FRAME *frm2 )
{
	int i;

	if( frm1->id != frm2->id )
		return ERROR;

	if( frm1->flags != frm2->flags )
		return ERROR;

	if( !(frm1->flags & MSCAN_RTR)){
		/* Note: CANalyzer sends remote frames always with DLC=0 */
		if( frm1->dataLen != frm2->dataLen )
			return ERROR;

		for( i=0; i<frm1->dataLen; i++ )
			if( frm1->data[i] != frm2->data[i] )
				return ERROR;
	}

	return OK;
}

/**********************************************************************/
/** DumpFrame
 *   This function dumps a CAN frame. Not only the data is compared also
 *   the id, flags and data length.
 * 
 *  \param frm1    	\IN frame 1
 *  \param frm2     \IN frame 2
 * 
 *  \return 0=OK, -1=ERROR
 */
static void DumpFrame( char *msg, const MSCAN_FRAME *frm )
{
	int i;
	printf("%s: ID=0x%08lx%s", 
		   msg,
		   frm->id, 
		   (frm->flags & MSCAN_EXTENDED) ? "x":"");

	if( frm->flags & MSCAN_RTR )
		printf(" RTR dataLen=%d", frm->dataLen );
	else {
		printf(" data=");

		for(i=0; i<frm->dataLen; i++ ){
			printf("%02x ", frm->data[i] );
		}
	}
	printf("\n");
}
