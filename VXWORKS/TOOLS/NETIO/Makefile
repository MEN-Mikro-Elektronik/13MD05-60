# Makefile for NETIO
#
# Author:  Kai Uwe Rommel <rommel@ars.de>
# Created: Wed Sep 25 1996

vxworks:
	$(MAKE) all CC="ccppc -O -s " O=.o X= \
	CFLAGS="-fdollars-in-identifiers -mcpu=604 -mhard-float -mstrict-align -fno-implicit-fp -ansi  -O2 -fvolatile -fno-builtin -Wall -DCPU=PPC32 -DTOOL_FAMILY=gnu -DTOOL=gnu -D_WRS_KERNEL -DCPU_VARIANT=_ppc604 -DMAC_MEM_MAPPED -DINET -UCPU -DCPU=PPC604 -DVXWORKS -D_VSB_CONFIG_FILE=\"/opt/WindRiver/vxw68/vxworks-6.8/target/lib/h/config/vsbConfig.h\"   -fno-zero-initialized-in-bss -G8 -msdata=eabi" 

INC_BASE = $(WIND_BASE)/target/usr/h
INC = -I. -I$(INC_BASE)/ -I$(INC_BASE)/wrn/coreip/ 

all: netio$X
# netbios$O getopt$O

netio$X: netio$O 
	$(CC) $(OUT) $@ netio$O $(LFLAGS) $(LIBS)

.SUFFIXES: .c $O
.c$O:
	$(CC) $(CFLAGS) $(INC) -c $*.c

netio$O: netio.c 

#tschnuer@tslinux2:~/work/VXWORKS/TOOLS/NETIO$ find /opt/WindRiver/vxw68/vxworks-6.8/ -name "socket\.h"
#/opt/WindRiver/vxw68/vxworks-6.8/target/h/wrn/coreip/sys/socket.h
#/opt/WindRiver/vxw68/vxworks-6.8/target/h/wrn/coreip/socket.h
#/opt/WindRiver/vxw68/vxworks-6.8/target/usr/h/wrn/coreip/sys/socket.h
#tschnuer@tslinux2:~/work/VXWORKS/TOOLS/NETIO$ 
#tschnuer@tslinux2:~/work/VXWORKS/TOOLS/NETIO$ 
#tschnuer