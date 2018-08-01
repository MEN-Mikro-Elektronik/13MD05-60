#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: 2007/08/21 16:53:11 $
#      $Revision: 1.2 $
#  
#    Description: makefile for m36ndevi
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.2  2007/08/21 16:53:11  ts
#   Compiles and loads, calculates median
#
#   Revision 1.1  2007/08/21 13:09:38  ts
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

#**************************************
#   define used tools
#

# Tornado
# include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL

CPU = PPC85XX
TOOL = gnu
TGT_DIR =$(WIND_BASE)/target
SMEN_DIR=s:/work

#ab vxWorks 6.0
include $(WIND_BASE)/target/h/tool/gnu/make.$(CPU)$(TOOL)

TGT_DIR =$(WIND_BASE)/target
COMPILER=$(CC) -Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU) -DVMEBLT=1
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=m36ndevi #-DDBG

#**************************************
#   output directories
#
#OBJ_DIR=$(MEN_VX_DIR)/VXWORKS/TOOLS/NATIVE/A404/A404_MTEST
#OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
OBJ_DIR=R:\users\fw\projects\0621_mmodule\a12b
MEN_LIB_DIR=$(MEN_VX_DIR)/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

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
    -I$(WIND_BASE)/target/usr/h \
    -I$(WIND_GNU_PATH)/$(WIND_HOST_TYPE)/lib/gcc-lib/powerpc-wrs-vxworks/3.3.2/include \
    -I.
#additional paths needed for 85xx gnu
ifeq ($(CPU),PPC85XX)
INC+= -I$(WIND_BASE)/target/h/wrn/coreip  \
      -I$(WIND_BASE)/host/gnu/3.3/$(WIND_HOST_TYPE)/lib/gcc-lib/powerpc-wrs-vxworks/3.3-e500/include
endif


all: $(OBJ_DIR)/m36ndevi.o


#**************************************
#   dependencies and commands
#
$(OBJ_DIR)/m36ndevi.o:  m36ndevi.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

