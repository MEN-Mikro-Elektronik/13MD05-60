
ENDIAN = "set here BIG_ENDIAN or LITTLE_ENDIAN of your host machine"

INLUDE_PATH = ../../../INCLUDE/COM
SRC = ../COM


descgen: $(SRC)/binary.c $(SRC)/csource.c $(SRC)/descgen.c\
		 $(SRC)/os9.c $(SRC)/os9k.c $(SRC)/parse.c \
		 $(SRC)/descgen.h $(SRC)/os9k.h \
		 $(INLUDE_PATH)/MEN/men_typs.h \
		 $(INLUDE_PATH)/MEN/desctyps.h
	cc -I$(INLUDE_PATH) -DVXWORKS -D$(ENDIAN) \
	     $(SRC)/binary.c $(SRC)/csource.c $(SRC)/descgen.c \
	     $(SRC)/os9.c $(SRC)/os9k.c $(SRC)/parse.c -o descgen


