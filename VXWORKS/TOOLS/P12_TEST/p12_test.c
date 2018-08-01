/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: p12_test.c
 *      Project: -
 *
 *       Author: uf
 *        $Date: 2005/10/27 09:43:42 $
 *    $Revision: 1.5 $
 *
 *  Description: This is test for the P12 ( or on board DEC21143) runniernal
 *               loopback mode. 
 *				 The test check all 32 address/data lines, 
 * bus mastering and interrupts
 *
 *        Note:  
 *
 *     Required:  -
 *     Switches:  VxWorks, OS9000 
 *
 * OS9000 Note: Currently programs PIC controller directly :-(
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: p12_test.c,v $
 * Revision 1.5  2005/10/27 09:43:42  UFranke
 * added
 *  + irqCount
 *
 * Revision 1.4  2002/09/04 10:38:47  JKirsch
 * changed
 *   -increased nbr of pci IRQ's from 16 to 256
 *
 * Revision 1.3  2001/11/19 08:47:17  kp
 * 1) bug fix in handling shared interrupts
 * 2) Light LED during transmissions
 *
 * Revision 1.2  2001/08/30 15:29:16  kp
 * ported to OS-9000 (F1N)
 * bitfields removed, timeout for setup packet added
 *
 * Revision 1.1  2001/08/27 10:16:28  franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2001 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char pcmip_p12_test_IdentString[]="$Id: p12_test.c,v 1.5 2005/10/27 09:43:42 UFranke Exp $";

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
}P12_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define PCI_VENDOR_ID_DEC			0x1011
#define PCI_DEVICE_ID_DEC21143	    0x0019

#define PCI_CFG_MODE            0x43
#define SLEEP_MODE_DIS          0x00    /* sleep mode disable */


#define csr0_busmode             0
#define csr1_txpoll              1
#define csr2_rxpoll              2
#define csr3_rxlist              3
#define csr4_txlist              4
#define csr5_status              5
#define csr6_command             6
#define csr7_intr                7
#define csr8_missed_frames       8
#define csr9_enetrom_mii         9   /* 21040 */
#define csr10_reserved_boot      10  /* 21040 */
#define csr11_full_duplex        11  /* 21040 */
#define csr12_sia_status         12  /* 2104x */
#define csr13_sia_connectivity   13  /* 2104x */
#define csr14_sia_tx_rx          14  /* 2104x */
#define csr15_sia_general        15  /* 2104x */

#define SROMSEL         0x0800
#define SROMCS          0x0001
#define SROMCLKON       0x0002
#define SROMCLKOFF      0x0002
#define SROMRD          0x4000
#define SROMWR          0x2000
#define SROM_BITWIDTH   6
#define SROMCMD_RD      6
#define SROMCSON        0x0001
#define SROMDOUT        0x0004
#define SROMDIN         0x0008


#define CSR5_TI			0x00000001		/* transmit interrupt */
#define CSR5_TU			0x00000004		/* transmit buffer unavail interrupt */
#define CSR5_NIS		0x00010000		/* normal   interrupt summary */

#define CSR6_HP			0x00000001		/* hash / perfect filter receive mode */
#define CSR6_SR			0x00000002		/* START/STOP RECEIVE */
#define CSR6_HO			0x00000004		/* hash only filter   */
#define CSR6_PB			0x00000008		/* pass bad frames    */
#define CSR6_IF			0x00000010		/* inverse filtering  */
#define CSR6_SB			0x00000020		/* start/stop back off counter */
#define CSR6_PR			0x00000040		/* promiscuouns mode	*/
#define CSR6_PM			0x00000080		/* pass all mulicast	*/
#define CSR6_ST			0x00002000		/* START/STOP TRANSMISSION */
#define CSR6_OM_NORMAL			0x00000000
#define CSR6_OM_INTERNAL_LOOP	0x00000400		
#define CSR6_OM_EXTERNAL_LOOP	0x00000800
#define CSR6_CA			0x00020000
#define CSR6_PS			0x00040000
#define CSR6_RA			0x40000000
#define CSR6_SC			0x80000000

