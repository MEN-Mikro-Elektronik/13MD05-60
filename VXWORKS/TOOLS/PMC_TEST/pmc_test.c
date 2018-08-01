/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: pmc_test.c
 *      Project: 0745_MEN_PMCTest
 *
 *       Author: ts
 *        $Date: 2008/01/21 10:36:40 $
 *    $Revision: 1.1 $
 *
 *  Description: This is the test for CPUs/carrier boards PMC Sockets.
 *               To be used in conjunction with special P599 test design
 *               P599-xx.
 *
 *        Note:  Test depth is limited to "GO/NO GO" since in case a PCI signal
 *               is missing the test wont even find the P599 at the beginning
 *               anyway with 99% propability.
 *
 *     Required:  -
 *     Switches:  VxWorks, OS9000 
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: pmc_test.c,v $
 * Revision 1.1  2008/01/21 10:36:40  ts
 * Initial Revision, 1st work checkin
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char pmctestIdent[]="$Id: pmc_test.c,v 1.1 2008/01/21 10:36:40 ts Exp $";

#ifdef VXWORKS
#include "vxWorks.h" /* always first, CPU type, family , endianess, etc. */
#include "iv.h"			/* for INUM_TO_IVEC, then config.h is not needed */
#include "vmLib.h"
#include "taskLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "logLib.h"
#include "stdlib.h"
#include "string.h"
#include "ioLib.h"
#include "drv/pci/pciConfigLib.h"	/* for PCI const */
#include "drv/pci/pciIntLib.h"		/* for PCI interrupt */
#endif

#ifdef OS9000
#include <stdio.h>        /* OS-9 defs */
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <errno.h>
#include <modes.h>
#include <cglob.h>
#include <MEN/uti.h>
#include <MEN/usr_oss.h>
#include <MEN/os_men.h>

#ifdef _BIG_ENDIAN
# undef __LITTLE_ENDIAN
#endif
#ifdef _LITTLE_ENDIAN
# define __LITTLE_ENDIAN
#endif
#endif /* OS9000 */

#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
typedef int (*IRQ_FUNC_PRT) (int);
typedef int (*INT_ENABLE_DISABLE_FUNC_PRT) (int);

struct txrxdesc {
	volatile unsigned long   status;         /* owner, status */
	unsigned long	tdes1;				/* control, sizes */
	unsigned long   buf1addr;      		/* buffer 1 address */
	unsigned long   buf2addr;      		/* buffer 2 address */
};

typedef struct
{
	unsigned long	baseAddr;
	unsigned long	pciToCpuMemOffs;
	int			 	bigEndian;
	int  			txIrqFired;
	int  			txIrqCount;
	int				verbose;

} P599_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/

#define PCI_VENDOR_ID_ALTERA		0x1172
#define PCI_DEVICE_ID_MENCHAM	   	0x4d45
#warning "Check final PMC Test Subvendor ID!"
#define PCI_SUBVENDOR_ID_P599		0x0033 

#define PCI_CFG_MODE            	0x43
#define SLEEP_MODE_DIS          	0x00    /* sleep mode disable */

#ifdef OS9000
# define bcopy(src,dst,n) 	memcpy(dst,src,n)
# define bzero(m,n) 		memset(m,0,n)
# define d(m,n,fmt) 		dump("",(const char *)m,n)
#endif

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
static int nRuns;

#ifdef VXWORKS
static int BIG_Endian = 0;
static int BaseAddr   = 0;
static int P599_IrqIsInstalled[MAX_IRQS]={0,0,0,0};
#endif

static P599_HANDLE PMC_Handle;  

/* the PMC_Handle must be global at least for VxWorks,
   because the shared IRQ handler will be installed only once
   and will be call for each device with the same pointer
   to this structure */
#ifdef OS9000
static int G_irq;
#endif

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

static struct txrxdesc txd;    
static unsigned char txb[BUFLEN];

	

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

static void toggleLedsGPIO_1_3
(
	int				bigEndian,
	unsigned long	baseAddr
);


/************************************ ms_delay  ******************************
 *
 *  Description:  Millisecond delay base on system tick.
 *
 *---------------------------------------------------------------------------
 *  Input......:  ms	milli second
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void ms_delay( int ms )
{
#ifdef VXWORKS
	/* convert to tick */
	ms = sysClkRateGet() * ms / 1000;
	if( !ms )
		ms = 1;

	taskDelay( ms );
