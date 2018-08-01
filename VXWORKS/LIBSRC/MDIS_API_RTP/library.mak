#***************************  M a k e f i l e  *******************************
#
#         Author: uf
#          $Date: 2006/10/18 13:40:09 $
#      $Revision: 1.4 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/MDIS_API_RTP/library.mak,v 1.4 2006/10/18 13:40:09 cs Exp $
#
#    Description: makefile descriptor file for vxWorks
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.4  2006/10/18 13:40:09  cs
#   removed MAK_NAME extension "_rtp", is set in rtp_component.mak now
#
#   Revision 1.3  2006/07/25 15:37:19  ufranke
#   added
#    + mdis_errstr_rtp.c
#
#   Revision 1.2  2005/12/23 15:46:54  UFRANKE
#   cosmetics
#
#   Revision 1.1  2005/12/23 11:17:42  UFRANKE
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=libmdis

MAK_INCL=$(MEN_INC_DIR)/men_typs.h   	\
         $(MEN_INC_DIR)/mdis_api.h    	\
         $(MEN_VX_INC_DIR)/mdis_rtp.h   \


MAK_INP1=mdis_api_rtp$(INP_SUFFIX)
MAK_INP2=mdis_errstr_rtp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) $(MAK_INP2)

