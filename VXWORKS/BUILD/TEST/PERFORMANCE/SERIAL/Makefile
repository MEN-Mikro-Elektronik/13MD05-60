# Makefile - makefile skeleton for demo/vxColor
#
# Copyright (C) 1996 Wind River Systems, Inc.
#
# This file was generated via the Tornado Project Facility.
#

CPU = R4000
CPU = I80486
CPU = MC68040
CPU = PPC603
TOOL = gnu

ifeq (PPC603,$(CPU))
	ADDED_CFLAGS = -Wall -gdwarf -O
else
	ADDED_CFLAGS = -Wall -g -O
endif


MEN_DIR=/work
VX_INC_DIR=$(MEN_DIR)/VXWORKS/INCLUDE/NATIVE
COM_INC_DIR=$(MEN_DIR)/VXWORKS/INCLUDE/COM

EXTRA_INCLUDE = -I. -I$(VX_INC_DIR) -I$(COM_INC_DIR) 

EXTRA_DEFINE  =

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)
include $(WIND_BASE)/target/h/make/rules.bsp
