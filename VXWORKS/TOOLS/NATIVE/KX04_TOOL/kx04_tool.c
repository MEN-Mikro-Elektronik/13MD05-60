/****************************************************************************
 ************                                                    ************
 ************                   KX04_TOOL                        ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ag
 *        $Date: 2003/07/08 14:36:31 $
 *    $Revision: 1.1 $
 *
 *  Description: Test tool for KD04/KM04-modules
 *               This tool can be used for HW debugging and simple 
 *               function checking.
 *               
 *                      
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: kx04_tool.c,v $
 * Revision 1.1  2003/07/08 14:36:31  ag
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_utl.h>
#include <MEN/canopen_drv.h>
#include <MEN/CANOPEN/canopen_types.h>
#include "MEN/kxxx_int.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#if defined(_BIG_ENDIAN_)
#elif defined(_LITTLE_ENDIAN_)
#   warning "In this program BIG ENDIAN is assumed unpredictable results may occure"
#else
#	error "Byte ordering is not set, please make sure that either _BIG_ENDIAN_ or _LITTLE_ENDIAN_ is defined"
#endif

#define CHK(expression) \
 if( !(expression)) {\
	 printf("*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
      printf("\n%s\n",M_errstring(UOS_ErrnoGet()));\
     goto abort;\
 }

#define NODE_ID       112 /* /*Selectron: 16 COS 112 */

#define KX04_ERROR_1  (-1)
#define KX04_ERROR_2  (-2)
#define NODE_ID_MASTER 120
#define BAUD_RATE      125  /* /* 1 Mbaud */
#define MAX_NODE_ID    127
#define MIN_NODE_ID    1

#define KX04_VERSION   "$Revision: 1.1 $"

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int8 led[2] = {0xa5, 0xb5};	
static u_int8 inputTestInput16Enable[2] = {0x00, 0x00};
static u_int8 inputSelect = 0;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintHelp( void );
static u_int8 GetHexByte( u_int8 def );
static u_int8 GetDecByte( u_int8 def );
static void PrintError( char *info );
static void DisplayKx04( int32 path );
static int32 SdoRead( int32 path, 
                u_int8 nodeId, 
                u_int16 index, 
                u_int8 sub, 
                u_int32 size, 
                void * dataP );
                
static int32 SdoWrite( int32 path, 
                u_int8 nodeId, 
                u_int16 index, 
                u_int8 sub, 
                u_int32 size, 
                void * dataP );
static int16 StartStack( int32 path, u_int8 masterNodeId, u_int16 baudRate );


static void Usage( void )
{
	printf(" %s\n", KX04_VERSION);
	printf("Syntax: kx04_tool <device> <baudrate> \n");
	printf("Function: Interactive tool for testing KD/KM-Modules\n");
	printf("Options:\n");
	printf("    device       device name\n");
	printf("    baudrate     CAN-Baudrate, allowed values: \n");
	printf("                 10, 20, 50, 125, 250, 500, 800, 1000 \n");
	printf("\n");
	
}

