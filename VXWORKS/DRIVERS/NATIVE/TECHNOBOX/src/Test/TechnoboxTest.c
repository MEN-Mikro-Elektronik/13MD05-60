/* TboxVxWorksUtil - Technobox's VxWorks Serial Port Utilities */
/* TboxSioUtils.c file, part of the TboxVxWorksUtil Serial Port Utilities */

/******************************************************************************
 *
 * Copyright 2006 by Technobox
 *
 * NOTE: SEE LICENSE TERMS IN DESCRIPTION BELOW OR LICENSE FILE.
 *
 * Functions: This file defines a Serial Port Utilites to demonstrate data
 *            accesses to the serial port.
 *
 *
 * NOTES:
 *	Known Bugs.  Input is not seen when you type it through the Tornado Shell. 
 *  Not sure why and I have not pursued this.
 *
 * Revision:
 * REV   WHO   DATE       SWF#   COMMENTS
 * ----+-----+----------+------+-----------------------------------------------
 * 004   SGL   04/21/06          Updates for output, fix clearing all input data
 * 003   SGL   03/23/04          Updates for documentation
 * 002   SGL   03/18/04          Added self-loopback test
 * 001   SGL   03/15/04          Added loopback test
 * 000   SGL   03/12/04          Initial Release
 *
 *****************************************************************************/

/* Includes follow (Tornado II) */
#include "vxWorks.h"
#include "stdio.h"
#include "tyLib.h"
#include "tasklib.h"
#include "string.h"
#include "siolib.h"

/*
DESCRIPTION
The TboxVxWorksUtil Serial Port utilities provide functions that test
the serial ports.  These utilities read and write data and make ioctl calls to
change the serial port parameters.

These utilities are designed as debugging aids only.  They are designed to 
demonstrate accesses to a serial port.  They are not intended to be a run 
time application.

Technobox provides both a Windows project file and the Makefile that was generated
from this project file.  We recommend that you use the project file if possible.
The makefile is automatically generated from the project file and has
hard coded directory names based on the relative directory names in the project.
If you use the makefile you will need to change the directory names.

LICENSE:

\ss
Redistribution, incorporation of a portion or all of this source code into user's application, and 
use in source and binary forms, with or without modification, are permitted by other parties 
provided that the following conditions are met:

1. This Sample Code may only be used with Technobox Inc. products.

2. This Sample Code is only designed to demonstrate access to the adapter.  Portions of this code 
   may not be appropriate for your run time application.
   
3. EXCEPT WHEN OTHERWISE STATED IN WRITING, TECHNOBOX INC. AND/OR OTHER PARTIES PROVIDE THE PROGRAM
   "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK
   AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE,
   YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL TECHNOBOX, OR ANY 
   OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU
   FOR DAMAGES, INCLUDING ANY GENERAL, DIRECT, INDIRECT, SPECIAL, INCIDENTAL, EXEMPLARY, OR 
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING, BUT NOT
   LIMITED TO, LOSS OF USE, DATA, OR PROFITS; DATA BEING RENDERED INACCURATE; PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSSES SUSTAINED BY YOU OR THIRD PARTIES; BUSINESS INTERRUPTION; OR A FAILURE 
   OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) HOWEVER CAUSED (INCLUDING NEGLIGENCE OR OTHERWISE),
   EVEN IF TECHNOBOX INC. OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. 
\se

*/

#define DATA_LOOPS 200
#define INNER_DATA 40

static UINT32 GTB_Size = 2048;
static UINT32 G_tx = 0;
static UINT32 G_rx = 0;

/*****************************************************************************
 *****************************************************************************
 *****************************************************************************
 *
 *
 *
 *
 * LOCAL FUNCTIONS FOLLOW - Not directly callable by the user
 *
 *
 *
 *
 *****************************************************************************
 *****************************************************************************
 *****************************************************************************/


/*******************************************************************************
*
* TboxUtilPciRevision - Print revision of the utilities
*
* This function simply prints the revision of the utilities
*/
LOCAL STATUS TboxUtilSioRevision( void )
{
    printf("\nTechnobox VxWorks Sio Tests Revision %s, Date %s\n\n",
		"", "");

	return OK;
}

