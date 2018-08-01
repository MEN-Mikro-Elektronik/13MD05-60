#***************************  M a k e f i l e  *******************************
#
#        $Author: kp $
#          $Date: 2000/03/17 15:07:45 $
#      $Revision: 2.1 $
#            $Id: library_pci.mak,v 2.1 2000/03/17 15:07:45 kp Exp $
#
#    Description: makefile for VxWorks MK library (with PCI support)
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library_pci.mak,v $
#   Revision 2.1  2000/03/17 15:07:45  kp
#   Initial Revision
#
#   Revision 1.1  1998/03/10 12:21:56  franke
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1997 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mk_pci

MAK_LIBS=

MAK_SWITCH=$(SW_PREFIX)PCI $(SW_PREFIX)MAC_MEM_MAPPED

MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/mdis_ers.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/maccess.h     \
         $(MEN_INC_DIR)/ll_defs.h     \
         $(MEN_INC_DIR)/ll_entry.h    \
         $(MEN_INC_DIR)/bb_defs.h     \
         $(MEN_INC_DIR)/bb_entry.h    \
         $(MEN_INC_DIR)/mbuf.h        \
         $(MEN_INC_DIR)/mdis_com.h    \
         $(MEN_VX_INC_DIR)/mk.h       \
         $(MEN_VX_INC_DIR)/bk.h       \
         $(MEN_VX_INC_DIR)/os2m.h


MAK_OPTIM=$(OPT_1)

MAK_INP1=os2m$(INP_SUFFIX)
MAK_INP2=mk$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2)  
