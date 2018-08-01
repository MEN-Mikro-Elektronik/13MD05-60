#***************************  M a k e f i l e  *******************************
#  
#         Author: kp
#          $Date: 2000/03/17 15:08:38 $
#      $Revision: 1.1 $
#  
#    Description: Retrieve MAK_NAME of a common makefile
#                      
#       Switches: COMMAKE		
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
#*****************************************************************************

include $(COMMAKE)


.PHONY: all

all: 
	MAK_NAME=$(MAK_NAME)
