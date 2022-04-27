#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2010/10/22 15:13:05 $
#      $Revision: 1.19 $
#
#    Description: Make the MDIS config files (ll_entry,bb_entry,prog_ref)
#
#       Switches:
#				  CONFIG_NAME	e.g. mdis_CFG1
#				  CPU			cpu code to compile
#				  TOOL			toolchain to use (defaults to "gnu")
#				  TOOLPATH		gnu or diab
#				  TEST			(yes/no)
#				  SMEN_DIR		where the VXWORKS directory is located
#				  THIS_DIR		directory where this file is located
#
#		Rules:	  all			build configs (e.g. mdis_CFG1.a)
#				  download		build config object file for download
#
#-----------------------------------------------------------------------------
# $Log: configs.mak,v $
# Revision 1.19  2010/10/22 15:13:05  UFranke
# R: TOOL sfgnu/sfdiab do not work under VxWorks 6.7/6.8
# M: switch CPU_/TOOL_SAVE defined in rules.mak now
#
# Revision 1.18  2010/10/22 10:55:29  UFranke
# R: toolchain for non FPU CPUs not supported
# M: prepared for TOOL sfgnu/sfdiab
#    set EXCLUDE_RTP = yes in custom.mak because of conflicts with RTP/sfTOOL
#
# Revision 1.17  2010/03/29 16:38:40  cs
# R: generalized ex-/include of RTP support
# M: pass RTP_FLAGS to every compiler call
#
# Revision 1.16  2009/05/29 13:17:38  ufranke
# R: kernel specific compiler switches missing
# M: added ADDED_KERNEL_CFLAGS
#
# Revision 1.15  2009/04/01 15:10:32  ufranke
# R: additional library which contains all mdis/oss/dbg stuff for BSP missing
# M: mdis_all.a added
#
# Revision 1.14  2009/03/31 14:35:45  ufranke
# R: a complete lib for BSP linking would be usefull
# M: added mdis_all.a
#
# Revision 1.13  2008/09/26 16:02:10  ufranke
# VxWorks 6.6 support
#
# Revision 1.12  2006/12/20 14:46:12  ufranke
# added
#  - OSS_HAS_IRQMASKR
#
# Revision 1.11  2006/10/18 14:48:08  cs
# added:
#   + ADDED_CFLAGS compiler switch
#
# Revision 1.10  2006/06/23 16:18:05  ufranke
# moved
#  - clean rule to the top makefile rules.mak
#
# Revision 1.9  2006/03/31 14:29:25  cs
# added directory prefix in lib and obj paths for VxWorks 6.x
#
# Revision 1.8  2006/03/03 18:23:06  cs
# changed prefix LIB$(CPU)$(TOOL) to lib$(CPU)$(TOOL)
#
# Revision 1.7  2005/12/23 15:47:00  UFRANKE
# cosmetics
#
# Revision 1.6  2005/12/07 14:04:27  UFranke
# added
#  + VxWorks 6.0/6.1 support
#
#-----------------------------------------------------------------------------
# (c) Copyright 2000..2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************

CPU  := $(CPU_SAVE)
TOOL := $(TOOL_SAVE)
	
ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	include $(WIND_BASE)/target/h/tool/$(TOOLPATH)/make.$(CPU)$(TOOL)
	MEN_VXWORKS_VERSION=-DMEN_VXWORKS_VERSION=$(MEN_VXWORKS_ENV_VER)
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	MEN_VXWORKS_VERSION=-DMEN_VXWORKS_VERSION=$(MEN_VXWORKS_ENV_VER)
else
	include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
endif

# use GNU AR and NM for diab toolchain too
ifneq (,$(findstring gnu,$(TOOL)))
	GNUAR  := $(AR)
	GNU_NM := $(NM)
else ifneq (,$(findstring llvm,$(TOOL)))
	GNUAR  := $(AR)
	GNU_NM := $(NM)
else
	GNUAR  := ar$(VX_CPU_FAMILY)
	GNU_NM := nm$(VX_CPU_FAMILY)
endif

#-----------------------------------------------------------
# MDIS CONFIGURATION FILES
#
ALL_CONFIG_FILES := ll_entry.o bb_entry.o prog_ref.o desc_ref.o
.PHONY: $(ALL_CONFIG_FILES)

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
TMP_OBJ_DIR	  := $(OBJOUTPUT_DIR)/$(CONFIG_NAME)

