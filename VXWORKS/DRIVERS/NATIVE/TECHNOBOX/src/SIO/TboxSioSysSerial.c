/* TboxSioSysSerial.c - Driver installation routines for Technobox Serial Port Adapters */

/******************************************************************************
 *
 * Copyright 2007 by Technobox
 *
 * Functions: This file contains the functions and data that initialize the Drivers.
 *
 * Revision:
 * REV   WHO   DATE       SWF#   COMMENTS
 * ----+-----+----------+------+-----------------------------------------------
 * 006   SGL   12/03/07   037    Change intConnect to pciIntConnect
 * 005   SGL   06/15/07   026    Separated BAR and Port programming
 *                               Update Part Numbers to Lead Free Ones
 * 004   SGL   04/12/06   030    Moved PCI and CPU BSP addresses to config file
 *                               Removed 4382 support.
 * 003   SGL   06/20/05   025    Updated constants
 * 002   SGL   03/24/04          Added Serial ATA Card Serial Ports.
 * 001   SGL   03/11/04          Split sysSerial functions seperately
 *                               Updated to support two of each Adapter
 * 000   SGL   03/10/04          Initial Release
 *
 *****************************************************************************/

/*
DESCRIPTION
This file contains routines for installation of the Technobox Serial Port
Adapters.  These routines setup the Channel Data Structure, initialize the data,
call the driver initialization file, and enable the interrupts.
The user needs to update the appropriate definitions for their
architecture in the TboxSioSupport.h file.

These routines support up to 2 Adapters of each type without code modifications.  
Additional adapters require the user to modify the installation code.
*/

#include "drv\sio\ns16552Sio.h"
#include "..\..\h\SIO\TboxSioSupport.h"

/* Setup and Initialize Local data structures */

#ifdef INCLUDE_TBOX_4960
	static NS16550_CHAN			TboxSio4960Channels[TBOX_4960_NUM_CARDS][TBOX_4960_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_4960A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio4960Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio4960Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio4960Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio4960Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_4960_NUM_CARDS > 1)
		#define TBOX_4960B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio4960Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio4960Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio4960Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio4960Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_4960B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_4960_NUM_CARDS > 1) */
#else
	#define TBOX_4960A_SIO_CHAN_ENTRIES
	#define TBOX_4960B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 4960 */

#ifdef INCLUDE_TBOX_5284
	static NS16550_CHAN	TboxSio5284Channels[TBOX_5284_NUM_CARDS][8];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_5284A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio5284Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][4].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][5].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][6].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5284Channels[0][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_5284_NUM_CARDS > 1)
		#define TBOX_5284B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio5284Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][4].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][5].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][6].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5284Channels[1][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_5284B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_5284_NUM_CARDS > 1) */
#else
	#define TBOX_5284A_SIO_CHAN_ENTRIES
	#define TBOX_5284B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 5284 */

#ifdef INCLUDE_TBOX_5436
	static NS16550_CHAN			TboxSio5436Channels[TBOX_5436_NUM_CARDS][TBOX_5436_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_5436A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio5436Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][4].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][5].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][6].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][7].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][8].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][9].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][10].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][11].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][12].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][13].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][14].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5436Channels[0][15].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_5436_NUM_CARDS > 1)
		#define TBOX_5436B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio5436Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][4].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][5].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][6].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][7].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][8].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][9].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][10].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][11].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][12].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][13].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][14].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5436Channels[1][15].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_5436B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_5436_NUM_CARDS > 1) */
#else
	#define TBOX_5436A_SIO_CHAN_ENTRIES
	#define TBOX_5436B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 5436 */

#ifdef INCLUDE_TBOX_5288
	static NS16550_CHAN			TboxSio5288Channels[TBOX_5288_NUM_CARDS][TBOX_5288_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_5288A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio5288Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][4].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][5].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][6].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5288Channels[0][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_5288_NUM_CARDS > 1)
		#define TBOX_5288B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio5288Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][4].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][5].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][6].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5288Channels[1][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_5288B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_5288_NUM_CARDS > 1) */
#else
	#define TBOX_5288A_SIO_CHAN_ENTRIES
	#define TBOX_5288B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 5288 */

#ifdef INCLUDE_TBOX_4966
	static NS16550_CHAN  TboxSio4966Channels[TBOX_4966_NUM_CARDS][TBOX_4966_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_4966A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio4966Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio4966Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_4966_NUM_CARDS > 1)
		#define TBOX_4966B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio4966Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio4966Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_4966B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_4966_NUM_CARDS > 1) */