/********************************* main *************************************
 *
 *  Description: Program main function
 *               CANopen Interface to test FW:
 *
 *               Host -> KX04-Module
 *               Ind     Sub   Size   Description
 *               6200    1     8      LED low byte
 *               6200    2     8      LED high byte
 *               6200    3     8      Input select (0-15)
 *               6200    5     8      Control Byte: TS, Inp. 17 en, EEPROM-Test
 *
 *               KX04-Module -> Host
 *               Ind     Sub   Size   Description
 *               6100    1     16     TC, IV, IC, VX_FAIL, ABUD, KD/KM, 
 *                                    EEPROM_ACK
 *               6100    4     16     EEPROM Test Status
 *               6100    5     16     Board Temperature
 *               6000    3     8      CAN address 0-7
 *               6000    4     8      CAN address 8-15
 *               6000    5     8      Connector ID
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/

int main( int argc, char *argv[] )
{

	int32 userInput;
	char * devName;
	int32 path;
	int rv = 0;
	int32 abort = FALSE;
	u_int8 bitDummy;
	u_int16 wordDummy;
	u_int16 baudrate;
	u_int8  nodeIdSlave = NODE_ID;
	
	printf(" ++++ kx04_tool program ++++\n");

	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		Usage();
		return(1);
	}
	
	devName = argv[1];
	baudrate = (u_int16) atoi( argv[2] );

	if( baudrate != 10 &&
	    baudrate != 20 &&
	    baudrate != 50 &&
	    baudrate != 125 &&
	    baudrate != 250 &&
	    baudrate != 500 &&
	    baudrate != 800 &&
	    baudrate != 1000 ) {
	    
	    Usage();
	    return 1;
	}

	printf(" %s\n", KX04_VERSION);
	printf("\nDevice name: %s\n ", devName);
	printf("\nNode-ID of M65: %d\n ", NODE_ID_MASTER);
	printf("\nBaudrate: %d\n ", baudrate);

	/*------------------------------+
	|  Open path to PROFIDP device  |
	+------------------------------*/
	CHK( (path = M_open( devName )) >= 0 );

	if( StartStack( path, NODE_ID_MASTER, baudrate) < 0 ) {
		
		goto abort;
	}

	/*-------------+
	|  Main loop   |
	+-------------*/
	PrintHelp();


	while( abort == FALSE ){

		/* show inputs values */
		DisplayKx04( path );
		
		/* update SDO-Write values */
		SdoWrite( path,
		         NODE_ID, 
		         0x6200, 
		         0x1,
		         sizeof(u_int8), 
		         (void *) &led[0] );

		SdoWrite( path,
		         NODE_ID, 
		         0x6200, 
		         0x2,
		         sizeof(u_int8), 
		         (void *) &led[1] );

		SdoWrite( path,
		         NODE_ID, 
		         0x6200, 
		         0x3,
		         sizeof(u_int8), 
		         (void *) &inputSelect );

		SdoWrite( path,
		         NODE_ID, 
		         0x6200, 
		         0x5,
		         sizeof(u_int8), 
		         (void *) &inputTestInput16Enable[0] );


		/* check for user input */
		/* 
		 * Special handling in VXWORKS: UOS_KeyPressed()
		 * does not work as expected. So user has to enter <CR>
		 * first, then the selection
		 */
		userInput = -1;
		if( UOS_KeyPressed() == '\n' ){
			PrintHelp();
			printf("Select Command: ");
			fflush(stdout);
			userInput = UOS_KeyWait();
		}
		switch( userInput ){
		case 'l':
			
			printf("LED low byte ");
			led[0] = GetHexByte(led[0]);

			printf("LED high byte ");
			led[1] = GetHexByte(led[1]);

			break;

		case 'i':

			do {
				printf("Input select (0-16 dec 255 inputs deselected) ");
				inputSelect = GetDecByte(inputSelect);
				if( inputSelect > KD04_MAX_INPUTS ) {
					printf("*** Value must be between 0 and %02d\n", KD04_MAX_INPUTS);
				}
			} while( inputSelect > KD04_MAX_INPUTS );

			if( inputSelect == 	16 ) {
				inputTestInput16Enable[0] |= INPUT16_EN;
			}
			else {
				inputTestInput16Enable[0] &= ~INPUT16_EN;
			}

			break;

		case 'n':
			do {
				printf("Set Node-ID of Slave to control ");
				nodeIdSlave = GetDecByte(nodeIdSlave);
				if( nodeIdSlave > MAX_NODE_ID || nodeIdSlave < MIN_NODE_ID ) {
					printf("*** Value must be between %d and %d\n", 
					MIN_NODE_ID, MAX_NODE_ID);
				}
			} while( nodeIdSlave > MAX_NODE_ID || nodeIdSlave < MIN_NODE_ID );

			break;

		case 'd':
			printf("\n\n\n++++ All inputs disabled ++++\n\n");
			/* disable inputs 0-15 */
			inputSelect = 0xff;
			/* disable input 16 */
			inputTestInput16Enable[0] &= ~INPUT16_EN;

			break;
		case 's':

			printf("Input test state ");
			if( inputTestInput16Enable[0] & INPUT_TEST ) {
				bitDummy = 1;
			}
			else {
				bitDummy = 0;				
			}
			
			if( GetHexByte( bitDummy ) ) {
				inputTestInput16Enable[0] |= INPUT_TEST;
			}
			else {
				inputTestInput16Enable[0] &= ~INPUT_TEST;	
			}

			break;

		case 'e':
			/* EEPROM test */
			inputTestInput16Enable[0] |= EEPROM_TEST;

			SdoWrite( path,
			         NODE_ID, 
			         0x6200, 
			         0x5,
			         sizeof(u_int8), 
			         (void *) &inputTestInput16Enable[0] );

			do {
				
				UOS_Delay(3000);
				SdoRead( path,
			         NODE_ID, 
			         0x6100, 
			         0x1, 
			         sizeof(u_int16), 
			         (void *) &wordDummy );
		
				
			} while( !(wordDummy & EEPROM_ACK) );

			SdoRead( path,
		         NODE_ID, 
		         0x6100, 
		         0x4, 
		         sizeof(u_int16), 
		         (void *) &wordDummy );
			
			printf("\nEEPROM Test Status = 0x%04x\n\n\n", wordDummy);			
			if( wordDummy ) {
				printf("\n*** EEPROM Test: FAILED ***\n\n\n");

			}
			else {
				printf("\nEEPROM Test: OK\n\n\n");
			}

			inputTestInput16Enable[0] &= ~EEPROM_TEST;
			
			SdoWrite( path,
			         NODE_ID, 
			         0x6200, 
			         0x5,
			         sizeof(u_int8), 
			         (void *) &inputTestInput16Enable[0] );

			break;

		case 't':
			/* Read Temperature */
			SdoRead( path,
		         NODE_ID, 
		         0x6100, 
		         0x5, 
		         sizeof(u_int16), 
		         (void *) &wordDummy );
			
			printf("\nActual temperature from LM75 = %03d°C\n\n\n", wordDummy);

			break;

		case 'a':
			/* exit */
			abort = TRUE;
			printf("\nEXITING...\n");
			break;

		case 'h':	
			PrintHelp();
		} /* switch */
		
		UOS_Delay(300);

	} /* while */

 end:	

	if( path >=0 ) 
		M_close( path );

	return rv;

 abort:
	rv = 1;
	goto end;
}


