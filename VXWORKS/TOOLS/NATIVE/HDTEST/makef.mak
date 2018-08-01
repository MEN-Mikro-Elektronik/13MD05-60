#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/09/11 10:47:44 $
#      $Revision: 1.6 $
#
#    Description: makefile for hdtest
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.6  2006/09/11 10:47:44  cs
#   added support for VxW 6.x
#
#   Revision 1.5  2005/05/04 15:08:29  kp
#   PID21 adaption
#
#   Revision 1.4  2000/03/28 16:11:51  Franke
#   replaced SMEN_DIR by MEN_WORK_DIR
#
#   Revision 1.3  2000/02/07 13:57:02  Franke
#   added SMEN_DIR checking
#
#   Revision 1.2  2000/01/13 11:09:27  kp
#   forgot to set PROG_FILE_NAME
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************


#******************
# Sandbox directory
# specify the full path name
# (include also the drive letter if Tornado or project dir is on a different
# drive)
# Better you set this macro externally by defining an environment variable
# with the same name: e.g. MEN_WORK_DIR=s:/work
#
ifeq ( , $(MEN_VX_DIR) )
    # print error message and remove the wrong dependency file
    MEN_VX_DIR = ******__MEN_VX_DIR__missing
    makef.mak: out
	@echo $(MEN_VX_DIR)
else
endif


export _BDGEN_=1
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
    MEN_VXVERSUB_DIR=$(WIND_PLATFORM)
endif

COMPILER=$(CC) -Wall -mlong-calls
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(GNUMAKE)

ACCESS=-DMAC_MEM_MAPPED -DVXWORKS -DPROG_FILE_NAME=hdtest #-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/$(MEN_VXVERSUB_DIR)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_VX_DIR)/LIB/MEN/$(MEN_VXVERSUB_DIR)/lib$(CPU)$(TOOL)$(DBGDIR)

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
    -I$(WIND_BASE)/target/h/wrn/coreip  \
    -I.

all: clean $(MEN_LIB_DIR)/hdtest.a


#**************************************
#   dependencies and comands
#
$(MEN_LIB_DIR)/hdtest.a: $(OBJ_DIR)/hdtest.o
ifeq ($(WIND_HOST_TYPE),x86-win32)
	-$(LIB)      $(LIB_FLAG_DEL) $(subst /,\,$@) $(subst /,\,$<)
	$(LIB)      $(LIB_FLAG_ADD) $(subst /,\,$@) $(subst /,\,$<)
else
	$(LIB)      $(LIB_FLAG_DEL) $@ $<
	$(LIB)      $(LIB_FLAG_ADD) $@ $<
endif
$(OBJ_DIR)/hdtest.o:  hdtest.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@


clean:
	-$(RM) $(OBJ_DIR)/hdtest.o $(MEN_LIB_DIR)/hdtest.a