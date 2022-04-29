# custom.mak:
CUSTOM_MAK_MSG = custom.mak has been read successfully
#  - please edit this files to add something not covered but removed by mdiswizvx
#
#i.e.
EXCLUDE_RTP = yes
# ALL_COM_TOOLS +=
# ALL_LL_TOOLS +=
# ALL_NATIVE_TOOLS +=
# ALL_USR_LIBS +=
# LIB_EXCLUDE_OSS = yes
# LIB_EXCLUDE_DBG = yes
# ALL_CORE += SMB2/COM/library.mak
# ADDED_CFLAGS +=
# ADDED_KERNEL_CFLAGS =
# ADDED_RTP_CFLAGS =
# ALL_LL_DRIVERS +=
# ALL_BB_DRIVERS += SMBPCI/DRIVER/COM/driver_16z001.mak

ADDED_CFLAGS += -std=c11 -fno-strict-aliasing  -fno-builtin -D__vxworks -D__VXWORKS__ -D__ELF__ -nostdlibinc -nostdinc++ -mllvm -two-entry-phi-node-folding-threshold=2 -D_USE_INIT_ARRAY -fasm  -O2  -w    -D_VX_CPU=_VX_ARMARCH7 -D_VX_TOOL_FAMILY=llvm -D_VX_TOOL=llvm -DARMEL -DARMEL -D_WRS_LIBC_STD_BUILD -DINET -D_WRS_KERNEL
ADDED_CFLAGS += -DTOOL_FAMILY=${TOOL_FAMILY} -DTOOL=${TOOL}
