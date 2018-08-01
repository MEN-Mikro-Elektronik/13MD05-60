/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  eth_looptest.c
 *
 *      \author  aw
 *        $Date: 2012/09/06 15:32:01 $
 *    $Revision: 1.2 $
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
 * $Log: eth_looptest.c,v $
 * Revision 1.2  2012/09/06 15:32:01  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:05  MKolpak
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/

/* notes
  This test sets the interface to 100MBit. Packets must be looped back to
  the interface (with i.e. loop stub).
  
  To Access the Ethernet the mux library was used.
  The function LfAttach binds the own protocol with protocol 
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
#include <string.h>
#include <vxWorks.h>
#include <MEN/men_typs.h>
#include <etherLib.h>
#include <endLib.h>
#include <ifLib.h>
#include <net/mbuf.h>
#include <stdio.h>
#include <semLib.h>
#include <tickLib.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <m2Lib.h>
#include <netinet/tcp.h>
#include <inetLib.h>
#include <muxLib.h>
#include <ipProto.h>
#include <vxBusLib.h>
#include <hwif/vxbus/vxBus.h>
#include <miiLib.h>
#include <end.h>

#include "sysLib.h"

/* ### error reporting and debugging ###*/
#define DBG_MYLEVEL	g_looptest_verbosity
#include <MEN/dbg.h>        /* debug module */

