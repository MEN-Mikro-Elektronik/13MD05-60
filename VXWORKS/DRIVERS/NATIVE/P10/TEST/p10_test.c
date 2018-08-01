/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: p10_test.c
 *      Project: P10/P11 VxWorks Driver
 *
 *       Author: rl
 *        $Date: 2002/04/22 14:26:47 $
 *    $Revision: 1.2 $
 *
 *  Description: VxWorks P10/P11 serial driver test program
 *
 *     Required: -
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: p10_test.c,v $
 * Revision 1.2  2002/04/22 14:26:47  Franke
 * cosmetics
 *
 * Revision 1.1  2000/03/?? ??.??.??  loesel
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static char *RCSid="$Id: p10_test.c,v 1.2 2002/04/22 14:26:47 Franke Exp $";

#include "vxWorks.h"			/* always first */
#include "usrLib.h"
#include "intLib.h"
#include "stdio.h"
#include "string.h"
#include "ioLib.h"
#include "errnoLib.h"
#include "sysLib.h"
#include <../p10_drv.h>

#define	SERIAL_BUF_SIZE		1024

#define STD_BAUD			9600

IMPORT int consoleFd; 
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
int p10Fd;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static char outPattern[]="THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG -\
 the quick brown fox jumps over the lazy dog\n";
 

/********************************* serTestId **********************************
 *  Description: Shows the version information. 
 *----------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  -
 *****************************************************************************/
void serTestId( void )
{
	printf("%s\n", RCSid );
}/*serTestId*/

/********************************* serXTest **********************************
 *
 *  Description: Check serial lines input and output 
 *
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  
 *
 *  Globals....:  
 *****************************************************************************/
