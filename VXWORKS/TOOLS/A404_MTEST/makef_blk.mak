#***************************  M a k e f i l e  *******************************
#  
#         Author: dp
#          $Date: $
#      $Revision: $
#  
#    Description: makefile for mtest - VMEBLT variant
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#-----------------------------------------------------------------------------
#   (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

#**************************************
#   define used tools
#
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
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
OBJ_DIR=H:/work/VXWORKS/TOOLS/NATIVE/A404/A404_MTEST
#OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
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
    -I.

all: $(OBJ_DIR)/mtestblk.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/mtestblk.o:  a404_mtest.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
					$(MEN_INC_DIR)/NATIVE/MEN/sysMenPci2Vme.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