#define CSR7_TIE		0x00000001		/* transmit interrupt  enable */
#define CSR7_TUE		0x00000004		/* transmit buffer unavail interrupt  enable */
#define CSR7_NIE		0x00010000		/* normal   interrupt  enable */


#define CSR15_CWE		0x08000000
#define CSR15_LGS3		0x00800000
#define CSR15_LGS2		0x00400000
#define CSR15_LGS1		0x00200000
#define CSR15_LGS0		0x00100000
#define CSR15_MD_3		0x00080000
#define CSR15_MD_2		0x00040000
#define CSR15_MD_1		0x00020000
#define CSR15_MD_0		0x00010000

#define MAX_BUFF		0x4000
#define BUFLEN 1600     /* must be longword divisable */
                        /* buffers must be longword aligned */
#define NRXD 4
#define ETHER_HDR_SIZE		14	/* Size of ethernet header */
#define ETHER_ADDR_SIZE		6
#define CHKSUM_SIZE			4

#define MAX_IRQS			256

/* pattern to send
 *
 * 32 * 4  = 128 + etherhdr = 142 < MTU 
 *
 * the datafield toggles each PCI data (addr ) line
 *
 */
#define P12_TX_DATA_SIZE	0x40
const unsigned long P12_TX_DATA[P12_TX_DATA_SIZE] = {
	1,2,3,4,5,6,8,
	0x87654321, 0xFFAA5500, 0x0055AAFF, 0xdeadbeaf,
	0x80000000, 0x40000000, 0x20000000, 0x10000000,
	0x08000000, 0x04000000, 0x02000000, 0x01000000, 
	0x00800000, 0x00400000, 0x00200000, 0x00100000,
	0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100, 
	0x00000080, 0x00000040, 0x00000020, 0x00000010,
	0x00000008, 0x00000004, 0x00000002, 0x00000001
	};

#ifdef OS9000
#define bcopy(src,dst,n) 	memcpy(dst,src,n)
#define bzero(m,n) 			memset(m,0,n)
#define d(m,n,fmt) 			dump("",(const char *)m,n)
#endif

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
static int nRuns;
static char node_addr[ETHER_ADDR_SIZE] = {0x00, 0xC0, 0x3A, 0x12, 0x00, 0x00};

#ifdef VXWORKS
static int BIG_Endian = 0;
static int BaseAddr   = 0;
static int P12_IrqIsInstalled[MAX_IRQS]={0,0,0,0};
#endif

static P12_HANDLE P12_Handle;  
/* the P12_Handle must be global at least for VxWorks,
   because the shared IRQ handler will be installed only once
   and will be call for each device with the same pointer
   to this structure */
#ifdef OS9000
static int G_irq;
#endif

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/* transmit descriptor and buffer */
static struct txrxdesc txd;    
static unsigned char txb[BUFLEN];

/* receive descriptor(s) and buffer(s) */
static struct txrxdesc rxd[NRXD];      
static unsigned char rxb[NRXD][BUFLEN];
static int rxd_tail = 0;                

static long packet[MAX_BUFF/4];

static unsigned char ehdr[ETHER_HDR_SIZE];    /* buffer for ethernet header */

	

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
static int p12Test
( 
	P12_HANDLE *p12Handle
);

unsigned long getCsr
(
	int				bigEndian,
	unsigned long	baseAddr,
	int				csr
);

void dumpCsr
(
	int				bigEndian,
	unsigned long	baseAddr
);

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

/********************************* getCsr ***************************
 *
 *  Description:  Get a CSR.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian     true on big endian machines
 *				  baseAddr		base address of the csr's
 *				  csr			csr number 0..15
 *  Output.....:  return CSR value
 *  Globals....:  -
 ****************************************************************************/
unsigned long getCsr
(
	int				bigEndian,
	unsigned long	baseAddr,
	int				csr
)
{
unsigned long value;

	value = *(volatile unsigned long*)((char*) baseAddr + (csr*8));

	if( bigEndian )
		value = ( ((value)>>24) | ((value)<<24) | 
				  (((value)>>8) & 0x0000ff00)   | 
				  (((value)<<8) & 0x00ff0000) );


	return( value );
}/*getCsr*/