#undef DEBUG_LOCAL
#ifdef DEBUG_LOCAL
	#define debug(fmt,args...) printf("%s - ",__FUNCTION__);printf (fmt ,##args)
	#define locate() printf("%s: line %u\n",__FILE__,__LINE__);OSS_Delay(NULL,1)	
	#define error(fmt,args...) printf("### %s - line %u: ",__FUNCTION__,__LINE__); \
								printf (fmt ,##args);OSS_Delay(NULL,1)
#else
	#define debug(fmt,args...) if(DBG_MYLEVEL)DBG_Write(DBH,"%s - "fmt,__FUNCTION__,##args)
	#define locate()
	#define error(fmt,args...) DBG_Write(DBH,"### %s - line %u: "fmt,__FUNCTION__,__LINE__,##args);
#endif

#undef DEBUG_ACCESS
#ifdef DEBUG_ACCESS
#define LF_PHY_WRITE(addr,reg,val) MiiPhyWrite(vxbMiiDev,addr,reg,val); \
				debug("PHY 0x%x W R:0x%02X D:0x%04X\n",addr,reg,val)
#define LF_PHY_READ(addr,reg,var) MiiPhyRead(vxbMiiDev,addr,reg,var); \
				debug("PHY 0x%x R R:0x%02X D:0x%04X\n",addr,reg,*var)
#else
#define LF_PHY_WRITE(addr,reg,val) MiiPhyWrite(vxbMiiDev,addr,reg,val)
#define LF_PHY_READ(addr,reg,var) MiiPhyRead(vxbMiiDev,addr,reg,var)
#endif

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/

#define PKTS_PER_RUN 	    10u     /* send and receive 10 packets per 
                                           pass                           */
#define MAX_FRAME_LENGTH   1500     /* maximum frame length */
#define LT_ETH_TYPE	 	   0x9000   /* ethernet type code for packets */

#define MIN_FRAME_LENGTH   64u     /* minimum frame length */


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
typedef struct {
	u_int32 seqNum;				/* packet sequence number */
	u_int32 len;				        /* length of test data */
	u_int8 data[MAX_FRAME_LENGTH-8]; /* data to be send */
} LT_PACKET;


typedef struct {
	LT_PACKET packet[PKTS_PER_RUN];	/* received packets */
	int packetIdx;				/* next packet to fill */
	SEM_ID lockSem;				/* semaphore to lock access to rcv pool */
	SEM_ID sigSem;				/* semaphore to post when rcv pool full */
	int init;
} LT_RCV_POOL;


/*-----------------------------------------+
|  MODULE GLOBALS                          |
+-----------------------------------------*/
#define DBH sysDbgHdlP

extern DBG_HANDLE * sysDbgHdlP;
static LT_RCV_POOL rcvPool;	/* global receive pool */
static LT_PACKET payLoad;
static M_BLK srcAddr; 
/* static M_BLK dstAddr;*/
static FUNCPTR MiiPhyRead,MiiPhyWrite,MiiMediaUpdate,MiiDevConnect,MiiUnlink;
static M_BLK_ID saveMBlkP[PKTS_PER_RUN];
static void *cookieP;
static int mBlkChainAttachedCnt = 0;
static int muxBound = FALSE;
static char * ifName = "quitelongnameforaninterface";
static u_int8 ifNum = 0;
static u_int8 phyAddr = 0;
/* static struct ifnet * ifNetP; */
static NET_POOL_ID	 netpool;
static VXB_DEVICE_ID vxbMiiDev;

/* test */
static u_int32 stLen = 1450;


/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/
u_int32 g_looptest_verbosity = 0;


enum
{
	PHY_ADDR_DTSEC2 = 0x1e,
	PHY_ADDR_DTSEC3 = 0x1f
};
#define PHY_ADDR_GEI 0x1;

#if 0
#ifndef VIRTUAL_STACK
IMPORT NET_POOL_ID	_pNetDpool;
#endif /* VIRTUAL_STACK */
#endif

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static BOOL LfRcvHook( void *, long, M_BLK_ID, LL_HDR_INFO *, void *);
static void LfState(END_OBJ* pEnd,END_ERR* pError, void* pSpare);
static void LfMblkInfo(char *,M_BLK_ID mblk);

void LfIfStats(void);
int LfInit(char * ifName,int ifNum);


static int LfAttach( int netIf )
{	
	long data = 0;
	int ret = 0;

	if(muxBound == FALSE){
		cookieP = muxBind(
			ifName, 
			ifNum, 
			&LfRcvHook, 
			NULL, 
			NULL, 
			&LfState, 
			MUX_PROTO_SNARF,
		/*	LT_ETH_TYPE, */
			"MEN netlooptest", 
			NULL);
	}
	
	if( cookieP == NULL ){
		muxBound = FALSE;
		return (ERROR);
	}

	/* set proiscous mode */
#if 0
	ret = muxIoctl(cookieP,EIOCGFLAGS,(caddr_t) &data);
	if(ret != OK)
	{
		error("muxIoctl (%u:%u)\n",ret,errnoGet()>>16,errno&0xFFFF);
		return ERROR;
	}else
		debug("muxIoctl flags 0x%X)\n",data);
	data |= IFF_PROMISC;
	muxIoctl(cookieP,EIOCSFLAGS,(caddr_t) data);
#endif
	mBlkChainAttachedCnt = 0;

	muxBound = TRUE;

	return OK;
}

/**********************************************************************/
/** Function that gets called when ethernet frame is received
 *
 * Checks if received packet is a GC loop test frame. If so, copy 
 * the frame to next buffer in rcvPool. If all buffers filled,
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
static BOOL LfRcvHook(
	void *cookieP,    
	long type, 
	M_BLK_ID mBlkP,   
	LL_HDR_INFO *linkHdrInfoP, 
	void *spareDataP
	)
{
	struct ether_header *ethHdr = (struct ether_header *)mBlkP->mBlkHdr.mData;
	LT_PACKET *pkt;

	if( type != LT_ETH_TYPE )
		return FALSE;

	semTake( rcvPool.lockSem, WAIT_FOREVER );

	/* copy received packet to receive pool */
	if( rcvPool.packetIdx < PKTS_PER_RUN ){
		pkt = (LT_PACKET *)(ethHdr + 1);
		memcpy( (void *)&rcvPool.packet[rcvPool.packetIdx], 
				(void *)pkt,
				sizeof(LT_PACKET) - 4);

		rcvPool.packetIdx++;

		if( rcvPool.packetIdx == PKTS_PER_RUN )
			semGive( rcvPool.sigSem );
	}
	/* free given Mblk */
    m_freem(mBlkP);
	semGive( rcvPool.lockSem );
	
	return TRUE;
}       


static void LfState
(
END_OBJ* pEnd,  /* pointer to END_OBJ */
END_ERR* pError, /* pointer to END_ERR */
void* pSpare    /* pointer to protocol private data passed in muxBind */
) 
{
	error("%s%u: %s (0x%x)\n",
			pEnd->devObject.name,
			pEnd->devObject.unit,
			pError->pMesg,
			pError->errCode);
}

