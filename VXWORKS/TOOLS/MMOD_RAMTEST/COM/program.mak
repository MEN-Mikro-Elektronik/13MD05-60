#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2014/05/23 18:19:09 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the PROFIDP example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2014/05/23 18:19:09  ts
#   Initial Revision
#
#   Revision 1.1  2014/05/15 12:54:44  ts
#   Initial Revision
#
#   Revision 1.3  2000/06/15 15:08:10  gromann
#   Replaced dp_config.h with dp_config_simp.h
#
#   Revision 1.2  2000/03/09 09:59:06  Gromann
#   Changes for Win NT
#
#   Revision 1.1  2000/02/29 12:05:20  Gromann
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mmod_ramtest

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h   \
		 $(MEN_INC_DIR)/usr_utl.h 		          

MAK_INP1=mmod_ramtest$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
