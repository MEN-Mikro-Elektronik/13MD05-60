/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  mvbcomm.c
 *
 *      \author  $
 *        $Date: 2007/12/14 14:44:58 $
 *    $Revision: 1.2 $
 *
 *  	 \brief  Hardware test program for verifying PP04 IRQs and MVB
 *				 Communication to Siemens ISA/104 Slave and SIMIS Busadmin
 * 
 * The used MVB Port definitions according to the one given from the SIMIS:
 *
 *	port| size |  cycle |        name | source |   tack|
 *	    |[byte]|   [ms] |             |        |       |
 *	----+------+--------+-------------+--------+-------|--------
 * 	 14 |   32 |    256 | 'ATO_ATS_S' | SlaveB | 0FFDFH|
 *	 15 |   32 |   1024 | 'ATO_DGP_S' | SlaveB | 0FF7FH|
 *	 16 |   32 |    128 | 'ATO_MMI_M' | SlaveB | 0FFEFH|
 *	 17 |    8 |     32 | 'ATO_TMS_F' | SlaveB | 0FFFBH|
 *	 18 |   32 |   1024 | 'ATO_TMS_S' | SlaveB | 0FF7FH|
 *	890 |    8 |     16 | 'VCU_ATO_F' | SlaveA | 0FFFBH|
 *	891 |   32 |    128 | 'VCU_ATO_M' | SlaveA | 0FFDFH|
 *
 *     Switches: -
 *				 
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mvbcomm.c,v $
 * Revision 1.2  2007/12/14 14:44:58  ts
 * Cosmetic defines added
 *
 * Revision 1.1  2005/03/21 12:55:40  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <vxWorks.h>
#include <timers.h>
#include <intLib.h>
#include <iv.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciIntLib.h>

#include "config.h"
#include "host_dep.h"
#include "dpr_dep.h"
#include "cl1_ret.h"
#include "cl1_1.h"
#include "irqtest.h"


/*------------------+
| DEFINES           |
+------------------*/

#define MMAP_TM_LEN		0xFFFE
#define  OUTREG(reg, val ) 	writeWordToTS((void*)G_pp04PciDev.bar1 + reg, val)
#define  INREG(reg)   		readWordFromTS((void*)G_pp04PciDev.bar1 + reg )

