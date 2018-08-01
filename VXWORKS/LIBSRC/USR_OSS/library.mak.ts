#***************************  M a k e f i l e  *******************************
#
#        $Author: Franke $
#          $Date: 1999/08/31 10:53:28 $
#      $Revision: 1.2 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/USR_OSS/library.mak,v 1.2 1999/08/31 10:53:28 Franke Exp $
#
#    Description: makefile descriptor file for vxWorks 
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.2  1999/08/31 10:53:28  Franke
#   added CALLBACK, SHAREDMEM, LINEARGS
#
#   Revision 1.1  1998/03/10 12:09:42  franke
#   Added by mcvs
#-----------------------------------------------------------------------------
#   (c) Copyright 1997..1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=usr_oss

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/usr_oss.h     \
         $(MEN_MOD_DIR)/usr_oss_intern.h  \


MAK_OPTIM=$(OPT_1)

MAK_INP1=usr_oss$(INP_SUFFIX)
MAK_INP2=usr_oss_ident$(INP_SUFFIX)
MAK_INP2=usr_oss_dl_list$(INP_SUFFIX)
MAK_INP3=usr_oss_sharedmem$(INP_SUFFIX)
MAK_INP4=usr_oss_errstr$(INP_SUFFIX)
MAK_INP5=usr_oss_key$(INP_SUFFIX)
MAK_INP6=usr_oss_signal$(INP_SUFFIX)
MAK_INP8=usr_oss_random$(INP_SUFFIX)
MAK_INP9=usr_oss_lineargs$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
		$(MAK_INP2) \
		$(MAK_INP3) \
		$(MAK_INP4) \
		$(MAK_INP5) \
		$(MAK_INP6) \
		$(MAK_INP7) \
		$(MAK_INP8) \
		$(MAK_INP9) \

# ts: not building if NO_CALLBACK is defined ???
# MAK_INP7=usr_oss_callback$(INP_SUFFIX)
