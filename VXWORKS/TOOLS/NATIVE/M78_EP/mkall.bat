@echo off
REM ***********************************************************************
REM
REM          Author: ts
REM           $Date: 2007/09/26 17:01:11 $
REM       $Revision: 1.1 $
REM
REM     Description: simple script to make all single M78 End tests
REM		
REM ************************************************************************
@echo "START"   > out
REM %GNUMAKE% -r -f ./A4270_TEST1/makef.mak   >> out

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\
set TOOL=gnu
set MEN_DIR=
set MEN_VX_DIR=s:/work_vxM78/VXWORKS
set CPU=PPC603

REM FOR %%S IN (A4270_DRV_TEST A4270_IO A4270_TEST1 A4270_TEST2 A4270_TRIGAB) DO DIR /%%S


cd A4270_DRV_TEST
CALL mk.bat
cd ..
cd A4270_IO
CALL mk.bat
cd ..
cd A4270_TEST1
CALL mk.bat
cd ..
cd A4270_MEM
CALL mk.bat
cd ..
cd A4270_TRIGAB
CALL mk.bat
cd ..

REM ## copy the MDIS object file ####
copy S:\work_vxM78\VXWORKS\LIB\MEN\vxworks-6.2_mdis_M78_EP\objPPC603gnutest\mdis_M78_EP.o  s:\work_vxM78\WFTPD_HOME\

REM ## copy the Test tools object files ####
copy S:\work_vxM78\VXWORKS\LIB\MEN\objPPC603gnu\*.o  s:\work_vxM78\WFTPD_HOME\

if errorlevel 1 GOTO FAIL
goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

