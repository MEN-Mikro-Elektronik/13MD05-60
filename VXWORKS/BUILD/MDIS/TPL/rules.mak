#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2010/10/22 15:08:21 $
#      $Revision: 1.23 $
#
#    Description: Defines top level rules for MDIS components
#                 This file should be included at the bottom of the
#				  configurations makefile
#
#       Switches:
#		Rules:
#
#-----------------------------------------------------------------------------
#
#
#  22.11.2015ts: added correct storage for e500v2 toolchains
#
#  ---- ts@men: cvs maintenance ended ----
# $Log: rules.mak,v $
# Revision 1.23  2010/10/22 15:08:21  UFranke
# R: TOOL sfgnu/sfdiab do not work under VxWorks 6.7/6.8
# M: switch CPU_SAVE, TOOL_SAVE added
#
# Revision 1.22  2010/10/22 10:56:27  UFranke
# R: toolchain for non FPU CPUs not supported
# M: prepared for TOOL sfgnu/sfdiab - added TOOLPATH
#    set EXCLUDE_RTP = yes in custom.mak because of conflicts with RTP/sfTOOL
#
# Revision 1.21  2010/03/29 16:44:15  cs
# R: generalized ex-/include of RTP support
# M: pass RTP_FLAGS to subsequent .mak files,
#    based on EXCLUDE_RTP and VxWorks version
#
# Revision 1.20  2009/05/29 13:21:03  ufranke
# R: kernel/RTP specific compiler switches missing
# M: added ADDED_KERNEL_CFLAGS and ADDED_RTP_CFLAGS
# R: possibility to exclude oss, dbg missing if already integrated in BSP
# M: switches LIB_EXCLUDE_OSS and LIB_EXCLUDE_DBG added
#    to exclude set corresponding switch to yes
#
# Revision 1.19  2009/03/31 14:38:08  ufranke
# R: VxWorks 6.6 SMP not stable without BSP changes and patches
# M: SMP support for VxWorks 6.7 only
#
# Revision 1.18  2008/09/26 16:02:17  ufranke
# VxWorks 6.6 support
#
# Revision 1.17  2008/01/28 14:32:22  ufranke
# fixed
#  - VxWorks < 6.0 custom.mak template file creation
#
# Revision 1.16  2008/01/25 14:11:39  ufranke
# added
#  ALL_CORE
#
# Revision 1.15  2008/01/23 14:36:00  ufranke
# cosmetics
#
# Revision 1.14  2008/01/23 14:22:32  ufranke
# added
#  + reading custom.mak
#    users can add special drivers and so one permanently
#    in custom.mak
#    ( cause: MDIS wizard removes unknown settings in Makefile )
#
# Revision 1.13  2006/12/21 13:48:17  ufranke
# fixed
# - make clean
#   replaced $(RM) with rm for Tornado 2.2.1 because vxrm is'nt compatible with rm
#
# Revision 1.12  2006/10/18 14:44:49  cs
# added:
#   + ADDED_CFLAGS
#   + rules for ALL_KERNEL_LIBS
#   + adapt ADDED_CFLAGS for strict-aliasing and out of order execution
#     problems with VxWorks 6.3
# changed:
#   - rule for ALL_USR_LIBS: compile/link libs also into libmdis_rtp
#
# Revision 1.11  2006/07/13 13:41:31  cs
# fixed mdis objs/libs tree for VxW < 6.0
#     (.../MEN/vxworks5x instead of .../MENvxworks5x)
#
# Revision 1.10  2006/06/23 16:17:12  ufranke
# changed
#  - mdis objs/libs now under a seperate tree
#    i.e. LIB/MEN/vxworks-6.2_mdis_MEN_PP01/libPPC603gnu
#  - this tree will be deleted completely by make clean
# fixed
#  - unsave make clean if working with more configurations
#    with the same CPU type
#
# Revision 1.9  2006/03/31 14:28:44  cs
# added
#   + directory prefix in lib and obj paths for VxWorks 6.x
#   + History log in file
# don't use back slashes in paths for x86-win32 and VxWorks 6.x
# Copyright line changed
#
# Revision 1.8  03.03.2006 18:23:10 by cs
# changed prefix LIB$(CPU)$(TOOL) to lib$(CPU)$(TOOL)
#
# Revision 1.7  23.12.2005 15:47:06 by UFRANKE
# added
#  + MDIS_API_RTP
#  + USR_OSS_RTP
#
# Revision 1.6  04.05.2005 14:44:12 by kp
# added echovars rule (makefile debugging)
#
# Revision 1.5  23.07.2004 14:47:50 by ufranke
# added
#  + native tools
#
# Revision 1.4  13.05.2004 14:59:26 by UFranke
# added
#  + EXTRA_INCLUDE
#
# Revision 1.3  11.12.2001 10:53:18 by Franke
# added debug switch
#
# Revision 1.2  28.03.2000 14:42:55 by Franke
# changed for Tornado 1.0.1 PS, DIRUP '\\'
#
# Revision 1.1  17.03.2000 15:08:47 by kp
# Initial Revision
#
#-----------------------------------------------------------------------------
# (c) Copyright 2000..2999 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************
ifndef TEST
	TEST := no
