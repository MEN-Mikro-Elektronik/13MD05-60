#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: $
#      $Revision: $
#  
#    Description: Example MDIS configuration for 0918_Siemens_SIL0 F218
#				  
#
#				  Defines the components to include in this MDIS configuration
#                 To be modified by the user     
#       
#   Resulting output files when make has finished:
#		VXWORKS/LIB/MEN/LIB$(CPU)$(TOOL)/mdis.a		
#		VXWORKS/LIB/MEN/LIB$(CPU)$(TOOL)/mdis2.a 	(or mdis2_pci.a)		
#		VXWORKS/LIB/MEN/obj$(CPU)$(TOOL)/mdis_xxx.o	(xxx: name of this dir.)
#
#	Make everything:
#		make				Makes everything for all CPUs, dbg/non-dbg version
#		make CPU=<xxx>		Makes this only for specified CPU
#		make TEST=<yes/no>	Makes only the debug/non-debug version
#
#	Make only some components:
#		make <what> CPU=<xxx> TEST=<yes/no>
#           <what> can be: 
#		    all_ll			Makes only LL drivers					   
#			all_bb			Makes only BBIS drivers					   
#			all_usr_libs	Makes only user libraries
#			all_core		Makes only MDIS core modules
#			all_ll_tools	Makes only low level tools
#			all_com_tools	Makes only common tools
#			all_desc		Makes only descriptors
#			config_files	Makes only the LLDRV/BBDRV table and program refs.
#			download		Makes only the mdis_xxx.o file
#			clean			Deletes all .o files
#
#	Make only specific component:
#		  make <commonmakefile> CPU=<xxx> TEST=<yes/no>      
#
#		  e.g. make CPU=PPC603 TEST=yes	D201/DRIVER/COM/driver.mak
#		  e.g. make CPU=PPC603 TEST=yes	M066/DRIVER/COM/driver.mak
#
#   Switches:
#         MDBG  remove @ for makefile debugging
#
#-----------------------------------------------------------------------------
# $Log: Makefile,v $
# Revision 1.3  2008/09/26 16:01:31  ufranke
# added -G8 for VxWorks 6.6
#
# Revision 1.2  2005/12/07 14:32:25  UFranke
# added
#  + VxWorks 6.0/6.1 support
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000..2005 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************


#----------------------------------------------------------
# Check VxWorks Develpoment Environment Version
ifeq ($(WIND_TOOLS),)
	export MEN_VXWORKS_ENV_VER := 
else
	export TGT_DIR := $(WIND_BASE)/target
	export MEN_VXWORKS_ENV_VER := VXWORKS_6_0
endif

include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

export TOOL 		:= diab
export MDBG			:= @

#-----------------------------------------------------------
# Define CPUs for which you want to build everything
#
ALL_CPUS		:= PPC32 

#-----------------------------------------------------------
# define wether you want to build debug and/or non-debug variants
# yes no	builds both
# yes		builds debug variant
# no		builds non debug variant
ALL_DBGS		:= no

ADDED_CFLAGS    := -D_WRS_KERNEL -DNO_CALLBACK -DNO_SHARED_MEM -Doff_t64=long long -DEXCLUDE_RTP

# Specify compiler options for debug-variants (e.g. -g or -gdwarf)
DEBUG_FLAGS		:= -G8
#-gdwarf -O0

LIB_EXCLUDE_OSS := yes

#----------------------------------------------------------
# Define wether your platform (and BSP) has support for PCI bus
PCI_SUPPORT		:= yes

# include list of MDIS core component
include ../TPL/corecomp.mak

#-----------------------------------------------------------
# Define the descriptors (board and device descriptors)
# to build
# 
# Copy the meta descriptors from the driver directories to
# this directory
#
ALL_DESC		:= system


#-----------------------------------------------------------
# Define Low level drivers to build
# Specify the path to their common makefiles here 
# (path relative to VXWORKS/DRIVERS/MDIS_LL)
#
# Always use swapped driver variants for devices on CPCI!
#
ALL_LL_DRIVERS	:= \
					M099/DRIVER/COM/driver_sw.mak

#-----------------------------------------------------------
# Define BBIS drivers to build
# Specify the path to their common makefiles here 
# (path relative to VXWORKS/DRIVERS/BBIS)
#
# Note: not all drivers may make sense for your platform
#
ALL_BB_DRIVERS 	:= \
					ISA/DRIVER/COM/driver.mak \
					CHAMELEON/DRIVER/COM/driver.mak \


#-----------------------------------------------------------
# Define user libraries to build
# Specify the path to their common makefiles here 
# (path relative to VXWORKS/LIBSRC)
#
ALL_USR_LIBS 	:= SMB2/COM/library.mak

# MSCAN_API/COM/library.mak


#-----------------------------------------------------------
# Define LL driver specific examples and tools to build
# Specify the path to their common makefiles here 
# (path relative to VXWORKS/DRIVERS/MDIS_LL)
#
ALL_LL_TOOLS	:= \
					M099/TOOLS/M99_LATENCY/COM/program.mak 

#					MSCAN/TOOLS/REPEATER_TEST/COM/program.mak \

#-----------------------------------------------------------
# Define common tools to build
# Specify the path to their common makefiles here 
# (path relative to VXWORKS/TOOLS)
#
ALL_COM_TOOLS	:= \
					MDIS_API/M_MOD_ID/COM/program.mak \
					MDIS_API/M_REV_ID/COM/program.mak \


ALL_NATIVE_TOOLS = NATIVE/PCISCANNER/program.mak


# include rules to build the components
include ../TPL/rules.mak

