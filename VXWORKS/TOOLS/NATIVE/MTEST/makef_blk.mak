#***************************  M a k e f i l e  *******************************
#
#         Author: dp
#          $Date: 2006/09/11 10:51:41 $
#      $Revision: 1.2 $
#
#    Description: makefile for mtest - VMEBLT variant
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef_blk.mak,v $
#   Revision 1.2  2006/09/11 10:51:41  cs
#   added support for VxW6.x
#
#   Revision 1.1  2005/11/28 12:53:25  dpfeuffer
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

#**************************************
#   define used tools
#
TGT_DIR=$(WIND_BASE)/target

ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
    MEN_VXVERSUB_DIR=
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=/$(WIND_PLATFORM)
endif

COMPILER=$(CC) -Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU) -DVMEBLT=1
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=mtest #-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN$(MEN_VXVERSUB_DIR)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_VX_DIR)/LIB/MEN$(MEN_VXVERSUB_DIR)/lib$(CPU)$(TOOL)$(DBGDIR)

#**************************************
#   input directories
#

MEN_INC_DIR=$(MEN_VX_DIR)/INCLUDE


#**************************************
#   include pathes
#
INC=-I$(MEN_INC_DIR)/COM     \
    -I$(MEN_INC_DIR)/NATIVE  \
    -I$(WIND_BASE)/target    \
    -I$(WIND_BASE)/target/h  \
    -I.

all: $(OBJ_DIR)/mtestblk.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/mtestblk.o:  mtest2.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
					$(MEN_INC_DIR)/NATIVE/MEN/sysMenPci2Vme.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