OUTPUT_LIB		:= $(CONFIG_NAME).a
DL_OBJECT 		:= $(OBJOUTPUT_DIR)/$(CONFIG_NAME).o


vpath %.c $(THIS_DIR)

#**************************************
#   include pathes
#
INC_DIR         :=  $(MEN_VX_DIR)/INCLUDE/COM
VX_INC_DIR      :=  $(MEN_VX_DIR)/INCLUDE/NATIVE
MEN_VX_INC_DIR  :=  $(VX_INC_DIR)/MEN

ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
INC_DIRS:=-I$(VXWORKS_VSB_DIR)/krnl/h/common \
    -I$(VXWORKS_VSB_DIR)/krnl/h/public       \
    -I$(VXWORKS_VSB_DIR)/krnl/h/system       \
    -I$(VXWORKS_VSB_DIR)/share/h             \
    -I$(INC_DIR)                             \
    -I$(VX_INC_DIR)                          \
    -I$(THIS_DIR)                            \
    $(EXTRA_INCLUDE)
else
INC_DIRS:=-I$(WIND_BASE)/target     \
    -I$(WIND_BASE)/target/h         \
    -I$(WIND_BASE)/target/h/drv/pci \
    -I$(INC_DIR)                    \
    -I$(VX_INC_DIR)                 \
    -I$(THIS_DIR)                   \
    $(EXTRA_INCLUDE)
endif

#**************************************
#   Compiler flags
#
FLAGS           :=$(CC_OPTIM_TARGET) -DCPU=$(CPU)
DEF             :=-DVXWORKS -DMAC_MEM_MAPPED -D_LL_DRV_ -DOSS_HAS_IRQMASKR
ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
DEF             +=-D_VSB_CONFIG_FILE=\"$(VXWORKS_VSB_DIR)/h/config/vsbConfig.h\"
endif

ifndef WARN_LEVEL
    WARN_LEVEL := -w
endif

CFLAGS          :=  $(FLAGS) $(RTP_FLAGS) $(DEF) $(DBG) $(INC_DIRS) $(WARN_LEVEL) \
					$(MEN_VXWORKS_VERSION)

#----------------------------------------------------------------------
# Rules for level #0 of make
#
.PHONY: all buildmsg buildinobjdir build

all: buildmsg buildinobjdir

buildmsg:
	@$(ECHO) +
	@$(ECHO) ++++++++ Building $(DBGSTR) version of config files for $(CONFIG_NAME) +++++++++++

buildinobjdir:
	$(MDBG)$(MAKE) -C $(TMP_OBJ_DIR) --no-print-directory\
	 -f $(CONFIGS_MAK) build


#----------------------------------------------------------------------
# Rules for level #1 of make
#

build: $(ALL_CONFIG_FILES)
	@$(ECHO) Adding $(ALL_CONFIG_FILES) to library $(OUTPUT_LIB)
	$(MDBG)$(GNUAR) -d $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) $(ALL_CONFIG_FILES)
	$(MDBG)$(GNUAR) -r $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) $(ALL_CONFIG_FILES)



# override default buildin rule for .c -> .o files (just to get less messages)
%.o : %.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $<


#----------------------------------------------------------------------
# Rules to build mdis download file
#
.PHONY: download


download: $(DL_OBJECT)

