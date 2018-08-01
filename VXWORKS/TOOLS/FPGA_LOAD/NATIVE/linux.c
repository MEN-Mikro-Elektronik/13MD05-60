/*********************  P r o g r a m  -  M o d u l e *************************/
/*!
 *         \file linux.c
 *      \project 13z100-91
 *
 *       \author Christian.Schuster@men.de
 *        $Date: 2005/01/31 13:58:18 $
 *    $Revision: 1.5 $
 *
 *        \brief Linux specific functions\n
 *               Bus errors are not caught and cause program termination.
 *
 *
 *     Required: -
 *    \switches  Z100_IO_MAPPED_EN:	switch must not be set for PPC compilers
 */
 /*---------------------------[ Public Functions ]----------------------------
 *
 *  void Z100_Os_init()
 *  void Z100_Os_exit()
 *  u_int32 Z100_Os_access_address( physadr, size, read, value, be_flag )
 *  int Z100_Os_findPciDevice( dev, allPciDevs[], numDevs, show_all )
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: linux.c,v $
 * Revision 1.5  2005/01/31 13:58:18  cs
 * fixed error message in function findPciDevices (vendor and device ID now printed)
 *
 * Revision 1.4  2005/01/21 13:36:50  cs
 * cosmetics
 *
 * Revision 1.3  2004/12/23 15:10:50  cs
 * removed user interaction
 * bugfixes: munmap now called with correct size
 * moved debug output of pci devices to fpga_load.c
 * changed name of some defines to Z100_*
 * cosmetics
 *
 * Revision 1.2  2004/12/13 18:03:35  cs
 * cosmetics for documentation and debug messages
 *
 * Revision 1.1  2004/11/30 18:04:58  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2004 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ******************************************************************************/

#include <MEN/men_typs.h>
#include "../COM/fpga_load.h"
#include "fpga_load_os.h"

#ifdef LINUX

#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#ifdef Z100_IO_MAPPED_EN
#	include <sys/io.h>
#endif

static int Os_MapMemory( u_int32 start, u_int32 size );

/**************************************************************
 *
 *   DEFINES
 *
 **************************************************************/

 #define Z100_LINUX_PROC_PCI_FILE "/proc/bus/pci/devices"
/**************************************************************
 *
 *   TYPEDEFS
 *
 **************************************************************/
/** Structure holding information about mapped memory area */
typedef struct Z100MEM_MAPPED {
	u_int32  start;			/**< start of mapped memory */
	u_int32  size;			/**< size of mapped memory */
	void*    map_adr;		/**< pointer to mapped memory area */
} Z100_MemMapped;

/**************************************************************
 *
 *   GLOBALS
 *
 **************************************************************/

static int Memdev;
static int G_os_end_fpga_load = 0;
static Z100_MemMapped Z100_MMapped;

/******************************** Z100_Os_init ********************************/
/** perform OS specific initialization
 *
 *  \param h		\OUT		pointer to DEV_HDL handle
 *
 ******************************************************************************/
extern void Z100_Os_init(DEV_HDL **h)
{
	u_int32 i;
	u_int8 *tempBuf;
	extern int assemble(), disassemble(), execute_program();
	void Catch_sigint(), Catch_sigbus();

	if( !(*h = (DEV_HDL *)malloc(sizeof(DEV_HDL))) ) {
                printf("\nERROR allocating memory for device handle\n");
                Z100_Os_exit(h);
                return;
	}
	tempBuf = (u_int8*)*h;
	for( i=0; i<sizeof(DEV_HDL); i++)
		*tempBuf++ = (u_int8)0;

	(*h)->flashDev.devHdl = *h;
	signal( SIGINT, Catch_sigint );
	signal( SIGBUS, Catch_sigbus );
	signal( SIGSEGV, Catch_sigbus );

	Z100_MMapped.start = 0;
	Z100_MMapped.size  = 0;
	Z100_MMapped.map_adr = NULL;

	if( (Memdev = open( "/dev/mem", O_RDWR )) < 0 ){
		printf("ERROR: can't open /dev/mem");
		Z100_Os_exit(h);
		return;
	}
#ifdef Z100_IO_MAPPED_EN
	if( iopl(3) != 0) {
		printf( "ERROR -iopl failed, root privileges needed\n"
				"       IO-mapped memory access will not work\n");
	}
#endif
}

/******************************* Z100_Os_exit *********************************/
/** perform OS specific cleanup
 *
 *  \param h		\IN		pointer to DEV_HDL handle
 *
 ******************************************************************************/
