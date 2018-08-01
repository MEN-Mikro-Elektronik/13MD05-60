/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  looptest.c
 *
 *      \author  aw
 *        $Date: 2009/07/27 11:03:19 $
 *    $Revision: 1.4 $
 *
 *        \brief Loopback test using raw ethernet frames
 *
 *
 *     Switches: 
 */
/*---------------------------[ Public Functions ]----------------------------
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: looptest.c,v $
 * Revision 1.4  2009/07/27 11:03:19  MKolpak
 * R: Cosmetics
 * M: Inserted note for ethernet looptest
 *
 * Revision 1.3  2009/03/06 17:02:20  AWanka
 * R: comment was wrong, GC_Loopframes returns not only OK or ERROR
 * M: changed comment
 *
 * Revision 1.2  2009/01/22 09:30:52  AWanka
 * R: The global mBlk caused memory leak
 * M: Use m_devget instead of a global mBlk
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/

/* notes
  To Access the Ethernet the mux library was used.
  The function LtAttach binds the own protocol with protocol 
  type 0xA000 to the mottsec. The frames are broadcast frames
  with:
  - 6 bytes destination address: FF:FF:FF:FF:FF:FF 
  - 6 bytes source address: FF:FF:FF:FF:FF:FF
  - 2 bytes protocol type: 0xa000
  - 4 bytes packet sequence number: e.g. 0xccccbbb0
  - 4 bytes length of test data: e.g. 0x38
  - 56 bytes - 893 bytes test data: e.g. 00 01 02 03... 

  To send a packet to the mux. The packet must consist of a mBlk chain.
  The function m_devget creates the second mBlk construct with the payload 
  (sequence number, length, test data). The function muxLinkHeaderCreate 
  creates the first mBlk construct containing the destination and source 
  address and the protocol type. The first mBlk will be combined with the 
  second mBlk to a mBlk chain.
*/

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/
#include "vxWorks.h"
#include <MEN/men_typs.h>
#include "etherLib.h"
#include "ifLib.h"
#include "stdio.h"
#include "semLib.h"
#include "tickLib.h"
#include "sysLib.h"
#include "netinet/in.h"
#include "net/ethernet.h"
#include "m2Lib.h"
#include <netinet/tcp.h>
#include "inetLib.h"
#include "muxLib.h"
#include "ipProto.h"
#include "gc_test_sw.h"


/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
#define GC_LT_ETH_TYPE	 	0xa000u		/* ethernet type code for packets */
#define GC_PKTS_PER_RUN 	    10u     /* send and receive 10 packets per 
                                           pass                           */
#define GC_MAX_FRAME_LENGTH   1500u     /* maximum frame length */
#define GC_MIN_FRAME_LENGTH     64u     /* minimum frame length */


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
typedef struct {
	u_int32 seqNum;				/* packet sequence number */
	u_int32 len;				        /* length of test data */
	u_int8 data[GC_MAX_FRAME_LENGTH-4]; /* data to be send */
} GC_LT_PACKET;


typedef struct {
	GC_LT_PACKET packet[GC_PKTS_PER_RUN];	/* received packets */
	int packetIdx;				/* next packet to fill */
	SEM_ID lockSem;				/* semaphore to lock access to rcv pool */
	SEM_ID sigSem;				/* semaphore to post when rcv pool full */
	int init;
} GC_LT_RCV_POOL;


/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/
static GC_LT_RCV_POOL G_rcvPool;	/* global receive pool */
void *G_cookieP;

int G_mBlkChainAttachedCnt = 0;
M_BLK_ID G_saveMBlkP[GC_PKTS_PER_RUN];
M_BLK G_srcAddr; 
M_BLK G_dstAddr;
int G_muxBound = FALSE;

#ifndef VIRTUAL_STACK
IMPORT NET_POOL_ID	_pNetDpool;
#endif /* VIRTUAL_STACK */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static BOOL LtInputHook( void *, long, M_BLK_ID, LL_HDR_INFO *, void *);


/**********************************************************************/
/** Obtain a pointer to the network device.
 *
 * \param netIf \IN network interface number (0-2)
 *
 * \return OK or ERROR
 */
static int LtAttach( int netIf )
{	
	if(G_muxBound == FALSE){
		G_cookieP = muxBind("mottsec", netIf, &LtInputHook, NULL, 
						  NULL, NULL, GC_LT_ETH_TYPE, "MEN looptest", 
						  NULL);
	}
	
	if( G_cookieP == NULL ){
		G_muxBound = FALSE;
		return (ERROR);
	}

	G_mBlkChainAttachedCnt = 0;

	G_muxBound = TRUE;

	return OK;
}

