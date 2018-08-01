/************************   P C I L O A D   *********************************/
/*!
 *
 *        \file  pciboot.c
 *
 *      \author  rt
 *        $Date: 2011/05/04 11:44:44 $
 *    $Revision: 1.6 $
 *
 *       \brief  Tool to load FPGA configurations into Flash over PCI bus
 *
 *
 *     Required: uti.l (under OS-9), libuti.a (under Linux)
 *     \switches ONE_NAMESPACE_PER_TOOL influences names of public functions
 *
 */
/*---------------------------[ Public Functions ]-------------------------------
 * none
 *
 *---------------------------------[ History ]----------------------------------
 *
 * $Log: pciboot.c,v $
 * Revision 1.6  2011/05/04 11:44:44  rt
 * R: New HW revision model -00 with new PCI/PCIe bridge (new PCI device ID).
 * M: main() changed.
 *
 * Revision 1.5  2011/01/19 14:28:51  rt
 * R: New HW model -03 with new PCI/PCIe bridge (new PCI vendor ID).
 * M: main() changed.
 *
 * Revision 1.4  2011/01/19 14:13:48  rt
 * R: New HW model -03 with new PCI/PCIe bridge (new PCI vendor ID).
 * M: main() changed.
 *
 * Revision 1.3  2007/10/01 17:59:13  rtrübenbach
 * added:
 * -support for VxWorks
 * -option -e
 * -ep4 now hot pluggable
 * -binaries are verified before starting MPC
 *
 * Revision 1.2  2007/05/16 13:50:10  rtrübenbach
 * fixed:
 * + free memory (windows hang-up)
 *
 * Revision 1.1  2007/02/28 13:22:39  rtrübenbach
 * Initial Revision
 *
 *
 * (cloned from fpga_load.c V1.7)
 *
 *
 *------------------------------------------------------------------------------
 * (c) Copyright 2007-2011 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ******************************************************************************/

#include "pciboot.h"
#include <string.h>
#include "MEN/dbg.h"

/*-----------------------------------------+
 |  GLOBALS                                |
 +-----------------------------------------*/
/*-----------------------------------------+
 |  DEFINES                                |
 +-----------------------------------------*/
#ifdef DBG
#	define PCIB_OSS_DBG_LEVEL	DBG_LEVERR | DBG_NORM | 0x01
#else
#	define PCIB_OSS_DBG_LEVEL	DBG_LEVERR
#endif

#define PRINT_USAGE( verbose, _x_ ) \
	if( (verbose) & level ) fprintf( stderr, (_x_) );
#define USAGE_SMALL  0x01
#define USAGE_MIDDLE 0x02
#define USAGE_LARGE  0x04
#define USAGE_ALL    0xFF

#define PCIB_BRIDGE_COMMAND			(0x04 | OSS_PCI_ACCESS_16)
#define PCIB_BRIDGE_PRI_BUS			(0x18 | OSS_PCI_ACCESS_8)
#define PCIB_BRIDGE_SEC_BUS			(0x19 | OSS_PCI_ACCESS_8)
#define PCIB_BRIDGE_SUB_BUS			(0x1A | OSS_PCI_ACCESS_8)
#define PCIB_BRIDGE_SEC_STATUS		(0x1E | OSS_PCI_ACCESS_16)
#define PCIB_BRIDGE_MEM_BASE		(0x20 | OSS_PCI_ACCESS_16)
#define PCIB_BRIDGE_MEM_LIMIT		(0x22 | OSS_PCI_ACCESS_16)
#define PCIB_BRIDGE_PRE_MEM_BASE	(0x24 | OSS_PCI_ACCESS_16)
#define PCIB_BRIDGE_PRE_MEM_LIMIT	(0x26 | OSS_PCI_ACCESS_16)

