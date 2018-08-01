#***************************  M a k e f i l e  *******************************
#  
#         Author: kp
#          $Date: 2005/05/04 15:11:03 $
#      $Revision: 1.3 $
#  
#    Description: makefile for mtest
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.3  2005/05/04 15:11:03  kp
#   MEN_VX_DIR used
#
#   Revision 1.2  2001/01/30 14:07:29  franke
#   cosmetics
#
#   Revision 1.1  2000/01/13 14:45:06  kp
#   Initial Revision
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
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU) -DVMEBLT=1
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=mtest #-DDBG

#**************************************
#   output directories
#
#OBJ_DIR=$(MEN_VX_DIR)/VXWORKS/TOOLS/NATIVE/A404/A404_MTEST
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
#additional paths needed for 85xx gnu
ifeq ($(CPU),PPC85XX)
INC+= -I$(WIND_BASE)/target/h/wrn/coreip  \
      -I$(WIND_BASE)/host/gnu/3.3/$(WIND_HOST_TYPE)/lib/gcc-lib/powerpc-wrs-vxworks/3.3-e500/include
endif


all: $(OBJ_DIR)/mtest2.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/mtest2.o:  a404_mtest.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

