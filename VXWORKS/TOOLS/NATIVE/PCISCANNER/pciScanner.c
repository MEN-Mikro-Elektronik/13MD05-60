/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: pciScanner.c
 *      Project: -
 *
 *       Author: rl
 *        $Date: 2009/04/01 15:10:43 $
 *    $Revision: 1.5 $
 *
 *  Description: This is a utility for scanning the PCI bus for devices
 *               and printout their PCI path.
 *
 *
 *        Note:  
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: pciScanner.c,v $
 * Revision 1.5  2009/04/01 15:10:43  ufranke
 * cosmetics
 *
 * Revision 1.4  2008/09/15 15:04:29  ufranke
 * R: diab compiler warnings
 * M: cosmetics
 *
 * Revision 1.3  2004/07/23 15:22:54  ufranke
 * changed
 *  - output now bus#/dev# -> path
 *
 * Revision 1.2  2000/03/22 15:56:14  loesel
 * some cosmetics
 *
 * Revision 1.1  2000/03/22 15:40:40  loesel
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#include <MEN/men_typs.h>

#include "vxWorks.h"		/* always first, CPU type, family , big/litte endian, etc. */
#include "stdio.h"
#include "stdlib.h"

#include "drv/pci/pciConfigLib.h"	/* for PCI const */
#include "drv/pci/pciIntLib.h"		/* for PCI interrupt */




/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#undef DBGPCI

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/




/******************************** sysPciFindBridge ***************************
 *
 *  Description: Find the pci bridge with the given bus numbers
 *
 *----------------------------------------------------------------------------
 *  Input......:  inBus         bus to find
 *
 *  Output.....:  outBus        found primary bus
 *                outDev        found bridge device number
 *                Return 		OK | ERROR
 *
 *  Globals....:  
 *****************************************************************************/
LOCAL STATUS sysPciScanBridge(int maxBus, UINT8 *outBus, int *outDev)
{
	int  		pciFunc;
    
    int			pciBus;
    int			pciDevice;
	UINT8		tmpBus;
	UINT8		tmpBus2;
	UINT8		tmpHelp;
    int    		vendor;
	


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



		pciConfigInLong (pciBus, pciDevice, pciFunc, PCI_CFG_VENDOR_ID,
				 (UINT32*)&vendor);

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
			
			pciConfigInByte (	pciBus,
								pciDevice,
								pciFunc,
								PCI_CFG_HEADER_TYPE,
								&tmpHelp);

			if ((tmpHelp & PCI_HEADER_TYPE_MASK) == PCI_HEADER_PCI_PCI)
			{	
				pciConfigInByte (	pciBus,
									pciDevice,
									pciFunc,
									PCI_CFG_SECONDARY_BUS,
									&tmpBus);
				#ifdef DBGPCI
					fprintf (stderr, "PCI Bridge found at 0x%x, 0x%x, secBus: %d\n", pciBus, pciDevice, tmpBus);
				#endif					
				if (tmpBus == maxBus)
				{
					pciConfigInByte (	pciBus,
										pciDevice,
										pciFunc,
										PCI_CFG_PRIMARY_BUS,
										&tmpBus2);
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

/******************************** sysPciScanner ******************************
 *
 *  Description: Scans the PCI bus and prints out all devices
 *
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  
 *****************************************************************************/
STATUS sysPciScan(void)
{
    int			busNo;
    int			deviceNo;
    int			funcNo;
    u_int16		device;
    u_int16		vendor;
	int			inBus;
	UINT8		outBus;
	int			outDev;
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

		pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_VENDOR_ID,
				 &vendor);
		pciConfigInWord (busNo, deviceNo, funcNo, PCI_CFG_DEVICE_ID,
				 &device);

		
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
			
			pciConfigInByte (	busNo,
								deviceNo,
								funcNo,
								PCI_CFG_HEADER_TYPE,
								&tmpHelp);

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
						if (ERROR == sysPciScanBridge (	inBus, &outBus,	&outDev))
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






