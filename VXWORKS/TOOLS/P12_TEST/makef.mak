#***************************  M a k e f i l e  *******************************
#  
#         Author: rl
#          $Date: 2001/08/27 10:17:43 $
#      $Revision: 1.1 $
#  
#    Description: makefile for timertest
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2001/08/27 10:17:43  franke
#   Initial Revision
#
#   Revision 1.1  2001/03/21 11:16:22  Lange
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
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU) -DVXWORKS -DPROG_FILE_NAME=p12_test
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

all: $(OBJ_DIR)/p12_test.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/p12_test.o:   p12_test.c \
						 $(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    	 $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
						 $(MEN_INC_DIR)/COM/MEN/testutil.h \
		            	 makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