#endif

#ifdef OS9000
	UOS_Delay(ms);
#endif
}



/********************************* PMC_Irq *********************************
 *
 *  Description:  P599 Tx Irq handler.
 *
 *---------------------------------------------------------------------------
 *  Input......:  p599Handle	handle to the P599 parameter
 *  Output.....:  return	0 | 1 if was my IRQ
 *                *fired    1 if fired and was my IRQ
 *  Globals....:  -
 ****************************************************************************/
int PMC_Irq( P599_HANDLE *p599Handle )
{
	unsigned long	val;

	val = MREAD(p599Handle->ma, ISRREG);

#ifdef VXWORKS
	if( p599Handle->verbose )
		logMsg("PMC_Irq CSR5 %08x\n",val,0,0,0,0,0 );
#endif

	/* check if IRQ was my */
	if( CSR5_NIS & val )
	{
		setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr5_status, CSR5_NIS | CSR5_TU | CSR5_TI );
		p599Handle->txIrqFired	= 1;
		p599Handle->txIrqCount++;
		return( 1 );
	}/*if*/


	return( 0 );
}/*PMC_Irq*/

#ifdef OS9000
/********************************* IrqHandler ********************************
 *
 *  Description: Wrapper for PMC_Irq
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: returns: always 0
 *  Globals....: PMC_Handle
 ****************************************************************************/
u_int32 IrqHandler(void)
{
	PMC_Irq(&PMC_Handle);
	return E_NOTME;
}
#endif /* OS9000 */

/********************************* tulip_transmit  ***************************
 *
 *  Description:  Transmit a buffer.
 *
 *---------------------------------------------------------------------------
 *  Input......:  verbose			verbose flag
 *				  pciToCpuMemOffs	offset from PCI device to CPU MEMORY
 *				  baseAddr			base address of the csr's
 *				  bigEndian     	true on big endian machines
 *				  size				size of buffer int byte
 *				  data				buffer
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void tulip_transmit
(
	int				verbose,
	unsigned long	baseAddr,
	unsigned long	pciToCpuMemOffs,
	int			 	bigEndian,
	unsigned int 	size, 
	char 		 	*data
)
{
	int				timeout = 10;
	unsigned short	packet_type = 0x0800; /* type IP */

	bcopy(node_addr, ehdr, ETHER_ADDR_SIZE);					/* destination */
	bcopy(node_addr, &ehdr[ETHER_ADDR_SIZE], ETHER_ADDR_SIZE);	/* source */
	ehdr[ETHER_ADDR_SIZE*2]   = (packet_type >> 8) & 0xff;		/* packet type */
	ehdr[ETHER_ADDR_SIZE*2+1] = packet_type & 0xff;

	if( verbose )
		printf( "tulip transmit\n" );	

	/* setup the transmit descriptor */

	bzero((char*)&txd,sizeof(struct txrxdesc));

	txd.buf1addr = (unsigned long)&ehdr[0] | pciToCpuMemOffs;        /* ethernet header */

	txd.buf2addr = (unsigned long)data | pciToCpuMemOffs;               /* packet to transmit */
	txd.tdes1    = (size << 11) | ETHER_HDR_SIZE;

	if( verbose > 1 )
	{
		d( ehdr, ETHER_HDR_SIZE, 1 );
		d( data, (size)/4, 4 );
	}

	txd.tdes1  |= 0x188 << 22;           /* LS+FS+TER */

	txd.status   = 0x80000000;      /* give it to 21143 */

	setCsr( bigEndian, baseAddr, csr1_txpoll, 1 );  /* kick transmit */

    /* Wait for transmit to complete before returning. */
    while(txd.status & 0x80000000 ) 
    {
   		ms_delay( 1 );
		if( verbose > 1 )
			printf("tulip_transmit: status %08x\n", txd.status );

    	if( (timeout--) < 1 )
    	{
			if( verbose )
    			printf("TX timeout\n");
    		break;
    	}/*if*/
    }/*while*/

	if( verbose > 1 )
		printf("tulip_transmit: status %08x\n", txd.status );
}

