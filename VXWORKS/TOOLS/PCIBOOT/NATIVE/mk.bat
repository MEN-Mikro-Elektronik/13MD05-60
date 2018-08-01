@echo off
REM ***********************************************************************
REM
REM          Author: rt
REM           $Date: 2007/10/02 13:16:32 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM          Syntax: mk 
REM ---------------------------------[ History ]----------------------------
REM
REM $Log: mk.bat,v $
REM Revision 1.1  2007/10/02 13:16:32  rtrübenbach
REM Initial Revision
REM
REM
REM
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 2007 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************

@echo START BUILDING (see "out.txt" for details)...
@echo START BUILDING...   > out.txt

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=make
rem set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=GNU
set TOOL_FAMILY=GNU

REM ====== DBG on/off ==========
set DBG=
set DBGDIR=

rem set DBG=-DDBG
rem set DBGDIR=

rem set DBG=-DDBG -DDBG_DUMP -g -O0
rem set DBGDIR=test

REM ====== MEN_BOARD_LOWERCASE ==========
REM set MEN_BOARD_LOWERCASE=%1 
REM we just support EP04 at the moment
set MEN_BOARD_LOWERCASE=ep04
:BOARD
if "%MEN_BOARD_LOWERCASE%"=="" goto ASK

REM ====== CPU ==========
set CPU=PENTIUM4

REM ====== start make ==========
%GNUMAKE% -r -f makef.mak   >> out.txt
 if errorlevel 1 GOTO FAIL
 @echo ...OK
 @echo ...OK   > out.txt
goto ENDEND

REM ====== ask for MEN_PREFIX ==========
:ASK
  set /p MEN_BOARD_LOWERCASE=Please enter board version (e.g. generic, ep04):
goto BOARD

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND

:ENDEND