/**********************************************************************/
/** Function that gets called when ethernet frame is received
 *
 * Checks if received packet is a GC loop test frame. If so, copy 
 * the frame to next buffer in G_rcvpool. If all buffers filled,
 * signal semaphore. 
 *
 * \param cookieP      \IN     returned by muxBind()
 * \param type         \IN     protocol type
 * \param mBlkP        \IN     packet with link-level info
 * \param linkHdrInfoP \IN     link-level header info structure
 * \param spareDataP   \IN     spare data
 *
 * \return TRUE or FALSE
 */
static BOOL LtInputHook(
	void *cookieP,    
	long type, 
	M_BLK_ID mBlkP,   
	LL_HDR_INFO *linkHdrInfoP, 
	void *spareDataP
	)
{
	struct ether_header *ethHdr = (struct ether_header *)mBlkP->mBlkHdr.mData;
	GC_LT_PACKET *pkt;

	if( type != GC_LT_ETH_TYPE )
		return FALSE;

	semTake( G_rcvPool.lockSem, WAIT_FOREVER );

	/* copy received packet to receive pool */
	if( G_rcvPool.packetIdx < GC_PKTS_PER_RUN ){
		pkt = (GC_LT_PACKET *)(ethHdr + 1);
		memcpy( (void *)&G_rcvPool.packet[G_rcvPool.packetIdx], 
				(void *)pkt,
				sizeof(GC_LT_PACKET) - 4);

		G_rcvPool.packetIdx++;

		if( G_rcvPool.packetIdx == GC_PKTS_PER_RUN )
			semGive( G_rcvPool.sigSem );
	}
	/* free given Mblk */
    m_freem(mBlkP);
	semGive( G_rcvPool.lockSem );
	
	return TRUE;
}       

/**********************************************************************/
/** Function that resets the receive pool
 *
 * The two Semaphores lockSem and sigSem are created only once. The packet
 * receive counter is set to 0. 
 *
 * \return OK
 */
static int RcvPoolReset(void)
{
	if( !G_rcvPool.init ){
		G_rcvPool.lockSem = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		G_rcvPool.sigSem = semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
		G_rcvPool.init = TRUE;
	}

	semTake( G_rcvPool.lockSem, WAIT_FOREVER );

	semTake( G_rcvPool.sigSem, NO_WAIT );	/* make sure it is empty */
	G_rcvPool.packetIdx = 0;
	
	semGive( G_rcvPool.lockSem );
	return OK;
}

/**********************************************************************/
/** Function that fills the packet
 *
 * The payload including the sequence number, test data length and test data 
 * is stored in the cluster. 
 *
 * \param pktP          \INOUT  pointer to the cluster
 * \param packetLen     \IN     length of test data
 * \param payLoadSeqNum \IN     sequence number
 */
static void LtPacketFill( GC_LT_PACKET *pktP, int packetLen, int payLoadSeqNum )
{
	int i;
	u_int8 *data = pktP->data;

    pktP->seqNum = htons(payLoadSeqNum);
	pktP->len = htons(packetLen - sizeof(pktP->seqNum) - sizeof(pktP->len));

	for( i=0; i<packetLen; i++ )
		*data++ = (u_int8)i;
}

/**********************************************************************/
/** Function that compares the received test data with the expected test data
 *
 * The payload including the sequence number, test data length and test data 
 * is stored in the cluster. 
 *
 * \param pktP          \IN     pointer to the received payload ( test data, 
 *                              sequence number and length
 * \return OK or ERROR
 */
static int LtPacketCmp( GC_LT_PACKET *pktP )
{
	int i;
	u_int32 len = pktP->len;
	u_int8 *data = pktP->data;

	for( i=0; i<len; i++, data++ ){
		if( *data != (u_int8)i ){
			return ERROR;
		}
	}
	return OK;
}

/**********************************************************************/
/** Send a raw ethernet frame for GC_loopback test
 * 
 * Packet payload is dynamically build within this function
 *
 * Packet will be formed as follows:
 * ether_src: 	filled by driver               --
 * ether_dest: 	broadcast address (all FFs)      |--> firstMBlkP
 * ether_type:  0xA000                         --
 * sequence number                             --
 * test data length                              |--> secondMBlkP
 * test data                                   -- 
 * 
 * \param netIf      \IN  network interface number (0-2)
 * \param packetLen  \IN  number of bytes in payload 
 * \param seqNum     \IN  sequence number
 *
 * \return OK or ERROR
 */