/********************************* tulip_receive  ***************************
 *
 *  Description:  Receive a buffer.
 *
 *---------------------------------------------------------------------------
 *  Input......:  buff				buffer
 *				  verbose			verbose flag
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
unsigned long tulip_receive( char *buff, int verbose )
{
	unsigned long len = 0;

	if( verbose > 1)
		printf("tulip_receive %d %08x\n", rxd_tail, rxd[rxd_tail].status );

    if( rxd[rxd_tail].status & 0x80000000 ) 
    	return( len );

	if( verbose == 1)
		printf("tulip_receive %d %08x\n", rxd_tail, rxd[rxd_tail].status );

    len = (rxd[rxd_tail].status & 0x3FFF0000) >> 16;

    /* copy packet to working buffer */
    /* XXX - this copy could be avoided with a little more work
       but for now we are content with it because the optimised
       bcopy is quite fast */

    bcopy(&rxb[rxd_tail][ETHER_HDR_SIZE],buff,len-ETHER_HDR_SIZE);

	if( verbose > 1 )
	{
		d( &rxb[rxd_tail][0]  , ETHER_HDR_SIZE, 1 );
		d( buff, (len-(ETHER_HDR_SIZE+CHKSUM_SIZE))/4, 4 );
	}

    /* return the descriptor and buffer to recieve ring */
    rxd[rxd_tail].status = 0x80000000;
    rxd_tail++;
    if (rxd_tail == NRXD)
    	rxd_tail = 0;

	if( len != 0 && len > ETHER_HDR_SIZE )
		len -= (ETHER_HDR_SIZE + CHKSUM_SIZE);

    return( len );
}



#ifdef VXWORKS
/********************************* ccsr  ***************************
 *
 *  Description:  Change CSR.
 *
 *---------------------------------------------------------------------------
 *  Input......:  regNum		0..15
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void ccsr( int regNum )
{
unsigned long  val;
char instr[20];

 	val = getCsr( BIG_Endian, BaseAddr, regNum );
 	printf("CSR%d -> %08x\n", regNum, val );
 	printf("         ");
 	gets(instr);
 	val = strtoul( instr, NULL, 16 );
 	printf("\n      %08x", val);
 	printf("\n");

 	setCsr( BIG_Endian, BaseAddr, regNum, val );
 	val = getCsr( BIG_Endian, BaseAddr, regNum );
}


/********************************* p12send  ***************************
 *
 *  Description:  send a buffer
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void p12send( void )
{
	tulip_transmit(	0,
					BaseAddr,
					0x80000000,
					BIG_Endian,
					P599_TX_DATA_SIZE*4, 
					(char*) &P599_TX_DATA );
}

void p12ShowTxIrqCount( char *devName )
{
	FILE *fd;
	
	if( devName != NULL )
	{
		fd = fopen( devName, "w" );
		if( fd )
		{
			fprintf( fd, "P599: irqcount %d\n", PMC_Handle.txIrqCount );
			fclose( fd );
		}
	}/*if*/
}/*p12ShowTxIrqCount*/

#endif /* VXWORKS */

/********************************* p12Test **********************************
 *
 *  Description:  
 * 				  
 *
 *---------------------------------------------------------------------------
 *  Input......:  p599Handle	handle to the P599 parameter
 *
 *  Output.....:  return 0 | 1 on error
 *
 *  Globals....:  -
 ****************************************************************************/
