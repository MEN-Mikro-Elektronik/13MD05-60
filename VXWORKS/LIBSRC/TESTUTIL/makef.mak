#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2011/07/27 15:35:13 $
#      $Revision: 1.6 $
#
#    Description: makefile for testutil
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: makef.mak,v $
#   Revision 1.6  2011/07/27 15:35:13  dpfeuffer
#   R: compile for 16G215 design test
#   M: tu_test.o added
#
#   Revision 1.5  2009/07/28 16:44:13  cs
#   R:1. compilation with VxW 6.7 failed
#   M:1.a) adapt paths to work with VxW6.7 as well
#        b) automatically create part of obj dir path
#
#   Revision 1.4  2006/06/09 17:37:22  cs
#   ported so it can be compiled under VxWorks 6.x as well
#
#   Revision 1.3  2006/02/20 16:20:59  ts
#   checkin for BSP Verification build
#
#   Revision 1.2  2001/01/30 14:03:27  franke
#   cosmetics
#
#   Revision 1.1  2000/01/13 13:00:45  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

#**************************************
#   define used tools
#
TGT_DIR=$(WIND_BASE)/target

# CPU=PPC32

ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
    MEN_OBJ_DIR_PREFIX=$(MEN_VX_DIR)/LIB/MEN
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_OBJ_DIR_PREFIX=$(MEN_VX_DIR)/LIB/MEN/$(WIND_PLATFORM)
endif

COMPILER=$(CC)
#-Wall
LIB=$(AR)
FLAGS=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
LIB_FLAG_DEL=-d
LIB_FLAG_ADD=-r
MK=$(MAKE)

ACCESS=-DVXWORKS

LIB_NAME=testutil.a


#**************************************
#   output directories
#

OBJ_DIR    =$(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL)$(DBGDIR)/TESTUTIL
MEN_OBJ_DIR=$(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL)$(DBGDIR)

ifeq ($(WIND_HOST_TYPE),x86-win32)
	MEN_OBJ_DIR_NAT	:= $(subst /,\,$(MEN_OBJ_DIR))
	MEN_LIB_DIR_NAT	:= $(subst /,\,$(MEN_LIB_DIR))
	OBJ_DIR_NAT		:= $(subst /,\,$(OBJ_DIR))
	PS := $(subst /,\,/)
else
	MEN_OBJ_DIR_NAT := $(MEN_OBJ_DIR)
	MEN_LIB_DIR_NAT := $(MEN_LIB_DIR)
	OBJ_DIR_NAT 	:= $(OBJ_DIR)
	DIRUP := ../
	PS := /
endif

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

all: mkobjdir $(MEN_OBJ_DIR)/testutil.o $(MEN_LIB_DIR)/$(LIB_NAME)

OBJS=$(OBJ_DIR)/tu_test.o \
	 $(OBJ_DIR)/screen.o \
	 $(OBJ_DIR)/time.o

#**************************************
#   dependencies and comands
#
mkobjdir: $(MEN_OBJ_DIR)/anchor
	- @ mkdir $(OBJ_DIR_NAT)

$(MEN_OBJ_DIR)/anchor:
	- @ mkdir $(MEN_OBJ_DIR_NAT)
	@$(ECHO) > $(MEN_OBJ_DIR_NAT)$(PS)anchor
	@$(ECHO) Directory $(MEN_OBJ_DIR) created

$(MEN_OBJ_DIR)/testutil.o:  $(OBJS) \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
		            makef.mak
	$(LD) -r -o $@ $(OBJS)

$(MEN_LIB_DIR)/$(LIB_NAME): $(OBJS)


$(OBJ_DIR)/tu_test.o: tu_test.c
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
	$(LIB)      $(LIB_FLAG_DEL) $(MEN_LIB_DIR)/$(LIB_NAME) $@
	$(LIB)      $(LIB_FLAG_ADD) $(MEN_LIB_DIR)/$(LIB_NAME) $@

$(OBJ_DIR)/screen.o: screen.c
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
	$(LIB)      $(LIB_FLAG_DEL) $(MEN_LIB_DIR)/$(LIB_NAME) $@
	$(LIB)      $(LIB_FLAG_ADD) $(MEN_LIB_DIR)/$(LIB_NAME) $@

$(OBJ_DIR)/time.o: time.c
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
	$(LIB)      $(LIB_FLAG_DEL) $(MEN_LIB_DIR)/$(LIB_NAME) $@
	$(LIB)      $(LIB_FLAG_ADD) $(MEN_LIB_DIR)/$(LIB_NAME) $@


clean:
	$(RM) $(OBJS) $(MEN_OBJ_DIR)/testutil.o $(MEN_LIB_DIR)/$(LIB_NAME)