static int LtPacketSend( 
	int netIfNumber,
	int packetLen,
	u_int32 seqNum
	)
{
	char param[500];
	struct ifnet *ifNetP;
	M_BLK_ID secondMblkP = NULL;
	M_BLK_ID firstMBlkP = NULL;
	GC_LT_PACKET payLoad;

	if( (packetLen < (sizeof(payLoad.seqNum) - sizeof(payLoad.len))) || 
		(packetLen > GC_MAX_FRAME_LENGTH) ){
		return ERROR;
	}
		  
	ipAttach(netIfNumber, GC_ETH_DRV_NAME);
	sprintf(param, "%s%d", GC_ETH_DRV_NAME, netIfNumber);
	ifNetP = ifunit(param);
	
	LtPacketFill( &payLoad, 
				  packetLen, seqNum );

	/* creates a mBlk/clBlk/cluster construct, the cluster contains the 
       data pointed to by payLoad */
	secondMblkP = m_devget ( (char *)&payLoad, packetLen, 1, ifNetP, NULL);
	if (secondMblkP == NULL){
		return ERROR;
	}
	
	memcpy( G_srcAddr.m_data, "\x00\xc0\x3a\x11\x11\x11", 6 );/* required? */
	memcpy( G_dstAddr.m_data, "\xFF\xFF\xFF\xFF\xFF\xFF", 6 );
	
	G_dstAddr.mBlkHdr.reserved = htons (GC_LT_ETH_TYPE);
	secondMblkP->mBlkHdr.mFlags |= M_BCAST; /* Broadcast ARP messages. */
	secondMblkP->mBlkHdr.reserved = htons (GC_LT_ETH_TYPE);
	
	/* creates a new mBlk chain that begins with an assembled link-level
       header, followed by the mBlk chain pointed to by secondMblkP */
	if ((firstMBlkP = muxLinkHeaderCreate (G_cookieP, secondMblkP,
											&G_srcAddr,
											&G_dstAddr, TRUE)) == NULL)
	{
		netTupleFree( secondMblkP );
		return (ERROR);
	}
	firstMBlkP->mBlkPktHdr.rcvif = NULL;

	/* save all allocated mBlk chains, so they can be freed if the 
	   corresponding packets are received */
	G_saveMBlkP[G_mBlkChainAttachedCnt] = firstMBlkP;
	G_mBlkChainAttachedCnt++;

	return muxSend( G_cookieP, firstMBlkP );
}

/**********************************************************************/
/** Check integrity of received packets (sent by LtPacketSend)
 * 
 * \param seqNum        first sent sequence number
 * \param bytesP        updated with number of correctly received bytes
 * \param debugLevel    1: print error messages only, 0: print all, 2: print
 *                      nothing
 * \return number of errors
 */
static int LtRcvCheck( u_int32 seqNum, u_int32 *bytesP, u_int8 debugLevel )
{
	int i;
	int errCount = 0;
	
	semTake( G_rcvPool.lockSem, WAIT_FOREVER );

	if( G_rcvPool.packetIdx != GC_PKTS_PER_RUN ){
		if(debugLevel == 1 || debugLevel == 0) {
			printf("*** Received too few packets (%d of %d)\n", 
			       G_rcvPool.packetIdx, GC_PKTS_PER_RUN );
		}
		errCount++;
	}
	
	for( i=0; i<G_rcvPool.packetIdx; i++, seqNum++ ){
		/* free MBlk/ClBlk/Cluster of received packets */
		if(i <= G_mBlkChainAttachedCnt)
            netMblkClChainFree( G_saveMBlkP[i] );

		if( seqNum != G_rcvPool.packet[i].seqNum ){
			if(debugLevel == 1 || debugLevel == 0) {
				printf("*** Bad sequence number (0x%08lx, should be 0x%08lx)\n", 
				       G_rcvPool.packet[i].seqNum, seqNum );
			}
			errCount++;
			seqNum = G_rcvPool.packet[i].seqNum; /* resync */
		}

		else if( LtPacketCmp( &G_rcvPool.packet[i] ) != OK ){
			if(debugLevel == 1 || debugLevel == 0) {
				printf("*** Bad data in packet sequence 0x%08lx\n", seqNum );
			}
			errCount++;
		}

		else {
			*bytesP += G_rcvPool.packet[i].len + 22;
		}

	}

	/* free MBlk/ClBlk/Cluster of not received packets */
	while( (i != GC_PKTS_PER_RUN) && (i <= G_mBlkChainAttachedCnt) )
	{
		netMblkClChainFree( G_saveMBlkP[i] );
		i++;
	}
	G_mBlkChainAttachedCnt = 0;

	semGive( G_rcvPool.lockSem );
	return errCount;
}