#define CHK(expression) \
 if( !(expression)) {\
     printf("\n *** pciboot: Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     goto end;\
 }

#define CHKEX(expression, args ) \
 if( !(expression)) {\
     printf("\n *** pciboot: Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
     printf args;\
     printf("\n");\
     goto end;\
 }

#define PRINT_ERROR(info)  fprintf( stderr, \
 "\n *** can't %s: \nfile %s\nline %d\n", (info), __FILE__, __LINE__);

/*-----------------------------------------+
 |  TYPEDEFS                               |
 +-----------------------------------------*/
/*-----------------------------------------+
 |  PROTOTYPES                             |
 +-----------------------------------------*/
static void Usage( const u_int8 level );
static void Version( void );
static int RestoreConfigHeader( OSS_HANDLE *osHdl, DEV_HDL *h );
static int CheckConfigHeader( OSS_HANDLE *osHdl, DEV_HDL *h );
static int32 CopyFileToTarget( 	DEV_HDL* h, char* fName, MACCESS mappedAddr);
static void Print_PciDeviceInfo( PCI_DEVS **pciDevs,
								 u_int32 numDevs,
								 u_int8 bars  );
static int32 FindPciDevice( OSS_HANDLE *osHdl,
							PCI_DEVS* dev[],
							PCI_DEVS* allPciDevs[],
							u_int32 *numDevs,
							int show_all );


/********************************* Usage **************************************/
/** print usage
*
* \param level         \IN  verbosity level
 ******************************************************************************/
static void Usage( const u_int8 level )
{
    PRINT_USAGE( USAGE_ALL, 
                 "Syntax  : pciboot [<opts>]\n\n" );
    PRINT_USAGE( USAGE_MIDDLE | USAGE_LARGE,
                 "Function: download bootloader and OS binaries "
				 "to EP4(MPC0)\n\n" );
    PRINT_USAGE( USAGE_ALL, 
                 "Options :\n" );
    PRINT_USAGE( USAGE_SMALL, 
                 "         -?         for more help\n" );
    PRINT_USAGE( USAGE_MIDDLE | USAGE_LARGE,
                 "         -b=<name>  bootloader file........ [bootloader.bin]\n"
                 "         -o=<name>  operating system file.......... [os.bin]\n"
				 "         -i=<id>    desired target instance............. [0]\n"
                 "         -v=<num>   verbosity level (0-3)............... [1]\n"
                 "         -c=<addr>  CCSR BAR address................. [auto]\n"
                 "         -d=<addr>  DDR area BAR address............. [auto]\n"
                 "         -e         end programm after download............ \n"
				 "\n"
				 "EP4 has to be in HOLD-OFF mode (check DIP switches at TB1)\n"
				 "during Host is booting."
				);
    Version();
}


/******************************* Version **************************************/
/** print version info
 ******************************************************************************/
static void Version( void )
{
    char* rev = "$Revision: 1.6 $";
    char* date = "$Date: 2011/05/04 11:44:44 $";
    char* p = rev + strlen("$Revision: ");

    fprintf(stderr, "\nV ");
    while( *p != '$' ) 
    	fprintf(stderr, "%c", *p++);
    fprintf(stderr, " ");
    p = date + strlen("$Date: ");
    while( *p != '$' )
    	fprintf(stderr, "%c", *p++);
    	
    fprintf(stderr, " (c) Copyright 2007-2011 by MEN Mikro Elektronik GmbH\n");
}


/**********************************************************************/
/** print message
*
* \param level         \IN  verbosity
* \param fmt           \IN  message
*/
static void PrintMsg( DEV_HDL* dev, int level, char *fmt, ... )
{
    va_list ap;

    if( dev->dbgLevel >= level )
    {
        va_start(ap,fmt);
        vprintf( fmt, ap );
        va_end(ap);
    }
}

/********************************* main ***************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          0
 ******************************************************************************/
int 
#ifdef WINNT
_cdecl
#endif
main(int argc, char *argv[ ])
{
	int n;
    char*            str;
	u_int8 firstReset = 1;
	u_int8 endPrg = 0;
	int error = 0;
	u_int8 *tempBuf;
	PCI_DEVS *allPciDevs[PCIB_MAX_DEVICES], *tmpPciDev;
	PCI_DEVS *pciDev[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	PCI_DEVS *pciDevEmpty[] = {NULL};
	PCI_DEVS *pciBridge = NULL;
	DEV_HDL *h = NULL;
	u_int32 numDevs = 0, instance = 0, memsize;
	u_int32 devHdlGotSize = 0, i;
	u_int8 *tempP, *errstr, errbuf[40];
	OSS_HANDLE *osHdl = NULL;

	for(n = 0; n < PCIB_MAX_DEVICES; n++)
		allPciDevs[n] = NULL;

#if defined(WINNT) || defined(LINUX)
	if( OSS_Init( "FPGA_LOAD", &osHdl ) )
#elif defined(VXWORKS)
	if( OSS_Init() )
#endif /* WINNT / LINUX / VXWORKS */
	{
		printf("\nERROR initializing OSS\n");
		goto end;
	}

	if( !(h = (DEV_HDL *)OSS_MemGet( osHdl, sizeof(DEV_HDL), &devHdlGotSize )) ){
		printf("\nERROR allocating memory for device handle\n");
		goto end;
	}

	tempBuf = (u_int8*)h;
	for( i=0; i<sizeof(DEV_HDL); i++)
		*tempBuf++ = (u_int8)0;

	pciDev[0] = &(h->pciDev[0]);
	pciDev[1] = &(h->pciDev[1]);
	pciDev[2] = &(h->pciDev[2]);
	pciDev[3] = &(h->pciDev[3]);
	pciDev[4] = &(h->pciDev[4]);
	pciDev[5] = &(h->pciDev[5]);
	pciBridge = &(h->pciBridge);

    /*--------------------+
    |  check arguments    |
    +--------------------*/
    if(( errstr = UTL_ILLIOPT("?b=o=d=c=i=v=e", errbuf) ))   /* check args */
    {
        printf( " *** %s\n\n", errstr );
        Usage( USAGE_SMALL );
        return(1);
    }
    
    for (n=1; n<argc; n++)
    {
        if (*argv[n] != '-')
        {
            printf( " *** unrecognized option '%s'\n\n", argv[n] );
            Usage( USAGE_SMALL );
            return(1);
        }
    }

    if( UTL_TSTOPT("?") )                      /* help requested ? */
    {
        Usage( USAGE_MIDDLE );
        return(1);
    }

    /*--------------------+
    |  get arguments      |
    +--------------------*/
	instance = ((str = UTL_TSTOPT("i=")) ? atoi(str) : 0);
    h->bootFileName = ((str = UTL_TSTOPT("b=")) ? str : "bootloader.bin");
    h->osFileName = ((str = UTL_TSTOPT("o=")) ? str : "os.img");
	h->dbgLevel	  = ((str = UTL_TSTOPT("v=")) ? atoi(str) : 1);
	h->physAddrCcsr = (void*) ((str = UTL_TSTOPT("c=")) ?
						strtoul(str, (char **) NULL, 0x10) : 0x0);
	h->physAddrDdr = (void*) ((str = UTL_TSTOPT("d=")) ?
						strtoul(str, (char **) NULL, 0x10) : 0x0);
	endPrg = !!UTL_TSTOPT("e");

	if( h->dbgLevel >= 4 )
		OSS_DbgLevelSet(osHdl, PCIB_OSS_DBG_LEVEL );
	else 
		OSS_DbgLevelSet(osHdl, DBG_LEVERR );

    /*--------------------+
    |  search device      |
    +--------------------*/
    
	/* allocate and init memory to detect all PCI devices */
	memsize= PCIB_MAX_DEVICES*sizeof(PCI_DEVS);
	if( !(tmpPciDev = (PCI_DEVS*)malloc(memsize)) ) {
		printf("Insufficient ressources for PCI device information\n");
		error = 1;
		goto end;
	}
	tempP=(u_int8*)tmpPciDev;
	while (memsize--)
		*(tempP++) = (u_int8)0;

	for(n = 0; n < PCIB_MAX_DEVICES; n++)
		allPciDevs[n] = tmpPciDev++;

	/* print all pci devices in sytem */
	if( h->dbgLevel >= 5 ){
		printf( "All PCI devices available:\n");
		if( !(error = FindPciDevice( osHdl, pciDevEmpty, allPciDevs, &numDevs, 1)) ) 
			Print_PciDeviceInfo(allPciDevs, numDevs, 1);
	}

	/* search for PLX bridge */
    PrintMsg( h, 1, "\nSearch for target (PCIe/PCI bridge)..." );
	pciDev[0]->venId = PCIB_BRIDGE_VENDOR_ID;
	pciDev[0]->devId = PCIB_BRIDGE_DEVICE_ID_8111;
	pciDev[0]->subSysVenId = 0xDEAD;
	pciDev[1]->venId = PCIB_BRIDGE_VENDOR_ID;
	pciDev[1]->devId = PCIB_BRIDGE_DEVICE_ID_8112;
	pciDev[1]->subSysVenId = 0xDEAD;
	pciDev[2]->venId = PCIB_BRIDGE_VENDOR_ID;
	pciDev[2]->devId = PCIB_BRIDGE_DEVICE_ID_8114;
	pciDev[2]->subSysVenId = 0xDEAD;
	pciDev[3]->venId = PCIB_BRIDGE_VENDOR_ID_2;
	pciDev[3]->devId = PCIB_BRIDGE_DEVICE_ID_8111;
	pciDev[3]->subSysVenId = 0xDEAD;
	pciDev[4]->venId = PCIB_BRIDGE_VENDOR_ID_2;
	pciDev[4]->devId = PCIB_BRIDGE_DEVICE_ID_8112;
	pciDev[4]->subSysVenId = 0xDEAD;
	pciDev[5]->venId = PCIB_BRIDGE_VENDOR_ID_2;
	pciDev[5]->devId = PCIB_BRIDGE_DEVICE_ID_8114;
	pciDev[5]->subSysVenId = 0xDEAD;

	if( (error = FindPciDevice( osHdl, pciDev, allPciDevs, &numDevs, 0)) )
	{
		printf( "\nMake shure EP4 was in HOLD-OFF mode (only EP4-00's red\n"
				"debug LED is on) during Host's (e.g. SC14) start-up.\n"
				"If not check EP4's DIP switches and reboot Host!!!\n" );
		goto end;
	}
	else 
		PrintMsg( h, 1, "OK\n" );

	/* print devices matching venId and devId */
	if( h->dbgLevel >= 2 ) {
		Print_PciDeviceInfo(allPciDevs, numDevs, 0);
	}

	if( instance > (numDevs - 1) ) {
		printf( "ERROR: instance Nr. is too large, found only %d devices\n",
				(int)numDevs);
		error = 1;
		goto end;
	}

	/* store bridge chosen */
	memcpy(pciBridge, allPciDevs[instance], sizeof(PCI_DEVS));
	PrintMsg( h, 2, "\nBridge chosen:\n");
	if( h->dbgLevel >= 2 ) {
		Print_PciDeviceInfo(&pciBridge, 1, 0);
	}

	/* restore bridge (to get access to MPC) */
	/*/* this does not work for windows (registers are unchanged) WHY??? */
	OSS_PciSetConfig( osHdl, pciBridge->bus, pciBridge->dev, pciBridge->fun,
					  PCIB_BRIDGE_COMMAND,
					  OSS_PCI_COMMAND_ENABLE_MEM_SPACE | 
					  OSS_PCI_COMMAND_ENABLE_BUS_MASTER );
	OSS_PciSetConfig( osHdl, pciBridge->bus, pciBridge->dev, pciBridge->fun,
					  PCIB_BRIDGE_PRI_BUS,
					  pciBridge->bus );
	OSS_PciSetConfig( osHdl, pciBridge->bus, pciBridge->dev, pciBridge->fun,
					  PCIB_BRIDGE_SEC_BUS,
					  pciBridge->bus + 1 );
	OSS_PciSetConfig( osHdl, pciBridge->bus, pciBridge->dev, pciBridge->fun,
					  PCIB_BRIDGE_SUB_BUS,
					  pciBridge->bus + 1 );

	/* search for MPC */
    PrintMsg( h, 1, "\nSearch for target (MPC)..." );
	pciDev[0]->venId = PCIB_VENDOR_ID;
	pciDev[0]->devId = PCIB_DEVICE_ID_8540;
	pciDev[0]->subSysVenId = PCIB_SUBVENDOR_ID;
	pciDev[1]->venId = PCIB_VENDOR_ID;
	pciDev[1]->devId = PCIB_DEVICE_ID_8541;
	pciDev[1]->subSysVenId = PCIB_SUBVENDOR_ID;
	pciDev[2]->venId = 0xdead;
	pciDev[2]->devId = 0xdead;
	pciDev[2]->subSysVenId = 0xdead;
	pciDev[3]->venId = 0xdead;
	pciDev[3]->devId = 0xdead;
	pciDev[3]->subSysVenId = 0xdead;
	pciDev[4]->venId = 0xdead;
	pciDev[4]->devId = 0xdead;
	pciDev[4]->subSysVenId = 0xdead;
	pciDev[5]->venId = 0xdead;
	pciDev[5]->devId = 0xdead;
	pciDev[5]->subSysVenId = 0xdead;
	if( (error = FindPciDevice( osHdl, pciDev, allPciDevs, &numDevs, 0)) )
		goto end;
	else 
		PrintMsg( h, 1, "OK\n" );

	/* print devices matching venId and devId */
	if( h->dbgLevel >= 2 ) {
		Print_PciDeviceInfo(allPciDevs, numDevs, 0);
	}

	if( instance > (numDevs - 1) ) {
		printf( "ERROR: instance Nr. is too large, found only %d devices\n",
				(int)numDevs);
		error = 1;
		goto end;
	}

	/* store device chosen */
	memcpy(pciDev[0], allPciDevs[instance], sizeof(PCI_DEVS));
	free( *allPciDevs );
	*allPciDevs = NULL;
	PrintMsg( h, 2, "\nDevice chosen:\n");
	if( h->dbgLevel >= 2 ) {
		Print_PciDeviceInfo(&pciDev[0], 1, 0);
	}

    /*--------------------+
    |  map memory         |
    +--------------------*/
	/* map addresses to user space */
	if( h->physAddrCcsr == 0x0 )
		h->physAddrCcsr = (void*) ( pciDev[0]->bar[PCIB_CCSR_BAR] 
						  & ~(0xF)/*addr is bit 63..4*/ );
	if( h->physAddrDdr == 0x0 )
		h->physAddrDdr = (void*) ( pciDev[0]->bar[PCIB_DDR_AREA_BAR] 
						  & ~(0xF)/*addr is bit 63..4*/ );
	PrintMsg( h, 2, "\nCCSR addr=%p (phys)  DDR addr=%p (phys)\n", 
			  h->physAddrCcsr, h->physAddrDdr );
	
	/* check base addresses */
	if( (u_int32)h->physAddrCcsr==0 || (u_int32)h->physAddrCcsr ==~0 )
	{
		printf( "\nERROR: CCSR addr %p is not valid.\nTry with option -c!\n",
				h->physAddrCcsr );
		error = 1;
		goto end;
	}
	if( (u_int32)h->physAddrDdr==0 || (u_int32)h->physAddrDdr ==~0 )
	{
		printf( "\nERROR: DDR addr %p is not valid.\nTry with option -d!\n",
				h->physAddrCcsr );
		error = 1;
		goto end;
	}
		
	if( (error = OSS_MapPhysToVirtAddr(osHdl,
						 h->physAddrCcsr,
						 PCIB_CCSR_SIZE,
						 OSS_ADDRSPACE_MEM,
						 OSS_BUSTYPE_PCI,
						 h->pciDev[0].bus,
						 (void**)&h->mappedAddrCcsr))
		|| (error = OSS_MapPhysToVirtAddr(osHdl,
						 h->physAddrDdr,
						 PCIB_DDR_AREA_SIZE,
						 OSS_ADDRSPACE_MEM,
						 OSS_BUSTYPE_PCI,
						 h->pciDev[0].bus,
						 (void**)&h->mappedAddrDdr))){
		printf( "ERROR: can't map address (err no: %d)\n", error);
		error = 1;
		goto end;
	}
	PrintMsg( h, 3, "\nCCSR addr=%p (virt)  DDR addr=%p (virt)\n", 
			  h->mappedAddrCcsr, h->mappedAddrDdr );

    /*--------------------+
    |  download binaries  |
    +--------------------*/
	/* copy files */
	do
	{
		PrintMsg( h, 1, "\n========== Press q for quit ==========\n\n" );

		/* EP4 running? */
		PrintMsg( h, 1, "\nCheck PCI Config Header..." );
		if( (CheckConfigHeader( osHdl, h ) == 0) && !firstReset ){
			PrintMsg( h, 1, "OK (running)\n" );
			OSS_Delay( osHdl, PCIB_WAIT_DELAY );
			continue;
		}
		if( firstReset ){
			PrintMsg( h, 1, "OK (valid -> start)\n" );
			firstReset = 0;
			}
		else
			PrintMsg( h, 1, "OK (invalid -> restart)\n" );

		/* restore PCI header */
		PrintMsg( h, 1, "\nRestore PCI Header..." );
		n=PCIB_RETRY_RESTORE_HEADER;
		do{ 
			if( RestoreConfigHeader( osHdl, h ) == 0 )
				break;
			OSS_Delay( osHdl, 100 );
			PrintMsg( h, 3, "%sRETRY (%03d)... ", 
					  (n==PCIB_RETRY_RESTORE_HEADER) ? " " : 
					  "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", n );
		} while(n--);
		if( n<0 )
			PrintMsg( h, 1, "ERROR\n" );			
		else
			PrintMsg( h, 1, "OK\n" );

		/* target in hold-off mode? */
		PrintMsg( h, 1, "\nCheck Hold-off mode..." );
		switch( MREAD_MEM_D8( h->mappedAddrCcsr, 0x1010 ) & 0xFF )
		{
			case 0x00:
				PrintMsg( h, 1, "OK\n" );
				break;
			case 0x01:
				PrintMsg( h, 1, "ERROR (target running)\n" );
				OSS_Delay( osHdl, PCIB_WAIT_DELAY );
				continue;
			default:
				PrintMsg( h, 1, "ERROR (no target connected)\n" );
				OSS_Delay( osHdl, PCIB_WAIT_DELAY );
				continue;
		}

		/* copy bootloader */
		PrintMsg( h, 2, "\nCopy file %s...", h->bootFileName );
		n=PCIB_RETRY_COPY_FILE;
		do{ 
			if( CopyFileToTarget( h, h->bootFileName, h->mappedAddrDdr) != -1 )
				break;
			OSS_Delay( osHdl, 100 );
			PrintMsg( h, 3, " RETRY (%03d)... ", n );
			if( n==0 )
			{
				PrintMsg( h, 2, "ERROR\n" );
				continue;
			}
		} while(n--);
		PrintMsg( h, 2, "OK\n" );
			
		/* copy OS */
		PrintMsg( h, 2, "\nCopy file %s...", h->osFileName );
		if( CopyFileToTarget(h, h->osFileName, h->mappedAddrDdr+PCIB_OS_OFFSET)	!= -1 )
			PrintMsg( h, 2, "OK\n", h->osFileName );

		/* start target */
		PrintMsg( h, 1, "\nStart target..." );
		MWRITE_MEM_D8( h->mappedAddrCcsr, 0x1010, 0x01 );
		/* verify start bit */
		if( MREAD_MEM_D8( h->mappedAddrCcsr, 0x1010 )  == 0x01 )
    		PrintMsg( h, 1, "OK\n" );
		else
    		PrintMsg( h, 1, "Error\n" );
    		
    	if( endPrg )
    		goto end;
    		 
	} while( UOS_KeyPressed() != 'q' );

end:
	if( h ) {
		if( *allPciDevs )
			free( *allPciDevs );
		if( h->mappedAddrCcsr ) 
		{
			OSS_UnMapVirtAddr(osHdl,
							  (void**)&h->mappedAddrCcsr,
							  PCIB_CCSR_SIZE,
							  OSS_ADDRSPACE_MEM);
		}
		if( h->mappedAddrDdr ) 
		{
			OSS_UnMapVirtAddr(osHdl,
							  (void**)&h->mappedAddrDdr,
							  PCIB_DDR_AREA_SIZE,
							  OSS_ADDRSPACE_MEM);
		}

		OSS_MemFree( osHdl, h, devHdlGotSize );
	}

#if defined(WINNT) || defined(LINUX)
	OSS_Exit(&osHdl);
#elif defined(VXWORKS)
	OSS_Exit();
#endif /* WINNT || LINUX */
	return( error );
}


/***************************** ConfigHeaderValid ******************************/
/** Check if EP4 PCI config header is configured
 *
 *  \param argp		\IN		os handle
 *  \param hexval	\IN		device handle
 *
 *  \return	          success (0) or error (-1)
 ******************************************************************************/
static int CheckConfigHeader( OSS_HANDLE *osHdl, DEV_HDL *h )
{
		u_int32 command = 0;
		u_int32 validCommand = OSS_PCI_COMMAND_ENABLE_MEM_SPACE | 
								OSS_PCI_COMMAND_ENABLE_BUS_MASTER;
		u_int32 ccsrBar = 0;
		u_int32 ddrBar = 0;	

		/* read COMMAND */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  OSS_PCI_COMMAND,
						  &command );
							   
		/* read CCSR BAR */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_CCSR_BAR),
						  &ccsrBar );
	    
		/* read DDR AREA BAR */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_DDR_AREA_BAR),
						  &ddrBar );
		
		/* tbd: more registers */

		ccsrBar &= ~(0xF); /* cut status bits */
		ddrBar &= ~(0xF); /* cut status bits */

		PrintMsg( h, 3, "COMMAND=0x%04x CCSR BAR=0x%08x DDR BAR=0x%08x\n", 
				  command, ccsrBar, ddrBar );

		/* check */
		if( (command&0xF) != validCommand || /*ddrBar != 0x0 ||*/ ccsrBar == 0x0 ){
			return -1;
		} 
	return 0;
}


