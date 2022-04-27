#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2010/10/22 15:13:22 $
#      $Revision: 1.20 $
#
#    Description: Make a VxWorks MDIS component (for a single CPU)
#
#       Switches:
#
#				  COMMAKE		common makefile name (with path)
#				  CPU			cpu code to compile
#				  TOOL			toolchain to use (defaults to "gnu")
#				  TOOLPATH		gnu or diab
#				  TEST			(yes/no)
#				  COMP_PREFIX 	component prefix (e.g ll_)
#				  SMEN_DIR		where the VXWORKS directory is located
#				  OUTPUT_LIB1 	library file #1 to put the object in.
#				  WARN_LEVEL 	(default -Wall)
#				  LLDRV 		define as -D_LL_DRV_ or nothing
#				  THIS_DIR		directory where this file is located
#				  SINGLE_FILES_TO_LIB
#								if "yes", puts each single object file to lib.
#								else puts prelinked object file to lib
#				  OUTPUT_OWN_LIB
#								if "yes", additionally creates an own lib for
#								the component (oss.a dbg.a)
#
#			      CC_OPTIM_TARGET
#
#                 CC_SOFT_FLOAT
#
#				  BINARY_OBJ	(without path)
#
#				  VX_MAIN_REPLACING enables PROG_FILE_NAME define
#
#		Rules:	  all			main rule for drivers, libraries, programs
#
#-----------------------------------------------------------------------------
# $Log: component.mak,v $
# Revision 1.20  2010/10/22 15:13:22  UFranke
# R: TOOL sfgnu/sfdiab do not work under VxWorks 6.7/6.8
# M: switch CPU_/TOOL_SAVE defined in rules.mak now
#
# Revision 1.19  2010/10/22 10:54:46  UFranke
# R: toolchain for non FPU CPUs not supported
# M: prepared for TOOL sfgnu/sfdiab
#    set EXCLUDE_RTP = yes in custom.mak because of conflicts with RTP/sfTOOL
#
# Revision 1.18  2010/03/29 16:38:25  cs
# R: generalized ex-/include of RTP support
# M: pass RTP_FLAGS to every compiler call
#
# Revision 1.17  2009/05/29 13:17:18  ufranke
# R: kernel specific compiler switches missing
# M: added ADDED_KERNEL_CFLAGS
#
# Revision 1.16  2008/09/26 16:02:05  ufranke
# VxWorks 6.6 support
#
# Revision 1.15  2006/10/18 14:39:43  cs
# added:
#   + ADDED_CFLAGS
#   + save/restore TOOL and CPU before/after including defs.default
#
# Revision 1.14  2006/06/23 16:18:03  ufranke
# moved
#  - clean rule to the top makefile rules.mak
#
# Revision 1.13  2006/03/31 14:29:18  cs
# added directory prefix in lib and obj paths for VxWorks 6.x
# don't use back slashes in paths for x86-win32 and VxWorks 6.x
# Copyright line changed
#
# Revision 1.12  2006/03/03 18:23:04  cs
# changed prefix LIB$(CPU)$(TOOL) to lib$(CPU)$(TOOL)
#
# Revision 1.11  2005/12/23 15:46:57  UFRANKE
# cosmetics
#
# Revision 1.10  2005/12/07 14:04:20  UFranke
# added
#  + VxWorks 6.0/6.1 support
#
#-----------------------------------------------------------------------------
# (c) Copyright 2000..2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************
ifndef TOOL
	TOOL := gnu
endif

#--------------------------------------------------------
# restore TOOL and CPU
CPU  := $(CPU_SAVE)
TOOL  := $(TOOL_SAVE)
TOOL_FAMILY := $(TOOL_SAVE)

ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/defs.$(TOOLPATH)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/make.$(CPU)$(TOOL)
	MEN_VXWORKS_VERSION=-DMEN_VXWORKS_VERSION=$(MEN_VXWORKS_ENV_VER)
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/defs.$(TOOLPATH)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/make.$(CPU)$(TOOL)
	MEN_VXWORKS_VERSION=-DMEN_VXWORKS_VERSION=$(MEN_VXWORKS_ENV_VER)
else
	include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
endif

# use GNU AR for diab toolchain too
ifneq (,$(findstring gnu,$(TOOL)))
	GNUAR := $(AR)
else
	GNUAR := ar$(VX_CPU_FAMILY)
endif

#------------------------
# include common makefile

# the following three macros are don't care for VxWorks
LIB_PREFIX      := xxx
LIB_SUFFIX      := xxx
MEN_LIB_DIR     := xxx
SW_PREFIX       := -D
INC_DIR         := $(MEN_VX_DIR)/INCLUDE/COM
MEN_INC_DIR     := $(INC_DIR)/MEN
INP_SUFFIX      := .o

# obsolete...
OPT_1           := opt1
OPT_2           := opt1
OPT_3           := opt1

MEN_MOD_DIR		:= $(dir $(COMMAKE))


ifdef COMMAKE
	include $(COMMAKE)
endif

#------------------------
# determine output paths
ifeq ($(TEST),yes)
    TESTSUFFIX := test
	DBG		   := -DDBG $(DEBUG_FLAGS)
	DBGSTR	   := debug
else
    TESTSUFFIX :=
	DBG		   := $(DEBUG_FLAGS)
	DBGSTR	   := non-debug
endif


COMP_NAME := $(COMP_PREFIX)$(MAK_NAME)
COMP_OBJ  := ../$(COMP_PREFIX)$(MAK_NAME).o