endif

# select tool path for no FPU ->sfgnu/sfdiab support
# ts: tool path is the folder name under target/h folder.
# new search criteria: TOOL contains either 'gnu' (=TOOLPATH gnu) or not (=TOOLPATH diab)
ifneq (,$(findstring gnu,$(TOOL)))
	export TOOLPATH	:= gnu
else ifneq (,$(findstring llvm,$(TOOL)))
	export TOOLPATH := llvm
else
    export TOOLPATH	:= diab
endif

#--------------------------------------------------------
# save TOOL and CPU because defs.default overwrites it
export TOOL_SAVE := $(TOOL)
export CPU_SAVE  := $(CPU)

export ALL_LL_DRIVERS
export ALL_BB_DRIVERS
export ALL_LL_TOOLS
export ALL_COM_TOOLS
export ALL_NATIVE_TOOLS
export ALL_DESC
export ALL_LL_OBJS
export DEBUG_FLAGS
export ADDED_CFLAGS
export EXTRA_INCLUDE
export TOOL
export TOOL_SAVE

export MDIS2_LIB      := mdis2$(PCI_EXT).a
export CONFIG_NAME    := mdis_$(notdir $(THIS_DIR))
export TPL_DIR	      := $(MEN_VX_DIR)/BUILD/MDIS/TPL/
export COMP_MAK       := $(TPL_DIR)/component.mak
export RTP_COMP_MAK   := $(TPL_DIR)/rtp_component.mak
export DESC_MAK       := $(TPL_DIR)/desc.mak
export CONFIGS_MAK    := $(TPL_DIR)/configs.mak
export USR_LIB_MAK    := $(TPL_DIR)/usr_lib.mak

ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	export MEN_VXVERSUB_DIR=$(WIND_PLATFORM)
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	export MEN_VXVERSUB_DIR=$(WIND_PLATFORM)
else	
	export MEN_VXVERSUB_DIR=vxworks5x
endif
export MEN_OBJ_DIR_PREFIX    := $(MEN_VX_DIR)/LIB/MEN/$(MEN_VXVERSUB_DIR)_$(CONFIG_NAME)


MAKEIT 			= $(MDBG)$(MAKE) --no-print-directory

ifeq "$(origin CPU)" "command line"
	ALL_CPUS := $(CPU)
endif

ifeq "$(origin TEST)" "command line"
	ALL_DBGS := $(TEST)
endif

#--------------------------------------
# determine path names for object files
ifeq ($(TEST),yes)
    TESTSUFFIX := test
else
    TESTSUFFIX :=
endif
OBJ_DIR 		  := $(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL)$(TESTSUFFIX)
RTP_OBJ_DIR 	  := $(MEN_OBJ_DIR_PREFIX)/obj$(CPU)$(TOOL)$(TESTSUFFIX)/RTP
LIB_DIR 		  := $(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL)$(TESTSUFFIX)
RTP_LIB_DIR 	  := $(MEN_OBJ_DIR_PREFIX)/lib$(CPU)$(TOOL)$(TESTSUFFIX)/RTP
CONFIG_OBJ_DIR	  := $(OBJ_DIR)/$(CONFIG_NAME)