static void PrintHelp( void )
{
	printf("\n ++++ KD/KM TEST TOOL ++++\n"
		   " %s\n"
		   "Legend:\n"
		   "TC:    TEST_CURRENT\n"
		   "IV:    INPUT_VALUE\n"
		   "IC:    INPUT_CURRENT\n"
		   "VSF:   VS_FAIL\n"
		   "AB:    AUTOBAUD\n"
		   "KD/KM: 1 -> KD04, 0 -> KM04\n"
		   "CAN-adr: Node-ID pins for CAN\n"
		   "Con-ID: Connector ID\n"
		   "Commands:\n"
		   " h: display this help\n"
		   " l: modify LED status\n"
		   " i: select input\n"
		   " d: disable all inputs\n"
		   " s: INPUT_TEST\n"
		   " e: EEPROM Test !!Attention!! destructive test\n"
		   " t: Read Temperature from LM75\n"
		   " n: Set Node-ID of Slave to control\n"
		   " a: abort/exit program \n\n", KX04_VERSION
		  );
}


static void DisplayKx04( int32 path )
{
	u_int16 sdoReadWord;
	u_int8  sdoReadByte;


	if( SdoRead( path,
	         NODE_ID, 
	         0x6100, 
	         0x1, 
	         sizeof(u_int16), 
	         (void *) &sdoReadWord ) == 0 ) {

		if( sdoReadWord & TEST_CURRENT ) {
			printf("TC= 0|");
		}
		else {
			printf("TC= 1|");
		}
	
		if( sdoReadWord & INPUT_VALUE ) {
			printf("IV= 0|");
		}
		else {
			printf("IV= 1|");
		}
	
		if( sdoReadWord & INPUT_CURRENT ) {
			printf("IC= 0|");
		}
		else {
			printf("IC= 1|");
		}
	
		if( sdoReadWord & VS_FAIL ) {
			printf("VSF= 1|");
		}
		else {
			printf("VSF= 0|");
		}
	
		if( sdoReadWord & AUTOBAUD ) {
			printf("AB= 1|");
		}
		else {
			printf("AB= 0|");
		}
	
		if( sdoReadWord & KD_KM ) {
			printf("KD/KM= 1|");
		}
		else {
			printf("KD/KM= 0|");
		}
	} /* if */

	if( SdoRead( path,
	         NODE_ID, 
	         0x6000, 
	         0x3,
	         sizeof(u_int8), 
	         (void *) &sdoReadByte ) == 0 ) {

		printf("CAN adr 0-7= 0x%02x|", sdoReadByte);

	} /* if */

	if( SdoRead( path,
	         NODE_ID, 
	         0x6000, 
	         0x4,
	         sizeof(u_int8), 
	         (void *) &sdoReadByte ) == 0 ) {
	         	
		printf("CAN adr 8-15= 0x%02x|", sdoReadByte);
	} /* if */

	if( SdoRead( path,
	         NODE_ID, 
	         0x6000, 
	         0x5,
	         sizeof(u_int8), 
	         (void *) &sdoReadByte ) == 0 ) {

		printf("Con-ID= 0x%02x", sdoReadByte);


	}			         			         		
	printf("\n");

	return;
}