#else
	#define TBOX_4966A_SIO_CHAN_ENTRIES
	#define TBOX_4966B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 4966 */

#ifdef INCLUDE_TBOX_5382
	static NS16550_CHAN			TboxSio5382Channels[TBOX_5382_NUM_CARDS][TBOX_5382_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_5382A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio5382Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][4].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][5].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][6].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio5382Channels[0][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_5382_NUM_CARDS > 1)
		#define TBOX_5382B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio5382Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][4].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][5].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][6].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio5382Channels[1][7].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_5382B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_5382_NUM_CARDS > 1) */
#else
	#define TBOX_5382A_SIO_CHAN_ENTRIES
	#define TBOX_5382B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 5382 */

#ifdef INCLUDE_TBOX_3607
	static NS16550_CHAN  TboxSio3607Channels[TBOX_3607_NUM_CARDS][TBOX_3607_UART_CHANNELS];

	/* Define the entries in the sysSioChans[] definition in sysSerial
	 * Place the definition inside the structure to get these entries added into the table
	 */
	#define TBOX_3607A_SIO_CHAN_ENTRIES \
	    (SIO_CHAN *) &TboxSio3607Channels[0][0].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][1].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][2].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][3].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][4].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][5].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][6].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][7].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][8].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][9].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][10].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][11].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][12].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][13].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][14].pDrvFuncs, /* /tyCo/<next #> */ \
	    (SIO_CHAN *) &TboxSio3607Channels[0][15].pDrvFuncs, /* /tyCo/<next #> */ 
	#if (TBOX_3607_NUM_CARDS > 1)
		#define TBOX_3607B_SIO_CHAN_ENTRIES \
			(SIO_CHAN *) &TboxSio3607Channels[1][0].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][1].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][2].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][3].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][4].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][5].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][6].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][7].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][8].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][9].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][10].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][11].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][12].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][13].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][14].pDrvFuncs, /* /tyCo/<next #> */ \
			(SIO_CHAN *) &TboxSio3607Channels[1][15].pDrvFuncs, /* /tyCo/<next #> */ 
	#else
		#define TBOX_3607B_SIO_CHAN_ENTRIES
	#endif /* (TBOX_3607_NUM_CARDS > 1) */
#else
	#define TBOX_3607A_SIO_CHAN_ENTRIES
	#define TBOX_3607B_SIO_CHAN_ENTRIES
#endif /* INCLUDE TBOX 3607 */


#define REG(reg, pchan) \
 (*(volatile UINT8 *)((UINT32)pchan->regs + (reg * pchan->regDelta)))
#define REGPTR(reg, pchan) \
 ((volatile UINT8 *)((UINT32)pchan->regs + (reg * pchan->regDelta)))


