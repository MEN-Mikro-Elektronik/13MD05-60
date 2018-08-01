/*********************  P r o g r a m  -  M o d u l e *************************/
/*!
 *         \file vxworks.c
 *      \project 13z100-91
 *
 *       \author Christian.Schuster@men.de
 *        $Date: 2005/01/31 14:10:06 $
 *    $Revision: 1.2 $
 *
 *        \brief VxWorks specific functions\n
 *
 *
 *     Required: -
 *     Switches: Z100_IO_MAPPED_EN:	switch must not be set for PPC compilers
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
 * $Log: vxworks.c,v $
 * Revision 1.2  2005/01/31 14:10:06  CSchuster
 * added include file sysLib.h
 * IO-mapped support now only for x86 CPUs enabled
 *
 * Revision 1.1  2004/12/23 16:31:02  CSchuster
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ******************************************************************************/
#if 0
#include <MEN/men_typs.h>

#ifdef VXWORKS

#include "vxWorks.h"		/* always first, CPU type, family , big/litte endian, etc. */
#include "../COM/fpga_load.h"
#include "fpga_load_os.h"
#include <unistd.h>

#include "drv/pci/PciConfigLib.h"	/* for PCI const */
#include "drv/pci/PciConfigShow.h"	/* for PCI show headers */
#include "drv/pci/pciIntLib.h"		/* for PCI interrupt */

#ifdef Z100_IO_MAPPED_EN
#include <sysLib.h>
#endif


/* TYPEDEFS */

/* GLOBALS */

/* PROTOTYPES */

/******************************* Z100_Os_init *********************************/
/** perform OS specific initialization
 *
 *  \param h		\OUT		pointer to DEV_HDL handle
 *
 ******************************************************************************/
extern void Z100_Os_init(DEV_HDL **h)
{
	u_int32 i;
	u_int8 *tempBuf;
	printf("\n-> %s\n", __FUNCTION__);
	if( !(*h = (DEV_HDL *)malloc(sizeof(DEV_HDL))) ) {
		printf("\nERROR allocating memory for device handle\n");
		Z100_Os_exit(h);
		return;
	}
	tempBuf = (u_int8*)*h;
	for( i=0; i<sizeof(DEV_HDL); i++)
		*tempBuf++ = (u_int8)0;

	(*h)->flashDev.devHdl = *h;
}

/******************************* Z100_Os_exit *********************************/
/** perform OS specific cleanup
 *
 *  \param h		\IN		pointer to DEV_HDL handle
 *
 ******************************************************************************/
extern void Z100_Os_exit(DEV_HDL **h)
{
	/* free device handle */
	if(*h)
		free(*h);
	*h = NULL;
}


/***************************** Z100_Os_access_address *************************/
/** read or write memory
 *
 *  \param bar		\IN		PCI BAR
 *  \param offs		\IN		offset from BAR
 *  \param size		\IN		1/2/4 size of access in bytes
 *  \param read		\IN		1 = read, 0 = write access
 *  \param value	\IN		value to be written to memory
 *  \param be_flag	\OUT	is set if buserror occurred (not yet implemented)
 *
 *  \return nothing on writes
 *          or read value
 ******************************************************************************/
extern u_int32 Z100_Os_access_address( u_int32 bar,
										u_int32 offs,
										int size,
										int read,
										u_int32 value,
										int *be_flag )
{

#ifdef Z100_IO_MAPPED_EN
#  if (CPU_FAMILY == I80X86)
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
				case 1: 	value = sysInByte( (int)((bar&~0xFF)+offs) ); break;
				case 2: 	value = sysInWord( (int)((bar&~0xFF)+offs) ); break;
				case 4: 	value = sysInLong( (int)((bar&~0xFF)+offs) ); break;
			}
			DBGOUT( "Z100_Os_access_address IO: read value: 0x%08x\n", value );
		} else {
			switch(size){
				case 1: 	sysOutByte( (int)((bar&~0xFF)+offs), value ); break;
				case 2: 	sysOutWord( (int)((bar&~0xFF)+offs), value ); break;
				case 4: 	sysOutLong( (int)((bar&~0xFF)+offs), value ); break;
			}
		}
	} else
