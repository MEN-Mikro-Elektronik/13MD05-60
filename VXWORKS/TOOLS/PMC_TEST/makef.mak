#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: 2008/01/21 10:36:38 $
#      $Revision: 1.1 $
#  
#    Description: makefile for PMC test together with P599 test design
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2008/01/21 10:36:38  ts
#   Initial Revision, 1st work checkin
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2008 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

#**************************************
#   define used tools
#
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL) 
COMPILER=$(CC) -Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU) -DVXWORKS -DPROG_FILE_NAME=pmc_test
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS 
#-DDBG

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

all: $(OBJ_DIR)/pmc_test.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/pmc_test.o:   pmc_test.c \
						 $(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    	 $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
						 $(MEN_INC_DIR)/COM/MEN/testutil.h \
		            	 makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