int serXTest( char* outDevName, char* inDevName, UINT8 mode )
{
char	*inBuf=NULL;
int		error;
int		retVal=-1;
int		outFd;
int		inFd=ERROR;
int		numOutBytes;
int		numInBytes=0;

	/*printf("=== Serial Test RX/TX\n");*/

	if( !outDevName )
	{
		fprintf(stderr, "\n*** out device name missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	if( !inDevName )
	{
		fprintf(stderr, "\n*** in device name missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	inBuf=malloc( SERIAL_BUF_SIZE );
	if( !inBuf )
	{
		fprintf(stderr, "\n*** Not enough memory (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	outFd = open( outDevName, O_WRONLY, 0 );	
	if( outFd == ERROR )
	{
		fprintf(stderr, "\n*** Can not open out device %s (%s line %d)\n", outDevName, __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	inFd = open( inDevName, O_RDONLY, 0  );	
	if( inFd == ERROR )
	{
		fprintf(stderr, "\n*** Can not open in device %s (%s line %d)\n", inDevName, __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	error = ioctl( outFd, FIOFLUSH, 0 );	
	error |= ioctl( inFd, FIOFLUSH, 0 );
	
	/* set options to RAW to avoid echo */
	(void) ioctl (outFd, FIOSETOPTIONS, OPT_RAW); 
	(void) ioctl (inFd, FIOSETOPTIONS, OPT_RAW); 
	
	/* set standard baudrate */
	ioctl (outFd, FIOBAUDRATE, STD_BAUD);
	ioctl (inFd, FIOBAUDRATE, STD_BAUD);
	
	if( error )
	{
		fprintf(stderr, "\n*** FIOFLUSH failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	if (mode == 1)
	{
		ioctl (outFd, P10_TX_FIFO_LEVEL, 0x10);
		ioctl (inFd, P10_RX_FIFO_LEVEL, 0x40);
	}
	else if (mode == 2)
	{
		ioctl (outFd, P10_TX_FIFO_LEVEL, 0x30);
		ioctl (inFd, P10_RX_FIFO_LEVEL, 0x80);
	}
	
	else if (mode == 0xff)
	{
		ioctl (outFd, P10_NO_FIFOS, 0xff);
		ioctl (inFd, P10_NO_FIFOS, 0xff);
	}

	numOutBytes = write( outFd, outPattern, sizeof(outPattern) );
	if( numOutBytes != sizeof(outPattern) )
	{
		fprintf(stderr, "\n*** write failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	/* delay a second, should be enough also for 200 baud */
	taskDelay( sysClkRateGet() );

	error |= ioctl( inFd, FIONREAD, (int)&numInBytes );
	if( error )
	{
		fprintf(stderr, "\n*** FIONREAD failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	if( numInBytes != numOutBytes )
	{
		fprintf(stderr, "\n*** number of received bytes=%d not equal %d number of sent bytes\n",
				numInBytes, numOutBytes );
		goto CLEANUP;
	}/*if*/

	numInBytes = read( inFd, inBuf, numInBytes );
	if( numInBytes != numOutBytes )
	{
		fprintf(stderr, "\n*** number of read bytes=%d not equal %d number of sent bytes\n",
				numInBytes, numOutBytes );
		goto CLEANUP;
	}/*if*/

	/* check pattern */
	error = strncmp( inBuf, outPattern, numOutBytes );
	if( error )
	{
		fprintf(stderr, "\n*** pattern mismatch\n" );
		error = -1;
		goto CLEANUP;
	}/*if*/

	/* say OK */
	retVal = 0;

CLEANUP:
	free( inBuf);
	if(outFd!=ERROR)
		close(outFd);
	if(inFd!=ERROR)
		close(inFd);
	if( !retVal )
		fprintf(stderr, "%d ==> OK\n", numInBytes);
	else
		fprintf(stderr, " ==> FAILED\n");
	return( retVal );	
}/*serTestRxTx*/


/********************************* com34Test *********************************
 *
 *  Description: Check serial lines COM3(RS485) and COM4(RS422)
 *               Special adapter is needed
 *
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  
 *
 *  Globals....:  
 *****************************************************************************/
STATUS p10Test()
{
	char	buf[10];
	
	/*----------------+
	| wait for input  |
	+----------------*/
	do {
		fprintf(stderr, "\nP10 Test (c) 2000 MEN Microelektronik $Revision: 1.2 $\n");
		fprintf(stderr, "Utility to check COM1 and COM2 hardware, press ESC to quit\n");
		
		fprintf(stderr, "\np10_0\n");
		fprintf(stderr, "1 - COM1 with COM2 (16)\n");
		fprintf(stderr, "2 - COM3 with COM4 (16)\n");
		fprintf(stderr, "3 - COM1 with COM2 (no FIFO)\n");
		fprintf(stderr, "4 - COM3 with COM4 (no FIFO)\n");	
		
		fprintf(stderr, "\np10_1\n");
		fprintf(stderr, "5 - COM1 with COM2 (16)\n");
		fprintf(stderr, "6 - COM3 with COM4 (16)\n");
		fprintf(stderr, "7 - COM1 with COM2 (no FIFO)\n");
		fprintf(stderr, "8 - COM3 with COM4 (no FIFO)\n");
		
		fprintf(stderr, "\np11_0\n");
		fprintf(stderr, "a - COM1 with COM2 (16)\n");
		fprintf(stderr, "b - COM3 with COM4 (16)\n");
		fprintf(stderr, "c - COM1 with COM2 (no FIFO)\n");
		fprintf(stderr, "d - COM3 with COM4 (no FIFO)\n");		
				
		gets (buf);
	   			
		switch( buf[0] ){
		case '1':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (16Byte FIFO)" );
					serXTest ("/p10_0/0", "/p10_0/1", 0);
					break;

		case '2':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (16Byte FIFO)" );
					serXTest ("/p10_0/2", "/p10_0/3", 0);
					break;
		
							
		case '3':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (no FIFO)" );
					serXTest ("/p10_0/0", "/p10_0/1", 0xff);
					break;

		case '4':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (no FIFO)" );
					serXTest ("/p10_0/2", "/p10_0/3", 0xff);
					break;
					
			
					
		case '5':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (16Byte FIFO)" );
					serXTest ("/p10_1/0", "/p10_1/1", 0);
					break;

		case '6':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (16Byte FIFO)" );
					serXTest ("/p10_1/2", "/p10_1/3", 0);
					break;
		
							
		case '7':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (no FIFO)" );
					serXTest ("/p10_1/0", "/p10_1/1", 0xff);
					break;

		case '8':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (no FIFO)" );
					serXTest ("/p10_1/2", "/p10_1/3", 0xff);
					break;
				
					
		case 'a':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (16Byte FIFO)" );
					serXTest ("/p11_0/0", "/p11_0/1", 0);
					break;

		case 'b':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (16Byte FIFO)" );
					serXTest ("/p11_0/2", "/p11_0/3", 0);
					break;
		
							
		case 'c':	fprintf ( stderr, "\nTesting P10 COM1 with P10 COM2 (no FIFO)" );
					serXTest ("/p11_0/0", "/p11_0/1", 0xff);
					break;

		case 'd':	fprintf ( stderr, "\nTesting P10 COM3 with P10 COM4 (no FIFO)" );
					serXTest ("/p11_0/2", "/p11_0/3", 0xff);
					break;														
					
		}
	
	
	} while (buf[0] != 27); /* ESC (ASCII 27) */
	
		
	
	
	return OK;
		
	
}




/********************************* com34Test *********************************
 *
 *  Description: Check serial lines COM3(RS485) and COM4(RS422)
 *               Special adapter is needed
 *
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  
 *
 *  Globals....:  
 *****************************************************************************/
STATUS redirect(int channel)
{
	char				tyName [20]; 
	
	
	sprintf (tyName, "%s%d", "/p10_0/", channel);	
	/*----------------+
	| wait for input  |
	+----------------*/
	
	p10Fd = open (tyName, O_RDWR, 0);
	
	(void) ioctl (p10Fd, FIOBAUDRATE, 115200);
    (void) ioctl (p10Fd, FIOSETOPTIONS, OPT_TERMINAL);
    ioGlobalStdSet (STD_IN,  p10Fd);
    ioGlobalStdSet (STD_OUT, p10Fd);
    ioGlobalStdSet (STD_ERR, p10Fd);
	
	
	
	return OK;
		
	
}

STATUS direct()
{
	
	
	/*----------------+
	| wait for input  |
	+----------------*/
	
	
	
	ioGlobalStdSet (STD_IN,  consoleFd);
    ioGlobalStdSet (STD_OUT, consoleFd);
    ioGlobalStdSet (STD_ERR, consoleFd);
    
    close (p10Fd);
	
	
	
	return OK;
		
	
}

