@echo off
REM ***********************************************************************
REM
REM          Author: ts
REM           $Date: 2007/09/26 17:01:03 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM		
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.1  2007/09/26 17:01:03  ts
REM    Initial Revision
REM
REM   
REM ************************************************************************
@echo "START"   > out

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\
set TOOL=gnu
set MEN_DIR=
set MEN_VX_DIR=s:/work_vxM78/VXWORKS
set CPU=PPC603

%GNUMAKE% -r -f makef.mak >> out
if errorlevel 1 GOTO FAIL
goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

