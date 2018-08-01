#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2011/07/27 15:35:15 $
#      $Revision: 1.6 $
#
#    Description: makefile for mtest
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.6  2011/07/27 15:35:15  dpfeuffer
#   R: no clean target
#   M: clean target added
#
#   Revision 1.5  2006/09/11 10:51:39  cs
#   added support for VxW6.x
#
#   Revision 1.4  2006/02/20 15:11:50  ts
#   added line for including make.$(CPU)$(TOOL) in 6.x
#
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
TGT_DIR=$(WIND_BASE)/target


ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
    MEN_VXVERSUB_DIR=
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=/$(WIND_PLATFORM)
endif

COMPILER=$(CC)  -Wall -mlong-calls
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=mtest #-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN$(MEN_VXVERSUB_DIR)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_VX_DIR)/LIB/MEN$(MEN_VXVERSUB_DIR)/lib$(CPU)$(TOOL)$(DBGDIR)

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

all: $(OBJ_DIR)/mtest.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/mtest.o:  mtest2.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@


clean:
	$(RM) $(OBJ_DIR)/mtest.o