/*********************************  setCsr ***************************
 *
 *  Description:  Set a CSR.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian     true on big endian machines
 *				  baseAddr		base address of the csr's
 *				  csr			csr number 0..15
 *				  value			new csr value
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void setCsr
(
	int				bigEndian,
	unsigned long	baseAddr,
	int				csr,
	unsigned long   value
)
{

	if( csr == 16 )
		printf(" %08x\n", value );

	if( bigEndian )
		value = ( ((value)>>24) | ((value)<<24) | 
				  (((value)>>8) & 0x0000ff00)   | 
				  (((value)<<8) & 0x00ff0000) );

	*(volatile unsigned long*)((char*) baseAddr + (csr*8)) = value;

	return;
}/*setCsr*/

/********************************* setMaskCsr  ***************************
 *
 *  Description:  Set a bit mask.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian     true on big endian machines
 *				  baseAddr		base address of the csr's
 *				  csr			csr number 0..15
 *				  mask			bit's to set
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void setMaskCsr
(
	int				bigEndian,
	unsigned long	baseAddr,
	int				csr,
	unsigned long   mask
)
{
unsigned long value;

	value = getCsr( bigEndian, baseAddr, csr );
	value |= mask;
	setCsr( bigEndian, baseAddr, csr, value );
}/*setMaskCsr*/

/********************************* clrMaskCsr  ***************************
 *
 *  Description:  Clear a bit mask.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian     true on big endian machines
 *				  baseAddr		base address of the csr's
 *				  csr			csr number 0..15
 *				  mask			bit's to clear
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void clrMaskCsr
(
	int				bigEndian,
	unsigned long	baseAddr,
	int				csr,
	unsigned long   mask
)
{
unsigned long value;

	value = getCsr( bigEndian, baseAddr, csr );
	value &= ~mask;
	setCsr( bigEndian, baseAddr, csr, value );
}/*clrMaskCsr*/


/********************************* dumpCsr  ***************************
 *
 *  Description:  Dump the csr's.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian     true on big endian machines
 *				  baseAddr		base address of the csr's
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
void dumpCsr
(
	int				bigEndian,
	unsigned long	baseAddr
)
{
	int 			csr;
	unsigned long	val;

#ifdef VXWORKS
	if( !baseAddr )
	{
		bigEndian	= BIG_Endian;
		baseAddr	= BaseAddr;
	}/*if*/
#endif

	for(csr=0;csr<16;csr++)
		printf(" CSR%2d: %08x\n", csr, (int)getCsr( bigEndian, baseAddr, csr ));
		
	csr = 5;
	val = getCsr( bigEndian, baseAddr, csr );
	
	printf("\n\ncsr5    %08x\n", val );
	printf(" RRRR  LC  GPI  EB  TS  RS  NIS AIS TU\n" );
	printf(" rrrr  ");
	printf(" %c  ",val & 0x08000000 ? '*' : ' '); /* LC */
	printf(" %c  ",val & 0x04000000 ? '*' : ' '); /* GPI */
	printf(" %x  ", (val >> 23) & 7 ); /* EB */
	printf(" %x  ", (val >> 20) & 7 ); /* TS */
	printf(" %x  ", (val >> 17) & 7 ); /* RS */
	printf(" %c  ",val & 0x00010000 ? '*' : ' '); /* NIS */
	printf(" %c  ",val & 0x00008000 ? '*' : ' '); /* AIS */
	printf(" %c  ",val & 0x00000004 ? '*' : ' '); /* TU  */

	setCsr( bigEndian, baseAddr, csr, val ); /* clear CSR 5 */
	printf("\n" );

	val = getCsr( bigEndian, baseAddr, csr );
	printf("\n\ncsr5    %08x\n", val );
	printf(" %x  ", (val >> 17) & 7 ); /* RS */
	printf("\n" );
}/*dumpCsr*/

