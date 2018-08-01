#***************************  M a k e f i l e  *******************************
#
#         Author: uf
#          $Date: 2006/10/18 13:44:01 $
#      $Revision: 1.3 $
#        $Header: /dd2/CVSR/VXWORKS/LIBSRC/USR_OSS_RTP/library.mak,v 1.3 2006/10/18 13:44:01 cs Exp $
#
#    Description: makefile descriptor file for vxWorks
#                 librarys           e.g. oss
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: library.mak,v $
#   Revision 1.3  2006/10/18 13:44:01  cs
#   removed MAK_NAME extension "_rtp", is set in rtp_component.mak now
#
#   Revision 1.2  2006/06/02 11:13:12  ufranke
#   added
#    + usr_oss_signal_rtp.c
#    + dbg_rtp.c
#
#   Revision 1.1  2005/12/23 15:23:29  UFRANKE
#   Initial Revision
#
#   Revision 1.1  2005/12/23 11:17:42  UFRANKE
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=libusr_oss

MAK_INCL=$(MEN_INC_DIR)/men_typs.h   	\
         $(MEN_INC_DIR)/usr_oss.h    	\
         $(MEN_INC_DIR)/usr_err.h    	\
         $(MEN_INC_DIR)/../../NATIVE/MEN/dbg_vx_rtp.h


MAK_INP=usr_oss_rtp$(INP_SUFFIX)		\
        dbg_rtp$(INP_SUFFIX)            \
        usr_oss_dl_list_rtp$(INP_SUFFIX)\
        usr_oss_errstr_rtp$(INP_SUFFIX) \
        usr_oss_signal_rtp$(INP_SUFFIX) \
        usr_oss_random_rtp$(INP_SUFFIX)

