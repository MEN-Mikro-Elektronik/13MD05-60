#***************************  M a k e f i l e  *******************************
#
#         Author: rt
#          $Date: 2007/02/28 13:22:44 $
#      $Revision: 1.1 $
#
#    Description: linux makefile descriptor file for fpga_load
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2007/02/28 13:22:44  rtrübenbach
#   Initial Revision
#
#
#	(cloned from fpga_load program_sw.mak V2.5)
#-----------------------------------------------------------------------------
# (c) Copyright 2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************

MAK_NAME=pciboot

MAK_SWITCH= \
            $(SW_PREFIX)ONE_NAMESPACE_PER_TOOL\
            $(SW_PREFIX)MAC_USERSPACE\
            $(SW_PREFIX)SMB2_API
#            $(SW_PREFIX)DBG\

MAK_LIBS= 	$(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)			\
			$(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)			\
        	$(LIB_PREFIX)$(MEN_LIB_DIR)/oss_usr$(LIB_SUFFIX)			\
        	$(LIB_PREFIX)$(MEN_LIB_DIR)/dbg_usr$(LIB_SUFFIX)			\
        	$(LIB_PREFIX)$(MEN_LIB_DIR)/Advapi32$(LIB_SUFFIX)

# for LINUX:
#MAK_LIBS+=	$(LIB_PREFIX)$(ELINOS_PROJECT)/src/pciutils-2.1.11/lib/libpci$(LIB_SUFFIX)		\
##			$(LIB_PREFIX)$(MEN_LIB_DIR)/vme4l_api$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/men_typs.h \
		 $(MEN_INC_DIR)/usr_oss.h \
		 $(MEN_INC_DIR)/usr_err.h \
		 $(MEN_INC_DIR)/usr_utl.h \
         $(MEN_MOD_DIR)/../COM/pciboot.h \

MAK_INP1=pciboot$(INP_SUFFIX)
MAK_INP2=pciboot_doc$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)\
		$(MAK_INP2)\