/*******************************************************************************
*
* TboxUtilGetNextc - Get next character from TTY
*
*/
LOCAL char TboxUtilGetNextc(int tty)
{
	char instr[10];
	int i;

	while ((i = read(tty,instr,1)) == 0);		/* Wait for next character */
	return(instr[0]);
}

/*******************************************************************************
*
* TboxUtilFdReadln - Read characters into string until we hit CR
*
*/
LOCAL void TboxUtilFdReadln(int fd, char *instr)
{
	char c;
	int i;

	instr[0] = '\0';

	while ((c = TboxUtilGetNextc(fd)) != (char)0x0a) 
	{
		i = strlen(instr);
		if (c == (char)0x8)	/* Backspace? */
		{
			if (i != 0)
			{
				fdprintf(fd, "\b \b");	/* BS, space, BS to clear last char */
				instr[i-1] = '\0';
			}
		}
		else
		{
			instr[i] = c;		/* Not backspace....append character */
			instr[i+1] = '\0';
			fdprintf(fd, "%c", instr[i]);
		}
	}
	fdprintf(fd, "\n");
}

/*******************************************************************************
*
* TboxUtilFdScanf - Formatted input from file descriptor
*
*/
LOCAL void TboxUtilFdScanf(int fd, char *fmtstr, void *result)
{
	char instr[100];

	*((unsigned int *) result) = 0;
	TboxUtilFdReadln(fd,instr);
	if (strlen(instr) != 0)
	{
		sscanf(instr,fmtstr,result);
	}
}

/*******************************************************************************
 *
 * TboxUtilSioLoopTestWrite - Write Data to the Serial Port
 *
 * Simply write data to the serial port.  This test is intended to be spawned.
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioLoopTestWrite(int ttyWr, char	*DataWr, int DataSize, int *BytesWritten)
{		
#if 0 /*for poll mode*/
	
	int rtn;	

	*BytesWritten = 0;
	while( DataSize > *BytesWritten) {
		if (G_tx   -  G_rx  < 15 ){			
			rtn = write(ttyWr, &DataWr[*BytesWritten], 1);		
			*BytesWritten += rtn;
			G_tx = *BytesWritten;
		}
		taskDelay(1);
	}
#else /*good for interrupt*/
	int rtn;
	int old_level;
	*BytesWritten = 0;
	while( DataSize > *BytesWritten) {
		if (G_tx  - G_rx  < 14){			
			rtn = write(ttyWr, &DataWr[*BytesWritten], 1);		
			G_tx += rtn;
			*BytesWritten = G_tx;			
		}
		/*taskDelay(1);*/
	}	
#endif
	return;
}

/*******************************************************************************
 *
 * TboxUtilSioLoopTestRead - Read Data from the Serial Port
 *
 * Simply read data from the serial port.  This test is intended to be spawned.
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioLoopTestRead(int ttyRd, char	*DataRd, int DataSize, int *BytesRead)
{
	int	NumBytes;

	NumBytes = 0;
	*BytesRead = 0;
	do
	{
		/* Offset Data Pointer based on number of bytes read */
		 NumBytes = read(ttyRd, (DataRd + *BytesRead), DataSize);
		 *BytesRead += NumBytes;
		 G_rx = *BytesRead;
	} while ((*BytesRead < DataSize));
	return;

}