ifeq ($(WIND_HOST_TYPE),x86-win32)
  ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	MEN_OBJ_DIR_PREFIX_NAT := $(subst \,/,$(MEN_OBJ_DIR_PREFIX))
	LIB_DIR_NAT 	:= $(subst \,/,$(LIB_DIR))
	OBJ_DIR_NAT 	:= $(subst \,/,$(OBJ_DIR))
	CONFIG_OBJ_DIR_NAT := $(subst \,/,$(CONFIG_OBJ_DIR))
	DIRUP := ../
	PS := /
  else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	MEN_OBJ_DIR_PREFIX_NAT := $(subst \,/,$(MEN_OBJ_DIR_PREFIX))
	LIB_DIR_NAT 	:= $(subst \,/,$(LIB_DIR))
	OBJ_DIR_NAT 	:= $(subst \,/,$(OBJ_DIR))
	CONFIG_OBJ_DIR_NAT := $(subst \,/,$(CONFIG_OBJ_DIR))
	DIRUP := ../
	PS := /
  else
	MEN_OBJ_DIR_PREFIX_NAT := $(subst /,\,$(MEN_OBJ_DIR_PREFIX))
	LIB_DIR_NAT		:= $(subst /,\,$(LIB_DIR))
	OBJ_DIR_NAT		:= $(subst /,\,$(OBJ_DIR))
	CONFIG_OBJ_DIR_NAT := $(subst /,\,$(CONFIG_OBJ_DIR))
	DIRUP := ..$(subst /,\,/)
	PS := $(subst /,\,/)
  endif
else
	MEN_OBJ_DIR_PREFIX_NAT := $(MEN_OBJ_DIR_PREFIX)
	LIB_DIR_NAT 	:= $(LIB_DIR)
	OBJ_DIR_NAT 	:= $(OBJ_DIR)
	CONFIG_OBJ_DIR_NAT := $(CONFIG_OBJ_DIR)
	DIRUP := ../
	PS := /
endif

# Fix for VxWorks >= 6.3:
# With inlining of static functions and reordering of instructions
# lots of code doesn't work!!!
#ifneq (,$(findstring gnu,$(TOOL)))
#	ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
#		ifeq (,$(findstring 6.2,$(WIND_PLATFORM)))
#			ADDED_CFLAGS += -fno-strict-aliasing -fno-schedule-insns  -fno-schedule-insns2
#			VXW63_WARN_STR = "+++ ATTENTION for VxWorks>=6.3: ADDED_CFLAGS now $(ADDED_CFLAGS)"
#		endif # WIND_PLATFORM !=6.2
#	endif #MEN_VXWORKS_ENV_VER == 6_0
#endif # TOOL==gnu

# disable diab compiler warnings
# (dcc:1604): Useless assignment to variable error. Assigned value not used.
# (dcc:1517): function internFunc is never used
# (dcc:1772): possible redundant expression
# info (etoa:4193): zero used for undefined preprocessing identifier
# info (etoa:4826): parameter ... was never referenced
ifeq ($(TOOL),diab)
	ADDED_CFLAGS +=-ei1604
	ADDED_CFLAGS +=-ei1517
	ADDED_CFLAGS +=-ei1516
	ADDED_CFLAGS +=-ei1772
	ADDED_CFLAGS +=-ei4193
	ADDED_CFLAGS +=-ei4826
endif
ifeq ($(TOOL),sfdiab)
	ADDED_CFLAGS +=-ei1604
	ADDED_CFLAGS +=-ei1517
	ADDED_CFLAGS +=-ei1516
	ADDED_CFLAGS +=-ei1772
	ADDED_CFLAGS +=-ei4193
	ADDED_CFLAGS +=-ei4826
endif


#----------------------------------------
# Rules build/clean
#
ifneq ($(RULE),clean)
	# build
.PHONY: buildall builddbgs all_ll all_bb all_usr_libs all_core all_ll_tools \
	   	all_com_tools all_native_tools all_desc config_files download \
	   	all_kernel_libs \
		$(ALL_CPUS) $(ALL_DBGS) $(ALL_LL_DRIVERS) \
		$(ALL_BB_DRIVERS) $(ALL_USR_LIBS) $(ALL_KERNEL_LIBS) $(ALL_CORE)

buildall: custom.mak $(ALL_CPUS)

