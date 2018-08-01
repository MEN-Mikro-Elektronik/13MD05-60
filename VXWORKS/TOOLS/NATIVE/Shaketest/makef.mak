#***************************  M a k e f i l e  *******************************
#  
#         Author: kp
#          $Date: 2000/03/01 17:37:36 $
#      $Revision: 1.1 $
#  
#    Description: makefile for sertest
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.1  2000/03/01 17:37:36  loesel
#   Initial Revision
#
#   Revision 1.1  2000/01/14 15:00:37  loesel
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

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=shaketest #-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(SMEN_DIR)/VXWORKS/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(SMEN_DIR)/VXWORKS/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

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

all: $(OBJ_DIR)/shaketest.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/shaketest.o:  shaketest.c \
						$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    	$(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
						$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
						$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
						$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            	makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

