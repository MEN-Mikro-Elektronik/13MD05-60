#***************************  M a k e f i l e  *******************************
#  
#         Author: rl
#          $Date: 2001/12/04 10:48:34 $
#      $Revision: 1.3 $
#  
#    Description: makefile for pciScanner
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.3  2001/12/04 10:48:34  Franke
#   cosmetics
#
#   Revision 1.2  2000/03/22 15:42:00  loesel
#   just cosmetics
#
#   Revision 1.1  2000/03/22 15:40:36  loesel
#   Initial Revision
#
#      
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

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

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS #-DDBG

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

all: $(OBJ_DIR)/pciScanner.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/pciScanner.o:  pciScanner.c \
						  makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

