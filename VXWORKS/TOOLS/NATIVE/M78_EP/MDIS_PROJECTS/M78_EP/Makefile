# MDIS project makefile
# Generated by mdiswiz
# 1.7.015-vxworks-1 Build Dec 20 2006 14:09:38

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

MEN_VX_DIR = s:/work_vxM78/VXWORKS

# Defines the compiler toolchain which is used to compile the
# MDIS modules

TOOL = gnu

# Additional compiler flags, e.g. -mlongcall
# String is passed to compiler as seen here

ADDED_CFLAGS = 

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

ALL_CPUS = PPC603

include $(MEN_VX_DIR)/BUILD/MDIS/TPL/corecomp.mak

ALL_LL_DRIVERS = \
	M078/DRIVER/COM/driver_sw.mak \
	M072/DRIVER/COM/driver_pre_sw.mak \
	M027/DRIVER/COM/driver_sw.mak \
	M036/DRIVER/COM/driver_sw.mak \
	M008/DRIVER/COM/driver_sw.mak \
	MMODPRG/DRIVER/COM/driver_sw.mak

ALL_BB_DRIVERS = \
	A12/DRIVER/COM/driver.mak \
	D203/DRIVER/COM/driver_sw.mak

ALL_USR_LIBS = IB_API/COM/library.mak

ALL_LL_TOOLS = \
	M078/EXAMPLE/M78_SIMP/COM/program.mak \
	M078/TEST/M78_ACTEST/COM/program.mak \
	M078/TEST/M78_TRIGTEST/COM/program.mak \
	M078/TEST/M78_MULTITRIG/COM/program.mak \
	M078/TOOLS/M78_EEPOT/COM/program.mak \
	M078/TOOLS/M78_PLL/COM/program.mak \
	M072/EXAMPLE/M72_FREQ/COM/program.mak \
	M072/EXAMPLE/M72_OUT/COM/program.mak \
	M072/EXAMPLE/M72_PULSE/COM/program.mak \
	M072/EXAMPLE/M72_SINGLE/COM/program.mak \
	M072/EXAMPLE/M72_TIMER/COM/program.mak \
	M072/EXAMPLE/M72_PERIOD/COM/program.mak \
	M072/EXAMPLE/M72_PRETRIG/COM/program.mak \
	M072/TOOLS/M72_COUNT/COM/program.mak \
	M027/EXAMPLE/M27_SIMP/COM/program.mak \
	M027/TOOLS/M27_RW/COM/program.mak \
	M036/EXAMPLE/M36_SIMP/COM/program.mak \
	M008/EXAMPLE/M8_SIMP/COM/program.mak 

ALL_COM_TOOLS = \
	MDIS_API/M_ERRSTR/COM/program.mak \
	MDIS_API/M_GETBLOCK/COM/program.mak \
	MDIS_API/M_GETSTAT/COM/program.mak \
	MDIS_API/M_GETSTAT_BLK/COM/program.mak \
	MDIS_API/M_MOD_ID/COM/program.mak \
	MDIS_API/M_OPEN/COM/program.mak \
	MDIS_API/M_READ/COM/program.mak \
	MDIS_API/M_REV_ID/COM/program.mak \
	MDIS_API/M_SETBLOCK/COM/program.mak \
	MDIS_API/M_SETSTAT/COM/program.mak \
	MDIS_API/M_SETSTAT_BLK/COM/program.mak \
	MDIS_API/M_WRITE/COM/program.mak \
	MDIS_API/M_MOD_ID/COM/program.mak \
	MDIS_API/M_REV_ID/COM/program.mak \

ALL_NATIVE_DRIVERS = 

ALL_NATIVE_LIBS = 

ALL_NATIVE_TOOLS = 

ALL_DESC = system

include $(MEN_VX_DIR)/BUILD/MDIS/TPL/rules.mak

