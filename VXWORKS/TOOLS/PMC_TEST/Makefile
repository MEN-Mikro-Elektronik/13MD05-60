#***************************  M a k e f i l e  *******************************
#  
#         Author: ts
#          $Date: 2008/01/21 10:36:37 $
#      $Revision: 1.1 $
#  
#    Description: makefile for PMC Test under VxWorks
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: Makefile,v $
#   Revision 1.1  2008/01/21 10:36:37  ts
#   Initial Revision, 1st work checkin
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2008 by MEN mikro elektronik GmbH, Nuremberg, Germany 
#*****************************************************************************

CPU = PPC603
CC  = ccppc

TOOL= gnu

ifeq ( , $(MEN_VX_DIR) )
    MEN_VX_DIR = /work/VXWORKS
endif


#**************************************
#   define used tools
#
COMPILER=$(CC) -Wall -O2 -fvolatile -fno-builtin -fno-for-scope -DCPU=$(CPU)
FLAGS=-DVXWORKS -DMAC_MEM_MAPPED -DPROG_FILE_NAME=pmc_test
#-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)

#**************************************
#   input directories
#

MEN_INC_DIR=$(MEN_VX_DIR)/INCLUDE


#**************************************
#   include pathes
#
INC=-I$(MEN_INC_DIR)/COM     \
    -I$(MEN_INC_DIR)/NATIVE  \
    -I$(WIND_BASE)/target    \
    -I$(WIND_BASE)/target/h  \
    -I.

all: $(OBJ_DIR)/pmc_test.o


#**************************************
#   dependencies and comands
#
$(OBJ_DIR)/pmc_test.o:   pmc_test.c \
						 $(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    	 $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
						 $(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
						 $(MEN_INC_DIR)/COM/MEN/testutil.h \
		            	 Makefile
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