/******************************************************************************
*
* TboxSioSerialHwInit - initialize the Technobox serial devices to a quiescent state
*
* This routine initializes the Technobox serial device descriptors and puts the
* devices in a quiescent state.  It should be called from sysSerialHwInit().
* Interrupt mode operations are enabled by TboxSioSerialHwInit2().
*
* <LevelOverride> allows the user to override the value the BSP setup.  Pass 0
* to keep the BSP value.  Most BSP's setup the value correctly.
*
* NOTES: 
* This routine does not enable interrupts.
*
* SEE ALSO: sysHwInit(), sysSerialHwInit2(), sysSerialHwInit2(), TboxSioSerialHwInit2()
*
* RETURNS: N/A
*/
void TboxSioSerialInit (UINT8 LevelOverride)
{
	NS16550_CHAN	*pChan;
	int				CardLoop;
	int				PortLoop;
    int				Bus = 0;       /* PCI bus number */
    int				Dev = 0;       /* PCI device number */
    int				Func = 0;      /* PCI function number */
	UINT32			Bar2;
	UINT32			Bar3;
	UINT32			Bar4;			/* Unused for some Adapters */
	UINT32			Bar5;			/* Unused for some Adapters */
	UINT8			IntLevel;

	/* Initialize the values in the Channel Structure 
	 * and then call the 16550 drv initialization routine.  
	 */

#ifdef INCLUDE_TBOX_4960
	for (CardLoop = 0; CardLoop < TBOX_4960_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_4960 & 0xFFFF), (PCI_ID_TBOX_4960 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInLong_M (Bus, Dev, Func, 0x20, &Bar4 );
			PciConfigInLong_M (Bus, Dev, Func, 0x24, &Bar5 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			Bar4 = PCI_IO_TO_CPU_ADDR(Bar4 & 0xFFFFFFFE);
			Bar5 = PCI_IO_TO_CPU_ADDR(Bar5 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_4960_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio4960Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs = (UINT8 *) (Bar2 + 0x00); break;  /* 1st Port on BAR 2 */
				case 1: pChan->regs = (UINT8 *) (Bar3 + 0x00); break;  /* 1st Port on BAR 3 */
				case 2: pChan->regs = (UINT8 *) (Bar4 + 0x00); break;  /* 1st Port on BAR 4 */
				case 3: pChan->regs = (UINT8 *) (Bar5 + 0x00); break;  /* 1st Port on BAR 5 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 

			/* Driver Initialization routine */
			ns16550DevInit(pChan);
		}
	}
#endif /* INCLUDE TBOX 4960 */

#ifdef INCLUDE_TBOX_5284
	for (CardLoop = 0; CardLoop < TBOX_5284_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_5284 & 0xFFFF), (PCI_ID_TBOX_5284 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < 8; PortLoop++)
		{
			pChan = &TboxSio5284Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs = (UINT8 *) (Bar2 + 0x00); break;  /* 1st Port on BAR 2 */
				case 1: pChan->regs = (UINT8 *) (Bar2 + 0x08); break;  /* 2nd Port on BAR 2 */
				case 2: pChan->regs = (UINT8 *) (Bar2 + 0x10); break;  /* 3rd Port on BAR 2 */
				case 3: pChan->regs = (UINT8 *) (Bar2 + 0x18); break;  /* 4th Port on BAR 2 */
				case 4: pChan->regs = (UINT8 *) (Bar3 + 0x00); break;  /* 1st Port on BAR 3 */
				case 5: pChan->regs = (UINT8 *) (Bar3 + 0x08); break;  /* 2nd Port on BAR 3 */
				case 6: pChan->regs = (UINT8 *) (Bar3 + 0x10); break;  /* 3rd Port on BAR 3 */
				case 7: pChan->regs = (UINT8 *) (Bar3 + 0x18); break;  /* 4th Port on BAR 3 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 			
			/* Driver Initialization routine */			
			ns16550DevInit(pChan);			
		}
	}
#endif /* INCLUDE TBOX 5284 */

#ifdef INCLUDE_TBOX_5436
	for (CardLoop = 0; CardLoop < TBOX_5436_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_5436 & 0xFFFF), (PCI_ID_TBOX_5436 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInLong_M (Bus, Dev, Func, 0x20, &Bar4 );
			PciConfigInLong_M (Bus, Dev, Func, 0x24, &Bar5 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			Bar4 = PCI_IO_TO_CPU_ADDR(Bar4 & 0xFFFFFFFE);
			Bar5 = PCI_IO_TO_CPU_ADDR(Bar5 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_5436_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5436Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs =  (UINT8 *) (Bar2 + 0x00); break; /* 1st Port on BAR 2 */
				case 1: pChan->regs =  (UINT8 *) (Bar2 + 0x08); break; /* 2nd Port on BAR 2 */
				case 2: pChan->regs =  (UINT8 *) (Bar2 + 0x10); break; /* 3rd Port on BAR 2 */
				case 3: pChan->regs =  (UINT8 *) (Bar2 + 0x18); break; /* 4th Port on BAR 2 */
				case 4: pChan->regs =  (UINT8 *) (Bar3 + 0x00); break; /* 1st Port on BAR 3 */
				case 5: pChan->regs =  (UINT8 *) (Bar3 + 0x08); break; /* 2nd Port on BAR 3 */
				case 6: pChan->regs =  (UINT8 *) (Bar3 + 0x10); break; /* 3rd Port on BAR 3 */
				case 7: pChan->regs =  (UINT8 *) (Bar3 + 0x18); break; /* 4th Port on BAR 3 */
				case 8: pChan->regs =  (UINT8 *) (Bar4 + 0x00); break; /* 1st Port on BAR 4 */
				case 9: pChan->regs =  (UINT8 *) (Bar4 + 0x08); break; /* 2nd Port on BAR 4 */
				case 10: pChan->regs = (UINT8 *) (Bar4 + 0x10); break; /* 3rd Port on BAR 4 */
				case 11: pChan->regs = (UINT8 *) (Bar4 + 0x18); break; /* 4th Port on BAR 4 */
				case 12: pChan->regs = (UINT8 *) (Bar5 + 0x00); break; /* 1st Port on BAR 5 */
				case 13: pChan->regs = (UINT8 *) (Bar5 + 0x08); break; /* 2nd Port on BAR 5 */
				case 14: pChan->regs = (UINT8 *) (Bar5 + 0x10); break; /* 3rd Port on BAR 5 */
				case 15: pChan->regs = (UINT8 *) (Bar5 + 0x18); break; /* 4th Port on BAR 5 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 			
			/* Driver Initialization routine */
			ns16550DevInit(pChan);			
		}
	}
#endif /* INCLUDE TBOX 5436 */

#ifdef INCLUDE_TBOX_5288
	for (CardLoop = 0; CardLoop < TBOX_5288_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_5288 & 0xFFFF), (PCI_ID_TBOX_5288 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_5288_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5288Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs = (UINT8 *) (Bar2 + 0x00); break;  /* 1st Port on BAR 2 */
				case 1: pChan->regs = (UINT8 *) (Bar2 + 0x08); break;  /* 2nd Port on BAR 2 */
				case 2: pChan->regs = (UINT8 *) (Bar2 + 0x10); break;  /* 3rd Port on BAR 2 */
				case 3: pChan->regs = (UINT8 *) (Bar2 + 0x18); break;  /* 4th Port on BAR 2 */
				case 4: pChan->regs = (UINT8 *) (Bar3 + 0x00); break;  /* 1st Port on BAR 3 */
				case 5: pChan->regs = (UINT8 *) (Bar3 + 0x08); break;  /* 2nd Port on BAR 3 */
				case 6: pChan->regs = (UINT8 *) (Bar3 + 0x10); break;  /* 3rd Port on BAR 3 */
				case 7: pChan->regs = (UINT8 *) (Bar3 + 0x18); break;  /* 4th Port on BAR 3 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 

			/* Driver Initialization routine */
			ns16550DevInit(pChan);
		}
	}
#endif /* INCLUDE TBOX 5288 */

#ifdef INCLUDE_TBOX_4966
	for (CardLoop = 0; CardLoop < TBOX_4966_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_4966 & 0xFFFF), (PCI_ID_TBOX_4966 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_4966_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio4966Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs = (UINT8 *) (Bar2 + 0x00); break;  /* 1st Port on BAR 2 */
				case 1: pChan->regs = (UINT8 *) (Bar3 + 0x00); break;  /* 1st Port on BAR 3 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 

			/* Driver Initialization routine */
			ns16550DevInit(pChan);
		}
	}
