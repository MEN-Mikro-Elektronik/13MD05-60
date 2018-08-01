/*********************  P r o g r a m  -  M o d u l e ***********************/
/*
 *        \file  p699_test.c
 *
 *      \author  ts
 *        $Date: 2008/09/09 10:35:06 $
 *    $Revision: 1.3 $
 *
 *  	\brief	P699 DMA Test Tool for 16Z062 DMA Core
 *           	
 *
 *---------------------------[ History ] --------------------------------------
 *
 * $Log: p699_test.c,v $
 * Revision 1.3  2008/09/09 10:35:06  ts
 * R: 1.) cosmetics, too many global Variables
 * M: 1.) typedef'ed struct p699_data with these Variables that gets dynamically allocated
 *
 * Revision 1.2  2007/12/14 14:44:02  ts
 * First Working Version, tested with Le Croy PCIe Tracer & P699 SN#3
 *
 * Revision 1.1  2007/12/12 15:26:52  ts
 * Initial Revision, Test P699 PCIe Transfers and DMA
 *
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "vxWorks.h"
#include "version.h"
#include "stdlib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "errno.h"
#include "errnoLib.h"
#include "memLib.h"

#include <MEN/men_typs.h>
#include <MEN/maccess.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/oss.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

static void usage(void);

/* keep all this in a struct rather than globals */
typedef struct p699_data {
	u_int32  	adrBAR0; 
	u_int32  	adrBAR1;
	int32 	 	nrRuns;
	int32 	 	nrIrq;
	int32 	 	doCheck; 
	int32 	 	size;	 
	int32 	 	offset;  
	int32 	 	finished;
	int32 		errCount;
	u_int32 	*txbufP;  
	u_int32 	*rxbufP;  
} P699_DAT;

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#define CHK(expression) \
 if((expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
     #expression,__FILE__,__LINE__);\
     goto ABORT;\
 }

/* 
 * DMA Buffer Registers / bits 
 * DMA BD Registers, offset to <BAR0> 
 */

/* To this adress DMA WRITES Data (e.g.to RAM)*/
#define REG_DMA_DST_ADDR		0x00
/* From this adress DMA READS Data */
#define REG_DMA_SRC_ADDR		0x04

#define REG_DMA_SIZE			0x08

#define REG_DMA_CONTROL			0x0c

/* Registers relative from BAR1 + 0x0 */
#define REG_DMA_STAT_CTRL		0x00

#define DMA_STAT_EN				0x01
#define DMA_STAT_IEN			0x02
#define DMA_STAT_IRQ			0x04
#define DMA_STAT_ERR			0x08
#define DMA_STAT_BDNUM			4

/* bits in control Reg */
#define DMA_CTRL_TERM			0x01
#define DMA_CTRL_INC_DST		0x02
#define DMA_CTRL_INC_SRC		0x04
#define DMA_CTRL_BURST_DST		0x10
#define DMA_CTRL_BURST_SRC		0x20
#define DMA_DEV_DST_SHIFT		12
#define DMA_DEV_SRC_SHIFT		16

#define DMA_SRC_LOC_RAMBD		0x01
#define DMA_SRC_LOC_RAMA		0x02
#define DMA_SRC_LOC_RAMB		0x04

#define DMA_DST_LOC_RAMBD		0x01
#define DMA_DST_LOC_RAMA		0x02
#define DMA_DST_LOC_RAMB		0x04

# define PCI_SEEK_VENDOR		0x1556
# define PCI_SEEK_DEVICE		0x5555

#define IRQ_OFFSET        		0x20       /* irq offset */
#define TSTART 					0x0
#define TEND   					0x200

/*------------------+
|   GLOBALS         |
+------------------*/

/* the only global data pointer */
P699_DAT	*theP699DatP = NULL;

/********************************* usage ***********************************/
/**  Print program usage
 */
static void usage(void)
{
	printf("Usage:\n");
	printf(" p699_test -s=size [-n=runs] [-o=offs] [-b][-d]\n" );
	printf(" Tool to perform P699 DMA Tests\n");
	printf("Options: \n");
	printf("  Syntax    Meaning                               default\n");
	printf("---------------------------------------------------------\n");
	printf(" -d         do NOT compare TX and RX Buffer       do compare\n");
	printf(" -s=<size>  transfersize in 32bit longs           256\n");
	printf(" -n=<runs>  nr. of DMA transfer runs to perform   1\n");
	printf(" -o=<offs>  Value to add to the tx/rxBuf Address  0\n");
	printf(" -b         Do NOT burst                          bursts on\n");
	printf("\n(c) 2007  by MEN mikro elektronik GmbH\n\n");
}


/***************************************************************************/
/**  helper to free a pointer if previously allocated by malloc
 *
 *  \param mem      \IN malloced pointer to free or NULL
 *
 *  \return	        -
 */
static void do_free(void *mem )
{
	if (mem)
		free(mem);
}




/***************************************************************************/
/**  Irq handler
 *
 *  \param ptr	  \IN free private driver data to pass at ISR registration
 *
 *  \return	      -
 */
LOCAL void dma_isr( void *ptr ) 
{

	P699_DAT *datP 	= (P699_DAT *)ptr;
	u_int32 pend = MREAD_D32(datP->adrBAR1, REG_DMA_STAT_CTRL);


	if ( pend & DMA_STAT_IRQ ) { 	/* our Irq ? */

		/* clear Irq by writing 1 back but disable DMA  */
		MWRITE_D32(datP->adrBAR1, 
				   REG_DMA_STAT_CTRL, pend & (~DMA_STAT_EN) );

		datP->nrIrq++;
		
		if (datP->nrIrq < datP->nrRuns) {

			MWRITE_D32(datP->adrBAR1, REG_DMA_STAT_CTRL, pend | DMA_STAT_EN);

		} else { /* were done! */

			printf("<Last ISR>\n");
			datP->finished = 1;
			MWRITE_D32(datP->adrBAR1, REG_DMA_STAT_CTRL, 0 );
		}
	}
}




/***************************************************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 *
 *  \brief  The DMA Core Layout is as follows:
 *          BAR0 0x000 - 0x3ff Buffer descriptors list
 *          BAR1 0x000   DMA Config / Status Reg
 *
 */
int main(int argc, char *argv[])
{
	char	 	buf[40], *errstr = NULL, *str;
	int32	 	irqVector=0, burst=0;
	int32 		i=0;
	STATUS		retval=0;
	int 		pciVendor	= PCI_SEEK_VENDOR;
	int 		pciDevice	= PCI_SEEK_DEVICE;
	int 		pciBus		= 0;
	int 		pciDev		= 0;
	int 		pciFunc		= 0;
	u_int32  	data 		= 0;

	printf("MEN P699 DMA test tool built %s %s. RAND_MAX=0x%08x\n",
		   __DATE__, __TIME__, RAND_MAX );
	
	/*----------------------+
    |  check arguments      |
    +----------------------*/
	if( (errstr = UTL_ILLIOPT("bds=o=n=", buf)) ){
		printf("*** %s\n", errstr);
		return(1);
	}

	if( argc < 2 ){
		usage();
		return(1);
	}

	CHK( !(theP699DatP = (P699_DAT*)malloc( sizeof(P699_DAT))) );	
	
	memset( theP699DatP, 0x00, sizeof(P699_DAT) );

	theP699DatP->nrRuns = ((str = UTL_TSTOPT("n=")) ? atoi(str) : 1);
	theP699DatP->offset = ((str = UTL_TSTOPT("o=")) ? atoi(str) : 0);
	theP699DatP->size 	= ((str = UTL_TSTOPT("s=")) ? atoi(str) : 128);
	burst 				= (UTL_TSTOPT("b") ? 
						   0 : (DMA_CTRL_BURST_DST|DMA_CTRL_BURST_SRC));
	theP699DatP->doCheck = (UTL_TSTOPT("d") ? 0 : 1 );

	/* 1. Allocate & init a 1k aligned Tx Rx Buffer, use 2xsize internal */

#if 0
	CHK(!(theP699DatP->rxbufP = 
		  (u_int32*)memalign( 0x1000, 
							  theP699DatP->size * 2 * sizeof(u_int32))));
	CHK(!(theP699DatP->txbufP = 
		  (u_int32*)memalign( 0x1000, 
							  theP699DatP->size * 2 * sizeof(u_int32))));
#else

	CHK(!(theP699DatP->rxbufP = 
		  (u_int32*)malloc( theP699DatP->size * 2 * sizeof(u_int32))) );
	CHK(!(theP699DatP->txbufP = 
		  (u_int32*)malloc( theP699DatP->size * 2 * sizeof(u_int32))) );

#endif

	memset(theP699DatP->rxbufP, 0x00, theP699DatP->size * 2 * sizeof(u_int32) );
	memset(theP699DatP->txbufP, 0x00, theP699DatP->size * 2 * sizeof(u_int32) );

	/* inform user */
	printf("size:    0x%04x\n", theP699DatP->size ); 
	printf("tx Buf:  0x%08x\n", theP699DatP->txbufP ); 
	printf("rx Buf:  0x%08x\n", theP699DatP->rxbufP ); 
	printf("runs %d offset %d bursting %s ErrCheck %s\n",	
		   theP699DatP->nrRuns, theP699DatP->offset, burst ? "ON" : "OFF",
		   theP699DatP->doCheck ? "ON" : "OFF"	); 

	/* init source Buffer from which DMA reads with a pattern */
	for (i = 0; i < theP699DatP->size; i++)
		MWRITE_D16( theP699DatP->txbufP, ((i*2) + theP699DatP->offset), rand());
		
	/* 2. Find the P599 FPGA (PCI Core) */
	retval = pciFindDevice(pciVendor, pciDevice,0,&pciBus, &pciDev, &pciFunc);
	if (retval == OK) {
		printf("Found P699, Bus %d Dev %d Func %d\n", pciBus, pciDev, pciFunc);
	} else {
		printf("*** not found!\n");
		return(ERROR);
	}

	OSS_PciGetConfig( NULL, pciBus, pciDev, pciFunc, OSS_PCI_ADDR_0, 
					  &theP699DatP->adrBAR0);
	
	OSS_PciGetConfig( NULL, pciBus, pciDev, pciFunc, OSS_PCI_ADDR_1, 
					  &theP699DatP->adrBAR1);
	
	if (theP699DatP->adrBAR0 & 0x8)
		theP699DatP->adrBAR0-=8;

	if (theP699DatP->adrBAR1 & 0x8)
		theP699DatP->adrBAR1-=8;
	
	OSS_PciGetConfig(NULL,pciBus,pciDev,pciFunc,OSS_PCI_INTERRUPT_LINE, 
					 &irqVector);
	printf("BAR0 0x%08x  BAR1 0x%08x IRQ %d\n",
		   theP699DatP->adrBAR0, theP699DatP->adrBAR1, irqVector);

	/* 	3. setup DMA buffer, Control Reg */
	MWRITE_D32( theP699DatP->adrBAR0, 
				REG_DMA_DST_ADDR, 
				((u_int32)theP699DatP->rxbufP) + theP699DatP->offset);
	
	MWRITE_D32( theP699DatP->adrBAR0, 
				REG_DMA_SRC_ADDR, 
				((u_int32)theP699DatP->txbufP) + theP699DatP->offset);

	MWRITE_D32( theP699DatP->adrBAR0, REG_DMA_SIZE, 	theP699DatP->size);

	/* build value for DMA Control  */
	data = 	DMA_CTRL_TERM | DMA_CTRL_INC_DST | DMA_CTRL_INC_SRC | burst |	
		DMA_DST_LOC_RAMA << DMA_DEV_DST_SHIFT | 
		DMA_SRC_LOC_RAMA << DMA_DEV_SRC_SHIFT ;

	printf("write 0x%08x to DMA Ctrl Reg\n", data );
	MWRITE_D32(theP699DatP->adrBAR0, REG_DMA_CONTROL, data);

	/* hook in interrupt Handler */
	pciIntConnect(INUM_TO_IVEC(irqVector+IRQ_OFFSET ), (VOIDFUNCPTR)dma_isr, 
				  (void*)theP699DatP);

	sysIntEnablePIC( irqVector );
	
	/* clear any spurious/previous IRQ */
	MWRITE_D32(theP699DatP->adrBAR1, REG_DMA_STAT_CTRL, DMA_STAT_IRQ );
	printf("Enabling/starting DMA:\n");
	data = 	MREAD_D32(theP699DatP->adrBAR1, REG_DMA_STAT_CTRL );
	data |= (DMA_STAT_EN | DMA_STAT_IEN);
	MWRITE_D32(theP699DatP->adrBAR1, REG_DMA_STAT_CTRL, data);

	printf("Waiting until %d DMA IRQs occured\n", theP699DatP->nrRuns);

#if 1
	while ( theP699DatP->finished == 0 ) {
		UOS_Delay(1);
	}
#else
	UOS_Delay(5000);
#endif

	for (i=0; i < 5; i ++) {
		printf("End program in %d sec.\n", i );
		UOS_Delay(1000);
	}

	sysIntDisablePIC( irqVector ); 
	printf("Done. Cycles with difference in Tx/Rx buffer: %d\n",
		   theP699DatP->errCount );
	
	/*--------------------+
	 |  cleanup            |
	 +--------------------*/

	UOS_Delay(50);
	do_free( theP699DatP->txbufP 	);
	do_free( theP699DatP->rxbufP	);
	/* free the super Object itself */
	do_free( theP699DatP );
	return(0);

ABORT:

	do_free( theP699DatP->txbufP 	);
	do_free( theP699DatP->rxbufP	);
	do_free( theP699DatP );
	return(UOS_ErrnoGet());
}
