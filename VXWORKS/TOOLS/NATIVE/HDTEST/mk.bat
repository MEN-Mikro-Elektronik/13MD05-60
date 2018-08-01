@echo off
setlocal
REM ***********************************************************************
REM
REM          Author: kp
REM           $Date: 2006/10/18 14:15:09 $
REM       $Revision: 1.5 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.5  2006/10/18 14:15:09  cs
REM    issue warning when no CPU is specified
REM
REM    Revision 1.4  2006/09/11 10:48:39  cs
REM    added support for VxW 6.x
REM
REM    Revision 1.3  2000/03/28 16:11:58  Franke
REM    replaced SMEN_DIR by MEN_WORK_DIR
REM
REM    Revision 1.2  2000/02/07 13:58:34  Franke
REM    SMEN_DIR now as env. variable
REM
REM    Revision 1.1  2000/01/12 10:56:35  kp
REM    Initial Revision
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************

@echo "START"   > out

if "%WIND_TOOLS%"=="" GOTO VXW_BEFORE_6X
GOTO VXW_6X

:VXW_BEFORE_6X
rem set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
rem set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\
GOTO VXW_ALL

:VXW_6X
set GNUMAKE=make
GOTO VXW_ALL

:VXW_ALL
set TOOL=gnu

if "%1%"=="" GOTO NOCPU
GOTO %1%

rem REM ====== DBG ==========
set DBG=
set DBGDIR=
rem set DBG=-DDBG -DDBG_DUMP -g -O0
rem set DBGDIR=test
rem REM
rem

:68K
rem ========================== MC68K ==================
set CPU=MC68040
  set DBG=
  set DBGDIR=
    %GNUMAKE% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL
rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

set CPU=MC68000
  set DBG=
  set DBGDIR=
    %GNUMAKE% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

goto ENDEND



:X86
rem =========================== X86 ===================
set CPU=I80386
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

set CPU=I80486
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

goto ENDEND

:PPC
rem =========================== PPC ===================
:PPC403
  set CPU=PPC403
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

  if %1%==PPC403  goto ENDEND

:PPC603
  set CPU=PPC603
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

  if %1%==PPC603  goto ENDEND

:PPC604
  set CPU=PPC604
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

  if %1%==PPC604  goto ENDEND

:PPC85XX
  set CPU=PPC85XX
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

  if %1%==PPC85XX  goto ENDEND

:PPC860
  set CPU=PPC860
  set DBG=
  set DBGDIR=
    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
    if errorlevel 1 GOTO FAIL

rem  set DBG=-DDBG -DDBG_DUMP -g -O0
rem  set DBGDIR=test
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem    if errorlevel 1 GOTO FAIL

  if %1%==PPC860  goto ENDEND

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND

:NOCPU
  echo ===========================
  echo "=> no CPU specified"
  echo ===========================

:ENDEND