#endif /* INCLUDE TBOX 4966 */

#ifdef INCLUDE_TBOX_5382
	for (CardLoop = 0; CardLoop < TBOX_5382_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_5382 & 0xFFFF), (PCI_ID_TBOX_5382 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_5382_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5382Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs = (UINT8 *) (Bar2 + 0x00); break;  /* 1st Port on BAR 2 */
				case 1: pChan->regs = (UINT8 *) (Bar2 + 0x08); break;  /* 2nd Port on BAR 2 */
				case 2: pChan->regs = (UINT8 *) (Bar2 + 0x10); break;  /* 3rd Port on BAR 2 */
				case 3: pChan->regs = (UINT8 *) (Bar2 + 0x18); break;  /* 4th Port on BAR 2 */
				case 4: pChan->regs = (UINT8 *) (Bar3 + 0x00); break;  /* 1st Port on BAR 3 */
				case 5: pChan->regs = (UINT8 *) (Bar3 + 0x08); break;  /* 2nd Port on BAR 3 */
				case 6: pChan->regs = (UINT8 *) (Bar3 + 0x10); break;  /* 3rd Port on BAR 3 */
				case 7: pChan->regs = (UINT8 *) (Bar3 + 0x18); break;  /* 4th Port on BAR 3 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 

			/* Driver Initialization routine */
			ns16550DevInit(pChan);
		}
	}
#endif /* INCLUDE TBOX 5382 */

