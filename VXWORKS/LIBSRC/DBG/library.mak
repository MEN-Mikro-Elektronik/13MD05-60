#***************************  M a k e f i l e  *******************************
#
#         Author: uf      
#          $Date: 1998/06/17 14:32:04 $
#      $Revision: 1.1 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/DBG/library.mak,v 1.1 1998/06/17 14:32:04 Franke Exp $
#
#    Description: makefile descriptor file for vxWorks 
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.1  1998/06/17 14:32:04  Franke
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=dbg

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/dbg.h         \


MAK_OPTIM=$(OPT_1)

MAK_INP1=dbg$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
