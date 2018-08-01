#***************************  M a k e f i l e  *******************************
#
#         Author: aw
#          $Date: 2009/03/06 17:07:59 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for GC testsw
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2009/03/06 17:07:59  AWanka
#   R: For the qualification test a CAN test and Com 2 test was needed.
#   M: Added GC_CanTestA_07320 and GC_Com2Test
#
#   Revision 1.1  2009/01/22 15:32:43  AWanka
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=gc_test_sw

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h	  \
         $(MEN_INC_DIR)/usr_oss.h	  \
         $(MEN_INC_DIR)/usr_utl.h

MAK_INP1=gc_test_sw$(INP_SUFFIX)
MAK_INP2=looptest$(INP_SUFFIX)
MAK_INP3=can_test$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)\
		$(MAK_INP2)\
		$(MAK_INP3)\





