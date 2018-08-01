#**************************  M a k e f i l e ********************************
#
#         Author: kp
#          $Date: 2007/06/18 09:20:04 $
#      $Revision: 1.6 $
#
#    Description: makefile to build sysparam2 library
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.6  2007/06/18 09:20:04  aw
#   added sysp_em09
#
#   Revision 1.5  2006/08/18 09:17:09  ub
#   Added sysp_em03.c
#
#   Revision 1.4  2005/02/22 15:00:50  kp
#   temporarily removed EM04A
#   removed sysp_paramhelp (not compilable under OS-9)
#
#   Revision 1.3  2005/01/12 14:19:44  kp
#   added more files
#
#   Revision 1.2  2004/01/26 14:05:06  kp
#   added sysp_core
#
#   Revision 1.1  2004/01/08 09:06:44  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2004 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************


MAK_NAME=sysparam2

MAK_LIBS=

MAK_INCL=$(MEN_INC_DIR)/men_typs.h \
		 $(MEN_INC_DIR)/sysparam2.h \
		 $(MEN_INC_DIR)/em04_eeprom.h \
		 $(MEN_INC_DIR)/em04a_eeprom.h \
		 $(MEN_INC_DIR)/ad65_eeprom.h \
		 $(MEN_INC_DIR)/eeprod.h \
		 $(MEN_MOD_DIR)/sysp_intern.h \
		 $(MEN_MOD_DIR)/sysp_sysdep.h \


MAK_SWITCH=$(SW_PREFIX)SYSP_CONFIG_MDIS_BUILD\
           #$(SW_PREFIX)SYSP_CONFIG_UNITTEST

MAK_INP1	=	sysp_api$(INP_SUFFIX)
MAK_INP2	=	sysp_apiex$(INP_SUFFIX)
MAK_INP3	=	sysp_eeprod$(INP_SUFFIX)
MAK_INP4	=	sysp_em04$(INP_SUFFIX)
MAK_INP5	=   sysp_tags_esm_cb$(INP_SUFFIX)
MAK_INP6	=	sysp_esm_cb$(INP_SUFFIX)
MAK_INP7	=	sysp_mmpara$(INP_SUFFIX)
MAK_INP8	=	sysp_mpgrp$(INP_SUFFIX)
MAK_INP9	=	sysp_strgrp$(INP_SUFFIX)
MAK_INP10	=	sysp_vxbline$(INP_SUFFIX)
MAK_INP11	=	sysp_tags$(INP_SUFFIX)
MAK_INP12	=	sysp_tags_mpc85xx$(INP_SUFFIX)
MAK_INP13	=   sysp_core$(INP_SUFFIX)
MAK_INP14	=	sysp_eeprod2$(INP_SUFFIX)
MAK_INP15	=	sysp_em03$(INP_SUFFIX)
MAK_INP16	=	#sysp_paramhelp$(INP_SUFFIX)
MAK_INP17   =   sysp_em09$(INP_SUFFIX)

MAK_INP		=	$(MAK_INP1) $(MAK_INP2) $(MAK_INP3) $(MAK_INP4) $(MAK_INP5) \
				$(MAK_INP6) $(MAK_INP7) $(MAK_INP8) $(MAK_INP9) $(MAK_INP10)\
				$(MAK_INP11) $(MAK_INP12) $(MAK_INP13) $(MAK_INP14) $(MAK_INP15) \
				$(MAK_INP16) $(MAK_INP17)\