#----------------------------------------
# custom.mak
#
ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
custom.mak:
	@$(ECHO) "==> custom.mak not exist: creating template custom.mak"
	@$(ECHO) "# custom.mak:"													        		  > custom.mak
	@$(ECHO) "CUSTOM_MAK_MSG = custom.mak has been read successfully"                             >> custom.mak
	@$(ECHO) "#  - please edit this files to add something not covered but removed by mdiswizvx"  >> custom.mak
	@$(ECHO) "#"  																			  	  >> custom.mak
	@$(ECHO) "#i.e."  																		  	  >> custom.mak
	@$(ECHO) "# EXCLUDE_RTP = yes"                                       						  >> custom.mak
	@$(ECHO) "# ALL_COM_TOOLS +="                                       						  >> custom.mak
	@$(ECHO) "# ALL_LL_TOOLS +="                                        						  >> custom.mak
	@$(ECHO) "# ALL_NATIVE_TOOLS +="                                        					  >> custom.mak
	@$(ECHO) "# ALL_USR_LIBS +="                                        						  >> custom.mak
	@$(ECHO) "# LIB_EXCLUDE_OSS = yes"                                        			  		  >> custom.mak
	@$(ECHO) "# LIB_EXCLUDE_DBG = yes"                                        			  		  >> custom.mak
	@$(ECHO) "# ALL_CORE += SMB2/COM/library.mak"                       						  >> custom.mak
	@$(ECHO) "# ADDED_CFLAGS +="                                        						  >> custom.mak
	@$(ECHO) "# ADDED_KERNEL_CFLAGS ="                                        			  		>> custom.mak
	@$(ECHO) "# ADDED_RTP_CFLAGS ="                                        				  		>> custom.mak
	@$(ECHO) "# ALL_LL_DRIVERS +="                                      						  >> custom.mak
	@$(ECHO) "# ALL_BB_DRIVERS += SMBPCI/DRIVER/COM/driver_16z001.mak"  						  >> custom.mak
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
custom.mak:
	@$(ECHO) "==> custom.mak not exist: creating template custom.mak"
	@$(ECHO) "# custom.mak:"													        		  > custom.mak
	@$(ECHO) "CUSTOM_MAK_MSG = custom.mak has been read successfully"                             >> custom.mak
	@$(ECHO) "#  - please edit this files to add something not covered but removed by mdiswizvx"  >> custom.mak
	@$(ECHO) "#"  																			  	  >> custom.mak
	@$(ECHO) "#i.e."  																		  	  >> custom.mak
	@$(ECHO) "# EXCLUDE_RTP = yes"                                       						  >> custom.mak
	@$(ECHO) "# ALL_COM_TOOLS +="                                       						  >> custom.mak
	@$(ECHO) "# ALL_LL_TOOLS +="                                        						  >> custom.mak
	@$(ECHO) "# ALL_NATIVE_TOOLS +="                                        					  >> custom.mak
	@$(ECHO) "# ALL_USR_LIBS +="                                        						  >> custom.mak
	@$(ECHO) "# LIB_EXCLUDE_OSS = yes"                                        			  		  >> custom.mak
	@$(ECHO) "# LIB_EXCLUDE_DBG = yes"                                        			  		  >> custom.mak
	@$(ECHO) "# ALL_CORE += SMB2/COM/library.mak"                       						  >> custom.mak
	@$(ECHO) "# ADDED_CFLAGS +="                                        						  >> custom.mak
	@$(ECHO) "# ADDED_KERNEL_CFLAGS ="                                        			  		>> custom.mak
	@$(ECHO) "# ADDED_RTP_CFLAGS ="                                        				  		>> custom.mak
	@$(ECHO) "# ALL_LL_DRIVERS +="                                      						  >> custom.mak
	@$(ECHO) "# ALL_BB_DRIVERS += SMBPCI/DRIVER/COM/driver_16z001.mak"  						  >> custom.mak	
