/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: memwrite.c
 *      Project: memwrite
 *
 *       Author: ag
 *        $Date: 2002/07/03 15:26:18 $
 *    $Revision: 1.1 $
 *
 *  Description: Memory write function which is callable form tmenu
 *
 *
 *     Required:
 *     Switches: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: memwrite.c,v $
 * Revision 1.1  2002/07/03 15:26:18  agromann
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <vxWorks.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>


#undef VERBOSE

/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


/********************************* main ***************************************
 *
 *  Description: Final inspection test
 *
 *----------------------------------------------------------------------------
 *  Input......:  argc    argument count value
 *                argv    pointer to argument value string
 *
 *  Output.....:  -
 *
 *  Globals....:  OK or ERROR
 *****************************************************************************/
int main( int argc, char *argv[] )
{

	u_int32 addr;
	u_int32 value;
	u_int16 access;
	
	if (argc < 4 || argc > 4) {
		fprintf(stderr, "\n Syntax  : memwrite <addr> <value> <access>\n\n"
						"   addr  :  address to write to (hex)\n"
						"   value :  value to write (hex)\n"
						"   access:  access mode 1=8 bit 2=16 bit 4=32 bit \n");
		return -1;
	}
	
	sscanf( argv[1], "%lx", &addr);
	sscanf( argv[2], "%lx", &value);
	access  = atoi(argv[3]);

	switch (access) {
		case 1:	
			*((u_int8 *) addr) = (u_int8) value;
			break;

		case 2:	
			*((u_int16 *) addr) = (u_int16) value;
			break;

		case 4:	
			*((u_int32 *) addr) = (u_int32) value;
			break;
			
		default:
			fprintf(stderr, "\n *** memwrite: access mode not valid");
			return -1;
						
	}

#ifdef VERBOSE
	printf("\n Wrote 0x%lx to address 0x%08lx, access = %d\n", value, addr, access);
#endif /* VERBOSE */

	return 0;
}
