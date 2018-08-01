#***************************  M a k e f i l e  *******************************
#
#-----------------------------------------------------------------------------
#	(c) Copyright 2018 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

#**************************************
#   define used tools
#
TGT_DIR=$(WIND_BASE)/target

# define architectures to build
ifeq ($(CPU), )
	CPU=PPC32
endif

ifeq ($(TOOLS), )
	TOOLS=e500v2gnu
endif

include $(TGT_DIR)/h/make/defs.default
include $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOL)
include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
MEN_VXVERSUB_DIR=$(WIND_PLATFORM)

COMPILER=$(CC) -te500v2 -mhard-float -fno-implicit-fp -Wall
FLAGS=-O0 -DCPU=$(CPU) -D$(CPU_BOARD)
MK=$(GNUMAKE)

ifeq ($(DBG),-g)
	DBGDIR=test
else
	DBGDIR=
endif

ACCESS=-DVXWORKS -DPROG_FILE_NAME=vme4vx_mtest 

#**************************************
#   output directories
#
OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/$(MEN_VXVERSUB_DIR)/obj$(CPU)$(TOOLS)$(DBGDIR)

#**************************************
#   input directories
#
MEN_INC_DIR=$(MEN_VX_DIR)/INCLUDE

#**************************************
#   include paths
#
INC=-I$(MEN_INC_DIR)/COM	\
    -I$(MEN_INC_DIR)/NATIVE	\
	-I$(TGT_DIR)/config/all	\
	-I$(BSP_DIR)			\
    -I$(WIND_BASE)/target	\
    -I$(WIND_BASE)/target/h	\
    -I$(WIND_BASE)/target/h/wrn/coreip	\
    -I.
	
TOOLS=e500v2gnu

clean:
	@$(RM) $(OBJ_DIR)/vme4vx_mtest.o*

all: 
	@$(MKDIR) $(OBJ_DIR)
	$(ECHO) $(TGT_DIR)/h/tool/$(TOOL_FAMILY)/make.$(CPU)$(TOOLS)
	$(COMPILER) $(FLAGS) $(DBG) $(INC) $(DEF) $(ACCESS) -c vme4vx_mtest.c -o $(OBJ_DIR)/vme4vx_mtest.o
