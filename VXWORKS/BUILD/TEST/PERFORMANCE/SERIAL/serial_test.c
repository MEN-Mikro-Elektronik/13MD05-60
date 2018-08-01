/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: .c
 *      Project: -
 *
 *       Author: uf
 *        $Date: 1999/10/26 18:33:16 $
 *    $Revision: 1.1 $
 *
 *  Description: MEN driver test program
 *
 *
 *        Note:  
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: serial_test.c,v $
 * Revision 1.1  1999/10/26 18:33:16  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char *SerialTestIdentString="$Id: serial_test.c,v 1.1 1999/10/26 18:33:16 Franke Exp $";

#include <stdio.h>
#include <errnoLib.h>
#include <ioLib.h>
#include <symLib.h>
#include <sysSymTbl.h>
#include <string.h>
#include <taskLib.h>
#include <tickLib.h>
#include <sysLib.h>


/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define	SER_BUF_SIZE	1024

#define I8250_MCR		0x04
#define I8250_MCR_RTS		0x02

#define I8250_MSR		0x06
#define I8250_MSR_CTS		0x10
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/* inputs */
const int SERTEST_INPUTS=	0x20;
#define _SERTEST_CTS		0x21
int SERTEST_CTS=		_SERTEST_CTS;


/* outputs */
const int SERTEST_OUTPUTS=	0x10;

#define _SERTEST_RTS		0x11
int SERTEST_RTS=		_SERTEST_RTS;

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
static char outPattern[]="THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG -\
 the quick brown fox jumps over the lazy dog\n";

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/*****************************  SERTEST_Ident  ****************************
 *
 *  Description:  Shows the men ident string.
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *
 *  Output.....:  -
 *
 *  Globals....:  -
 ****************************************************************************/
void SERTEST_Ident( void )
{
	printf("%s\n", SerialTestIdentString );
}/*SERTEST_Ident*/




int serTestRxTx( char* outDevName, char* inDevName )
{
char	*inBuf=NULL;
int		error;
int		retVal=-1;
int		outFd;
int		inFd=ERROR;
int		numOutBytes;
int		numInBytes=0;

	printf("=== Serial Test RX/TX\n");

	if( !outDevName )
	{
		printf("*** out device name missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	if( !inDevName )
	{
		printf("*** in device name missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	inBuf=malloc( SER_BUF_SIZE );
	if( !inBuf )
	{
		printf("*** Not enough memory (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	outFd = open( outDevName, O_WRONLY, 0 );	
	if( outFd == ERROR )
	{
		printf("*** Can not open out device %s (%s line %d)\n", outDevName, __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	inFd = open( inDevName, O_RDONLY, 0  );	
	if( inFd == ERROR )
	{
		printf("*** Can not open in device %s (%s line %d)\n", inDevName, __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	error = ioctl( outFd, FIOFLUSH, 0 );	
	error |= ioctl( inFd, FIOFLUSH, 0 );
	if( error )
	{
		printf("*** FIOFLUSH failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	numOutBytes = write( outFd, outPattern, sizeof(outPattern) );
	if( numOutBytes != sizeof(outPattern) )
	{
		printf("*** write failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	/* delay a second, should be enough also for 200 baud */
	taskDelay( sysClkRateGet() );

	error |= ioctl( inFd, FIONREAD, (int)&numInBytes );
	if( error )
	{
		printf("*** FIONREAD failed (%s line %d)\n", __FILE__, __LINE__ );
		goto CLEANUP;
	}/*if*/

	if( numInBytes != numOutBytes )
	{
		printf("*** number of received bytes=%d not equal %d number of sent bytes\n",
				numInBytes, numOutBytes );
		goto CLEANUP;
	}/*if*/

	numInBytes = read( inFd, inBuf, numInBytes );
	if( numInBytes != numOutBytes )
	{
		printf("*** number of read bytes=%d not equal %d number of sent bytes\n",
				numInBytes, numOutBytes );
		goto CLEANUP;
	}/*if*/

	/* check pattern */
	error = strncmp( inBuf, outPattern, numOutBytes );
	if( error )
	{
		printf("*** pattern mismatch\n" );
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
		printf(" ==> OK\n");
	else
		printf(" ==> FAILED\n");
	return( retVal );	
}/*serTestRxTx*/



int serTestSetHandshake8250( unsigned char *baseAddr, int line, int value )
{
volatile unsigned char regVal;

	if( !baseAddr )
	{
		printf("*** base address missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	if( line & SERTEST_INPUTS )
	{
		printf("*** handshake line parameter wrong (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/
	
	switch( line )
	{
	  case _SERTEST_RTS:
	  	regVal = *(baseAddr + I8250_MCR);
	  	if( value )
	  		regVal |= I8250_MCR_RTS;
	  	else
	  		regVal &= ~I8250_MCR_RTS;
	  	*(baseAddr + I8250_MCR) = regVal;
		break;

	  default:
		printf("*** handshake line parameter wrong (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*switch*/

	return( 0 );
}/*serTestSetHandshake8250*/

int serTestGetHandshake8250( unsigned char *baseAddr, int line, int *valueP )
{
volatile unsigned char regVal;

	if( !baseAddr )
	{
		printf("*** base address missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	if( line & SERTEST_OUTPUTS )
	{
		printf("*** handshake line parameter wrong (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/
	
	switch( line )
	{
	  case _SERTEST_CTS:
	  	regVal = *(baseAddr + I8250_MSR);
	  	if( regVal & I8250_MSR_CTS )
	  		*valueP = 1;
	  	else
	  		*valueP = 0;
		break;

	  default:
		printf("*** handshake line parameter wrong (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*switch*/

	return( 0 );
}/*serTestSetHandshake8250*/

int serTestHandshake( unsigned char *baseAddr, int outLine, int inLine )
{
int		retVal=-1;
int		outVal;
int		inVal;

	printf("=== Serial Test HANDSHAKE\n");

	if( !baseAddr )
	{
		printf("*** base address missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	/*------------------+
	|	Stuck at high	|
	+------------------*/
	outVal = 0;
	if( serTestSetHandshake8250( baseAddr, outLine, outVal ) )
	{
		goto CLEANUP;
	}/*if*/

	/* delay a bit */
	taskDelay( sysClkRateGet()/100 + 1 );

	if( serTestGetHandshake8250( baseAddr, inLine, &inVal ) )
	{
		goto CLEANUP;
	}/*if*/
	
	if( outVal != inVal )
	{
		printf("*** Stuck at high\n" );
		goto CLEANUP;
	}/*if*/
	
	/*------------------+
	|	Stuck at low 	|
	+------------------*/
	outVal = 1;
	if( serTestSetHandshake8250( baseAddr, outLine, outVal ) )
	{
		goto CLEANUP;
	}/*if*/

	/* delay a bit */
	taskDelay( sysClkRateGet()/100 + 1 );

	if( serTestGetHandshake8250( baseAddr, inLine, &inVal ) )
	{
		goto CLEANUP;
	}/*if*/
	
	if( outVal != inVal )
	{
		printf("*** Stuck at low\n" );
		goto CLEANUP;
	}/*if*/
	


	/* say OK */
	retVal = 0;

CLEANUP:
	if( !retVal )
		printf(" ==> OK\n");
	else
		printf(" ==> FAILED\n");
	return( retVal );	
}/*serTestHandshake*/