#define  CHK(expression) \
 if( (expression)) {\
	 printf("\n*** Error during: %s\nfile %s\nline %d\n", \
      #expression, __FILE__ , __LINE__ );\
     goto abort;\
 }


#define MVB_PRT_INDX_MAX		255	/* max portnumber to set in Asic 	*/
#define MVB_PRT_COUNT			7	/* How many ports are set up at all */
#define NR_SLAVES_B    			5	/* Number of Sourceports we have	*/
#define NR_SLAVES_A    			2	/* Number of Sinkports we have 		*/
#define MVB_IRQ_TEST_COUNT		10	/* how often IRQs occur in 1st test */
#define MVB_MAX_ERRORS			5	/* after so many errors stop loop 	*/

/* MVB Addresses of us and the opponent Slave as expected by the SIMIS */
#define MVB_DA_SLAVEA   		0x004d
#define MVB_DA_SLAVEB   		0x0001

#define MVB_TS_ID				0	/* Traffic store ID 				*/
#define MVB_MAX_RX_TIME_DIFF	2  	/* time in s to wait for data 		*/ 

#define WDC_INIT_VALUE			500 /* Watchdog reload Value (millisec) */
#define RX_RUNS_DEFAULT		100	/* how often to receive a byte 		*/

#define	PCI_VENDOR_ID_ALTERA	0x1172
#define PCI_DEVICE_ID_PP04		0x000c

#define MVB_MCM_MODE_16K		0
#define MVB_MCM_MODE_32K		1
#define MVB_MCM_MODE_64K		2
#define MVB_MCM_MODE_256K		3



/*--------------------------------------+
| TYPEDEFS                              |
+--------------------------------------*/


static const struct MVB_PORT {
	unsigned int number;
	unsigned int size;
	unsigned int cycle;
	unsigned int idx;
	char *name;
	char *source;
	unsigned int tack;
} G_ports[]={
	{  14, 32,  256,  4  ,"ATO_ATS_S", "SlaveB", 0xffdf	},
	{  15, 32, 1024,  8  ,"ATO_DGP_S", "SlaveB", 0xff7f	},
	{  16, 32,  128, 12  ,"ATO_MMI_M", "SlaveB", 0xffef	},
	{  17,  8,   32, 24  ,"ATO_TMS_F", "SlaveB", 0xfffb	},
	{  18, 32, 1024, 16  ,"ATO_TMS_S", "SlaveB", 0xff7f	},
	{ 890,  8,   16, 25  ,"VCU_ATO_F", "SlaveA", 0xfffb	},
	{ 891, 32,  128, 20  ,"VCU_ATO_M", "SlaveA", 0xffdf	}
};

typedef struct {
	unsigned int 	bus;
	unsigned int 	dev;
	unsigned int 	fct;
	unsigned char 	irq;
	unsigned int  	bar0;
	unsigned int  	bar1;
	unsigned int  	bar2;
	unsigned int  	bar3;
	unsigned int  	bar4;
	unsigned int  	bar5;
} PP04_PCIDEV;


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

static timer_t 	G_timerid; 
static struct timespec G_timeLastData;

static unsigned short G_v1old = 0, G_v1new = 0;
static unsigned short G_v2old = 0, G_v2new = 0;
static volatile unsigned int G_errCount		= 0;
static volatile unsigned int G_irqTestOK 	= 0;
static volatile unsigned int G_IsrCount 	= 0;
static volatile unsigned int G_LeaveMVBLoop	= 0;

/* the central buffer for outgoing and incoming MVB Frames */
static unsigned short G_bufTx[5][32];
static unsigned short G_bufRx[2][32]; 

/* the central entity to keep the Info about PP04 */
static PP04_PCIDEV G_pp04PciDev = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const char *usage_str = "\n"
"Syntax:    mvbtest_main <num>\n"
"Function: PP04+PP01/KM01 MVB Asic Test\n"
"Options:\n"
"    <num>  number of passes to receive data (default: 100)\n";



/*--------------------------------------+
| DECLARATIONS                          |
+--------------------------------------*/

static void pp04_isr( int dummy );




/***************************************************************
 ** usage info
 *
 */
static int usage( int no )
{

	printf(  "%s", usage_str );
	return 0;

}


/***************************************************************
 *
 * usage info
 *
 */
static void MVB_InitGlobals(void)
{

	G_v1old 		= 0;
	G_v1new 		= 0;
	G_v2old 		= 0;
	G_v2new 		= 0;
	G_errCount		= 0;
	G_irqTestOK 	= 0;
	G_IsrCount 		= 0;
	G_LeaveMVBLoop	= 0;

}



/**********************************************************************/
/** Initialize the information for one MVB port (class 1.1)
 * 
 * \param ts_cfgP	address of MVB Port struct to hook port in
 * \param port		port number (to be equal over whole system)
 * \param idx		Index of this port in TM 
 * \param size		size of the port in byte (Class 1.1)
 * \param type		if port is a source (send out) or sink
 *
 * \return -
 */
static void MVB_InitTsStruct( TYPE_LP_PRT_CFG *ts_cfgP, 
					   UNSIGNED16 port, 
					   UNSIGNED16 idx, 
					   UNSIGNED16 size, 
					   UNSIGNED16 type )
{

	ts_cfgP->prt_addr = port;
  	ts_cfgP->prt_indx = idx;
  	ts_cfgP->size     = size;
  	ts_cfgP->type     = type;

}



/**********************************************************************/
/** Initialize a TX Buffer with some test values
 * 
 * \param idx	   	address of Value within the single Port 
 * \param val	   	Value to fill into TM, is sent to other Slave
 *
 * \return -
 */
static void MVB_InitTxBuf(unsigned int idx, unsigned int val)
{

#if 0
	G_bufTx[idx][2] = (val << (1+idx)) & 0xffff;

#elif 0
	unsigned int i, cnt=1;
	for (i = 1+val; i < (G_ports[idx].size-1)*2; i+=2 ){
		G_bufTx[idx][cnt++] = (i<<8) | (i+1 ) ;
	}
#elif 1	
	unsigned int i;
	for (i = 0; i < G_ports[idx].size; i++ )
		G_bufTx[idx][i] = val;
#endif

}



/**********************************************************************/
/** Shutdown all IRQ sources on the MVB Asic
 * 
 */
static void MVB_DisableMVBCS1Interrupts(void)
{

	/*
	 *	disable Interrupts by setting Interrupt Mask Register to 0 and
	 *	Interrupt pending Register to 0
	 */	
	OUTREG( MVB_REG_IMR0, 0x0000 );
	OUTREG( MVB_REG_IMR1, 0x0000 );
	OUTREG( MVB_REG_ISR0, 0x0000 );
	OUTREG( MVB_REG_ISR1, 0x0000 );

}




/**********************************************************************/
/** perform cleanup tasks after successful or failed tests
 *
 * \param exitval   0: tests successful, otherwise fail message.
 * 
 */
static void endprog(int exitval)
{
	if (!exitval)
		printf("TEST SUCCEEDED\n");
	else 
		printf("TEST FAILED\n");
	
	MVB_DisableMVBCS1Interrupts();
    
	timer_delete( G_timerid );

	pciIntDisconnect2( INUM_TO_IVEC((int)G_pp04PciDev.irq),
					   pp04_isr,
					   G_pp04PciDev.bar1 );

}




/************************************************************
 * Monitoring of incoming bytes from Siemens MVB Slave 
 * check if time between reception of last byte and 'now'
 * exceeds a certain amount of time
 *
 */
static void pp04_timer(void)
{

	struct timespec now;
	unsigned int deltaT = 0;

	clock_gettime(CLOCK_REALTIME, &now );

	deltaT = now.tv_sec - G_timeLastData.tv_sec;

	if ( deltaT > MVB_MAX_RX_TIME_DIFF ){
		/* no data since n second -> alert */
		printf("NO MVB DATA!\n");
		G_errCount ++;
		if (G_errCount > MVB_MAX_ERRORS)
			G_LeaveMVBLoop = 1;
		return;
	}
}


/**********************************************************************/
/** MVB interrupt handler for vxWorks 
 * 
 * \param dummy  	parameter identifying the certain ISR, needed in
 *				    pciIntDisconnect2
 * 
 */
static void pp04_isr( int dummy )
{
	void *baseP = (void*)dummy;
	unsigned short ivr = 0, isr = 0;

	/* IRQ caused from us ? */
	isr = readWordFromTS( baseP + MVB_REG_ISR1 );
 	if (  isr & MVB_IRQ_WD ){
		/* WD IRQ bit set. This ISR read also 'freezes' the IRQcontroller*/

		/* clear the pending bits */
		writeWordToTS( (void*)(baseP + MVB_REG_IPR1), 0x0000 );

		/* 4. clear interrupt bits (also cleared by IVR reads) */
		writeWordToTS( (void*)(baseP + MVB_REG_ISR1), 0x0000 );

		/* reload 1000 ms into WDC */
		writeWordToTS( (void*)(baseP + MVB_REG_WDC ), WDC_INIT_VALUE );

		printf("irq #%d\n" , ++G_IsrCount );

		/* release IRcontroller by writing IVR1 */
		writeWordToTS( (void*)(baseP + MVB_REG_IVR1 ), ivr );
		
		if ( G_IsrCount > MVB_IRQ_TEST_COUNT ){
			printf("IRQ test ok, disabling MVBCS1 IRQs\n");
			MVB_DisableMVBCS1Interrupts();
			pciIntDisconnect2( INUM_TO_IVEC((int)G_pp04PciDev.irq),
							   pp04_isr,
							   G_pp04PciDev.bar1 );
			G_IsrCount = 0;
			/* let test go on in mvbmain() */
			G_irqTestOK = 1;
		}
	}
	return;

}




/**********************************************************************/
/** initially clear pending Registers and select used IRQ Source 
 * 
 *
 */
static int MVB_ClearAndInitInterruptSource(void)
{
	/* clear possible pending IRQs */
	OUTREG( MVB_REG_IPR0, 0x0000 );
	OUTREG( MVB_REG_IPR1, 0x0000 );

	/* enable Watchdog Interrupt */
	OUTREG( MVB_REG_IMR1, MVB_IRQ_WD);

	/* start WD timer by writing initial millisecond Value */
	OUTREG( MVB_REG_WDC, WDC_INIT_VALUE );

	return 0;

}



/**********************************************************************/
/** The main communication Loop, receives and sends Port data  
 * 
 * \param runs	how often bytes from opponent slave shall be received
 * \param mvbP	address of MVB Port setup struct, see MVBCS1 Software
 *
 * \return 0 if all runs succeeded or nonzero if loop aborted
 *
 */
static int MVB_MainCommunicationLoop( unsigned int runs, TYPE_LP_TS_CFG *mvbP )
{

	unsigned int rp[NR_SLAVES_B], rg[NR_SLAVES_A],txVal=0;
	UNSIGNED16 age1, age2;
	unsigned int pass = 0, i;

	/* init time of teststart */
	/* clock_gettime(CLOCK_REALTIME, &G_timeLastData ); */
	
	while( pass < runs ){

		/* feed data to the 5 source ports  */
		if ( !(pass % 8 ) ){
			txVal ++;
			for (i = 0; i < NR_SLAVES_B ; i++ ){
				MVB_InitTxBuf(i,  txVal+i);	/* change/init data to transmit */
			}
			/* transmit the 5 source ports */
			for (i=0; i< NR_SLAVES_B; i++)
				rg[i]= MVBCPutPort(G_ports[i].number,&G_bufTx[i][2],MVB_TS_ID);
		}
		/* receive the 2 sink ports */
		rp[0] = MVBCGetPort( G_ports[5].number, 
							 &G_bufRx[0][0],0x0000, &age1, MVB_TS_ID);
		rp[1] = MVBCGetPort( G_ports[6].number, &G_bufRx[1][0],0x0000, 
							 &age2, MVB_TS_ID);

		G_v1new = G_bufRx[0][1];
		G_v2new = G_bufRx[1][1];
		
		/* stop this if too many errors counted */
		if (G_LeaveMVBLoop)
			break;

		/* for vxWorks and Prod.test just dump the 2 incoming byte */
		if (G_v1old != G_v1new){
			/* polling till a new byte came in */
			printf(  "%02x ",  G_v1new );
			clock_gettime(CLOCK_REALTIME, &G_timeLastData );
			printf( " pass: %d\n", ++pass  );
		}

		G_v1old = G_v1new;
		G_v2old = G_v2new;

	};

	MVBCStop(0);
	return G_LeaveMVBLoop;

}




/**********************************************************************/
/** Initialize the MVB Ports structure
 *
 * \param mvbP			Trafficstore configuration
 * \param baseadr	   	base address of TM
 * 
 *
 */
static void	MVB_InitMvbStruct(TYPE_LP_TS_CFG *mvbP, 
							  void *baseadr,
							  TYPE_LP_PRT_CFG *ts_cfgP)
{
  	mvbP->tm_start 	= (UNSIGNED32)baseadr;
  	mvbP->pb_pit 	= (UNSIGNED32)(baseadr + 0x0000);
  	mvbP->pb_pcs 	= (UNSIGNED32)(baseadr + 0xC000);
  	mvbP->pb_prt 	= (UNSIGNED32)(baseadr + 0x4000);
  	mvbP->pb_frc 	= (UNSIGNED32)(baseadr + 0x8000);

  	mvbP->pb_def 		= 0;
  	mvbP->ownership 	= 1;
  	mvbP->ts_type 		= 1;
  	mvbP->prt_addr_max 	= 4095;
  	mvbP->prt_indx_max 	= MVB_PRT_INDX_MAX; /* must be highest in array +1*/
  	mvbP->prt_count 	= MVB_PRT_COUNT;

	/* from NSDB: MVBC-INITIALISIERUNG */
  	mvbP->mcm 	   		= MVB_MCM_MODE_64K;  /* 64k mode */
  	mvbP->msq_offset 	= 0;        
  	mvbP->mft_offset 	= 0;        
  	mvbP->line_cfg 		= 2;
  	mvbP->reply_to 		= 1;

	/*
	 * set the correct Slave DA according to SIMIS port definitions!
	 * Master (SIMIS) = 0x58   SlaveA: 0x4d   SlaveB:  0x01
	 */       
  	mvbP->dev_addr 		= MVB_DA_SLAVEB; 
  	mvbP->p_prt_cfg 	= (UNSIGNED32)ts_cfgP;

}




/**********************************************************************/
/** Connect the interrupt source in vxWorks 
 * 
 * \param pDev	address of struct PP04_PCIDEV struct keeping data
 *
 * \return 	0 if device found or error code
 *
 */
static unsigned int vxw_InitAndConnectInterrupt(PP04_PCIDEV *pDev)
{
	
	/* connect IRQ and enable it */
	CHK(pciIntConnect(INUM_TO_IVEC((int)pDev->irq),
					  pp04_isr,
					  (int)pDev->bar1));

	CHK(intEnable( pDev->irq ));

	return 0;
 abort:
	return -1;

}


/**********************************************************************/
/** Init/kickoff the surveillance timer to check communication stop
 * 
 * \param	t1P		address of itimerstruct with starttime
 * \param   msec	time interval in milliseconds to check 
 *
 */
static unsigned int vxw_InitTimer(struct itimerspec *t1P, unsigned int msec )
{

	CHK( !msec );
    CHK( timer_create (CLOCK_REALTIME, NULL, &G_timerid));
	CHK( timer_connect( G_timerid, pp04_timer, 0 ) );

	/* let timer expire after 1 s, then every <msec> milliseconds */
	t1P->it_value.tv_sec 		= 1;
	t1P->it_value.tv_nsec 		= 0;
	t1P->it_interval.tv_sec 	= 0;
	t1P->it_interval.tv_nsec 	= msec * 1000000;
	CHK( timer_settime(	G_timerid, TIMER_ABSTIME, t1P, NULL ));
	return 0;
 abort:
	return -1;
}

/**********************************************************************/
/** find the PP04 pci dev under vxWorks
 *
 * \param pDev	address of struct PP04_PCIDEV struct where to keep data
 * 
 * \return 0 on success or non 0 on error
 */
static unsigned int vxw_FindPciDevice(PP04_PCIDEV *pDev)
{

	if( pciFindDevice( PCI_VENDOR_ID_ALTERA, PCI_DEVICE_ID_PP04, 0,
					   &pDev->bus, &pDev->dev, &pDev->fct) ){
		printf("could not Find PCI device 0x%04x / 0x%04x\n",
			   PCI_VENDOR_ID_ALTERA, PCI_DEVICE_ID_PP04 );
		return -1;
	} else 
		printf("found PP04 at bus %02x dev %02x fct %02x\n",
			   pDev->bus, pDev->dev, pDev->fct );
		
	CHK( pciConfigInLong( pDev->bus, 
						  pDev->dev, 
						  pDev->fct,
						  PCI_CFG_BASE_ADDRESS_1, 
						  &pDev->bar1 ));

	CHK( pciConfigInByte( pDev->bus, 
						  pDev->dev, 
						  pDev->fct,
						  PCI_CFG_DEV_INT_LINE, 
						  &pDev->irq ));

	return 0;

 abort:
	return -1;

}

/**********************************************************************/
/** The programs main function
 * 
 * \param nRuns	 hof often to receive a byte
 */
int mvbtest_main( int nRuns )
{

	unsigned int i;
  	TYPE_LP_PRT_CFG mvb_ts_cfg[7];
	TYPE_LP_TS_CFG theMVB;
	struct itimerspec tm_setting;

	if (! nRuns){
		usage(0);
		goto end;
	}
	
	MVB_InitGlobals();

	CHK( vxw_FindPciDevice( &G_pp04PciDev ));

	printf( "PP04 test built %s %s\n", __DATE__, __TIME__ );
	printf(" on vxWorks: BAR1 = 0x%08x IRQ = 0x%02x\n", 
		   G_pp04PciDev.bar1, G_pp04PciDev.irq );

	MVB_InitMvbStruct( &theMVB, (void*)G_pp04PciDev.bar1, mvb_ts_cfg );

	for (i=0; i<NR_SLAVES_B; i++)
	MVB_InitTsStruct( &mvb_ts_cfg[i], G_ports[i].number, G_ports[i].idx ,
					  G_ports[i].size, LP_CFG_SRCE );
	for (i=NR_SLAVES_B; i<NR_SLAVES_B+NR_SLAVES_A; i++)
	MVB_InitTsStruct( &mvb_ts_cfg[i], G_ports[i].number,G_ports[i].idx , 
					  G_ports[i].size, LP_CFG_SINK );

    /*
	 *	Test 1: does the WD IRQ occur ? Initialize the MVBCS1 prior to
	 *  IRQ usage.
	 */
  	CHK( MVBCInit( &theMVB , MVB_TS_ID ));
  	CHK( MVBCStart( MVB_TS_ID));
	CHK( MVB_ClearAndInitInterruptSource());
	CHK( vxw_InitAndConnectInterrupt( &G_pp04PciDev ));		
	
	/* wait for several IRQs only first */
	printf("Checking Interrupts:\n");

	/* idle here in main until enough interrupts received */
	while(!G_irqTestOK);

    /*
	 * Test 2: test regular MVB communication
	 */
	CHK( vxw_InitTimer( &tm_setting, 1000 /* msec */ ));
	printf("Checking MVB Communication: doing %d receives\n", nRuns);
  	CHK( MVBCInit( &theMVB , MVB_TS_ID ));
  	CHK( MVBCStart( MVB_TS_ID));

	endprog( MVB_MainCommunicationLoop( nRuns, &theMVB ) );

 end:	
	return 0;
	/* errornous execution */
 abort:
	printf("Error occured, aborting program.\n");
	return -1;

}     


