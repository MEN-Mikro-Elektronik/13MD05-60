#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: 2006/03/31 10:26:22 $
#      $Revision: 1.1 $
#  
#    Description: Make whetstone floatingpoint benchmark
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

TOOL 	 	= gnu

# define architectures to build
ifeq ( , $(CPUS) )
	CPUS 	= PPC32 
endif

TARGET	= all
CLEANTARGET = clean
INC_DIRS=-I$(WIND_BASE)/target/h

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
	@$(MAKE) -f makef.mak CPU=$(CPU) TOOL=$(TOOL) $(INC_DIRS) SMEN_DIR=$(SMEN_DIR) \
	 DBGDIR= $(TARGET) >>out
