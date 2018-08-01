# Makefile for UGL.
#
# modification history
# --------------------
# $Log: Makefile,v $
# Revision 1.2  2006/10/15 16:43:34  cs
# Updated for WindMl 4.2
#
#
# Windriver History
# --------------------
# 01e,30mar05,edc  Updated Makefile for WindML 4.0
# 01d,28sep00,jlb  Moved rules.windml to h/ugl/make
# 01c,15aug00,jlb  New make targets for WindML
# 01b,09feb99,jcs  removing docs generation.
# 01a,10sep98,ics  created

ifneq (,$(findstring 6.2,$(WIND_PLATFORM)))
	C_SOURCE = $(wildcard *.c)
else
	SUBDIRS  =

	ifeq ($(LIB_TYPE),DYNAMIC)
		C_SOURCE = $(wildcard *.c)
	else
		C_SOURCE =
	endif
endif

DOC_FILES =

OBJS = $(C_SOURCE:.c=.o)

EXTRA_INCLUDE = -I$(WINDML_BASE)/src/ugl/config

include $(WINDML_BASE)/h/ugl/make/rules.windml
