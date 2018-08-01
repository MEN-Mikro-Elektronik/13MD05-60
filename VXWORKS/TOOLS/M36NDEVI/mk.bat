@echo off
REM ***********************************************************************
REM
REM          Author: many
REM           $Date: 2007/08/21 13:09:40 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM		here for MTEST2.o VME test with block access
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.1  2007/08/21 13:09:40  ts
REM    Initial Revision
REM
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************
@echo "START"   > out

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=gnu
set MEN_DIR=
set MEN_VX_DIR=s:/work/VXWORKS
set CPU=PPC603

%GNUMAKE% -r -f makef.mak   >> out
if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