else #MEN_VXWORKS_ENV_VER == VXWORKS_6_0
custom.mak:
	@$(ECHO) # custom.mak:													        		  	> custom.mak
	@$(ECHO) CUSTOM_MAK_MSG = custom.mak has been read successfully                             >> custom.mak
	@$(ECHO) #  - please edit this files to add something not covered but removed by mdiswizvx  >> custom.mak
	@$(ECHO) #  																			  	>> custom.mak
	@$(ECHO) #i.e.  																		  	>> custom.mak
	@$(ECHO) # ALL_COM_TOOLS +=                                       						  	>> custom.mak
	@$(ECHO) # ALL_LL_TOOLS +=                                        						  	>> custom.mak
	@$(ECHO) # ALL_NATIVE_TOOLS +=                                        						>> custom.mak
	@$(ECHO) # ALL_USR_LIBS +=                                        						  	>> custom.mak
	@$(ECHO) # LIB_EXCLUDE_OSS = yes                                        			    >> custom.mak
	@$(ECHO) # LIB_EXCLUDE_DBG = yes                                        			    >> custom.mak
	@$(ECHO) # ALL_CORE += SMB2/COM/library.mak                       						  	>> custom.mak
	@$(ECHO) # ADDED_CFLAGS +=                                        						  	>> custom.mak
	@$(ECHO) # ALL_LL_DRIVERS +=                                      						  	>> custom.mak
	@$(ECHO) # ALL_BB_DRIVERS += SMBPCI/DRIVER/COM/driver_16z001.mak  						  	>> custom.mak
endif #MEN_VXWORKS_ENV_VER == VXWORKS_6_0

include custom.mak

export EXCLUDE_RTP
export LIB_EXCLUDE_OSS
export LIB_EXCLUDE_DBG
export ADDED_KERNEL_CFLAGS
export ADDED_RTP_CFLAGS


ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	ifeq ($(EXCLUDE_RTP),yes)
		RTP_FLAGS := -DEXCLUDE_RTP
	else #EXCLUDE_RTP==yes
		RTP_FLAGS := -DINCLUDE_RTP
	endif #EXCLUDE_RTP==yes
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	ifeq ($(EXCLUDE_RTP),yes)
		RTP_FLAGS := -DEXCLUDE_RTP
	else #EXCLUDE_RTP==yes
		RTP_FLAGS := -DINCLUDE_RTP
	endif #EXCLUDE_RTP==yes	
else #MEN_VXWORKS_ENV_VER == VXWORKS_6_0
	RTP_FLAGS := -DEXCLUDE_RTP
endif #MEN_VXWORKS_ENV_VER == VXWORKS_6_0

export RTP_FLAGS

builddbgs: $(ALL_DBGS)

$(ALL_CPUS):
	@$(ECHO) $(CUSTOM_MAK_MSG)
	@$(ECHO) $(VXW63_WARN_STR)
	$(MAKEIT) CPU=$@ RULE=$(RULE) builddbgs

$(ALL_DBGS):
	$(MAKEIT) CPU=$(CPU) RULE=$(RULE) TEST=$@ buildfordbg

clean:
	$(MAKEIT) RULE=clean

else
	# clean
.PHONY: cleanall

cleanall: $(ALL_CPUS)

$(ALL_CPUS):
	$(MAKEIT) CPU=$@ RULE=$(RULE) cleancpu

cleancpu:
	@$(ECHO) +++ Removing OBJ/LIB directories of configuration $(CONFIG_NAME) +++
ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	$(MDBG)$(RM) -f ll_entryi.h
	$(MDBG)$(RM) -f bb_entryi.h
	$(MDBG)$(RM) -f desc_refi.h
	$(MDBG)$(RM) -f ll_entry.c
	$(MDBG)$(RM) -f bb_entry.c
	$(MDBG)$(RM) -f prog_ref.c
	$(MDBG)$(RM) -f desc_ref.c
	$(MDBG)$(RM) -f -r $(MEN_OBJ_DIR_PREFIX_NAT)
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	$(MDBG)$(RM) -f ll_entryi.h
	$(MDBG)$(RM) -f bb_entryi.h
	$(MDBG)$(RM) -f desc_refi.h
	$(MDBG)$(RM) -f ll_entry.c
	$(MDBG)$(RM) -f bb_entry.c
	$(MDBG)$(RM) -f prog_ref.c
	$(MDBG)$(RM) -f desc_ref.c
	$(MDBG)$(RM) -f -r $(MEN_OBJ_DIR_PREFIX_NAT)	