/*******************************************************************************
 *
 * TboxUtilSioLoopTest - Loop Test between two different serial ports
 *
 * Send and receive from both ports at the same time and verify the results
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioLoopTest(char	*pDeviceOut, char	*pDeviceIn)
{
	int				ttyOut;
	int				ttyIn;
	int				WriteTaskId1, WriteTaskId2;
	int				ReadTaskId1, ReadTaskId2;
	STATUS			WriteTaskVr1, WriteTaskVr2;
	STATUS			ReadTaskVr1, ReadTaskVr2;
	int				BytesWritten1, BytesWritten2;
	int				BytesRead1, BytesRead2;
	unsigned char	*pDataOut1, *pDataOut2;
	unsigned char	*pDataIn1, *pDataIn2;
	unsigned char	*tmpData, *tmpDataRd;
	int				Size;
	int				RdSize;
	int				Loop;
	int				InnerLoop;
	int				ErrorDisplay;
	unsigned char	TestFailed = FALSE;
	
	printf(" --> TboxUtilSioLoopTest: Testing %s and %s.\n", pDeviceOut, pDeviceIn);

	ttyOut = open(pDeviceOut,O_RDWR,0);
	if (ttyOut == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDeviceOut);
		return;
	}
	ttyIn = open(pDeviceIn,O_RDWR,0);
	if (ttyIn == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDeviceIn);
		return;
	}
		
	/* Allocate space for the buffer */
	Size = GTB_Size;

	pDataOut1 = (unsigned char *) malloc (Size + 256);
	if (!pDataOut1)
	{
		printf("***> ERROR!!!! in TboxUtilSioLoopTest. Failed to allocate buffer\n");
		return;
	}

	for(Loop = 0; Loop < Size; Loop ++)		
		pDataOut1[Loop] = (UINT8)(Loop & 0xFF);

	pDataIn1 = (unsigned char *) malloc (Size + 256);
	if (!pDataIn1)
	{
		printf("***> ERROR!!!! in TboxUtilSioLoopTest. Failed to allocate buffer\n");
		free(pDataOut1);
		return;
	}
	tmpData = pDataOut1;

	ioctl (ttyOut, FIONREAD, (int) &BytesRead1);
	while (BytesRead1 > 0)
	{
		RdSize = read(ttyOut, (char *) pDataIn1, BytesRead1);
		printf(" --> Flushing Input Data. Found %d bytes, Read %d\n", BytesRead1, RdSize); 
		taskDelay(10);					/* Give time to clear write/read buffer*/
		ioctl (ttyOut, FIONREAD, (int) &BytesRead1);
	} 

	/* Now spawn the Read tasks */
	ReadTaskId1 = taskSpawn("tsiord1", 5, 0, 0x2000, (FUNCPTR) TboxUtilSioLoopTestRead, 
		ttyIn, (int) pDataIn1, Size, (int) &BytesRead1, 0, 0, 0, 0, 0, 0);
	if (ReadTaskId1 == ERROR)
	{
		printf("***> Error.  Could not open the read 1, %x\n", ReadTaskId1);
		TestFailed = TRUE;
	}

	taskDelay(5);
	if ( (BytesRead1 != 0) )
	{
		printf("***> Error.  Read got something already. Exp %d, actual %d\nn", 0, BytesRead1);
		TestFailed = TRUE;
	}
	
	/* Now spawn the Write tasks */
	WriteTaskId1 = taskSpawn("tsiowr1", 10, 0, 0x2000, (FUNCPTR) TboxUtilSioLoopTestWrite, 
		ttyOut, (int) pDataOut1, Size, (int) &BytesWritten1, 0, 0, 0, 0, 0, 0);
	if (WriteTaskId1 == ERROR)
	{
		printf("***> Error.  Could not open the write 1, %x\n", WriteTaskId1);
		TestFailed = TRUE;
	}

	printf(" --> Sending ");
	Loop = 0x4000;
	do
	{
		taskDelay(1);
		Loop--;
		WriteTaskVr1 = taskIdVerify(WriteTaskId1);
		if ((Loop & 0x3F) == 0)
		{
			printf(".");
		}
	} while ( ((WriteTaskVr1 == OK)));/* && (Loop > 0));*/

	printf("\n --> Reading ");
	Loop = 0x1000;
	do
	{
		taskDelay(1);
		Loop--;
		ReadTaskVr1 = taskIdVerify(ReadTaskId1);
		if ((Loop & 0x3F) == 0)
		{
			printf("+");
		}
	} while ( ((ReadTaskVr1 == OK)) && (Loop > 0));
	printf("\n");
	taskDelay(100);
	if (WriteTaskVr1 == OK)
	{
		printf("***> Error.  Write Task 1 never completed, Exp %x, Act %x\n", OK, WriteTaskVr1);
		taskDelete(WriteTaskId1);
		TestFailed = TRUE;
	}

	if (ReadTaskVr1 == OK)
	{
		printf("***> Error.  Read Task 1 never completed, Exp %x, Act %x\n", OK, ReadTaskVr1);
		taskDelete(ReadTaskId1);
		TestFailed = TRUE;
	}

	/* Now Check the Sent and Receive Counts */
	if ( (BytesWritten1 != Size))
	{
		printf("***> Error.  Write did not send all. Exp %d, Act %d\n", Size, BytesWritten1);
		TestFailed = TRUE;
	}
	if ( (BytesRead1 != Size))
	{
		printf("***> Error.  Read did not get all. Exp %d, Act %d\n", Size, BytesRead1);
		TestFailed = TRUE;
	}

	/* Now Compare the Data */
	tmpData = pDataOut1;
	tmpDataRd = pDataIn1;
	ErrorDisplay = 60;
	printf(" --> Verifying\n");

	for (Loop = 0; Loop < Size; Loop++)
	{
		if ((pDataOut1[Loop] != pDataIn1[Loop]) && (ErrorDisplay > 0))
			{
				printf("***> Data Miscompare.  Byte %d exp %02x act %02x\n",
					Loop, pDataOut1[Loop], pDataIn1[Loop]);
				ErrorDisplay--;
				TestFailed = TRUE;
			}
		
	}

	if (!TestFailed)
	{
		printf(" --> Test Completed\n");
	}
	else
	{
		printf("***> Test Failed.\n");
	}

	free(pDataOut1);
	free(pDataIn1);

	close(ttyOut);

	return;
}

