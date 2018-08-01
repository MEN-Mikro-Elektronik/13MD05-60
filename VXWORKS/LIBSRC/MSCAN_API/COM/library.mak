#***************************  M a k e f i l e  *******************************
#  
#         Author: kp
#          $Date: 2003/01/29 14:03:16 $
#      $Revision: 1.1 $
#                      
#    Description: Makefile descriptor file for MSCAN_API lib
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.1  2003/01/29 14:03:16  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

MAK_NAME=mscan_api


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    	\
		 $(MEN_INC_DIR)/mdis_err.h		\
         $(MEN_INC_DIR)/mdis_api.h		\
		 $(MEN_INC_DIR)/mscan_api.h		\
		 $(MEN_INC_DIR)/mscan_drv.h		\


MAK_INP1 = mscan_api$(INP_SUFFIX)
MAK_INP2 = mscan_strings$(INP_SUFFIX)

MAK_INP  = $(MAK_INP1) \
		   $(MAK_INP2)