$(DL_OBJECT): $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) \
			  $(LIBOUTPUT_DIR)/mdis.a \
			  $(LIBOUTPUT_DIR)/$(MDIS2_LIB)
	$(MDBG)rm -f $(LIBOUTPUT_DIR)/mdis_all.a
	$(MDBG)$(GNUAR) -q $(LIBOUTPUT_DIR)/mdis_all.a $(OBJOUTPUT_DIR)/*.o
	$(MDBG)$(GNUAR) -d $(LIBOUTPUT_DIR)/mdis_all.a $@
	$(LD) -r -o $@ \
			 $(OBJOUTPUT_DIR)/$(CONFIG_NAME)/prog_ref.o \
			 $(OBJOUTPUT_DIR)/$(CONFIG_NAME)/desc_ref.o \
			 $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) \
			 $(LIBOUTPUT_DIR)/mdis.a \
			 $(LIBOUTPUT_DIR)/$(MDIS2_LIB) \
			 $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) \
			 $(LIBOUTPUT_DIR)/mdis.a \
			 $(LIBOUTPUT_DIR)/$(MDIS2_LIB) \
			 $(LIBOUTPUT_DIR)/$(OUTPUT_LIB) \
			 $(LIBOUTPUT_DIR)/mdis.a \
			 $(LIBOUTPUT_DIR)/$(MDIS2_LIB) \


#----------------------------------------------------------------------
# Rules to build ll_entry.c/bb_entry.c/prog_ref.c/desc_ref.c
#
.PHONY: llbeg bbbeg progbeg descbeg $(ALL_LL_DRIVERS) $(ALL_BB_DRIVERS) \
		$(ALL_LL_TOOLS) $(ALL_COM_TOOLS) $(ALL_DESC) $(ALL_LL_OBJS)

ll_entry.o: $(THIS_DIR)/ll_entry.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $<

$(THIS_DIR)/ll_entry.c: llbeg $(ALL_LL_DRIVERS) $(ALL_LL_OBJS)
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entryend.tcl $(THIS_DIR)/ll_entry.c LL

llbeg:
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entrybeg.tcl $(THIS_DIR)/ll_entry.c LL

$(ALL_LL_DRIVERS):
	@$(ECHO) processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entrydrv.tcl $(MAKE) $(GNU_NM) $(TPL_DIR) \
		$(THIS_DIR)/ll_entry.c $(LL_PATH)/$@ ll_

$(ALL_LL_OBJS):
	@$(ECHO) processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entryobj.tcl $(GNU_NM) $(TPL_DIR) \
		$(THIS_DIR)/ll_entry.c $@ ll_


# bb_entry
bb_entry.o: $(THIS_DIR)/bb_entry.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $<

$(THIS_DIR)/bb_entry.c: bbbeg $(ALL_BB_DRIVERS)
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entryend.tcl $(THIS_DIR)/bb_entry.c BB

bbbeg:
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entrybeg.tcl $(THIS_DIR)/bb_entry.c BB

$(ALL_BB_DRIVERS):
	@$(ECHO) processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)ll_entrydrv.tcl $(MAKE) $(GNU_NM) $(TPL_DIR) \
		$(THIS_DIR)/bb_entry.c $(BB_PATH)/$@ bb_

# prog_ref
prog_ref.o: $(THIS_DIR)/prog_ref.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $<

$(THIS_DIR)/prog_ref.c: progbeg $(ALL_LL_TOOLS) $(ALL_COM_TOOLS) \
						$(ALL_NATIVE_TOOLS)
	$(MDBG)$(WTXTCL) $(TPL_DIR)progrefend.tcl $(THIS_DIR)/prog_ref.c

progbeg:
	$(MDBG)$(WTXTCL) $(TPL_DIR)progrefbeg.tcl $(THIS_DIR)/prog_ref.c

$(ALL_LL_TOOLS):
	@$(ECHO) LL TOOLS processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)progrefent.tcl $(MAKE) $(TPL_DIR) \
		$(THIS_DIR)/prog_ref.c $(LL_PATH)/$@

$(ALL_COM_TOOLS):
	@$(ECHO) COM TOOLS processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)progrefent.tcl $(MAKE) $(TPL_DIR) \
		$(THIS_DIR)/prog_ref.c $(TO_PATH)/$@

$(ALL_NATIVE_TOOLS):
	@$(ECHO) NATIVE TOOLS processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)progrefent.tcl $(MAKE) $(TPL_DIR) \
		$(THIS_DIR)/prog_ref.c $(TO_PATH)/$@


#desc_ref
desc_ref.o: $(THIS_DIR)/desc_ref.c
	@$(ECHO) Compiling $*.c
	$(MDBG)$(CC) $(CFLAGS) $(CPPFLAGS) $(ADDED_CFLAGS) $(ADDED_KERNEL_CFLAGS) -c $<

$(THIS_DIR)/desc_ref.c: descbeg $(ALL_DESC)
	$(MDBG)$(WTXTCL) $(TPL_DIR)descrefend.tcl $(THIS_DIR)/desc_ref.c

descbeg:
	$(MDBG)$(WTXTCL) $(TPL_DIR)descrefbeg.tcl $(THIS_DIR)/desc_ref.c

$(ALL_DESC):
	@$(ECHO) processing $@
	$(MDBG)$(WTXTCL) $(TPL_DIR)descrefent.tcl $(GNU_NM) $(TPL_DIR) $(THIS_DIR)/desc_ref.c $@