else
	$(MDBG)rm -f ll_entryi.h
	$(MDBG)rm -f bb_entryi.h
	$(MDBG)rm -f desc_refi.h
	$(MDBG)rm -f ll_entry.c
	$(MDBG)rm -f bb_entry.c
	$(MDBG)rm -f prog_ref.c
	$(MDBG)rm -f desc_ref.c
	$(MDBG)rm -f -r $(MEN_OBJ_DIR_PREFIX_NAT)
	$(MDBG)if     exist $(MEN_OBJ_DIR_PREFIX_NAT) @$(ECHO) *** Error removing $(MEN_OBJ_DIR_PREFIX_NAT)
	$(MDBG)if NOT exist $(MEN_OBJ_DIR_PREFIX_NAT) @$(ECHO) Directory $(MEN_OBJ_DIR_PREFIX_NAT) removed
endif #VXWORKS_6_0

endif # build/clean

buildfordbg: all

all:			all_ll all_ll_objs all_bb all_core \
				all_kernel_libs all_usr_libs \
				all_ll_tools all_com_tools all_native_tools \
				all_desc config_files download \


all_ll:				$(ALL_LL_DRIVERS)
all_ll_objs:		$(ALL_LL_OBJS)
all_bb:				$(ALL_BB_DRIVERS)
all_core: 			$(ALL_CORE) $(MK_CORE) $(OSS_CORE) $(DBG_CORE) $(RTP_CORE)
all_usr_libs: 		$(ALL_USR_LIBS)
all_kernel_libs:	$(ALL_KERNEL_LIBS)
all_ll_tools:		$(ALL_LL_TOOLS)
all_com_tools:		$(ALL_COM_TOOLS)
all_native_tools:	$(ALL_NATIVE_TOOLS)
all_desc:			$(ALL_DESC)


