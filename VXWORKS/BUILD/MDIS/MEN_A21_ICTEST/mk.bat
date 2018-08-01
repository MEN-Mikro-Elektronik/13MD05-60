cls
make clean
make

@echo off@
echo "copying to %WIND_PLATFORM% folder:"
copy /Y c:\work\work_VXWORKS\LIB\MEN\%WIND_PLATFORM%_mdis_MEN_A21_ICTEST\objPPC32e500v2gnutest\mdis_MEN_A21_ICTEST.o C:\work\work_10A021-60\%WIND_PLATFORM%_mdis_MEN_A21_ICTEST\objPPC32e500v2gnutest\
