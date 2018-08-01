/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: shaketest.c
 *      Project: -
 *
 *       Author: 
 *        $Date: 2000/03/10 13:24:21 $
 *    $Revision: 1.5 $
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
 * $Log: shaketest.c,v $
 * Revision 1.5  2000/03/10 13:24:21  loesel
 * cosmetics
 *
 * Revision 1.4  2000/03/08 10:19:57  loesel
 * modified so it runs with sertest
 *
 * Revision 1.3  2000/03/03 09:31:06  loesel
 * bugfix in command line arguments (argc, argv)
 *
 * Revision 1.2  2000/03/02 08:37:01  loesel
 * added check of all lines
 *
 * Revision 1.1  2000/03/01 17:37:40  loesel
 * Initial Revision
 *
 * Revision 1.1  2000/01/14 15:00:42  loesel
 * Initial Revision
 *
 * 
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char *ShakeTestIdentString="$Id: shaketest.c,v 1.5 2000/03/10 13:24:21 loesel Exp $";

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

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/*****************************  SHAKETEST_Ident  ****************************
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
void SHAKETEST_Ident( void )
{
	printf("%s\n", ShakeTestIdentString );
}/*SERTEST_Ident*/


/******************************** ser_atox ***********************************
 *
 *  Description: ASCII to hex converter
 *
 *----------------------------------------------------------------------------
 *  Input......:  str           the ASCII string of the hex number
 *
 *  Output.....:  Return 		the converted number
 *
 *  Globals....:
 *****************************************************************************/
UINT32 ser_atox(char *str)
{
    UINT32 l;
    
    sscanf(str,"%x",(unsigned int*)&l);
    return(l);
}






int serXTestHandshake( int baseAddr1, int baseAddr2 )
{
	volatile unsigned char regVal;
		
	
	
	if( !baseAddr1 || !baseAddr2 )
	{
		fprintf(stderr, "*** base addresses missing (%s line %d)\n", __FILE__, __LINE__ );
		return( -1 );
	}/*if*/

	
	/* clearing RTS + DTR line */
	fprintf (stderr, "\nClearing RTS + DTR low:\n");
	regVal = sysInByte(baseAddr1 + SERIAL_MCR);
	regVal &= ~SERIAL_MCR_RTS;
	regVal &= ~SERIAL_MCR_DTR;
	sysOutByte (baseAddr1 + SERIAL_MCR, regVal);
	
	/* delay a bit */
	taskDelay( 1 );
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_CTS )
		fprintf (stderr, "CTS => high\n");
	else fprintf (stderr, "CTS => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DCD )
		fprintf (stderr, "DCD => high\n");
	else fprintf (stderr, "DCD => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DSR )
		fprintf (stderr, "DSR => high\n");
	else fprintf (stderr, "DSR => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_RI )
		fprintf (stderr, "RI => high\n");
	else fprintf (stderr, "RI => low\n");
	
	
	
	/* setting RTS line */
	fprintf (stderr, "\nStuck RTS high:\n");
	regVal = sysInByte(baseAddr1 + SERIAL_MCR);
	regVal |= SERIAL_MCR_RTS;
	regVal &= ~SERIAL_MCR_DTR;
	sysOutByte (baseAddr1 + SERIAL_MCR, regVal);
	
	/* delay a bit */
	taskDelay( 1 );
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_CTS )
		fprintf (stderr, "CTS => high\n");
	else fprintf (stderr, "CTS => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DCD )
		fprintf (stderr, "DCD => high\n");
	else fprintf (stderr, "DCD => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DSR )
		fprintf (stderr, "DSR => high\n");
	else fprintf (stderr, "DSR => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_RI )
		fprintf (stderr, "RI => high\n");
	else fprintf (stderr, "RI => low\n");
	
	
	/* setting DTR line */
	fprintf (stderr, "\nStuck DTR high:\n");
	regVal = sysInByte(baseAddr1 + SERIAL_MCR);
	regVal &= ~SERIAL_MCR_RTS;
	regVal |= SERIAL_MCR_DTR;
	sysOutByte (baseAddr1 + SERIAL_MCR, regVal);
	
	/* delay a bit */
	taskDelay( 1 );
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_CTS )
		fprintf (stderr, "CTS => high\n");
	else fprintf (stderr, "CTS => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DCD )
		fprintf (stderr, "DCD => high\n");
	else fprintf (stderr, "DCD => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_DSR )
		fprintf (stderr, "DSR => high\n");
	else fprintf (stderr, "DSR => low\n");
	
	if ( sysInByte (baseAddr2 + SERIAL_MSR) & SERIAL_MSR_RI )
		fprintf (stderr, "RI => high\n");
	else fprintf (stderr, "RI => low\n");
	
	
	/* setting RTS + DTR line */
	regVal = sysInByte(baseAddr1 + SERIAL_MCR);
	regVal |= SERIAL_MCR_RTS;
	regVal |= SERIAL_MCR_DTR;
	sysOutByte (baseAddr1 + SERIAL_MCR, regVal);

	
	return( 0 );
}

int main( int argc, char *argv[] )
{
	UINT32	base1;
	UINT32	base2;	
	/*
	 * get name of the testfile
	 */
	
	if (argc < 2) goto USAGE;	
	
	else if (argc < 3) 
	{
		base1 = (int)ser_atox (argv[1]);
		serXTestHandshake( base1, base1 );

	}
	else if (argc < 4) 
	{
		base1 = (int)ser_atox (argv[1]);
		base2 = (int)ser_atox (argv[2]);
		serXTestHandshake( base1, base2 );
	}
	
	else goto USAGE;
	
		
	return 0;
		
USAGE:
	fprintf (stderr, "USAGE: shaketest [baseAddress] [baseAddress]\n");
	fprintf (stderr, "EXAMPLE: shaketest 0x3f8 0x2f8\n");
	return 0;
	
}
