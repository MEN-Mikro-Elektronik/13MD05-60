#***************************  M a k e f i l e  *******************************
#
#         Author: uf  
#          $Date: 2005/12/23 11:18:11 $
#      $Revision: 1.4 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/MDIS_API/library.mak,v 1.4 2005/12/23 11:18:11 UFRANKE Exp $
#
#    Description: makefile descriptor file for vxWorks 
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.4  2005/12/23 11:18:11  UFRANKE
#   added
#    + mdis_api_sc.c
#
#   Revision 1.3  2000/03/17 15:07:55  kp
#   fixed bad MAK_NAME
#
#   Revision 1.2  1999/08/31 12:14:55  Franke
#   MDIS 4.3
#
#   Revision 1.1  1998/03/10 12:22:14  franke
#   Added by mcvs
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1997 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mdis_api

MAK_LIBS=


MAK_INCL=$(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/mdis_ers.h    \
         $(MEN_INC_DIR)/oss.h         \
         $(MEN_INC_DIR)/desc.h        \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_com.h    \
         $(MEN_VX_INC_DIR)/os2m.h     \


MAK_INP1=mdis_api$(INP_SUFFIX)
MAK_INP2=mdis_errstr$(INP_SUFFIX)
MAK_INP3=mdis_api_sc$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)  \
		$(MAK_INP2)  \
		$(MAK_INP3)  \

