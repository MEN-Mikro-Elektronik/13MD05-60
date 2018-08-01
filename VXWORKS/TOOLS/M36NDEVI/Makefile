#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: 2007/08/21 13:09:34 $
#      $Revision: 1.1 $
#  
#    Description: Make m36ndevi tool to measure standard deviation of ADC data
#                      
#       Switches: -
#                      
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

ifeq ( , $(MEN_VX_DIR) )
    MEN_VX_DIR = /home/tschnuer/work/VXWORKS
endif

TOOL = gnu

# define architectures to build
ifeq ( , $(CPUS) )
	CPUS 	= PPC603 #PPC85XX #I80486
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