/********************************* toggleLedsGPIO_1_3 ************************
 *
 *  Description:  Toggles the LED's on the DUT.
 *
 *---------------------------------------------------------------------------
 *  Input......:  bigEndian		should be true on big endian CPU's
 *                baseAddr		base address of the DEC21143
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
static void toggleLedsGPIO_1_3
(
	int				bigEndian,
	unsigned long	baseAddr
)
{
	unsigned long val, i;

	/*--------------------+
	| prepare GPIO 3, 1   |
	+--------------------*/
	
	/* enable write to gep control */
	val = getCsr( bigEndian, baseAddr, csr15_sia_general );
	val |= CSR15_CWE;
	setCsr( bigEndian, baseAddr, csr15_sia_general, val );

	/* set GEP 3 & 1 to general purpose - bit bit 27..20 are write only */
	val &= ~( CSR15_LGS3 | CSR15_LGS1 );
	setCsr( bigEndian, baseAddr, csr15_sia_general, val );

	/* set GEP 3 & 1 to outputs */
	val |= CSR15_MD_3 | CSR15_MD_1;
	setCsr( bigEndian, baseAddr, csr15_sia_general, val );

	/* enable write to gep data */
	val &= ~CSR15_CWE;
	setCsr( bigEndian, baseAddr, csr15_sia_general, val );
	
	/*--------------------+
	| toggle  GPIO 3, 1   |
	+--------------------*/
	for( i=0; i<5; i++ ){
		val |= CSR15_MD_3 | CSR15_MD_1;
		setCsr( bigEndian, baseAddr, csr15_sia_general, val );
		ms_delay( 100 );;

		val &= ~(CSR15_MD_3 | CSR15_MD_1);
		setCsr( bigEndian, baseAddr, csr15_sia_general, val );
		ms_delay( 100 );
	}
}/*toggleLedsGPIO_1_3*/


/********************************* P12_TxIrq *********************************
 *
 *  Description:  P12 Tx Irq handler.
 *
 *---------------------------------------------------------------------------
 *  Input......:  p12Handle	handle to the P12 parameter
 *  Output.....:  return	0 | 1 if was my IRQ
 *                *fired    1 if fired and was my IRQ
 *  Globals....:  -
 ****************************************************************************/
int P12_TxIrq( P12_HANDLE *p12Handle )
{
	unsigned long	val;


	val = getCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr5_status );

#ifdef VXWORKS
	if( p12Handle->verbose )
		logMsg("P12_TxIrq CSR5 %08x\n",val,0,0,0,0,0 );
#endif

	/* check if IRQ was my */
	if( CSR5_NIS & val )
	{
		setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr5_status, CSR5_NIS | CSR5_TU | CSR5_TI );
		p12Handle->txIrqFired	= 1;
		p12Handle->txIrqCount++;
		return( 1 );
	}/*if*/


	return( 0 );
}/*P12_TxIrq*/

#ifdef OS9000
/********************************* IrqHandler ********************************
 *
 *  Description: Wrapper for P12_TxIrq
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: returns: always 0
 *  Globals....: P12_Handle
 ****************************************************************************/
u_int32 IrqHandler(void)
{
	P12_TxIrq(&P12_Handle);
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
					P12_TX_DATA_SIZE*4, 
					(char*) &P12_TX_DATA );
}

void p12ShowTxIrqCount( char *devName )
{
	FILE *fd;
	
	if( devName != NULL )
	{
		fd = fopen( devName, "w" );
		if( fd )
		{
			fprintf( fd, "P12: irqcount %d\n", P12_Handle.txIrqCount );
			fclose( fd );
		}
	}/*if*/
}/*p12ShowTxIrqCount*/

#endif /* VXWORKS */

/********************************* p12Test **********************************
 *
 *  Description:  Setup the DEC21143. Clear the receive buffers.
 * 				  Send buffer, try to receive.
 *                Compare the received buffer and check if transmit irq
 *                happens.
 *
 *---------------------------------------------------------------------------
 *  Input......:  p12Handle	handle to the P12 parameter
 *
 *  Output.....:  return 0 | 1 on error
 *
 *  Globals....:  -
 ****************************************************************************/