/*******************************************************************************
 *
 * TboxUtilSioLoopSelfTest - Loop Test to yourself
 *
 * Send and receive data and verify the results
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioLoopSelfTest(char	*pDevice)
{
	int				ttyOut;
	int				WriteTaskId1;
	int				ReadTaskId1;
	STATUS			WriteTaskVr1;
	STATUS			ReadTaskVr1;
	int				BytesWritten1;
	int				BytesRead1;
	unsigned char	*pDataOut1;
	unsigned char	*pDataIn1;
	unsigned char	*tmpData, *tmpDataRd;
	int				Size;
	int				RdSize;
	int				Loop;
	int				InnerLoop;
	int				ErrorDisplay;
	unsigned char	TestFailed = FALSE;

	printf(" --> TboxUtilSioLoopSelfTest: Testing %s\n", pDevice);

	ttyOut = open(pDevice,O_RDWR,0);
	if (ttyOut == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDevice);
		return;
	}
#if 0 /*half-full interrupt*/
	*(UINT8*)0xFBFF908A = 0x1;
	*(UINT8*)0xFBFF908A = 0x87;
#endif

#if 0 /*poll mode*/
	*(UINT8*)0xFBFF9089 = 0x0;
	*(UINT8*)0xFBFF908A = 0x1;