static int pmc_test( P599_HANDLE *p599Handle )
{
	int x, i, idx;
	unsigned long len;
    unsigned long csr6, val;

	/* soft reset */
	setMaskCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr0_busmode, 0x00000001 );
	ms_delay( 100 );
	clrMaskCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr0_busmode, 0x00000001 );

	/* blinking the current P599 */
	toggleLedsGPIO_1_3( p599Handle->bigEndian, p599Handle->baseAddr );


	/* soft reset */
	setMaskCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr0_busmode, 0x00000001 );
	ms_delay( 100 );
	
	/* media chip reset and selection of activity LED */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr15_sia_general, 
			0x08af0008 );
	ms_delay( 100 );
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr15_sia_general, 
			0x00050008 );

    /* turn off reset and set cache align=16lword, burst=unlimit */
	if( !p599Handle->bigEndian )
		setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr0_busmode, 0x01A08000 );
	else
		setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr0_busmode, 0x01B08000 ); /*DBO bit 20- decriptor byte ordering - big endian*/

	

	/*------------------------------------------------+
	|  Perform ethernet 10BASET HD INTERNAL LOOP      |
	+------------------------------------------------*/
	csr6 = 0x0200C000 /* threshold */ | CSR6_SC | CSR6_CA | CSR6_RA | CSR6_PR | CSR6_OM_INTERNAL_LOOP;
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr6_command, csr6 );

	csr6 &= ~CSR6_PS;
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr6_command, csr6 );
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr13_sia_connectivity, 0 );  /* SIA RESET   */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr14_sia_tx_rx, 0x7f3f );	   /* 10baseT HD  */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr13_sia_connectivity, 1 );  /* ! SIA RESET */

	/* enable transmit irq */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr7_intr, CSR7_NIE | CSR7_TUE | CSR7_TIE );

	if( p599Handle->verbose > 2 )
		dumpCsr( p599Handle->bigEndian, p599Handle->baseAddr );


    /* construct setup packet which is used by the 21143 to
       program its CAM to recognize interesting MAC addresses */

	bzero( (char*) &txd,sizeof(struct txrxdesc));
	txd.buf1addr = (unsigned long)&txb[0] | p599Handle->pciToCpuMemOffs;
	txd.buf2addr = (unsigned long)&txb[0] | p599Handle->pciToCpuMemOffs;         /* just in case */
	txd.tdes1	 = (0x28<<22) | 192; /* setup packet size must be 192 bytes */

   /*
    * construct perfect filter frame
    * with mac address as first match
    * and broadcast address for all others
    */
	for(x=0;x<192;x++) txb[x] = 0xff;

	txb[0] = node_addr[0];
	txb[1] = node_addr[1];
	txb[4] = node_addr[2];
	txb[5] = node_addr[3];
	txb[8] = node_addr[4];
	txb[9] = node_addr[5];


	txd.status   = 0x80000000;      /* give ownership to 21143 */

    /* tell DC211XX where to find tx descriptor list */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr4_txlist, p599Handle->pciToCpuMemOffs | (unsigned long) &txd );

	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr6_command, csr6 | CSR6_ST );  /* start transmitter */

	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr1_txpoll, 1 );  /* kick transmit */


	/* wait for setup packet complete sended */
	{
		int tout=100;
		while( txd.status & 0x80000000 ){
			if( tout-- ==  0){
				if( p599Handle->verbose ){
					printf("*** timeout in setup packet\n");
					return -1;
				}
			}				
			ms_delay( 1 );
		}
	}

	if( p599Handle->verbose > 2 )
		dumpCsr( p599Handle->bigEndian, p599Handle->baseAddr );

    /* setup receive descriptor */
	for(x=0;x<NRXD;x++) 
	{
            bzero((char*)&rxd[x],sizeof(struct txrxdesc));
            rxd[x].buf1addr = (unsigned long)&rxb[x][0] | p599Handle->pciToCpuMemOffs;
            rxd[x].buf2addr = 0;        /* not used */
            rxd[x].tdes1    = BUFLEN;
            rxd[x].status   = 0x80000000; /* give ownership it to 21143 */
	}/* for */
	rxd[NRXD - 1].tdes1  |= (0x008<<22);       /* Set Receive end of ring on last descriptor */
	rxd_tail = 0;

    /* tell DC211XX where to find rx descriptor list */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr3_rxlist, p599Handle->pciToCpuMemOffs | (unsigned long)&rxd[0] );

    /* start the receiver */
	setCsr( p599Handle->bigEndian, p599Handle->baseAddr, csr6_command, csr6 | CSR6_SR | CSR6_ST );

	/* send/receive frames n-times */
	for( i=0; i<nRuns || nRuns==0; i++ )
	{
		/* clear rx buffer */
		bzero( (char*)rxb, NRXD*BUFLEN );
		if( p599Handle->verbose > 1 )
			printf("rxb at 0x%08x\n", rxb );

		p599Handle->txIrqFired = 0;
		tulip_transmit(	p599Handle->verbose,
						p599Handle->baseAddr,
						p599Handle->pciToCpuMemOffs,
						p599Handle->bigEndian,
						P599_TX_DATA_SIZE*4, 
						(char*) &P12_TX_DATA );

		if( p599Handle->verbose > 2 )
			dumpCsr( p599Handle->bigEndian, p599Handle->baseAddr );

		for( x=0; x<5; x++ )
		{
			/* clear packet buffer */
			bzero( (char*)packet, MAX_BUFF );

			/* receive */
			len = tulip_receive( (char*)packet, p599Handle->verbose );
			if( len != 0 )
				break;
			ms_delay( 100 );
		}/*for*/
		
		if( len/4 == P12_TX_DATA_SIZE )
		{
			/* check buffer */
			for( idx=0; idx < P12_TX_DATA_SIZE; idx++ )
			{
				if( packet[idx]	!= P12_TX_DATA[idx] )
				{
					if( p599Handle->verbose )
						printf("*** pattern mismatch\n");
					if( p599Handle->verbose > 1 )
						printf(" at %d %08x != %08x\n", idx, packet[idx], P12_TX_DATA[idx] );
					
					goto CLEANUP;
				}
			}/*for*/			
		}
		else
			goto CLEANUP;

		if( !p599Handle->txIrqFired )
			goto CLEANUP;
	
	}/*for*/

	return( 0 );

