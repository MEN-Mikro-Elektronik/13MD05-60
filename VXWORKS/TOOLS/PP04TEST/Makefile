# native Makefile for mvbtest 
# build under Win2000 / Tornado
#
# use -DMVB_SWAP_BYTES to perform separate PCI host bridge swapping
# use -DDBG_CL11 to obtain debugprints from the CL1_1 stack

PROGNAME=mvbtest

CCDEFS += -O2 -Wall -DMVB_SWAP_BYTES  #-DDBG_CL11

VXW_INC_BASE = $(WIND_BASE)/target/h
CCDEFS += -DVXWORKS
#set to desired directory
INST_DIR=.
CC = ccppc $(CCDEFS) -I$(VXW_INC_BASE)/
LD = ldppc

all: $(PROGNAME)
	cp $(PROGNAME) $(INST_DIR)

.o:.c

clean:
	rm -f *.o $(PROGNAME)

$(PROGNAME):cl1_1.o host_dep.o mvbcomm.o 
	$(LD) -r host_dep.o cl1_1.o mvbcomm.o -o $(PROGNAME) 