#endif
	/* Allocate space for the buffer */

	Size = GTB_Size;

	pDataOut1 = (unsigned char *) malloc (Size + 256);
	if (!pDataOut1)
	{
		printf("***> ERROR!!!! in TboxUtilSioLoopSelfTest. Failed to allocate buffer\n");
		return;
	}

	for(Loop = 0; Loop < Size; Loop ++)
		/*pDataOut1[Loop] = (Loop & 1) ? 0xFF : 0;*/ /*(UINT8)(Loop + 1);*/
		pDataOut1[Loop] = (UINT8)(Loop & 0xFF);

	pDataIn1 = (unsigned char *) malloc (Size + 256);
	if (!pDataIn1)
	{
		printf("***> ERROR!!!! in TboxUtilSioLoopSelfTest. Failed to allocate buffer\n");
		free(pDataOut1);
		return;
	}
	tmpData = pDataOut1;

	ioctl (ttyOut, FIONREAD, (int) &BytesRead1);
	while (BytesRead1 > 0)
	{
		RdSize = read(ttyOut, (char *) pDataIn1, BytesRead1);
		printf(" --> Flushing Input Data. Found %d bytes, Read %d\n", BytesRead1, RdSize); 
		taskDelay(10);					/* Give time to clear write/read buffer*/
		ioctl (ttyOut, FIONREAD, (int) &BytesRead1);
	} 

	/* Now spawn the Read tasks */
	ReadTaskId1 = taskSpawn("tsiord1", 5, 0, 0x2000, (FUNCPTR) TboxUtilSioLoopTestRead, 
		ttyOut, (int) pDataIn1, Size, (int) &BytesRead1, 0, 0, 0, 0, 0, 0);
	if (ReadTaskId1 == ERROR)
	{
		printf("***> Error.  Could not open the read 1, %x\n", ReadTaskId1);
		TestFailed = TRUE;
	}

	taskDelay(5);
	if ( (BytesRead1 != 0) )
	{
		printf("***> Error.  Read got something already. Exp %d, actual %d\nn", 0, BytesRead1);
		TestFailed = TRUE;
	}
	
	/* Now spawn the Write tasks */
	WriteTaskId1 = taskSpawn("tsiowr1", 10, 0, 0x2000, (FUNCPTR) TboxUtilSioLoopTestWrite, 
		ttyOut, (int) pDataOut1, Size, (int) &BytesWritten1, 0, 0, 0, 0, 0, 0);
	if (WriteTaskId1 == ERROR)
	{
		printf("***> Error.  Could not open the write 1, %x\n", WriteTaskId1);
		TestFailed = TRUE;
	}

	printf(" --> Sending ");
	Loop = 0x4000;
	do
	{
		taskDelay(1);
		Loop--;
		WriteTaskVr1 = taskIdVerify(WriteTaskId1);
		if ((Loop & 0x3F) == 0)
		{
			printf(".");
		}
	} while ( ((WriteTaskVr1 == OK)));/* && (Loop > 0));*/

	printf("\n --> Reading ");
	Loop = 0x1000;
	do
	{
		taskDelay(1);
		Loop--;
		ReadTaskVr1 = taskIdVerify(ReadTaskId1);
		if ((Loop & 0x3F) == 0)
		{
			printf("+");
		}
	} while ( ((ReadTaskVr1 == OK)) && (Loop > 0));
	printf("\n");

	if (WriteTaskVr1 == OK)
	{
		printf("***> Error.  Write Task 1 never completed, Exp %x, Act %x\n", OK, WriteTaskVr1);
		taskDelete(WriteTaskId1);
		TestFailed = TRUE;
	}

	if (ReadTaskVr1 == OK)
	{
		printf("***> Error.  Read Task 1 never completed, Exp %x, Act %x\n", OK, ReadTaskVr1);
		taskDelete(ReadTaskId1);
		TestFailed = TRUE;
	}

	/* Now Check the Sent and Receive Counts */
	if ( (BytesWritten1 != Size))
	{
		printf("***> Error.  Write did not send all. Exp %d, Act %d\n", Size, BytesWritten1);
		TestFailed = TRUE;
	}
	if ( (BytesRead1 != Size))
	{
		printf("***> Error.  Read did not get all. Exp %d, Act %d\n", Size, BytesRead1);
		TestFailed = TRUE;
	}

	/* Now Compare the Data */
	tmpData = pDataOut1;
	tmpDataRd = pDataIn1;
	ErrorDisplay = 60;
	printf(" --> Verifying\n");

	for (Loop = 0; Loop < Size; Loop++)
	{
		if ((pDataOut1[Loop] != pDataIn1[Loop]) && (ErrorDisplay > 0))
			{
				printf("***> Data Miscompare.  Byte %d exp %02x act %02x\n",
					Loop, pDataOut1[Loop], pDataIn1[Loop]);
				ErrorDisplay--;
				TestFailed = TRUE;
			}
		
	}

	if (!TestFailed)
	{
		printf(" --> Test Completed\n");
	}
	else
	{
		printf("***> Test Failed.\n");
	}

	free(pDataOut1);
	free(pDataIn1);

	close(ttyOut);

	return;
}