static int p12Test
( 
	P12_HANDLE *p12Handle
)
{
	int x, i, idx;
	unsigned long len;
    unsigned long csr6, val;

	
	/* soft reset */
	setMaskCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr0_busmode, 0x00000001 );
	ms_delay( 100 );
	clrMaskCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr0_busmode, 0x00000001 );

	/* blinking the current P12 */
	toggleLedsGPIO_1_3( p12Handle->bigEndian, p12Handle->baseAddr );


	/* soft reset */
	setMaskCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr0_busmode, 0x00000001 );
	ms_delay( 100 );
	
	/* media chip reset and selection of activity LED */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr15_sia_general, 
			0x08af0008 );
	ms_delay( 100 );
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr15_sia_general, 
			0x00050008 );

    /* turn off reset and set cache align=16lword, burst=unlimit */
	if( !p12Handle->bigEndian )
		setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr0_busmode, 0x01A08000 );
	else
		setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr0_busmode, 0x01B08000 ); /*DBO bit 20- decriptor byte ordering - big endian*/

	

	/*------------------------------------------------+
	|  Perform ethernet 10BASET HD INTERNAL LOOP      |
	+------------------------------------------------*/
	csr6 = 0x0200C000 /* threshold */ | CSR6_SC | CSR6_CA | CSR6_RA | CSR6_PR | CSR6_OM_INTERNAL_LOOP;
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr6_command, csr6 );

	csr6 &= ~CSR6_PS;
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr6_command, csr6 );
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr13_sia_connectivity, 0 );  /* SIA RESET   */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr14_sia_tx_rx, 0x7f3f );	   /* 10baseT HD  */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr13_sia_connectivity, 1 );  /* ! SIA RESET */

	/* enable transmit irq */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr7_intr, CSR7_NIE | CSR7_TUE | CSR7_TIE );

	if( p12Handle->verbose > 2 )
		dumpCsr( p12Handle->bigEndian, p12Handle->baseAddr );


    /* construct setup packet which is used by the 21143 to
       program its CAM to recognize interesting MAC addresses */

	bzero( (char*) &txd,sizeof(struct txrxdesc));
	txd.buf1addr = (unsigned long)&txb[0] | p12Handle->pciToCpuMemOffs;
	txd.buf2addr = (unsigned long)&txb[0] | p12Handle->pciToCpuMemOffs;         /* just in case */
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
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr4_txlist, p12Handle->pciToCpuMemOffs | (unsigned long) &txd );

	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr6_command, csr6 | CSR6_ST );  /* start transmitter */

	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr1_txpoll, 1 );  /* kick transmit */


	/* wait for setup packet complete sended */
	{
		int tout=100;
		while( txd.status & 0x80000000 ){
			if( tout-- ==  0){
				if( p12Handle->verbose ){
					printf("*** timeout in setup packet\n");
					return -1;
				}
			}				
			ms_delay( 1 );
		}
	}

	if( p12Handle->verbose > 2 )
		dumpCsr( p12Handle->bigEndian, p12Handle->baseAddr );

    /* setup receive descriptor */
	for(x=0;x<NRXD;x++) 
	{
            bzero((char*)&rxd[x],sizeof(struct txrxdesc));
            rxd[x].buf1addr = (unsigned long)&rxb[x][0] | p12Handle->pciToCpuMemOffs;
            rxd[x].buf2addr = 0;        /* not used */
            rxd[x].tdes1    = BUFLEN;
            rxd[x].status   = 0x80000000; /* give ownership it to 21143 */
	}/* for */
	rxd[NRXD - 1].tdes1  |= (0x008<<22);       /* Set Receive end of ring on last descriptor */
	rxd_tail = 0;

    /* tell DC211XX where to find rx descriptor list */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr3_rxlist, p12Handle->pciToCpuMemOffs | (unsigned long)&rxd[0] );

    /* start the receiver */
	setCsr( p12Handle->bigEndian, p12Handle->baseAddr, csr6_command, csr6 | CSR6_SR | CSR6_ST );

	/* send/receive frames n-times */
	for( i=0; i<nRuns || nRuns==0; i++ )
	{
		/* clear rx buffer */
		bzero( (char*)rxb, NRXD*BUFLEN );
		if( p12Handle->verbose > 1 )
			printf("rxb at 0x%08x\n", rxb );

		p12Handle->txIrqFired = 0;
		tulip_transmit(	p12Handle->verbose,
						p12Handle->baseAddr,
						p12Handle->pciToCpuMemOffs,
						p12Handle->bigEndian,
						P12_TX_DATA_SIZE*4, 
						(char*) &P12_TX_DATA );

		if( p12Handle->verbose > 2 )
			dumpCsr( p12Handle->bigEndian, p12Handle->baseAddr );

		for( x=0; x<5; x++ )
		{
			/* clear packet buffer */
			bzero( (char*)packet, MAX_BUFF );

			/* receive */
			len = tulip_receive( (char*)packet, p12Handle->verbose );
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
					if( p12Handle->verbose )
						printf("*** pattern mismatch\n");
					if( p12Handle->verbose > 1 )
						printf(" at %d %08x != %08x\n", idx, packet[idx], P12_TX_DATA[idx] );
					
					goto CLEANUP;
				}
			}/*for*/			
		}
		else
			goto CLEANUP;

		if( !p12Handle->txIrqFired )
			goto CLEANUP;
	
	}/*for*/

	return( 0 );