#  endif
#endif
	{
		/* BAR is memory mapped */
		u_int32 adr = (bar&~0xFF)+offs;

		DBGOUT(("Z100_Os_access_address mem: 0x%x  size:%d  read:%d value: 0x%08x\n", adr, size, read, value ));

		/*--------------+
		| access memory |
		+--------------*/
		if( read ){
			switch(size){
				case 1: 	value = *(volatile u_int8* )adr; break;
				case 2: 	value = *(volatile u_int16*)adr; break;
				case 4: 	value = *(volatile u_int32*)adr; break;
			}
			DBGOUT(("Z100_Os_access_address mem: read value: 0x%08x\n", value ));
		} else {
			switch(size){
				case 1: 	*(volatile u_int8*) adr = value; break;
				case 2: 	*(volatile u_int16*)adr = value; break;
				case 4: 	*(volatile u_int32*)adr = value; break;
			}
		}

	}

/*	*be_flag = 0; */
    return value;
}

/******************************** Z100PciFindBridge ***************************/
/** Find the pci bridge with the given bus numbers
 *
 *  \param maxBus		\IN		bus to find
 *  \param outBus		\OUT	found primary bus
 *  \param outDev		\OUT	found bridge device number
 *
 *  \return OK | ERROR
 ******************************************************************************/
LOCAL STATUS Z100PciScanBridge(int maxBus, UINT8 *outBus, int *outDev)
{

	int  		pciFunc;
    int			pciBus;
    int			pciDevice;
	UINT8		tmpBus;
	UINT8		tmpBus2;
	UINT8		tmpHelp;
   int    		vendor;
   printf("\n-> %s\n", __FUNCTION__);
    /*------------------------------------------+
    | search for PCI Bridges and their busses   |
    +------------------------------------------*/

   for (pciBus = 0; pciBus < maxBus; pciBus++)				/* optimized */
	   for (pciDevice = 0; pciDevice < 0x1f; pciDevice++)	/* optimized */
		   for (pciFunc = 0; pciFunc < 1; pciFunc++)		/* optimized, no multifunction needed */
    {

		/*---------------------------+
		| avoid a special bus cycle  |
		+---------------------------*/

		if ((pciDevice == 0x1f) && (pciFunc == 0x07))
		    continue;

		OSS_PciGetConfig (OSS_VXWORKS_OS_HDL,
			pciBus, pciDevice, pciFunc, PCI_CFG_VENDOR_ID,
			(int32*)&vendor);

		/*---------------------------------------------------+
		| only look at vendor ID field for existence check   |
		| this field must exsist for every PCI device        |
		| if 0xFFFF is returned, go to next device           |
		+----------------------------------------------------*/

		if (((vendor & 0x0000ffff) != 0x0000FFFF))
		{
			/*------------------------------+
			| read out header type          |
			| skip if not PCI_PCI bridge    |
			+-------------------------------*/
			OSS_PciGetConfig (OSS_VXWORKS_OS_HDL,
				pciBus,
				pciDevice,
				pciFunc,
				PCI_CFG_HEADER_TYPE,
				(int32*)&tmpHelp);

			if ((tmpHelp & PCI_HEADER_TYPE_MASK) == PCI_HEADER_PCI_PCI)
			{
				OSS_PciGetConfig (OSS_VXWORKS_OS_HDL,
					pciBus,
					pciDevice,
					pciFunc,
					PCI_CFG_SECONDARY_BUS,
					(int32*)&tmpBus);
				#ifdef DBGPCI
					fprintf(stderr, "PCI Bridge found at 0x%x, 0x%x, secBus: %d\n",
									pciBus, pciDevice, tmpBus);
				#endif
				if (tmpBus == maxBus)
				{
					OSS_PciGetConfig (OSS_VXWORKS_OS_HDL,
						pciBus,
						pciDevice,
						pciFunc,
						PCI_CFG_PRIMARY_BUS,
						(int32*)&tmpBus2);
					*outBus = tmpBus2;
					*outDev = pciDevice;
					#ifdef DBGPCI
						printf ("primaryBus: %d\n", tmpBus2);
					#endif
					return OK;
				}

			}


		}

	}
	return ERROR;
}