/*******************************************************************************
 *
 * TboxUtilSioIoctlTest - Test various ioctl calls
 *
 * Test the ioctl calls and verify or display the results
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioIoctlTest(char	*pDevice)
{
	int		tty;
	int		Data;
	int		OrigData;
	STATUS	Status;
	char	Data1[] = "Baud Test. This should be good.\n";
	char	Data2[] = "Baud Test. This should be garbled.  If not then test failed.\n";

	printf(" --> TboxUtilSioIoctlTest: Testing %s.\n", pDevice);

	tty = open(pDevice,O_RDWR,0);
	if (tty == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDevice);
		return;
	}

	OrigData = ioctl(tty, FIOGETOPTIONS, 0);

	ioctl(tty,FIOSETOPTIONS,OPT_TERMINAL);
	Data = ioctl(tty, FIOGETOPTIONS, 0);
	if (Data != OPT_TERMINAL)
	{
		printf("***> Error.  Setting Data did not match Exp %x Actual %x\n", OPT_TERMINAL, Data);
	}

	ioctl(tty,FIOSETOPTIONS,OrigData);
	Data = ioctl(tty, FIOGETOPTIONS, 0);
	if (Data != OrigData)
	{
		printf("***> Error.  Restore Data did not match Exp %x Actual %x\n", OrigData, Data);
	}

	Data = ioctl(tty, FIOISATTY, 0);
	if (!Data)
	{
		printf("***> Error.  Device is not a tty device\n");
	}

	OrigData = 1234;

	Status = ioctl(tty, SIO_BAUD_GET, (int) &OrigData);
	if (Status == OK)
	{
		printf(" --> Current Baud Rate Settings %d\n", OrigData );
	}
	else
	{
		printf("***> Error.  Could not get Baud Rate\n");
	}

	Status = ioctl(tty, SIO_HW_OPTS_GET,  (int) &Data);
	if (Status == OK)
	{
		printf(" --> Current HW Options Settings %02x\n", Data );
	}
	else
	{
		printf("***> Error.  Could not get Baud Rate\n");
	}

	Status = ioctl(tty, SIO_MODE_GET,  (int) &Data);
	if (Status == OK)
	{
		if (Data != SIO_MODE_INT)
		{
			printf("***> Error.  Not Interrupt Mode\n");
		}
	}
	else
	{
		printf("***> Error.  Could not get Baud Rate\n");
	}

	Status = ioctl(tty, SIO_AVAIL_MODES_GET,  (int) &Data);
	if (Status == OK)
	{
		if (Data != (SIO_MODE_INT | SIO_MODE_POLL) )
		{
			printf("***> Error.  Not available Interrupt and POLL Mode\n");
		}
	}
	else
	{
		printf("***> Error.  Could not get Baud Rate\n");
	}

	fdprintf(tty, "%s", Data1);
	printf(" --> Check Serial Port Output for text.  Should be good.\n");
	taskDelay(100);
	Status = ioctl(tty,FIOBAUDRATE,300);
	if (Status == OK)
	{
		Status = ioctl(tty, SIO_BAUD_GET, (int) &Data);
		taskDelay(5);
		if (Data != 300)
		{
			printf("***> Error.  Baud Rate did not match\n");
		}
		fdprintf(tty, "%s", Data2);
		printf(" --> Check Serial Port Output for text.  Should be garbled.\n");
		taskDelay(200);

		Status = ioctl(tty,FIOBAUDRATE,OrigData);
		taskDelay(5);

		Status = ioctl(tty, SIO_BAUD_GET, (int) &Data);
		if (Data != OrigData)
		{
			printf("***> Error.  Baud Rate was not restored\n");
		}
		fdprintf(tty, "%s", Data1);
		printf(" --> Check Serial Port Output for text.  Should be good, garbled, good.\n");
	}
	else
	{
		printf("***> Error.  Could not set Baud Rate\n");
	}
	
	close(tty);

	printf(" --> Test Completed.\n");

}

/*******************************************************************************
 *
 * TboxUtilSioWriteTest - Write Data to the Serial Port
 *
 * Simply write a string to the serial port
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioWriteTest(char	*pDevice)
{
	char	Data1[] = "\n\nThis is a test of writing data to the serial port.\n\n";
	char	Data2[] = "Test Completed.  Return to shell.\n";
	int		tty;

	printf(" --> TboxUtilSioWriteTest: Testing %s.  Check port for output.\n", pDevice);

	tty = open(pDevice,O_RDWR,0);
	if (tty == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDevice);
		return;
	}
	ioctl(tty,FIOSETOPTIONS,OPT_TERMINAL & ~OPT_LINE & ~OPT_ECHO);

	fdprintf(tty, "%s", Data1);
	fdprintf(tty, "%s", Data2);

	close(tty);

	printf(" --> Data Sent.  Verify data was received on the serial port.\n");

}

/*******************************************************************************
 *
 * TboxUtilSioReadTest - Read Data From the Serial Port
 *
 * Simply read a string to the serial port
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioReadTest(char	*pDevice)
{
	char	Data[80];
	int		tty;
	int		Loop;

	printf(" --> TboxUtilSioReadTest: Testing %s.\n", pDevice);

	tty = open(pDevice,O_RDWR,0);
	if (tty == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDevice);
		return;
	}
	ioctl(tty,FIOSETOPTIONS,OPT_TERMINAL & ~OPT_LINE & ~OPT_ECHO);

	printf(" --> Input data on the serial port.  Max 40 chars or 'x' to terminate.\n");

	Loop = 0;
	do
	{
		Data[Loop] = TboxUtilGetNextc(tty);
		fdprintf(tty, "%c", Data[Loop]);
		Loop++;
	} while ( (Loop < 40) && (Data[Loop-1] != 'x'));

	Data[Loop] = '\0';
	printf(" --> Data Received.  The Data you typed was:\n");
	printf(" --> '%s'\n", Data);

	close(tty);

}

/*******************************************************************************
 *
 * TboxUtilSioSetBaud - Set the baud rate
 *
 * Get new baud rate and program it
 *
 * RETURNS:
 * NA.
 */
