#***************************  M a k e f i l e  *******************************
#
#        $Author: franke $
#          $Date: 1998/03/10 12:05:34 $
#      $Revision: 1.1 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/BK/library.mak,v 1.1 1998/03/10 12:05:34 franke Exp $
#
#    Description: makefile descriptor file for vxWorks 
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.1  1998/03/10 12:05:34  franke
#   Added by mcvs
#
#   Revision 1.1  1998/02/20 15:32:42  uf
#   initial
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=bk

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/bb_defs.h     \
         $(MEN_INC_DIR)/bb_entry.h    \
         $(MEN_VX_INC_DIR)/bk.h       \


MAK_OPTIM=$(OPT_1)

MAK_INP1=bk$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
        $(MAK_INP3)