LIBOUTPUT_DIR := $(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL_SAVE)$(TESTSUFFIX)
OBJOUTPUT_DIR := $(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL_SAVE)$(TESTSUFFIX)
TMP_OBJ_DIR	  := $(OBJOUTPUT_DIR)/$(COMP_NAME)

ifeq ($(WIND_HOST_TYPE),x86-win32)
  ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	TMP_OBJ_DIR_NAT := $(subst \,/,$(TMP_OBJ_DIR))
	DIRUP := ../
	PS := /
  else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	TMP_OBJ_DIR_NAT := $(subst \,/,$(TMP_OBJ_DIR))
	DIRUP := ../
	PS := /	
  else
	TMP_OBJ_DIR_NAT := $(subst /,\,$(TMP_OBJ_DIR))
	DIRUP := ..$(subst /,\,/)
	PS := $(subst /,\,/)
  endif
else
	TMP_OBJ_DIR_NAT := $(TMP_OBJ_DIR)
	DIRUP := ../
	PS := /
endif


TMP_COMP_OBJS := $(addprefix $(COMP_NAME)_,$(MAK_INP))

vpath %.c $(MEN_MOD_DIR)

#**************************************
#   include pathes
#
VX_INC_DIR      :=  $(MEN_VX_DIR)/INCLUDE/NATIVE
MEN_VX_INC_DIR  :=  $(VX_INC_DIR)/MEN

INC_DIRS:=-I$(WIND_BASE)/target     \
    -I$(WIND_BASE)/target/h         \
    -I$(WIND_BASE)/target/h/drv/pci \
    -I$(WIND_BASE)/target/h/wrn/coreip \
    -I$(INC_DIR)                    \
    -I$(VX_INC_DIR)                 \
	$(EXTRA_INCLUDE)				\
    -I$(MEN_MOD_DIR)

#**************************************
#   Compiler flags
#
FLAGS           :=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
DEF             :=-DVXWORKS
ifdef VX_MAIN_REPLACING
	DEF             +=-DPROG_FILE_NAME=$(MAK_NAME)
endif


ifndef MAK_SWITCH
    MAK_SWITCH := -DMAC_MEM_MAPPED
endif

ifndef WARN_LEVEL
    WARN_LEVEL := -Wall
endif

CFLAGS          :=  $(FLAGS) $(RTP_FLAGS) $(DEF) $(DBG) $(MAK_SWITCH) $(INC_DIRS) \
					$(WARN_LEVEL) $(LLDRV) $(CC_SOFT_FLOAT) \
					$(MEN_VXWORKS_VERSION)

#----------------------------------------------------------------------
# Rules for level #0 of make
#
.PHONY: all buildmsg buildinobjdir build binary_comp

ifndef BINARY_OBJ
all: buildmsg $(TMP_OBJ_DIR)/anchor buildinobjdir
else
all: binary_comp
endif

buildmsg:
	@$(ECHO) +
	@$(ECHO) ++++++++ Building $(COMP_NAME) for $(CPU)/$(TOOL) $(DBGSTR) +++++++++++

$(TMP_OBJ_DIR)/anchor:
	- @ mkdir $(TMP_OBJ_DIR_NAT)
	@$(ECHO) > $(TMP_OBJ_DIR_NAT)$(PS)anchor
	@$(ECHO) Directory $(TMP_OBJ_DIR) created

buildinobjdir:
	$(MDBG)$(MAKE) -C $(TMP_OBJ_DIR) --no-print-directory\
	 -f $(COMP_MAK) build


#----------------------------------------------------------------------
# Rules for level #1 of make
#

build: $(COMP_OBJ)

# override default buildin rule for .c -> .o files (just to get less messages)
$(COMP_NAME)_%.o : %.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $< -o $@

# partially link file (for dynamic download & linking)
# and add it to library for static linking
$(COMP_OBJ): $(TMP_COMP_OBJS)
	@$(ECHO) Linking $@
	$(MDBG)$(LD) -r -o $@ $(TMP_COMP_OBJS)
ifeq ($(SINGLE_FILES_TO_LIB),yes)
		@$(ECHO) Adding $(TMP_COMP_OBJS) to library $(OUTPUT_LIB1)
		$(MDBG)$(GNUAR) -dc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) $(TMP_COMP_OBJS)
		$(MDBG)$(GNUAR) -qc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) $(TMP_COMP_OBJS)
else
		@$(ECHO) Adding $@ to library $(OUTPUT_LIB1)
		$(MDBG)$(GNUAR) -dc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) $(notdir $@)
		$(MDBG)$(GNUAR) -qc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) $@
endif
ifeq ($(OUTPUT_OWN_LIB),yes)
		@$(ECHO) Adding $(TMP_COMP_OBJS) to own library $(COMP_NAME).a
		$(MDBG)$(RM) $(LIBOUTPUT_DIR)/$(COMP_NAME).a
		$(MDBG)$(GNUAR) -rc $(LIBOUTPUT_DIR)/$(COMP_NAME).a $(TMP_COMP_OBJS)
endif

# Dependencies
$(TMP_COMP_OBJS): 	$(MAK_INCL) \
			 		$(COMMAKE)

#----------------------------------------------------------------------
# Special rule for precompiled components (not delivered in source)
#

binary_comp:
	@$(ECHO) Adding $(BINARY_OBJ) to library $(OUTPUT_LIB1)
	$(MDBG)$(GNUAR) -dc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) $(BINARY_OBJ)
	$(MDBG)$(GNUAR) -qc $(LIBOUTPUT_DIR)/$(OUTPUT_LIB1) \
					   $(OBJOUTPUT_DIR)/$(BINARY_OBJ)




