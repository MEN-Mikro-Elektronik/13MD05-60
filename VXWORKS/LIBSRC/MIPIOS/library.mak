#***************************  M a k e f i l e  *******************************
#
#         Author: uf
#          $Date: 2009/03/31 10:46:35 $
#      $Revision: 1.2 $
#            $Id: library.mak,v 1.2 2009/03/31 10:46:35 ufranke Exp $
#
#    Description: makefile for VxWorks MK library (without PCI support)
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.2  2009/03/31 10:46:35  ufranke
#   changed
#    - compile mipios_vx_api.c only
#
#   Revision 1.1  2009/01/30 09:20:59  ufranke
#   checkin at project freeze
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2008..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mipios

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/mdis_ers.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_com.h    			\
         $(MEN_INC_DIR)/mipios.h    			\
         $(MEN_VX_INC_DIR)/mipios_vx_api.h		\
         $(MEN_VX_INC_DIR)/os2m.h				\
         $(MEN_MOD_DIR)/mipios_communication.h	\
         $(MEN_MOD_DIR)/mipios_line_manager.h	\


MAK_OPTIM=$(OPT_1)

MAK_INP1=mipios_vx_api$(INP_SUFFIX)
# MAK_INP2=mipios_line_manager$(INP_SUFFIX)
# MAK_INP3=mipios_communication$(INP_SUFFIX)
# MAK_INP4=mipios_crc32$(INP_SUFFIX)
# MAK_INP5=mipios_show_socket_options$(INP_SUFFIX)
# MAK_INP6=mipios_oss_vx$(INP_SUFFIX)
# MAK_INP7=mipios_signal$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
        $(MAK_INP3) \
        $(MAK_INP4) \
        $(MAK_INP5) \
        $(MAK_INP6) \
        $(MAK_INP7) \