CLEANUP:
	if( p599Handle->verbose && !p599Handle->txIrqFired )
		printf("*** no TX IRQ\n");
	if( p599Handle->verbose && !len )
		printf("*** no receive\n");
	if( p599Handle->verbose && len != P12_TX_DATA_SIZE*4 )
		printf("*** receive length\n");
	return( -1 );
}/*p12Test*/

/********************************* usage ************************************
 *
 *  Description: Print program usage and revision
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void usage(void)
{
#ifdef VXWORKS
	printf("Usage: pmc_test [<-u=>] <-b= -d= -p= -m= -M -v=>\n");
#endif
#ifdef OS9000
	printf("Usage: pmc_test [<-u=>] <-b= -d= -v=>\n");
#endif
	printf("Function:"
		   "\tTest for PMC CPU/Carrier connectors.\n"
    	   "\t\tChecks AD[31:0], /INTA-/INTD and all PCI system signals\n"
	       "Options:\n"
	       "    -b=   pci bus no\n"
		   "    -d=   pci dev no\n"
		   "    -u=   optional unit no, if no bus/dev given [0..x]\n"
		   "    -r=   number of packets (default 3), 0=endless\n"
#ifdef VXWORKS
		   "    -p=   cpu to pci offset\n"
		   "    -m=   pci to memory offset\n"
	       "    -M    bigendian\n"
#endif
	       "    -v=   verbose [0..3]\n"
#ifdef VXWORKS
	       "    -n    no interrupt installation\n"
	       "          optional for VxWorks\n"
	       "          interrupt must be installed and enabled external\n"
#endif
	       "\n"
	       "(c) by MEN mikro elektronik GmbH (%s)\n", pmctestIdent);
}

/********************************* Cleanup ***********************************
 *
 *  Description: Do any required cleanup
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: error		error code from main
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void Cleanup( int error )
{
#ifdef OS9000	
	unsigned long val = getCsr( PMC_Handle.bigEndian, PMC_Handle.baseAddr, csr15_sia_general );
	val &= ~(CSR15_MD_1 | CSR15_MD_3 );
	setCsr( PMC_Handle.bigEndian, PMC_Handle.baseAddr, csr15_sia_general, val );
	_osmen_irq( G_irq, 2, NULL, _glob_data );	
	exit(error);
#endif
}

/************************************* main **********************************
 *
 *  Description:  P12 Test user interface.
 *
 *---------------------------------------------------------------------------
 *  Input......:  argc, argv
 *  Output.....:  return 0 | -1 on error
 *  Globals....:  -
 ****************************************************************************/
