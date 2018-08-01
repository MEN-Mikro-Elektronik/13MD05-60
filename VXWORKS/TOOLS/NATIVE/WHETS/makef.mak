#***************************  M a k e f i l e  *******************************
#  
#        $Author: ts $
#          $Date: 2006/03/31 10:26:29 $
#      $Revision: 1.1 $
#  
#    Description: makefile for whetstone
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2006/03/31 10:26:29  ts
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

# usually set by mk.bat but this makef.mak can be invoked by 
# make -f makef.mak
CPU = PPC603
TOOL = gnu
TGT_DIR =$(WIND_BASE)/target
SMEN_DIR=/home/tschnuer/work/work_EM10A/VXWORKS

#**************************************
#   define used tools
#

# for vxWorks 5.5.1
# include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)

# for vxWorks 6.x
include $(WIND_BASE)/target/h/tool/gnu/make.$(CPU)$(TOOL)

COMPILER=$(CC) -Wall
LINKER:=$(LD) -Map mapfile
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED

#**************************************
#   additional settings
#
OPT_0=$(DBG)
OPT_1=$(DBG)
OPT_2=$(DBG)
INP_SUFFIX=

#**************************************
#   output directories
#
OBJ_DIR=$(SMEN_DIR)/LIB/MEN/$(WIND_PLATFORM)/obj$(CPU)$(TOOL)$(DBGDIR)


#**************************************
#   input directories
#

MEN_INC_DIR=$(SMEN_DIR)/VXWORKS/INCLUDE


#**************************************
#   include pathes
#
INC=-I$(MEN_INC_DIR)/COM     \
    -I$(MEN_INC_DIR)/NATIVE  \
    -I$(WIND_BASE)/target    \
    -I$(WIND_BASE)/target/h  \
    -I.

#additional paths needed for 85xx gnu
ifeq ($(CPU),PPC85XX)
INC+= -I$(WIND_BASE)/target/h/wrn/coreip  \
      -I$(WIND_BASE)/host/gnu/3.3/$(WIND_HOST_TYPE)/lib/gcc-lib/powerpc-wrs-vxworks/3.3-e500/include
endif


all:  $(OBJ_DIR)/whets.o


#**************************************
#   dependencies and comands
#


$(OBJ_DIR)/whets.o:	whets.c \
					makef.mak 
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