LOCAL void TboxUtilSioSetBaud(char	*pDevice)
{
	unsigned int	BaudRate;					/* BaudRate*/
	unsigned int	BaudData;					/* Programmed BaudRate*/
	int				tty;
	STATUS			Status;

	tty = open(pDevice,O_RDWR,0);
	if (tty == ERROR)
	{
		printf("***> Error. Could not open Device %s\n", pDevice);
		return;
	}

	printf("Enter Baud Rate: ");
	TboxUtilFdScanf(0, "%u", &BaudRate);
	Status = ioctl(tty,FIOBAUDRATE, BaudRate);
	taskDelay(5);
	Status = ioctl(tty, SIO_BAUD_GET, (int) &BaudData);
	if (BaudData != BaudRate)
	{
		printf("***> Failed to update baud rate, exp %d act %d\n", BaudRate, BaudData);
	}
	else
	{
		printf("New Baud Rate is %d\n", BaudRate);
	}
	close(tty);
}

/*******************************************************************************
*
* TboxUtilSioTest - Test the Serial Port
*
* The function is the main entry point into the Serial Port Utilities test application.
* The function will prompt for the serial port to use and then provide a menu 
* of options.
*
* The menu options are as follows:
* .iP "" 4
* Write Data to Serial Port - Write small amount of text and returns
* .iP
* Read Data from Serial Port - Reads up to 40 bytes or first x and displays text read then returns.
* .iP
* Test Ioctl - Tests various ioctl calls
* .iP
* Loop data between two ports - sends/receives data between two serial ports.  Prompts for second 
* serial port to use.  Must have loopback cable between both ports.
* .iP
* Loop data to same port - sends/receives data to itself.  Must have 
* RX/TX loopback cable on this port.
* .iP
* Change Baud Rate - Change the baud rate.  Enter new rate, ex 9600.
* .iP
* Change Serial Port - Change the serial port under test.
* .iP
* Change iteration count - Specify the number of times to run a test.
* .LP
*
* NOTE:
* Known Bugs.  Input is not seen when you type it through the Tornado Shell. 
* Not sure why and I have not pursued this.  Simply type the data and press return
* even though you do not see the results.
* 
* RETURNS:
* OK.
*/
STATUS TboxTest(void )
{
	int				data;
	char			c;
	unsigned short	menuOK;
	unsigned int	icount;
	unsigned int	x;
	unsigned short	expertmode;
	unsigned int	iterations;					/*Iteration count*/
	char			TestPort[32];
	char			LoopbackPort[32];

	TboxUtilSioRevision();

	printf("Enter Serial Port under test (will not see text) (format is '/tyCo/#'): ");
	TboxUtilFdScanf(0, "%s", TestPort);
	printf("Serial Port Entered is %s\n",TestPort);

	iterations = 1;
	expertmode = FALSE;
	menuOK = TRUE;

	while (menuOK)
	{
		if (!expertmode)
		{
			printf("\n");
			printf("  -- -----------------------------------------------------------\n");
			printf("  0. Write Data to Serial Port\n");
			printf("  1. Read Data from Serial Port\n");
			printf("  2. Test Ioctl\n");
			printf("  3. Loop data between two ports\n");
			printf("  4. Loop data to same ports\n");
			printf("  B. Change Baud Rate\n");
			printf("  P. Change Serial Port\n");
			printf("  I. Change iteration count\n");
			printf("  S. Change data size\n");
			printf("  X. Exit\n");
			printf("\n");
		}
		printf("Enter Selection then return (will not see selection): ");
		data = TboxUtilGetNextc(0);
		TboxUtilGetNextc(0);
		c = (char) data;
		if ((c <= 'z') && (c >= 'a'))
		{
			c = c -'a' + 'A';
		}
		printf("%c\n\n",c);

		icount = iterations;

		for (x = 0; x < icount; x++)
		{
			switch (c)
			{
				case '0':	TboxUtilSioWriteTest(TestPort); 
							break;
				case '1':	TboxUtilSioReadTest(TestPort);
							break;
				case '2':	TboxUtilSioIoctlTest(TestPort);
							break;
				case '3':	if (x == 0) /* Get port the first time through */
							{
								printf("Enter Loopback Serial Port (format is '/tyCo/#'): ");
								TboxUtilFdScanf(0, "%s", LoopbackPort);
								printf("Loopback Serial Port Entered is %s\n",LoopbackPort);
							}
							TboxUtilSioLoopTest(TestPort, LoopbackPort);
							break;
				case '4':	TboxUtilSioLoopSelfTest(TestPort);
							break;
				case 'I':	x = icount;
							printf("Enter iteration loop count: ");
							TboxUtilFdScanf(0, "%u", &iterations);
							printf("New iteration count is %d\n",iterations);
							break;
				case 'B':	x = icount;
							TboxUtilSioSetBaud(TestPort);
							break;
				case 'P':	x = icount;
							printf("Current Serial Port is %s\n", TestPort);
							printf("Enter new Serial Port (will not see text) (format is '/tyCo/#'): ");
							TboxUtilFdScanf(0, "%s", TestPort);
							printf("Serial Port Entered is %s\n",TestPort);
							break;
				case 'S':	x = icount;
							printf("Current Size is %d\n", GTB_Size);
							printf("Enter new Size : ");
							TboxUtilFdScanf(0, "%u", &GTB_Size);
							printf("Serial Port Entered is %d\n",GTB_Size);
							break;

				case 'X':	menuOK = FALSE;
							x = icount; 
							break;
				default:	x = icount;
							break;
			}
		}
	}

	return( OK );
}

