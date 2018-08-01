# Makefile - makefile for AE52 system test
#


CPU              = PPC32
#optional soft-float kernel 
TOOL             = diab

#for loadable objects
ifeq ($(MAKECMDGOALS),_ray_make) 
	SDA_DISABLE = DISABLE 
	ADDED_CFLAGS=-Xcode-absolute-far -D_WRS_CONFIG_SMP
endif

#Support for 36 bit physical addresses in the MMU can be added for 
#vxWorks 6.2 and later with the following CPU_VARIANT.
CPU_VARIANT      = _ppc85XX_e500mc

TGT_DIR = $(WIND_BASE)/target

include $(TGT_DIR)/h/make/defs.bsp

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.

TARGET_DIR	= ./
VENDOR		= MEN
BOARD		= men_xm51

#RELEASE		+= bootrom_uncmp.hex bootrom_uncmp.bin 

CONFIG_ALL = ../all_p4080_ds

## Until GCC supports e500mc
RELEASE_PRJ     = prj_diab

#EXTRA_DEFINE    = -D_BSP_TEST -DVXWORKS -DVX_WORKS -DAE52 
EXTRA_DEFINE    += -DVXWORKS -DVX_WORKS -DAE52 
#EXTRA_DEFINE    += -DUSB_EHCD_ENABLE_POLLING

MEN_VX_DIR = ../../..
MEN_VX_COM_DIR =    $(MEN_VX_DIR)/INCLUDE/COM/
MEN_VX_NATIVE_DIR = $(MEN_VX_DIR)/INCLUDE/NATIVE/

EXTRA_INCLUDE += -I$(MEN_VX_NATIVE_DIR) -I$(MEN_VX_COM_DIR)
#EXTRA_INCLUDE += -I$(WIND_BASE)/target/src/hwif/busCtlr/usb/hcd/ehcd

#EXTRA_INCLUDE += -I.
#EXTRA_INCLUDE += -I./DDC/libprj/include
#EXTRA_INCLUDE += -I./DDC/ddcm
#EXTRA_INCLUDE += -I./DDC/flash/vxworks
#EXTRA_INCLUDE += -I./DDC/flash
#EXTRA_INCLUDE += -I./DDC/drivers/ddc
#EXTRA_INCLUDE += -I./DDC/drivers/ddc
#EXTRA_INCLUDE += -I./DDC/utils

## The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
## in config.h and Makefile. All definitions for these constants must be
## identical.

ROM_TEXT_ADRS    = fff00100 # ROM entry address
ROM_SIZE         = 00100000 # number of bytes of ROM space
ROM_WARM_ADRS    = fff00108 # ROM warm entry adrdress 
ROM_LINK_ADRS	 = fff00000
RAM_LOW_ADRS     = 00100000 # RAM text/data address
RAM_HIGH_ADRS    = 01000000 # RAM text/data address

LOCAL_MEM_LOCAL_ADRS = 0x00000000

TFTP_DIR = /tftpboot/XM51
SWS_TFTP_DIR = /mnt/swsdisk/users/agftp/XM51
MACH_RAYTHEON = ml605.o systemtest.o eth_looptest.o #sertest.o
#MACH_RAYTHEON += systemtest_AE52.o

_ray_clean:
	for p in $(MACH_RAYTHEON); do rm -f $$p; done

_ray_make: _ray_clean $(MACH_RAYTHEON) 
	@echo "Raytheon specific files..."
	for p in $(MACH_RAYTHEON); do cp $$p $(TFTP_DIR); done
#	for p in $(MACH_RAYTHEON); do cp $$p $(SWS_TFTP_DIR); done
	#cp $(MEN_VX_DIR)/vxworks-6.8/VXW68_RAYTHEON_DDC/PPC32e500v2diab/VXW68_RAYTHEON_DDC_partialImage/Debug/Objects/VXW68_RAYTHEON_DDC/MsgTest.o $(TFTP_DIR)/

	
BOOT_EXTRA       = 
LIB_EXTRA        =	$(MEN_VX_DIR)/LIB/MEN/vxworks-6.8_mdis_MEN_SC23/libPPC85XXgnutest/mdis_all.a 
#LIB_EXTRA        +=	$(WIND_BASE)/target/lib/ppc/PPC32/common/libddc_drv_vxb.a
#LIB_EXTRA        += $(MEN_VX_DIR)/vxworks-6.8/target/config/men_xm51/DDC/library/PPC32diab/libDdcAcex.a


# LDFLAGS     = -tPPCE500MCFH:vxworks68 -X -N -Xgenerate-paddr -m > xxx.map

#VMA_START        = 0x0
#USR_ENTRY        = usrInit
## Only redefine make definitions above this point, or the expansion of 
## makefile target dependencies may be incorrect.

include $(TGT_DIR)/h/make/rules.bsp
#include $(TGT_DIR)/h/make/rules.$(WIND_HOST_TYPE)

