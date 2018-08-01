#***************************  M a k e f i l e  *******************************
#  
#         Author: sv
#          $Date: 2007/12/12 15:26:47 $
#      $Revision: 1.1 $
#  
#    Description: makefile 
#                      
#---------------------------------[ History ]---------------------------------
#
#   $Log: Makefile,v $
#   Revision 1.1  2007/12/12 15:26:47  ts
#   Initial Revision, Test P699 PCIe Transfers and DMA
#
#   Revision 1.1  2006/04/04 15:54:03  SVogel
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
#*****************************************************************************
CPU = PENTIUM4

CC  = ccpentium
LD  = ldpentium


TOOL= gnu

ifeq ( , $(MEN_VX_DIR) )
    MEN_VX_DIR = s:/work/VXWORKS
endif

#**************************************
#   define used tools
#
COMPILER=$(CC) -Wall -O2 -fvolatile -fno-builtin -DCPU=$(CPU)
FLAGS=-DVXWORKS -DMAC_MEM_MAPPED -DPROG_FILE_NAME=p699_test
#-DDBG

#**************************************
#   output directories
#

OBJ_DIR=$(MEN_VX_DIR)/LIB/MEN/obj$(CPU)$(TOOL)$(DBGDIR)
MEN_LIB_DIR=$(MEN_WORK_DIR)/VXWORKS/LIB/MEN/lib$(CPU)$(TOOL)$(DBGDIR)

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
    -I$(WIND_BASE)/target/usr/h \
	-I$(WIND_BASE)/target/h/wrn/coreip/  \
	-I.

all: $(OBJ_DIR)/p699_test.o

#**************************************
#   dependencies and commands
#
$(OBJ_DIR)/p699_test.o:  p699_test.c \
					$(MEN_INC_DIR)/COM/MEN/men_typs.h    \
				    $(MEN_INC_DIR)/COM/MEN/mdis_api.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_oss.h	  \
					$(MEN_INC_DIR)/COM/MEN/usr_utl.h	  \
					$(MEN_INC_DIR)/COM/MEN/testutil.h \
                     Makefile
	$(COMPILER) $(FLAGS)  $(DBG) $(INC) $(DEF) $(ACCESS) -c $< -o $@
