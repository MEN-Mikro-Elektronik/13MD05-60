@echo Internal test wrapper for MEN to build object and copy to used VIP folder at once - ADAPT TO OWN FOLDER!
cls
make clean
make
@REM copy the MDIS object to the BSP work folder for linkage
@REM object with debugs
copy /Y c:\work\work_VXdeploy\VXWORKS\LIB\MEN\vxworks-6.9_mdis_MEN_CC10C00\objARMARCH7gnutest\mdis_MEN_CC10C00.o c:\work\10CC10-60\vxworks-6.9_mdis_MEN_CC10C00\objARMARCH7gnutest\mdis_MEN_CC10C00.o