int main( int  argc, char *argv[] )
{
	int error = -1;
	char *str;
	int pciBus = 0, pciDevice = 0, pciFunc=0;
	int unitNbr = 0;
	int	verbose = 0;
	int	bigEndian;
#ifdef VXWORKS
	unsigned long	cpuToPciMemOffs;
	unsigned long	pciToCpuMemOffs;
	int key;
	unsigned long  baseAddr = 0;

	unsigned char  irq;

#endif

#ifdef OS9000
	u_int32 baseAddr=0;
	u_int8 irq;
	u_int16 venId, devId;
	u_int32 pciIoBaseOff, pciMemBaseOff, irqOff;
#endif
	
	printf("\nP12 Test %s\n", pmctestIdent );

	if (argc<2 || UTL_TSTOPT("?")) 
	{			/* help requested ? */
		usage();
		return(1);
	}

	pciBus    = ((str = UTL_TSTOPT("b=")) ? atoi(str) : 0);
	pciDevice = ((str = UTL_TSTOPT("d=")) ? atoi(str) : 0);
	unitNbr   = ((str = UTL_TSTOPT("u=")) ? atoi(str) : 0xffff);
	verbose	  = ((str = UTL_TSTOPT("v=")) ? atoi(str) : 0);
	nRuns	  = ((str = UTL_TSTOPT("r=")) ? atoi(str) : 3);

#ifdef VXWORKS
	cpuToPciMemOffs = ((str = UTL_TSTOPT("p=")) ? strtoul(str, NULL, 16) : 0);
	pciToCpuMemOffs = ((str = UTL_TSTOPT("m=")) ? strtoul(str, NULL, 16) : 0);
	bigEndian = (UTL_TSTOPT("M") ? 1 : 0);
	noIrqInstallation = (UTL_TSTOPT("n") ? 1 : 0);
#endif
#ifdef OS9000
# ifdef __LITTLE_ENDIAN
    bigEndian = 0;
# else
	bigEndian = 1;
#endif
#endif

	if( verbose )
	{
		printf("\n%s\n", pmctestIdent );
		printf("\tparameter:\n");
		printf("\t\tpciBus   :\t%d\n",         pciBus          );
		printf("\t\tpciDevice:\t%d\n",         pciDevice       );
		printf("\t\tunitNbr  :\t%d\n",         unitNbr         );
		printf("\t\tverbose  :\t%d\n",         verbose         );
		printf("\t\tbigEndian:\t%d\n",         bigEndian       );
#ifdef VXWORKS
		printf("\t\tcpuToPciMemOffs:\t%08x\n", (int)cpuToPciMemOffs );
		printf("\t\tpciToCpuMemOffs:\t%08x\n", (int)pciToCpuMemOffs );
#endif		
	}/*if*/
	
#ifdef VXWORKS	
	if( unitNbr != 0xFFFF )	{
		/*-------------------------+
		| PCI stuff                |
		*-------------------------*/
   		error = pciFindDevice(	PCI_VENDOR_ID_DEC,
								PCI_DEVICE_ID_DEC21143,
   								unitNbr,
        	              		&pciBus, &pciDevice, &pciFunc );
    	if( error )
	    {
    		printf("*** P12 # %d not found\n", unitNbr );
    		goto CLEANUP;
		}/*if*/
	}/*if*/

	/* get address + irq */
    pciConfigInLong (	pciBus, pciDevice, pciFunc,	PCI_CFG_BASE_ADDRESS_1,
                    	(int*) &baseAddr);
    
    if( baseAddr == 0xFFFFFFFF )
    {
   		printf("*** no P12 at bus/dev/func %d/%d/%d\n",
   		       pciBus, pciDevice, pciFunc );
   		goto CLEANUP;
	}/*if*/
    
    
    baseAddr +=cpuToPciMemOffs;
                   
    pciConfigInByte (	pciBus,	pciDevice, pciFunc,	PCI_CFG_DEV_INT_LINE,
                    	&irq);

	/* sleep mode disable */
    pciConfigOutByte (	pciBus,	pciDevice, pciFunc, PCI_CFG_MODE,
                    	SLEEP_MODE_DIS);
	

	BIG_Endian = bigEndian;
 	BaseAddr   = baseAddr;

	if( verbose )
	{
    	printf("\t P12 #%d found\n", unitNbr );
	}/*if*/

	key = intLock(); /* avoid problems with shared IRQ's */
	PMC_Handle.baseAddr      	= baseAddr;      
	PMC_Handle.pciToCpuMemOffs	= pciToCpuMemOffs;
	PMC_Handle.bigEndian     	= bigEndian;     
	PMC_Handle.txIrqFired    	= 0;    
	PMC_Handle.txIrqCount    	= 0;    
	PMC_Handle.verbose       	= verbose;       
	intUnlock( key ); /* avoid problems with shared IRQ's */

	/*-------------------------+	
	| interrupt installation   |
	*-------------------------*/
	if( irq >= MAX_IRQS )
	{
    	printf("*** IRQ out of range\n" );
    	goto CLEANUP;
	}/*if*/

	if( !P12_IrqIsInstalled[irq] ) {
#ifdef __LITTLE_ENDIAN
		P12_IrqIsInstalled[irq] = 1;
	    pciIntConnect( (VOIDFUNCPTR *)INUM_TO_IVEC(0x20+irq),
			            (void*) PMC_Irq,
			            (int)   &PMC_Handle );
		sysIntEnablePIC( irq );
#else
		P12_IrqIsInstalled[irq] = 1;
		intConnect(INUM_TO_IVEC((int)irq ), (void*)PMC_Irq, (int)&PMC_Handle);
		intEnable( irq );
#endif
	}/*if*/
#endif /* VXWORKS */

#ifdef OS9000
	/*--------------------------------------------+
	|  OS-9000 stuff                              |
	+--------------------------------------------*/
	if( unitNbr != 0xFFFF ){
		int index = 0, found=0;
		
		pciFunc = 0;

		/* search for P12 */
		for( pciBus = 0; pciBus < 0x100; pciBus++ ) {
			for( pciDevice = 0; pciDevice < 32; pciDevice++ ){
				_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, 	
										  PCI_REG_ADDR_VENDOR_ID, &venId );

				_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, 
										  PCI_REG_ADDR_DEVICE_ID, &devId );

				if( venId == PCI_VENDOR_ID_ALTERA && 
					devId == PCI_DEVICE_ID_MENCHAM && 
					subvenId == PCI_SUBVENDOR_ID_P599 ) {
					if( verbose )
						printf("Unit #%d on PCI %d/%d/%d\n", 
							   index, pciBus, pciDevice, pciFunc );
					if( index++ == unitNbr){
						found = 1;
						break;
					}
				}
			}
			if( found ) break;
		}
		if( !found ){
			printf("*** P12 # %d not found\n", unitNbr );
    		goto CLEANUP;
		}
	}

	/* check if it's really a P12 */
	_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, 
							  PCI_REG_ADDR_VENDOR_ID, &venId );
	_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, 
							  PCI_REG_ADDR_DEVICE_ID, &devId );

	if( venId!=PCI_VENDOR_ID_DEC || devId!=PCI_DEVICE_ID_DEC21143 ){
		printf("*** not a P12 on %d/%d/%d\n", pciBus, pciDevice, pciFunc );
		goto CLEANUP;
	}
		
		
	/* get address + irq */
	_osmen_pci_read_cfg_long( pciBus, pciDevice, pciFunc, 
							  PCI_REG_ADDR_BASE_1, &baseAddr );
	_osmen_pci_read_cfg_byte( pciBus, pciDevice, pciFunc, 
							  PCI_REG_ADDR_INT_LINE, &irq );

	/* get baseaddr/irq offsets */	
	_osmen_pci_mapping( &pciIoBaseOff, &pciMemBaseOff, &irqOff );
	baseAddr += pciMemBaseOff;

	PMC_Handle.baseAddr      	= baseAddr;      
	PMC_Handle.pciToCpuMemOffs	= pciMemBaseOff == 0xC0000000 ? 0x80000000 : 0x00000000;
	PMC_Handle.bigEndian     	= bigEndian;     
	PMC_Handle.txIrqFired    	= 0;    
	PMC_Handle.verbose       	= verbose;       

	intercept(Cleanup);

	/* enable access to registers and PIC */
	_os_permit( (void *)baseAddr, 0x800, S_IREAD | S_IWRITE, _procid);
	_os_permit( (void *)pciIoBaseOff, 0x100, S_IREAD | S_IWRITE, _procid);
	
	/* disable sleep mode of device */
	_osmen_pci_write_cfg_byte(pciBus, 
		 				pciDevice, 
		 				pciFunc,
                    	PCI_CFG_MODE,
                    	SLEEP_MODE_DIS); 
	/* install interrupt handler */
	G_irq = irq+irqOff;

	_osmen_irq( G_irq, 2, (void *)IrqHandler, _glob_data );
	
	/* enable PIC interrupt FIXME ??? This assumes a PIC controller */
	if( irq >= 8 ){
		*(volatile u_int8 *)(pciIoBaseOff + 0xa1) &= ~(1<<(irq-8));
		*(volatile u_int8 *)(pciIoBaseOff + 0x21) &= ~0x4;
	}
	else {
		*(volatile u_int8 *)(pciIoBaseOff + 0x21) &= ~(1<<(irq));
	}


#endif /* OS9000 */

   	printf("\tbus/dev/func %d/%d/%d at %08x irq %d\n", 
		   pciBus, pciDevice, pciFunc, (int)baseAddr, irq );

	/*-------------------------+	
	| run the test             |
	*-------------------------*/
	error = p12Test( &PMC_Handle );
	if( error )
		printf("*** TEST FAILED\n");
	else
		printf("OK\n");
	  
CLEANUP:
	Cleanup(error);
	return error;
}/*p12test*/


