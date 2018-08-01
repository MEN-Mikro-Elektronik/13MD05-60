#***************************  M a k e f i l e  *******************************
#
#        $Author: ts $
#          $Date: 2011/08/15 18:57:05 $
#      $Revision: 1.7 $
#            $Id: library.mak,v 1.7 2011/08/15 18:57:05 ts Exp $
#
#    Description: makefile for VxWorks OSS lib (without PCI support)
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.7  2011/08/15 18:57:05  ts
#   R: spinlock functions were introduced in MDIS
#   M: added oss_spinlock.c to vxWorks OSS
#
#   Revision 1.6  2006/06/02 11:04:47  ufranke
#   added
#    + oss_vx_kernel_bridge.c
#
#   Revision 1.5  2005/12/23 15:22:20  UFRANKE
#   added
#    + oss_smb
#
#   Revision 1.4  2000/03/16 16:05:19  kp
#   added dependency to oss_os.h
#
#   Revision 1.3  1999/08/30 11:03:15  Franke
#   added new modules shared mem, callback, isa pnp
#
#   Revision 1.2  1999/05/05 11:11:05  Franke
#   	added oss_swap, oss_task
#
#   Revision 1.1  1998/03/10 12:08:03  franke
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1997..1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=oss

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_MOD_DIR)/oss_intern.h  \
		 $(MEN_INC_DIR)/../../NATIVE/MEN/oss_os.h 


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

