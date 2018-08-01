#***************************  M a k e f i l e  *******************************
#
#        $Author: ts $
#          $Date: 2011/08/15 18:57:19 $
#      $Revision: 1.4 $
#            $Id: library_pci.mak,v 1.4 2011/08/15 18:57:19 ts Exp $
#
#    Description: makefile for VxWorks OSS lib (with PCI support)
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library_pci.mak,v $
#   Revision 1.4  2011/08/15 18:57:19  ts
#   R: spinlock functions were introduced in MDIS
#   M: added oss_spinlock.c to vxWorks OSS
#
#   Revision 1.3  2006/06/02 11:04:49  ufranke
#   added
#    + oss_vx_kernel_bridge.c
#
#   Revision 1.2  2005/12/23 15:22:22  UFRANKE
#   added
#    + oss_smb
#
#   Revision 1.1  2000/03/16 16:05:21  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1997..2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=oss_pci

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_MOD_DIR)/oss_intern.h  \
		 $(MEN_INC_DIR)/../../NATIVE/MEN/oss_os.h 

MAK_SWITCH=$(SW_PREFIX)PCI $(SW_PREFIX)MAC_MEM_MAPPED

MAK_OPTIM=$(OPT_1)

MAK_INP1=oss$(INP_SUFFIX)
MAK_INP2=oss_dl_list$(INP_SUFFIX)
MAK_INP3=oss_swap$(INP_SUFFIX)
MAK_INP4=oss_task$(INP_SUFFIX)
MAK_INP5=oss_alarm$(INP_SUFFIX)
MAK_INP6=oss_irq$(INP_SUFFIX)
MAK_INP7=oss_sig$(INP_SUFFIX)
MAK_INP8=oss_sem$(INP_SUFFIX)
MAK_INP9=oss_bustoaddr$(INP_SUFFIX)
MAK_INP10=oss_sharedmem$(INP_SUFFIX)
MAK_INP11=oss_callback$(INP_SUFFIX)
MAK_INP12=oss_isa_pnp$(INP_SUFFIX)
MAK_INP13=oss_smb$(INP_SUFFIX)
MAK_INP14=oss_vx_kernel_bridge$(INP_SUFFIX)
MAK_INP15=oss_spinlock$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
        $(MAK_INP3) \
        $(MAK_INP4) \
        $(MAK_INP5) \
        $(MAK_INP6) \
        $(MAK_INP7) \
        $(MAK_INP8) \
        $(MAK_INP9) \
        $(MAK_INP10) \
        $(MAK_INP11) \
        $(MAK_INP12) \
        $(MAK_INP13) \
        $(MAK_INP14) \
        $(MAK_INP15) 
