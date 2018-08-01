@echo off
REM ***********************************************************************
REM
REM          Author: ts
REM           $Date: $
REM       $Revision: $
REM
REM     Description: simple script to build M36N calibration Tool
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


cd M36_CALIB
CALL mk.bat
cd ..


REM ## copy the Test tools object file ####
copy S:\work\VXWORKS\LIB\MEN\objPPC603gnu\m36_calib.o  s:\work_vxM78\WFTPD_HOME\

if errorlevel 1 GOTO FAIL
goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

