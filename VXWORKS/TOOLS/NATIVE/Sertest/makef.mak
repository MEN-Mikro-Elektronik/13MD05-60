#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2012/03/23 16:49:29 $
#      $Revision: 1.4 $
#
#    Description: makefile for sertest
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.4  2012/03/23 16:49:29  ts
#   R: changed include paths
#
#   Revision 1.3  2006/09/11 10:52:15  cs
#   added support for VxW6.x
#
#   Revision 1.2  2004/03/02 08:50:14  UFranke
#   fixed for Tornado 2.2
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
TGT_DIR=$(WIND_BASE)/target

ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
    MEN_OBJ_DIR_PREFIX=$(MEN_VX_DIR)/LIB/MEN
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_OBJ_DIR_PREFIX=$(MEN_VX_DIR)/LIB/MEN/$(WIND_PLATFORM)
endif

COMPILER=$(CC) -Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=sertest -DNO_CALLBACK -DNO_SHARED_MEM -D__RTP__

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL)$(DBGDIR)

#**************************************
#   input directories
#

MEN_INC_DIR=$(MEN_VX_DIR)/INCLUDE


#**************************************
#   include pathes
#
INC=-I$(MEN_INC_DIR)/COM     			\
    -I$(MEN_INC_DIR)/NATIVE  			\
    -I$(WIND_BASE)/target    			\
    -I$(WIND_BASE)/target/usr/h 		\
    -I$(WIND_BASE)/target/h/wrn/coreip  \
    -I.

#    -I$(WIND_BASE)/target/h  			\

all: $(OBJ_DIR)/sertest.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/sertest.o:  sertest.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@

