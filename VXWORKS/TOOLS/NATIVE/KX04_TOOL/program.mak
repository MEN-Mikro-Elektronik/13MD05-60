#***************************  M a k e f i l e  *******************************
#
#         Author: ag
#          $Date: 2003/07/08 14:36:33 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the KXX Testprogramm
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2003/07/08 14:36:33  ag
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=kx04_tool

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/usr_oss.h   \
         $(MEN_INC_DIR)/usr_utl.h   \


MAK_INP1=kx04_tool$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

