/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: sertest.c
 *      Project: -
 *
 *       Author: 
 *        $Date: 2012/07/11 13:07:24 $
 *    $Revision: 1.4 $
 *
 *  Description: MEN serial driver test program
 *               adapted from serial_test.c by uf
 *
 *
 *        Note:  
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sertest.c,v $
 * Revision 1.4  2012/07/11 13:07:24  ts
 * R: Compiler warning about uinitialized variable usage
 * M: initialized variables
 *
 * Revision 1.3  2000/03/03 09:25:22  loesel
 * bugfix for command line arguments (argc, argv)
 * save and restore tty option word
 *
 * Revision 1.2  2000/03/02 13:44:22  loesel
 * switch options to RAW to avoid echos
 *
 * Revision 1.1  2000/01/14 15:00:42  loesel
 * Initial Revision
 *
 * 
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char *SerialTestIdentString="$Id: sertest.c,v 1.4 2012/07/11 13:07:24 ts Exp $";

#include <vxWorks.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <errnoLib.h>
#include <ioLib.h>
#include <symLib.h>
#include <sysSymTbl.h>
#include <string.h>
#include <taskLib.h>
#include <tickLib.h>
#include <sysLib.h>
#include <types.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/testutil.h>
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


#define SERIAL_MCR			0x04
#define SERIAL_MCR_RTS		0x02
#define SERIAL_MCR_DTR		0x01

#define SERIAL_MSR			0x06
#define SERIAL_MSR_CTS		0x10
#define SERIAL_MSR_DSR		0x20
#define SERIAL_MSR_RI		0x40
#define SERIAL_MSR_DCD		0x80

#define SERIAL_COM1_BASE	0x3f8 
#define SERIAL_COM2_BASE	0x2f8
#define SERIAL_COM3_BASE	0x3e8
#define SERIAL_COM4_BASE	0x2e8

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
int		optionWord1=0;
int		optionWord2=0;


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
	
	
	/* set options to RAW to avoid echo */
	optionWord1 = ioctl (outFd, FIOGETOPTIONS, 0);
	optionWord2 = ioctl (inFd, FIOGETOPTIONS, 0);
	
	
	(void) ioctl (outFd, FIOSETOPTIONS, OPT_RAW); 
	(void) ioctl (inFd, FIOSETOPTIONS, OPT_RAW); 
	
	
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
	
	(void) ioctl (outFd, FIOSETOPTIONS, optionWord1); 
	(void) ioctl (inFd, FIOSETOPTIONS, optionWord2); 
	
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




int main( int argc, char *argv[] )
{
	/*
	 * get name of the testfile
	 */
	if (argc < 2) goto USAGE;
	
	if (argc < 3) serTestRxTx( argv[1], argv[1] );
	else serTestRxTx( argv[1], argv[2] );
	
	return 0;
		
USAGE:
	fprintf (stderr, "USAGE: sertest [outDevName] [inDevName]\n");
	fprintf (stderr, "EXAMPLE: sertest /tyCo/1 /tyCo/2\n");
	return 0;
	
}

