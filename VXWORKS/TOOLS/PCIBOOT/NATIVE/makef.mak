#***************************  M a k e f i l e  *******************************
#  
#         Author: rt
#          $Date: 2007/10/02 13:16:33 $
#      $Revision: 1.1 $
#  
#    Description: makefile for pciboot
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2007/10/02 13:16:33  rtrübenbach
#   Initial Revision
#
#
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2007-2007 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************


#******************
# Sandbox directory
# specify the full path name 
# (include also the drive letter if Tornado or project dir is on a different
# drive)
# Better you set this macro externally by defining an environment variable
# with the same name: e.g. MEN_WORK_DIR=s:/work
#
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
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
COMPILER=$(CC) -Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)
LINKER=$(LD)

#**************************************
#   determine prefix
#
MEN_BOARD := $(subst a,A,$(MEN_BOARD_LOWERCASE))
MEN_BOARD := $(subst b,B,$(MEN_BOARD))
MEN_BOARD := $(subst c,C,$(MEN_BOARD))
MEN_BOARD := $(subst d,D,$(MEN_BOARD))
MEN_BOARD := $(subst e,E,$(MEN_BOARD))
MEN_BOARD := $(subst f,F,$(MEN_BOARD))
MEN_BOARD := $(subst g,G,$(MEN_BOARD))
MEN_BOARD := $(subst h,H,$(MEN_BOARD))
MEN_BOARD := $(subst i,I,$(MEN_BOARD))
MEN_BOARD := $(subst j,J,$(MEN_BOARD))
MEN_BOARD := $(subst k,K,$(MEN_BOARD))
MEN_BOARD := $(subst l,L,$(MEN_BOARD))
MEN_BOARD := $(subst m,M,$(MEN_BOARD))
MEN_BOARD := $(subst n,N,$(MEN_BOARD))
MEN_BOARD := $(subst o,O,$(MEN_BOARD))
MEN_BOARD := $(subst p,P,$(MEN_BOARD))
MEN_BOARD := $(subst q,Q,$(MEN_BOARD))
MEN_BOARD := $(subst r,R,$(MEN_BOARD))
MEN_BOARD := $(subst s,S,$(MEN_BOARD))
MEN_BOARD := $(subst t,T,$(MEN_BOARD))
MEN_BOARD := $(subst u,U,$(MEN_BOARD))
MEN_BOARD := $(subst v,V,$(MEN_BOARD))
MEN_BOARD := $(subst w,W,$(MEN_BOARD))
MEN_BOARD := $(subst x,X,$(MEN_BOARD))
MEN_BOARD := $(subst y,Y,$(MEN_BOARD))
MEN_BOARD := $(subst z,Z,$(MEN_BOARD))

ifeq (generic,$(MEN_BOARD_LOWERCASE))
    MEN_BOARD_PREFIX := 
else
    MEN_BOARD_PREFIX := $(MEN_BOARD_LOWERCASE)_
endif

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=$(MEN_BOARD_PREFIX)pciboot -D$(MEN_BOARD)

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
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
    -I$(WIND_BASE)/target/h/wrn/coreip  \
    -I../COM  \
    -I.


#**************************************
#   dependencies and comands
#

all: $(OBJ_DIR)/$(MEN_BOARD_PREFIX)pciboot.o
	

$(OBJ_DIR)/$(MEN_BOARD_PREFIX)pciboot.o:  ../COM/pciboot.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
					../COM/pciboot.h \
		            ./makef.mak \
                    ./mk*.bat
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
	
	