#ifdef INCLUDE_TBOX_3607
	for (CardLoop = 0; CardLoop < TBOX_3607_NUM_CARDS; CardLoop++)
	{
		if (PciFindDevice_M ((PCI_ID_TBOX_3607 & 0xFFFF), (PCI_ID_TBOX_3607 >> 16) & 0xFFFF,
						   CardLoop, &Bus, &Dev, &Func) != ERROR)
		{
			/* Read BAR and Interrupt Registers */
			PciConfigInLong_M (Bus, Dev, Func, 0x18, &Bar2 );
			PciConfigInLong_M (Bus, Dev, Func, 0x1c, &Bar3 );
			PciConfigInLong_M (Bus, Dev, Func, 0x20, &Bar4 );
			PciConfigInLong_M (Bus, Dev, Func, 0x24, &Bar5 );
			PciConfigInByte_M (Bus, Dev, Func, 0x3c, &IntLevel);

			Bar2 = PCI_IO_TO_CPU_ADDR(Bar2 & 0xFFFFFFFE);
			Bar3 = PCI_IO_TO_CPU_ADDR(Bar3 & 0xFFFFFFFE);
			Bar4 = PCI_IO_TO_CPU_ADDR(Bar4 & 0xFFFFFFFE);
			Bar5 = PCI_IO_TO_CPU_ADDR(Bar5 & 0xFFFFFFFE);
			if (LevelOverride != 0)
			{
				IntLevel = LevelOverride;
			}
		}
		else
		{
			logMsg("ERROR.  Could not find card\n", 0,0,0,0,0,0);
		}

		for (PortLoop = 0; PortLoop < TBOX_3607_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio3607Channels[CardLoop][PortLoop];

			/* Initialization */
			switch (PortLoop)
			{
				case 0: pChan->regs =  (UINT8 *) (Bar2 + 0x00); break; /* 1st Port on BAR 2 */
				case 1: pChan->regs =  (UINT8 *) (Bar2 + 0x08); break; /* 2nd Port on BAR 2 */
				case 2: pChan->regs =  (UINT8 *) (Bar2 + 0x10); break; /* 3rd Port on BAR 2 */
				case 3: pChan->regs =  (UINT8 *) (Bar2 + 0x18); break; /* 4th Port on BAR 2 */
				case 4: pChan->regs =  (UINT8 *) (Bar3 + 0x00); break; /* 1st Port on BAR 3 */
				case 5: pChan->regs =  (UINT8 *) (Bar3 + 0x08); break; /* 2nd Port on BAR 3 */
				case 6: pChan->regs =  (UINT8 *) (Bar3 + 0x10); break; /* 3rd Port on BAR 3 */
				case 7: pChan->regs =  (UINT8 *) (Bar3 + 0x18); break; /* 4th Port on BAR 3 */
				case 8: pChan->regs =  (UINT8 *) (Bar4 + 0x00); break; /* 1st Port on BAR 4 */
				case 9: pChan->regs =  (UINT8 *) (Bar4 + 0x08); break; /* 2nd Port on BAR 4 */
				case 10: pChan->regs = (UINT8 *) (Bar4 + 0x10); break; /* 3rd Port on BAR 4 */
				case 11: pChan->regs = (UINT8 *) (Bar4 + 0x18); break; /* 4th Port on BAR 4 */
				case 12: pChan->regs = (UINT8 *) (Bar5 + 0x00); break; /* 1st Port on BAR 5 */
				case 13: pChan->regs = (UINT8 *) (Bar5 + 0x08); break; /* 2nd Port on BAR 5 */
				case 14: pChan->regs = (UINT8 *) (Bar5 + 0x10); break; /* 3rd Port on BAR 5 */
				case 15: pChan->regs = (UINT8 *) (Bar5 + 0x18); break; /* 4th Port on BAR 5 */
				default: logMsg("ERROR.  Default Case %d\n", PortLoop, 0,0,0,0,0);
			}
			pChan->level = IntLevel;
			pChan->regDelta = TBOX_ADJ_REGS;
			pChan->xtal = TBOX_SIO_BAUD_XTAL; 
			pChan->baudRate = TBOX_SIO_BAUD_RATE; 

			/* Driver Initialization routine */
			ns16550DevInit(pChan);
		}
	}
#endif /* INCLUDE TBOX 3607 */

	return;
}

