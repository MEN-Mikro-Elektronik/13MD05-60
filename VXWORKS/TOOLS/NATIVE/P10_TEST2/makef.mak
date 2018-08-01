#***************************  M a k e f i l e  *******************************
#  
#        $Author: kp $
#          $Date: 2000/11/22 15:10:57 $
#      $Revision: 1.1 $
#  
#    Description: makefile for P10 Test Utility
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2000/11/22 15:10:57  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

#**************************************
#   define used tools
#
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
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

OBJ_DIR=$(SMEN_DIR)/VXWORKS/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(SMEN_DIR)/VXWORKS/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

MY_OBJ_DIR=$(SMEN_DIR)/VXWORKS/DRIVERS/NATIVE/P10/test
# MEN_LIB_DIR=$(SMEN_DIR)/VXWORKS/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)


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

all:  $(OBJ_DIR)/p10_test2.o
      

     

#**************************************
#   dependencies and comands
#


$(OBJ_DIR)/p10_test2.o:   p10_test2.c \
                        makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@