static void LfMblkInfo(char * info,M_BLK_ID mblk)
{
	debug("%s\n"
		"netpool....................0x%08X\n"
		"mblk->mBlkHdr.mData..........0x%08X\n"
		"mblk->mBlkHdr.mType..........0x%08X\n"
		"mblk->mBlkHdr.mLen...........0x%08X\n"
		"mblk->mBlkHdr.mNext..........0x%08X\n"
		"mblk->mBlkHdr.mNextPkt.......0x%08X\n"
		"mblk->mBlkPktHdr.len.........0x%08X\n"
		"mblk->mBlkPktHdr.header......0x%08X\n"
		"mblk->pClBlk->clSize.........0x%08X\n"
		"mblk->pClBlk->clRefCnt.......0x%08X\n"
		"mblk->pClBlk->pNetPool.......0x%08X\n"
		"pMblk->pClBlk->clNode.pClBuf.0x%08X\n",
		info,
		netpool,
		mblk->mBlkHdr.mData,
		mblk->mBlkHdr.mType,
		mblk->mBlkHdr.mLen,
		mblk->mBlkHdr.mNext,
		mblk->mBlkHdr.mNextPkt,
		mblk->mBlkPktHdr.len,
		mblk->mBlkPktHdr.header,		
		mblk->pClBlk->clSize,
		mblk->pClBlk->clRefCnt,
		mblk->pClBlk->pNetPool,
		mblk->pClBlk->clNode.pClBuf
		);
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
	if( !rcvPool.init ){
		rcvPool.lockSem = semBCreate( SEM_Q_PRIORITY, SEM_FULL );
		rcvPool.sigSem = semBCreate( SEM_Q_PRIORITY, SEM_EMPTY );
		rcvPool.init = TRUE;
	}

	semTake( rcvPool.lockSem, WAIT_FOREVER );

	semTake( rcvPool.sigSem, NO_WAIT );	/* make sure it is empty */
	rcvPool.packetIdx = 0;
	
	semGive( rcvPool.lockSem );
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
static void LfPacketFill( LT_PACKET *pktP, int packetLen )
{
	int i;
	u_int8 *data = pktP->data;
	pktP->len = htons(packetLen - 8);

	for( i=0; i < (packetLen - 8); i++ )
		*data++ = (u_int8)i;
}

#if 0
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
static int LfPacketCmp( LT_PACKET *pktP )
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
#endif

/**********************************************************************/
/** Send a raw ethernet frame for loopback test
 * 
 * Packet payload is dynamically build within this function
 *
 * Packet will be formed as follows:
 * ether_src: 	filled by driver               --
 * ether_dest: 	broadcast address (all FFs)      |--> frameBlkP
 * ether_type:  0xA000                         --
 * sequence number                             --
 * test data length                              |--> payloadBlkP
 * test data                                   -- 
 * 
 * \param netIf      \IN  network interface number (0-2)
 * \param packetLen  \IN  number of bytes in payload 
 * \param seqNum     \IN  sequence number
 *
 * \return OK or ERROR
 */
static int LfPacketSend( 
	int netIfNumber,
	int packetLen,
	u_int32 seqNum
	)
{
	M_BLK_ID payloadBlkP = NULL;
	M_BLK_ID frameBlkP = NULL;
	int ret = 0;

	locate();
	if( (packetLen < (sizeof(payLoad.seqNum) - sizeof(payLoad.len))) || 
		(packetLen > MAX_FRAME_LENGTH) ){
		return ERROR;
	}
	locate();
	/* get a tuple from end pool */
    payloadBlkP = endPoolTupleGet(netpool);
	if (payloadBlkP == NULL)
	{
		error("netTupleGet payloadBlkP (%u:%u)\n",errnoGet()>>16,errno&0xFFFF);
		return ERROR;
	}
	payloadBlkP->mBlkHdr.mFlags |= M_BCAST; /* Broadcast messages. */
	payloadBlkP->mBlkHdr.reserved = stLen++ /* test htons (LT_ETH_TYPE); */
	locate();
	
	if(g_looptest_verbosity)
		if(seqNum == 0)
				LfMblkInfo("endPoolTupleGet payloadBlkP",payloadBlkP);

	/* set sequence number */
	payLoad.seqNum = seqNum;
    /* test */
    // payLoad.len = stLen++;*/
    
	/* copy payload to buffer */
	memcpy(payloadBlkP->mBlkHdr.mData,&payLoad,MAX_FRAME_LENGTH);
	/* creates a new mBlk chain that begins with an assembled link-level
       header, followed by the mBlk chain pointed to by payloadBlkP */
    if ((frameBlkP = muxAddressForm(
			cookieP,
			payloadBlkP,
			&srcAddr, /* both srcAddr - dst is set in next step */
			&srcAddr)) == NULL)
	{
		error("muxAddressForm frameBlkP (%u:%u)\n",errnoGet()>>16,errno&0xFFFF);
		ret = ERROR;
		goto err;
	}
	locate();
	/* set destination address */
    frameBlkP->mBlkHdr.mData[0] = 0xFF;
    frameBlkP->mBlkHdr.mData[1] = 0xFF;
	frameBlkP->mBlkHdr.mData[2] = 0xFF;
	frameBlkP->mBlkHdr.mData[3] = 0xFF;
	frameBlkP->mBlkHdr.mData[4] = 0xFF;
	frameBlkP->mBlkHdr.mData[5] = 0xFF;
		
	if(g_looptest_verbosity)
		if(seqNum == 0)
		    LfMblkInfo("muxAddressForm frameBlkP",frameBlkP);
	/* save all allocated mBlk chains, so they can be freed if the 
	   corresponding packets are received */
		locate();


	if ((ret = muxSend( cookieP, frameBlkP )) != OK){
		error("muxSend (0x%x,%u:%u)\n",ret,errnoGet()>>16,errno&0xFFFF);
		endPoolTupleFree( frameBlkP );
		ret = ERROR;
		goto err;
	}
	saveMBlkP[mBlkChainAttachedCnt++] = frameBlkP;
	return ret;
err:
	if(frameBlkP)
		endPoolTupleFree(frameBlkP);
	else if(payloadBlkP)
		endPoolTupleFree(payloadBlkP);
	
	return ret;
}

/**********************************************************************/
/** Check integrity of received packets (sent by LfPacketSend)
 * 
 * \param seqNum        first sent sequence number
 * \param bytesP        updated with number of correctly received bytes
 * \param debugLevel    1: print error messages only, 0: print all, 2: print
 *                      nothing
 * \return number of errors
 */
#define LT_PACKET_DUMP
static int LfRcvCheck( u_int32 seqNum, u_int32 *bytesP, u_int8 debugLevel )
{
	int i,j;
	int errCount = 0;
    u_int8 * data = NULL;
	
	if(semTake( rcvPool.lockSem, WAIT_FOREVER ) != OK)
	{
		error("semTake\n");
		return ERROR;
	}	

	if( rcvPool.packetIdx != PKTS_PER_RUN ){
		error("Received too few packets (%d of %d)\n", 
			       rcvPool.packetIdx, PKTS_PER_RUN );
		errCount++;
	}
    	
	for( i=0; i<rcvPool.packetIdx; i++, seqNum++ ){
		/* free MBlk/ClBlk/Cluster of received packets */
		if(i <= mBlkChainAttachedCnt)
            netMblkClChainFree( saveMBlkP[i] );

		if( seqNum != rcvPool.packet[i].seqNum ){
			error("Bad sequence number (0x%08lx, should be 0x%08lx)\n", 
				       rcvPool.packet[i].seqNum, seqNum );
			errCount++;
			seqNum = rcvPool.packet[i].seqNum; /* resync */
		}
#ifdef	LT_CHECK_DATA		
		else if( LfPacketCmp( &rcvPool.packet[i] ) != OK ){
			error("Bad data in packet sequence 0x%08lx\n", seqNum );
			errCount++;
		}
#endif
		else {
			*bytesP += rcvPool.packet[i].len + 22;
		}
#ifdef LT_PACKET_DUMP
        data = &rcvPool.packet[i].data;
        printf("Packet (s:%u,l:%u) dump:\n",
            rcvPool.packet[i].seqNum,
            rcvPool.packet[i].len);
        j = 0;
        do
        {
            if(!j%8)
                printf("%u\t",j);
            printf(" %02x",data[j]);
            if(!j%8)
                printf("\n");
            j++;
        }while(j < rcvPool.packet[i].len);
#endif
	}

	/* free MBlk/ClBlk/Cluster of not received packets */
	while( (i != PKTS_PER_RUN) && (i <= mBlkChainAttachedCnt) )
	{
		netMblkClChainFree( saveMBlkP[i] );
		i++;
	}
	mBlkChainAttachedCnt = 0;

	semGive( rcvPool.lockSem );
	return errCount;
}



/**********************************************************************/
/** Ethernet loop test
 * Send a raw ethernet frame for loopback test.
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
 * \param monFunc       function to be called with the byteRate (NULL if none)
 * \param debugLevel    1: print error messages only, 0: print all, 2: print
 *                      nothing
 *
 * \return  0: OK 
 *         -1: ERROR
 *         >0: number of errors
 */
int Loopframes(	
	char * name,
	int unit,
	int duration,  
	int debugLevel,
	int minFrameLen, 
	int maxFrameLen, 
	void (*monFunc)(u_int32))
{
	int ret = 0;
	u_int32 outtick = tickGet();
	u_int32 starttick = outtick;
	u_int32 seqNum=0;
	u_int32 pktNum=0;
	u_int32 bytCount=0;
	int errCount = 0;
	int locErrCnt = 0;
	int i = 0;

#ifdef	DEBUG_ACCESS
	printf("%s: DEBUG_ACCESS!\n",__FUNCTION__);
#endif

	/* validate parameters */
	if( duration == 0 )
		duration = 1;

	debug("name:%s unit:%u dur:%u dbg:%u\n",
			name,
			unit,
			duration,
			debugLevel);

	/* init done? */
	if(strncmp(ifName,name,strlen(name)))
	{
		locate();
		if(LfInit(name,unit) != OK)
        {
            errCount = 1;
			goto ABORT;
        }
	}

	locate();

    /* test */
    stLen = 1400;
	
	/* run test */
	starttick = tickGet();
	while( tickGet()  < (starttick + duration * sysClkRateGet() )){
	    if( RcvPoolReset() != OK ){
			error("Can't init receive pool\n");
            errCount = ERROR;
			goto ABORT;
		}
		locate();
		/* send packets */
		for( i=0; i<PKTS_PER_RUN; i++ )
		{
#if 0
			int pktLen = minFrameLen + 
				(((maxFrameLen - minFrameLen) / PKTS_PER_RUN) * i );
#endif
			if( LfPacketSend( 0, MAX_FRAME_LENGTH, seqNum+i ) != OK)
			{
				error("Error sending pkt seqNum=0x%08lx\n", seqNum+i );
				errCount++;
			}
		}
		
		locate();
		/* wait for packets */
		semTake( rcvPool.sigSem,  sysClkRateGet() / 20 );

		/* verify received packets */
		if( (locErrCnt = LfRcvCheck( seqNum, &bytCount, debugLevel )) != 0 )
			errCount += locErrCnt;

		seqNum += PKTS_PER_RUN;
		pktNum += PKTS_PER_RUN;

		/* call monitor function if any */
		if( monFunc )
		{
			(*monFunc)(pktNum*MAX_FRAME_LENGTH);
		}

		if( tickGet() >= (outtick + sysClkRateGet())){
			outtick = tickGet();
			if(debugLevel)
				printf( "%10ld packets %ld bytes %ld kBytes/s...\n", 
						pktNum,
						bytCount,
						bytCount/1/1024);
			else
				debug( "%10ld packets %ld bytes %ld kBytes/s...\n", 
						pktNum,
						bytCount,
						bytCount/1/1024);
			bytCount = 0;
		}
	} 
	if( errCount && (debugLevel) ) {
		printf("%d total errors in %ld packets\n", errCount, pktNum);
	}

	/* free MBlk/ClBlk/Cluster of not received packets */
	while( mBlkChainAttachedCnt )
	{
		netMblkClChainFree( saveMBlkP[mBlkChainAttachedCnt] );
		mBlkChainAttachedCnt--;
	}
	
 ABORT:
	/* free MBlk/ClBlk/Cluster of not received packets */
	while( mBlkChainAttachedCnt )
	{
		netMblkClChainFree( saveMBlkP[mBlkChainAttachedCnt] );
		mBlkChainAttachedCnt--;
	}
#if 0
	muxBound = FALSE;
	muxUnbind(cookieP, LT_ETH_TYPE, &LfRcvHook);
	endPoolDestroy(netpool);
#endif
	return errCount;
}

int LfInit(char * name,int idx)
{
	u_int16 phyData = 0;
	u_int32 starttick = 0;

	/* set globals */
	strcpy(ifName,name);
	ifNum = idx;

	/* get phy addr */
	/* get this from hwconf? */
	/* TODO: get this from OS - this is AE52 dependent */
    if(!strncmp(ifName,"dtsec",strlen(ifName)))
    {
    	switch(ifNum)
    	{
    		case 2:
    			phyAddr = PHY_ADDR_DTSEC2; 
    			break;
    		case 3:
    			phyAddr = PHY_ADDR_DTSEC3;
    			break;
    		default:
    			error("ifNum %u no PHY-Addr\n",ifNum);
    			return ERROR;
    	}
    }else if(!strncmp(ifName,"gei",strlen(ifName)))
    {
        phyAddr = PHY_ADDR_GEI;
    }else
    {
        error("ifName %s no unknown\n",ifName);
    }
        
        

	/* prepare packet */
	LfPacketFill( &payLoad, 
				  MAX_FRAME_LENGTH );
	
	/* get vxBus device */
	vxbMiiDev = vxbInstByNameFind (ifName, ifNum); 
	if(vxbMiiDev == NULL)
	{
		error("vxbInstByNameFind\n");
		return ERROR;
	}	
	
	/* get access functions */
	MiiPhyRead = vxbDevMethodGet (vxbMiiDev, DEVMETHOD_CALL(miiRead));
	MiiPhyWrite = vxbDevMethodGet (vxbMiiDev, DEVMETHOD_CALL(miiWrite)); 
	MiiMediaUpdate = vxbDevMethodGet (vxbMiiDev, DEVMETHOD_CALL(miiMediaUpdate)); 
	MiiDevConnect = vxbDevMethodGet (vxbMiiDev, DEVMETHOD_CALL(muxDevConnect)); 
	MiiUnlink = vxbDevMethodGet (vxbMiiDev, DEVMETHOD_CALL(vxbDrvUnlink)); 

	while(!muxUnbind(
		cookieP, 
		MUX_PROTO_SNARF,
		&LfRcvHook));
	muxBound = FALSE;
	

#if 0
	/* detach */
	MiiUnlink(vxbMiiDev);
	/* start polling stats */
	_func_m2PollStatsIfPoll = &LfIfStats;
	MiiDevConnect(vxbMiiDev);
	OSS_Delay(0,500);
#endif
	
	memcpy( srcAddr.m_data, "\x00\xc0\x3a\x11\x11\x11", 6 );
	srcAddr.mBlkHdr.reserved = htons (LT_ETH_TYPE);

	/* init netBufLib */
	if ((endPoolCreate(PKTS_PER_RUN, &netpool)) == ERROR)
	{
		error("endPoolCreate\n");
		return ERROR;
	}
	
	/* bind own protocol */
	if( LfAttach(0) != OK ){
		error("*** Can't bind to mux\n");
		return ERROR;
	}

	/* set speed to 100FD */
#define CHANGE_SPEED
#ifdef CHANGE_SPEED
	LfEthModeSet(PHY_100BASE_TX_FDX);
#endif
	/* wait for link */
	starttick = tickGet();
	while(tickGet() < (starttick + 6*sysClkRateGet()))
	{
		OSS_Delay(0,1000);
		LF_PHY_READ(phyAddr,MII_STAT_REG,&phyData);
		if(phyData & MII_SR_LINK_STATUS)
			break;
	}
	if(!(phyData & MII_SR_LINK_STATUS))
	{
		error("no link\n");
		return ERROR;
	}	
	/* propagate changes */
	MiiMediaUpdate(vxbMiiDev);

	/* have a break */
	OSS_Delay(0,500);

	return OK;
	
}


/* use defines from miiLib.h */
int LfEthModeSet(u_int32 mode)
{
	u_int16 phyData = 0;
	int ret = 0;

	/* read current config */
	ret = LF_PHY_READ(phyAddr,MII_CTRL_REG,&phyData);
	if(ret != OK)
	{
		error("MiiPhyRead\n");
		return ERROR;
	}

	switch(mode)
	{
		case PHY_100BASE_TX_FDX:     /* 100 Base TX, full duplex */
			phyData &= ~MII_CR_AUTO_EN; 
			phyData |= MII_CR_100 | MII_CR_FDX | MII_CR_RESET;
			break;
		case PHY_AN_ENABLE:         
			phyData |= MII_CR_AUTO_EN | MII_CR_RESET;
			break;
		case PHY_10BASE_T:           /* 10 Base-T */
			/* fall through */
		case PHY_10BASE_T_FDX:       /* 10 Base Tx, full duplex */
			/* fall through */
		case PHY_100BASE_TX:         /* 100 Base Tx */
			/* fall through */
		default:
			error("mode not supported\n");
			return ERROR;
	}

	/* set config*/
	ret = LF_PHY_WRITE(phyAddr,MII_CTRL_REG,phyData);
	if(ret != OK)
	{
		error("MiiPhyWrite\n");
		return ERROR;
	}
	return OK;
	
}