/*************************** RestoreConfigHeader ******************************/
/** Restore EP4 PCI config header
 *
 *  \param argp		\IN		os handle
 *  \param hexval	\IN		device handle
 *
 *  \return	          success (0) or error (-1)
 ******************************************************************************/
static int RestoreConfigHeader( OSS_HANDLE *osHdl, DEV_HDL *h )
{
		u_int32 command = 0;
		u_int32 setCommand = OSS_PCI_COMMAND_ENABLE_MEM_SPACE | 
							 OSS_PCI_COMMAND_ENABLE_BUS_MASTER;
		u_int32 ccsrBar = 0;
		u_int32 ddrBar = 0;

		PrintMsg( h, 4, "\nbus=%d dev=%d fun=%d ", 
				  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun );
		PrintMsg( h, 5, "(CCSR BAR=0x%08x DDR BAR=0x%08x)\n", 
				  h->physAddrCcsr, h->physAddrDdr );

		/*----------------------------------+
		|  restore bridge                   |
		+----------------------------------*/
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_MEM_BASE,
						  (u_int16)(((u_int32)h->physAddrCcsr )>>16));
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_MEM_LIMIT,
						  (u_int16)((((u_int32)h->physAddrCcsr )>>16) + 0xB0) );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_PRE_MEM_BASE,
						  (u_int16)(((u_int32)h->physAddrDdr )>>16) );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_PRE_MEM_LIMIT,
						  (u_int16)((((u_int32)h->physAddrDdr )>>16) + 0x1F0) );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_COMMAND,
						  setCommand );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_PRI_BUS,
						  h->pciBridge.bus );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_SEC_BUS,
						  h->pciBridge.bus + 1 );
		OSS_PciSetConfig( osHdl, 
						  h->pciBridge.bus, h->pciBridge.dev, h->pciBridge.fun,
						  PCIB_BRIDGE_SUB_BUS,
						  h->pciBridge.bus + 1 );

		/*----------------------------------+
		|  restore device                   |
		+----------------------------------*/
		/* restore COMMAND */
		OSS_PciSetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  OSS_PCI_COMMAND,
						  setCommand );
							   
		/* restore CCSR BAR */
		OSS_PciSetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_CCSR_BAR),
						  (u_int32)h->physAddrCcsr );
	    
		/* restore DDR AREA BAR */
		OSS_PciSetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_DDR_AREA_BAR),
						  (u_int32)h->physAddrDdr );

		/*----------------------------------+
		|  verify		                    |
		+----------------------------------*/

		/* verify COMMAND */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  OSS_PCI_COMMAND,
						  &command );
							   
		/* verify CCSR BAR */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_CCSR_BAR),
						  &ccsrBar );
	    
		/* verify DDR AREA BAR */
		OSS_PciGetConfig( osHdl, 
						  h->pciDev[0].bus, h->pciDev[0].dev, h->pciDev[0].fun,
						  (OSS_PCI_ADDR_0+PCIB_DDR_AREA_BAR),
						  &ddrBar );

		ccsrBar &= ~(0xF); /* cut status bits */
		ddrBar &= ~(0xF); /* cut status bits */

		/* check */
		if( command != setCommand || ddrBar != (u_int32) h->physAddrDdr 
			|| ccsrBar != (u_int32) h->physAddrCcsr ){
			PrintMsg( h, 4, "ERROR: COMMAND=0x%04x (!=0x%04x) CCSR BAR=0x%08x"
					  " (!=0x%08x) DDR BAR=0x%08x (!=0x%08x)\n", 
					  command, setCommand, ccsrBar, (u_int32) h->physAddrCcsr,
					  ddrBar, (u_int32) h->physAddrDdr );
			return -1;
		}
	return 0;
}