$(ALL_LL_DRIVERS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LL_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX=ll_ \
		OUTPUT_LIB1=mdis.a \
		LLDRV=-D_LL_DRV_

$(ALL_LL_OBJS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX=ll_ \
		OUTPUT_LIB1=mdis.a \
		BINARY_OBJ=$@

$(ALL_BB_DRIVERS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(BB_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX=bb_ \
		OUTPUT_LIB1=mdis.a \
		LLDRV=-D_LL_DRV_

$(ALL_CORE):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a \
		SINGLE_FILES_TO_LIB=yes

MK/library.mak:
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis2.a \
		SINGLE_FILES_TO_LIB=yes

MK/library_pci.mak:
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis2_pci.a \
		SINGLE_FILES_TO_LIB=yes

OSS/library.mak:
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis2.a \
		OUTPUT_OWN_LIB=yes \
		SINGLE_FILES_TO_LIB=yes


OSS/library_pci.mak:
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis2_pci.a \
		OUTPUT_OWN_LIB=yes \
		SINGLE_FILES_TO_LIB=yes


DBG/library.mak:
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a  \
		OUTPUT_OWN_LIB=yes \
		SINGLE_FILES_TO_LIB=yes

MDIS_API_RTP/library.mak:
	$(MAKEIT) -f $(RTP_COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=libmdis_rtp.a \
		SINGLE_FILES_TO_LIB=yes

USR_OSS_RTP/library.mak:
	$(MAKEIT) -f $(RTP_COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=libusr_oss_rtp.a \
		SINGLE_FILES_TO_LIB=yes

$(ALL_KERNEL_LIBS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a \
		SINGLE_FILES_TO_LIB=yes

$(ALL_USR_LIBS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) \
		COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a \
		SINGLE_FILES_TO_LIB=yes
  ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
    ifneq ($(EXCLUDE_RTP), yes)
		$(MAKEIT) -f $(RTP_COMP_MAK) $(RULE) \
			COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
			TEST=$(TEST) COMP_PREFIX= \
			OUTPUT_LIB1=libmdis_rtp.a \
			SINGLE_FILES_TO_LIB=yes
    endif #EXCLUDE_RTP != yes
  else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
    ifneq ($(EXCLUDE_RTP), yes)
		$(MAKEIT) -f $(RTP_COMP_MAK) $(RULE) \
			COMMAKE=$(LS_PATH)/$@ CPU=$(CPU)\
			TEST=$(TEST) COMP_PREFIX= \
			OUTPUT_LIB1=libmdis_rtp.a \
			SINGLE_FILES_TO_LIB=yes
    endif #EXCLUDE_RTP != yes	
  endif #MEN_VXWORKS_ENV_VER == 6_0

$(ALL_LL_TOOLS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE) VX_MAIN_REPLACING=1 \
		COMMAKE=$(LL_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a

$(ALL_COM_TOOLS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE)  VX_MAIN_REPLACING=1 \
		COMMAKE=$(TO_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a

$(ALL_NATIVE_TOOLS):
	$(MAKEIT) -f $(COMP_MAK) $(RULE)  VX_MAIN_REPLACING=1 \
		COMMAKE=$(TO_PATH)/$@ CPU=$(CPU)\
		TEST=$(TEST) COMP_PREFIX= \
		OUTPUT_LIB1=mdis.a

$(ALL_DESC):
	$(MAKEIT) -f $(DESC_MAK) $(RULE) DESC=$@ TEST=$(TEST) CPU=$(CPU)

config_files:
	$(MAKEIT) -f $(CONFIGS_MAK) $(RULE) TEST=$(TEST) CPU=$(CPU)

download:
ifneq ($(RULE),clean)
	$(MAKEIT) -f $(CONFIGS_MAK) TEST=$(TEST) CPU=$(CPU) download
endif

#------------------------------
# create output directories
#
.PHONY: mkobjprefdir mkobjdir mktmpobjdir mklibdir

$(ALL_LL_DRIVERS) $(ALL_LL_OBJS) $(ALL_BB_DRIVERS) $(ALL_CORE) $(MK_CORE) $(OSS_CORE) $(DBG_CORE) $(ALL_USR_LIBS) $(ALL_KERNEL_LIBS) $(ALL_LL_TOOLS) $(ALL_COM_TOOLS) $(ALL_NATIVE_TOOLS) $(ALL_DESC) $(RTP_CORE): mkobjprefdir mkobjdir mktmpobjdir mklibdir

mkobjprefdir: $(MEN_OBJ_DIR_PREFIX)/anchor

$(MEN_OBJ_DIR_PREFIX)/anchor:
	- @ mkdir -p $(MEN_OBJ_DIR_PREFIX_NAT)
	@$(ECHO) > $(MEN_OBJ_DIR_PREFIX_NAT)$(PS)anchor
	@$(ECHO) Directory $(MEN_OBJ_DIR_PREFIX) created

mkobjdir: $(OBJ_DIR)/anchor

$(OBJ_DIR)/anchor:
	- @ mkdir -p $(OBJ_DIR_NAT)
	@$(ECHO) > $(OBJ_DIR_NAT)$(PS)anchor
	@$(ECHO) Directory $(OBJ_DIR) created

mktmpobjdir: $(CONFIG_OBJ_DIR)/anchor

$(CONFIG_OBJ_DIR)/anchor:
	- @ mkdir -p $(CONFIG_OBJ_DIR_NAT)
	@$(ECHO) > $(CONFIG_OBJ_DIR_NAT)$(PS)anchor
	@$(ECHO) Directory $(CONFIG_OBJ_DIR) created

mklibdir: $(LIB_DIR)/anchor

$(LIB_DIR)/anchor:
	- @ mkdir -p $(LIB_DIR_NAT)
	@$(ECHO) > $(LIB_DIR_NAT)$(PS)anchor
	@$(ECHO) Directory $(LIB_DIR) created


echovars:
	@$(ECHO) "ALL_CPUS=<$(ALL_CPUS)>"
	@$(ECHO) "CPU=<$(CPU)>"
	@$(ECHO) "TOOL=<$(TOOL)>"
	@$(ECHO) "LIB_DIR_NAT=<$(LIB_DIR_NAT)>"
	@$(ECHO) "THIS_DIR=<$(THIS_DIR)> cwd=$(cwd)"
	@$(ECHO) "CONFIG_NAME=<$(CONFIG_NAME)>"
	@$(ECHO) "CONFIG_OBJ_DIR=<$(CONFIG_OBJ_DIR)>"
	@$(ECHO) "CONFIG_OBJ_DIR_NAT=<$(CONFIG_OBJ_DIR_NAT)>"




