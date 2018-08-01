@echo off
REM ***********************************************************************
REM
REM          Author: rl
REM           $Date: 2003/04/22 13:05:36 $
REM       $Revision: 1.2 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.2  2003/04/22 13:05:36  UFranke
REM    PPC603 default
REM
REM    Revision 1.1  2000/03/14 14:35:02  loesel
REM    Initial Revision
REM
REM    Revision 1.1  2000/02/28 15:38:16  loesel
REM    Initial Revision
REM
REM    Revision 1.1  2000/01/14 14:55:11  loesel
REM    Initial Revision
REM
REM    Revision 1.2  2000/01/04 09:20:35  loesel
REM    makefile for objects with and without debug messages
REM
REM    Revision 1.1  1999/12/17 13:12:38  loesel
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

set TOOL=GNU
set MEN_DIR=
set SMEN_DIR=/work

rem REM ====== DBG ==========
rem set DBG=-DDBG -DDBG_DUMP
rem rem -g -O0
rem set DBGDIR=test
rem REM
rem
rem set CPU=I80486
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL
rem
rem goto ENDEND




set DBG=
set DBGDIR=

rem set CPU=I80386


rem set CPU=I80486


rem set CPU=PPC403


set CPU=PPC603


rem set CPU=PPC604


rem set CPU=PPC860

rem set CPU=MC68000

%GNUMAKE% -r -f makef.mak   >> out
if errorlevel 1 GOTO FAIL

REM ====== DBG ==========
REM set DBG= -g -O0
REM set DBGDIR=test
REM

rem set CPU=I80386

REM set CPU=I80486

rem set CPU=MC68000

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403

set CPU=PPC603

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