CLEANUP:
	if( p12Handle->verbose && !p12Handle->txIrqFired )
		printf("*** no TX IRQ\n");
	if( p12Handle->verbose && !len )
		printf("*** no receive\n");
	if( p12Handle->verbose && len != P12_TX_DATA_SIZE*4 )
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
	printf("Usage: p12_test [<-u=>] <-b= -d= -p= -m= -M -v=>\n");
#endif
#ifdef OS9000
	printf("Usage: p12_test [<-u=>] <-b= -d= -v=>\n");
#endif
	printf("Function:"
		   "\tThis is test for the P12 or on board DEC21143\n"
    	   "\t\trunning in internal loopback mode.\n"
    	   "\t\tThe test check all 32 address/data lines\n"
    	   "\t\tbus mastering and interrupts.\n"
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
	       "(c) by MEN mikro elektronik GmbH, %s\n\n", pcmip_p12_test_IdentString);
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
	unsigned long val = getCsr( P12_Handle.bigEndian, P12_Handle.baseAddr, csr15_sia_general );
	val &= ~(CSR15_MD_1 | CSR15_MD_3 );
	setCsr( P12_Handle.bigEndian, P12_Handle.baseAddr, csr15_sia_general, val );
	_osmen_irq( G_irq, 2, NULL, _glob_data );	
	exit(error);
#endif
}

/********************************* main ***************************
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
	int noIrqInstallation;
	unsigned long  baseAddr = 0;
	unsigned char  irq;
#endif
#ifdef OS9000
	u_int32 baseAddr=0;
	u_int8 irq;
	u_int16 venId, devId;
	u_int32 pciIoBaseOff, pciMemBaseOff, irqOff;
#endif
	
	printf("\nP12 Test %s\n", pcmip_p12_test_IdentString );

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
		printf("\n%s\n", pcmip_p12_test_IdentString );
		printf("\tparameter:\n");
		printf("\t\tpciBus   :\t%d\n",         pciBus          );
		printf("\t\tpciDevice:\t%d\n",         pciDevice       );
		printf("\t\tunitNbr  :\t%d\n",         unitNbr         );
		printf("\t\tverbose  :\t%d\n",         verbose         );
		printf("\t\tbigEndian:\t%d\n",         bigEndian       );
#ifdef VXWORKS
		printf("\t\tcpuToPciMemOffs:\t%08x\n", (int)cpuToPciMemOffs );
		printf("\t\tpciToCpuMemOffs:\t%08x\n", (int)pciToCpuMemOffs );
		printf("\t\tnoIrqInstall   :\t%08x\n", (int)noIrqInstallation );
#endif		
	}/*if*/
	
