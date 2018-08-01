# Makefile - makefile skeleton for mem_perf.c
#
# Copyright (C) 1996 Wind River Systems, Inc.
#
# This file was generated via the Tornado Project Facility.
#

CPU = PPC603
CPU	= PPC85XX

# call: make PPC604/membench.o CPU=PPC604 TOOL=gnu
CPU = PPC604

TOOL= gnu

TARGET = membench
ADDED_CFLAGS = -O2 -I../../../../INCLUDE/COM -I../../../../INCLUDE/NATIVE 

include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)

$(CPU)/membench.o: membench.o membench_ppc.o
	$(LD) -r -o $@ membench.o membench_ppc.o

membench_ppc.o: membench_ppc.s
	$(CC) $(CFLAGS) -xassembler-with-cpp -o $@ $? 
