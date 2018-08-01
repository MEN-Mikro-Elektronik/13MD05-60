#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2010/10/22 15:12:47 $
#      $Revision: 1.14 $
#
#    Description: Make VxWorks MDIS descriptors
#
#       Switches: DESC			filename of descriptor (without extension)
#				  CONFIG_NAME	e.g. mdis_CFG1
#				  CPU			cpu code to compile
#				  TOOL			toolchain to use (defaults to "gnu")
#				  TOOLPATH		gnu or diab
#				  TEST			(yes/no)
#				  SMEN_DIR		where the VXWORKS directory is located
#				  THIS_DIR		directory where this file is located
#                 ADDED_KERNEL_CFLAGS kernel specific compiler switches
#
#		Rules:	  all			make descriptor
#
#
#-----------------------------------------------------------------------------
# $Log: desc.mak,v $
# Revision 1.14  2010/10/22 15:12:47  UFranke
# R: TOOL sfgnu/sfdiab do not work under VxWorks 6.7/6.8
# M: switch CPU_/TOOL_SAVE defined in rules.mak now
#
# Revision 1.13  2010/10/22 10:55:54  UFranke
# R: toolchain for non FPU CPUs not supported
# M: prepared for TOOL sfgnu/sfdiab
#    set EXCLUDE_RTP = yes in custom.mak because of conflicts with RTP/sfTOOL
#
# Revision 1.12  2010/03/29 16:42:35  cs
# R: generalized ex-/include of RTP support
# M: pass RTP_FLAGS to every compiler call
#
# Revision 1.11  2009/05/29 13:16:49  ufranke
# R: kernel specific compiler switches missing
# M: added ADDED_KERNEL_CFLAGS
#
# Revision 1.10  2008/09/26 16:02:15  ufranke
# VxWorks 6.6 support
#
# Revision 1.9  2006/10/18 14:40:43  cs
# added:
#   + ADDED_CFLAGS
#
# Revision 1.8  2006/06/23 16:18:07  ufranke
# moved
#  - clean rule to the top makefile rules.mak
#
# Revision 1.7  2006/03/31 14:29:29  cs
# added directory prefix in lib and obj paths for VxWorks 6.x
# Copyright line changed
#
# Revision 1.6  2005/12/23 15:47:04  UFRANKE
# cosmetics
#
# Revision 1.5  2005/12/07 14:04:34  UFranke
# added
#  + VxWorks 6.0/6.1 support
#
#-----------------------------------------------------------------------------
# (c) Copyright 2000..2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************

include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)
include $(WIND_BASE)/target/h/make/defs.default

CPU  := $(CPU_SAVE)
TOOL := $(TOOL_SAVE)

ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/make.$(CPU)$(TOOL)
	MEN_VXWORKS_VERSION=-DMEN_VXWORKS_VERSION=$(MEN_VXWORKS_ENV_VER)
else
	include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
endif

# use GNU AR for diab toolchain too
ifeq ($(TOOL),gnu)
	GNUAR := $(AR)
else
	GNUAR := ar$(VX_CPU_FAMILY)
endif

#------------------------
# determine output paths
ifeq ($(TEST),yes)
    TESTSUFFIX := test
	DBG		   := -DDBG $(DEBUG_FLAGS)
else
    TESTSUFFIX :=
	DBG		   := $(DEBUG_FLAGS)
endif


LIBOUTPUT_DIR 	:= $(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL_SAVE)$(TESTSUFFIX)
OBJOUTPUT_DIR 	:= $(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL_SAVE)$(TESTSUFFIX)/$(CONFIG_NAME)
OUTPUT_LIB		:= $(CONFIG_NAME).a

vpath %.c $(THIS_DIR)
vpath %.dsc $(THIS_DIR)

#**************************************
#   include pathes
#
VX_INC_DIR      :=  $(MEN_VX_DIR)/INCLUDE/NATIVE
INC_DIR         :=  $(MEN_VX_DIR)/INCLUDE/COM
MEN_VX_INC_DIR  :=  $(VX_INC_DIR)/MEN

INC_DIRS:=-I$(WIND_BASE)/target     \
    -I$(WIND_BASE)/target/h         \
    -I$(INC_DIR)                    \
	$(EXTRA_INCLUDE)				\
    -I$(VX_INC_DIR)

#**************************************
#   Compiler flags
#
FLAGS           :=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
DEF             :=-DVXWORKS

ifndef WARN_LEVEL
    WARN_LEVEL := -Wall
endif


CFLAGS          :=  $(FLAGS) $(RTP_FLAGS) $(DEF) $(DBG) $(INC_DIRS) $(WARN_LEVEL) $(MEN_VXWORKS_VERSION)

#----------------------------------------------------------------------
# Rules for level #0 of make
#
.PHONY: all buildmsg buildinobjdir build buildc

all: buildmsg buildinobjdir

buildmsg:
	@$(ECHO) +
	@$(ECHO) ++++++++ Building descriptor $(DESC) for $(CPU)/$(TOOL) +++++++++++

buildinobjdir:
	$(MDBG)$(MAKE) -C $(OBJOUTPUT_DIR) --no-print-directory\
	 -f $(DESC_MAK) build


#----------------------------------------------------------------------
# Rules for level #1 of make
#

build: $(DESC).o


$(DESC).o : $(DESC).c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $(THIS_DIR)/$(DESC).c
	@$(ECHO) Adding $@ to library $(OUTPUT_LIB)
	$(MDBG)$(GNUAR) -d $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) $(notdir $@)
	$(MDBG)$(GNUAR) -r $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) $@



$(DESC).c: $(DESC).dsc
	$(MDBG)$(MAKE) -C $(THIS_DIR) -f $(DESC_MAK) --no-print-directory buildc \
	DESC=$(DESC)

buildc:
	descgen -r $(DESC).dsc






