#***************************  M a k e f i l e  *******************************
#
#         Author: mkolpak
#          $Date: 2009/04/07 11:56:57 $
#      $Revision: 1.1 $
#
#    Description: makefile for p511prog
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2009/04/07 11:56:57  MKolpak
#   Initial Revision
# 
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************


ifeq ( , $(MEN_VX_DIR) )
    # print error message and remove the wrong dependency file
    MEN_VX_DIR = ******__MEN_VX_DIR__missing
    makef.mak: out
	@echo $(MEN_VX_DIR)
else
endif


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
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=p511prog -DBSP_WITH_P511 -DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN$(MEN_VXVERSUB_DIR)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_VX_DIR)/LIB$(MEN_VXVERSUB_DIR)/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

#**************************************
#   input directories
#

MEN_INC_DIR=$(MEN_VX_DIR)/INCLUDE


#**************************************
#   include pathes
#
INC=-I$(MEN_INC_DIR)/COM     \
    -I$(MEN_INC_DIR)/NATIVE  \
    -I$(MEN_VX_DIR)/DRIVERS/NATIVE/Z077END \
    -I$(WIND_BASE)/target    \
    -I$(WIND_BASE)/target/h  \
    -I$(WIND_BASE)/target/h/wrn/coreip  \
    -I.

all: $(OBJ_DIR)/p511prog.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/p511prog.o:  p511prog.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

