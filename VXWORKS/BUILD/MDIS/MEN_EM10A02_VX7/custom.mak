# custom.mak:
CUSTOM_MAK_MSG = custom.mak has been read successfully
#  - please edit this files to add something not covered but removed by mdiswizvx
#
#i.e.
EXCLUDE_RTP = no
# ALL_COM_TOOLS +=
# ALL_LL_TOOLS +=
# ALL_NATIVE_TOOLS +=
# ALL_USR_LIBS +=
# LIB_EXCLUDE_OSS = yes
# LIB_EXCLUDE_DBG = yes
# ALL_CORE += SMB2/COM/library.mak
ADDED_CFLAGS += -DMK_NUM_FILES=200 -DMEN_EM10A
ADDED_KERNEL_CFLAGS = -D_WRS_KERNEL
ADDED_RTP_CFLAGS = -D__RTP__
# ALL_LL_DRIVERS +=
# ALL_BB_DRIVERS += SMBPCI/DRIVER/COM/driver_16z001.mak


CC_CODE_OPTIONS = -std=c11 -fno-strict-aliasing -fno-builtin -fasm -O2 -w
CC_CPU_OPTIONS  = -mcpu=603e -DCPU=_VX_PPC603 -DCPU_VARIANT=__83xx -DPPC_83xx \
                  -D__ppc -D__ppc__
CC_OS_OPTIONS   = -D__vxworks -D__VXWORKS__ -D__ELF__ -D_USE_INIT_ARRAY \
                  -D_WRS_HARDWARE_FP -D_WRS_LIBC_STD_BUILD -DINET
CC_TOOL_OPTIONS = -D_VX_TOOL_FAMILY=gnu -D_VX_TOOL=gnu

ADDED_CFLAGS += $(CC_CODE_OPTIONS) $(CC_CPU_OPTIONS) \
                $(CC_OS_OPTIONS) $(CC_TOOL_OPTIONS)
ADDED_CFLAGS += -DTOOL_FAMILY=${TOOL_FAMILY} -DTOOL=${TOOL}