/********************************* GetHexByte ********************************
 *
 *  Description: Prompt user to input a HEX value
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: def		default input value
 *  Output.....: returns:	actual input value
 *  Globals....: -
 ****************************************************************************/
static u_int8 GetHexByte(u_int8 def)
{
	char buf[20];
	int val;

	while( TRUE ){
		printf("[%02x]: ", def );
		fgets( buf, sizeof(buf), stdin );

		if( buf[0] == '\n' )
			return def;

		if( sscanf( buf, "%x", &val ) == 1 
			&& val >=0  && val < 0x100)

			return val;
		printf("*** Illegal hex value\n");
	}
}

/********************************* GetDecByte ********************************
 *
 *  Description: Prompt user to input a DEC value
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: def		default input value
 *  Output.....: returns:	actual input value
 *  Globals....: -
 ****************************************************************************/
static u_int8 GetDecByte(u_int8 def)
{
	char buf[20];
	int val;

	while( TRUE ){
		printf("[%02d]: ", def );
		fgets( buf, sizeof(buf), stdin );

		if( buf[0] == '\n' )
			return def;

		if( sscanf( buf, "%d", &val ) == 1 )

			return val;
		printf("*** Illegal dec value\n");
	}
}


/********************************* SdoWrite **********************************
 *
 *  Description: Perform a SDO-Write access
 *
 *               The follwing error values are returned:
 *               KX04_ERROR_1   error alocating SDO-Write buffer
 *               KX04_ERROR_2   fatal error during SDO-Write
 *               Furthermore a specific SDO-Error Value is returned
 *			   
 *---------------------------------------------------------------------------
 *  Input......: path			path to CANOPEN device
 *               nodeId			ID of node to access
 *				 index          index to access
 *               sub            sub-index to access
 *               size           data size to write
 *               dataP          pointer to data
 *  Output.....: returns:		0 or neg. error code
 *  Globals....: -
 ****************************************************************************/
static int32 SdoWrite( int32 path, 
                u_int8 nodeId, 
                u_int16 index, 
                u_int8 sub, 
                u_int32 size, 
                void * dataP )
{
	
	M_SG_BLOCK               blk;
	CANOPEN_SDO_WRITE_STRUCT *sdoWriteStruct;
	u_int8                   *sdoWriteData;
	u_int8                   *sdoWriteBuff;
	u_int32                  sdoErrorCode = 0;
	int32                    error = 0;

	
	sdoWriteBuff = malloc(sizeof(CANOPEN_SDO_WRITE_STRUCT) + 
	                     (size * sizeof(u_int8)));

	                     
	if (sdoWriteBuff == NULL) {
		PrintError("allocate memory");
		error = KX04_ERROR_1;
		goto abort;
	}
		
	sdoWriteStruct = (CANOPEN_SDO_WRITE_STRUCT*) sdoWriteBuff;
	sdoWriteData   = (u_int8*) (sdoWriteStruct + 1);

	/* copy write data to SDO-Write buffer */
	memcpy( sdoWriteData, dataP, size );


	/* prepare SDO write sturcture */
	sdoWriteStruct->nodeId   = (u_int8)  nodeId;    /* node ID */
	sdoWriteStruct->index    = (u_int16) index;     /* index */
	sdoWriteStruct->subIndex = (u_int8)  sub;       /* sub index */
	sdoWriteStruct->length   = (u_int32) size;      /* data length */
	sdoWriteStruct->useBlock = (u_int8)  0;         /* use block transmission mode */
	sdoWriteStruct->ackTrans = (u_int8)  0;         /* acklnowledge transfer via evtent queue */


	/* assign set stat variable */
	blk.data = (void *) sdoWriteBuff;
	blk.size = (sizeof(CANOPEN_SDO_WRITE_STRUCT) + (size * sizeof(u_int8)));

	/* initate SDO write transfer */
	if ((M_getstat(path, CANOPEN_BLK_SDO_WRITE, (int32*) &blk)) < 0) {
		PrintError("setstat CANOPEN_BLK_SDO_WRITE");
		error = KX04_ERROR_2;
		goto abort;
	}
	
	/* read error code of previous transfer */
	sdoErrorCode = *((u_int32*) blk.data);

	if (sdoErrorCode) {
		printf("*** Error during SDO transfer\n");
		error = (int32) sdoErrorCode;
		goto abort;
	}

abort:
	free ((void*) sdoWriteBuff); /* free memory */
	return error;	
}

