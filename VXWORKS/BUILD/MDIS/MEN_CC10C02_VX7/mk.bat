@echo Internal test tool for MEN
cls
make clean
make
@REM copy the MDIS object to the BSP work folder for linkage 
@REM object with debugs
copy /Y C:\work\work_VXdeploy\VXWORKS\LIB\MEN\vxworks-6.9_mdis_MEN_CC10C\objARMARCH7gnutest\mdis_MEN_CC10C.o C:\work\work_10CC10-60\vxworks-6.9_mdis_MEN_CC10C\objARMARCH7gnutest\mdis_MEN_CC10C.o