/******************************************************************************
*
* TboxSioSerialHwInit2 - connect Technobox serial device interrupts
*
* This routine connects the BSP serial device interrupts.
* It should be called from sysSerialHwInit2().
*
* <VectorOverride> allows the user to override the value the BSP setup.  Pass 0
* to keep the BSP value.  Most BSP's setup the value correctly.
*
* SEE ALSO: sysHwInit(), sysSerialHwInit(), sysSerialHwInit2(), TboxSioSerialHwInit()
*
* RETURNS: N/A
*/
void TboxSioSerialInit2 (int VectorOverride)
{
	int					CardLoop;
	int					PortLoop;
	int					Vector;
	NS16550_CHAN		*pChan;
#ifdef INCLUDE_TBOX_4382
	volatile UINT8		*pAddr;
	UINT8				Data;
#endif

	/* Connect the Interrupt Vector and Enable the Interrupt. */

#ifdef INCLUDE_TBOX_4960
	for (CardLoop = 0; CardLoop < TBOX_4960_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_4960_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio4960Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			/* See installation manual for pciIntConnect vs. intConnect discussion */
			(void) pciIntConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 4960 */

#ifdef INCLUDE_TBOX_5284
	for (CardLoop = 0; CardLoop < TBOX_5284_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < 8; PortLoop++)
		{
			pChan = &TboxSio5284Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}			
			/* Connect Interrupt Routine */
			(void) intConnect ((VOIDFUNCPTR*)Vector, ns16550Int, (int)pChan );
		
			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 5284 */

#ifdef INCLUDE_TBOX_5436
	for (CardLoop = 0; CardLoop < TBOX_5436_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5436_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5436Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			(void) intConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 5436 */

#ifdef INCLUDE_TBOX_5288
	for (CardLoop = 0; CardLoop < TBOX_5288_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5288_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5288Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			(void) intConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 5288 */

#ifdef INCLUDE_TBOX_4966
	for (CardLoop = 0; CardLoop < TBOX_4966_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_4966_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio4966Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			(void) intConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 4966 */

#ifdef INCLUDE_TBOX_5382
	for (CardLoop = 0; CardLoop < TBOX_5382_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5382_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio5382Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			(void) intConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 5382 */

#ifdef INCLUDE_TBOX_3607
	for (CardLoop = 0; CardLoop < TBOX_3607_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_3607_UART_CHANNELS; PortLoop++)
		{
			pChan = &TboxSio3607Channels[CardLoop][PortLoop];
			if (VectorOverride != 0)
			{
				Vector = VectorOverride;
			}
			else
			{
				Vector = (int)pChan->level;
			}

			/* Connect Interrupt Routine */
			(void) intConnect (INUM_TO_IVEC (Vector), ns16550Int, (int)pChan );

			/* Enable Interrupt Routine */
			intEnable (pChan->level); 
		}
	}
#endif /* INCLUDE TBOX 3607 */

	return;
}


/******************************************************************************
*
* TboxSioSerialHwReset - Reset the Technobox serial devices to a quiescent state
*
* This routine resets the Technobox serial ports and puts the
* devices in a quiescent state.  It should be called from sysSerialHwReset().
*
* NOTES: 
* This routine simply calls the driver iniitialization.
*
* SEE ALSO: sysHwInit(), sysSerialHwInit2(), sysSerialHwInit2(), TboxSioSerialHwInit2()
*
* RETURNS: N/A
*/
void TboxSioSerialHwReset (void)
{
	int					CardLoop;
	int					PortLoop;

	/* Call the 16550 drv initialization routine for each port. */

#ifdef INCLUDE_TBOX_4960
	for (CardLoop = 0; CardLoop < TBOX_4960_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_4960_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio4960Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 4960 */

#ifdef INCLUDE_TBOX_5284
	for (CardLoop = 0; CardLoop < TBOX_5284_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < 8; PortLoop++)
		{
			ns16550DevInit(&TboxSio5284Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 5284 */

#ifdef INCLUDE_TBOX_5436
	for (CardLoop = 0; CardLoop < TBOX_5436_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5436_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio5436Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 5436 */

#ifdef INCLUDE_TBOX_5288
	for (CardLoop = 0; CardLoop < TBOX_5288_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5288_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio5288Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 5288 */

#ifdef INCLUDE_TBOX_4966
	for (CardLoop = 0; CardLoop < TBOX_4966_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_4966_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio4966Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 4966 */

#ifdef INCLUDE_TBOX_5382
	for (CardLoop = 0; CardLoop < TBOX_5382_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_5382_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio5382Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 5382 */

#ifdef INCLUDE_TBOX_3607
	for (CardLoop = 0; CardLoop < TBOX_3607_NUM_CARDS; CardLoop++)
	{
		for (PortLoop = 0; PortLoop < TBOX_3607_UART_CHANNELS; PortLoop++)
		{
			ns16550DevInit(&TboxSio3607Channels[CardLoop][PortLoop]);
		}
	}
#endif /* INCLUDE TBOX 3607 */

	return;
}