#ifdef VXWORKS	
	if( unitNbr != 0xFFFF )
	{
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
    pciConfigInLong (	pciBus, 
		 				pciDevice, 
		 				pciFunc,
                    	PCI_CFG_BASE_ADDRESS_1,
                    	(int*) &baseAddr);
    
    if( baseAddr == 0xFFFFFFFF )
    {
   		printf("*** no P12 at bus/dev/func %d/%d/%d\n",
   		       pciBus, pciDevice, pciFunc );
   		goto CLEANUP;
	}/*if*/
    
    
    baseAddr +=cpuToPciMemOffs;
                   
    pciConfigInByte (	pciBus, 
		 				pciDevice, 
		 				pciFunc,
                    	PCI_CFG_DEV_INT_LINE,
                    	&irq);

	/* sleep mode disable */
    pciConfigOutByte (	pciBus, 
		 				pciDevice, 
		 				pciFunc,
                    	PCI_CFG_MODE,
                    	SLEEP_MODE_DIS);
	

	BIG_Endian = bigEndian;
 	BaseAddr   = baseAddr;

	if( verbose )
	{
    	printf("\t P12 #%d found\n", unitNbr );
	}/*if*/

	key = intLock(); /* avoid problems with shared IRQ's */
	P12_Handle.baseAddr      	= baseAddr;      
	P12_Handle.pciToCpuMemOffs	= pciToCpuMemOffs;
	P12_Handle.bigEndian     	= bigEndian;     
	P12_Handle.txIrqFired    	= 0;    
	P12_Handle.txIrqCount    	= 0;    
	P12_Handle.verbose       	= verbose;       
	intUnlock( key ); /* avoid problems with shared IRQ's */
	

	/*-------------------------+	
	| interrupt installation   |
	*-------------------------*/
	if( irq >= MAX_IRQS )
	{
    	printf("*** IRQ out of range\n" );
    	goto CLEANUP;
	}/*if*/

	if( !P12_IrqIsInstalled[irq] && !noIrqInstallation )
	{
#ifdef __LITTLE_ENDIAN
		P12_IrqIsInstalled[irq] = 1;
	    pciIntConnect( (VOIDFUNCPTR *)INUM_TO_IVEC(0x20+irq),
			            (void*) P12_TxIrq,
			            (int)   &P12_Handle );
		sysIntEnablePIC( irq );
#else
		P12_IrqIsInstalled[irq] = 1;
		intConnect( INUM_TO_IVEC( (int)irq ),
		            (void*) P12_TxIrq,
		            (int)   &P12_Handle );
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
		for( pciBus = 0; pciBus < 0x100; pciBus++ ){
			for( pciDevice = 0; pciDevice < 32; pciDevice++ ){
				_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_VENDOR_ID, &venId );
				_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_DEVICE_ID, &devId );

				if( venId==PCI_VENDOR_ID_DEC && devId==PCI_DEVICE_ID_DEC21143 ){
					if( verbose )
						printf("Unit #%d on PCI %d/%d/%d\n", index, pciBus, pciDevice, pciFunc );
					if( index++ == unitNbr){
						found=1;
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
	_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_VENDOR_ID, &venId );
	_osmen_pci_read_cfg_word( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_DEVICE_ID, &devId );
	if( venId!=PCI_VENDOR_ID_DEC || devId!=PCI_DEVICE_ID_DEC21143 ){
		printf("*** not a P12 on %d/%d/%d\n", pciBus, pciDevice, pciFunc );
		goto CLEANUP;
	}
		
		
	/* get address + irq */
	_osmen_pci_read_cfg_long( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_BASE_1, &baseAddr );
	_osmen_pci_read_cfg_byte( pciBus, pciDevice, pciFunc, PCI_REG_ADDR_INT_LINE, &irq );

	/* get baseaddr/irq offsets */	
	_osmen_pci_mapping( &pciIoBaseOff, &pciMemBaseOff, &irqOff );
	baseAddr += pciMemBaseOff;

	P12_Handle.baseAddr      	= baseAddr;      
	P12_Handle.pciToCpuMemOffs	= pciMemBaseOff == 0xC0000000 ? 0x80000000 : 0x00000000;
	P12_Handle.bigEndian     	= bigEndian;     
	P12_Handle.txIrqFired    	= 0;    
	P12_Handle.verbose       	= verbose;       

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


   	printf("\tbus/dev/func %d/%d/%d at %08x irq %d\n", pciBus, pciDevice, pciFunc, (int)baseAddr, irq );

	/*-------------------------+	
	| run the test             |
	*-------------------------*/
	error = p12Test( &P12_Handle );
	if( error )
		printf("*** TEST FAILED\n");
	else
		printf("OK\n");
	  
CLEANUP:
	Cleanup(error);
	return error;
}/*p12test*/


