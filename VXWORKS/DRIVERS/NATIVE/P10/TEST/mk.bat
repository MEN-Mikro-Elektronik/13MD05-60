@echo off
REM ***********************************************************************
REM
REM          Author: rl
REM           $Date: 2002/04/22 14:27:04 $
REM       $Revision: 1.2 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.2  2002/04/22 14:27:04  Franke
REM    cosmetics
REM
REM    Revision 1.1  2000/03/14 14:30:40  loesel
REM    Initial Revision
REM
REM    
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

set TOOL=GNU
set MEN_DIR=
set SMEN_DIR=/work


set DBG=
set DBGDIR=

rem set CPU=I80386

rem set CPU=I80486

rem set CPU=PENTIUM

rem set CPU=PPC403

set CPU=PPC603

rem set CPU=PPC604

rem set CPU=PPC860

rem set CPU=MC68000

%GNUMAKE% -r -f makef.mak   >> out
if errorlevel 1 GOTO FAIL

REM ====== DBG ==========
set DBG= -g -O0
set DBGDIR=test
REM

rem set CPU=I80386

rem set CPU=I80486

rem set CPU=PENTIUM

rem set CPU=MC68000

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403

rem set CPU=PPC603

rem set CPU=PPC604

rem set CPU=PPC860

rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

