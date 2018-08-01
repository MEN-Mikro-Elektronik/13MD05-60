/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: desc_test.c
 *      Project: -
 *
 *       Author: uf
 *        $Date: 2001/12/10 16:47:40 $
 *    $Revision: 1.1 $
 *
 *  Description: This is test for descriptor generator and DESC decoder lib.
 *               It checks the whole chain of Host tool and target lib:
 *                           Big/Little endian and byte padding.
 *        Note:  -
 *
 *     Required:  Following meta descriptor:

# TEST: value types and sub-pathes
MAIN_DIR {
    DESC_TYPE       = U_INT32       1
    HW_TYPE         = STRING        DRV
    U_INT32_VAL     = U_INT32       0x87654321

    SUB1_A {
             U_INT32_VAL     = U_INT32       0
             STRING_VAL      = STRING        veni_vidi_vice
             BINARY_VAL      = BINARY        0x01
    }
    
    SUB1_B {
             U_INT32_VAL     = U_INT32       0xFFFFFFFF
             STRING_VAL      = STRING        erare_humanum_est
             BINARY_VAL      = BINARY        0x01,0x02
	    
	    SUB2_B {
    	        U_INT32_VAL     = U_INT32       0xaffedead
        	    STRING_VAL      = STRING        alea_jacta_est
    	        BINARY_VAL      = BINARY        0x01,0x02,0x30
    	        BINARY_VAL_B    = BINARY        0x01,0x02,0x30
        }
    }

    U_INT32_VAL_R   = U_INT32       0x87654321
    STRING_VAL      = STRING        StRiNgTeSt    # -> StRiNgTeSt
    
    SUB1_C {                                                                                
             U_INT32_VAL     = U_INT32       0xdeadbeef
             STRING_VAL      = STRING        VENI-VIDI-VICI
             BINARY_VAL      = BINARY        0x01,0x02,0x30,0x40
	                                                                                        
	    SUB2_C {                                                                            
    	        U_INT32_VAL     = U_INT32       0x41
        	    STRING_VAL      = STRING        ERARE_HUMANUM_EST
    	        BINARY_VAL      = BINARY        0x01,0x02,0x30,0x40,0x55
       
            SUB3_C {
                    U_INT32_VAL     = U_INT32       0x55443322
                    STRING_VAL      = STRING        ALEA_JACT_EST
                    BINARY_VAL      = BINARY        0x01,0x02,0x30,0x40,0x55,0x66
            }
        }    
    }
    BINARY_VAL      = BINARY        0x01,0x02,0x30,0x40,0x55,0x66,0xaa,0xBB,0xcc,0xFF
}


 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: desc_test.c,v $
 * Revision 1.1  2001/12/10 16:47:40  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2001 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char DescTestRCSid[]="$Id: desc_test.c,v 1.1 2001/12/10 16:47:40 Franke Exp $";

#include "vxWorks.h"
#include "taskLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#define VXWORKS
#include <MEN/mdis_err.h>       /* descriptor functions           */
#include <MEN/oss.h>       /* descriptor functions           */
#include <MEN/desc.h>       /* descriptor functions           */


#define CLEANUP_MCR					{ line = __LINE__; goto CLEANUP; }

const u_int32 UINT32_VAL_R		=   0x87654321;
const u_int32 DEFAULT_UINT32	=	0x77665544;
const unsigned char DEFAULT_BINARY[] =	{0x77,0x66,0x55,0x44,0x33,0x22,0x11,0xff};
const unsigned char BINARY_VAL_B[]   =  {0x01,0x02,0x30};

int desc_test( int verbose )
{
	int line=0;
	u_int32 len, val;
	int retCode;
    DESC_HANDLE *descHdl;
    unsigned char binBuf[0x200];

	printf("%s\n", (char*) DescTestRCSid );

    if( DESC_Init( (void*) &MAIN_DIR, NULL, &descHdl ))
		CLEANUP_MCR

	/*not found*/
	len = sizeof(binBuf);
    retCode = DESC_GetBinary( descHdl, 
    						  (unsigned char*)DEFAULT_BINARY, sizeof(DEFAULT_BINARY),
    						  binBuf, &len,
                              "BINARY_VAL_B" );
	if( retCode != ERR_DESC_KEY_NOTFOUND
	    || len  != sizeof(DEFAULT_BINARY)
	    || bcmp( binBuf, (char*)DEFAULT_BINARY, sizeof(DEFAULT_BINARY) )
	  )
		CLEANUP_MCR


	/*found*/
	len = sizeof(binBuf);
    retCode = DESC_GetBinary( descHdl, 
    						  (unsigned char*)DEFAULT_BINARY, sizeof(DEFAULT_BINARY),
    						  binBuf, &len,
                              "SUB1_B/SUB2_B/BINARY_VAL_B" );
	if( retCode != ERR_SUCCESS
	    || len  != sizeof(BINARY_VAL_B)
	    || bcmp( binBuf, (char*)BINARY_VAL_B, sizeof(BINARY_VAL_B) )
	  )
		CLEANUP_MCR

	/*not found*/
    retCode = DESC_GetUInt32( descHdl,
                              DEFAULT_UINT32,
                              &val,
                              "SUB1_A/U_INT32_VAL_R" );
	if( retCode != ERR_DESC_KEY_NOTFOUND
	    || val  != DEFAULT_UINT32
	  )
		CLEANUP_MCR

	/*found*/
    retCode = DESC_GetUInt32( descHdl,
                              DEFAULT_UINT32,
                              &val,
                              "U_INT32_VAL_R" );
	if( retCode != ERR_SUCCESS
	    || val  != UINT32_VAL_R
	  )
		CLEANUP_MCR



    DESC_Exit( &descHdl );
	return( 0 );	

CLEANUP:
	printf("*** ERROR: line %d\n", line );
    DESC_Exit( &descHdl );
	return( -1 );	
}/*desc_test*/

