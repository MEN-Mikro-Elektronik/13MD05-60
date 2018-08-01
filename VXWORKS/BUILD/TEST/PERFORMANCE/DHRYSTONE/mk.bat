make dhry_p1.o
make dhry_p2.o

rem ld386 -o dhry.o -r dhry_p1.o dhry_p2.o
ldppc -o dhry.o -r dhry_p1.o dhry_p2.o
rem ld68k -o dhry.o -r dhry_p1.o dhry_p2.o

