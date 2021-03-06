#***************************  M a k e f i l e  *******************************
#  
#         Author: dieter.pfeuffer@men.de
#          $Date: 2005/12/07 10:58:12 $
#      $Revision: 1.1 $
#  
#    Description: makefile for a404_ltest
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2005/12/07 10:58:12  dpfeuffer
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright by MEN mikro elektronik GmbH, Nuernberg, Germany 
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

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=a404_ltest #-DDBG

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
    -I.

all: $(OBJ_DIR)/ltest.o
	$(LD) -r $(OBJ_DIR)/ltest.o -o $(OBJ_DIR)/a404_ltest.o

clean:
	rm $(OBJ_DIR)/ltest.o $(OBJ_DIR)/a404_ltest.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/ltest.o:  ltest.c \
						makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