void Z100_Os_exit(DEV_HDL **h)
{
	if(*h)
		free(*h);
	*h = NULL;

	/* unmap  mapped memory */
	if( Z100_MMapped.map_adr )
		munmap( Z100_MMapped.map_adr, Z100_MMapped.size );

	if( Memdev > 0 )
		close( Memdev );
}
#if 0
/********************************* Z100_Os_get_char ***************************/
/** get character from stdin
 *
 *  \return ascii code of character read
 *
 ******************************************************************************/
static int Z100_Os_get_char()
{
	char c;

	if( read( 0, &c, 1 ) == -1 )
		return -1;
	return c;
}
#endif
/********************************* Z100_Catch_sigx ****************************
 * catch bus error signal
 ******************************************************************************/
void Catch_sigbus()
{
	printf(" *** bus error\n");
	signal( SIGSEGV, SIG_IGN );
}

void Catch_sigint()
{
	printf(" ^C");
	G_os_end_fpga_load = 1;
	/* FLUSH; */
}

/******************************* Z100_Os_access_address ***********************/
/** read or write memory
 *
 *  \param bar		\IN		PCI BAR
 *  \param offs		\IN		offset from BAR
 *  \param size		\IN		1/2/4 size of access in bytes
 *  \param read		\IN		1 = read, 0 = write access
 *  \param value	\IN		value to be written to memory
 *  \param be_flag	\OUT	is set if buserror occurred (not yet implemented)

 *  \return nothing on writes
 *          or read value
 ******************************************************************************/
extern u_int32 Z100_Os_access_address( 	u_int32 bar,
										u_int32 offs,
										int size,
										int read,
										u_int32 value,
										int *be_flag )
{

#ifdef Z100_IO_MAPPED_EN
	if( bar & 0x01 ) {
		/* BAR is I/O mapped */

		DBGOUT( "Z100_Os_access_address IO: bar: 0x%08x offs: 0x%x  size:%d  "
				"read:%d value: 0x%08x\n",
 				bar, offs, size, read, value );

		/*-----------------+
		| access registers |
		+-----------------*/
		if( read ){
			switch(size){
				case 1: 	value = inb((unsigned short int)((bar&~0xFF)+offs)); break;
				case 2: 	value = inw((unsigned short int)((bar&~0xFF)+offs)); break;
				case 4: 	value = inl((unsigned long)((bar&~0xFF)+offs)); break;
			}
			DBGOUT( "Z100_Os_access_address IO: read value: 0x%08x\n", value );
		} else {
			switch(size){
				case 1: 	outb((unsigned char)value,      (unsigned short int)((bar&~0xFF)+offs)); break;
				case 2: 	outw((unsigned short int)value, (unsigned short int)((bar&~0xFF)+offs)); break;
				case 4: 	outl((unsigned int)value,       (unsigned short int)((bar&~0xFF)+offs)); break;
			}
		}
	} else
#endif
	{
		/* BAR is memory mapped */
		void *adr;
		u_int32 physadr = (bar&~0xFF)+offs;

		DBGOUT( "Z100_Os_access_address mem: 0x%x  size:%d  "
				"read:%d value: 0x%08x\n",
				 physadr, size, read, value );

		if( (!Z100_MMapped.map_adr || Z100_MMapped.map_adr == MAP_FAILED ||
			 (physadr < Z100_MMapped.start) ||
			 (physadr > (Z100_MMapped.start + Z100_MMapped.size)) ) &&
			Os_MapMemory(physadr, 0x1000)) {
			printf("ERROR - Z100_Os_access_address failed\n");
			return(-1);
		}

	    adr = Z100_MMapped.map_adr + (physadr & 0xfff);

		/*--------------+
		| access memory |
		+--------------*/
		if( read ){
			switch(size){
                                case 1:         value = *(volatile u_int8*)adr; break;
                                case 2:         value = *(volatile u_int16*)adr; break;
                                case 4:         value = *(volatile u_int32*)adr; break;
			}
			DBGOUT( "Z100_Os_access_address mem: read value: 0x%08x\n", value );
		} else {
			switch(size){
                                case 1:         *(volatile u_int8*)adr  = value; break;
                                case 2:         *(volatile u_int16*)adr = value; break;
                                case 4:         *(volatile u_int32*)adr = value; break;
			}
			msync( Z100_MMapped.map_adr, 4, MS_SYNC );
		}

	}
	/* *be_flag = 0; */
    return value;
}

/********************************* Os_MapMemory *******************************/
/** map memory on Mem Mapped BARs
 *  address of mapped memory is stored in global variable
 *
 *  \param start	 \IN		physical start address
 *  \param size		 \IN		size of needed memory space in bytes
 *
 *  \return success (0) or error (-1)
 *          or read value
 ******************************************************************************/
