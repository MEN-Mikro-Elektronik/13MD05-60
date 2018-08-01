#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/09/11 10:51:37 $
#      $Revision: 1.7 $
#
#    Description: Make mtest
#
#       Switches: -
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

TGT_DIR=$(WIND_BASE)/target

ifeq ($(WIND_TOOLS),)
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=
else
    include $(TGT_DIR)/h/make/defs.default
    include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
    MEN_VXVERSUB_DIR=/$(WIND_PLATFORM)
endif

ifeq ( , $(MEN_VX_DIR) )
    MEN_VX_DIR = s:/work/VXWORKS
endif

TOOL 	 	= gnu


# define architectures to build
ifeq ( , $(CPUS) )
	CPUS 	= PPC604 #PPC603 PPC85XX #I80486
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



arch: $(CPU)$(TOOL)


$(CPU)$(TOOL):
	@$(ECHO) +++++ making $@ $(TARGET)
	@$(MAKE) -f makef.mak CPU=$(CPU) TOOL=$(TOOL) SMEN_DIR=$(SMEN_DIR) \
	 DBGDIR= $(TARGET) >>out
	@$(MAKE) -f makef_blk.mak CPU=$(CPU) TOOL=$(TOOL) SMEN_DIR=$(SMEN_DIR) \
	 DBGDIR= $(TARGET) >>out
