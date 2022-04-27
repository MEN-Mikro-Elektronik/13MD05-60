#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2012/07/17 14:49:00 $
#      $Revision: 1.14 $
#
#    Description: Defines MDIS core components
#
#       Switches: MEN_VX_DIR
#		Rules:	  none
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

ifeq ($(PCI_SUPPORT),yes)
	PCI_EXT := _pci
else
	PCI_EXT :=
endif

# determine current directory and configuration name
ifeq ($(WIND_HOST_TYPE),x86-win32)
	cwd := $(shell cmd /c cd)
	cwd := $(subst \,/,$(cwd))
	export ECHO		:=echo
  ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	export WTXTCL	:=tclsh
  else
	export WTXTCL	:=wtxtcl
  endif
else
	cwd := $(shell pwd)
endif

export THIS_DIR		:= $(cwd)

# set root path of dir system
ifndef MEN_VX_DIR
  ERROR:
	@$(ECHO) "*** MEN_VX_DIR missing"
	@$(ECHO) "*** Please set i.e. set MEN_VX_DIR=C:/work/VXWORKS"
else
  ifeq ($(MEN_VX_DIR),)
    ERROR:
	@$(ECHO) "*** MEN_VX_DIR empty"
	@$(ECHO) "*** Please set i.e. set MEN_VX_DIR=C:/work/VXWORKS"
  endif
endif
MEN_VX_DIR			:= $(patsubst %/,%,$(MEN_VX_DIR))

export MEN_VX_DIR
export LL_PATH		:= $(MEN_VX_DIR)/DRIVERS/MDIS_LL
export BB_PATH		:= $(MEN_VX_DIR)/DRIVERS/BBIS
export LS_PATH		:= $(MEN_VX_DIR)/LIBSRC
export TO_PATH		:= $(MEN_VX_DIR)/TOOLS

#-----------------------------------------------------------
# MDIS CORE COMPONENTS (paths relative to VXWORKS/LIBSRC)
#
# if necessary set LIB_EXCLUDE_IO=yes in Makefile if _io.mak variants shall
# not be built for some purpose, currently just used for x86 maximum driver
# build test.
ifeq ($(LIB_EXCLUDE_IO),yes)
	ALL_CORE 	:= \
				MIPIOS/library.mak  			\
				BK/library.mak  				\
				USR_OSS/library.mak 			\
				MDIS_API/library.mak 			\
				DESC/COM/library.mak 			\
				ID/COM/library.mak 				\
				ID/COM/library_sw.mak 			\
				MBUF/COM/library.mak 			\
				PLD/COM/library.mak 			\
				PLD/COM/library_sw.mak 			\
				USR_UTL/COM/library.mak 		\
				CHAMELEON/COM/library.mak 		\
				CHAMELEON/COM/library_sw.mak
else
	ALL_CORE 	:= \
				MIPIOS/library.mak  			\
				BK/library.mak  				\
				USR_OSS/library.mak 			\
				MDIS_API/library.mak 			\
				DESC/COM/library.mak 			\
				ID/COM/library.mak 				\
				ID/COM/library_sw.mak 			\
				MBUF/COM/library.mak 			\
				PLD/COM/library.mak 			\
				PLD/COM/library_sw.mak 			\
				USR_UTL/COM/library.mak 		\
				CHAMELEON/COM/library.mak 		\
				CHAMELEON/COM/library_sw.mak 	\
				CHAMELEON/COM/library_io.mak 	\
				CHAMELEON/COM/library_io_sw.mak
endif


ifeq ($(LIB_EXCLUDE_OSS),yes)
else
	OSS_CORE		:= 	OSS/library.mak \
						OSS/library_pci.mak
endif

ifeq ($(LIB_EXCLUDE_DBG),yes)
else
	DBG_CORE		:=  DBG/library.mak
endif

MK_CORE			:=  MK/library.mak \
					MK/library_pci.mak

#------------------------------------------------------------
# RTP support starts with VxWorks 6.0
#
ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_6_0)
	ifeq ($(RTP_FLAGS), -DINCLUDE_RTP)
		RTP_CORE 		:= MDIS_API_RTP/library.mak	\
						   USR_OSS_RTP/library.mak
	else #RTP_FLAGS != -DINCLUDE_RTP
		RTP_CORE		:=
	endif #RTP_FLAGS != -DINCLUDE_RTP
else ifeq ($(MEN_VXWORKS_ENV_VER),VXWORKS_7_0)
	ifeq ($(RTP_FLAGS), -DINCLUDE_RTP)
		RTP_CORE 		:= MDIS_API_RTP/library.mak	\
						   USR_OSS_RTP/library.mak
	else #RTP_FLAGS != -DINCLUDE_RTP
		RTP_CORE		:=
	endif #RTP_FLAGS != -DINCLUDE_RTP
endif