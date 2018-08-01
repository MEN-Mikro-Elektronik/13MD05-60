#************************** MDIS4 test descriptor ***************************
#
#        Author: uf
#         $Date: 2001/12/10 16:47:42 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for test of 
#                  1. descgen host tool 
#                  2. DESC Library
#
#****************************************************************************

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