static int Os_MapMemory( u_int32 start, u_int32 size )
{
	int32 pagesize = sysconf(_SC_PAGESIZE);
	int32 map_size;

	map_size = pagesize * (size/pagesize);
	if( size%pagesize )
		map_size += pagesize;

	DBGOUT("pagesize = 0x%08x; map_size = 0x%08x\n", pagesize, map_size);

	/* unmap previous mapped memory */
	if( Z100_MMapped.map_adr )
		munmap( Z100_MMapped.map_adr, Z100_MMapped.size );

	/* mmap offset parameter must be a multiple of the page size */
	Z100_MMapped.map_adr = mmap( NULL, map_size, PROT_READ|PROT_WRITE,
								 MAP_SHARED, Memdev, start & ~(pagesize-1) );

	if( Z100_MMapped.map_adr == MAP_FAILED ){
		printf("ERROR: Couldn't map physical memory\n");
		return(-1);
	} else {
		Z100_MMapped.start = start & ~(pagesize-1);
		Z100_MMapped.size = map_size;
	}
	return(0);
}


/**************************** Z100_Os_findPciDevice ***************************/
/** find PCI device specified by vendor and device ID and get parameters
 *
 *  \param dev			\IN		PCI_DEVS handle
 *  \param allPciDevs	\OUT	structure to fill with found devices
 *  \param numdevs		\OUT	number of PCI devices found
 *  \param show_all		\IN		return all PCI devices in system
 *
 *  \return success (0) or error (-1)
 *          or read value
 ******************************************************************************/
extern int Z100_Os_findPciDevice( PCI_DEVS *dev,
								  PCI_DEVS* allPciDevs[],
								  u_int32 *numDevs,
								  int show_all )
{
	char line[300];
	FILE *fp = fopen(Z100_LINUX_PROC_PCI_FILE, "r" );
	int vendorId=dev->venId,
		deviceId=dev->devId;
	uint32_t busdev;
	uint32_t vendev, irq;
	PCI_DEVS curldev, *curdev = &curldev;
	int n;

	*numDevs = 0;

	/* open file with PCI bus and device information */
	if( fp == NULL ){
		printf("ERROR: can't open %s\n", Z100_LINUX_PROC_PCI_FILE);
		goto error_end;
	}

	curdev = allPciDevs[0];

	while( fgets( line, sizeof(line), fp ) && !G_os_end_fpga_load ){
		/* printf("%s(%d) line:\n %s\n", __FUNCTION__, __LINE__, line); */
		if( (n = sscanf( line, "%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x\t%x",
					&busdev, &vendev, &irq,
					&curdev->bar[0], &curdev->bar[1], &curdev->bar[2],
					&curdev->bar[3], &curdev->bar[4], &curdev->bar[5] )) != 9 )
			fprintf(stderr,"Error converting line:\n %s n=%d\n", line, n );
		else {
			curdev->venId = vendev>>16;
			curdev->devId = vendev & 0xFFFF;
			curdev->bus   = busdev>>8;
			curdev->dev   = (busdev & 0xff) >>3;
			curdev->fun   = busdev & 0x07;
			if( !show_all && (((vendorId<<16)|deviceId) == vendev) ){
			(*numDevs)++;
				if(curdev == allPciDevs[Z100_MAX_DEVICES-1]) {
					/* found Z100_MAX_DEVICEs */
					break;
				}
				curdev++;
			} else if(show_all) {
				(*numDevs)++;
				curdev++;

			}
		}
	}

	if( !(*numDevs) ){
		printf( "ERROR: No PCI devices matching vendor ID %04x"
				" and device ID %04x found!\n", vendorId, deviceId);
		dev=NULL;
		goto error_end;
	}
	/* printf( "Found %d PCI device(s) matching:\n"
	 *		"Nr. | bus Nr. | dev Nr. |"
	 *		"   BAR0   |   BAR1   |   BAR2\n", *numDevs);
	 * for(n = 0; n < *numDevs; n++) {
	 *	printf( " %2d | %5d   | %5d   | %08x | %08x | %08x\n",
	 *			n, allPciDevs[n]->bus, allPciDevs[n]->dev,
	 *			allPciDevs[n]->bar[0], allPciDevs[n]->bar[1], allPciDevs[n]->bar[2]);
	 *}
	 */

	fclose(fp);
	return 0;
error_end:
	if( fp )
		fclose(fp);
	return( -1 );
}

#endif /* LINUX */