/********************************* SdoRead ***********************************
 *
 *  Description: Perform a SDO-Read access
 *			   
 *               Used error codes:
 *               KX04_ERROR_1   error alocating SDO-Read buffer
 *               KX04_ERROR_2   fatal error during SDO-Read
 *               Furthermore a specific SDO-Error Value is returned
 *			   
 *---------------------------------------------------------------------------
 *  Input......: path			path to CANOPEN device
 *               nodeId			ID of node to access
 *				 index          index to access
 *               sub            sub-index to access
 *               size           buffer size
 *               buffP          pointer to data buffer
 *  Output.....: returns:		0 or neg. error code
 *  Globals....: -
 ****************************************************************************/
static int32 SdoRead( int32 path, 
                u_int8 nodeId, 
                u_int16 index, 
                u_int8 sub, 
                u_int32 size, 
                void * dataP )
{
	
	M_SG_BLOCK               blk;
	CANOPEN_SDO_READ_STRUCT  *sdoReadStruct;
	u_int8                   *sdoReadData;
	u_int8                   *sdoReadBuff;
	u_int32                  sdoErrorCode = 0;
	int32                    error = 0;

	
	/* assign block structure */
	sdoReadBuff = malloc(sizeof(CANOPEN_SDO_READ_STRUCT) + 
	                    ( size * sizeof(u_int8)));
	                    
	if (sdoReadBuff == NULL) {
		PrintError("allocate memory");
		error = KX04_ERROR_1;
		goto abort;
	}
		
	sdoReadStruct = (CANOPEN_SDO_READ_STRUCT*) sdoReadBuff;
	sdoReadData   = (u_int8*) (sdoReadStruct + 1);

	/* prepare SDO read sturcture */
	sdoReadStruct->nodeId   = (u_int8)  nodeId;
	sdoReadStruct->index    = (u_int16) index;
	sdoReadStruct->subIndex = (u_int8)  sub;
	sdoReadStruct->useBlock = (u_int8)  0;
	sdoReadStruct->ackTrans = (u_int8)  0;

	/* assign get stat variable */
	blk.data = (void *) sdoReadStruct;
	blk.size = (sizeof(CANOPEN_SDO_READ_STRUCT) + ( size * sizeof(u_int8)));

	/* initiate SDO read transfer and check for general error */
	if ((M_getstat(path, CANOPEN_BLK_SDO_READ, (int32*) &blk)) < 0) {
		PrintError("setstat CANOPEN_BLK_SDO_READ");
		error = KX04_ERROR_2;
		goto abort;
	}	
	
	/* check for specific SDO error */
	sdoErrorCode = *((u_int32*) blk.data);
	if (sdoErrorCode) {
		printf("\n*** Error SDO-Read, error code = %08lx\n", sdoErrorCode);
		error = (int32) sdoErrorCode;
		goto abort;
	}

	/* copy read data to user buffer */
	memcpy( dataP, sdoReadData, size );

abort:
	free ((void*) sdoReadBuff); /* free memory */
	return error;
}

/********************************* StartStack ********************************
 *
 *  Description: Start CANopen Stack
 *			   
 *               Used error codes:
 *               KX04_ERROR_1   serious error during start of stack
 *			   
 *---------------------------------------------------------------------------
 *  Input......: path           path to CANOPEN device
 *               masterNodeId	node ID of master
 *               baudRate		CAN bus baudrate
 *
 *  Output.....: returns:		0 or neg. error code
 *  Globals....: -
 ****************************************************************************/
static int16 StartStack( int32 path, u_int8 masterNodeId, u_int16 baudRate )
{
	M_SG_BLOCK      blk;
	CANOPEN_START_STACK_STRUCT startParam;
	int16 error = 0;
	
	/* start stack */
	startParam.baudRate = baudRate;      /* CAN-bus baudrate */
	startParam.nodeId   = masterNodeId;  /* node ID of CANopen master */

	blk.data = (void *) &startParam;
	blk.size = (sizeof(CANOPEN_START_STACK_STRUCT));

	if ((M_setstat(path, CANOPEN_BLK_START_STACK, (int32) &blk)) < 0) {
		PrintError("setstat CANOPEN_BLK_START_STACK");
		error = KX04_ERROR_1;
	}
	return error;
}

/********************************* PrintError *******************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}
