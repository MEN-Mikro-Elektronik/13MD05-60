#***************************  M a k e f i l e  *******************************
#  
#         Author: kp
#          $Date: 2005/05/04 15:11:10 $
#      $Revision: 1.5 $
#  
#    Description: Make mtest
#                      
#       Switches: -
#                      
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

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
