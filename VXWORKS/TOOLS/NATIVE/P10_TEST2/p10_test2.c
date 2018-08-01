#include "vxWorks.h"			/* always first */
#include "usrLib.h"
#include "intLib.h"
#include "stdio.h"
#include "string.h"
#include "ioLib.h"
#include "errnoLib.h"
#include "sysLib.h"

#define P10_MANUAL_RTS		0x101	/* manual RTS control */
#define P10_MANUAL_DTR		0x102	/* manual DTR control */ 
#define P10_MANUAL_CTS		0x103	/* manual CTS control */
#define P10_MANUAL_DSR		0x104	/* manual DSR control */
#define P10_MANUAL_DCD		0x105	/* manual DCD control */

#define	SERIAL_BUF_SIZE		1024

#define STD_BAUD			9600

#define HS_RTS				0x01	/* test RTS with CTS */
#define HS_DTR1				0x02	/* test DTR with DCD and DSR */
#define HS_DTR2				0x04	/* test DTR with DCD, RI and DSR */
								    /* note: RI currently not supported */

/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static char outPattern[]="THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG -\
 the quick brown fox jumps over the lazy dog\n";
 

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
static int serXTest( char* outDevName, char* inDevName, UINT8 mode, int hsMode )
{
char	*inBuf=NULL;
int		error;
int		retVal=-1;
int		outFd;
int		inFd=ERROR;
int		numOutBytes;
int		numInBytes=0;

	printf("=== Serial Test RX/TX %s -> %s\n", outDevName, inDevName);

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
		ioctl (outFd, 0x115, 0x10);
		ioctl (inFd, 0x116, 0x40);
	}
	else if (mode == 2)
	{
		ioctl (outFd, 0x115, 0x30);
		ioctl (inFd, 0x116, 0x80);
	}
	
	else if (mode == 0xff)
	{
		ioctl (outFd, 0x117, 0xff);
		ioctl (inFd, 0x117, 0xff);
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


	/*-----------------+
	|  Handshake test  |
	+-----------------*/
	if( hsMode & HS_RTS ){
		int i, status;
		
		printf("=== Handshake Test RTS/CTS %s -> %s\n", outDevName, inDevName);
		for( i=0; i<10; i++ ){
			error = ioctl ( outFd, P10_MANUAL_RTS, i&1 );
			if( error ){
				fprintf(stderr, "\n*** Error setting RTS\n");
				goto CLEANUP;
			}
			taskDelay( 3 );
		
			status = ioctl( inFd, P10_MANUAL_CTS, 0 );

			if( status != (i&1)){
				fprintf(stderr, "\n*** Error verifying CTS level is"
						" %d shouldbe %d\n",
						status, i&1);
				goto CLEANUP;
			}
		}
	}
	if( hsMode & (HS_DTR1 | HS_DTR2) ){
		int i, status;
		
		printf("=== Handshake Test DTR/DCD/DSR %s -> %s\n", 
			   outDevName, inDevName);

		for( i=0; i<10; i++ ){
			error = ioctl ( outFd, P10_MANUAL_DTR, i&1 );
			if( error ){
				fprintf(stderr, "\n*** Error setting DTR\n");
				goto CLEANUP;
			}
			taskDelay( 3 );
		
			status = ioctl( inFd, P10_MANUAL_DCD, 0 );

			if( status != (i&1)){
				fprintf(stderr, "\n*** Error verifying DCD level is"
						" %d shouldbe %d\n",
						status, i&1);
				goto CLEANUP;
			}

			status = ioctl( inFd, P10_MANUAL_DSR, 0 );

			if( status != (i&1)){
				fprintf(stderr, "\n*** Error verifying DSR level is"
						" %d shouldbe %d\n",
						status, i&1);
				goto CLEANUP;
			}
		}
	}
		
	/* say OK */
	retVal = 0;

CLEANUP:
	free( inBuf);
	if(outFd!=ERROR)
		close(outFd);
	if(inFd!=ERROR)
		close(inFd);
	if( !retVal )
		fprintf(stderr, " ==> OK\n");
	else
		fprintf(stderr, " ==> FAILED\n");
	return( retVal );	
}/*serTestRxTx*/


/********************************* p10Test2 *********************************
 *
 *  Description: Check serial lines of P10
 *               
 *				 COM1/COM2 must be connected through nullmodem cable
 *				 COM3/COM4 must be connected through nullmodem cable
 *		
 *				 Handshake lines are also tested 
 *				 (RTS/CTS on all channels, DTR/DSR/DCD on channel 0/1)
 *				 
 *				 RI currently not tested
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  
 *
 *  Globals....:  
 *****************************************************************************/
STATUS p10Test2(char *devName)
{
	int run, status;
	char devBuf[4][60];

	if( devName == NULL )
		devName = "/p10";

	sprintf(devBuf[0], "%s/0", devName );
	sprintf(devBuf[1], "%s/1", devName );
	sprintf(devBuf[2], "%s/2", devName );
	sprintf(devBuf[3], "%s/3", devName );

	printf("========= P10 Test =======\n");

	for(run=0; run<10; run++){
		status = serXTest (devBuf[0], devBuf[1], 0, HS_RTS | HS_DTR1);
		if( status )
			break;
		status = serXTest (devBuf[1], devBuf[0], 0, HS_RTS | HS_DTR1);
		if( status )
			break;
		status = serXTest (devBuf[2], devBuf[3], 0, HS_RTS );
		if( status )
			break;
		status = serXTest (devBuf[3], devBuf[2], 0, HS_RTS );
		if( status )
			break;
	}
	
	return status;	
}


/********************************* ad45SerTest *******************************
 *
 *  Description: Check serial lines of ad45
 *               
 *				 COM1/COM2 must be connected through nullmodem cable
 *				 COM3/COM4 must be connected through nullmodem cable
 *		
 *				 Handshake lines are also tested 
 *				 (RTS/CTS on all channels, DTR/DSR/DCD on channel 0/1)
 *				 
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  
 *
 *  Globals....:  
 *****************************************************************************/
STATUS ad45SerTest(char *devName)
{
	int run, status=0;
	char devBuf[4][60];

	if( devName == NULL )
		devName = "/p10";


	sprintf(devBuf[0], "%s/0", devName );
	sprintf(devBuf[1], "%s/1", devName );
	sprintf(devBuf[2], "%s/2", devName );
	sprintf(devBuf[3], "%s/3", devName );

	printf("========= AD45 Serial Test =======\n");

	for(run=0; run<1; run++){
		status = serXTest (devBuf[0], devBuf[1], 0, HS_RTS | HS_DTR1);
		if( status )
			break;
		status = serXTest (devBuf[1], devBuf[0], 0, HS_RTS | HS_DTR1);
		if( status )
			break;
		status = serXTest (devBuf[2], devBuf[3], 0, HS_RTS );
		if( status )
			break;
		status = serXTest (devBuf[3], devBuf[2], 0, HS_RTS );
		if( status )
			break;
	}
	
	return status;	
}