/**************************** CopyFileToTarget ******************************/
/** copy file to target
 *
 *  \param fName	\IN		name of file to read
 *  \param buf		\OUT	pointer to target memory (virtual address)
 *
 *  \return			file size or error (-1)
 ****************************************************************************/
static int32 CopyFileToTarget(	DEV_HDL* h, char* fName, MACCESS mappedAddr)
{
	unsigned int n;
	size_t fSize=0;
	u_int8* tempBuf=NULL;
	u_int32* tempBuf32;
	FILE *inFp=NULL;
	int error=0;

	/*----------------------------------+
	|  open input file                  |
	+----------------------------------*/
	PrintMsg( h, 3, "read..." );
	inFp = fopen( fName, "rb" );
	if( inFp == NULL ){
		printf("ERROR: Can't open input file\n");
		error = -1;
		goto end;
	}

	/* Determine size of input file */
	fseek( inFp, 0, SEEK_END );
	fSize = ftell( inFp );
	fseek( inFp, 0, SEEK_SET );

	/* read in file */
	if( (tempBuf = calloc( fSize+3/*round up*/, 1 )) == NULL ){
		printf("ERROR: can't allocate buffer\n");
		error = -1;
		goto end;
	}

	if( fread( tempBuf, 1, fSize, inFp ) != (size_t)fSize ){
		printf("ERROR: reading file (%s)\n", fName );
		error = -1;
		goto end;
	}

	/*----------------------------------+
	|  copy file                        |
	+----------------------------------*/
	PrintMsg( h, 3, "copy..." );
	tempBuf32 = (u_int32*) tempBuf;
	for( n=0; n<fSize; n+=4 )
	{
		MWRITE_MEM_D32(mappedAddr, n, *(tempBuf32++));
	}

	/* Verify */
	PrintMsg( h, 3, "verify..." );
	tempBuf32 = (u_int32*) tempBuf;
	for( n=0; n<fSize; n+=4 )
	{
		if( MREAD_MEM_D32(mappedAddr, n ) != *(tempBuf32++) )
		{
			printf("ERROR: Verify at offset 0x%08X\n", n );
			error = -1;
			goto end;
		}
	}	

end:
	if( tempBuf != NULL )
		free(tempBuf);
	if( inFp != NULL )
		fclose(inFp);
	return(error);
}