/**********************************************************************/
/** Ethernet loop test
 * Send a raw ethernet frame for GC_loopback test.
 * This test shall be a generic test that 
 * -	sends raw ethernet frames from the management CPU 
 * -	expects some external loopback (must boot with LB-adapter plugged)
 * -	the test expects the sent frames to be received 
 *		back on the management CPU.
 *
 * \param netIf         network interface number
 * \param duration	    test duration in seconds (defaults to 10 if duration==0)
 * \param minFrameLen   minimum frame length in test (defaults to 60)
 * \param maxFrameLen   maximum frame length in test (defaults to 1500)
 * \param monFunc       function to be called periodically (NULL if none)
 * \param debugLevel    1: print error messages only, 0: print all, 2: print
 *                      nothing
 *
 * \return  0: OK 
 *         -1: ERROR
 *         >0: number of errors
 */
int GC_Loopframes(	
	int netIf,
	int duration, 
	int minFrameLen, 
	int maxFrameLen, 
	int (*monFunc)(void),
	int debugLevel)
{
	u_int32 startTick = tickGet();
	u_int32 lastPrintTick = startTick;
	u_int32 seqNum=0xccccbbb0;
	u_int32 pktNum=0;
	u_int32 bytCount=0;
	int errCount = 0;
	int locErrCnt;
	int i;

	/* validate parameters */
	if( duration == 0 )
		duration = 10;

	if( minFrameLen < GC_MIN_FRAME_LENGTH )
		minFrameLen = GC_MIN_FRAME_LENGTH;

	if( maxFrameLen < GC_MIN_FRAME_LENGTH || maxFrameLen > GC_MAX_FRAME_LENGTH )
		maxFrameLen = GC_MAX_FRAME_LENGTH;

	if( minFrameLen > maxFrameLen )
		minFrameLen = maxFrameLen;

	/* bind own protocol */
	if( LtAttach(netIf) == ERROR ){
		if(debugLevel != 2) {
		    printf("*** Can't bind to mux\n");
		}
		return ERROR;
	}

	/* run test */
	while( (tickGet() - startTick) < (duration * sysClkRateGet() )){
	    if( RcvPoolReset() != OK ){
			if(debugLevel != 2) {
				printf("*** Can't init receive pool\n");
			}
            errCount = ERROR;
			goto ABORT;
		}

		/* send packets */
		for( i=0; i<GC_PKTS_PER_RUN; i++ ){
			int pktLen = minFrameLen + 
				(((maxFrameLen - minFrameLen) / GC_PKTS_PER_RUN) * i );

			if( LtPacketSend( netIf, pktLen, seqNum+i ) == (ERROR || ENETDOWN) ){
				if(debugLevel == 1 || debugLevel == 0) {
					printf("*** Error sending pkt seqNum=0x%08lx\n", seqNum+i );
				}
				errCount++;
			}
		}
		
		/* call monitor function if any */
		if( monFunc ){
			if( monFunc()!= OK )
				errCount++;
		}

		/* wait for packets */
		semTake( G_rcvPool.sigSem,  sysClkRateGet() / 20 );

		/* verify received packets */
		if( (locErrCnt = LtRcvCheck( seqNum, &bytCount, debugLevel )) != 0 )
			errCount += locErrCnt;


		seqNum += GC_PKTS_PER_RUN;
		pktNum += GC_PKTS_PER_RUN;
		
		if( (tickGet() - lastPrintTick) >= (1 * sysClkRateGet())){
			lastPrintTick = tickGet();

			if( debugLevel == 0 ) {
				printf( "%10ld packets %ld bytes %ld kBytes/s...\n", 
						pktNum,
						bytCount,
						bytCount/1/1024);
			}
			bytCount = 0;
		}
	} 
	if( errCount && (debugLevel == 1 || debugLevel == 0) ) {
		printf("%d total errors in %ld packets\n", errCount, pktNum);
	}

	/* free MBlk/ClBlk/Cluster of not received packets */
	while( G_mBlkChainAttachedCnt )
	{
		netMblkClChainFree( G_saveMBlkP[G_mBlkChainAttachedCnt] );
		G_mBlkChainAttachedCnt--;
	}

	G_muxBound = FALSE;
	muxUnbind(G_cookieP, GC_LT_ETH_TYPE, &LtInputHook);


	return errCount;
	
 ABORT:
	/* free MBlk/ClBlk/Cluster of not received packets */
	while( G_mBlkChainAttachedCnt )
	{
		netMblkClChainFree( G_saveMBlkP[G_mBlkChainAttachedCnt] );
		G_mBlkChainAttachedCnt--;
	}

	G_muxBound = FALSE;
	muxUnbind(G_cookieP, GC_LT_ETH_TYPE, &LtInputHook);
	return errCount;
}

