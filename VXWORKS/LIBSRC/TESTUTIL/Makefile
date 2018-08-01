#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/06/09 17:37:15 $
#      $Revision: 1.4 $
#
#    Description: Make Testutil lib
#
#       Switches: -
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************
export _BDGEN_ = yes

TGT_DIR=$(WIND_BASE)/target

ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=/$(WIND_PLATFORM)
endif

#******************
# Sandbox directory
# specify the full path name
# (include also the drive letter if Tornado or project dir is on a different
# drive)
# Better you set this macro externally by defining an environment variable
# with the same name: e.g. MEN_VX_DIR=s:/work/VXWORKS
#
ifeq ( , $(MEN_VX_DIR) )
    # print error message and remove the wrong dependency file
    MEN_VX_DIR = ******__MEN_VX_DIR__missing
    out: Makefile
	@$(ECHO) $(MEN_VX_DIR)
else
endif

TOOL 	 	= gnu

# define architectures to build
ifeq ( , $(CPU) )
	CPUS 	?= MC68040 PPC604 PPC603 PPC85XX I80486 PPC32
else
	CPUS = $(CPU)
endif

TARGET	= all
CLEANTARGET = clean

$(TARGET): clearout $(CPUS)

$(CLEANTARGET):
	$(MAKE) TARGET=clean CLEANTARGET=xxx

clearout:
	@$(RM) out

$(CPUS):
	@$(ECHO) +++ making $@
	@$(MAKE) CPU=$@ arch TARGET=$(TARGET)

arch: $(CPU)$(TOOL) #$(CPU)$(TOOL)test

$(CPU)$(TOOL) $(CPU)$(TOOL)test:
	@$(ECHO) +++++ making $@ $(TARGET)
	@$(MAKE) -f makef.mak CPU=$(CPU) TOOL=$(TOOL) MEN_VX_DIR=$(MEN_VX_DIR) $(TARGET) >>out


