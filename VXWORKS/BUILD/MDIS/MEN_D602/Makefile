# MDIS project makefile
# Generated by mdiswiz
# 1.8.017-vxworks-3 Build May 14 2009 09:16:38

# Check VxWorks Develpoment Environment Version

ifeq ($(WIND_TOOLS),)
export MEN_VXWORKS_ENV_VER := 

else
export TGT_DIR := $(WIND_BASE)/target

export MEN_VXWORKS_ENV_VER := VXWORKS_6_0

endif

include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

# The directory where you have installed the MDIS system
# package for VxWorks, e.g. /MEN/VXWORKS
# note: FORWARD SLASHES '/' needed

MEN_VX_DIR = /home/tschnuer/work/VXWORKS
# MEN_VX_DIR = s:/work_MDIS/VXWORKS

# Defines the compiler toolchain which is used to compile the
# MDIS modules

TOOL = gnu

# Additional compiler flags, e.g. -mlongcall
# String is passed to compiler as seen here

ADDED_CFLAGS = -DEXCLUDE_RTP

PCI_SUPPORT		:= yes
#no

# Defines the system type:
# uniprocessor (UP) or a symmetrical multi-processor (SMP)
# system

ADDED_CFLAGS += -D_UP_COMPATIBLE_ONLY -DDBG -DMAC_MEM_MAPPED

# Defines wether to build and install the release (no) or debug
# (yes) versions of the modules. The debug version of the
# modules issue debug messages using DBG_Write for trouble
# shooting. The message display function is DBG_Show().

ALL_DBGS = yes

# Compiler Switches for Debug Informations i.e. -g or -gdwarf
# or -E for preprocessor -S for assembler output

DEBUG_FLAGS = -Wall

# CPU has PCIbus yes or no

PCI_SUPPORT = yes

# Makefile debug output disable (@) or empty to enable

export MDBG = @

# Defines the CPU

ALL_CPUS = PPC604

include $(MEN_VX_DIR)/BUILD/MDIS/TPL/corecomp.mak

ALL_LL_DRIVERS = M022/DRIVER/COM/driver.mak \
				 Z017/DRIVER/COM/driver.mak \

ALL_BB_DRIVERS = \
			   CHAMELEON/DRIVER/COM/driver.mak 


ALL_USR_LIBS = 

ALL_CORE_LIBS = no

ALL_LL_TOOLS = M022/EXAMPLE/M22_SIMP/COM/program.mak \
			   M022/TEST/M22_MAIN/COM/program.mak

ALL_COM_TOOLS = 

ALL_NATIVE_DRIVERS = 

ALL_NATIVE_LIBS = 

ALL_NATIVE_TOOLS = 

ALL_DESC = system

include $(MEN_VX_DIR)/BUILD/MDIS/TPL/rules.mak