/***************************** Print_PciDeviceInfo ****************************/
/** Print information of PCI devices
 *
 * \param pciDevs	\IN pointer to PCI_DEVS handle array
 * \param numDevs	\IN number of devices to print info for
 * \param bars		\IN flag print BAR infos
 *
 * \return void
 ******************************************************************************/
static void Print_PciDeviceInfo( PCI_DEVS **pciDevs,
								 u_int32 numDevs,
								 u_int8 bars )
{
	u_int32 n;
	if( !bars ) {
		printf( "\nNr.|bus|dev|fun| Ven ID | Dev ID | SubVen ID |\n");
		for(n = 0; n < numDevs; n++) {
			printf( "%3ld %3d %3d %3d  0x%04x   0x%04x    0x%04x\n",
					n, pciDevs[n]->bus, pciDevs[n]->dev, pciDevs[n]->fun,
					(unsigned int)pciDevs[n]->venId,
					(unsigned int)pciDevs[n]->devId,
					(unsigned int)pciDevs[n]->subSysVenId);
		}
	} else {
		printf( "\nNr.|bus|dev|fun|"
				"   BAR0  |   BAR1  |   BAR2  |"
				"   BAR3  |   BAR4  |   BAR5\n");
		for(n = 0; n < numDevs; n++)
			printf( "%3ld %3d %3d %3d  %08x  %08x  %08x "
					" %08x  %08x  %08x\n",
					n, pciDevs[n]->bus, pciDevs[n]->dev, pciDevs[n]->fun,
					(unsigned int)pciDevs[n]->bar[0],
					(unsigned int)pciDevs[n]->bar[1],
					(unsigned int)pciDevs[n]->bar[2],
					(unsigned int)pciDevs[n]->bar[3],
					(unsigned int)pciDevs[n]->bar[4],
					(unsigned int)pciDevs[n]->bar[5]);
	}
	return;
}

