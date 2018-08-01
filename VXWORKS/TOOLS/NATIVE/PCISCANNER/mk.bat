@echo off
REM ***********************************************************************
REM
REM          Author: kp
REM           $Date: 2001/12/04 10:48:38 $
REM       $Revision: 1.2 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.2  2001/12/04 10:48:38  Franke
REM    cosmetics
REM
REM    Revision 1.1  2000/03/22 15:40:38  loesel
REM    Initial Revision
REM
REM    
REM
REM    
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************

@echo "START"   > out

rem set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
rem set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=GNU
set MEN_DIR=
rem set MEN_WORK_DIR=/work

rem REM ====== DBG ==========
rem set DBG=-DDBG -DDBG_DUMP
rem rem -g -O0
rem set DBGDIR=test
rem REM
rem
rem set CPU=I80486
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL
rem
rem goto ENDEND




set DBG=
set DBGDIR=

rem set CPU=I80386

rem set CPU=PENTIUM

rem set CPU=PPC403

set CPU=PPC603

rem set CPU=PPC604

rem set CPU=PPC860

rem set CPU=MC68000

%GNUMAKE% -r -f makef.mak   >> out
if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

