#***************************  M a k e f i l e  *******************************
#
#         Author: Christian.Schuster@men.de
#          $Date: 2005/01/21 13:36:52 $
#      $Revision: 1.2 $
#
#    Description: makefile descriptor file for z100_oslib
#                 library (needed by FPGA_LOAD tool)
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.2  2005/01/21 13:36:52  cs
#   added switch Z100_IO_MAPPED_EN
#
#   Revision 1.1  2004/12/23 15:10:52  cs
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=z100_oslib

MAK_SWITCH= \
			$(SW_PREFIX)Z100_IO_MAPPED_EN \

MAK_LIBS= \

MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/dbg.h         \
         $(MEN_MOD_DIR)/../COM/fpga_load.h \
         $(MEN_MOD_DIR)/fpga_load_os.h \

MAK_INP1=linux$(INP_SUFFIX)
MAK_INP2=vxworks$(INP_SUFFIX)
MAK_INP3=
MAK_INP4=
MAK_INP5=

MAK_INP=$(MAK_INP1) \
		$(MAK_INP2) \
		$(MAK_INP3) \
		$(MAK_INP4) \
		$(MAK_INP5) \