/******************************** Z100PciScan ********************************/
/** Scans the PCI bus and prints out all devices
 *
 *  \return OK | ERROR
 *****************************************************************************/
LOCAL STATUS Z100PciScan(void)
{
    /* STATUS		status = ERROR; */
    int			busNo;
    int			deviceNo;
    int			funcNo;
    u_int16		device;
    u_int16		vendor;
    /* char		header; */
	int			inBus;
	UINT8		outBus;
	int			outDev;
	/* int			inDev; */
	int			pathCount = 0;
	int			devicePath[16];
	int 		i;
	UINT8		tmpHelp;

/*    if (pciLibInitStatus != OK) */			/* sanity check */
/*        return (ERROR); */

	printf ("\nPCI BUS SCANNER\n");
	printf ("VENDOR  DEVICE  BusNo/DeviceId(DevNo) -> Path\n");

    for (busNo=0; busNo < 0x40; busNo++)				/*optimized*/
        for (deviceNo=0; deviceNo < 0x1f; deviceNo++)
            for (funcNo=0; funcNo < 1; funcNo++)		/*optimized*/
	{
		/* avoid a special bus cycle */

		if ((deviceNo == 0x1f) && (funcNo == 0x07))
		    continue;

		OSS_PciGetConfig(OSS_VXWORKS_OS_HDL,
				busNo, deviceNo, funcNo, PCI_CFG_VENDOR_ID,
				(int32*)&vendor);

		OSS_PciGetConfig (OSS_VXWORKS_OS_HDL,
				busNo, deviceNo, funcNo, PCI_CFG_DEVICE_ID,
				(int32*)&device);


		/*---------------------------------------------------+
		| only look at vendor ID field for existence check   |
		| this field must exsist for every PCI device        |
		| if 0xFFFF is returned, go to next device           |
		+----------------------------------------------------*/

		if (((vendor & 0x0000ffff) != 0x0000FFFF))
		{

			/*------------------------------+
			| read out header type          |
			| skip if PCI_PCI bridge        |
			+-------------------------------*/

			OSS_PciGetConfig (	OSS_VXWORKS_OS_HDL,
				busNo,
				deviceNo,
				funcNo,
				PCI_CFG_HEADER_TYPE,
				(int32*)&tmpHelp);

			if ((tmpHelp & PCI_HEADER_TYPE_MASK) != PCI_HEADER_PCI_PCI)
			{

				if (busNo == 0)
				{
					devicePath[0] = deviceNo;
					pathCount = 1;
				}
				else
				{
					devicePath[0] = deviceNo;
					pathCount = 1;
					inBus = busNo;
					do
					{
						if (ERROR == Z100PciScanBridge( inBus,
														&outBus,
														&outDev))
							return ERROR;

						devicePath[pathCount] = outDev;
						pathCount++;
						inBus = outBus;
					}while ((outBus != 0) | (pathCount>16));
				}

				/* device path */
				printf ("0x%04x  0x%04x: ", vendor, device);
				fprintf(stderr, " 0x%02x / 0x%02x -> ", busNo, devicePath[0] );

				for (i=pathCount-1; i>0; i--)
					fprintf(stderr, "0x%02x  ", devicePath[i]);

				printf ("\n");

				/* reset counter for next unit */
				pathCount = 0;
			}
		}

	}

    return OK;
}
#endif /* VXWORKS */
#endif
