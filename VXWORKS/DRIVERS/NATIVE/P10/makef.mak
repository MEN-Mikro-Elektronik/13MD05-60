#***************************  M a k e f i l e  *******************************
#  
#        $Author: ts $
#          $Date: 2006/02/23 12:24:29 $
#      $Revision: 1.3 $
#  
#    Description: makefile for p10_drv
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.3  2006/02/23 12:24:29  ts
#   support for EM3 family (85XX)
#
#   Revision 1.2  2002/04/24 15:49:46  Franke
#   cosmetics
#
#   Revision 1.1  2000/03/14 14:30:36  loesel
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************
#**************************************
#   define used tools
#
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
COMPILER=$(CC) -Wall
LIB=$(AR)
#-DDBG
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

OBJ_DIR=$(MEN_WORK_DIR)/VXWORKS/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_WORK_DIR)/VXWORKS/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

#**************************************
#   input directories
#

MEN_INC_DIR=$(MEN_WORK_DIR)/VXWORKS/INCLUDE


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



all: $(OBJ_DIR)/p10_drv.o




#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/p10_drv.o:  p10_drv.c \
					   p10_drv.h \
                       makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
	$(LIB)      $(LIB_FLAG_DEL) $(MEN_LIB_DIR)/p10_drv.a $@
	$(LIB)      $(LIB_FLAG_ADD) $(MEN_LIB_DIR)/p10_drv.a $@