/**************************** FindPciDevice ***************************/
/** find PCI device specified by vendor and device ID and get parameters
 *
 *  \param osHdl        \IN     OSS_HANDLE
 *  \param dev			\IN		devices to search for (terminated by NULL)
 *  \param allPciDevs	\OUT	structure to fill with found devices
 *  \param numdevs		\OUT	number of PCI devices found
 *  \param show_all		\IN		return all PCI devices in system
 *
 *  \return success (0) or error (-1)
 *          or read value
 ******************************************************************************/
static int32 FindPciDevice( OSS_HANDLE *osHdl,
							PCI_DEVS* dev[],
							PCI_DEVS* allPciDevs[],
							u_int32 *numDevs,
							int show_all )
{
	int i;
	u_int32 headerType = 0;
	int32 val1, val2;
	PCI_DEVS *pCurdev;
	u_int32 bus, slot, function, barn;
	u_int32 slotIndex = 0; /* current slot */
	*numDevs = 0;

	pCurdev = allPciDevs[0];

    /* for each bus */
    for(bus = 0; bus < 0x100; bus++) {
        /* for slot */
        for(slot = 0; slot < 32; slot++) {
            /* for each function */
            for(function = 0; function < 8; function++) {
                /* get the initial bus data */
				OSS_PciGetConfig( osHdl, bus, slot, function,
								  OSS_PCI_VENDOR_ID, &val1 );
				/* check if a card is installed */
				if( bus == 3 && slot == 0 && function == 0 )
					/* restore COMMAND */
					OSS_PciSetConfig( osHdl, 
									  bus, slot, function,
									  OSS_PCI_COMMAND,
									  OSS_PCI_COMMAND_ENABLE_MEM_SPACE | 
									  OSS_PCI_COMMAND_ENABLE_BUS_MASTER );

				/* if PCI bus is not implemented, 0 is returned instead of -1 */
                if ( (val1 == 0xFFFF) || (val1 == 0x0000) ) {
                    if (function == 0) {
                        /* nothing in slot, go to next */
                        break;
                    } else {
                        /* function 0 is required on all cards,
                         * if this is a multifunction card, other
                         * functions do not need to be contiguous,
                         * search for additional functions */
                        continue;
                    }
                } else {
					/* get parameters */
 					pCurdev = allPciDevs[slotIndex];
					pCurdev->bus = (u_int8)bus;
					pCurdev->dev = (u_int8)slot;
					pCurdev->fun = (u_int8)function;

					pCurdev->venId = val1;
					OSS_PciGetConfig( osHdl,
									  bus, slot, function,
									  OSS_PCI_DEVICE_ID,
									  &pCurdev->devId);
					OSS_PciGetConfig( osHdl,
									  bus, slot, function,
									  OSS_PCI_SUBSYS_VENDOR_ID,
									  &pCurdev->subSysVenId);
					OSS_PciGetConfig( osHdl,
									  bus, slot, function,
									  OSS_PCI_COMMAND,
									  (u_int32*)&pCurdev->comReg);
					OSS_PciGetConfig( osHdl,
									  bus, slot, function,
									  OSS_PCI_HEADER_TYPE,
									  &headerType);

					if( !(headerType & 0x7f) ) /* some bridge? */
						for (barn = 0; barn < 6; barn++)
							OSS_PciGetConfig( osHdl,
											  bus, slot, function,
											  OSS_PCI_ADDR_0+barn,
											  &pCurdev->bar[barn]);

					/* check for match and continue to next */
					if( !show_all )
					{	
						u_int8 found=0;
						
						for( i=0; dev[i]!=NULL && !found; i++ )
						{
							found |= (pCurdev->venId == dev[i]->venId) &&
									 (pCurdev->devId == dev[i]->devId) &&
									 ((!(headerType & 0x7f)) ? 
									 /* normal device: */
									 (pCurdev->subSysVenId == dev[i]->subSysVenId)
									 /* bridge: */
									 : 1);
						}
						if(!found)
							continue;
							
						(*numDevs)++;

						if( *numDevs == PCIB_MAX_DEVICES )
							goto end;

						slotIndex++;

						/* function 0 is required on all cards,
						 * if this is a multifunction card, other
						 * functions do not need to be contiguous,
						 * search for additional functions */
						if (function == 0) {
							OSS_PciGetConfig( osHdl,
											  bus, slot, 0,
											  OSS_PCI_HEADER_TYPE,
											  &val2);
							/* look in Header Type if we have a multifunction card */
							if(!(val2 & 0x80)) {
								/* not a multi-function card */
								break;
							}
						}
					}  else if ( show_all ) {
						(*numDevs)++;

						if( *numDevs == PCIB_MAX_DEVICES )
							goto end;

						slotIndex++;
					}

				}
            }
        }
    }

end:
	if( !(*numDevs) ){
		printf( "ERROR\n\n" );
		for( i=0; dev[i]!=NULL ; i++ ){
			printf( "No PCI devices matching vendor ID           0x%04lx,\n"
					"                               device ID    0x%04lx and\n"
					"                               subVendor ID 0x%04lx found!\n",
					dev[i]->venId, dev[i]->devId, dev[i]->subSysVenId);
		}
		goto error_end;
	}

	return 0;
error_end:
	return( -1 );
}

